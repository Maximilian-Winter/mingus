# Mingus v1 — Language Status Report

**Date:** February 2026
**Status:** Compiles and executes optimized native binaries — **30 feature tests + 21 stress tests passing (51/51)**

---

## Architecture

```
Source (.mingus) -> ANTLR4 Parser -> AST -> Import Resolution -> Semantic Analysis (4 passes) -> LLVM IR -> Optimized IR -> Native Code
```

| Stage | Implementation |
|-------|----------------|
| Lexer/Parser | ANTLR4 grammar (MingusLexer.g4, MingusParser.g4) |
| AST | 62 node types with full visitor pattern |
| Sema Pass 1 | SymbolTableBuilder — scopes, symbols, type declarations, import resolution |
| Sema Pass 2 | TypeResolver — resolve all type references |
| Sema Pass 3 | TypeChecker — expression types, overload resolution, access modifier enforcement |
| Sema Pass 4 | SemanticValidator — RAII analysis, control flow validation, escape analysis, self-capture detection |
| Codegen | LLVM 21.1.8 IR generation via AST visitor, optional DIBuilder debug info |
| Optimization | LLVM PassBuilder with configurable O0/O1/O2 pipeline |
| Compilation | Clang (from LLVM distribution) compiles IR to native |

**Build System:** CMake + Ninja + MSVC (Windows), CLion IDE or standalone `build.bat`
**Test Runner:** `run_tests.bat` (combined 51 tests), `tests/run_all_tests.bat` (features), `tests/run_stress_tests.bat` (stress) — supports `--code`, `--ir`, `--output` flags
**Showcase:** `examples/showcase.bat` — 9 example programs (including multi-module import demo); `tools/README.md` — compiler and API tools

---

## Compiler Documentation

Detailed technical documentation of the compiler internals:

| Document | Covers |
|----------|--------|
| [GRAMMAR_AND_AST.md](GRAMMAR_AND_AST.md) | Grammar rules, operator precedence, AST node inventory, ASTGenerator mapping |
| [SEMANTIC_ANALYSIS.md](SEMANTIC_ANALYSIS.md) | All 4 sema passes: SymbolTableBuilder, TypeResolver, TypeChecker, SemanticValidator |
| [MEMORY_AND_LIFETIMES.md](MEMORY_AND_LIFETIMES.md) | Stack vs heap, RAII system, closure reference counting, string memory, zero-init |
| [TYPE_SYSTEM_AND_DISPATCH.md](TYPE_SYSTEM_AND_DISPATCH.md) | LLVM type representations, vtables, interface dispatch, fat pointers, operators |
| [CODEGEN_PATTERNS.md](CODEGEN_PATTERNS.md) | Lambda codegen, pipe operator, match expressions, imports, string interpolation |
| [KNOWN_LIMITATIONS.md](KNOWN_LIMITATIONS.md) | All known limitations across grammar, sema, codegen, and runtime |

---

## Working Features

### Verified by test suite (30 feature tests + 21 stress tests = 51/51 passing)

