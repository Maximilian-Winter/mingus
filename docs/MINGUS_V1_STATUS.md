# Mingus v1 — Language Status Report

**Date:** February 2026
**Status:** Compiles and executes optimized native binaries — **15/15 feature tests passing**

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
| Sema Pass 3 | TypeChecker — expression types, overload resolution |
| Sema Pass 4 | SemanticValidator — RAII analysis, control flow validation |
| Codegen | LLVM 21.1.8 IR generation via AST visitor |
| Optimization | LLVM PassBuilder with configurable O0/O1/O2 pipeline |
| Compilation | Clang (from LLVM distribution) compiles IR to native |

**Build System:** CMake + MSVC (Windows), CLion IDE
**Test Runner:** `run_all_tests.bat` — generates IR, compiles with clang, validates output against `.expected` files
**Showcase:** `showcase.bat` — displays source code and program output for all tests

---

## Working Features

### Verified by test suite (15/15 passing)

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

#### 3. Functions (Tests 01-15)
- Function declarations with typed parameters and return types: `func name(int x) => int`
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

#### 5. Classes, RAII, and Inheritance (Tests 03, 13)
- Class declarations with fields and methods
- Constructors: `constructor(int size) { ... }`
- Destructors: `destructor { ... }`
- Automatic destructor calls at scope exit (RAII)
- Destructor suppression for returned values
- `new` and `delete` for heap allocation
- Arrow operator for pointer member access: `node->left`
- Recursive destructors (tree cleanup)
- `private` access modifier (parsed; semantic enforcement planned)
- **Single inheritance**: `class Dog : Animal { ... }`
- **Virtual dispatch**: all methods are virtual by default (Java-like), vtable-based dispatch through base pointers
- **Constructor chaining**: `constructor() : super(4) { ... }` calls base constructor
- **Destructor chaining**: derived destructor body runs first, then base destructor called automatically
- **Inherited field access**: derived classes access base class fields through `this`
- **Polymorphic assignment**: `Derived*` assignable to `Base*`
- **Abstract classes**: `abstract class Shape { ... }` cannot be instantiated; concrete subclasses must override all abstract methods

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

#### 8. Lambdas and Higher-Order Functions (Tests 06, 10, 11)
- Lambda expressions: `(double x) => { return x * 2.0; }`
- Variables holding function values: `var doubler = (double x) => { ... }`
- Higher-order functions: `func apply(double x, (double) => double f) => double`
- Passing lambdas as arguments
- Calling lambdas stored in variables
- Pipe operator with lambda arguments: `7.0 |> apply((double x) => { return x + 3.0; })`

#### 9. Closures with Captures (Tests 10, 11)
- Fat pointer representation: all function-typed values are `{ fnPtr, envPtr }` structs
- All lambdas receive `ptr %env` as final parameter (uniform calling convention)
- Closures capturing variables from enclosing scope: `func makeScaler(double factor) => (double) => double`
- Heap-allocated capture structs via `malloc`
- Indirect calls extract `fnPtr` and `envPtr` from fat pointer, pass env as last arg
- Closures as function arguments and return values
- Composed closures: `func compose((double) => double f, (double) => double g) => (double) => double`
- Closures capturing other closures (fat pointers in capture struct)
- Closures calling module functions from within lambda body

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

#### 13. Single Inheritance and Virtual Dispatch (Test 13)
- `class Dog : Animal { ... }` — inherits all fields and methods
- All non-static, non-constructor, non-destructor methods are virtual (vtableIndex assigned)
- Vtable: global constant `[N x ptr]` array, stored in field 0 of class struct when vtableSize > 0
- Virtual dispatch through base pointer: load vtable ptr → GEP to slot → indirect call
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

#### 16. Optimization Pipeline
- LLVM PassBuilder integration with configurable optimization levels
- `--opt 0` (default): no optimization
- `--opt 1`: O1 pipeline (basic simplifications, mem2reg)
- `--opt 2`: O2 pipeline (inlining, GVN, SROA, instcombine, vectorization, DCE)
- Optimization runs between IR generation and LLVM verification
- All tests run with `--opt 2` enabled

### Verified by IR generation (passes LLVM verifier, not yet runtime-tested individually)

- Tuple expressions: `return (name, prec, isOperator)`
- Tuple destructuring: `(var a, var b, var c) = tokenInfo(tok)`
- `DynamicArray.map` with lambda + pipe: `arr.map((int x) => { return x * 2; })`
- `memcpy` interop for array grow
- Complex number arithmetic (operator overloading)

