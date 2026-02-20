#!/usr/bin/env python3
"""
cpp_ast_tool.py — Semantic C++ codebase analyzer using libclang.

Extracts class APIs, inheritance hierarchies, call graphs, free functions,
namespaces, and detects unused classes/symbols. Designed as a foundation
for LLM coding agents.

Usage:
    python cpp_ast_tool.py <path> [options]

Options:
    --format json|summary|api   Output format (default: api)
    -I, --include-dir DIR       Add include search path (repeatable)
    --find-unused               Report potentially unused classes
    --query QUERY               Query mode: "class X", "inherits Y", "file Z", "calls X"
    --compile-args ...          Additional compiler arguments
    --headers-only              Only parse header files (.h, .hpp, .hxx)
    --no-private                Hide private members from output

Environment:
    LIBCLANG_PATH               Path to libclang shared library
                                (default: auto-detect)

Examples:
    # Analyze a project (auto-detects include dirs)
    python cpp_ast_tool.py ./myproject

    # Explicit include paths
    python cpp_ast_tool.py src -I include -I third_party/include

    # Query specific class
    python cpp_ast_tool.py include -I include --query "class IRGenerator"

    # Find unused classes
    python cpp_ast_tool.py src -I include --find-unused

    # JSON export for programmatic use
    python cpp_ast_tool.py include -I include --format json
"""

import json
import sys
import os
import re
from pathlib import Path
from dataclasses import dataclass, field, asdict
from typing import Optional
from collections import defaultdict

# ── libclang setup ───────────────────────────────────────────────────────────

def _setup_libclang():
    """Configure libclang path from environment or auto-detect."""
    from clang.cindex import Config

    libclang_path = os.environ.get("LIBCLANG_PATH")
    if libclang_path:
        if os.path.isfile(libclang_path):
            Config.set_library_file(libclang_path)
        elif os.path.isdir(libclang_path):
            Config.set_library_path(libclang_path)
        else:
            print(f"Warning: LIBCLANG_PATH '{libclang_path}' not found", file=sys.stderr)

    # Try platform-specific auto-detection if not set
    if not libclang_path:
        candidates = []
        if sys.platform == "win32":
            # Check common Windows locations
            for base in [Path("."), Path("./extern")]:
                candidates.extend(base.glob("**/libclang.dll"))
            # LLVM install paths
            for drive in ["C:", "D:"]:
                candidates.extend(Path(f"{drive}/Program Files/LLVM/bin").glob("libclang.dll"))
        elif sys.platform == "darwin":
            candidates.extend(Path("/opt/homebrew/opt/llvm/lib").glob("libclang.dylib"))
            candidates.extend(Path("/usr/local/opt/llvm/lib").glob("libclang.dylib"))
        else:
            candidates.extend(Path("/usr/lib").glob("**/libclang*.so*"))
            candidates.extend(Path("/usr/lib64").glob("**/libclang*.so*"))

        for c in candidates:
            if c.exists():
                Config.set_library_file(str(c))
                break

_setup_libclang()

from clang.cindex import (
    Index, CursorKind, AccessSpecifier, TranslationUnit, Cursor, TypeKind
)


# ── Data Models ──────────────────────────────────────────────────────────────

@dataclass
class ParamInfo:
    name: str
    type: str

@dataclass
class MethodInfo:
    name: str
    return_type: str
    params: list[ParamInfo]
    access: str  # public, protected, private
    is_virtual: bool = False
    is_pure_virtual: bool = False
    is_override: bool = False
    is_static: bool = False
    is_const: bool = False
    is_noexcept: bool = False
    is_deleted: bool = False
    is_defaulted: bool = False
    brief_comment: str = ""
    attributes: list[str] = field(default_factory=list)
    file: str = ""
    line: int = 0

@dataclass
class FieldInfo:
    name: str
    type: str
    access: str
    is_static: bool = False
    default_value: str = ""
    file: str = ""
    line: int = 0

@dataclass
class EnumValue:
    name: str
    value: Optional[int] = None

@dataclass
class EnumInfo:
    name: str
    qualified_name: str
    is_scoped: bool  # enum class
    underlying_type: str
    values: list[EnumValue]
    file: str = ""
    line: int = 0

@dataclass
class ClassInfo:
    name: str
    qualified_name: str
    kind: str  # class, struct
    bases: list[str]
    methods: list[MethodInfo]
    fields: list[FieldInfo]
    nested_classes: list[str] = field(default_factory=list)
    brief_comment: str = ""
    is_abstract: bool = False
    is_template: bool = False
    template_params: list[str] = field(default_factory=list)
    file: str = ""
    line: int = 0

@dataclass
class FunctionInfo:
    """Free (non-member) function."""
    name: str
    qualified_name: str
    return_type: str
    params: list[ParamInfo]
    is_template: bool = False
    template_params: list[str] = field(default_factory=list)
    brief_comment: str = ""
    file: str = ""
    line: int = 0

@dataclass
class TypeAlias:
    name: str
    underlying_type: str
    file: str = ""
    line: int = 0

@dataclass
class NamespaceInfo:
    name: str
    qualified_name: str
    classes: list[str] = field(default_factory=list)
    functions: list[str] = field(default_factory=list)
    enums: list[str] = field(default_factory=list)

@dataclass
class SymbolReference:
    """A reference to a symbol from somewhere in the codebase."""
    symbol: str
    kind: str  # 'type_ref', 'member_ref', 'call', 'base_class', 'instantiation'
    from_file: str
    from_line: int


