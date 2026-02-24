# Mingus FFI Improvement Phases

Roadmap for making real C libraries (SDL2, SQLite3, POSIX, OpenGL, Win32, stb) usable from Mingus.

Based on analysis of what the compiler already supports vs what's truly missing (Feb 2026).

---

## Current State Summary

### Already Fully Working
- Address-of `&x` and dereference `*p` (test_08)
- Typed pointers as function params (`int*`, `Foo*`)
- Opaque pointer types in extern blocks
- Extern functions (including varargs)
- Extern structs with primitive fields
- Extern enums with underlying type

### Implemented but Untested (may have edge-case bugs)
- Multi-level pointers (`int**`, `byte***`) — full pipeline, zero tests
- Array fields in extern structs (`byte[8] data;`) — full pipeline, zero tests
- Nested extern structs (`Outer { Inner pos; }`) — works if declared in order, zero tests
- Function pointer fields in extern structs — full pipeline, zero tests

### Truly Missing
- Union types (no grammar, AST, symbols, or codegen)
- Module-level variable codegen (grammar parses, but IRGenerator only creates stack allocas)
- Extern variable declarations (no grammar rule)
- Static local variables (`isStatic` on AST node is ignored)
- `#define` constant extraction in binding generator
- Layout annotations (`@packed`, `@align`)
- Platform-sized type aliases (`size_t`, `uintptr_t`)
- Configurable target triple and DataLayout

---

## Phase 1: Test and Fix What Exists

**Goal:** Verify that multi-level pointers, array fields, nested extern structs, and function pointer struct fields actually work end-to-end. Fix any edge-case bugs.

**No new language features needed — only tests.**

### Test Cases

1. **Multi-level pointers**
   - `int** pp` — pointer to pointer to int
   - Pass `int*` output param to extern C function (e.g., address-of local)
   - `char**` (pointer to string array) if possible

2. **Array fields in extern structs**
   - `extern { struct Buf { byte[8] data; int len; } }`
   - Read/write array elements through struct field

3. **Nested extern structs**
   - `extern { struct Inner { int x; int y; } struct Outer { Inner pos; int z; } }`
   - Access nested fields: `outer.pos.x`

4. **Function pointer fields in extern structs**
   - `extern { struct Ops { (int, int) => int apply; } }`
   - Assign a lambda and call through the struct field

5. **Typed pointer output params to C functions**
   - Pass `&localVar` to an extern function expecting `int*`
   - Verify the extern function can write through the pointer

### Expected Outcome
Identify which features work cleanly, which have bugs, and what edge cases need fixing before building on top of them.

### Binding Generator Fixes (parallel)
- Emit `int*` instead of `byte*` for typed pointer params
- Emit `int**` for double-pointer params
- Emit array field syntax (`byte[8]` not `byte*`)
- Emit correct enum underlying types (not hardcoded `int`)

---

## Phase 2: Module-Level Variables + Extern Globals

**Goal:** Enable `errno`, `stdin`/`stdout`/`stderr`, and user-defined module state.

### Language Changes

1. **Module-level variable codegen**
   - Grammar already parses `variableDeclaration` at module level
   - IRGenerator must emit `llvm::GlobalVariable` (not alloca) when scope is module-level
   - Support initialization (constant initializers for now)
   - Default visibility: public (as decided)

2. **Extern variable declarations**
   - New grammar rule: `externVariableDeclaration` inside `externMember`
   - Syntax: `extern { int errno; }` or `extern int errno;`
   - LLVM: `@errno = external global i32`
   - Read/write access from Mingus code

3. **Static local variables** (lower priority)
   - `static int counter = 0;` inside function body
   - LLVM: `@funcName.counter = internal global i32 0`
   - `VariableDeclaration::isStatic` already exists on the AST node — needs codegen

4. **Visibility modifiers on module-level declarations**
   - `public` (default), `private` (module-internal)
   - LLVM linkage: `external` vs `internal`

### Unlocks
- `errno` access
- `stdin`/`stdout`/`stderr` (as extern `FILE*` globals)
- Module-scoped mutable state
- Persistent counters, caches, singletons

