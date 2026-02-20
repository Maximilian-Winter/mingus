#!/usr/bin/env python3
"""
cpp_ast_tool.py — Semantic C++ codebase analyzer using libclang.

Extracts class APIs, inheritance hierarchies, call graphs, and detects
unused classes/symbols. Designed as a foundation for LLM coding agents.

Usage:
    python cpp_ast_tool.py <path> [--format json|summary|api] [--find-unused] [--compile-args ...]
"""

import json
import sys
import os
from pathlib import Path
from dataclasses import dataclass, field, asdict
from typing import Optional
from collections import defaultdict
from clang.cindex import Config
Config.set_library_file("./extern/clang+llvm-21.1.8-x86_64-pc-windows-msvc/bin/libclang.dll")
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
    attributes: list[str] = field(default_factory=list)  # [[nodiscard]], etc.
    file: str = ""
    line: int = 0

@dataclass
class FieldInfo:
    name: str
    type: str
    access: str
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
    qualified_name: str  # with namespace
    kind: str  # class, struct
    bases: list[str]
    methods: list[MethodInfo]
    fields: list[FieldInfo]
    file: str = ""
    line: int = 0

@dataclass
class TypeAlias:
    name: str
    underlying_type: str
    file: str = ""
    line: int = 0

@dataclass
class SymbolReference:
    """A reference to a symbol from somewhere in the codebase."""
    symbol: str  # qualified name of referenced symbol
    kind: str  # 'type_ref', 'member_ref', 'call', 'base_class', 'instantiation'
    from_file: str
    from_line: int


# ── AST Walker ───────────────────────────────────────────────────────────────