# ── AST Walker ───────────────────────────────────────────────────────────────

class CppAstExtractor:
    def __init__(self, compile_args: list[str] = None, project_root: str = "."):
        self.index = Index.create()
        self.compile_args = compile_args or ["-std=c++20", "-x", "c++"]
        self.project_root = Path(project_root).resolve()

        self.classes: dict[str, ClassInfo] = {}
        self.enums: dict[str, EnumInfo] = {}
        self.functions: dict[str, FunctionInfo] = {}
        self.type_aliases: dict[str, TypeAlias] = {}
        self.namespaces: dict[str, NamespaceInfo] = {}
        self.references: list[SymbolReference] = []
        self.files_parsed: list[str] = []
        self._resolved_files: set[str] = set()  # dedup
        self._reference_targets: set[str] = set()
        self._error_count = 0
        self._warning_count = 0

    def parse_file(self, filepath: str):
        """Parse a single translation unit."""
        filepath = str(Path(filepath).resolve())
        if filepath in self._resolved_files:
            return
        self._resolved_files.add(filepath)

        tu = self.index.parse(
            filepath,
            args=self.compile_args,
            options=(
                    TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD |
                    TranslationUnit.PARSE_SKIP_FUNCTION_BODIES
            )
        )

        # Report diagnostics
        errors = [d for d in tu.diagnostics if d.severity >= 3]
        warnings = [d for d in tu.diagnostics if d.severity == 2]
        self._error_count += len(errors)
        self._warning_count += len(warnings)

        if errors:
            print(f"  ⚠ {len(errors)} error(s) in {Path(filepath).name}:", file=sys.stderr)
            for d in errors[:3]:
                print(f"    {d}", file=sys.stderr)

        self.files_parsed.append(filepath)
        self._walk(tu.cursor, filepath)

    def parse_directory(self, dirpath: str, extensions=None):
        """Parse all C++ files in a directory."""
        if extensions is None:
            extensions = (".h", ".hpp", ".hxx", ".cpp", ".cxx", ".cc")
        dirpath = Path(dirpath)
        files = sorted(f for f in dirpath.rglob("*") if f.suffix in extensions)
        print(f"Found {len(files)} C++ files in {dirpath}", file=sys.stderr)
        for f in files:
            print(f"  Parsing {f.name}...", file=sys.stderr)
            self.parse_file(str(f))

        # Summary
        print(f"\n  ── Parsed {len(self.files_parsed)} files: "
              f"{len(self.classes)} classes, {len(self.enums)} enums, "
              f"{len(self.functions)} free functions", file=sys.stderr)
        if self._error_count:
            print(f"  ── {self._error_count} parse errors "
                  f"(try adding -I flags for include directories)", file=sys.stderr)

    def _is_in_project(self, cursor: Cursor) -> bool:
        """Check if a cursor location is within our project."""
        if not cursor.location.file:
            return True  # translation unit root
        loc = str(Path(str(cursor.location.file)).resolve())
        # Accept if in any of our parsed files OR under project root
        return (loc in self._resolved_files or
                loc.startswith(str(self.project_root)))

    def _walk(self, cursor: Cursor, source_file: str):
        """Recursively walk the AST."""
        if not self._is_in_project(cursor):
            return

        kind = cursor.kind

        if kind in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL, CursorKind.CLASS_TEMPLATE):
            self._extract_class(cursor)
        elif kind == CursorKind.ENUM_DECL:
            self._extract_enum(cursor)
        elif kind in (CursorKind.TYPE_ALIAS_DECL, CursorKind.TYPEDEF_DECL):
            self._extract_type_alias(cursor)
        elif kind == CursorKind.NAMESPACE:
            self._extract_namespace(cursor)
        elif kind in (CursorKind.FUNCTION_DECL, CursorKind.FUNCTION_TEMPLATE):
            # Only free functions (not methods)
            if not cursor.semantic_parent or cursor.semantic_parent.kind in (
                    CursorKind.TRANSLATION_UNIT, CursorKind.NAMESPACE
            ):
                self._extract_function(cursor)
        elif kind == CursorKind.TYPE_REF:
            self._record_reference(cursor, 'type_ref')
        elif kind == CursorKind.MEMBER_REF_EXPR:
            self._record_reference(cursor, 'member_ref')
        elif kind == CursorKind.CALL_EXPR:
            self._record_reference(cursor, 'call')
        elif kind == CursorKind.DECL_REF_EXPR:
            ref = cursor.referenced
            if ref:
                self._reference_targets.add(self._qualified_name(ref))

        for child in cursor.get_children():
            self._walk(child, source_file)

    def _extract_class(self, cursor: Cursor):
        """Extract class/struct information."""
        if not cursor.is_definition():
            return

        qname = self._qualified_name(cursor)
        if not qname:
            return

        bases = []
        methods = []
        fields = []
        nested = []
        is_template = cursor.kind == CursorKind.CLASS_TEMPLATE
        template_params = []
        has_pure_virtual = False

        for child in cursor.get_children():
            if child.kind == CursorKind.CXX_BASE_SPECIFIER:
                base_name = child.type.spelling
                bases.append(base_name)
                self._reference_targets.add(base_name)
                self.references.append(SymbolReference(
                    symbol=base_name, kind='base_class',
                    from_file=self._relpath(cursor), from_line=cursor.location.line
                ))

            elif child.kind in (CursorKind.CXX_METHOD, CursorKind.CONSTRUCTOR,
                                CursorKind.DESTRUCTOR, CursorKind.CONVERSION_FUNCTION):
                method = self._extract_method(child)
                if method.is_pure_virtual:
                    has_pure_virtual = True
                methods.append(method)

            elif child.kind == CursorKind.FIELD_DECL:
                fields.append(FieldInfo(
                    name=child.spelling,
                    type=child.type.spelling,
                    access=self._access(child),
                    file=self._relpath(child),
                    line=child.location.line
                ))

            elif child.kind == CursorKind.VAR_DECL:
                # Static member variables
                fields.append(FieldInfo(
                    name=child.spelling,
                    type=child.type.spelling,
                    access=self._access(child),
                    is_static=True,
                    file=self._relpath(child),
                    line=child.location.line
                ))

            elif child.kind in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL):
                if child.is_definition() and child.spelling:
                    nested.append(self._qualified_name(child))

            elif child.kind == CursorKind.TEMPLATE_TYPE_PARAMETER:
                template_params.append(child.spelling or "T")
            elif child.kind == CursorKind.TEMPLATE_NON_TYPE_PARAMETER:
                template_params.append(f"{child.type.spelling} {child.spelling}")

        # Get brief comment if available
        comment = ""
        raw = cursor.raw_comment
        if raw:
            comment = self._extract_brief(raw)

        struct_kind = cursor.kind
        if cursor.kind == CursorKind.CLASS_TEMPLATE:
            # Peek at the actual template to determine struct vs class
            struct_kind = CursorKind.CLASS_DECL

        self.classes[qname] = ClassInfo(
            name=cursor.spelling,
            qualified_name=qname,
            kind="struct" if struct_kind == CursorKind.STRUCT_DECL else "class",
            bases=bases,
            methods=methods,
            fields=fields,
            nested_classes=nested,
            brief_comment=comment,
            is_abstract=has_pure_virtual,
            is_template=is_template,
            template_params=template_params,
            file=self._relpath(cursor),
            line=cursor.location.line
        )

        # Register in namespace
        ns = self._namespace_of(cursor)
        if ns and ns in self.namespaces:
            self.namespaces[ns].classes.append(qname)

    def _extract_method(self, cursor: Cursor) -> MethodInfo:
        """Extract method signature."""
        params = []
        for child in cursor.get_children():
            if child.kind == CursorKind.PARM_DECL:
                params.append(ParamInfo(
                    name=child.spelling or "",
                    type=child.type.spelling
                ))

        # Detect attributes
        attrs = []
        for child in cursor.get_children():
            if child.kind == CursorKind.ANNOTATE_ATTR:
                attrs.append(child.spelling)
            elif child.kind == CursorKind.UNEXPOSED_ATTR:
                # Try to catch [[nodiscard]] etc.
                extent = child.extent
                if extent.start.file:
                    try:
                        with open(str(extent.start.file), 'r', errors='ignore') as f:
                            content = f.read()
                            snippet = content[extent.start.offset:extent.end.offset]
                            if 'nodiscard' in snippet:
                                attrs.append('nodiscard')
                            if 'deprecated' in snippet:
                                attrs.append('deprecated')
                            if 'maybe_unused' in snippet:
                                attrs.append('maybe_unused')
                    except (OSError, IndexError):
                        pass

        result_type = cursor.result_type

        # Check for deleted/defaulted
        is_deleted = False
        is_defaulted = False
        # libclang doesn't have direct API for this, check tokens
        try:
            tokens = list(cursor.get_tokens())
            token_strs = [t.spelling for t in tokens]
            if '=' in token_strs:
                eq_idx = len(token_strs) - 1 - token_strs[::-1].index('=')
                if eq_idx + 1 < len(token_strs):
                    if token_strs[eq_idx + 1] == 'delete':
                        is_deleted = True
                    elif token_strs[eq_idx + 1] == 'default':
                        is_defaulted = True
        except Exception:
            pass

        comment = ""
        raw = cursor.raw_comment
        if raw:
            comment = self._extract_brief(raw)

        return MethodInfo(
            name=cursor.spelling,
            return_type=result_type.spelling if result_type else "",
            params=params,
            access=self._access(cursor),
            is_virtual=cursor.is_virtual_method(),
            is_pure_virtual=cursor.is_pure_virtual_method(),
            is_override=any(c.kind == CursorKind.CXX_OVERRIDE_ATTR for c in cursor.get_children()),
            is_static=cursor.is_static_method(),
            is_const=cursor.is_const_method() if hasattr(cursor, 'is_const_method') else False,
            is_deleted=is_deleted,
            is_defaulted=is_defaulted,
            brief_comment=comment,
            attributes=attrs,
            file=self._relpath(cursor),
            line=cursor.location.line
        )

    def _extract_enum(self, cursor: Cursor):
        """Extract enum information."""
        if not cursor.spelling:
            return
        qname = self._qualified_name(cursor)
        values = []
        for child in cursor.get_children():
            if child.kind == CursorKind.ENUM_CONSTANT_DECL:
                values.append(EnumValue(name=child.spelling, value=child.enum_value))

        self.enums[qname] = EnumInfo(
            name=cursor.spelling,
            qualified_name=qname,
            is_scoped=cursor.is_scoped_enum(),
            underlying_type=cursor.enum_type.spelling if cursor.enum_type else "",
            values=values,
            file=self._relpath(cursor),
            line=cursor.location.line
        )

        ns = self._namespace_of(cursor)
        if ns and ns in self.namespaces:
            self.namespaces[ns].enums.append(qname)

    def _extract_function(self, cursor: Cursor):
        """Extract free function information."""
        if not cursor.spelling:
            return
        qname = self._qualified_name(cursor)
        if qname in self.functions:
            return  # already seen (declaration + definition)

        params = []
        template_params = []
        for child in cursor.get_children():
            if child.kind == CursorKind.PARM_DECL:
                params.append(ParamInfo(
                    name=child.spelling or "",
                    type=child.type.spelling
                ))
            elif child.kind == CursorKind.TEMPLATE_TYPE_PARAMETER:
                template_params.append(child.spelling or "T")
            elif child.kind == CursorKind.TEMPLATE_NON_TYPE_PARAMETER:
                template_params.append(f"{child.type.spelling} {child.spelling}")

        comment = ""
        raw = cursor.raw_comment
        if raw:
            comment = self._extract_brief(raw)

        self.functions[qname] = FunctionInfo(
            name=cursor.spelling,
            qualified_name=qname,
            return_type=cursor.result_type.spelling if cursor.result_type else "",
            params=params,
            is_template=cursor.kind == CursorKind.FUNCTION_TEMPLATE,
            template_params=template_params,
            brief_comment=comment,
            file=self._relpath(cursor),
            line=cursor.location.line
        )

        ns = self._namespace_of(cursor)
        if ns and ns in self.namespaces:
            self.namespaces[ns].functions.append(qname)

    def _extract_namespace(self, cursor: Cursor):
        """Extract namespace information."""
        qname = self._qualified_name(cursor)
        if not qname:
            return
        if qname not in self.namespaces:
            self.namespaces[qname] = NamespaceInfo(
                name=cursor.spelling,
                qualified_name=qname
            )

    def _extract_type_alias(self, cursor: Cursor):
        """Extract using/typedef declarations."""
        qname = self._qualified_name(cursor)
        self.type_aliases[qname] = TypeAlias(
            name=cursor.spelling,
            underlying_type=cursor.underlying_typedef_type.spelling
            if cursor.kind == CursorKind.TYPEDEF_DECL
            else cursor.type.spelling,
            file=self._relpath(cursor),
            line=cursor.location.line
        )

    def _record_reference(self, cursor: Cursor, kind: str):
        """Record a symbol reference for usage tracking."""
        ref = cursor.referenced
        if ref:
            qname = self._qualified_name(ref)
            self._reference_targets.add(qname)
            self.references.append(SymbolReference(
                symbol=qname, kind=kind,
                from_file=self._relpath(cursor),
                from_line=cursor.location.line
            ))

    # ── Helpers ───────────────────────────────────────────────────────────

    @staticmethod
    def _qualified_name(cursor: Cursor) -> str:
        """Build fully qualified name (namespace::class::member)."""
        parts = []
        c = cursor
        while c and c.kind != CursorKind.TRANSLATION_UNIT:
            if c.spelling:
                parts.append(c.spelling)
            c = c.semantic_parent
        return "::".join(reversed(parts))

    @staticmethod
    def _namespace_of(cursor: Cursor) -> str:
        """Get the namespace containing this cursor."""
        parts = []
        c = cursor.semantic_parent
        while c and c.kind != CursorKind.TRANSLATION_UNIT:
            if c.kind == CursorKind.NAMESPACE and c.spelling:
                parts.append(c.spelling)
            c = c.semantic_parent
        return "::".join(reversed(parts)) if parts else ""

    @staticmethod
    def _access(cursor: Cursor) -> str:
        access = cursor.access_specifier
        if access == AccessSpecifier.PUBLIC:
            return "public"
        elif access == AccessSpecifier.PROTECTED:
            return "protected"
        elif access == AccessSpecifier.PRIVATE:
            return "private"
        return "unknown"

    def _relpath(self, cursor: Cursor) -> str:
        """Get path relative to project root."""
        if not cursor.location.file:
            return ""
        try:
            return str(Path(str(cursor.location.file)).resolve().relative_to(self.project_root))
        except ValueError:
            return str(cursor.location.file)

    @staticmethod
    def _extract_brief(raw_comment: str) -> str:
        """Extract a brief description from a raw comment."""
        if not raw_comment:
            return ""
        # Strip comment markers
        text = raw_comment.strip()
        text = re.sub(r'^/\*\*?', '', text)
        text = re.sub(r'\*/$', '', text)
        text = re.sub(r'^///?\s?', '', text, flags=re.MULTILINE)
        text = re.sub(r'^\s*\*\s?', '', text, flags=re.MULTILINE)
        # Take first sentence/line
        lines = [l.strip() for l in text.strip().split('\n') if l.strip()]
        if not lines:
            return ""
        brief = lines[0]
        # Strip @brief prefix
        brief = re.sub(r'^@brief\s+', '', brief)
        brief = re.sub(r'^\\brief\s+', '', brief)
        return brief[:200]  # cap length

    # ── Analysis ─────────────────────────────────────────────────────────

    def find_unused_classes(self) -> list[ClassInfo]:
        """Find classes that are never referenced anywhere."""
        unused = []
        for qname, cls in self.classes.items():
            is_referenced = (
                    qname in self._reference_targets or
                    cls.name in self._reference_targets or
                    any(qname in ref.symbol or cls.name in ref.symbol
                        for ref in self.references)
            )
            if not is_referenced:
                unused.append(cls)
        return unused

    def get_inheritance_tree(self) -> dict:
        """Build inheritance hierarchy."""
        tree = {}
        for qname, cls in self.classes.items():
            tree[qname] = {
                "bases": cls.bases,
                "derived": [
                    other_qname for other_qname, other_cls in self.classes.items()
                    if any(qname in b or cls.name in b for b in other_cls.bases)
                ]
            }
        return tree

    def query(self, query_str: str) -> str:
        """
        Answer structured queries about the codebase.

        Supported queries:
            class <name>        - Show detailed info about a class
            inherits <name>     - Show inheritance chain for a class
            file <name>         - Show all symbols defined in a file
            calls <name>        - Show what references/calls a symbol
            methods <name>      - List methods of a class
            fields <name>       - List fields of a class
            enum <name>         - Show enum values
            namespace <name>    - Show namespace contents
            deps <name>         - Show what a class depends on
        """
        parts = query_str.strip().split(None, 1)
        if len(parts) < 2:
            return f"Usage: --query '<command> <name>'\nCommands: class, inherits, file, calls, methods, fields, enum, namespace, deps"

        cmd, name = parts[0].lower(), parts[1].strip()
        lines = []

        if cmd == "class":
            cls = self._find_class(name)
            if not cls:
                return f"Class '{name}' not found. Available: {', '.join(sorted(self.classes.keys()))}"
            lines.append(self._format_class_detail(cls))

        elif cmd == "inherits":
            cls = self._find_class(name)
            if not cls:
                return f"Class '{name}' not found."
            lines.append(f"Inheritance chain for {cls.qualified_name}:")
            # Walk up
            chain_up = [cls.qualified_name]
            for base in cls.bases:
                chain_up.append(f"  ← {base}")
                parent = self._find_class(base)
                while parent and parent.bases:
                    chain_up.append(f"    ← {parent.bases[0]}")
                    parent = self._find_class(parent.bases[0])
            lines.extend(chain_up)
            # Walk down
            tree = self.get_inheritance_tree()
            derived = tree.get(cls.qualified_name, {}).get("derived", [])
            if derived:
                lines.append(f"  Derived classes:")
                for d in derived:
                    lines.append(f"    → {d}")

        elif cmd == "file":
            matches = []
            for qname, cls in self.classes.items():
                if name in cls.file:
                    matches.append(f"  {cls.kind} {qname} (line {cls.line})")
            for qname, fn in self.functions.items():
                if name in fn.file:
                    matches.append(f"  fn {qname} (line {fn.line})")
            for qname, enum in self.enums.items():
                if name in enum.file:
                    matches.append(f"  enum {qname} (line {enum.line})")
            if not matches:
                return f"No symbols found in file matching '{name}'."
            lines.append(f"Symbols in '{name}':")
            lines.extend(sorted(matches))

        elif cmd == "calls":
            refs = [r for r in self.references if name in r.symbol]
            if not refs:
                return f"No references to '{name}' found."
            lines.append(f"References to '{name}' ({len(refs)}):")
            seen = set()
            for r in refs:
                key = f"{r.from_file}:{r.from_line}"
                if key not in seen:
                    seen.add(key)
                    lines.append(f"  {r.kind}: {r.from_file}:{r.from_line}")

        elif cmd == "methods":
            cls = self._find_class(name)
            if not cls:
                return f"Class '{name}' not found."
            lines.append(f"Methods of {cls.qualified_name}:")
            for m in cls.methods:
                sig = self._format_method_sig(m)
                lines.append(f"  [{m.access}] {sig}")

        elif cmd == "fields":
            cls = self._find_class(name)
            if not cls:
                return f"Class '{name}' not found."
            lines.append(f"Fields of {cls.qualified_name}:")
            for f in cls.fields:
                prefix = "static " if f.is_static else ""
                lines.append(f"  [{f.access}] {prefix}{f.type} {f.name}")

        elif cmd == "enum":
            enum = self._find_enum(name)
            if not enum:
                return f"Enum '{name}' not found. Available: {', '.join(sorted(self.enums.keys()))}"
            scope = "enum class" if enum.is_scoped else "enum"
            lines.append(f"{scope} {enum.qualified_name}:")
            for v in enum.values:
                lines.append(f"  {v.name} = {v.value}")

        elif cmd == "namespace":
            ns = None
            for qn, n in self.namespaces.items():
                if name == qn or name == n.name:
                    ns = n
                    break
            if not ns:
                return f"Namespace '{name}' not found. Available: {', '.join(sorted(self.namespaces.keys()))}"
            lines.append(f"namespace {ns.qualified_name}:")
            if ns.classes:
                lines.append(f"  Classes: {', '.join(ns.classes)}")
            if ns.functions:
                lines.append(f"  Functions: {', '.join(ns.functions)}")
            if ns.enums:
                lines.append(f"  Enums: {', '.join(ns.enums)}")

        elif cmd == "deps":
            cls = self._find_class(name)
            if not cls:
                return f"Class '{name}' not found."
            lines.append(f"Dependencies of {cls.qualified_name}:")
            deps = set()
            # Base classes
            for b in cls.bases:
                deps.add(f"  base: {b}")
            # Field types
            for f in cls.fields:
                clean = self._extract_type_names(f.type)
                for t in clean:
                    deps.add(f"  field: {t} ({f.name})")
            # Method param/return types
            for m in cls.methods:
                for t in self._extract_type_names(m.return_type):
                    deps.add(f"  return: {t} ({m.name})")
                for p in m.params:
                    for t in self._extract_type_names(p.type):
                        deps.add(f"  param: {t} ({m.name})")
            lines.extend(sorted(deps))

        else:
            return f"Unknown query command '{cmd}'. Use: class, inherits, file, calls, methods, fields, enum, namespace, deps"

        return "\n".join(lines)

    def _find_class(self, name: str) -> Optional[ClassInfo]:
        """Find a class by exact or partial name."""
        if name in self.classes:
            return self.classes[name]
        for qname, cls in self.classes.items():
            if cls.name == name or qname.endswith("::" + name):
                return cls
        return None

    def _find_enum(self, name: str) -> Optional[EnumInfo]:
        if name in self.enums:
            return self.enums[name]
        for qname, enum in self.enums.items():
            if enum.name == name or qname.endswith("::" + name):
                return enum
        return None

    @staticmethod
    def _extract_type_names(type_str: str) -> list[str]:
        """Extract meaningful type names from a type string, filtering primitives."""
        # Remove common wrappers
        cleaned = type_str
        for wrapper in ['const ', '&', '*', 'volatile ']:
            cleaned = cleaned.replace(wrapper, '')
        # Extract identifiers that look like user types
        names = re.findall(r'\b([A-Z][A-Za-z0-9_]+)\b', cleaned)
        # Filter out common STL types
        stl = {'string', 'vector', 'map', 'unordered_map', 'set', 'unique_ptr',
               'shared_ptr', 'optional', 'variant', 'pair', 'tuple', 'array',
               'function', 'any', 'size_t'}
        return [n for n in names if n.lower() not in stl and len(n) > 1]

    # ── Output Formats ───────────────────────────────────────────────────

    def to_json(self) -> dict:
        """Full JSON export."""
        return {
            "files_parsed": [str(Path(f).relative_to(self.project_root))
                             if Path(f).is_relative_to(self.project_root) else f
                             for f in self.files_parsed],
            "classes": {k: asdict(v) for k, v in self.classes.items()},
            "enums": {k: asdict(v) for k, v in self.enums.items()},
            "functions": {k: asdict(v) for k, v in self.functions.items()},
            "type_aliases": {k: asdict(v) for k, v in self.type_aliases.items()},
            "namespaces": {k: asdict(v) for k, v in self.namespaces.items()},
            "inheritance": self.get_inheritance_tree(),
            "statistics": {
                "total_classes": len(self.classes),
                "total_enums": len(self.enums),
                "total_free_functions": len(self.functions),
                "total_methods": sum(len(c.methods) for c in self.classes.values()),
                "total_references": len(self.references),
                "parse_errors": self._error_count,
            }
        }

    def to_api_summary(self, show_private: bool = True) -> str:
        """Compact API summary optimized for LLM context windows.

        Features:
        - Collapses repetitive visitor overloads into compact lists
        - Groups by namespace
        - Shows relative file paths
        - Omits private members if requested
        """
        lines = []
        access_filter = ("public", "protected", "private") if show_private else ("public", "protected")

        # Group classes by namespace for cleaner output
        ns_classes = defaultdict(list)
        for qname, cls in self.classes.items():
            ns = self._namespace_of_qname(qname)
            ns_classes[ns].append((qname, cls))

        for ns in sorted(ns_classes.keys()):
            if ns:
                lines.append(f"namespace {ns} {{")
                indent = "  "
            else:
                indent = ""

            for qname, cls in ns_classes[ns]:
                # Class header
                display_name = cls.name
                if cls.is_template and cls.template_params:
                    display_name = f"{cls.name}<{', '.join(cls.template_params)}>"

                header = f"{indent}{'struct' if cls.kind == 'struct' else 'class'} {display_name}"
                if cls.bases:
                    header += f" : {', '.join(cls.bases)}"
                if cls.is_abstract:
                    header += "  [abstract]"

                loc = f"{cls.file}:{cls.line}" if cls.file else ""
                if loc:
                    header += f"  // {loc}"
                lines.append(header)

                if cls.brief_comment:
                    lines.append(f"{indent}  // {cls.brief_comment}")

                # Methods grouped by access
                for access in access_filter:
                    methods = [m for m in cls.methods if m.access == access]
                    if not methods:
                        continue

                    # Detect and collapse visitor-pattern overloads
                    visitor_methods, regular_methods = self._split_visitor_overloads(methods)

                    lines.append(f"{indent}  {access}:")

                    # Show collapsed visitor overloads
                    if visitor_methods:
                        visited_types = [self._extract_visited_type(m) for m in visitor_methods]
                        visited_types = [t for t in visited_types if t]
                        if visited_types:
                            # Determine visitor signature pattern
                            sample = visitor_methods[0]
                            quals = "virtual " if sample.is_virtual else ""
                            suffix = " = 0" if sample.is_pure_virtual else ""
                            const_q = " const" if sample.is_const else ""
                            override_q = " override" if sample.is_override else ""
                            lines.append(f"{indent}    {quals}void visit(<T> & node){const_q}{override_q}{suffix}"
                                         f"  // {len(visited_types)} overloads")
                            # Compact list of visited types
                            type_line = f"{indent}      T ∈ {{ {', '.join(visited_types)} }}"
                            # Wrap long lines
                            if len(type_line) > 120:
                                lines.append(f"{indent}      T ∈ {{")
                                chunk = []
                                line_len = 0
                                for t in visited_types:
                                    if line_len + len(t) + 2 > 100:
                                        lines.append(f"{indent}        {', '.join(chunk)},")
                                        chunk = []
                                        line_len = 0
                                    chunk.append(t)
                                    line_len += len(t) + 2
                                if chunk:
                                    lines.append(f"{indent}        {', '.join(chunk)}")
                                lines.append(f"{indent}      }}")
                            else:
                                lines.append(type_line)

                    # Show regular methods
                    for m in regular_methods:
                        sig = self._format_method_sig(m, indent=indent + "    ")
                        lines.append(f"{indent}    {sig}")

                # Fields grouped by access
                for access in access_filter:
                    fields = [f for f in cls.fields if f.access == access]
                    if fields:
                        for f in fields:
                            prefix = "static " if f.is_static else ""
                            lines.append(f"{indent}    {prefix}{f.type} {f.name}")

                lines.append("")

            if ns:
                lines.append(f"}}  // namespace {ns}")
                lines.append("")

        # Free functions
        if self.functions:
            lines.append("// ── Free Functions ──")
            for qname, fn in self.functions.items():
                tpl = ""
                if fn.is_template:
                    tpl = f"template<{', '.join(fn.template_params)}> "
                params_str = ", ".join(
                    f"{p.type} {p.name}" if p.name else p.type for p in fn.params
                )
                loc = f"  // {fn.file}:{fn.line}" if fn.file else ""
                lines.append(f"{tpl}{fn.return_type} {qname}({params_str}){loc}")
            lines.append("")

        # Enums
        for qname, enum in self.enums.items():
            scope = "enum class" if enum.is_scoped else "enum"
            vals = ", ".join(
                f"{v.name}={v.value}" if v.value is not None else v.name
                for v in enum.values
            )
            lines.append(f"{scope} {qname} {{ {vals} }}")

        # Type aliases
        if self.type_aliases:
            lines.append("")
            for qname, alias in self.type_aliases.items():
                lines.append(f"using {qname} = {alias.underlying_type}")

        return "\n".join(lines)

    def unused_report(self) -> str:
        """Report on potentially unused classes."""
        unused = self.find_unused_classes()
        if not unused:
            return "✓ No unused classes detected."

        lines = [f"⚠ Found {len(unused)} potentially unused class(es):\n"]
        for cls in unused:
            loc = f"{cls.file}:{cls.line}" if cls.file else "unknown"
            lines.append(f"  • {cls.qualified_name} ({cls.kind}) at {loc}")
            lines.append(f"    Methods: {len(cls.methods)}, Fields: {len(cls.fields)}")
        return "\n".join(lines)

    # ── Format helpers ───────────────────────────────────────────────────

    @staticmethod
    def _split_visitor_overloads(methods: list[MethodInfo]) -> tuple[list[MethodInfo], list[MethodInfo]]:
        """Separate visitor-pattern overloads from regular methods.

        Detects methods named 'visit' with a single reference parameter that
        differ only in the parameter type — the hallmark of visitor patterns.
        """
        visitors = []
        regular = []
        for m in methods:
            if (m.name == "visit" and
                    len(m.params) == 1 and
                    '&' in m.params[0].type):
                visitors.append(m)
            else:
                regular.append(m)

        # Only collapse if there are enough to be worth it
        if len(visitors) < 4:
            regular.extend(visitors)
            return [], regular

        return visitors, regular

    @staticmethod
    def _extract_visited_type(method: MethodInfo) -> str:
        """Extract the visited type from a visitor method parameter."""
        param_type = method.params[0].type if method.params else ""
        # "const ProgramNode &" -> "ProgramNode"
        cleaned = param_type.replace("const ", "").replace(" &", "").replace("&", "").strip()
        # Remove namespace prefixes for readability
        if "::" in cleaned:
            cleaned = cleaned.split("::")[-1]
        return cleaned

    @staticmethod
    def _format_method_sig(m: MethodInfo, indent: str = "") -> str:
        """Format a method signature."""
        quals = ""
        if m.is_static:
            quals += "static "
        if m.is_virtual:
            quals += "virtual "
        params_str = ", ".join(
            f"{p.type} {p.name}" if p.name else p.type for p in m.params
        )
        suffix = ""
        if m.is_const:
            suffix += " const"
        if m.is_noexcept:
            suffix += " noexcept"
        if m.is_override:
            suffix += " override"
        if m.is_pure_virtual:
            suffix += " = 0"
        if m.is_deleted:
            suffix += " = delete"
        if m.is_defaulted:
            suffix += " = default"

        attrs = ""
        if m.attributes:
            attrs = f" [[{', '.join(m.attributes)}]]"

        sig = f"{quals}{m.return_type} {m.name}({params_str}){suffix}{attrs}"

        if m.brief_comment:
            sig += f"  // {m.brief_comment}"

        return sig

    @staticmethod
    def _namespace_of_qname(qname: str) -> str:
        """Extract namespace from a qualified name like 'ns1::ns2::ClassName'."""
        parts = qname.split("::")
        if len(parts) <= 1:
            return ""
        # Last part is the class name, everything before is namespace
        # But we need to handle nested classes — heuristic: namespace parts are lowercase
        ns_parts = []
        for p in parts[:-1]:
            ns_parts.append(p)
        return "::".join(ns_parts) if ns_parts else ""