---

## Phase 3: Untagged Unions

**Goal:** Enable SDL2 `SDL_Event`, Win32 `LARGE_INTEGER`, and other C union types.

### Language Changes (new feature from scratch)

1. **Grammar**
   - New keyword: `union`
   - Syntax: `union Name { Type1 field1; Type2 field2; ... }`
   - Also valid inside `extern { }` blocks: `extern { union Name { ... } }`

2. **AST**
   - New node: `UnionDeclaration` (similar to `StructDeclaration`)
   - Fields are overlapping, not sequential

3. **Symbols**
   - New symbol: `UnionSymbol` (or extend `StructSymbol` with `isUnion` flag)
   - Size = max(field sizes), alignment = max(field alignments)

4. **Type Resolution**
   - Register union types in SymbolTable
   - Field access returns field type but all at offset 0

5. **Codegen**
   - LLVM struct type sized to largest member
   - All field accesses use `bitcast` / pointer to offset 0
   - Example: `union { int i; float f; }` becomes `{ [4 x i8] }` with appropriate casts for field access

6. **Binding generator**
   - Distinguish `UNION_DECL` from `STRUCT_DECL`
   - Emit `union` keyword instead of `struct`

### Extern Unions
- `extern { union Value { int i; float f; } }` — C-compatible layout, no RAII

### Unlocks
- `SDL_Event` (discriminated by `type` field, matched manually)
- Win32 `LARGE_INTEGER`, `OVERLAPPED`
- POSIX `sockaddr` family (through union + cast patterns)

---

## Phase 4: `#define` Constant Extraction

**Goal:** Make flag-based C APIs usable (SDL init flags, OpenGL constants, POSIX constants).

### Binding Generator Enhancement

1. **Macro extraction strategy**
   - libclang cannot see `#define` after preprocessing
   - Options:
     a. Parse raw header text with regex for `#define NAME integer_expr` patterns
     b. Use `clang -dM -E` to dump all macros, then filter by prefix
     c. Use libclang's token-level API to find macro definitions
   - Option (b) is most reliable

2. **Output format**
   - Emit `const uint SDL_INIT_VIDEO = 0x00000020;` at module level
   - Choose appropriate type based on value (int for small, uint for large positive, long for 64-bit)
   - Group by prefix with comments

3. **Filtering**
   - Only extract macros from target headers (not system headers)
   - Skip function-like macros
   - Skip macros that expand to non-integer expressions
   - Respect `--prefix` filter

### Unlocks
- SDL2 init flags, event types, key codes, window flags
- OpenGL constants (GL_TRIANGLES, GL_FLOAT, etc.) — 500+ constants
- POSIX constants (O_RDONLY, AF_INET, SOCK_STREAM, etc.)
- Win32 message IDs, style flags, error codes

---

## Phase 5: Tagged Unions

**Goal:** Safe discriminated unions with compiler-managed tags, integrated with `match`.

### Language Changes

1. **Grammar**
   - Keyword: `tagged union` (distinct from plain `union`)
   - Syntax:
     ```
     tagged union Result {
         Ok(int value),
         Err(string message)
     }
     ```

2. **AST**
   - New node: `TaggedUnionDeclaration`
   - Variants with optional payload types

3. **Codegen**
   - LLVM: `{ i32 tag, [max_payload x i8] }`
   - Constructor functions for each variant
   - `match` integration with automatic tag checking and payload extraction

4. **Type Safety**
   - Cannot access payload without matching
   - Exhaustiveness checking in `match` expressions

### Unlocks
- Idiomatic Mingus error handling (`Result<T, E>` pattern)
- Type-safe alternatives to C-style tagged unions
- Pattern matching on variant data

---

## Phase 6: Layout Annotations + Platform Types

**Goal:** Correct struct layout for binary protocols, hardware registers, and cross-platform types.

### Layout Annotations

1. **Attribute syntax: `@attribute`**
   - `@packed` — no padding between fields (`llvm::StructType::setBody(types, /*isPacked=*/true)`)
   - `@align(N)` — minimum alignment for the type
   - Applicable to `struct`, `union`, `extern struct`, `extern union`

