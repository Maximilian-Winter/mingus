# Mingus Memory Management and Lifetimes

This document describes how Mingus manages memory: the RAII system for deterministic
destruction, closure reference counting, string memory, heap object lifecycle, and
zero-initialization patterns. Mingus has no garbage collector -- memory is managed
through RAII (stack objects), reference counting (closure environments), and manual
`new`/`delete` (heap objects).

**Source files:**
- `include/mingus/codegen/IRGenerator.h` -- RAII data structures, closure RC function caches
- `src/mingus/codegen/IRGenerator.cpp` -- All memory management codegen (~4400 lines)
- `include/mingus/sema/SemanticValidator.h` -- ScopeRAIIInfo, LambdaContext
- `src/mingus/sema/SemanticValidator.cpp` -- Pass 4: RAII tracking, capture analysis, escape detection

---

## Table of Contents

1. [Overview](#1-overview)
2. [RAII Scope Stack](#2-raii-scope-stack)
3. [Scope Destruction](#3-scope-destruction)
4. [Closure Reference Counting](#4-closure-reference-counting)
5. [Closure Lifecycle](#5-closure-lifecycle)
6. [Struct Cleanup](#6-struct-cleanup)
7. [Class Destructor Epilogue](#7-class-destructor-epilogue)
8. [String Memory](#8-string-memory)
9. [Heap Objects](#9-heap-objects)
10. [Lambda RAII Isolation](#10-lambda-raii-isolation)
11. [Escape Analysis](#11-escape-analysis)
12. [Weak Captures](#12-weak-captures)
13. [Universal Zero-Init](#13-universal-zero-init)
14. [Known Memory Limitations](#14-known-memory-limitations)

---

## 1. Overview

Mingus uses three complementary memory management strategies:

| Strategy | Applies To | Mechanism |
|----------|-----------|-----------|
| **RAII** (stack) | Local variables, struct/class instances, closures, strings | Deterministic destruction at scope exit in LIFO order |
| **Reference counting** (heap) | Closure capture environments | `retain`/`release` with per-closure cleanup functions |
| **Manual** (heap) | Objects created with `new` | Programmer calls `delete`; virtual destructor dispatch via vtable |

There is no garbage collector. The compiler inserts all cleanup code at compile time.
Stack-allocated objects are automatically destroyed when they go out of scope. Heap-allocated
closure environments are reference-counted so they can be shared across multiple closures
and survive past the creating scope. Heap-allocated class instances created with `new` must
be explicitly freed with `delete`.

### Allocation Summary

| Declaration | Allocation | Cleanup |
|------------|-----------|---------|
| `var x = 42;` | `alloca` (stack) | None needed (primitive) |
| `var arr = DynamicArray(8);` | `alloca` (stack) | Destructor called via RAII |
| `var f = [=](int x) => { ... };` | Fat pointer on stack, env on heap | RAII releases env; RC frees at zero |
| `var p = new Foo(1);` | `malloc` (heap) | Programmer calls `delete p;` |
| `var s = a + b;` (strings) | `malloc` (heap) | `__mingus_string_free` via RAII |

---

## 2. RAII Scope Stack

### Data Structures

The RAII system is built on two data structures: a compile-time scope tracker in semantic
analysis (Pass 4) and a runtime scope stack in codegen.

**Semantic analysis** (`SemanticValidator.h`):

```cpp
struct ScopeRAIIInfo {
    struct Destructible {
        VariableSymbol* variable;      // the variable
        DestructorSymbol* destructor;  // its class destructor
    };
    std::vector<Destructible> destructibles;
};
```

Pass 4 builds a `map<Scope*, ScopeRAIIInfo>` identifying which variables in each scope
need destructor calls. This is exported to codegen via `getRAIIInfo()`.

**Codegen** (`IRGenerator.h`):

```cpp
struct RAIIScope {
    std::vector<std::pair<llvm::Value*, llvm::Function*>> destructibles;
    std::set<Symbol*> returnedVars;  // suppress cleanup for returned values
};
std::vector<RAIIScope> raiiScopeStack_;
```

Each `RAIIScope` holds a list of `(pointer, destructor_function)` pairs. The `returnedVars`
set tracks variables whose ownership is being transferred out via `return`, so their
destructors should be suppressed.

### Core Operations

Three functions manage the RAII scope stack:

```cpp
void IRGenerator::pushRAIIScope() {
    raiiScopeStack_.push_back({});
}

void IRGenerator::popRAIIScope() {
    if (!raiiScopeStack_.empty()) {
        raiiScopeStack_.pop_back();
    }
}

void IRGenerator::registerRAII(llvm::Value* ptr, llvm::Function* dtor) {
    if (!raiiScopeStack_.empty()) {
        raiiScopeStack_.back().destructibles.push_back({ptr, dtor});
    }
}
```

### Scope Boundaries

| Event | Action |
|-------|--------|
| Function body entry | `pushRAIIScope()` |
| Block `{ }` entry | `pushRAIIScope()` |
| Variable declaration (with RAII type) | `registerRAII(alloca, dtor)` on innermost scope |
| Block `{ }` exit (no terminator) | `emitScopeDestructors()` + `popRAIIScope()` |
| Function body end (no terminator) | `emitScopeDestructors()` + `popRAIIScope()` |
| `return` statement | `emitReturnDestructors()` (all scopes) |
| `break` / `continue` | `emitBreakDestructors()` (loop scopes only) |

### What Gets Registered

| Object Type | Destructor Function | Registration Site |
|------------|-------------------|-------------------|
| Class instance (stack) | `ClassName_destructor` | `visit(VariableDeclaration)` |
| Struct with closure fields | `__struct_cleanup_<Name>` | `visit(VariableDeclaration)` |
| Closure-typed variable | `__mingus_closure_release_wrapper` | `visit(VariableDeclaration)` |
| String concat result | `__mingus_string_free` | `emitStringConcat()` |
| Interpolated string buffer | `__mingus_string_free` | `visit(InterpolatedStringExpression)` |
| Temporary closure argument | `__mingus_closure_release_wrapper` | `visit(CallExpression)` |

Registration in `visit(VariableDeclaration)`:

```cpp
// Register RAII for class types with destructors
if (varSym->getType() && varSym->getType()->is<ClassSymbol>()) {
    auto* classSym = varSym->getType()->as<ClassSymbol>();
    if (classSym && classSym->hasRAII()) {
        auto dtorIt = functionCache_.find(classSym->destructor.get());
        if (dtorIt != functionCache_.end()) {
            registerRAII(alloca, dtorIt->second);
        }
    }
}

// Register RAII for structs with closure-typed fields
if (varSym->getType() && varSym->getType()->is<StructSymbol>()) {
    auto* structSym = varSym->getType()->as<StructSymbol>();
    if (structSym && structSym->needsCleanup()) {
        registerRAII(alloca, getOrCreateStructCleanupFn(structSym));
    }
}

// Register RAII for closure-typed variables
if (varSym->getType() && varSym->getType()->is<FunctionTypeSymbol>()) {
    registerRAII(alloca, getOrCreateClosureReleaseWrapper());
}
```

---

## 3. Scope Destruction

Three distinct destruction paths handle scope exit, early return, and loop control flow.

### Normal Scope Exit -- `emitScopeDestructors()`

Called when a block `{ }` or function body exits normally (falls through without a
terminator). Iterates the **current scope only**, in **reverse order** (LIFO):

```cpp
void IRGenerator::emitScopeDestructors() {
    if (raiiScopeStack_.empty()) return;
    auto& scope = raiiScopeStack_.back();
    for (auto it = scope.destructibles.rbegin();
         it != scope.destructibles.rend(); ++it) {
        // Check if this variable is being returned (skip if so)
        bool skip = false;
        for (auto& [sym, val] : namedValues_) {
            if (val == it->first && scope.returnedVars.count(sym)) {
                skip = true;
                break;
            }
        }
        if (!skip && it->second) {
            builder_.CreateCall(it->second, {it->first});
        }
    }
}
```

The `returnedVars` check implements return value suppression: when a `return` statement
names a variable (e.g., `return arr;`), that variable's symbol is added to the scope's
`returnedVars` set, and its destructor is skipped. This transfers ownership to the caller.

### BlockStatement guard: The codegen for `BlockStatementNode` checks for a terminator
before emitting scope destructors, because `return` statements already call
`emitReturnDestructors()`:

```cpp
void IRGenerator::visit(BlockStatementNode& node) {
    pushRAIIScope();
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
        if (builder_.GetInsertBlock()->getTerminator()) break;
    }
    if (!builder_.GetInsertBlock()->getTerminator()) {
        emitScopeDestructors();
    }
    popRAIIScope();
}
```

### Early Return -- `emitReturnDestructors()`

Called from `visit(ReturnStatement)`. Walks **ALL** active scopes from innermost to
outermost, destroying everything in LIFO order:

```cpp
void IRGenerator::emitReturnDestructors() {
    for (auto scopeIt = raiiScopeStack_.rbegin();
         scopeIt != raiiScopeStack_.rend(); ++scopeIt) {
        for (auto it = scopeIt->destructibles.rbegin();
             it != scopeIt->destructibles.rend(); ++it) {
            bool skip = false;
            for (auto& [sym, val] : namedValues_) {
                if (val == it->first && scopeIt->returnedVars.count(sym)) {
                    skip = true;
                    break;
                }
            }
            if (!skip && it->second) {
                builder_.CreateCall(it->second, {it->first});
            }
        }
    }
}
```

This ensures that an early `return` from inside a nested block still cleans up all
outer scopes. The return value suppression check applies at each scope level.

### Break/Continue -- `emitBreakDestructors()`

Called from `visit(BreakStatement)` and `visit(ContinueStatement)`. Only destroys
scopes created **inside** the loop body, not outer scopes:

```cpp
void IRGenerator::emitBreakDestructors() {
    for (size_t i = raiiScopeStack_.size(); i > loopRAIIScopeDepth_; --i) {
        auto& scope = raiiScopeStack_[i - 1];
        for (auto it = scope.destructibles.rbegin();
             it != scope.destructibles.rend(); ++it) {
            if (it->second) {
                builder_.CreateCall(it->second, {it->first});
            }
        }
    }
}
```

### Loop RAII Depth Tracking

The `loopRAIIScopeDepth_` field records the RAII stack depth at the point where the
loop body begins. This is the boundary -- `emitBreakDestructors()` only cleans up
scopes above this depth.

Both `for` and `while` loops save/restore the depth for nested loop support:

```cpp
// In visit(ForStatement) and visit(WhileStatement):
auto* prevExitBlock = loopExitBlock_;
auto* prevIterBlock = loopIterBlock_;
auto prevLoopRAIIDepth = loopRAIIScopeDepth_;
loopExitBlock_ = exitBB;
loopIterBlock_ = iterBB;        // or condBB for while
loopRAIIScopeDepth_ = raiiScopeStack_.size();

node.body->accept(*this);

loopExitBlock_ = prevExitBlock;
loopIterBlock_ = prevIterBlock;
loopRAIIScopeDepth_ = prevLoopRAIIDepth;
```

**Example**: Consider a `break` inside a nested block within a loop:

```
func example() {         // RAII scope 0 (function)
    var a = Resource();  // registered in scope 0
    while (true) {       // loopRAIIScopeDepth_ = 1
        var b = Res2();  // RAII scope 1 (block)
        {
            var c = Res3();  // RAII scope 2 (nested block)
            break;           // emitBreakDestructors: destroys scopes 2, 1 (not 0)
        }
    }
}                        // emitScopeDestructors: destroys scope 0
```

---

## 4. Closure Reference Counting

### Environment Struct Layout

Every capturing closure allocates a heap environment struct with a three-field RC header
followed by the captured values:

```
closure_env = {
    i64  strong_count,  // field 0 -- starts at 1
    i64  weak_count,    // field 1 -- starts at 0
    ptr  cleanup_fn,    // field 2 -- per-closure cleanup function, or null
    T0   capture_0,     // field 3 -- first captured variable
    T1   capture_1,     // field 4 -- second captured variable
    ...
}
```

The `headerOffset` is 3 (captures start at field index 3).

Capture storage depends on capture mode:
- **By-value** captures (`[x]` or `[=]`): The actual value is copied into the env struct.
- **By-reference** captures (`[&x]` or `[&]`): A `ptr` to the original stack `alloca` is stored.
- **Weak** captures (`[weak x]`): The `{ ptr, ptr }` fat pointer is stored, and the
  inner closure's envPtr is weak-retained (weak_count incremented, not strong_count).
- **Captured closures** (by value): The `{ ptr, ptr }` fat pointer is stored, and the
  inner closure's envPtr is retained (strong_count incremented).

### Fat Pointer Representation

All closures (capturing or not) are represented as fat pointers `{ ptr, ptr }`:
- **Field 0**: Function pointer (the lambda's generated function)
- **Field 1**: Environment pointer (heap-allocated env struct, or `null` if no captures)

```llvm
%fat_ptr = type { ptr, ptr }
; Field 0 = fnPtr, Field 1 = envPtr
```

### RC Runtime Functions

Five internal LLVM functions are lazily created (once per module):

**`__mingus_closure_retain(ptr %env)`** -- Increment strong_count:

```
entry:
    if %env == null -> return          ; null check (nullable closures)
do_retain:
    %sc = load i64 from %env[0]        ; strong_count
    %sc_inc = add i64 %sc, 1
    store i64 %sc_inc to %env[0]
done:
    ret void
```

**`__mingus_closure_release(ptr %env)`** -- Decrement strong_count, cleanup at zero:

```
entry:
    if %env == null -> return          ; null check
do_release:
    %sc = load i64 from %env[0]        ; strong_count
    %sc_dec = sub i64 %sc, 1
    store i64 %sc_dec to %env[0]
    if %sc_dec != 0 -> return          ; still alive
cleanup:
    %cleanup_fn = load ptr from %env[2] ; cleanup_fn at field 2
    if %cleanup_fn != null -> call %cleanup_fn(%env)
do_free:
    %wc = load i64 from %env[1]        ; weak_count
    if %wc != 0 -> return              ; weak refs still exist, keep memory alive
actual_free:
    call free(%env)
done:
    ret void
```

**`__mingus_closure_weak_retain(ptr %env)`** -- Increment weak_count:

```
entry:
    if %env == null -> return
do_retain:
    %wc = load i64 from %env[1]        ; weak_count
    %wc_inc = add i64 %wc, 1
    store i64 %wc_inc to %env[1]
done:
    ret void
```

**`__mingus_closure_weak_release(ptr %env)`** -- Decrement weak_count, free if both zero:

```
entry:
    if %env == null -> return
do_release:
    %wc = load i64 from %env[1]        ; weak_count
    %wc_dec = sub i64 %wc, 1
    store i64 %wc_dec to %env[1]
    if %wc_dec != 0 -> return          ; other weak refs exist
check_free:
    %sc = load i64 from %env[0]        ; strong_count
    if %sc != 0 -> return              ; still strongly referenced
do_free:
    call free(%env)                    ; both counts zero, safe to free
done:
    ret void
```

**`__mingus_closure_release_wrapper(ptr %alloca_ptr)`** -- RAII adapter:

```
entry:
    %fat = load { ptr, ptr } from %alloca_ptr
    %env = extractvalue %fat, 1
    call __mingus_closure_release(%env)
    ret void
```

The wrapper takes a pointer to the fat pointer's alloca (not the env directly), because
RAII `registerRAII` stores the alloca address. The wrapper loads the fat pointer, extracts
the envPtr, and calls release.

### Per-Closure Cleanup Function

When a closure captures other closures by value, a cleanup function is generated to
release the inner closures when the outer closure's env is freed. Named
`__closure_cleanup_N`:

```cpp
llvm::Function* IRGenerator::generateClosureCleanupFn(
    llvm::StructType* closureTy,
    const std::vector<SymbolPtr>& capturedVars,
    int headerOffset,
    const std::vector<CaptureMode>* captureModes)
{
    // For each captured variable:
    //   - Skip if captured by reference
    //   - If it's a FunctionTypeSymbol (closure):
    //     - Weak captures: call weak_release
    //     - Strong captures: call release
    for (size_t i = 0; i < capturedVars.size(); i++) {
        if (isByReference) continue;
        if (varSym->getType()->is<FunctionTypeSymbol>()) {
            auto* fieldPtr = b.CreateStructGEP(closureTy, env, headerOffset + i);
            auto* fatVal = b.CreateLoad(fatPtrTy, fieldPtr);
            auto* envPtr = b.CreateExtractValue(fatVal, {1});
            if (isWeak)
                b.CreateCall(getOrCreateClosureWeakReleaseFn(), {envPtr});
            else
                b.CreateCall(getOrCreateClosureReleaseFn(), {envPtr});
        }
    }
}
```

This enables recursive deep release: when a closure containing other closures is freed,
the inner closures' refcounts are decremented, potentially triggering a chain of frees.

---

## 5. Closure Lifecycle

### Creation

When a lambda expression with captures is evaluated, codegen:

1. **Allocates** the env struct on the heap via `malloc`:
   ```llvm
   %closure.ptr = call ptr @malloc(i64 <sizeof(env_struct)>)
   ```

2. **Initializes refcount** to 1:
   ```llvm
   %rc.slot = getelementptr %closure_ty, ptr %closure.ptr, 0, 0
   store i64 1, ptr %rc.slot
   ```

3. **Stores cleanup function** (or null if no inner closures):
   ```llvm
   %cleanup.slot = getelementptr %closure_ty, ptr %closure.ptr, 0, 1
   store ptr @__closure_cleanup_N, ptr %cleanup.slot  ; or null
   ```

4. **Copies captured values** into env fields:
   ```llvm
   ; By-value: load value from outer alloca, store into env
   %val = load i32, ptr %outer_x
   %cap.slot = getelementptr %closure_ty, ptr %closure.ptr, 0, 2
   store i32 %val, ptr %cap.slot

   ; By-reference: store pointer to outer alloca directly
   store ptr %outer_x, ptr %cap.slot

   ; Captured closures: copy fat pointer + retain inner env
   %inner.fat = load { ptr, ptr }, ptr %outer_closure
   store { ptr, ptr } %inner.fat, ptr %cap.slot
   %inner.env = extractvalue %inner.fat, 1
   call void @__mingus_closure_retain(ptr %inner.env)
   ```

5. **Builds fat pointer** `{ fnPtr, envPtr }`:
   ```llvm
   %fat = insertvalue { ptr, ptr } undef, ptr @__lambda_0, 0
   %fat.1 = insertvalue { ptr, ptr } %fat, ptr %closure.ptr, 1
   ```

For non-capturing lambdas, the fat pointer has `null` as the envPtr (no heap allocation).

### Variable Assignment

When the fat pointer is stored into a local variable's alloca, the RAII system registers
the alloca with `__mingus_closure_release_wrapper`:

```cpp
registerRAII(alloca, getOrCreateClosureReleaseWrapper());
```

The refcount remains 1. At scope exit, RAII calls the wrapper, which releases the env.

### Reassignment

When a closure variable is reassigned, codegen releases the old env before storing the new value:

```cpp
// Release old closure envPtr before overwriting
if (isFunctionKind(targetType)) {
    auto* oldFat = builder_.CreateLoad(getFatPtrType(), targetPtr, "old.fat");
    auto* oldEnv = builder_.CreateExtractValue(oldFat, {1}, "old.env");
    builder_.CreateCall(getOrCreateClosureReleaseFn(), {oldEnv});
}
// Store new value
builder_.CreateStore(lastValue_, targetPtr);
```

### Retain-on-Field-Store

When assigning a closure to a struct or class **field** (via `MemberAccessExpression`),
an extra retain is needed because the source variable's RAII will also release its own
reference:

```cpp
// Retain new closure envPtr when storing into a field
if (isFunctionKind(targetType)) {
    bool isFieldStore = (node.target->as<MemberAccessExpression>() != nullptr);
    if (!isFieldStore) {
        if (auto* ident = node.target->as<IdentifierExpression>()) {
            if (auto* vs = ident->resolvedSymbol->as<VariableSymbol>()) {
                isFieldStore = (vs->role == VariableRole::Field);
            }
        }
    }
    if (isFieldStore) {
        auto* newEnv = builder_.CreateExtractValue(lastValue_, {1}, "new.env");
        builder_.CreateCall(getOrCreateClosureRetainFn(), {newEnv});
    }
}
```

Local variable reassignment does NOT retain because RAII already holds exactly one
reference for the alloca.

### Null Closures

Mingus supports nullable closures (`var f: (int) -> int = null;`). Null is represented
as a zero fat pointer `{ null, null }`:

```cpp
if (isFunctionKind(targetType) && llvm::isa<llvm::ConstantPointerNull>(lastValue_)) {
    lastValue_ = llvm::ConstantAggregateZero::get(getFatPtrType());
}
```

The retain and release functions both perform null checks on the envPtr, so operations
on null closures are safe no-ops.

### Self-Capturing Closures

A closure can reference the variable it is being assigned to (letrec pattern):

```mingus
var f = [=](int x) => { return f(x - 1); };
```

Pass 4 (`SemanticValidator`) detects this pattern and sets `lambda->selfCapture = true`.
Codegen patches the env struct after initial construction to store the fat pointer in
the self-capture slot:

```cpp
if (selfCaptureIdx >= 0) {
    auto* envPtr = builder_.CreateExtractValue(lastValue_, {1}, "self.env");
    auto* selfSlot = builder_.CreateStructGEP(closureTy, envPtr,
        headerOffset + selfCaptureIdx, "self.capture.slot");
    builder_.CreateStore(lastValue_, selfSlot);  // store fat ptr
}
```

The self-reference is **unretained** to avoid a guaranteed reference cycle.

---

## 6. Struct Cleanup

Structs do not have destructors, but they may contain closure-typed fields that need
cleanup. For each such struct, codegen generates a synthetic cleanup function named
`__struct_cleanup_<Name>`:

```cpp
llvm::Function* IRGenerator::getOrCreateStructCleanupFn(StructSymbol* structSym) {
    // Cached per struct name
    auto* fn = /* create internal function taking ptr %struct_ptr */;

    for (auto& field : structSym->fields) {
        if (field->getType() && field->getType()->is<FunctionTypeSymbol>()) {
            auto* fieldPtr = b.CreateStructGEP(structTy, structPtr,
                                                field->fieldIndex);
            auto* fatVal = b.CreateLoad(fatPtrTy, fieldPtr);
            auto* envPtr = b.CreateExtractValue(fatVal, {1});
            b.CreateCall(getOrCreateClosureReleaseFn(), {envPtr});
        }
    }
    b.CreateRetVoid();
    return fn;
}
```

The generated IR looks like:

```llvm
define internal void @__struct_cleanup_Oscillator(ptr %struct_ptr) {
entry:
    ; For each closure-typed field:
    %callback.cleanup = getelementptr %Oscillator, ptr %struct_ptr, 0, 2
    %callback.fat = load { ptr, ptr }, ptr %callback.cleanup
    %callback.env = extractvalue { ptr, ptr } %callback.fat, 1
    call void @__mingus_closure_release(ptr %callback.env)
    ret void
}
```

The cleanup function is registered via RAII when the struct variable is declared:

```cpp
if (structSym && structSym->needsCleanup()) {
    registerRAII(alloca, getOrCreateStructCleanupFn(structSym));
}
```

The `needsCleanup()` method on `StructSymbol` returns true when any field has a
`FunctionTypeSymbol` type.

---

## 7. Class Destructor Epilogue

Class destructors have compiler-generated cleanup code that runs **after** the user's
destructor body. This cleanup releases any closure-typed fields owned by the class:

```cpp
void IRGenerator::visit(DestructorDeclaration& node) {
    // ... emit user destructor body ...

    // Release closure-typed fields (auto-generated epilogue)
    if (!builder_.GetInsertBlock()->getTerminator()) {
        auto* structTy = getStructType(currentClassSym_);
        auto* fatPtrTy = getFatPtrType();

        for (auto& field : currentClassSym_->fields) {
            if (field->getType() && field->getType()->is<FunctionTypeSymbol>()) {
                int gepIdx = getFieldGEPIndex(currentClassSym_, field.get());
                if (gepIdx >= 0) {
                    auto* fieldPtr = builder_.CreateStructGEP(structTy, currentThisPtr_,
                                                              gepIdx);
                    auto* fatVal = builder_.CreateLoad(fatPtrTy, fieldPtr);
                    auto* envPtr = builder_.CreateExtractValue(fatVal, {1});
                    builder_.CreateCall(getOrCreateClosureReleaseFn(), {envPtr});
                }
            }
        }
    }

    // Chain to base class destructor
    if (currentClassSym_->resolvedBaseClass &&
        currentClassSym_->resolvedBaseClass->destructor) {
        builder_.CreateCall(baseDtorFn, {currentThisPtr_});
    }
    builder_.CreateRetVoid();
}
```

The destruction order within a class is:
1. User destructor body executes
2. Compiler releases all closure-typed fields (this class only, not inherited)
3. Base class destructor is called (which recursively does the same)

This mirrors C++ destruction order: derived class cleanup first, then base class.

---

## 8. String Memory

### String Constants

`StringLiteral` nodes produce global string constants via `CreateGlobalStringPtr()`.
These live in the `.rodata` section and require no allocation or cleanup.

### String Concatenation

`emitStringConcat()` for the `+` operator on strings:

```cpp
llvm::Value* IRGenerator::emitStringConcat(llvm::Value* left, llvm::Value* right) {
    llvm::Value* len1 = builder_.CreateCall(strlenCallee, {left}, "len1");
    llvm::Value* len2 = builder_.CreateCall(strlenCallee, {right}, "len2");
    llvm::Value* sum = builder_.CreateAdd(len1, len2, "sum");
    llvm::Value* total = builder_.CreateAdd(sum, ConstantInt::get(i64Ty, 1), "total");
    llvm::Value* buf = builder_.CreateCall(mallocCallee, {total}, "str.buf");
    builder_.CreateCall(strcpyCallee, {buf, left});
    builder_.CreateCall(strcatCallee, {buf, right});
    registerRAII(buf, getOrCreateStringFreeFn());
    return buf;
}
```

Every concatenation allocates a new heap buffer and registers it for RAII cleanup.
The `__mingus_string_free` function simply wraps `free(ptr)`:

```cpp
llvm::Function* IRGenerator::getOrCreateStringFreeFn() {
    // void __mingus_string_free(ptr %str)
    //   call free(%str)
    //   ret void
}
```

### String Interpolation

Interpolated strings use a two-pass `snprintf` approach:

```llvm
; Pass 1: compute needed buffer size
%len = call i32 @snprintf(ptr null, i32 0, ptr @fmt, <args>)
; Allocate buffer
%buf = call ptr @malloc(i64 %len + 1)
; Pass 2: format into buffer
call i32 @snprintf(ptr %buf, i32 %len+1, ptr @fmt, <args>)
; Register for RAII cleanup
registerRAII(%buf, @__mingus_string_free)
```

Format specifiers: `double`/`float` use `%f`, `int`/`byte`/`char`/`enum` use `%d`,
`bool` uses `%d`, `string`/`ptr` use `%s`.

### Chain Waste

Concatenation chains like `a + b + c` create intermediate buffers. `a + b` allocates
one buffer, then `(a+b) + c` allocates another. Both buffers are registered for RAII in
the same scope and persist until scope exit, even though only the final result is used.

---

## 9. Heap Objects

### `new` Expression

`new Type(args)` calls `malloc` + constructor:

```cpp
void IRGenerator::visit(NewExpression& node) {
    // Calculate object size via DataLayout
    auto& dl = module_->getDataLayout();
    uint64_t objSize = dl.getTypeAllocSize(objTy);

    // Allocate
    llvm::Value* rawPtr = builder_.CreateCall(mallocCallee, {sizeVal}, "new.ptr");

    // Call constructor (passes rawPtr as 'this')
    if (classSym->constructor) {
        std::vector<llvm::Value*> ctorArgs;
        ctorArgs.push_back(rawPtr);
        // ... add user arguments ...
        builder_.CreateCall(ctorFn, ctorArgs);
    } else {
        storeVtablePtr(rawPtr, classSym);  // still need vtable
    }
    lastValue_ = rawPtr;
}
```

Array allocation with `new Type[N]`:

```llvm
%total = mul i32 %N, <sizeof(element)>
%arr = call ptr @malloc(i32 %total)
```

The programmer is responsible for calling `delete`. Heap objects are NOT registered
for RAII -- there is no automatic cleanup.

### `delete` Statement

`delete` calls the destructor (with virtual dispatch if applicable), then `free`:

```cpp
void IRGenerator::visit(DeleteStatement& node) {
    // Evaluate target expression to get pointer
    node.target->accept(*this);
    llvm::Value* ptrVal = lastValue_;

    if (/* interface pointer */) {
        // Extract object pointer from fat pointer { objPtr, itablePtr }
        ptrVal = builder_.CreateExtractValue(ptrVal, {0}, "iface.del.obj");
    } else if (auto* classSym = /* class type */) {
        if (classSym->destructor) {
            if (classSym->hasVtable() && classSym->destructor->vtableIndex >= 0) {
                // VIRTUAL destructor dispatch via vtable slot 0
                auto* vtablePtrPtr = builder_.CreateStructGEP(structTy, ptrVal, 0);
                auto* vtable = builder_.CreateLoad(ptrTy, vtablePtrPtr);
                auto* dtorSlot = builder_.CreateGEP(ptrTy, vtable,
                    builder_.getInt32(0));   // slot 0 = destructor
                auto* dtorFn = builder_.CreateLoad(ptrTy, dtorSlot);
                builder_.CreateCall(dtorFnTy, dtorFn, {ptrVal});
            } else {
                // Direct (non-virtual) destructor call
                builder_.CreateCall(dtorFn, {ptrVal});
            }
        }
    }

    // Always free the memory
    builder_.CreateCall(freeCallee, {ptrVal});
}
```

### Virtual Destructor Dispatch

Vtable slot 0 is always the destructor. When `delete` is called on a base-class pointer,
the virtual dispatch mechanism loads the correct destructor from the vtable:

```llvm
; delete basePtr;  (where basePtr points to a Derived)
%vtable.ptr = getelementptr %Base, ptr %basePtr, 0, 0     ; field 0 = vtable ptr
%vtable = load ptr, ptr %vtable.ptr
%dtor.slot = getelementptr ptr, ptr %vtable, i32 0         ; slot 0 = dtor
%dtor.fn = load ptr, ptr %dtor.slot                        ; -> Derived_destructor
call void %dtor.fn(ptr %basePtr)
call void @free(ptr %basePtr)
```

The destructor chain (derived -> base) is handled within each destructor's epilogue
(see [Class Destructor Epilogue](#7-class-destructor-epilogue)).

### Stack-Allocated Class Instances

Class instances declared without `new` are stack-allocated:

```mingus
var arr = DynamicArray(8);   // stack alloca, RAII cleanup
```

```llvm
%arr.tmp = alloca %DynamicArray
; store vtable ptr
call void @DynamicArray_constructor(ptr %arr.tmp, i32 8)
%arr.val = load %DynamicArray, ptr %arr.tmp
store %DynamicArray %arr.val, ptr %arr
; RAII registered -> destructor called at scope exit
```

---

## 10. Lambda RAII Isolation

Lambdas create separate LLVM functions but share the same `IRGenerator` instance. Without
isolation, the RAII scope stack from the parent function would leak into the lambda, causing
`emitReturnDestructors()` inside the lambda to walk the parent's RAII stack and create
cross-function IR references (using a Value from function A inside function B). This
crashes LLVM verification.

The fix: save, clear, and restore the entire RAII stack (plus other codegen state) when
entering and leaving a lambda:

```cpp
void IRGenerator::visit(LambdaExpression& node) {
    // CRITICAL: Save/restore state for lambda isolation
    auto* prevFunction = currentFunction_;
    auto* prevThisPtr = currentThisPtr_;
    auto savedNamedValues = namedValues_;
    auto savedInsertPoint = builder_.GetInsertBlock();
    auto savedInsertPointIt = builder_.GetInsertPoint();
    auto savedRAIIStack = std::move(raiiScopeStack_);   // <-- save
    raiiScopeStack_.clear();                             // <-- isolate

    currentFunction_ = lambdaFn;
    currentThisPtr_ = nullptr;
    namedValues_.clear();

    // ... emit lambda body with its own RAII scope ...
    pushRAIIScope();
    // ... emit body ...
    popRAIIScope();

    // Restore state
    currentFunction_ = prevFunction;
    currentThisPtr_ = prevThisPtr;
    namedValues_ = savedNamedValues;
    raiiScopeStack_ = std::move(savedRAIIStack);         // <-- restore
    builder_.SetInsertPoint(savedInsertPoint, savedInsertPointIt);
}
```

The lambda gets its own clean RAII stack. After the lambda body is emitted, the parent's
RAII stack is restored, and codegen continues in the parent function's context (building
the closure env struct and fat pointer).

---

## 11. Escape Analysis

Pass 4 (`SemanticValidator`) performs a simple escape analysis for lambda expressions.
Lambdas that are passed directly as function arguments (without being stored in a variable
first) are marked as non-escaping:

```cpp
void SemanticValidator::visit(CallExpression& node) {
    if (node.arguments) {
        for (size_t i = 0; i < node.arguments->expressions.size(); i++) {
            auto& arg = node.arguments->expressions[i];
            if (arg) {
                arg->accept(*this);

                // Non-escaping lambda detection: lambdas passed directly
                // as arguments don't escape (can be stack-allocated)
                if (auto* lambda = arg->as<LambdaExpression>()) {
                    lambda->escapes = false;
                }
            }
        }
    }
}
```

By default, `LambdaExpression::escapes` is `true`. When a lambda is passed directly as a
call argument (e.g., `arr.map([=](int x) => { return x * 2; })`), it is marked
`escapes = false`.

This analysis result is available for codegen to use as a stack-allocation optimization:
a non-escaping closure's env struct could be `alloca`'d on the stack instead of `malloc`'d
on the heap, avoiding allocation overhead and eliminating the need for reference counting.
The current codegen does not yet use this information for optimization, but the analysis
is in place for future use.

---

## 12. Weak Captures

### Purpose

Weak captures (`[weak x]`) provide non-owning references to closure environments, enabling
cycle breaking in mutual closure references. Without weak captures, if closure A captures B
and B captures A, neither is ever freed (classic ARC cycle problem).

### Syntax

```mingus
var callback = [=]() => { return 42; };

// Strong capture (default) — increments strong_count
var strong = [=]() => { callback(); };

// Weak capture — increments weak_count, not strong_count
var weak_ref = [weak callback]() => {
    if (callback != null) {
        return callback();
    }
    return -1;
};

// Mixed captures — strong default with one weak override
var mixed = [=, weak callback]() => {
    // x is captured strongly (by-value default)
    // callback is captured weakly
};
```

### Semantics

**At capture time** (closure creation):
- The fat pointer `{ fnPtr, envPtr }` is stored in the env struct (same as by-value).
- `weak_retain(envPtr)` is called — increments `weak_count`, NOT `strong_count`.

**At access time** (lambda body entry):
Three-way check determines if the weak capture is alive:
1. If `fnPtr == null` → genuinely null closure → dead (store null fat pointer)
2. If `fnPtr != null` and `envPtr == null` → capture-less closure → always alive (use directly)
3. If `envPtr != null` → check `strong_count > 0`:
   - Alive: retain (promote to temporary strong ref), use, release via RAII at scope exit
   - Dead: store null fat pointer `{ null, null }`

**At cleanup time** (outer closure freed):
- `weak_release(envPtr)` is called instead of `release(envPtr)`.
- `weak_release` decrements `weak_count`. If both `weak_count` and `strong_count` are 0,
  `free()` is called. Otherwise, memory stays alive for remaining weak/strong refs.

### Cycle Breaking Example

```mingus
var a = [=]() => { b(); };          // a captures b strongly
var b = [weak a]() => { a(); };     // b captures a weakly

// a: strong=1 (var), weak=1 (b's weak capture)
// b: strong=2 (var + a's capture), weak=0

// At scope exit:
// 1. b released: strong 2→1 (still alive — a holds it)
// 2. a released: strong 1→0 → cleanup:
//    - a's cleanup releases b: strong 1→0 → b freed
//    - After a's cleanup: check weak_count=1 > 0 → don't free a yet
// 3. b's cleanup: weak_release(a): weak 1→0, strong=0 → free(a)
// Both freed. No leak.
```

### Restrictions

- `[weak x]` is only valid when `x` has a closure type (`FunctionTypeSymbol`). Weak captures
  of non-closure types (integers, structs, etc.) are rejected by the semantic validator.
- Dead weak captures resolve to null. The programmer must null-check before calling.

---

## 13. Universal Zero-Init

Four distinct zero-initialization sites prevent use of uninitialized memory:

### 1. Closure-typed Local Variables

Before any initializer runs, the alloca is zeroed to prevent `__mingus_closure_release`
from calling `free` on garbage if the variable is reassigned before initialized:

```cpp
if (varSym->getType() && varSym->getType()->is<FunctionTypeSymbol>()) {
    builder_.CreateStore(llvm::ConstantAggregateZero::get(varTy), alloca);
}
```

```llvm
store { ptr null, ptr null }, ptr %closure_alloca
```

### 2. All Struct and Class Construction

ALL struct and class allocas are zeroed using `zeroinitializer` (not `undef`):

```cpp
if (varSym->getType() && (varSym->getType()->is<StructSymbol>() ||
                           varSym->getType()->is<ClassSymbol>())) {
    builder_.CreateStore(llvm::Constant::getNullValue(varTy), alloca);
}
```

This prevents NaN/garbage propagation in accumulator patterns where fields are read
before explicit assignment (e.g., `mix = mix + osc`). Prior to this change, `undef`
values caused non-deterministic NaN results in floating-point accumulation.

### 3. Class Constructor Closure-Field Init

After `storeVtablePtr` and before the user constructor body, all closure-typed fields
are explicitly zeroed:

```cpp
// In visit(ConstructorDeclaration):
storeVtablePtr(currentThisPtr_, currentClassSym_);

// Zero-init closure-typed fields
for (auto& field : currentClassSym_->fields) {
    if (field->getType() && field->getType()->is<FunctionTypeSymbol>()) {
        int gepIdx = getFieldGEPIndex(currentClassSym_, field.get());
        if (gepIdx >= 0) {
            auto* fieldPtr = builder_.CreateStructGEP(structTy, currentThisPtr_, gepIdx);
            builder_.CreateStore(llvm::ConstantAggregateZero::get(fatPtrTy), fieldPtr);
        }
    }
}
// ... user constructor body runs ...
```

This ensures that if the user constructor does not assign all closure fields, the
destructor epilogue's release calls encounter null envPtrs (which are safe no-ops)
rather than garbage pointers.

### 4. Null-to-Zero Fat Pointer Conversion

When assigning `null` to a closure-typed variable, `ConstantPointerNull` is converted
to a zero fat pointer:

```cpp
if (isFunctionKind(targetType) && llvm::isa<llvm::ConstantPointerNull>(lastValue_)) {
    lastValue_ = llvm::ConstantAggregateZero::get(getFatPtrType());
}
```

This ensures `null` closures are represented as `{ null, null }` rather than a bare
null pointer, maintaining the fat pointer invariant.

---

## 14. Known Memory Limitations

### By-Reference Captures That Escape

`[&x]` captures store a pointer to the original stack alloca. If the closure escapes
the scope where `x` is declared (e.g., returned or stored in a data structure), the
pointer becomes dangling. This is programmer responsibility, same as C++:

```mingus
func makeCounter() -> (int) -> int {
    var count = 0;
    return [&count](int n) => { count = count + n; return count; };
    // BUG: count is on makeCounter's stack, which is gone after return
}
```

### Temporary Closure Argument Leak

Closures passed directly as function arguments without variable storage get RAII
cleanup via a temporary alloca, but the interaction between the temporary and the
callee's potential retain is imperfect. In some cases, one refcount may leak:

```mingus
arr.map([=](int x) => { return x * 2; });
// The temporary closure alloca gets RAII cleanup, but edge cases exist
```

### Captures are Copy-on-Entry

By-value captures (`[x]` or `[=]`) copy the value into the env struct at creation
time, then copy again into a local alloca inside the lambda body. Writes inside the
lambda modify only the local copy -- they are never written back to the env or outer
variable:

```mingus
var x = 10;
var f = [=](int n) => { x = x + n; return x; };
// Modifying x inside f does NOT affect the outer x
```

### Self-Capturing Closures and Cycles

Self-references in the env struct are unretained (to avoid a guaranteed reference
cycle). If the closure is freed while still calling itself, the self-reference
becomes dangling:

```mingus
var f = [=](int x) => { return f(x - 1); };
// Self-reference is unretained -- safe as long as f stays alive
```

### No Cycle Detection

The reference counting system has no cycle detection. If closures form a reference
cycle (A captures B by value, B captures A by value), neither will ever be freed:

```mingus
var a = [=]() => { b(); };   // a captures b
var b = [=]() => { a(); };   // b captures a
// Reference cycle -- both leak
```

### String Concatenation Chain Waste

`a + b + c` creates two heap buffers: one for `a+b` and one for `(a+b)+c`. Both
are registered for RAII. The intermediate buffer persists for the entire scope even
though its contents have been copied into the final buffer.

### No Debug Info for RC Operations

Retain, release, and destructor calls have no `DebugLoc` attached -- they appear
as "unknown location" in debuggers and cannot be stepped through meaningfully.

### Move Semantics and Ownership Transfer

Move constructors (`constructor(ClassName&& other)`) enable transferring ownership
of resources between objects. The move constructor body is user-written and typically:

1. Copies field values from the source to the destination
2. Zeros the source's fields to prevent double-free or stale access
3. Marks the source as "moved" (optional, for debugging)

```mingus
constructor(Resource&& other)
{
    this.value = other.value;
    other.value = 0;       // zero source to prevent double-free
    other.moved = 1;       // optional: mark as moved
}
```

**Interaction with RAII and destructors**: After a move, the source object still exists
and its destructor will be called at scope exit. The move constructor must leave the
source in a valid state (typically zeroed) so the destructor is a safe no-op. For
closure-typed fields, zeroing ensures the destructor epilogue's release calls encounter
null envPtrs (safe no-ops) rather than dangling pointers to transferred environments.

Both `&` (reference) and `&&` (rvalue reference) map to `ptr` in LLVM IR -- the
distinction is purely semantic, controlling which constructor the `NewExpression`
dispatches to.