# ── Include Path Auto-Detection ──────────────────────────────────────────────

def auto_detect_includes(project_path: Path) -> list[str]:
    """Try to detect include directories from project structure."""
    includes = []
    root = project_path if project_path.is_dir() else project_path.parent

    # Walk up to find project root indicators
    search_root = root.resolve()
    for _ in range(5):  # max 5 levels up
        if (search_root / "CMakeLists.txt").exists():
            root = search_root
            break
        if (search_root / "Makefile").exists():
            root = search_root
            break
        if (search_root / ".git").exists():
            root = search_root
            break
        if search_root.parent == search_root:
            break
        search_root = search_root.parent

    # Common include directory patterns
    for pattern in ["include", "inc", "src", "lib", "."]:
        candidate = root / pattern
        if candidate.is_dir():
            includes.append(f"-I{candidate.resolve()}")

    # Check for CMakeLists.txt to find target_include_directories
    cmake_file = root / "CMakeLists.txt"
    if cmake_file.exists():
        try:
            content = cmake_file.read_text(errors='ignore')
            # Simple regex for include_directories
            for m in re.finditer(r'include_directories\s*\(([^)]+)\)', content):
                for d in m.group(1).split():
                    d = d.strip('"').strip("'")
                    if d.startswith('$'):
                        continue
                    p = root / d
                    if p.is_dir():
                        includes.append(f"-I{p.resolve()}")
        except OSError:
            pass

    return includes


