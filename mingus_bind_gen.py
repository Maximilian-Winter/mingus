#!/usr/bin/env python3
"""
mingus_bind_gen.py — Generate Mingus FFI bindings from C header files.

Uses libclang to parse C headers and emits Mingus `extern { }` declarations
with automatic type mapping.

Usage:
    python mingus_bind_gen.py <header_or_dir> [options]

Options:
    -I, --include-dir DIR    Add include search path (repeatable)
    --module NAME            Module name (default: derived from filename)
    -o, --output FILE        Write to file instead of stdout
    --prefix PREFIX          Only include symbols matching prefix (repeatable)
    --no-structs             Skip struct generation
    --no-enums               Skip enum generation
    --compile-args ...       Extra clang args (e.g. -DFOO=1)

Examples:
    # Generate bindings for stb_image_write
    python mingus_bind_gen.py examples/extern_c/stb_image_write.h

    # Generate bindings with prefix filter
    python mingus_bind_gen.py SDL2/SDL.h -I /usr/include --prefix SDL_

    # Write to file with custom module name
    python mingus_bind_gen.py mylib.h --module MyLib -o bindings.mingus
"""

import sys
import os
import re
from pathlib import Path
from dataclasses import dataclass, field

# ── libclang setup (reused from cpp_ast_tool.py) ────────────────────────────

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

    if not libclang_path:
        candidates = []
        if sys.platform == "win32":
            for base in [Path("."), Path("./extern")]:
                candidates.extend(base.glob("**/libclang.dll"))
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
    Index, CursorKind, TranslationUnit, Cursor, TypeKind
)


# ── Data Models ──────────────────────────────────────────────────────────────

@dataclass
class ParamBinding:
    name: str
    mingus_type: str
    c_type: str          # original C type string for comments
    warning: str = ""    # non-empty if type mapping lost precision

@dataclass
class FuncBinding:
    name: str
    return_type: str     # Mingus return type
    params: list[ParamBinding]
    is_variadic: bool
    c_signature: str     # original C signature for comment
    warning: str = ""

@dataclass
class FieldBinding:
    name: str
    mingus_type: str
    c_type: str
    warning: str = ""

@dataclass
class StructBinding:
    name: str
    fields: list[FieldBinding]
    is_opaque: bool      # True if forward-declared / no visible body

@dataclass
class EnumBinding:
    name: str
    values: list[tuple[str, int]]  # (name, value)
    underlying_type: str = "int"   # Mingus type for the underlying integer

@dataclass
class TypeWarning:
    location: str        # e.g. "stbi_write_png param 3"
    c_type: str
    mingus_type: str
    message: str


# ── Binding Generator ────────────────────────────────────────────────────────

