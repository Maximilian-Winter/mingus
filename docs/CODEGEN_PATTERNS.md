# Mingus Code Generation Patterns

This document describes how key Mingus language features are lowered to LLVM IR: lambdas/closures, pipe operator, pattern matching, multi-module imports, and string interpolation.

**Source files:**
- `src/mingus/codegen/IRGenerator.cpp` — All codegen patterns
- `include/mingus/codegen/IRGenerator.h` — Caches and state

---

## Table of Contents

1. [Lambda / Closure Codegen](#1-lambda--closure-codegen)
2. [Pipe Operator](#2-pipe-operator)
3. [Match Expressions](#3-match-expressions)
4. [Multi-Module Imports](#4-multi-module-imports)
5. [String Interpolation](#5-string-interpolation)
6. [Reference Parameters](#6-reference-parameters)
7. [Constructor and Destructor Codegen](#7-constructor-and-destructor-codegen)
8. [Entry Point Wrapper](#8-entry-point-wrapper)

---

## 1. Lambda / Closure Codegen

### Lambda Function Generation

Each lambda creates a new internal LLVM function:

```llvm
define internal <retTy> @__lambda_0(<params>, ptr %env) {
    ; ... body ...
}
```

ALL lambdas receive `ptr %env` as their **last parameter**, even non-capturing ones (value is `null`). This uniform calling convention allows all function-typed values to be called through the same fat pointer interface.

### Non-Capturing Lambda

```mingus
var f = [](int x) => { return x * 2; };
```

```llvm
define internal i32 @__lambda_0(i32 %x, ptr %env) {
    %result = mul i32 %x, 2
    ret i32 %result
}

; Fat pointer with null envPtr:
%fat = insertvalue { ptr, ptr } undef, ptr @__lambda_0, 0
%fat1 = insertvalue { ptr, ptr } %fat, ptr null, 1
store { ptr, ptr } %fat1, ptr %f
```

### Capturing Lambda (By Value)

```mingus
var scale = 2.0;
var f = [=](double x) => { return x * scale; };
```

```llvm
; Closure env struct: { i64 refcount, ptr cleanup, double scale }
%env.size = ...
%env.ptr = call ptr @malloc(i64 %env.size)

; Init refcount = 1
%rc.slot = getelementptr { i64, ptr, double }, ptr %env.ptr, i32 0, i32 0
store i64 1, ptr %rc.slot

; Init cleanup = null (no inner closures)
%cleanup.slot = getelementptr { i64, ptr, double }, ptr %env.ptr, i32 0, i32 1
store ptr null, ptr %cleanup.slot

; Store captured value
%scale.val = load double, ptr %scale.alloca
%cap.slot = getelementptr { i64, ptr, double }, ptr %env.ptr, i32 0, i32 2
store double %scale.val, ptr %cap.slot

; Build fat pointer
%fat = insertvalue { ptr, ptr } undef, ptr @__lambda_1, 0
%fat1 = insertvalue { ptr, ptr } %fat, ptr %env.ptr, 1
```

Inside the lambda function, captures are loaded from env into local allocas:

```llvm
define internal double @__lambda_1(double %x, ptr %env) {
    ; Load scale from env field 2
    %scale.ptr = getelementptr { i64, ptr, double }, ptr %env, i32 0, i32 2
    %scale = load double, ptr %scale.ptr
    %scale.alloca = alloca double
    store double %scale, ptr %scale.alloca

    ; Body
    %x.load = load double, ptr %x.alloca
    %scale.load = load double, ptr %scale.alloca
    %result = fmul double %x.load, %scale.load
    ret double %result
}
```

### Capturing Lambda (By Reference)

```mingus
var counter = 0;
var inc = [&counter]() => { counter = counter + 1; return counter; };
```

```llvm
; Env stores pointer to outer alloca, not the value:
%cap.slot = getelementptr { i64, ptr, ptr }, ptr %env.ptr, i32 0, i32 2
store ptr %counter.alloca, ptr %cap.slot     ; store the alloca pointer itself

; Inside lambda:
define internal i32 @__lambda_2(ptr %env) {
    %counter.ref.ptr = getelementptr { i64, ptr, ptr }, ptr %env, i32 0, i32 2
    %counter.ref = load ptr, ptr %counter.ref.ptr  ; pointer to outer alloca
    ; Reads/writes go through this pointer to the outer variable
    %val = load i32, ptr %counter.ref
    %inc = add i32 %val, 1
    store i32 %inc, ptr %counter.ref               ; persists to outer scope!
    ret i32 %inc
}
```

### Indirect Call (Calling a Fat Pointer)

```llvm
; Call f(42) where f is { ptr, ptr }:
%fat = load { ptr, ptr }, ptr %f.alloca
%fn = extractvalue { ptr, ptr } %fat, 0
%env = extractvalue { ptr, ptr } %fat, 1
%result = call i32 (i32, ptr) %fn(i32 42, ptr %env)
```

### State Saved/Restored During Lambda Codegen

- `raiiScopeStack_` — saved and cleared (lambda RAII isolation)
- `currentFunction_` — saved and set to new lambda function
- `currentThisPtr_` — saved and cleared
- `namedValues_` — not saved; captures are added under the lambda's symbol keys
- `loopRAIIScopeDepth_` — saved

---

## 2. Pipe Operator

`x |> f |> g(a, b)` lowers to sequential function calls where each result feeds as the first argument to the next stage.

### Direct Function Pipe

```mingus
var result = 5.0 |> square |> addOne;
```

```llvm
; Stage 1: square(5.0)
%pipe.0 = call double @square(double 5.0)
; Stage 2: addOne(pipe.0)
%pipe.1 = call double @addOne(double %pipe.0)
```

### Pipe with Extra Arguments

```mingus
var result = 3.0 |> multiply(2.0) |> clamp(0.0, 10.0);
```

```llvm
; Stage 1: multiply(3.0, 2.0) — piped value is first arg
%pipe.0 = call double @multiply(double 3.0, double 2.0)
; Stage 2: clamp(pipe.0, 0.0, 10.0) — piped value is first arg
%pipe.1 = call double @clamp(double %pipe.0, double 0.0, double 10.0)
```

### Pipe with Closure Stage

If the stage function is a closure (fat pointer), the env is extracted and appended:

```llvm
; x |> closureVar
%fat = load { ptr, ptr }, ptr %closureVar.alloca
%fn = extractvalue { ptr, ptr } %fat, 0
%env = extractvalue { ptr, ptr } %fat, 1
%pipe.result = call double (double, ptr) %fn(double %x, ptr %env)
```

---

## 3. Match Expressions

### Switch Optimization

When the subject is integer-kinded and all patterns are literals or wildcards (no guards, no binding, no range):

```llvm
switch i32 %subject, label %match.default [
    i32 0, label %match.arm0
    i32 1, label %match.arm1
    i32 2, label %match.arm2
]

match.arm0:
    ; ... body ...
    br label %match.merge

match.default:
    ; ... wildcard/default body ...
    br label %match.merge

match.merge:
    %result = phi i32 [ %val0, %match.arm0 ], [ %val1, %match.arm1 ], ...
```

### Conditional Branch Chain

For float subjects, guarded patterns, binding patterns, or range patterns:

```llvm
; Literal pattern:
match.test0:
    %cmp = fcmp oeq double %subject, 1.0
    br i1 %cmp, label %match.arm0, label %match.test1

; Range pattern (1..10):
match.test1:
    %ge = icmp sge i32 %subject, 1
    %le = icmp sle i32 %subject, 10
    %in_range = and i1 %ge, %le
    br i1 %in_range, label %match.arm1, label %match.test2

; Binding pattern (var x):
match.test2:
    %x = alloca i32
    store i32 %subject, ptr %x
    br label %match.arm2     ; unconditional

; Guarded pattern (var x if x > 0):
match.test3:
    %x2 = alloca i32
    store i32 %subject, ptr %x2
    %guard = icmp sgt i32 %subject, 0
    br i1 %guard, label %match.arm3, label %match.test4

; Wildcard:
match.test4:
    br label %match.arm4     ; unconditional
```

### Result PHI Node

If the match expression produces a value, a PHI node merges all arm results:

```llvm
match.merge:
    %result = phi double [ %val0, %arm0.end ],
                         [ %val1, %arm1.end ],
                         [ %val2, %arm2.end ]
```

---

## 4. Multi-Module Imports

### Resolution is Pure Sema

Import resolution happens entirely in Pass 1b (`SymbolTableBuilder::resolveAllImports`). The codegen `visit(ImportNode&)` is a no-op. By codegen time, all imported symbols are aliased into the importing module's scope.

### Single LLVM Module

All Mingus modules compile into a **single LLVM module** (`mingus_module`). The `ProgramNode` contains all `ModuleNode`s, visited sequentially.

### Name Mangling

Functions are mangled with their module prefix:

```llvm
; module Math { func add(int a, int b) => int { ... } }
define i32 @Math_add(i32 %a, i32 %b) { ... }

; module Main { import add from Math; }
; Call to add() in Main resolves to @Math_add
```

### Extern Deduplication

Externs declared in multiple modules but imported from one:

```mingus
// DSPLib.mingus
module DSPLib {
    extern func sin(double x) => double;
    // ...
}

// Main.mingus
module Main {
    import sin from DSPLib;   // shares the same FunctionSymbol*
    // No duplicate extern needed!
}
```

Since `defineAs()` shares the same `FunctionSymbol*`, the `functionCache_` sees one pointer and declares `sin` only once.

**Known issue**: If both modules independently declare `extern func sin(...)`, codegen creates `@sin` and `@sin.1` — the second becomes a linker error. Solution: declare externs in one module and import them.

---

## 5. String Interpolation

```mingus
var msg = "x=${x}, name=${name}";
```

### Two-Pass snprintf Approach

```llvm
; Build format string: "x=%d, name=%s"
@.str.fmt = private unnamed_addr constant [16 x i8] c"x=%d, name=%s\00"

; Pass 1: compute size
%len = call i32 @snprintf(ptr null, i32 0, ptr @.str.fmt, i32 %x, ptr %name)

; Allocate
%needed = sext i32 %len to i64
%alloc = add i64 %needed, 1
%buf = call ptr @malloc(i64 %alloc)

; Pass 2: format
%buf_size = add i32 %len, 1
call i32 @snprintf(ptr %buf, i32 %buf_size, ptr @.str.fmt, i32 %x, ptr %name)

; Register for RAII cleanup
registerRAII(%buf, @__mingus_string_free)
```

### Format Specifier Mapping

| Mingus Type | Format | Notes |
|------------|--------|-------|
| `int`, `byte`, `char`, enum | `%d` | |
| `bool` | `%d` | 0 or 1 |
| `double` | `%f` | `float` promoted to `double` first |
| `string`, `ptr` | `%s` | |

---

## 6. Reference Parameters

### Call Site

```mingus
func swap(int& a, int& b) => void { ... }
var x = 10;
var y = 20;
swap(x, y);
```

```llvm
; Caller passes alloca pointers:
call void @swap(ptr %x.alloca, ptr %y.alloca)
```

### Callee

```llvm
define void @swap(ptr %a.ref, ptr %b.ref) {
    ; %a.ref and %b.ref point directly to caller's allocas
    %tmp = load i32, ptr %a.ref
    %b.val = load i32, ptr %b.ref
    store i32 %b.val, ptr %a.ref
    store i32 %tmp, ptr %b.ref
    ret void
}
```

No copy, no alloca in callee for reference params. Reads and writes go through to the caller's stack.

**V2 improvement**: `FunctionTypeSymbol::ParameterInfo::isReference` carries the ref flag into function types themselves, so closures taking ref params work correctly. The `ArgumentsNode::isReference` vector is populated for ALL call types (direct, closure, HOF).

---

## 7. Constructor and Destructor Codegen

### Constructor

```llvm
define void @ClassName_constructor(ptr %this, <params>) {
    ; 1. Call base constructor (if inheriting):
    call void @BaseClass_constructor(ptr %this, <base_args>)

    ; 2. Store vtable pointer (overwrites base vtable):
    %vt.slot = getelementptr %ClassName, ptr %this, i32 0, i32 0
    store ptr @ClassName_vtable, ptr %vt.slot

    ; 3. Zero-init closure-typed fields:
    %field.ptr = getelementptr %ClassName, ptr %this, i32 0, i32 <idx>
    store { ptr null, ptr null }, ptr %field.ptr

    ; 4. User constructor body:
    ; ...
}
```

### Destructor

```llvm
define void @ClassName_destructor(ptr %this) {
    ; 1. User destructor body:
    ; ...

    ; 2. Auto-generated epilogue: release closure-typed fields
    %field.ptr = getelementptr %ClassName, ptr %this, i32 0, i32 <idx>
    %fat = load { ptr, ptr }, ptr %field.ptr
    %env = extractvalue { ptr, ptr } %fat, 1
    call void @__mingus_closure_release(ptr %env)

    ; 3. Chain to base destructor:
    call void @BaseClass_destructor(ptr %this)
}
```

---

## 8. Entry Point Wrapper

The `--entry` flag specifies which module function is the entry point. The compiler generates a `main` wrapper:

```llvm
define i32 @main() {
    %result = call i32 @ModuleName_entryFunction()
    ret i32 %result
}
```

This bridges the Mingus module naming convention to the C runtime's expected `main` symbol.