---

## Known Limitations

### Language Limitations

| Feature | Status | Notes |
|---------|--------|-------|
| **Virtual destructors** | Limitation | Destructor dispatch is static (not via vtable). `delete basePtr` calls base destructor only, not derived. Use typed pointers for delete. |
| **Protected access** | Parsed only | Access modifiers are parsed but not enforced by sema. |
| **Static methods** | Parsed only | `static` modifier is parsed but static dispatch not implemented in codegen. |
| **Generics/templates** | Not supported | No generic types or functions. |
| **Error recovery** | Minimal | First semantic error usually stops further analysis. |
| **Multiple class inheritance** | Not supported | `class C : A, B` where both A and B are classes is a sema error. Multiple interface implementation is supported. |

### Codegen Limitations

| Area | Limitation |
|------|------------|
| **Closure memory** | Heap-allocated closure structs are never freed (known leak). Escape analysis planned for future. |
| **ABI** | Struct return by value relies on LLVM's default ABI lowering. Not tested with very large structs. |
| **Debug info** | No DWARF/PDB debug information emitted. |
| **Module visibility** | `public`/`private` on symbols is parsed but only partially enforced (whole-module import skips non-public). No separate compilation or linking — all imported files compiled together. |

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

**All 16 tests produce correct output validated against `.expected` files with `--opt 2` enabled.**

---

## File Map

```
mingus_antlr_grammar_revived_v1/
├── MingusLexer.g4                          # ANTLR4 lexer grammar
├── MingusParser.g4                         # ANTLR4 parser grammar
├── README.md                               # Project overview and quick start
├── MINGUS_V1_STATUS.md                     # This file
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
│   │   ├── TypeChecker.h                   # Pass 3: expression types + overloads
│   │   └── SemanticValidator.h             # Pass 4: RAII + control flow + interface completeness
│   ├── codegen/
│   │   └── IRGenerator.h                   # LLVM IR generation visitor + getFatPtrType() + itableCache_
│   ├── parser/
│   │   └── ASTGenerator.h                  # ANTLR parse tree -> AST
│   ├── Sema.h                              # Aggregate sema header
│   └── Codegen.h                           # Aggregate codegen header
├── src/mingus/
│   ├── sema/*.cpp                          # Sema implementations (4 passes)
│   ├── codegen/IRGenerator.cpp             # ~3500 lines of codegen
│   └── parser/ASTGenerator.cpp             # Parse tree -> AST
├── examples/
│   ├── mingus_ir_tool.cpp                  # CLI: parse -> import resolve -> sema -> codegen -> optimize -> verify -> emit
│   ├── TOOL_GUIDE.md                       # mingus_ir_tool usage reference
│   ├── MathLib.mingus                      # Reusable library module (imported by test_12)
│   ├── test_01_basics.mingus               # through test_15_interfaces.mingus
│   ├── test_*.expected                     # Expected output for automated validation (CRLF)
│   ├── run_all_tests.bat                   # Automated test runner (--opt 2, fc /b validation)
│   └── showcase.bat                        # Display source code + output for all tests
└── CMakeLists.txt                          # Build system (LLVM, ANTLR4, MSVC)
```

---

## Advised Next Steps

### Short-term (High impact, moderate effort)

1. **Access Modifier Enforcement**
   Enforce `public`/`private`/`protected` in sema. Currently parsed but not checked.

2. **Escape Analysis for Closures**
   Detect closures that don't escape and stack-allocate them. Add `free()` calls where lifetime is deterministic. Eliminates the current closure memory leak for common cases.

3. **Virtual Destructors**
   Route `delete basePtr` through the vtable so the correct derived destructor runs. Requires adding a destructor slot to every class vtable.

### Medium-term

4. **Static Methods**
   Implement static dispatch in codegen — `Type.staticMethod()` calls the function without a `this` pointer.

5. **Debug Information**
   Emit LLVM debug metadata (DIBuilder) for DWARF/PDB output. Enables debugging with Visual Studio or gdb.

6. **Error Recovery**
   Improve parser and sema to report multiple errors per compilation instead of stopping at the first critical one.

### Long-term

7. **Generic Types**
   `class Array<T>`, `func map<T, U>(...)` — requires monomorphization or type erasure strategy.

8. **Standard Library**
   Collections (Array, Map, Set), I/O, and math utilities written in Mingus itself, using extern for OS primitives.

9. **REPL / JIT Mode**
   Use LLVM's ORC JIT for interactive evaluation. Useful for exploration and teaching.
