# Mingus Known Limitations

Consolidated reference of all known limitations, edge cases, and workarounds in the Mingus compiler. Organized by category. Current as of February 2026 with 72 passing tests (51 feature + 21 stress).

---

## Table of Contents

1. [Language Limitations](#1-language-limitations)
2. [Closure System Limitations](#2-closure-system-limitations)
3. [Memory Management Limitations](#3-memory-management-limitations)
4. [Tooling Limitations](#4-tooling-limitations)
5. [Semantic Analysis Edge Cases](#5-semantic-analysis-edge-cases)
6. [Code Generation Edge Cases](#6-code-generation-edge-cases)
7. [Workarounds](#7-workarounds)
8. [Previously Known Limitations (Now Fixed)](#8-previously-known-limitations-now-fixed)
9. [Severity Guide](#9-severity-guide)

---

## 1. Language Limitations

Missing language features that have not been implemented.

| Limitation | Description | Severity |
|-----------|-------------|----------|
| **No generics/templates** | All types are concrete. No parameterized types (`List<T>`) or generic functions (`func map<T, U>(...)`). Each concrete type must be written explicitly. | High |
| **No standard library** | No built-in data structures, I/O abstractions, or utility functions. Only `extern` C functions (libc) are available for I/O, math, and memory. | High |
| **Limited first-class arrays** | Fixed-size arrays (`int[N]`) are first-class: stack allocation, function parameters/returns (passed by pointer), array literals (`[1, 2, 3]`), and struct/class fields. Dynamic arrays still require pointers via `new T[N]`. No bounds checking. No array length property. | Medium |
| **No string type** | Strings are C-style null-terminated `char*`. No built-in string class with length tracking, slicing, or Unicode support. String concatenation via `+` produces heap-allocated results registered for RAII cleanup. | Medium |
| **No garbage collection** | Manual memory management only. Raw heap objects (`new Foo()`) must be explicitly freed with `delete`. Closure environments and shared pointers (`new shared Foo()`) use reference counting. All other heap allocations are manual. | Medium |
| **No multiple return types (beyond tuples)** | Functions can return tuples, but there is no named-return or multi-value return beyond the tuple mechanism. | Low |
| **No exceptions** | No `try`/`catch`/`throw`. Error handling must use return codes, error enums, or similar patterns. | Medium |
| **Range patterns are integer-only** | `match` arm ranges (`1..10`) work only with integer literals. Float or char ranges are not supported. | Low |
| **Array size must be literal** | `int[N]` requires an integer literal for `N`. Constant expressions or variables cannot be used for array dimensions. `int[n]` (variable) is a parse error. Runtime-sized arrays use `new int[n]` (heap allocation). | Low |
| **Float literal always double** | `1.0` is always `double`. No `1.0f` suffix for `float`. Requires explicit cast for `float` assignment. | Low |
| **Single compilation unit** | Each `.mingus` file compiles independently. Cross-file interaction uses the `import` system, which links at the LLVM IR level. No header files or forward declarations across modules. | Medium |
| **No untyped lambda params** | Lambda parameters can syntactically omit types (`[=](x) => x`), but type inference for untyped parameters is not implemented. Sema will report an error. All lambda params must have explicit types. | Low |

---

## 2. Closure System Limitations

Known edge cases and limitations in the closure (lambda) system, which uses fat pointers `{ fnPtr, envPtr }` with reference-counted capture environments.

| Limitation | Description | Severity |
|-----------|-------------|----------|
| **By-value captures are copy-on-entry** | By-value captures (`[=]` or `[x]`) are loaded from the environment into local allocas at lambda entry. Writes inside the lambda modify the local copy only and are never written back to the environment or the outer variable. This is by design (same as C++), but can surprise users expecting mutation to propagate. | Low |
| **Capture propagation boundary** | In nested lambdas, if an intermediate lambda's capture list does not allow capturing a particular variable (e.g., `[]` empty capture), propagation to outer lambdas stops. The innermost lambda cannot capture variables that intermediate lambdas refuse to carry. | Low |

---

## 3. Memory Management Limitations

Limitations related to memory allocation, RAII, reference counting, and string handling.

### Reference Counting

| Limitation | Description | Severity |
|-----------|-------------|----------|
| **RC is closure-only** | Reference counting applies only to closure capture environments. Class instances, structs, arrays, and other heap allocations are not reference-counted. | Medium |

### String Memory

| Limitation | Description | Severity |
|-----------|-------------|----------|
| **No small string optimization** | All string concatenation results are heap-allocated via `malloc`. Even short results like `"a" + "b"` go through heap allocation. | Low |
| **Concat chain intermediate waste** | `a + b + c` creates two heap buffers; the intermediate result from `a + b` persists for the entire scope (RAII cleanup happens at scope exit, not immediately after use). | Low |
| **String interpolation allocates** | `"hello ${name}"` internally uses `snprintf` + `malloc`, producing a heap-allocated result registered for RAII cleanup. | Low |

### General Memory

| Limitation | Description | Severity |
|-----------|-------------|----------|
| **Fat pointer null comparison** | `f != null` where `f` is a closure or interface compares only `fnPtr` / `objPtr` (field 0 of the fat pointer). The `envPtr` / `itablePtr` (field 1) is not tested separately. This is correct for null fat pointers (both fields are zero), but does not support partial-null states. | Low |
| **No RAII for raw pointers** | Pointers obtained via `new` are not automatically freed at scope exit. Only class instances stored in local variables (not pointers) get RAII cleanup. Heap-allocated objects require explicit `delete`. | Medium |

---

## 4. Tooling Limitations

Limitations in compiler diagnostics, error recovery, and development tooling.

| Limitation | Description | Severity |
|-----------|-------------|----------|
| **Minimal error recovery** | The first parse error or semantic error typically stops compilation. The compiler does not attempt to recover and report multiple errors in a single pass. | Medium |
| **No DWARF/PDB debug info** | The `emitDebugInfo_` flag and DIBuilder infrastructure exist in codegen, but debug information emission is not fully implemented. Compiled binaries cannot be stepped through in a debugger with source-level mapping. | Medium |
| **No LSP / IDE integration** | No Language Server Protocol implementation. No syntax highlighting definitions, autocompletion, or go-to-definition support for editors. | Low |
| **No REPL** | No interactive read-eval-print loop. All code must be compiled and executed as files. | Low |
| **Error messages lack suggestions** | Error messages report what went wrong but do not suggest fixes (e.g., "did you mean..." or "consider adding..."). | Low |
| **No source-map in IR** | Generated LLVM IR does not carry source location metadata. When inspecting IR output, there is no mapping back to source lines. | Low |

---

## 5. Semantic Analysis Edge Cases

Limitations in the four semantic analysis passes (SymbolTableBuilder, TypeResolver, TypeChecker, SemanticValidator).

| Limitation | Description | Severity |
|-----------|-------------|----------|
| **No forward declarations** | A class used as a base must be defined before the derived class in source order (or imported from another module). Mutual class references within a single module are not supported. | Medium |
| **No forward type references** | Types must be defined before use in type annotations. A struct referencing another struct that is defined later in the file will fail. | Medium |
| **Operator overload is left-only** | Operator resolution checks only the left operand's type. `42 + vec` will not find `Vec::operator+`. The overloaded type must be on the left side: `vec + 42`. | Low |
| **Lambda return type inference** | Block-bodied lambdas infer their return type from the first `return` statement encountered. Conflicting return types in different branches are not cross-checked. | Low |
| **No definite assignment analysis** | Variables can be read before assignment without a compiler error. Uninitialized variables contain whatever was in the alloca (zero for zero-initialized structs, undefined for primitives). | Medium |
| **No null safety** | Pointers and nullable closures can be dereferenced without null checks. No `?.` safe-navigation operator or nullable type system. | Medium |
| **Operator imports not transferred** | Whole-module import (`import Mod;`) transfers named symbols but does not transfer operator overload definitions. Operators from imported modules may not be available. | Low |
| **Limited constructor forms** | Each class supports one regular constructor, one copy constructor (`T&`), and one move constructor (`T&&`). General constructor overloading with arbitrary signatures is not supported. Only one destructor per class. | Low |
| **Vtable ordering is alphabetical** | New virtual methods introduced in derived classes are ordered alphabetically (from `std::map` iteration), not in source order. This affects vtable layout but not correctness for single-inheritance. | Low |
| **Enum exhaustiveness is name-based** | Match exhaustiveness checking for enums uses case names only. Numeric patterns, complex expressions, or range patterns covering enum values are not recognized as exhaustive. | Low |
| **Loop return analysis is conservative** | `for`/`while` bodies are always classified as `NeverReturns` for return completeness checking, even for provably infinite loops. Functions that return only inside a loop may get false "missing return" warnings. | Low |
| **Limited dangling reference detection** | `[&x]` captures that escape via return or field store are detected and rejected. However, escaping via function arguments that store the closure (requires interprocedural analysis) is not detected. | Low |
| **Char literal escape processing** | ASTGenerator reads `text[1]` for char literals without fully processing escape sequences. `'\n'`, `'\t'`, etc. may not produce the expected control characters. | Low |

---

## 6. Code Generation Edge Cases

Limitations in the LLVM IR generation stage.

| Limitation | Description | Severity |
|-----------|-------------|----------|
| **Duplicate cross-module externs** | If two modules both declare `extern func sin(double x) => double;`, codegen creates `@sin` and `@sin.1` (LLVM name deduplication). The linker then fails because `@sin.1` has no definition. **Workaround**: Declare the extern in one module and `import` it in others. | Medium |
| **No debug locations on RC ops** | Retain, release, and destructor calls emitted by the closure RC system have no `DebugLoc` attached. They appear as "unknown location" if debug info were enabled. | Low |

---

## 7. Workarounds

Documented workarounds for the limitations above.

### Duplicate Cross-Module Externs

**Problem**: Two modules declaring the same `extern func` causes linker errors.

**Workaround**: Declare the extern in exactly one module, then import it in other modules:
```
// MathLib.mingus
module MathLib {
    extern func sin(double x) => double;
    extern func cos(double x) => double;
}

// MyModule.mingus
module MyModule {
    import sin, cos from MathLib;
    // ...
}
```

### No Generics

**Problem**: Cannot write type-parameterized containers or algorithms.

**Workaround**: Write concrete versions for each type needed, or use `void*` (pointer to byte) with manual casting in `raw` blocks:
```
// Concrete approach:
func maxInt(int a, int b) => int { return a > b ? a : b; }
func maxDouble(double a, double b) => double { return a > b ? a : b; }
```

### No Standard Library

**Problem**: No built-in containers, I/O, or utilities.

**Workaround**: Use `extern` declarations to access C standard library functions:
```
extern {
    func malloc(int size) => byte*;
    func free(byte* ptr) => void;
    func memcpy(byte* dst, byte* src, int n) => byte*;
    func printf(string fmt, ...) => int;
    func puts(string s) => int;
}
```

### Dynamic Arrays / Bounds Checking

**Problem**: No dynamic arrays with bounds checking or length tracking. Fixed-size arrays (`int[N]`) now support parameters, returns, literals, and fields. Dynamic arrays still need manual management.

**Workaround**: Implement a DynamicArray as a class with manual memory management:
```
class DynamicArray {
    int* data;
    int size;
    int capacity;

    constructor(int cap) {
        this.capacity = cap;
        this.size = 0;
        this.data = (int*)malloc(cap * 4);
    }

    destructor {
        free((byte*)this.data);
    }

    func push(int val) => void {
        this.data[this.size] = val;
        this.size = this.size + 1;
    }

    func get(int idx) => int {
        return this.data[idx];
    }
}
```

---

## 8. Previously Known Limitations (Now Fixed)

These were documented as limitations in earlier versions but have been fixed and verified by passing tests.

| Former Limitation | Fix | Verified By |
|-------------------|-----|-------------|
| **Closures with struct params** | `mapParamType()` returns `ptr` for struct params, consistent across all call types. | test_31 |
| **Closures with ref params** | `FunctionTypeSymbol::ParameterInfo::isReference` carries ref info; `ArgumentsNode::isReference` propagated to all call types. | test_32 |
| **Interface parameters** | Arg building wraps class pointer to interface fat pointer via `emitWrapToInterfacePtr()` at call site. | test_33 |
| **No varargs in extern** | `extern func printf(string fmt, ...) => int;` syntax with `...` ellipsis. Varargs promotion (small int to i32, float to double) handled in codegen. | test_34 |
| **For-loop multi-init** | `for (int i = 0, int j = 10; ...)` with multiple typed or inferred declarations in the initializer. | test_35 |
| **No const modifier** | `const int x = 42;` and `const pi = 3.14;` with type-inferred and explicit-typed variants. Assignment to const is rejected by sema. | test_36 |
| **Pipe targets restricted to free functions** | `x \|> obj->method` and `x \|> obj->method(extra_args)` now work. Pipe targets support member access chains. | test_37 |
| **No bare field access** | Class fields can be accessed by name alone (without `this.` prefix) in methods and constructors, including inherited fields. Local variables shadow fields correctly. | test_38 |
| **No do-while loop** | `do { } while (cond);` construct implemented with at-least-once semantics. Supports break, continue, and nesting. | test_39 |
| **No covariant return types** | Overriding virtual methods can return a more derived pointer type (e.g., `Dog*` where base returns `Animal*`). TypeChecker validates subclass relationship. | test_40 |
| **No typedef/type alias** | `typedef int Count;` creates transparent type aliases. Aliases resolve to underlying type during semantic analysis. | test_41 |
| **No labeled break/continue** | `outer: for (...) { break outer; }` enables targeting outer loops from nested contexts. Works with for, while, and do-while. RAII cleanup across label jumps. | test_42 |
| **No copy constructors** | `constructor(ClassName& other)` detected by ASTGenerator when parameter type matches enclosing class. User-defined copy logic. Mangled as `ClassName_copy_constructor`. | test_43 |
| **No function overloading** | Multiple functions with the same name but different parameter counts or types. Scoring-based overload resolution in TypeChecker. `$_type` mangled name suffix for LLVM disambiguation. | test_44 |
| **No move semantics** | `constructor(ClassName&& other)` for move constructors, `move(x)` expression for rvalue marking. Enables ownership transfer with source zeroing. Mangled as `ClassName_move_constructor`. | test_45 |
| **No auto-generated ctor/dtor** | SymbolTableBuilder injects synthetic ConstructorDeclaration and DestructorDeclaration with empty bodies when a class lacks explicit ones. | Multiple tests |
| **Temporary closure argument leak** | Closures returned from functions and passed directly as arguments are now immediately released after the call returns. No more one-count leaks. | test_46, stress_07 |
| **Self-capturing closures dangle** | Self-capturing lambdas now retain their env pointer at entry and release at exit via RAII. Self-references are safe during recursion. | test_47 |
| **By-reference captures escape undetected** | Compiler now rejects returning closures with `[&x]` captures or storing them in fields. Escape analysis catches the three most common dangling-reference patterns. | test_48 |
| **No cycle detection / weak references** | `[weak x]` capture mode added. Weak captures hold non-owning references that don't prevent cleanup. Dead weak captures resolve to null. Breaks reference cycles between closures. | test_49 |
| **Nullable closures** | `NullType` is compatible with `FunctionType` in sema. Null converts to zero fat pointer `{ null, null }` in codegen. | test_21, stress tests |
| **Lambda literal assignment** | `f = [=](int x) => { ... };` works as assignment RHS. Grammar and ASTGenerator handle lambda expressions in assignment context. | Multiple tests |
| **Printf special-cased** | Now handled through general varargs support (`...` in extern declarations), not name-based special casing. Any extern can be declared variadic. | test_34 |
| **No opt-in ARC for heap objects** | `new shared Foo()` creates reference-counted class instances typed as `shared Foo*`. Reuses closure RC infrastructure (`{ i64 strong, i64 weak, ptr cleanup }` header). RAII release at scope exit, retain/release on assignment, `delete` as release. Classes only (not structs). `shared Foo*` and `Foo*` are incompatible types. | test_50 |
| **No array parameters/returns** | Fixed-size arrays (`int[N]`) can now be passed to and returned from functions. Passed by pointer (like structs) for zero-copy semantics. Array literals `[1, 2, 3]` supported for inline initialization. Arrays work as struct/class fields. | test_51 |

---

## 9. Severity Guide

- **High**: Blocks common use cases or prevents expressing standard patterns. Would need to be addressed for the language to be viable for general-purpose use.
- **Medium**: Inconvenient or surprising behavior. Workarounds exist but add friction or require non-obvious patterns.
- **Low**: Minor inconvenience, edge case, or style issue. Rarely encountered in practice or has a trivial workaround.
