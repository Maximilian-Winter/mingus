# Mingus Known Limitations

Consolidated reference of all known limitations across grammar, semantic analysis, code generation, and runtime. Organized by subsystem.

---

## Table of Contents

1. [Grammar and Parser](#1-grammar-and-parser)
2. [AST Generation](#2-ast-generation)
3. [Semantic Analysis](#3-semantic-analysis)
4. [Code Generation](#4-code-generation)
5. [Runtime / Memory](#5-runtime--memory)
6. [Tooling and Infrastructure](#6-tooling-and-infrastructure)

---

## 1. Grammar and Parser

| Limitation | Description | Severity |
|-----------|-------------|----------|
| **No varargs in extern** | `extern func printf(string, int)` must declare exact parameter count. Cannot declare true variadic functions. | Medium |
| **For loop init** | Only the first variable declaration in `for (int i = 0, int j = 0; ...)` is used; subsequent ones are silently dropped. | Low |
| **Range pattern integers only** | `1..10` works, but float or char ranges are not supported. | Low |
| **Pipe target restricted** | `x \|> f(args)` works, but `x \|> obj.method(args)` does not — pipe targets must be qualified names, not member access chains. | Medium |
| **No do-while loop** | Only `for` and `while` loops are available. | Low |
| **No labeled break/continue** | Cannot break from an outer loop when inside a nested loop. | Low |
| **No generics/templates** | No parameterized types or functions. | High |
| **No typedef/using alias** | No way to create type aliases. | Low |
| **No const modifier** | Variables and parameters cannot be marked `const`. No immutability enforcement. | Medium |
| **Single compilation unit** | Each `.mingus` file compiles independently. Cross-file linking uses `import`. | Medium |
| **Error recovery minimal** | First parse or semantic error typically stops compilation. | Medium |

---

## 2. AST Generation

| Limitation | Description | Severity |
|-----------|-------------|----------|
| **VariableDeclaration cast quirk** | Local `var` declarations are wrapped in `ExpressionStatement` with `VariableDeclaration` cast to `ExpressionNode`. Downstream passes must special-case this. | Internal |
| **Char literal escapes** | ASTGenerator reads `text[1]` without processing escape sequences in char literals (e.g., `'\n'` may not work correctly). | Low |
| **Separate identifier types** | Simple identifiers → `IdentifierExpression`, dotted names → `QualifiedNameExpression`. These are semantically related but separate AST types. | Internal |
| **Match as ExpressionStatement** | `matchStatement` is represented as `ExpressionStatement{MatchExpression}`, requiring match to handle both expression and statement contexts. | Internal |
| **Untyped lambda params** | Lambda parameters can omit types (`[=](x) => x`), producing null type nodes. Type inference for these is not supported — sema will error. | Low |

---

## 3. Semantic Analysis

### Pass 1: Symbol Table Builder

| Limitation | Description | Severity |
|-----------|-------------|----------|
| **No forward declarations** | A class used as a base must be defined before the derived class in source order (or imported). | Medium |
| **Operator imports** | Whole-module import (`import Mod;`) does not transfer operator overloads, only named symbols. | Low |
| **Single ctor/dtor** | Only one constructor and destructor per class. Multiple definitions silently overwrite. | Low |
| **Vtable ordering** | New virtual methods in derived classes are ordered alphabetically (from `std::map` iteration), not source order. | Low |

### Pass 2: Type Resolver

| Limitation | Description | Severity |
|-----------|-------------|----------|
| **No forward type references** | Types must be defined before use in type annotations. | Medium |
| **Array size must be literal** | `int[N]` requires an integer literal for `N`. Constant expressions not evaluated. | Low |

### Pass 3: Type Checker

| Limitation | Description | Severity |
|-----------|-------------|----------|
| **No function overloading** | Only one function per name in a scope. Operator overloads are the only form of overloading. | Medium |
| **Lambda return inference** | Block-bodied lambdas infer return type from the first `return` statement. Conflicting types in different branches are not cross-checked. | Low |
| **No mutability enforcement** | `isMutable` flag exists but is never set to `false` or checked. | Medium |
| **No definite assignment** | Variables can be read before assignment without error. | Medium |
| **No null safety** | Pointers and nullable closures can be dereferenced without null checks. | Medium |
| **Float literal always double** | `1.0` is always `double`. No `1.0f` suffix for `float`. | Low |
| **Operator overload left-only** | Resolution checks only the left operand's type. `42 + vec` won't find `Vec::operator+`. | Low |
| **No covariant returns** | Overriding a virtual method requires the exact same return type. | Low |

### Pass 4: Semantic Validator

| Limitation | Description | Severity |
|-----------|-------------|----------|
| **Minimal escape analysis** | Only lambda-literal-as-argument is marked non-escaping. Closures stored in variables are always assumed escaping. | Low |
| **Enum exhaustiveness name-based** | Doesn't handle numeric patterns or complex expressions for enum coverage. | Low |
| **Loop return analysis** | `for`/`while` always classified as `NeverReturns`, even infinite loops. Functions returning only inside a loop get false errors. | Low |
| **No dangling reference detection** | `[&x]` captures that escape the captured variable's scope are not detected. | Medium |
| **Capture propagation boundary** | If an intermediate lambda doesn't allow a capture, propagation to outer lambdas stops. | Low |

---

## 4. Code Generation

### Calling Convention Issues

| Limitation | Description | Severity |
|-----------|-------------|----------|
| **Closures with struct params** | Lambda function types are built from `FunctionType::parameterTypes` via `mapType()`, which returns the struct LLVM type (not pointer). Regular functions pass structs by pointer. This ABI mismatch means closures taking struct parameters crash at runtime. | High |
| **Closures with reference params** | `isReference` is a `VariableSymbol` property, not part of `FunctionType`. Lambda codegen doesn't detect reference params, so `i32` is passed instead of `ptr`. | High |
| **Interface parameters** | Passing an interface fat pointer (`{ ptr, ptr }`) as a function argument has a calling convention mismatch — the function signature expects `ptr` but receives a struct. | High |

### Other Codegen Issues

| Limitation | Description | Severity |
|-----------|-------------|----------|
| **Duplicate cross-module externs** | Two modules declaring `extern func sin(...)` causes codegen to create `@sin` and `@sin.1` — linker error. Declare in one module, import in others. | Medium |
| **Printf/snprintf special-cased** | Only `printf` and `snprintf` are treated as varargs (detected by name). Other extern variadics not supported. | Low |
| **No DWARF/PDB debug info** | The `emitDebugInfo_` flag exists but debug info emission is not implemented. | Medium |
| **No debug locations on RC ops** | Retain/release/destructor calls have no `DebugLoc` — appear as "unknown location" in debuggers. | Low |

---

## 5. Runtime / Memory

### Closure Reference Counting

| Limitation | Description | Severity |
|-----------|-------------|----------|
| **Temporary closure leak** | Closures passed directly as function arguments (without variable storage) may leak one refcount. A temporary alloca with RAII is created, but the interaction with the callee's potential retain is imperfect. | Medium |
| **Captures copy-on-entry** | By-value captures are loaded from the env into local allocas inside the lambda. Writes modify the local copy only — never written back to the env or outer variable. | Medium |
| **Self-capturing closures** | Self-references in env are unretained (to avoid cycles). If the closure is freed while self-calling, the reference dangles. | Medium |
| **No cycle detection** | If closures form a reference cycle (A captures B, B captures A), neither is ever freed. | Medium |

### String Memory

| Limitation | Description | Severity |
|-----------|-------------|----------|
| **No small string optimization** | All strings are heap-allocated. Even short concat results go through `malloc`. | Low |
| **Concat chain waste** | `a + b + c` creates two heap buffers; the intermediate persists for the entire scope. | Low |
| **Heap-allocated strings** | String operations always produce heap allocations registered for RAII cleanup. | Low |

### General

| Limitation | Description | Severity |
|-----------|-------------|----------|
| **Fat pointer null comparison** | `f != null` where `f` is a closure works by comparing `fnPtr` (field 0). This is correct for null fat pointers but doesn't expose separate envPtr testing. | Low |
| **No move semantics** | All values are copied. No move constructors or ownership transfer. | Medium |
| **No copy constructors** | Copying a class instance copies raw bytes, not invoking any user-defined copy logic. Closure fields get duplicate references without retain. | Medium |

---

## Severity Guide

- **High**: Causes crashes, incorrect code, or blocks common use cases. Should be fixed for language viability.
- **Medium**: Inconvenient or surprising behavior. Workarounds exist but are non-obvious.
- **Low**: Minor inconvenience, edge case, or style issue. Would be nice to fix eventually.
- **Internal**: Implementation detail that affects compiler developers, not end users.