#### 1. Core Language (Test 01)
- Integer types: `int`, `byte`, `bool`
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`
- Logical operators: `&&`, `||`, `!`
- Type inference: `var x = 42;`
- Variable declarations with explicit types
- Assignment operators: `=`, `+=`, `-=`, `*=`, `/=`
- Pre/post increment and decrement: `i++`, `++i`

#### 2. Control Flow (Tests 01, 05)
- `if` / `else` statements
- `for` loops with init, condition, and increment
- `while` loops
- `break` and `continue`
- Nested loops and control flow
- `switch` statements with `case` and `default`

#### 3. Functions (Tests 01-15, 29)
- Function declarations with typed parameters and return types: `func name(int x) => int`
- **Reference parameters** (Test 29): `func swap(int& a, int& b) => void` — callee receives pointer to caller's alloca, writes persist
- Recursive functions
- Extern function declarations for C interop: `extern func printf(string fmt) => int`
- Grouped extern blocks: `extern { func sin(double x) => double; }`
- Module-scoped functions with name mangling (`Module_function`)
- Varargs support for printf/snprintf

#### 4. Structs and Operator Overloading (Tests 02, 11)
- Struct declarations with typed fields
- Field access via dot notation: `v.x`
- Method declarations within structs
- `this` reference in methods
- Operator overloading: `+`, `-`, `*`, `/`, `[]`, `==`, `!=`, `<`, `>`, `<=`, `>=`
- Chained method calls: `a.cross(b).normalize()`
- Chained operator expressions: `(a + b) * 2.5`
- Struct return values from operators and methods
- Mixed operator chains: `a * b / c`
- **Static methods on structs**: `Point.origin()` syntax, no `this` pointer (Test 24)

#### 5. Classes, RAII, Inheritance, and Access Control (Tests 03, 13, 22, 23, 24)
- Class declarations with fields and methods
- Constructors: `constructor(int size) { ... }` (auto-generated if omitted)
- Destructors: `destructor { ... }` (auto-generated if omitted)
- Automatic destructor calls at scope exit (RAII)
- Destructor suppression for returned values
- `new` and `delete` for heap allocation
- Arrow operator for pointer member access: `node->left`
- Recursive destructors (tree cleanup)
- **Access modifier enforcement** (Test 22):
  - `private` members accessible only within the declaring class
  - `protected` members accessible from the class and derived classes (walks `baseClass` chain)
  - `public` and unmodified members accessible from anywhere
  - Enforced in TypeChecker on `MemberAccessExpression` for both fields and methods
- **Single inheritance**: `class Dog : Animal { ... }`
- **Virtual dispatch**: all methods are virtual by default (Java-like), vtable-based dispatch through base pointers
- **Virtual destructors** (Test 23): destructor occupies vtable slot 0; `delete basePtr` dispatches through vtable, ensuring the most-derived destructor runs. Verified with three-level inheritance chains (`Base → Middle → Leaf`).
- **Constructor chaining**: `constructor() : super(4) { ... }` calls base constructor
- **Destructor chaining**: derived destructor body runs first, then base destructor called automatically
- **Inherited field access**: derived classes access base class fields through `this`
- **Polymorphic assignment**: `Derived*` assignable to `Base*`
- **Abstract classes**: `abstract class Shape { ... }` cannot be instantiated; concrete subclasses must override all abstract methods
- **Static methods** (Test 24): `ClassName.staticMethod()` syntax, no `this` pointer, supports recursion (`MathUtils.factorial(n - 1)`)

#### 6. Pipes and Pattern Matching (Tests 04, 11)
- Pipe operator: `sample |> applyGain(0.8) |> softClip`
- Multi-argument piped functions (value becomes first argument)
- Chained pipes with no-argument functions
- Pipe with closure arguments: `10.0 |> apply(doubler)`
- `match` expressions with value result
- Literal patterns: `0 => "zero"`
- Wildcard pattern: `_ => defaultValue`
- Binding patterns with guards: `var x if x > 1.0 => 1.0`
- Ternary expressions inside match arms
- Range patterns in match arms: `1..10 => "small"`
- Match used in return statements

#### 7. Enums (Tests 05, 09, 11)
- Enum declarations with underlying types: `enum TokenKind : int`, `enum Flags : byte`, `enum Method : string`
- Enum member access in match patterns: `TokenKind.Plus => 1`
- Enum member access in expressions: `var r = Color.Red;`
- Enum values as function arguments: `printf("%d", Color.Blue)`
- Enum arithmetic: `SmallFlags.Read + SmallFlags.Write`
- Enum comparison: `r == Color.Red`
- Enum in match expressions (both pattern and value contexts)
- String-backed enums: `enum HttpMethod : string { Get = "GET", ... }`
- Switch statements on integers with constant-case optimization (LLVM `switch` instruction)

#### 8. Lambdas and Higher-Order Functions (Tests 06, 10, 11, 28)
- **Mandatory C++ capture lists**: all lambdas require `[...]` syntax
  - `[]` — no captures
  - `[=]` — all by value (copy at capture time)
  - `[&]` — all by reference (writes persist to outer scope)
  - `[x]` — explicit by value
  - `[&x]` — explicit by reference
  - `[=, &x]` — all by value, specific ones by reference
  - `[&, x]` — all by reference, specific ones by value
- Lambda expressions: `[=](double x) => { return x * 2.0; }`
- Variables holding function values: `var doubler = [=](double x) => { ... }`
- Lambda literal assignment: `f = [=](int x) => { return x * 2; };` (reassign closure variables)
- Higher-order functions: `func apply(double x, (double) => double f) => double`
- Passing lambdas as arguments
- Calling lambdas stored in variables
- Pipe operator with lambda arguments: `7.0 |> apply([](double x) => { return x + 3.0; })`

#### 9. Closures with Captures (Tests 10, 11, 21, 25, 26, 28, 30)
- Fat pointer representation: all function-typed values are `{ fnPtr, envPtr }` structs
- All lambdas receive `ptr %env` as final parameter (uniform calling convention)
- Closures capturing variables from enclosing scope: `func makeScaler(double factor) => (double) => double`
- Heap-allocated capture structs via `malloc` with **reference-counted header** `{ i64 refcount, ptr cleanup_fn, ...fields }`
- Retain/release at assignment boundaries: old closure released before reassignment, new closure retained on field store
- Per-closure cleanup functions for nested closures (releases captured closure envPtrs)
- Indirect calls extract `fnPtr` and `envPtr` from fat pointer, pass env as last arg
- Closures as function arguments and return values
- Composed closures: `func compose((double) => double f, (double) => double g) => (double) => double`
- Closures capturing other closures (fat pointers in capture struct)
- Closures calling module functions from within lambda body
- **By-reference captures** (Tests 28, 30): `[&x]` stores pointer to original alloca in env struct; reads and writes inside the lambda go through to the original variable, enabling stateful closures (counters, accumulators, min/max trackers)
- **Capture-time vs call-time semantics**: `[=]` freezes variable value at capture time; `[&]` sees current value at call time — matching C++ behavior exactly
- **Mixed capture modes**: `[=, &accum]` captures most variables by value but specific ones by reference; `[&, scale]` captures most by reference but specific ones by value
- **Nested lambda capture propagation**: when inner lambdas reference outer-scope variables, all intermediate lambdas in the chain automatically capture them too (semantic validator walks entire lambda stack)
- **Nullable closures**: `(int) => int f = null;` — FunctionType variables can be null-initialized and reassigned
- **Fat pointer null comparison** (Test 21): `f == null`, `f != null`, `null == f` — extracts the `fnPtr` component (index 0) from the fat pointer and compares against null
- **Closures in struct fields**: synthetic cleanup functions release closure fields at scope exit
- **Closures in class fields**: destructor epilogue auto-releases closure fields; constructor auto-zero-inits them
- **Escape analysis** (Test 25): temporary closures passed directly as function arguments are RAII-wrapped to prevent leaks; lambda arguments in `CallExpression` are marked non-escaping by `SemanticValidator`
- **Self-capturing closures** (Test 26): `(int) => int fib = [=](int n) => { return fib(n-1) + fib(n-2); };` — letrec-style indirection: alloca created first, lambda compiled (captures zero fat pointer), final fat pointer stored, then env struct patched with the real self-reference (unretained to avoid cycles)

#### 10. Floating Point and Math (Tests 07, 11)
- `double` and `float` types
- Float arithmetic: `+`, `-`, `*`, `/`
- Math functions via extern: `sin`, `cos`, `sqrt`, `pow`
- Integer-to-float widening: `intVal * 2.0`
- Ternary expressions: `x > 0.0 ? x : -x`
- Unary negation on floats

#### 11. Pointers and Raw Blocks (Test 08)
- Pointer types: `int*`, `byte*`
- Address-of operator: `&arr[0]`
- Dereference operator: `*(ptr + 5)`
- Pointer arithmetic
- `null` literal and null comparisons
- Fixed-size arrays: `int[10] arr`
- Array indexing: `arr[i]`
- `raw` blocks for unsafe operations
- `malloc` / `free` via extern
- Pointer casts: `(int*)malloc(...)`, `(byte*)data`
- `sizeof` operator

#### 12. Multi-Module Imports (Test 12)
- `import` statements to bring symbols from other modules/files
- Whole-module import: `import MathLib;` (imports all public symbols)
- Selective import: `import add, square from MathLib;`
- Aliased import: `import add as myAdd from MathLib;`
- Automatic file discovery: `import X from MathLib;` finds `MathLib.mingus` in same directory
- Transitive imports: imported files can have their own imports
- "Compile together" approach: all modules merged into one LLVM module
- Imported symbols share the same pointer as the original — name mangling, type checking, codegen all work automatically
- Two-sub-pass in SymbolTableBuilder: Pass 1a builds all module scopes, Pass 1b resolves imports

#### 13. Single Inheritance and Virtual Dispatch (Tests 13, 23)
- `class Dog : Animal { ... }` — inherits all fields and methods
- All non-static, non-constructor, non-destructor methods are virtual (vtableIndex assigned, starting at index 1)
- **Vtable slot 0 reserved for destructor**: every class's vtable has its destructor at index 0, methods at index 1+
- Vtable: global constant `[N x ptr]` array, stored in field 0 of class struct when vtableSize > 0
- Virtual dispatch through base pointer: load vtable ptr → GEP to slot → indirect call
- Virtual destructor dispatch: `delete basePtr` loads vtable[0] and calls through it, ensuring derived destructors run
- Three-level inheritance verified: `Base → Middle → Leaf`, delete through `Base*` fires `~Leaf → ~Middle → ~Base`
- `super(args)` constructor chaining: base constructor called before vtable store
- Automatic destructor chaining: derived body runs, then base destructor called
- Inherited field access via `this` in derived classes
- `Derived*` polymorphically assignable to `Base*`
- `abstract class` enforcement: abstract classes cannot be instantiated; concrete subclasses must override all abstract methods
- `isSubclassOf()` in TypeRegistry walks `baseClass` chain for compatibility checking

#### 14. String Operations (Test 14)
- String literals and variables: `var s = "hello";`
- **Concatenation**: `var greeting = hello + " " + name;`
- **Content comparison**: `if (a == b) { ... }` and `if (a != b) { ... }`
- **Built-in `.length()` method**: `greeting.length()` returns character count as `int`
- **Built-in `.substring(start, len)` method**: `greeting.substring(0, 5)` returns a new string
- **Compound assignment**: `s += " suffix";`
- **Interpolation**: `var msg = "value=${x}";` — embed any expression with `${...}`

#### 15. Interfaces and Fat Pointer Dispatch (Test 15)
- `interface Drawable { func draw() => void; }` — method contract, no fields or bodies
- Classes implement multiple interfaces: `class Circle : Drawable, Resizable { ... }`
- **SemanticValidator** enforces completeness: error if class doesn't implement all interface methods
- **Go-style fat pointer**: `Drawable*` compiles to LLVM `{ ptr, ptr }` — object pointer + itable pointer
- Per-(class, interface) **itable** globals: compile-time constant `[N x ptr]` array of method pointers
- Interface dispatch: `extractvalue fat, 0` → objPtr; `extractvalue fat, 1` → itable; `GEP(itable, methodIdx)` → indirect call
- Reuses `getFatPtrType()` from the closure system — consistent `{ ptr, ptr }` representation throughout
- `delete d` on interface pointer correctly extracts and frees the underlying object
- Passing interface pointer to function: `func renderAll(Drawable* d) => void { d->draw(); }`

#### 16. Tuples and Destructuring (Test 18)
- Tuple return types: `func divmod(int a, int b) => (int, int)`
- Tuple destructuring: `(var quot, var rem) = divmod(17, 5);`
- Mixed-type tuples: `(string, int, bool)`
- Tuple construction via `CreateInsertValue`, destructuring via `CreateExtractValue`
- Works with recursive functions (fibonacci pair returning `(int, int)`)

#### 17. DynamicArray.map and memcpy Interop (Test 19)
- `DynamicArray.map((int) => int transform)` — returns new array with mapped values
- Pipe integration inside class methods: `this[i] |> transform`
- Capacity growth via `memcpy` for efficient buffer reallocation
- Demonstrates closures, pipes, operator overloading, and RAII together

#### 18. Complex Number Arithmetic (Test 20)
- `struct Complex` with `operator+` and `operator*` returning `Complex`
- Operator composition: `(a + b) * b` via chained method calls
- `magnitude_squared()` returning `double`
- Demonstrates struct return values and operator overloading for mathematical types

#### 19. Optimization Pipeline
- LLVM PassBuilder integration with configurable optimization levels
- `--opt 0` (default): no optimization
- `--opt 1`: O1 pipeline (basic simplifications, mem2reg)
- `--opt 2`: O2 pipeline (inlining, GVN, SROA, instcombine, vectorization, DCE)
- Optimization runs between IR generation and LLVM verification
- All tests run with `--opt 2` enabled

#### 20. Debug Information (Test 27)
- Optional `--debug` flag on `mingus_ir_tool`
- LLVM DIBuilder integration: `DICompileUnit`, `DIFile`, `DISubprogram`, `DILocalVariable`
- Function-level debug info: each function gets a `DISubprogram` with subroutine type
- Variable-level debug info: `dbg.declare` intrinsic for local variables and parameters
- Type mapping: Mingus types mapped to DI types (`int` → DW_ATE_signed 32-bit, `double` → DW_ATE_float 64-bit, etc.)
- Source locations: `emitDebugLocation()` calls on all statement visitors
- CodeView format on Windows via `module->addModuleFlag("CodeView", 1)`
- Debug info does not alter runtime behavior — verified by test_27

---

## Known Limitations

### Language Limitations

| Feature | Status | Notes |
|---------|--------|-------|
| **Generics/templates** | Not supported | No generic types or functions. |
| **Multiple class inheritance** | Not supported | `class C : A, B` where both A and B are classes is a sema error. Multiple interface implementation is supported. |

### Codegen Limitations

| Area | Limitation |
|------|------------|
| **Reference lifetime** | `[&x]` captures that escape their scope produce dangling references. This is the programmer's responsibility, same as C++. |
| **Self-capture lifetime** | Self-capturing closures use an unretained self-reference to avoid RC cycles. The closure is valid only while the owning variable is in scope. |
| **Temporary closure leak** | Closures passed directly as function arguments without variable storage leak one refcount. |
| **Closures with struct params** | Closures that take struct-typed parameters have a calling convention mismatch: the generated lambda expects the struct by value (`%Vec2`), but the fat-pointer call site passes a pointer (`ptr`). Workaround: pass struct fields as separate scalar arguments. |
| **Closures with ref params** | Closures that take reference parameters (`int&`) have a similar mismatch: the caller passes the integer value instead of a pointer to the alloca. Workaround: use regular parameters and return the result, or use a non-closure function. |
| **Duplicate cross-module externs** | If two modules both declare the same `extern func` (e.g. `sin`), codegen creates duplicate LLVM declarations that get name-mangled (`sin.3`), causing linker errors. Workaround: declare externs in one module only, import them in others. |
| **ABI** | Struct return by value relies on LLVM's default ABI lowering. Not tested with very large structs. |
| **Error recovery** | Parser and sema generally stop at the first error. No multi-error recovery or cascading diagnostics. |
| **Module visibility** | `public`/`private` on module-level symbols is parsed but only partially enforced (whole-module import skips non-public). No separate compilation or linking — all imported files compiled together. |

---

## Test Suite Summary

| Test | Feature Coverage | Result |
|------|-----------------|--------|
| test_01_basics | int arithmetic, if/else, for, while, nested loops | PASS |
| test_02_structs_operators | struct fields, operator+/*, dot, cross, length, normalize, chaining | PASS |
| test_03_classes_raii | class, constructor, destructor, RAII, new/delete, DynamicArray, tree | PASS |
| test_04_pipes_match | pipe operator, chained pipes, match with guards, wildcard, classification | PASS |
| test_05_enums_switch | enum match patterns, switch statement, boolean match | PASS |
| test_06_lambdas_funcptr | lambdas, higher-order functions, applyTwice, pipe with lambda | PASS |
| test_07_floats_math | float arithmetic, sin/cos/sqrt/pow, widening, ternary | PASS |
| test_08_pointers_raw | stack arrays, pointer arithmetic, raw blocks, malloc/free, null check | PASS |
| test_09_enum_expressions | enum member access in expressions, byte/string enums, arithmetic | PASS |
| test_10_closures | fat pointer closures, compose, apply, applyTwice, pipe with closures | PASS |
| test_11_dsp_showcase | structs+operators, enums+match, closures, pipes, math, composed effects, stereo DSP | PASS |
| test_12_imports | multi-file imports, selective import from MathLib, composed calls across modules | PASS |
| test_13_inheritance | single inheritance, vtable virtual dispatch, super() constructor, polymorphic calls through base pointer, inherited fields, abstract class | PASS |
| test_14_strings | string concatenation (+), content comparison (==, !=), .length(), .substring(), +=, interpolation | PASS |
| test_15_interfaces | interfaces (Drawable, Resizable), multiple implementation per class, fat pointer dispatch, interface pointer as function parameter | PASS |
| test_16_dsp_wav | inheritance + interfaces (Effect, Named) + oscillator classes → WAV file output; demonstrates the full feature stack together | PASS |
| test_17_hex_literals | hex (`0xFF`), binary (`0b1010`), octal (`0o77`) integer literals; bitwise operations | PASS |
| test_18_tuples | tuple return types `(int, int)`, destructuring `(var a, var b) = ...`, mixed-type tuples, recursive fibonacci pair | PASS |
| test_19_dynamic_array_map | DynamicArray with `map()` method, capacity growth via `memcpy`, lambda+pipe integration, operator[] | PASS |
| test_20_complex_numbers | Complex struct with `operator+`, `operator*`, magnitude squared, chained operator expressions | PASS |
| test_21_fat_ptr_null | fat pointer null comparison: `f == null`, `f != null`, `null == f`, after assignment | PASS |
| test_22_access_modifiers | private/protected/public fields and methods, inheritance access, no-modifier default | PASS |
| test_23_virtual_destructor | virtual destructor dispatch through vtable slot 0; two-level and three-level inheritance chains; delete through Base*, Middle*, and direct | PASS |
| test_24_static_methods | `ClassName.staticMethod()` syntax, recursive static, struct static method | PASS |
| test_25_escape_analysis | temporary closure RAII wrapping, named closures, chained calls, non-escaping detection | PASS |
| test_26_self_capture | self-capturing closures: recursive fibonacci, factorial, countdown via letrec-style env patching | PASS |
| test_27_debug_info | `--debug` flag produces correct runtime behavior, verifies debug info doesn't break compilation | PASS |
| test_28_explicit_captures | `[]`, `[=]`, `[&]`, `[x]`, `[&x]`, `[=, &x]`, `[&, x]`, nested captures, capture-time vs call-time semantics | PASS |
| test_29_ref_params | `func swap(int& a, int& b)`, `func increment(int& x)`, mixed ref/value params, `divmod` with output params | PASS |
| test_30_capture_writeback | `[&counter]` increment, `[&sum]` accumulator, `[&min, &max]` tracker, `[=, &total]` mixed, `[&]` default ref, write-back through HOF | PASS |

### Stress Tests (21/21 passing)

| Test | Stress Target | Result |
|------|--------------|--------|
| stress_01_closure_churn | 50k closure create/call/discard cycles | PASS |
| stress_02_nested_capture | 20k nested closure chains (closure capturing closure) | PASS |
| stress_03_reassignment | 30k closure variable reassignment with release-before-assign | PASS |
| stress_04_early_return_raii | Early return from function with active RAII objects | PASS |
| stress_05_interface_closure | 20k iterations mixing interface dispatch and closure calls | PASS |
| stress_06_recursive_match | Recursive fibonacci via match expressions | PASS |
| stress_07_temporary_leak | 50k temporary closure creation (leak detection) | PASS |
| stress_08_destructor_closure | Interleaved destructor calls and closure invocations | PASS |
| stress_09_triple_reassign | Triple closure reassignment verifying release ordering | PASS |
| stress_10_closure_in_struct | 20k iterations storing closures in struct fields with RAII cleanup | PASS |
| stress_11_closure_in_class | 20k iterations storing closures in class fields with destructor cleanup | PASS |
| stress_13_break_continue_raii | RAII destructor cleanup on break/continue inside nested loops | PASS |
| stress_14_match_guard_raii | RAII objects active during match expressions with guards | PASS |
| stress_15_struct_ptr_copy | Struct with raw pointer — shallow copy semantics verification | PASS |
| stress_16_shadow_capture | Variable shadowing with closure capture in nested scopes | PASS |
| stress_17_long_running | 100k iterations combining closures, RAII, interfaces, recursion | PASS |
| stress_18_break_outer_raii | Break from inner loop preserves outer-scope RAII objects | PASS |
| stress_19_null_closure | Null-initialized closure variable, reassignment, and call | PASS |
| stress_20_reentrant_closure | 20k recursive closure wrapping (5 levels deep per iteration) | PASS |
| stress_21_cyclic_capture | 10k heap objects with closure fields, no explicit ctor/dtor (auto-generated) | PASS |
| stress_22_destructor_reentrant | 10k destructor bodies calling closure fields before epilogue releases them | PASS |

**All 51 tests produce correct output validated against `.expected` files with `--opt 2` enabled.**

---

## File Map

```
mingus/
├── MingusLexer.g4                          # ANTLR4 lexer grammar
├── MingusParser.g4                         # ANTLR4 parser grammar
├── README.md                               # Project overview and quick start
├── build.bat                               # Standalone build script (Ninja + MSVC)
├── run_tests.bat                           # Combined test runner (all 51 tests)
├── docs/
│   └── MINGUS_STATUS.md                    # This file
├── include/mingus/
│   ├── ast/                                # AST node types + visitor
│   │   ├── ASTNode.h, Declarations.h
│   │   ├── Expressions.h, Statements.h
│   │   ├── ASTVisitor.h, TypeNodes.h
│   │   └── Patterns.h
│   ├── sema/                               # Semantic analysis
│   │   ├── Symbol.h                        # Symbol types (Variable, Function, Class, Interface, Enum, Operator)
│   │   ├── SymbolTable.h                   # Scope tree + symbol lookup
│   │   ├── TypeRegistry.h                  # Type interning + canonical types + isCompatible/isImplements
│   │   ├── ErrorReporter.h                 # Diagnostic collection
│   │   ├── SymbolTableBuilder.h            # Pass 1: build scopes + symbols
│   │   ├── TypeResolver.h                  # Pass 2: resolve type references
│   │   ├── TypeChecker.h                   # Pass 3: expression types + overloads + access enforcement
│   │   └── SemanticValidator.h             # Pass 4: RAII + control flow + interface completeness + escape analysis
│   ├── codegen/
│   │   └── IRGenerator.h                   # LLVM IR generation visitor + DIBuilder + getFatPtrType() + itableCache_
│   ├── parser/
│   │   └── ASTGenerator.h                  # ANTLR parse tree -> AST
│   ├── Sema.h                              # Aggregate sema header
│   └── Codegen.h                           # Aggregate codegen header
├── src/mingus/
│   ├── sema/*.cpp                          # Sema implementations (4 passes)
│   ├── codegen/IRGenerator.cpp             # ~4400 lines of codegen
│   └── parser/ASTGenerator.cpp             # Parse tree -> AST
├── tools/
│   ├── mingus_ir_tool.cpp                  # CLI: parse -> sema -> codegen -> optimize -> verify -> emit
│   ├── mingus_sema_tool.cpp                # Semantic analysis dump tool
│   ├── mingus_ast_tool.cpp                 # AST dump tool
│   ├── simple_example.cpp                  # Minimal AST construction API example
│   ├── factorial_example.cpp               # Factorial AST + IR generation API example
│   ├── parser_example.cpp                  # Parser API example
│   ├── TOOL_GUIDE.md                       # mingus_ir_tool usage reference
│   ├── README.md                           # Tools overview and build guide
│   └── CMakeLists.txt                      # Build config for all tools
├── examples/
│   ├── DSPLib.mingus                       # Reusable DSP library (Envelope, Oscillator, WAV writer)
│   ├── example_01–09_*.mingus              # 9 showcase programs (DSP, state machine, iterators, parser, allocator, captures, data structures, particles, groove)
│   ├── showcase.bat                        # Run all 9 examples: source, IR, or output
│   ├── mingus_ir_tool.exe                  # Copied here by CMake post-build
│   └── archive/                            # Retired scratch/debug .mingus files
├── tests/
│   ├── test_01_basics.mingus … test_30_capture_writeback.mingus  # 30 feature tests
│   ├── stress_01_closure_churn.mingus … stress_22_destructor_reentrant.mingus  # 21 stress tests
│   ├── *.expected                          # Expected output for automated validation
│   ├── MathLib.mingus                      # Copy for test_12 imports
│   ├── run_all_tests.bat                   # Feature test runner (30 tests)
│   ├── run_stress_tests.bat                # Stress test runner (21 tests)
│   └── mingus_ir_tool.exe                  # Copied here by CMake post-build
└── CMakeLists.txt                          # Build system (LLVM, ANTLR4, MSVC)
```

---

## Recently Completed

### C++ Capture Lists and Reference Parameters (February 2026)

Replaced implicit capture-by-value with **mandatory C++ capture list syntax**. All lambdas now require explicit `[...]`:

- **Full capture modes**: `[]`, `[=]`, `[&]`, `[x]`, `[&x]`, `[=, &x]`, `[&, x]`
- **True by-reference captures**: `[&x]` stores a pointer to the original variable's alloca in the closure env struct. Reads and writes inside the lambda operate on the original variable — enabling stateful closures (counters, accumulators, trackers) without workarounds.
- **Reference parameters**: `func swap(int& a, int& b) => void` — `ReferenceType` in the type system, codegen passes pointer to caller's alloca, callee reads/writes through it.
- **Nested capture propagation**: `checkLambdaCapture()` walks the entire lambda stack to ensure intermediate lambdas capture variables needed by inner lambdas.
- **New tests**: test_28 (explicit captures), test_29 (reference parameters), test_30 (capture write-back).
- **Migration**: All ~52 lambdas across 25 test/example files updated from `(params) =>` to `[...](params) =>`.

---

## Advised Next Steps

### Short-term (High impact, moderate effort)

1. **Error Recovery**
   Improve parser and sema to report multiple errors per compilation instead of stopping at the first critical one. Parser error messages should include context about what was expected and where.

2. **Generic Types**
   `class Array<T>`, `func map<T, U>(...)` — requires monomorphization or type erasure strategy.

### Medium-term

3. **Standard Library**
    Collections (Array, Map, Set), I/O, and math utilities written in Mingus itself, using extern for OS primitives.

4. **Separate Compilation**
    Support compiling modules independently and linking them. Requires stable ABI for module boundaries and a header/interface file format.

### Long-term

5. **REPL / JIT Mode**
    Use LLVM's ORC JIT for interactive evaluation. Useful for exploration and teaching.

6. **Cross-Platform Support**
    Test and fix codegen for Linux/macOS targets. The core LLVM IR is portable, but ABI conventions and debug info formats differ.