# ── CLI ──────────────────────────────────────────────────────────────────────

def main():
    import argparse
    parser = argparse.ArgumentParser(
        description="C++ AST extraction tool for LLM agents",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s ./myproject                          # auto-detect includes
  %(prog)s src -I include -I third_party        # explicit includes
  %(prog)s include --query "class IRGenerator"  # query specific class
  %(prog)s src --find-unused                    # find dead code
  %(prog)s include --format json > api.json     # export JSON
        """
    )
    parser.add_argument("path", help="File or directory to analyze")
    parser.add_argument("--format", "-f", choices=["json", "summary", "api"], default="api",
                        help="Output format (default: api)")
    parser.add_argument("--include-dir", "-I", action="append", default=[],
                        help="Add include search path (repeatable)")
    parser.add_argument("--find-unused", action="store_true",
                        help="Report potentially unused classes")
    parser.add_argument("--query", "-q", type=str, default=None,
                        help="Query mode: 'class X', 'inherits Y', 'file Z', etc.")
    parser.add_argument("--headers-only", action="store_true",
                        help="Only parse header files")
    parser.add_argument("--no-private", action="store_true",
                        help="Hide private members from output")
    parser.add_argument("--no-auto-include", action="store_true",
                        help="Disable automatic include path detection")
    parser.add_argument("--compile-args", nargs="*", default=[],
                        help="Additional compiler arguments")
    args = parser.parse_args()

    path = Path(args.path).resolve()
    project_root = path if path.is_dir() else path.parent

    # Build compile args
    compile_args = ["-std=c++20", "-x", "c++"]

    # Auto-detect includes
    if not args.no_auto_include:
        auto_includes = auto_detect_includes(path)
        if auto_includes:
            print(f"Auto-detected include paths: {' '.join(auto_includes)}", file=sys.stderr)
        compile_args.extend(auto_includes)

    # Explicit -I flags
    for inc in args.include_dir:
        inc_path = Path(inc).resolve()
        arg = f"-I{inc_path}"
        if arg not in compile_args:
            compile_args.append(arg)

    compile_args.extend(args.compile_args)

    extractor = CppAstExtractor(compile_args=compile_args, project_root=str(project_root))

    extensions = None
    if args.headers_only:
        extensions = (".h", ".hpp", ".hxx")

    if path.is_dir():
        extractor.parse_directory(str(path), extensions=extensions)
    else:
        extractor.parse_file(str(path))

    # Query mode
    if args.query:
        print(extractor.query(args.query))
        return

    # Unused report
    if args.find_unused:
        print(extractor.unused_report())
        print()

    # Output
    if args.format == "json":
        print(json.dumps(extractor.to_json(), indent=2))
    elif args.format == "api":
        print(extractor.to_api_summary(show_private=not args.no_private))
    elif args.format == "summary":
        data = extractor.to_json()
        stats = data["statistics"]
        print(f"Classes: {stats['total_classes']}")
        print(f"Enums: {stats['total_enums']}")
        print(f"Free Functions: {stats['total_free_functions']}")
        print(f"Methods: {stats['total_methods']}")
        print(f"References tracked: {stats['total_references']}")
        if stats['parse_errors']:
            print(f"Parse errors: {stats['parse_errors']}")
        print(f"\nInheritance:")
        for cls, info in data["inheritance"].items():
            if info["bases"] or info["derived"]:
                print(f"  {cls}")
                if info["bases"]:
                    print(f"    ← {', '.join(info['bases'])}")
                if info["derived"]:
                    print(f"    → {', '.join(info['derived'])}")
        print(f"\nNamespaces:")
        for ns_name, ns_info in data["namespaces"].items():
            parts = []
            if ns_info["classes"]:
                parts.append(f"{len(ns_info['classes'])} classes")
            if ns_info["functions"]:
                parts.append(f"{len(ns_info['functions'])} functions")
            if ns_info["enums"]:
                parts.append(f"{len(ns_info['enums'])} enums")
            if parts:
                print(f"  {ns_name}: {', '.join(parts)}")


if __name__ == "__main__":
    main()