class MingusBindingGenerator:
    # Mingus keywords that cannot be used as parameter/field names
    MINGUS_KEYWORDS = {
        "func", "var", "class", "struct", "enum", "interface", "module",
        "if", "else", "while", "for", "do", "return", "break", "continue",
        "new", "delete", "null", "true", "false", "this", "extern", "import",
        "using", "match", "operator", "constructor", "destructor",
        "int", "double", "float", "byte", "char", "bool", "string", "void",
        "short", "ushort", "uint", "long", "ulong",
        "shared", "opaque", "raw", "weak", "link",
    }

    def __init__(self, compile_args: list[str] = None):
        self.index = Index.create()
        self.compile_args = ["-x", "c", "-std=c11"]
        if compile_args:
            self.compile_args.extend(compile_args)

        self.functions: list[FuncBinding] = []
        self.structs: dict[str, StructBinding] = {}
        self.enums: dict[str, EnumBinding] = {}
        self.warnings: list[TypeWarning] = []

        self._seen_funcs: set[str] = set()
        self._target_files: set[str] = set()  # files we want to extract from
        self._typedef_to_struct: dict[str, str] = {}  # typedef name -> struct name
        self._typedef_to_enum: dict[str, str] = {}    # typedef name -> enum name
        self._struct_to_typedef: dict[str, str] = {}  # struct name -> preferred typedef name

    def parse(self, filepath: str):
        """Parse a C header file and collect declarations."""
        filepath = str(Path(filepath).resolve())
        self._target_files.add(filepath)

        tu = self.index.parse(
            filepath,
            args=self.compile_args,
            options=TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD
        )

        # Report errors
        errors = [d for d in tu.diagnostics if d.severity >= 3]
        if errors:
            print(f"  Warning: {len(errors)} parse error(s) in {Path(filepath).name}:",
                  file=sys.stderr)
            for d in errors[:5]:
                print(f"    {d}", file=sys.stderr)

        # First pass: collect typedef→struct/enum mappings
        self._collect_typedefs(tu.cursor)
        # Second pass: collect all declarations
        self._walk(tu.cursor)

    def parse_directory(self, dirpath: str, extensions=None):
        """Parse all C header files in a directory."""
        if extensions is None:
            extensions = (".h", ".hpp")
        dirpath = Path(dirpath)
        files = sorted(f for f in dirpath.rglob("*") if f.suffix in extensions)
        print(f"Found {len(files)} header file(s) in {dirpath}", file=sys.stderr)
        for f in files:
            print(f"  Parsing {f.name}...", file=sys.stderr)
            self.parse(str(f))

    def _is_target_file(self, cursor: Cursor) -> bool:
        """Check if cursor is in one of our target files (not system headers)."""
        if not cursor.location.file:
            return False
        loc = str(Path(str(cursor.location.file)).resolve())
        return loc in self._target_files

    def _collect_typedefs(self, cursor: Cursor):
        """Pre-pass to map typedefs to their underlying struct/enum names.
        Scans all files (including system headers) to capture typedefs like FILE."""
        for child in cursor.get_children():
            if child.kind == CursorKind.TYPEDEF_DECL:
                underlying = child.underlying_typedef_type
                canonical = underlying.get_canonical()
                if canonical.kind == TypeKind.RECORD:
                    decl = canonical.get_declaration()
                    if decl and not decl.spelling:
                        # Anonymous struct with typedef name
                        self._typedef_to_struct[child.spelling] = child.spelling
                    elif decl and decl.spelling:
                        self._typedef_to_struct[child.spelling] = decl.spelling
                        # Reverse: struct name -> typedef (prefer shorter/cleaner names)
                        existing = self._struct_to_typedef.get(decl.spelling)
                        if not existing or len(child.spelling) < len(existing):
                            self._struct_to_typedef[decl.spelling] = child.spelling
                elif canonical.kind == TypeKind.ENUM:
                    decl = canonical.get_declaration()
                    if decl and not decl.spelling:
                        self._typedef_to_enum[child.spelling] = child.spelling
                    elif decl and decl.spelling:
                        self._typedef_to_enum[child.spelling] = decl.spelling

    def _walk(self, cursor: Cursor):
        """Walk AST and collect function, struct, enum declarations."""
        for child in cursor.get_children():
            if not self._is_target_file(child):
                continue

            if child.kind == CursorKind.FUNCTION_DECL:
                self._collect_function(child)
            elif child.kind in (CursorKind.STRUCT_DECL, CursorKind.UNION_DECL):
                self._collect_struct(child)
            elif child.kind == CursorKind.ENUM_DECL:
                self._collect_enum(child)
            elif child.kind == CursorKind.TYPEDEF_DECL:
                # Check if this is a typedef for an anonymous struct/enum
                underlying = child.underlying_typedef_type.get_canonical()
                if underlying.kind == TypeKind.RECORD:
                    decl = underlying.get_declaration()
                    if decl and not decl.spelling and decl.is_definition():
                        self._collect_struct(decl, override_name=child.spelling)
                elif underlying.kind == TypeKind.ENUM:
                    decl = underlying.get_declaration()
                    if decl and not decl.spelling:
                        self._collect_enum(decl, override_name=child.spelling)

    def _collect_function(self, cursor: Cursor):
        """Collect a function declaration."""
        name = cursor.spelling
        if not name or name in self._seen_funcs:
            return

        # Skip static (internal linkage) functions
        if cursor.storage_class.name == "STATIC":
            return
        # Skip compiler builtins
        if name.startswith("__builtin") or name.startswith("_"):
            return

        self._seen_funcs.add(name)

        # Get C signature for comment
        c_sig = cursor.type.spelling
        # Better: reconstruct from result + params
        result_type = cursor.result_type
        is_variadic = cursor.type.is_function_variadic()

        # Map return type
        ret_mingus, ret_warn = self._map_type(result_type)

        # Collect parameters
        params = []
        param_cursors = [c for c in cursor.get_children() if c.kind == CursorKind.PARM_DECL]
        for i, pc in enumerate(param_cursors):
            param_name = self._safe_name(pc.spelling or f"p{i}")
            param_mingus, param_warn = self._map_type(pc.type)
            params.append(ParamBinding(
                name=param_name,
                mingus_type=param_mingus,
                c_type=pc.type.spelling,
                warning=param_warn
            ))

        # Build C signature string
        c_params = ", ".join(pc.type.spelling for pc in param_cursors)
        if is_variadic:
            c_params += ", ..." if c_params else "..."
        c_sig_str = f"{result_type.spelling} {name}({c_params})"

        func = FuncBinding(
            name=name,
            return_type=ret_mingus,
            params=params,
            is_variadic=is_variadic,
            c_signature=c_sig_str,
            warning=ret_warn
        )
        self.functions.append(func)

    def _collect_struct(self, cursor: Cursor, override_name: str = None):
        """Collect a struct declaration."""
        name = override_name or cursor.spelling
        if not name:
            return

        # Skip if we already have a concrete definition
        if name in self.structs and not self.structs[name].is_opaque:
            return

        if cursor.is_definition():
            fields = []
            for child in cursor.get_children():
                if child.kind == CursorKind.FIELD_DECL:
                    f_mingus, f_warn = self._map_type(child.type)
                    fields.append(FieldBinding(
                        name=self._safe_name(child.spelling),
                        mingus_type=f_mingus,
                        c_type=child.type.spelling,
                        warning=f_warn
                    ))
            self.structs[name] = StructBinding(
                name=name,
                fields=fields,
                is_opaque=len(fields) == 0
            )
        else:
            # Forward declaration — opaque
            if name not in self.structs:
                self.structs[name] = StructBinding(
                    name=name, fields=[], is_opaque=True
                )

    def _collect_enum(self, cursor: Cursor, override_name: str = None):
        """Collect an enum declaration."""
        name = override_name or cursor.spelling
        if not name:
            return
        # Skip anonymous enums with no typedef name (can't represent in Mingus)
        if name.startswith("(unnamed") or name.startswith("(anonymous"):
            return
        if name in self.enums:
            return

        values = []
        for child in cursor.get_children():
            if child.kind == CursorKind.ENUM_CONSTANT_DECL:
                values.append((child.spelling, child.enum_value))

        # Detect underlying integer type from the enum's canonical type
        enum_int_type = cursor.enum_type
        underlying = "int"
        if enum_int_type:
            ek = enum_int_type.get_canonical().kind
            if ek == TypeKind.UINT:
                underlying = "uint"
            elif ek in (TypeKind.ULONG, TypeKind.ULONGLONG):
                underlying = "ulong"
            elif ek in (TypeKind.LONG, TypeKind.LONGLONG):
                underlying = "long"
            elif ek == TypeKind.USHORT:
                underlying = "ushort"
            elif ek == TypeKind.SHORT:
                underlying = "short"
            elif ek in (TypeKind.UCHAR, TypeKind.CHAR_U):
                underlying = "byte"
            elif ek in (TypeKind.SCHAR, TypeKind.CHAR_S):
                underlying = "char"

        if values:
            self.enums[name] = EnumBinding(name=name, values=values,
                                           underlying_type=underlying)

    # ── Helpers ────────────────────────────────────────────────────────

    def _safe_name(self, name: str) -> str:
        """Ensure a name doesn't clash with Mingus keywords."""
        if name in self.MINGUS_KEYWORDS:
            return name + "_"
        return name

    # ── Type Mapping ─────────────────────────────────────────────────────

    def _map_type(self, clang_type) -> tuple[str, str]:
        """
        Map a libclang Type to a Mingus type string.
        Returns (mingus_type, warning_message).
        """
        canonical = clang_type.get_canonical()
        kind = canonical.kind

        # Void
        if kind == TypeKind.VOID:
            return ("void", "")

        # Bool
        if kind == TypeKind.BOOL:
            return ("bool", "")

        # 8-bit: signed char → char, unsigned char → byte
        if kind in (TypeKind.CHAR_S, TypeKind.SCHAR):
            return ("char", "")
        if kind in (TypeKind.CHAR_U, TypeKind.UCHAR):
            return ("byte", "")

        # 16-bit: short → short, unsigned short → ushort
        if kind == TypeKind.SHORT:
            return ("short", "")
        if kind == TypeKind.USHORT:
            return ("ushort", "")

        # 32-bit: int → int, unsigned int → uint
        if kind == TypeKind.INT:
            return ("int", "")
        if kind == TypeKind.UINT:
            return ("uint", "")

        # Platform-dependent: C long (32-bit on Windows LLP64, 64-bit on Linux LP64)
        if kind == TypeKind.LONG:
            return ("int", "C long — 64-bit on LP64 platforms (Linux/macOS)")
        if kind == TypeKind.ULONG:
            return ("uint", "C unsigned long — 64-bit on LP64 platforms (Linux/macOS)")

        # 64-bit: long long → long, unsigned long long → ulong
        if kind == TypeKind.LONGLONG:
            return ("long", "")
        if kind == TypeKind.ULONGLONG:
            return ("ulong", "")

        # Float / Double
        if kind == TypeKind.FLOAT:
            return ("float", "")
        if kind == TypeKind.DOUBLE:
            return ("double", "")
        if kind == TypeKind.LONGDOUBLE:
            return ("double", "long double mapped to double(f64)")

        # Pointer types
        if kind == TypeKind.POINTER:
            return self._map_pointer_type(canonical)

        # Function prototype (non-pointer — rare at top level)
        if kind == TypeKind.FUNCTIONPROTO:
            return (self._map_function_proto(canonical), "")

        # Record (struct/union passed by value)
        if kind == TypeKind.RECORD:
            decl = canonical.get_declaration()
            name = decl.spelling if decl else canonical.spelling
            # Check typedef mapping for anonymous structs
            if not name:
                for tname, sname in self._typedef_to_struct.items():
                    if sname == name:
                        name = tname
                        break
            if name:
                return (name, "")
            return ("byte*", "anonymous struct mapped to byte*")

        # Enum (by value) — use the enum's name if available
        if kind == TypeKind.ENUM:
            decl = canonical.get_declaration()
            enum_name = decl.spelling if decl else ""
            # Check typedef mapping for anonymous enums
            if not enum_name:
                for tname, ename in self._typedef_to_enum.items():
                    if ename == "":
                        enum_name = tname
                        break
            if enum_name and enum_name in self.enums:
                return (enum_name, "")
            return ("int", "")

        # Fixed-size array → Mingus array type (e.g., int[8], byte[32])
        if kind == TypeKind.CONSTANTARRAY:
            elem_type, warn = self._map_type(canonical.element_type)
            count = canonical.element_count
            return (f"{elem_type}[{count}]", warn)

        # Incomplete array (e.g. int[])
        if kind == TypeKind.INCOMPLETEARRAY:
            return ("byte*", "incomplete array mapped to byte*")

        # Elaborated (e.g. "struct Foo" — resolve through canonical)
        if kind == TypeKind.ELABORATED:
            return self._map_type(canonical)

        # Fallback
        return ("byte*", f"unmapped C type: {clang_type.spelling}")

    def _map_pointer_type(self, ptr_type) -> tuple[str, str]:
        """Map a pointer type to Mingus."""
        pointee = ptr_type.get_pointee()
        pointee_canon = pointee.get_canonical()
        pk = pointee_canon.kind

        # char * / const char * → string
        if pk in (TypeKind.CHAR_S, TypeKind.SCHAR):
            return ("string", "")
        # const unsigned char pointer check
        if pk in (TypeKind.CHAR_U, TypeKind.UCHAR):
            # Could be string-like or byte buffer — default to byte*
            return ("byte*", "")

        # void * → byte*
        if pk == TypeKind.VOID:
            return ("byte*", "")

        # Function pointer → Mingus closure type
        if pk == TypeKind.FUNCTIONPROTO:
            return (self._map_function_proto(pointee_canon), "")

        # Struct pointer
        if pk == TypeKind.RECORD:
            decl = pointee_canon.get_declaration()
            raw_name = decl.spelling if decl else ""
            # Prefer typedef name over internal struct name (e.g. FILE over _iobuf)
            name = self._struct_to_typedef.get(raw_name, raw_name)
            # Check typedef mappings for anonymous structs
            if not name:
                for tname, sname in self._typedef_to_struct.items():
                    if sname == "":
                        name = tname
                        break
            if name:
                # Register as opaque if not already known
                if name not in self.structs:
                    self.structs[name] = StructBinding(
                        name=name, fields=[], is_opaque=True
                    )
                return (f"{name}*", "")
            return ("byte*", "anonymous struct pointer")

        # Pointer to pointer → recurse for T**
        if pk == TypeKind.POINTER:
            inner, inner_warn = self._map_pointer_type(pointee_canon)
            if inner.endswith("*"):
                return (f"{inner}*", inner_warn)
            return ("byte*", f"pointer-to-pointer ({pointee.spelling}*)")

        # Pointer to integer types → typed pointer (int*, uint*, short*, etc.)
        int_pointer_map = {
            TypeKind.INT: "int*",
            TypeKind.UINT: "uint*",
            TypeKind.SHORT: "short*",
            TypeKind.USHORT: "ushort*",
            TypeKind.LONG: "int*",       # C long = 32-bit on Windows LLP64
            TypeKind.ULONG: "uint*",
            TypeKind.LONGLONG: "long*",
            TypeKind.ULONGLONG: "ulong*",
        }
        if pk in int_pointer_map:
            return (int_pointer_map[pk], "")

        # Pointer to float/double
        if pk == TypeKind.FLOAT:
            return ("float*", "")
        if pk == TypeKind.DOUBLE:
            return ("double*", "")

        # Pointer to bool
        if pk == TypeKind.BOOL:
            return ("bool*", "")

        # Any other pointer → byte*
        return ("byte*", f"pointer to {pointee.spelling}")

    def _map_function_proto(self, func_type) -> str:
        """Map a function prototype to Mingus closure syntax: (params) => ret."""
        result, _ = self._map_type(func_type.get_result())
        param_types = []
        for arg_type in func_type.argument_types():
            mapped, _ = self._map_type(arg_type)
            param_types.append(mapped)

        params_str = ", ".join(param_types)
        return f"({params_str}) => {result}"

    # ── Code Generation ──────────────────────────────────────────────────

    def generate(self, module_name: str, source_files: list[str] = None,
                 include_structs: bool = True, include_enums: bool = True,
                 prefixes: list[str] = None) -> str:
        """Generate complete Mingus binding source code."""
        lines = []

        # Header comment
        if source_files:
            file_list = ", ".join(Path(f).name for f in source_files)
            lines.append(f"// Auto-generated Mingus bindings for {file_list}")
            lines.append(f"// Source: {file_list}")
        lines.append("// Generated by mingus_bind_gen.py")
        lines.append("")
        lines.append(f"module {module_name}")
        lines.append("{")

        # Filter by prefix if specified
        funcs = self.functions
        structs = dict(self.structs)
        enums = dict(self.enums)

        if prefixes:
            funcs = [f for f in funcs
                     if any(f.name.startswith(p) for p in prefixes)]
            structs = {k: v for k, v in structs.items()
                       if any(k.startswith(p) for p in prefixes) or
                       self._struct_used_by_funcs(k, funcs)}
            enums = {k: v for k, v in enums.items()
                     if any(k.startswith(p) for p in prefixes) or
                     self._enum_used_by_funcs(k, funcs)}

        # Collect what we'll actually emit
        opaque_types = {k: v for k, v in structs.items() if v.is_opaque} if include_structs else {}
        concrete_structs = {k: v for k, v in structs.items() if not v.is_opaque} if include_structs else {}
        emit_enums = enums if include_enums else {}

        has_content = opaque_types or concrete_structs or emit_enums or funcs
        if not has_content:
            lines.append("    // No declarations found")
            lines.append("}")
            return "\n".join(lines)

        lines.append("    extern {")

        # Opaque types
        if opaque_types:
            lines.append("        // --- Opaque Types ---")
            for name in sorted(opaque_types.keys()):
                lines.append(f"        opaque {name};")
            lines.append("")

        # Concrete structs
        if concrete_structs:
            lines.append("        // --- Structs ---")
            for name in sorted(concrete_structs.keys()):
                sb = concrete_structs[name]
                lines.append(f"        struct {name} {{")
                for fld in sb.fields:
                    warn_comment = f"  // WARNING: {fld.warning}" if fld.warning else ""
                    lines.append(f"            {fld.mingus_type} {fld.name};{warn_comment}")
                lines.append("        }")
            lines.append("")

        # Enums
        if emit_enums:
            lines.append("        // --- Enums ---")
            for name in sorted(emit_enums.keys()):
                eb = emit_enums[name]
                lines.append(f"        enum {name} : {eb.underlying_type} {{")
                for i, (vname, vval) in enumerate(eb.values):
                    comma = "," if i < len(eb.values) - 1 else ""
                    lines.append(f"            {vname} = {vval}{comma}")
                lines.append("        }")
            lines.append("")

        # Functions
        if funcs:
            lines.append("        // --- Functions ---")
            for func in funcs:
                # Comment with original C signature
                lines.append(f"        // {func.c_signature}")
                if func.warning:
                    lines.append(f"        // WARNING: return type — {func.warning}")
                for p in func.params:
                    if p.warning:
                        lines.append(f"        // WARNING: param '{p.name}' — {p.warning}")

                # Mingus declaration
                params_parts = []
                for p in func.params:
                    params_parts.append(f"{p.mingus_type} {p.name}")
                if func.is_variadic:
                    params_parts.append("...")
                params_str = ", ".join(params_parts)

                lines.append(f"        func {func.name}({params_str}) => {func.return_type};")
                lines.append("")

        lines.append("    }")
        lines.append("}")
        lines.append("")

        return "\n".join(lines)

    def _struct_used_by_funcs(self, struct_name: str, funcs: list[FuncBinding]) -> bool:
        """Check if a struct is referenced by any of the given functions."""
        for f in funcs:
            if struct_name in f.return_type:
                return True
            for p in f.params:
                if struct_name in p.mingus_type:
                    return True
        return False

    def _enum_used_by_funcs(self, enum_name: str, funcs: list[FuncBinding]) -> bool:
        """Check if an enum is referenced by any of the given functions."""
        for f in funcs:
            for p in f.params:
                if enum_name in p.c_type:
                    return True
        return False

    def print_summary(self):
        """Print a summary of collected declarations to stderr."""
        opaque = sum(1 for s in self.structs.values() if s.is_opaque)
        concrete = sum(1 for s in self.structs.values() if not s.is_opaque)
        print(f"\n  Collected: {len(self.functions)} functions, "
              f"{concrete} structs, {opaque} opaque types, "
              f"{len(self.enums)} enums", file=sys.stderr)


