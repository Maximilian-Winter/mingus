# C FFI

Mingus has a comprehensive C Foreign Function Interface for calling C libraries, using C types, and interoperating with C data structures. The FFI system covers extern functions, opaque types, extern structs, extern enums, extern unions, calling conventions, link directives, and packed layout.

## Extern Functions

Import C functions with `extern func`:

```mingus
extern func printf(string fmt, ...) => int;
extern func puts(string s) => int;
extern func malloc(int size) => byte*;
extern func free(byte* ptr) => void;
extern func strlen(string s) => size_t;
extern func memcmp(const byte* a, const byte* b, size_t n) => int;
```

Extern functions follow C calling conventions. Variadic functions use `...`:

```mingus
extern func printf(string fmt, ...) => int;

printf("int: %d\n", 42);
printf("double: %f\n", 3.14);
printf("mixed: %d %f %s\n", 1, 2.718, "hello");
```

Small integer types (`byte`, `short`) are promoted to `int` in varargs calls, and `float` is promoted to `double`.

## Extern Blocks

Group multiple extern declarations in a block:

```mingus
extern {
    func fopen(string path, string mode) => FILE*;
    func fclose(FILE* fp) => int;
    func fputs(string s, FILE* fp) => int;
    func fflush(FILE* fp) => int;
}
```

## Opaque Types

For C types where you only need a pointer (the internal structure is hidden):

```mingus
extern {
    opaque FILE;
}

var fp = fopen("output.txt", "w");
if (fp != null)
{
    fputs("hello from Mingus\n", fp);
    fclose(fp);
}
```

Opaque types can only be used through pointers (`FILE*`). You cannot create instances, access fields, or determine their size.

## Extern Structs

Define C-compatible struct layouts:

```mingus
extern {
    struct Point {
        int x;
        int y;
    }

    struct IntBuffer {
        int[4] data;
        int count;
    }

    struct Vec2 {
        int x;
        int y;
    }

    struct Rect {
        Vec2 pos;     // Nested extern struct
        Vec2 size;
    }
}
```

Extern structs use C memory layout (no Mingus RAII, no constructor/destructor):

```mingus
Point p;
p.x = 42;
p.y = 99;
printf("Point(%d, %d)\n", p.x, p.y);

// Nested struct access
Rect r;
r.pos.x = 10;
r.pos.y = 20;
r.size.x = 640;
r.size.y = 480;
```

### Array Fields in Extern Structs

```mingus
IntBuffer buf;
buf.data[0] = 100;
buf.data[1] = 200;
buf.data[2] = 300;
buf.data[3] = 400;
buf.count = 4;
```

## Extern Enums

Define C-compatible enumerations:

```mingus
extern {
    enum Color : int {
        Red = 0,
        Green = 1,
        Blue = 2
    }
}

var c = Color.Green;
printf("Color.Green = %d\n", c);   // 1
```

Extern enums use dot syntax (`Color.Green`), just like Mingus-native enums.

## Extern Unions

Define C-compatible unions:

```mingus
extern {
    union RawValue {
        int asInt;
        float asFloat;
        double asDouble;
    }
}

RawValue rv;
rv.asInt = 100;
printf("int: %d\n", rv.asInt);

rv.asDouble = 2.718;
printf("double: %f\n", rv.asDouble);
```

## Link Directives

Specify libraries to link against:

```mingus
extern link "SDL2";
extern link "sqlite3";
```

This is equivalent to passing `--link SDL2` on the command line.

## Packed Structs

Use `@packed` to eliminate padding:

```mingus
@packed
struct PackedPoint
{
    byte x;
    int y;
    byte z;
}

printf("sizeof(PackedPoint) = %d\n", sizeof(PackedPoint));   // 6 (no padding)
```

Without `@packed`, the same struct would be 12 bytes due to alignment padding.

## Calling Conventions

Specify calling conventions for extern functions:

```mingus
extern {
    @stdcall
    func MessageBoxA(int hwnd, string text, string caption, int type) => int;

    @cdecl
    func printf(string fmt, ...) => int;

    @fastcall
    func fast_compute(int a, int b) => int;
}
```

| Attribute | Calling Convention |
|-----------|-------------------|
| `@cdecl` | C default (caller cleans stack) |
| `@stdcall` | Windows API convention (callee cleans stack) |
| `@fastcall` | First two args in registers |

On x86_64, these are mostly equivalent, but they matter for 32-bit targets and Windows API interop.

## Const Pointers in FFI

Many C functions take `const` pointers:

```mingus
extern func memcmp(const byte* a, const byte* b, size_t n) => int;
extern func strlen(string s) => size_t;
```

Mingus enforces const semantics:
- `T*` can be implicitly widened to `const T*`
- `const T*` cannot be implicitly narrowed to `T*`
- `string` is compatible with both `byte*` and `const byte*`

## Platform Types

For C interoperability, use pointer-width types:

```mingus
extern func strlen(string s) => size_t;
extern func memcpy(byte* dest, byte* src, size_t n) => byte*;

size_t len = strlen("hello");
printf("strlen = %llu\n", len);   // 5
```

See [Types and Values](02_types_and_values.md) for the full list of platform types.

## Compiler Flags for FFI

| Flag | Description | Example |
|------|-------------|---------|
| `--link <lib>` | Link against a library | `--link sqlite3` |
| `--lib-path <dir>` | Add library search path | `--lib-path ./lib` |
| `--include-path <dir>` | Add include path | `--include-path ./include` |

### Full Build Example

```bash
# Compile Mingus source with FFI
mingus_v2_tool.exe app.mingus --emit app.ll --entry App_main --opt 2

# Link against C library
clang -O2 -o app.exe app.ll -lsqlite3 -L./lib
```

## Complete FFI Example

A real-world example using SQLite3:

```mingus
module SQLiteDemo
{
    extern func printf(string fmt, ...) => int;

    extern {
        opaque sqlite3;
        opaque sqlite3_stmt;

        func sqlite3_open(string filename, sqlite3** ppDb) => int;
        func sqlite3_close(sqlite3* db) => int;
        func sqlite3_prepare_v2(sqlite3* db, string sql, int nByte,
                                sqlite3_stmt** ppStmt, byte** pzTail) => int;
        func sqlite3_step(sqlite3_stmt* stmt) => int;
        func sqlite3_finalize(sqlite3_stmt* stmt) => int;
        func sqlite3_column_int(sqlite3_stmt* stmt, int col) => int;
        func sqlite3_column_double(sqlite3_stmt* stmt, int col) => double;
        func sqlite3_errmsg(sqlite3* db) => const byte*;
    }

    const int SQLITE_OK = 0;
    const int SQLITE_ROW = 100;

    func main() => int
    {
        sqlite3* db = null;
        int rc = sqlite3_open(":memory:", &db);
        if (rc != SQLITE_OK)
        {
            printf("Failed to open DB\n");
            return 1;
        }

        // ... create tables, insert data, query ...

        sqlite3_close(db);
        return 0;
    }
}
```

## Known Limitations

- No automatic header parsing — bindings must be written manually or generated
- Callback parameters (C function pointers) in extern functions require `byte*` workaround
- No `#define` macro import — define constants manually
- No bitfield support in extern structs
- No flexible array members
- No `volatile` qualifier
- Extern structs/enums/unions must be inside `extern { }` blocks, not standalone

## See Also

- [Getting Started](01_getting_started.md) for basic extern function usage
- [Types and Values](02_types_and_values.md) for platform types
- [Pointers and Memory](12_pointers_and_memory.md) for pointer operations and raw blocks
- [Modules and Imports](13_modules_and_imports.md) for import syntax