class CppAstExtractor:
    def __init__(self, compile_args: list[str] = None):
        self.index = Index.create()
        self.compile_args = compile_args or ["-std=c++20", "-x", "c++"]
        
        self.classes: dict[str, ClassInfo] = {}
        self.enums: dict[str, EnumInfo] = {}
        self.type_aliases: dict[str, TypeAlias] = {}
        self.references: list[SymbolReference] = []
        self.files_parsed: list[str] = []
        self._reference_targets: set[str] = set()  # for quick unused detection

    def parse_file(self, filepath: str):
        """Parse a single translation unit."""
        filepath = str(Path(filepath).resolve())
        tu = self.index.parse(
            filepath,
            args=self.compile_args,
            options=(
                TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD |
                TranslationUnit.PARSE_SKIP_FUNCTION_BODIES  # faster, we want API not impl
            )
        )
        
        # Report diagnostics
        errors = [d for d in tu.diagnostics if d.severity >= 3]
        if errors:
            print(f"  ⚠ {len(errors)} error(s) in {Path(filepath).name}:", file=sys.stderr)
            for d in errors[:3]:
                print(f"    {d}", file=sys.stderr)
        
        self.files_parsed.append(filepath)
        self._walk(tu.cursor, filepath)

    def parse_directory(self, dirpath: str, extensions=(".h", ".hpp", ".hxx", ".cpp", ".cxx", ".cc")):
        """Parse all C++ files in a directory."""
        dirpath = Path(dirpath)
        files = sorted(f for f in dirpath.rglob("*") if f.suffix in extensions)
        print(f"Found {len(files)} C++ files in {dirpath}", file=sys.stderr)
        for f in files:
            print(f"  Parsing {f.name}...", file=sys.stderr)
            self.parse_file(str(f))

    def _walk(self, cursor: Cursor, source_file: str):
        """Recursively walk the AST."""
        # Only process nodes from our source files (skip system headers)
        if cursor.location.file and str(cursor.location.file) not in self.files_parsed:
            # But still check if it's in our source directory
            loc = str(cursor.location.file)
            source_dir = str(Path(source_file).parent)
            if not loc.startswith(source_dir):
                return

        kind = cursor.kind

        if kind in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL):
            self._extract_class(cursor)
        elif kind == CursorKind.ENUM_DECL:
            self._extract_enum(cursor)
        elif kind in (CursorKind.TYPE_ALIAS_DECL, CursorKind.TYPEDEF_DECL):
            self._extract_type_alias(cursor)
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
        
        bases = []
        methods = []
        fields = []

        for child in cursor.get_children():
            if child.kind == CursorKind.CXX_BASE_SPECIFIER:
                base_name = child.type.spelling
                bases.append(base_name)
                self._reference_targets.add(base_name)
                self.references.append(SymbolReference(
                    symbol=base_name, kind='base_class',
                    from_file=self._file(cursor), from_line=cursor.location.line
                ))

            elif child.kind in (CursorKind.CXX_METHOD, CursorKind.CONSTRUCTOR,
                                CursorKind.DESTRUCTOR, CursorKind.CONVERSION_FUNCTION):
                methods.append(self._extract_method(child))

            elif child.kind == CursorKind.FIELD_DECL:
                fields.append(FieldInfo(
                    name=child.spelling,
                    type=child.type.spelling,
                    access=self._access(child),
                    file=self._file(child),
                    line=child.location.line
                ))

        self.classes[qname] = ClassInfo(
            name=cursor.spelling,
            qualified_name=qname,
            kind="struct" if cursor.kind == CursorKind.STRUCT_DECL else "class",
            bases=bases,
            methods=methods,
            fields=fields,
            file=self._file(cursor),
            line=cursor.location.line
        )

    def _extract_method(self, cursor: Cursor) -> MethodInfo:
        """Extract method signature."""
        params = []
        for child in cursor.get_children():
            if child.kind == CursorKind.PARM_DECL:
                params.append(ParamInfo(
                    name=child.spelling or "",
                    type=child.type.spelling
                ))

        # Detect attributes like [[nodiscard]]
        attrs = []
        for child in cursor.get_children():
            if child.kind == CursorKind.ANNOTATE_ATTR:
                attrs.append(child.spelling)
        
        # Check for [[nodiscard]] via result type
        result_type = cursor.result_type
        if cursor.raw_comment and "nodiscard" in (cursor.raw_comment or ""):
            attrs.append("nodiscard")

        return MethodInfo(
            name=cursor.spelling,
            return_type=result_type.spelling if hasattr(cursor, 'result_type') else "",
            params=params,
            access=self._access(cursor),
            is_virtual=cursor.is_virtual_method(),
            is_pure_virtual=cursor.is_pure_virtual_method(),
            is_override=any(c.kind == CursorKind.CXX_OVERRIDE_ATTR for c in cursor.get_children()),
            is_static=cursor.is_static_method(),
            is_const=cursor.is_const_method() if hasattr(cursor, 'is_const_method') else False,
            attributes=attrs,
            file=self._file(cursor),
            line=cursor.location.line
        )

    def _extract_enum(self, cursor: Cursor):
        """Extract enum information."""
        if not cursor.spelling:  # anonymous enum
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
            file=self._file(cursor),
            line=cursor.location.line
        )

    def _extract_type_alias(self, cursor: Cursor):
        """Extract using/typedef declarations."""
        qname = self._qualified_name(cursor)
        self.type_aliases[qname] = TypeAlias(
            name=cursor.spelling,
            underlying_type=cursor.underlying_typedef_type.spelling 
                if cursor.kind == CursorKind.TYPEDEF_DECL 
                else cursor.type.spelling,
            file=self._file(cursor),
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
                from_file=self._file(cursor),
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
    def _access(cursor: Cursor) -> str:
        access = cursor.access_specifier
        if access == AccessSpecifier.PUBLIC:
            return "public"
        elif access == AccessSpecifier.PROTECTED:
            return "protected"
        elif access == AccessSpecifier.PRIVATE:
            return "private"
        return "unknown"

    @staticmethod
    def _file(cursor: Cursor) -> str:
        if cursor.location.file:
            return str(cursor.location.file)
        return ""

    # ── Analysis ─────────────────────────────────────────────────────────

    def find_unused_classes(self) -> list[ClassInfo]:
        """Find classes that are never referenced anywhere."""
        unused = []
        for qname, cls in self.classes.items():
            # Check if this class name appears in any reference
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

    # ── Output Formats ───────────────────────────────────────────────────

    def to_json(self) -> dict:
        """Full JSON export."""
        return {
            "files_parsed": self.files_parsed,
            "classes": {k: asdict(v) for k, v in self.classes.items()},
            "enums": {k: asdict(v) for k, v in self.enums.items()},
            "type_aliases": {k: asdict(v) for k, v in self.type_aliases.items()},
            "inheritance": self.get_inheritance_tree(),
            "statistics": {
                "total_classes": len(self.classes),
                "total_enums": len(self.enums),
                "total_methods": sum(len(c.methods) for c in self.classes.values()),
                "total_references": len(self.references),
            }
        }

    def to_api_summary(self) -> str:
        """Compact API summary optimized for LLM context windows."""
        lines = []
        
        for qname, cls in self.classes.items():
            header = f"{'struct' if cls.kind == 'struct' else 'class'} {qname}"
            if cls.bases:
                header += f" : {', '.join(cls.bases)}"
            lines.append(header)
            
            # Group methods by access
            for access in ("public", "protected", "private"):
                methods = [m for m in cls.methods if m.access == access]
                if not methods:
                    continue
                lines.append(f"  {access}:")
                for m in methods:
                    quals = ""
                    if m.is_static:
                        quals += "static "
                    if m.is_virtual:
                        quals += "virtual "
                    params_str = ", ".join(f"{p.type} {p.name}" if p.name else p.type 
                                          for p in m.params)
                    suffix = ""
                    if m.is_const:
                        suffix += " const"
                    if m.is_override:
                        suffix += " override"
                    if m.is_pure_virtual:
                        suffix += " = 0"
                    lines.append(f"    {quals}{m.return_type} {m.name}({params_str}){suffix}")
            
            # Fields
            for access in ("public", "protected", "private"):
                fields = [f for f in cls.fields if f.access == access]
                if fields:
                    for f in fields:
                        lines.append(f"    {f.type} {f.name}")
            
            lines.append("")

        for qname, enum in self.enums.items():
            scope = "enum class" if enum.is_scoped else "enum"
            vals = ", ".join(
                f"{v.name}={v.value}" if v.value is not None else v.name 
                for v in enum.values
            )
            lines.append(f"{scope} {qname} {{ {vals} }}")
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
            loc = f"{Path(cls.file).name}:{cls.line}" if cls.file else "unknown"
            lines.append(f"  • {cls.qualified_name} ({cls.kind}) at {loc}")
            lines.append(f"    Methods: {len(cls.methods)}, Fields: {len(cls.fields)}")
        return "\n".join(lines)


# ── CLI ──────────────────────────────────────────────────────────────────────

def main():
    import argparse
    parser = argparse.ArgumentParser(description="C++ AST extraction tool for LLM agents")
    parser.add_argument("path", help="File or directory to analyze")
    parser.add_argument("--format", choices=["json", "summary", "api"], default="api",
                        help="Output format (default: api)")
    parser.add_argument("--find-unused", action="store_true",
                        help="Report potentially unused classes")
    parser.add_argument("--compile-args", nargs="*", default=[],
                        help="Additional compiler arguments")
    args = parser.parse_args()

    extractor = CppAstExtractor(
        compile_args=["-std=c++20", "-x", "c++"] + args.compile_args
    )

    path = Path(args.path)
    if path.is_dir():
        extractor.parse_directory(str(path))
    else:
        extractor.parse_file(str(path))

    if args.find_unused:
        print(extractor.unused_report())
        print()

    if args.format == "json":
        print(json.dumps(extractor.to_json(), indent=2))
    elif args.format == "api":
        print(extractor.to_api_summary())
    elif args.format == "summary":
        data = extractor.to_json()
        stats = data["statistics"]
        print(f"Classes: {stats['total_classes']}")
        print(f"Enums: {stats['total_enums']}")
        print(f"Methods: {stats['total_methods']}")
        print(f"References tracked: {stats['total_references']}")
        print(f"\nInheritance:")
        for cls, info in data["inheritance"].items():
            if info["bases"] or info["derived"]:
                print(f"  {cls}")
                if info["bases"]:
                    print(f"    ← {', '.join(info['bases'])}")
                if info["derived"]:
                    print(f"    → {', '.join(info['derived'])}")


if __name__ == "__main__":
    main()