# ── Helpers ──────────────────────────────────────────────────────────────────

def module_name_from_file(filepath: str) -> str:
    """Derive a PascalCase module name from a filename."""
    stem = Path(filepath).stem
    # Remove common prefixes/suffixes
    stem = re.sub(r'^(lib|stb_)', '', stem)
    # Convert snake_case / kebab-case to PascalCase
    parts = re.split(r'[_\-]+', stem)
    return "".join(p.capitalize() for p in parts if p)


# ── CLI ──────────────────────────────────────────────────────────────────────

def main():
    import argparse
    parser = argparse.ArgumentParser(
        description="Generate Mingus FFI bindings from C header files",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s stb_image_write.h
  %(prog)s SDL2/SDL.h -I /usr/include --prefix SDL_
  %(prog)s mylib.h --module MyLib -o bindings.mingus
        """
    )
    parser.add_argument("path", help="C header file or directory to generate bindings for")
    parser.add_argument("--include-dir", "-I", action="append", default=[],
                        help="Add include search path (repeatable)")
    parser.add_argument("--module", "-m", type=str, default=None,
                        help="Module name (default: derived from filename)")
    parser.add_argument("--output", "-o", type=str, default=None,
                        help="Write output to file instead of stdout")
    parser.add_argument("--prefix", action="append", default=[],
                        help="Only include symbols matching prefix (repeatable)")
    parser.add_argument("--no-structs", action="store_true",
                        help="Skip struct generation")
    parser.add_argument("--no-enums", action="store_true",
                        help="Skip enum generation")
    parser.add_argument("--compile-args", nargs="*", default=[],
                        help="Additional compiler arguments")

    args = parser.parse_args()
    path = Path(args.path).resolve()

    # Build compile args
    compile_args = []
    for inc in args.include_dir:
        compile_args.append(f"-I{Path(inc).resolve()}")
    compile_args.extend(args.compile_args)

    gen = MingusBindingGenerator(compile_args=compile_args)

    source_files = []
    if path.is_dir():
        gen.parse_directory(str(path))
        source_files = [str(f) for f in sorted(path.rglob("*.h"))]
    else:
        print(f"Parsing {path.name}...", file=sys.stderr)
        gen.parse(str(path))
        source_files = [str(path)]

    gen.print_summary()

    # Determine module name
    if args.module:
        mod_name = args.module
    elif path.is_file():
        mod_name = module_name_from_file(str(path))
    else:
        mod_name = module_name_from_file(str(path.name))

    # Generate
    output = gen.generate(
        module_name=mod_name,
        source_files=source_files,
        include_structs=not args.no_structs,
        include_enums=not args.no_enums,
        prefixes=args.prefix if args.prefix else None
    )

    # Output
    if args.output:
        Path(args.output).write_text(output, encoding="utf-8")
        print(f"\nWrote bindings to {args.output}", file=sys.stderr)
    else:
        print(output)


if __name__ == "__main__":
    main()
