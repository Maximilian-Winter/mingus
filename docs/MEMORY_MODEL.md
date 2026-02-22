# Mingus Memory Model

Complete reference for how Mingus allocates, stores, and reclaims memory.
Mingus has no garbage collector -- all memory management is resolved at compile time
through four complementary strategies: stack RAII, reference counting, shared pointers,
and manual `new`/`delete`.

**Source files:**
- `include/mingus/codegen/IRGenerator.h` -- RAII data structures, RC function caches
- `src/mingus/codegen/IRGenerator.cpp` -- all memory management codegen
- `include/mingus/sema/SemanticValidator.h` -- ScopeRAIIInfo, LambdaContext
- `src/mingus/sema/SemanticValidator.cpp` -- Pass 4: RAII tracking, capture analysis

---

## Table of Contents

1. [Overview](#1-overview)
2. [Stack Allocations](#2-stack-allocations)
3. [Heap Allocations](#3-heap-allocations)
4. [Global and Static Data](#4-global-and-static-data)
5. [RAII Scope Stack](#5-raii-scope-stack)
6. [Deallocation Patterns](#6-deallocation-patterns)
7. [Value vs Reference Semantics](#7-value-vs-reference-semantics)
8. [Array Memory](#8-array-memory)
9. [Summary Tables](#9-summary-tables)

---

## 1. Overview

Every value in Mingus lives in exactly one of three memory regions:

| Region | Lifetime | Examples |
|--------|----------|----------|
| **Stack** | Scope-bound, deterministic | Local variables, parameters, fixed-size arrays, temporaries |
| **Heap** | Varies by strategy | Class instances, dynamic arrays, closures, string concat results |
| **Global/static** | Program lifetime | Vtables, itables, string literals, enum string values |

Heap memory is further divided by management strategy:

| Strategy | Syntax | Cleanup | Overhead |
|----------|--------|---------|----------|
| **Manual** | `new Foo()` | Programmer calls `delete` | Zero runtime overhead |
| **Shared (ARC)** | `new shared Foo()` | Automatic RC at scope exit | 24-byte header per object |
| **Closure RC** | Implicit (lambda captures) | Automatic RC via fat pointer | 24-byte header per environment |
| **String RAII** | `a + b` (concat) | Automatic free at scope exit | Registered in RAII stack |

---

## 2. Stack Allocations

All stack allocations use LLVM `alloca` instructions placed in the function entry block
via `createEntryBlockAlloca()` (IRGenerator.cpp:360). This ensures consistent stack frame
layout regardless of control flow.

### 2.1 Local Variables

Every local variable declaration produces an `alloca`:

```mingus
int x = 42;           // alloca i32
float pi = 3.14;      // alloca double
string name = "Rex";  // alloca ptr (pointer to constant string data)
```

```llvm
%x = alloca i32, align 4
store i32 42, ptr %x
```

**Source:** `visit(VariableDeclaration&)` at IRGenerator.cpp:2176

### 2.2 Struct and Class Instances (Automatic Storage)

Struct and class instances declared without `new` live on the stack:

```mingus
var arr = DynamicArray(8);   // stack-allocated class instance
Vec2 pos;                    // stack-allocated struct
```

```llvm
%arr = alloca %DynamicArray, align 8
; zero-init, then call constructor
call void @DynamicArray_constructor(ptr %arr, i32 8)
; RAII registered -> destructor called at scope exit
```

All structs and classes are zero-initialized at allocation (`getNullValue`) to prevent
undef propagation. This is critical for accumulator patterns like `mix = mix + osc`.

### 2.3 Function Parameters

Parameters are handled differently based on type and modifier:

| Parameter type | Mechanism | Copy? |
|---------------|-----------|-------|
| Primitive (`int`, `float`, etc.) | `alloca` + store arg value | Yes |
| Struct | `alloca` + load from caller's ptr + store | Yes (deep copy) |
| Reference (`int& x`) | Use caller's pointer directly | No (alias) |
| Class pointer (`Foo*`) | `alloca` + store pointer value | Pointer copied |

```cpp
// Reference parameter — no alloca, use caller's pointer directly
if (paramSym->isReference) {
    namedValues_[paramSym] = argVal;  // argVal IS the pointer
}
// Struct parameter — indirect copy
else if (isUserStructKind(paramType)) {
    auto* alloca = createEntryBlockAlloca(fn, structTy, name);
    auto* val = builder_.CreateLoad(structTy, argVal);
    builder_.CreateStore(val, alloca);
    namedValues_[paramSym] = alloca;
}
```

**Source:** `visit(FunctionDeclaration&)` at IRGenerator.cpp:1437-1451

### 2.4 Fixed-Size Arrays

```mingus
int[10] arr;     // alloca [10 x i32]
double[4] vec;   // alloca [4 x double]
```

```llvm
%arr = alloca [10 x i32], align 4
; Indexing uses 2-index GEP: [0, i]
%elem = getelementptr [10 x i32], ptr %arr, i32 0, i32 %i
```

Array size must be an integer literal. Variable-sized stack arrays are not supported.

### 2.5 Temporaries

Struct values that need to be passed to functions are stored in temporary allocas
because structs are passed by pointer at the ABI level:

```cpp
// When a struct value needs to become a pointer for a function call
if (objPtr->getType()->isStructTy()) {
    auto* tmp = createEntryBlockAlloca(currentFunction_, objPtr->getType(), "tmp");
    builder_.CreateStore(objPtr, tmp);
    objPtr = tmp;  // Pass pointer to temporary
}
```

These temporaries are cleaned up when the function returns (stack frame destroyed).

### 2.6 Other Stack Allocations

| What | When | Source |
|------|------|--------|
| Tuple destructuring bindings | `var (a, b) = tuple;` | IRGenerator.cpp:2296 |
| Match pattern bindings | `case x:` in match | IRGenerator.cpp:3968 |
| Lambda parameters | Each param in lambda body | IRGenerator.cpp:4096 |
| Weak capture promotions | Temp strong ref for alive weak captures | IRGenerator.cpp:4156 |
| Constructor `this` temp | During construction | IRGenerator.cpp:1502 |

---

## 3. Heap Allocations

All heap allocations use C `malloc()`. There is no custom allocator.

### 3.1 Raw Class Instances (`new Foo()`)

```mingus
Animal* a = new Animal(1, "Rex");
```

Allocates exactly `sizeof(Animal)` bytes. The variable holds a direct pointer to the
object. Vtable pointer is stored at field 0.

```
objPtr → [ vtable_ptr | field0 | field1 | ... ]
```

```llvm
%obj = call ptr @malloc(i32 24)          ; sizeof(Animal)
store ptr @Animal_vtable, ptr %obj       ; vtable at offset 0
call void @Animal_constructor(ptr %obj, i32 1, ptr @str.rex)
```

**No automatic cleanup.** The programmer must call `delete a;` to invoke the destructor
and free memory. Forgetting `delete` leaks memory.

**Source:** `visit(NewExpression&)` at IRGenerator.cpp:3606

### 3.2 Shared Class Instances (`new shared Foo()`)

```mingus
shared Animal* a = new shared Animal(1, "Rex");
```

Allocates `headerSize + sizeof(Animal)` bytes. The variable holds a pointer to the
RC header (not the object). The object lives immediately after the header.

```
rcPtr → [ i64 strong | i64 weak | ptr cleanup | vtable_ptr | field0 | field1 | ... ]
         ├──────── RC header (24 bytes) ────────┤├──────── object ────────────────────┤
                                                 ^
                                                 objPtr = rcPtr + 24
```

```llvm
%rc = call ptr @malloc(i32 48)                    ; 24 (header) + 24 (Animal)
store i64 1, ptr %rc                              ; strong_count = 1
%weak = getelementptr i8, ptr %rc, i64 8
store i64 0, ptr %weak                            ; weak_count = 0
%cleanup = getelementptr i8, ptr %rc, i64 16
store ptr @__shared_cleanup_Animal, ptr %cleanup   ; cleanup function
%obj = getelementptr i8, ptr %rc, i64 24           ; object starts after header
store ptr @Animal_vtable, ptr %obj                 ; vtable
call void @Animal_constructor(ptr %obj, ...)       ; construct at objPtr
```

**Automatic cleanup.** RAII release wrapper decrements strong count at scope exit.
When strong count reaches 0, the cleanup function calls the destructor and the
release function frees the allocation.

The RC header layout is identical to closure environments, so `closureRetainFn` and
`closureReleaseFn` work unchanged for shared pointers.

**Source:** `visit(NewExpression&)` at IRGenerator.cpp:3508

### 3.3 Dynamic Arrays (`new T[N]`)

```mingus
int* data = new int[100];
```

Allocates `N * sizeof(T)` bytes. The array size expression is evaluated at runtime,
so `new int[n]` where `n` is a variable works correctly.

```llvm
%total = mul i32 %n, 4                    ; n * sizeof(int)
%arr = call ptr @malloc(i32 %total)
```

**No automatic cleanup.** No bounds tracking. No length field. The programmer must
call `delete data;` to free. The common pattern is to wrap in a class with
destructor (see DynamicArray in test_03).

**Source:** `visit(NewExpression&)` at IRGenerator.cpp:3493

### 3.4 Closure Capture Environments

When a lambda captures variables, the compiler allocates a heap environment struct:

```mingus
int x = 10;
auto f = [=](int y) => { return x + y; };
```

```
envPtr → [ i64 strong | i64 weak | ptr cleanup | i32 x_captured ]
          ├──────── RC header (24 bytes) ────────┤├── captures ──┤
```

```llvm
%env = call ptr @malloc(i32 28)          ; header(24) + captured int(4)
store i64 1, ptr %env                    ; strong = 1
; ... init weak=0, cleanup=cleanup_fn ...
%x.slot = getelementptr i8, ptr %env, i64 24
store i32 %x.val, ptr %x.slot           ; copy captured value
```

The closure is represented as a fat pointer `{ fnPtr, envPtr }`. The environment is
reference-counted: retain on copy, release when the fat pointer goes out of scope.
When strong count hits 0, the cleanup function releases any captured closures, then
the allocation is freed.

**Source:** `visit(LambdaExpression&)` at IRGenerator.cpp:4283

### 3.5 String Concatenation and Interpolation

```mingus
string greeting = "Hello, " + name + "!";
```

Each `+` on strings calls `emitStringConcat()`, which:
1. Calls `strlen()` on both operands
2. Calls `malloc(len1 + len2 + 1)`
3. Copies via `strcpy` + `strcat`
4. Registers result for RAII cleanup via `__mingus_string_free`

```llvm
%len1 = call i64 @strlen(ptr %left)
%len2 = call i64 @strlen(ptr %right)
%total = add i64 %len1, %len2
%total1 = add i64 %total, 1              ; +1 for null terminator
%buf = call ptr @malloc(i64 %total1)
call ptr @strcpy(ptr %buf, ptr %left)
call ptr @strcat(ptr %buf, ptr %right)
; Registered for RAII — freed at scope exit
```

String interpolation (`"Value: ${x}"`) uses `snprintf` to format into a malloc'd buffer,
also registered for RAII.

**Source:** `emitStringConcat()` at IRGenerator.cpp:1356

---

## 4. Global and Static Data

These live in the binary's read-only data sections and are never freed.

### 4.1 Vtables

Every class with virtual methods gets a global constant vtable:

```llvm
@Animal_vtable = internal constant [3 x ptr] [
    ptr @Animal_destructor,    ; slot 0 = always destructor
    ptr @Animal_speak,         ; slot 1+  = virtual methods
    ptr @Animal_getId
]
```

**Slot 0 is always the destructor.** This enables virtual destructor dispatch via
`delete basePtr;` — load vtable, GEP to slot 0, indirect call.

**Source:** `declareVtables()` at IRGenerator.cpp:438

### 4.2 Interface Tables (itables)

For each (class, interface) pair where the class implements the interface:

```llvm
@Dog.Speakable.itable = internal constant [1 x ptr] [
    ptr @Dog_speak
]
```

Used in interface fat pointers `{ objPtr, itablePtr }` for virtual dispatch through
interfaces.

**Source:** `declareItables()` at IRGenerator.cpp:475

### 4.3 String Literals

All string literals become global constants:

```llvm
@str.0 = private unnamed_addr constant [6 x i8] c"Hello\00", align 1
```

These are **never freed** — they point to read-only data. This is why string
variables holding only literals need no cleanup, while concatenation results
(which are malloc'd) do.

### 4.4 Enum String Values

String-backed enum members are stored as global string constants, accessed via
`CreateGlobalStringPtr()`.

---

## 5. RAII Scope Stack

The RAII system provides deterministic destruction for stack-allocated resources.

### 5.1 Data Structures

```cpp
struct RAIIScope {
    std::vector<std::pair<llvm::Value*, llvm::Function*>> destructibles;
    std::set<Symbol*> returnedVars;  // skip cleanup for returned values
};
std::vector<RAIIScope> raiiScopeStack_;
```

Each scope (function body, block, loop body) pushes a new `RAIIScope`. Variables
registered for cleanup are added to the current scope's `destructibles` list.

### 5.2 Registration

```cpp
void IRGenerator::registerRAII(llvm::Value* ptr, llvm::Function* dtor) {
    if (!raiiScopeStack_.empty()) {
        raiiScopeStack_.back().destructibles.push_back({ptr, dtor});
    }
}
```

**Source:** IRGenerator.cpp:890

### 5.3 What Gets Registered

| Type | Cleanup function | Effect |
|------|-----------------|--------|
| Class instance (stack) | Class destructor | Calls user destructor + base chain |
| Struct with closure fields | `__struct_cleanup_Name` | Releases closure-typed fields |
| Closure variable | `__mingus_closure_release_wrapper` | Loads fat ptr, releases env |
| Shared pointer variable | `__mingus_shared_release_wrapper` | Loads rcPtr, calls release |
| String concat result | `__mingus_string_free` | Calls `free()` on buffer |
| Weak capture promotion | `__mingus_closure_release_wrapper` | Releases temp strong ref |

### 5.4 Scope Destruction Paths

There are three distinct destruction paths:

**Normal scope exit** (`emitScopeDestructors`, IRGenerator.cpp:896):
Walk the current scope's destructibles in reverse (LIFO) order, calling each
cleanup function. Skips variables that were returned (to avoid destroying the
return value).

**Early return** (`emitReturnDestructors`, IRGenerator.cpp:913):
Walk ALL scopes from innermost to function root, destroying everything. This
ensures all nested scopes are cleaned up when returning from deep nesting.

**Break/continue** (`emitBreakDestructors`, IRGenerator.cpp:932):
Walk only the scopes created inside the loop body (using `loopStack_.back().raiiScopeDepth`
to know where to stop). Does not destroy scopes outside the loop.

---

## 6. Deallocation Patterns

### 6.1 `delete` Statement

For raw pointers — calls destructor (with virtual dispatch if applicable), then `free`:

```llvm
; delete animal;
%vtable = load ptr, ptr %animal              ; load vtable pointer
%dtor = load ptr, ptr %vtable                ; slot 0 = destructor
call void %dtor(ptr %animal)                 ; virtual destructor call
call void @free(ptr %animal)                 ; free memory
```

For shared pointers — calls `closureReleaseFn` (decrements RC, cleanup+free if zero):

```llvm
; delete sharedAnimal;
call void @closureReleaseFn(ptr %sharedAnimal)   ; RC handles everything
; No explicit free — release does it when strong_count reaches 0
```

**Source:** `visit(DeleteStatement&)` at IRGenerator.cpp:2116

### 6.2 Closure Release

The release function is shared between closures and shared pointers:

```
closureReleaseFn(rcPtr):
    strong_count--
    if (strong_count == 0):
        if (cleanup_fn != null):
            cleanup_fn(rcPtr)          // destructor or captured-closure release
        if (weak_count == 0):
            free(rcPtr)                // actually free the memory
```

**Source:** `getOrCreateClosureReleaseFn()` at IRGenerator.cpp:983

### 6.3 String Free

Simple wrapper around `free()` for string buffers:

```cpp
llvm::Function* IRGenerator::getOrCreateStringFreeFn() {
    // void __mingus_string_free(ptr bufPtr) { free(bufPtr); }
}
```

**Source:** IRGenerator.cpp:1338

---

## 7. Value vs Reference Semantics

### 7.1 Passing Conventions

| Type | Passed as | Copy semantics |
|------|-----------|---------------|
| `int`, `float`, `bool`, `char` | Register value | Full copy |
| `string` | Pointer value | Pointer copy (shares underlying data) |
| Struct | Pointer to temp alloca | Full copy (caller makes temp, callee loads) |
| Class pointer (`Foo*`) | Pointer value | Pointer copy (same object) |
| Shared pointer (`shared Foo*`) | Pointer value | Pointer copy (no retain — borrow) |
| Closure (`(int) => int`) | Fat pointer `{ fn, env }` | Fat pointer copy (no retain — borrow) |
| Interface (`Drawable`) | Fat pointer `{ obj, itable }` | Fat pointer copy |
| Reference param (`int&`) | Pointer to caller's alloca | No copy (alias) |

### 7.2 Borrow Semantics for Function Calls

When passing a shared pointer or closure to a function, no retain/release occurs.
The caller's reference guarantees liveness for the duration of the call. This is
a zero-cost borrow — no RC overhead on function calls.

### 7.3 Assignment Semantics

Assignment to local variables:
- **Shared pointers:** Release old, store new (new already has strong=1 from creation)
- **Closures:** Release old env, store new fat pointer (retain only on field store)
- **Raw pointers:** Simple store (no RC)
- **Structs:** Full value copy via load+store

Assignment to struct/class fields:
- **Closures:** Release old env, store new, **retain new env** (field store requires retain)
- **Shared pointers:** Release old, store new, **retain new** (field store requires retain)

---

## 8. Array Memory

### 8.1 Fixed-Size Stack Arrays

```mingus
int[10] arr;
```

- **Storage:** Stack (`alloca [10 x i32]`)
- **Size:** Must be integer literal (no expressions)
- **Indexing:** 2-index GEP: `getelementptr [10 x i32], ptr %arr, i32 0, i32 %i`
- **Bounds checking:** None
- **Cleanup:** Automatic (stack frame)

### 8.2 Dynamic Heap Arrays

```mingus
int* data = new int[n];    // n can be a runtime variable
```

- **Storage:** Heap (`malloc(n * sizeof(int))`)
- **Size:** Any runtime expression
- **Indexing:** 1-index GEP: `getelementptr i32, ptr %data, i32 %i`
- **Bounds checking:** None
- **Cleanup:** Manual (`delete data;`)
- **Length tracking:** None (programmer's responsibility)

### 8.3 Practical Pattern: DynamicArray Class

The recommended pattern wraps heap arrays in a class for automatic cleanup:

```mingus
class DynamicArray {
    private int* data;
    private int size;
    private int capacity;

    constructor(int cap) {
        this.capacity = cap;
        this.size = 0;
        raw { this.data = (int*)malloc(cap * sizeof(int)); }
    }

    destructor {
        raw { free((byte*)this.data); }
    }

    func operator[](int index) => int {
        raw { return *(this.data + index); }
    }

    func push(int value) => void {
        raw { *(this.data + this.size) = value; }
        this.size = this.size + 1;
    }
}
```

This provides:
- Constructor allocates, destructor frees (RAII)
- `operator[]` for clean indexing syntax
- Size/capacity tracking for bounds awareness

### 8.4 Limitations

| Feature | Status |
|---------|--------|
| Fixed-size stack arrays (`int[16]`) | Supported |
| Dynamic heap arrays (`new T[n]`) | Supported |
| Variable-size stack arrays (`int[n]`) | Not supported (n must be literal) |
| Array parameters/returns | Not supported (use pointers) |
| Array struct/class fields | Not supported (use `T*` + manual alloc) |
| Array literals `[1, 2, 3]` | Not supported |
| Multidimensional arrays | Not supported |
| Bounds checking | Not supported |
| Shared arrays (`new shared T[n]`) | Not supported (by design) |

---

## 9. Summary Tables

### 9.1 Where Things Live

| What | Location | Lifetime | Cleanup |
|------|----------|----------|---------|
| Local primitives | Stack | Scope | None needed |
| Local structs/classes | Stack | Scope | RAII destructor |
| Function parameters | Stack (or caller's stack for `&`) | Function | None (caller owns) |
| Fixed-size arrays | Stack | Scope | None needed |
| Struct temporaries | Stack | Expression | Function exit |
| `new Foo()` | Heap | Until `delete` | Manual |
| `new shared Foo()` | Heap | Until RC reaches 0 | Automatic (RAII + RC) |
| `new T[N]` | Heap | Until `delete` | Manual |
| Closure environments | Heap | Until RC reaches 0 | Automatic (RC) |
| String concat results | Heap | Scope | Automatic (RAII) |
| Vtables | Global constant | Program | None needed |
| Itables | Global constant | Program | None needed |
| String literals | Global constant | Program | None needed |

### 9.2 Cleanup Strategy by Type

| Variable type | Registration | Cleanup function | When |
|--------------|-------------|-----------------|------|
| Class (stack) | `registerRAII(alloca, dtor)` | Class destructor | Scope exit |
| Struct (closure fields) | `registerRAII(alloca, structCleanup)` | `__struct_cleanup_Name` | Scope exit |
| Closure | `registerRAII(alloca, closureReleaseWrapper)` | `__mingus_closure_release_wrapper` | Scope exit |
| Shared pointer | `registerRAII(alloca, sharedReleaseWrapper)` | `__mingus_shared_release_wrapper` | Scope exit |
| String buffer | `registerRAII(buf, stringFree)` | `__mingus_string_free` | Scope exit |
| Raw pointer | Not registered | None | Manual `delete` |
| Dynamic array | Not registered | None | Manual `delete` |

### 9.3 RC Header Layout (Shared by Closures and Shared Pointers)

```
Offset 0:   i64 strong_count    (ownership references)
Offset 8:   i64 weak_count      (non-owning references)
Offset 16:  ptr cleanup_fn      (destructor/release callback, or null)
Offset 24+: [object data or captured variables]
```

- `strong_count` reaching 0 triggers cleanup (destructor call) and possible free
- `weak_count` reaching 0 (when strong is already 0) triggers actual `free()`
- `cleanup_fn` is null for closures with no captured closures to release
- Same layout enables reuse of `retain`/`release` functions across closures and shared ptrs

### 9.4 Fat Pointer Layout (Shared by Closures and Interfaces)

```
{ ptr slot0, ptr slot1 }

Closures:    slot0 = fnPtr     slot1 = envPtr (or null if no captures)
Interfaces:  slot0 = objPtr    slot1 = itablePtr
```

Both use the same LLVM struct type `{ ptr, ptr }`, enabling uniform handling in codegen.
