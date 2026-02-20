# Mingus Memory Management and Lifetimes

This document describes how Mingus manages memory: stack vs heap allocation, the RAII system, closure reference counting, string memory, and zero-initialization patterns.

**Source files:**
- `src/mingus/codegen/IRGenerator.cpp` — All memory management codegen
- `include/mingus/codegen/IRGenerator.h` — RAII data structures
- `src/mingus/sema/SemanticValidator.cpp` — RAII tracking (Pass 4)

---

## Table of Contents

1. [Stack vs Heap Allocation](#1-stack-vs-heap-allocation)
2. [The RAII System](#2-the-raii-system)
3. [Closure Reference Counting](#3-closure-reference-counting)
4. [String Memory](#4-string-memory)
5. [Zero-Initialization Patterns](#5-zero-initialization-patterns)
6. [Memory Limitations](#6-memory-limitations)

---

## 1. Stack vs Heap Allocation

### Local Variables — Stack (`alloca`)

All local variables are stack-allocated using LLVM `alloca` in the function's entry block:

```llvm
; Variable: var x = 42;
%x = alloca i32, align 4       ; entry block
store i32 42, ptr %x            ; initialization
```

The `createEntryBlockAlloca()` helper places all allocas at the start of the entry block, following LLVM best practice (enables mem2reg promotion).

Variables are tracked in `namedValues_[symbol] → alloca_ptr`.

### Function Parameters

Parameters are also stack-allocated with allocas in the entry block:

```llvm
define i32 @func(i32 %x.arg) {
    %x = alloca i32
    store i32 %x.arg, ptr %x
    ; ... use %x as alloca ...
}
```

**Reference parameters** (`int& x`): The argument IS the pointer to the caller's alloca — used directly, no copy:

```llvm
define void @swap(ptr %a.ref, ptr %b.ref) {
    ; %a.ref and %b.ref point to caller's stack
    %tmp = load i32, ptr %a.ref
    ; ...
}
```

**Struct/class parameters**: Passed by pointer at the IR level, copied into local alloca for GEP access.

### `new` Expression — Heap Allocation

`new Type(args)` calls `malloc` + constructor:

```llvm
%obj = call ptr @malloc(i32 <sizeof(Type)>)
; store vtable ptr if needed
call void @ClassName_constructor(ptr %obj, <args>)
; result: %obj (raw pointer, no RAII)
```

The programmer is responsible for `delete`. No automatic RAII for heap objects.

### `new Type[N]` — Array Heap Allocation

```llvm
%total = mul i32 %N, <sizeof(element)>
%arr = call ptr @malloc(i32 %total)
```

### `delete` Statement

Calls destructor (virtual if applicable), then `free`:

```llvm
; For virtual destructor:
%vtable = load ptr, ptr %obj         ; load vtable from slot 0
%dtor.slot = getelementptr ptr, ptr %vtable, i32 0
%dtor = load ptr, ptr %dtor.slot
call void %dtor(ptr %obj)            ; virtual destructor call
call void @free(ptr %obj)
```

### Stack-Allocated Class Instances

`var arr = DynamicArray(8);` without `new` allocates on the stack:

```llvm
%arr.tmp = alloca %DynamicArray
; store vtable ptr
call void @DynamicArray_constructor(ptr %arr.tmp, i32 8)
%arr.val = load %DynamicArray, ptr %arr.tmp
store %DynamicArray %arr.val, ptr %arr
; RAII registered → destructor called at scope exit
```

---

## 2. The RAII System

### Data Structures

```cpp
struct RAIIScope {
    vector<pair<llvm::Value*, llvm::Function*>> destructibles;  // (ptr, dtor_fn)
    set<sema::Symbol*> returnedVars;  // suppress cleanup for returned values
};
vector<RAIIScope> raiiScopeStack_;
```

### Scope Stack Lifecycle

| Event | Action |
|-------|--------|
| Function body start | `pushRAIIScope()` |
| Block `{ }` entry | `pushRAIIScope()` |
| Variable declaration | `registerRAII(ptr, dtor)` on innermost scope |
| Block `{ }` exit | `emitScopeDestructors()` + pop |
| Function body end | `emitScopeDestructors()` + pop |
| `return` statement | `emitReturnDestructors()` (all scopes) |
| `break`/`continue` | `emitBreakDestructors()` (loop scopes only) |

### What Gets Registered

| Object Type | Destructor Function | Registration Site |
|------------|-------------------|-------------------|
| Class instance (stack) | `ClassName_destructor` | `VariableDeclaration` |
| Struct with closure fields | `__struct_cleanup_<Name>` | `VariableDeclaration` |
| Closure-typed variable | `__mingus_closure_release_wrapper` | `VariableDeclaration` |
| String concat result | `__mingus_string_free` | `emitStringConcat` |
| Interpolated string buffer | `__mingus_string_free` | `InterpolatedString` |
| Temporary closure argument | `__mingus_closure_release_wrapper` | `CallExpression` |

### Destruction Order

**Normal scope exit** (`emitScopeDestructors`): Reverse registration order (LIFO) within the current scope only.

```cpp
// Iterates destructibles in reverse
for (auto it = scope.destructibles.rbegin(); it != scope.destructibles.rend(); ++it) {
    if (!isReturned(it->first))
        builder_.CreateCall(it->second, {it->first});
}
```

**Early return** (`emitReturnDestructors`): Walks ALL active scopes from inner to outer, LIFO within each:

```cpp
for (auto scopeIt = raiiScopeStack_.rbegin(); scopeIt != raiiScopeStack_.rend(); ++scopeIt) {
    for (auto it = scopeIt->destructibles.rbegin(); ...) {
        // call dtor
    }
}
```

**Break/Continue** (`emitBreakDestructors`): Only destroys scopes created INSIDE the loop body. Uses `loopRAIIScopeDepth_` as the boundary:

```cpp
// Only scopes at index > loopRAIIScopeDepth_ are destroyed
for (size_t i = raiiScopeStack_.size(); i > loopRAIIScopeDepth_; --i) {
    // call dtors in reverse
}
```

`loopRAIIScopeDepth_` is saved/restored at loop entry/exit to handle nested loops correctly.

### Return Value Suppression

When returning a named variable, its destructor is suppressed:

```cpp
// var arr = DynamicArray(8);
// return arr;  ← don't destroy arr, caller takes ownership
scope.returnedVars.insert(arrSymbol);
```

### Lambda RAII Isolation

Lambdas create separate LLVM functions but share the same `IRGenerator`. The RAII stack is saved and cleared before entering a lambda, then restored after:

```cpp
auto savedRAIIStack = std::move(raiiScopeStack_);
raiiScopeStack_.clear();
// ... emit lambda body ...
raiiScopeStack_ = std::move(savedRAIIStack);
```

Without this, `emitReturnDestructors()` inside a lambda would walk the parent function's RAII stack, creating cross-function IR references that crash LLVM.

---

## 3. Closure Reference Counting

### Environment Struct Layout

Every capturing closure allocates a heap environment:

```
closure_env = {
    i64  refcount,     ; field 0 — starts at 1
    ptr  cleanup_fn,   ; field 1 — per-closure cleanup, or null
    T0   capture_0,    ; field 2 — first captured variable
    T1   capture_1,    ; field 3 — second captured variable
    ...
}
```

- By-value captures: store the actual value
- By-reference captures: store a `ptr` to the original `alloca`
- Captured closures (by value): store the `{ ptr, ptr }` fat pointer + retain the inner envPtr

### RC Runtime Functions

Three internal LLVM functions (lazily created, one per module):

**`__mingus_closure_retain(ptr %env)`**
```
if %env == null → return
%rc = load i64 from %env[0]
%rc_inc = add i64 %rc, 1
store i64 %rc_inc to %env[0]
```

**`__mingus_closure_release(ptr %env)`**
```
if %env == null → return
%rc = load i64 from %env[0]
%rc_dec = sub i64 %rc, 1
store i64 %rc_dec to %env[0]
if %rc_dec != 0 → return
; refcount hit zero:
%cleanup = load ptr from %env[1]
if %cleanup != null → call %cleanup(%env)
call free(%env)
```

**`__mingus_closure_release_wrapper(ptr %alloca_ptr)`**
```
; RAII adapter — takes alloca pointer, extracts envPtr, calls release
%fat = load { ptr, ptr } from %alloca_ptr
%env = extractvalue %fat, 1
call __mingus_closure_release(%env)
```

### Per-Closure Cleanup Function

Generated for closures that capture other closures by value. Named `__closure_cleanup_N`:

```
define internal void @__closure_cleanup_0(ptr %env) {
    ; For each by-value-captured closure field:
    %field = getelementptr %closure_ty, ptr %env, 0, <2+i>
    %fat = load { ptr, ptr }, ptr %field
    %inner_env = extractvalue %fat, 1
    call __mingus_closure_release(%inner_env)
}
```

This enables recursive deep release: when a closure containing other closures is freed, the inner closures' refcounts are decremented.

### Lifecycle

1. **Creation**: `malloc(sizeof(env))`, refcount = 1, captures stored
2. **Variable assignment**: Fat pointer stored in alloca, `release_wrapper` registered via RAII
3. **Reassignment**: Release old envPtr, store new fat pointer. Retain only for struct/class field assignments.
4. **Scope exit**: RAII calls `release_wrapper(alloca)` → releases envPtr
5. **Refcount zero**: Cleanup function runs (releases inner closures), then `free(env)`

### Retain-on-Field-Store

When assigning a closure to a struct/class **field** (not a local variable):

```cpp
// Assignment to obj.closure_field:
release(old_env);        // release what was there before
store new_fat to field;
retain(new_env);         // because source variable's RAII will also release
```

Local variable reassignment does NOT retain because RAII already holds exactly one reference.

### Struct Cleanup Functions

For structs with closure-typed fields, `__struct_cleanup_<Name>` releases all closure fields:

```
define internal void @__struct_cleanup_Foo(ptr %struct_ptr) {
    ; For each closure-typed field:
    %field = getelementptr %Foo, ptr %struct_ptr, 0, <fieldIndex>
    %fat = load { ptr, ptr }, ptr %field
    %env = extractvalue %fat, 1
    call __mingus_closure_release(%env)
}
```

### Class Destructor Epilogue

After the user's destructor body runs, the compiler auto-generates cleanup for closure-typed fields:

```
; In ClassName_destructor, after user body:
; For each closure-typed field in this class (not base):
%field_ptr = getelementptr %ClassName, ptr %this, 0, <gepIdx>
%fat = load { ptr, ptr }, ptr %field_ptr
%env = extractvalue %fat, 1
call __mingus_closure_release(%env)
; Then chain to base destructor:
call void @BaseClass_destructor(ptr %this)
```

---

## 4. String Memory

### String Constants

`StringLiteral` → `CreateGlobalStringPtr()` → global `.rodata` constant. No allocation, no RAII needed.

### String Concatenation

`emitStringConcat` for `s1 + s2`:

```
%len1 = call i32 @strlen(ptr %s1)
%len2 = call i32 @strlen(ptr %s2)
%total = add + 1
%buf = call ptr @malloc(i64 %total)
call ptr @strcpy(ptr %buf, ptr %s1)
call ptr @strcat(ptr %buf, ptr %s2)
registerRAII(%buf, @__mingus_string_free)
```

The `__mingus_string_free` function simply wraps `free(ptr)`.

Every concatenation allocates a new buffer. Concatenation chains (`a + b + c`) create intermediate buffers that are all registered for RAII cleanup at the current scope — they persist for the entire scope even though only the final buffer is used.

### String Interpolation

Two-pass `snprintf` approach:

```
; Pass 1: compute needed size
%len = call i32 @snprintf(ptr null, i32 0, ptr @fmt, <args>)
; Allocate
%buf = call ptr @malloc(i64 %len + 1)
; Pass 2: format into buffer
call i32 @snprintf(ptr %buf, i32 %len+1, ptr @fmt, <args>)
registerRAII(%buf, @__mingus_string_free)
```

Format specifiers: `double`/`float` → `%f`, `int`/`byte`/`char`/`enum` → `%d`, `bool` → `%d`, `string`/`ptr` → `%s`.

---

## 5. Zero-Initialization Patterns

Four distinct zero-init sites prevent use of uninitialized memory:

### 1. Closure-typed local variables

Before any initializer runs, the alloca is zeroed:
```llvm
store { ptr null, ptr null }, ptr %closure_alloca
```
Prevents `__closure_release` from calling `free` on garbage if the variable is reassigned before initialized.

### 2. Structs with closure fields

The entire struct alloca is zeroed:
```llvm
store %StructType zeroinitializer, ptr %struct_alloca
```
Ensures closure fields start as null fat pointers.

### 3. All struct construction

All struct literal construction uses `zeroinitializer` (not `undef`):
```llvm
%s = zeroinitializer  ; not undef
```
Prevents NaN/garbage in accumulator patterns where fields are read before explicit assignment (e.g., `mix = mix + osc`).

### 4. Class constructor closure-field init

After `storeVtablePtr` and before the user constructor body, all closure-typed fields are zeroed:
```llvm
; In constructor prologue:
%field = getelementptr %Class, ptr %this, 0, <field_idx>
store { ptr null, ptr null }, ptr %field
```

---

## 6. Memory Limitations

### Temporary Closure Leak

Closures passed directly as function arguments (without variable storage) leak one refcount in some cases. The compiler creates a temporary alloca with RAII cleanup, but the interaction between the temporary and the callee's potential retain is imperfect.

### Captures are Copy-on-Entry

By-value captures copy the value into the env struct at creation time, then copy again into a local alloca inside the lambda body. Writes modify the local copy only — never written back to the env or outer variable. Workaround: capture by reference (`[&x]`) or capture a pointer (`var ptr = &obj;`).

### Self-Capturing Closures

Self-references in the env struct are **unretained** (to avoid reference cycles). If the closure is freed while still calling itself, the self-reference becomes dangling.

### String Concatenation Chain Waste

`a + b + c` creates two heap buffers: one for `a+b` and one for `(a+b)+c`. Both are registered for RAII. The intermediate buffer persists for the entire scope even though its contents have been copied. Not a correctness bug, but a space inefficiency.

### No Debug Info for RC Operations

Retain/release/destructor calls have no `DebugLoc` attached — they appear as "unknown location" in debuggers.

### No Cycle Detection

The reference counting system has no cycle detection. If closures form a reference cycle (A captures B, B captures A), neither will ever be freed.
