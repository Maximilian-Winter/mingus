# Mingus C FFI — Limitations and Blocking Issues

Current state of the FFI system as of February 2026. Covers both the compiler (`mingus_v2_tool`) and the binding generator (`mingus_bind_gen.py`).

---

## Blocking Limitations

These prevent use of many real C libraries.

### 1. No union type

No `union` keyword exists anywhere — lexer, parser, type system, codegen. The binding generator treats `UNION_DECL` as structs (sequential layout instead of overlapping). This alone blocks SDL2 (`SDL_Event`), Win32 (`LARGE_INTEGER`), and most non-trivial C libraries.

**Impact:** SDL2, Win32, POSIX, Xlib, nearly every non-trivial C library.

### 2. No extern global variables

The `externMember` grammar only allows functions, structs, enums, opaque types, and link directives. There is no way to declare `extern int errno;` or `extern FILE* stdin;`. This blocks access to standard streams and many POSIX/Win32 globals.

**Impact:** `errno`, `stdin`/`stdout`/`stderr`, Win32 `_environ`, `optarg`/`optind` (getopt).

### 3. Pointer-to-pointer lost in bindings

`char**`, `void**`, `int**` all collapse to `byte*` in the binding generator (`mingus_bind_gen.py:500-502`). This breaks any C API using output parameters (`sqlite3_open(const char*, sqlite3**)`) or string arrays (`char** argv`).

**Impact:** SQLite3, most APIs with output parameters, `main(int argc, char** argv)`.

### 4. `#define` constants inaccessible

libclang parses after preprocessing — macro constants like `SDL_INIT_VIDEO`, `O_RDONLY`, `SEEK_SET` are invisible to the binding generator. Flag-based APIs are essentially unusable without manually defining these as Mingus constants.

**Impact:** SDL2 init flags, OpenGL constants, POSIX constants, Win32 message IDs.

### 5. Hardcoded Windows target triple

`IRGenerator.cpp:128` hardcodes `x86_64-pc-windows-msvc`. No cross-compilation possible. The C ABI conventions always follow Windows rules even if someone tried to use Mingus on Linux.

**Impact:** Cross-platform use, Linux/macOS targets.

---

## Significant Limitations

Workaroundable but painful.

### 6. Structs always passed by pointer to extern functions

`mapParamType()` in `IRGenerator.cpp:380` converts ALL struct types to `ptr`. C functions expecting small structs by value (e.g., `struct div_t div(int, int)`) get the wrong ABI. Most C APIs pass structs by pointer anyway, but not all.

**Impact:** `div()`, `ldiv()`, some math library functions, certain platform APIs.

### 7. No struct return by value from extern functions

No `sret` attribute annotation on extern function declarations. Functions like `div()` or `ldiv()` that return structs by value could produce incorrect results.

**Impact:** Same as above — any C function returning a struct by value.

### 8. No array fields in extern structs

The grammar supports `int[32] name;` syntax in extern fields, but the binding generator maps `CONSTANTARRAY` to `byte*` and cannot emit it. C structs like `struct sockaddr_in` with `sin_zero[8]` are unrepresentable in generated bindings.

**Impact:** `struct sockaddr_in`, `struct stat`, OpenGL matrix types, any struct with fixed-size arrays.

### 9. Closures with captures cannot be C callbacks

`IRGenerator.cpp:4452-4458` extracts only `fnPtr` from the fat pointer, silently dropping `envPtr`. Only capture-less lambdas work as C callbacks. C APIs with `void* userdata` parameters (SDL, GLFW, qsort_r) cannot use Mingus closures — no compile-time error either.

**Impact:** Any callback-based C API (SDL event handlers, GLFW input, signal handlers).

### 10. No DataLayout on LLVM module

`setDataLayout()` is never called despite `getDataLayout()` being used in 7 places for sizeof/alignof calculations. Could cause subtle struct size/alignment mismatches with MSVC ABI.

**Impact:** Potentially any extern struct with non-trivial alignment requirements.

---

## Moderate Limitations

### 11. No packed structs / alignment control

No `#pragma pack` equivalent. `setBody(fieldTypes)` uses default alignment. Blocks binary protocol structs, USB descriptors, `BITMAPFILEHEADER`.

### 12. No bitfield support

No representation anywhere in the grammar, AST, or codegen. Structs with `unsigned flags : 3;` get wrong layout.

### 13. No calling convention annotations

No `stdcall`/`fastcall` support. Fine on x86_64 (uniform convention) but blocks 32-bit Windows API.

### 14. No `size_t` / `intptr_t` type

No platform-aware pointer-width integer type. Must manually pick `uint` or `ulong` depending on target.

### 15. C `long` mapping is platform-dependent

Binding generator maps C `long` to Mingus `int` (correct on Windows LLP64, wrong on Linux LP64 where `long` is 64-bit). Warning comment is emitted but the generated code would be wrong on Linux.

---

## Inconvenient Limitations

### 16. Anonymous/nested structs in extern structs

Named nested structs work if declared separately, but inline `struct { int x; int y; } pos;` has no representation.

### 17. No const/volatile qualifiers in extern context

`const void*` and `void*` both become `byte*`. No const-correctness across the FFI boundary.

### 18. No typedef inside extern blocks

`typedefDeclaration` is not in the `externMember` grammar production. Must declare outside the extern block.

### 19. Enum underlying type hardcoded to `int` in generator

`mingus_bind_gen.py:592` always emits `enum Name : int`, ignoring the C enum's actual underlying type. The compiler supports other underlying types — only the generator is limited.

### 20. Typed pointers mapped to `byte*` in generator

`mingus_bind_gen.py:504-506` maps `int*`, `uint*`, etc. to `byte*` even though Mingus supports typed pointers like `int*`. Only affects the binding generator, not hand-written bindings.

### 21. `long double` truncated to `double`

80-bit extended precision lost. No Mingus equivalent type exists.

### 22. Inline/static functions silently skipped

Header-only libraries (stb_*) define everything as `static` — must compile the `.c` implementation file separately and link the resulting `.obj`.

### 23. No function-like macro handling

`#define MAX(a,b) ((a)>(b)?(a):(b))` and similar are invisible to libclang and cannot be represented in Mingus bindings.

---

## Real-World Library Assessment

| Library | Status | Key Blocking Issues |
|---------|--------|---------------------|
| **stb_image** | Works | Must compile .c separately (#22) |
| **SQLite3** | Partial | `sqlite3_open` needs `byte*` for `sqlite3**` (#3) |
| **POSIX I/O** | Partial | No `stdin`/`stdout` (#2), `size_t` manual (#14) |
| **SDL2** | Blocked | `SDL_Event` union (#1), constants (#4) |
| **Win32 API** | Blocked | Constants (#4), unions (#1), nested structs (#16) |

---

## Priority Recommendation

The first three blocking issues would unlock the largest number of real C libraries if addressed:

1. **Unions** — required by nearly every non-trivial C library
2. **Extern global variables** — required for `errno`, standard streams, etc.
3. **Pointer-to-pointer support** — required for output parameters and string arrays