2. **Grammar**
   - New `attribute` rule: `AtSign Identifier ( OpenParen expression CloseParen )?`
   - Attributes precede declarations: `@packed struct Header { ... }`

3. **Implementation**
   - Store attributes on AST declaration nodes
   - Codegen reads attributes when building LLVM struct types

### Platform-Sized Type Aliases

1. **Built-in type aliases based on target pointer width**
   - `size_t` = `uint` (32-bit target) or `ulong` (64-bit target)
   - `ssize_t` = `int` (32-bit) or `long` (64-bit)
   - `intptr_t` = `int` (32-bit) or `long` (64-bit)
   - `uintptr_t` = `uint` (32-bit) or `ulong` (64-bit)

2. **Configurable target triple**
   - CLI flag: `--target x86_64-pc-linux-gnu`
   - Default: host platform
   - Sets both LLVM target triple and DataLayout

3. **DataLayout**
   - Set `module->setDataLayout(...)` from target machine
   - Ensures correct struct layout, alignment, and sizeof calculations

### Unlocks
- Packed structs for network protocols, binary file formats
- `size_t` in `malloc`, `strlen`, `fread`, etc.
- `WPARAM`/`LPARAM` (pointer-sized) for Win32
- Cross-compilation correctness

---

## Phase 7: Calling Conventions + Const Type Qualifier

**Goal:** Win32 API support and const-correctness across FFI boundary.

### Calling Conventions

1. **Syntax:** Attribute on extern functions
   - `@stdcall extern func MessageBoxA(...) => int;`
   - Or: `extern @stdcall { func ... }`

2. **Implementation**
   - Store calling convention on `FunctionSymbol`
   - Set `llvm::Function::setCallingConv()` during codegen

3. **Supported conventions**
   - `@cdecl` (default)
   - `@stdcall` (Win32 API)
   - `@fastcall` (optional)

### Const Type Qualifier

1. **Simple model: `const` on pointed-to type only**
   - `const int* p` — pointer to immutable int
   - No `int* const` syntax (use `const` variable instead)
   - No const member functions

2. **Implementation**
   - `ConstQualifiedType` wrapper in type system
   - TypeChecker prevents writes through `const T*`
   - No LLVM IR impact (const is purely a semantic check)

### Unlocks
- All Win32 API functions (32-bit target)
- Const-safety documentation in function signatures
- Better C API fidelity in bindings

---

## Cross-Cutting: Binding Generator Updates

Each phase has corresponding binding generator improvements:

| Phase | Generator Update |
|-------|-----------------|
| 1 | Emit `int*` not `byte*`; emit `int**`; emit `byte[8]` array fields; fix enum types |
| 2 | Emit `extern int errno;` for global variables |
| 3 | Emit `union` for `UNION_DECL`; correct union sizing |
| 4 | Extract `#define` constants via `clang -dM -E` |
| 5 | (No generator change — tagged unions are Mingus-only) |
| 6 | Emit `@packed` when C struct has `__attribute__((packed))`; emit `size_t` |
| 7 | Emit `@stdcall` annotations; emit `const int*` for `const` params |

---

## Library Unlock Timeline

| Library | Usable After | Key Dependency |
|---------|-------------|----------------|
| **stb_image** | Phase 1 | `int*` output params (already works, needs testing) |
| **SQLite3** | Phase 1-2 | `sqlite3**` double pointer, extern globals |
| **POSIX basic I/O** | Phase 2 | `errno`, `stdin`/`stdout` globals |
| **SDL2** | Phase 3-4 | `SDL_Event` union, `#define` constants |
| **OpenGL** | Phase 4 | 500+ `#define` constants |
| **Win32** | Phase 3-6 | Unions, `#define` constants, `size_t`, `@stdcall` |

---

## Design Decisions (locked in)

- **Attribute syntax:** `@attribute` (Java/Kotlin style)
- **Module default visibility:** public
- **Union model:** both untagged (Phase 3) and tagged (Phase 5)
- **Const model:** simple — `const T*` only, no `T* const` or const methods
- **Target:** configurable via CLI, default to host platform
