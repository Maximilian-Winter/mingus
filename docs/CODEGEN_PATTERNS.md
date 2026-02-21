# Mingus Code Generation Patterns

Comprehensive reference for how the Mingus compiler lowers its semantically-annotated AST to LLVM IR. Covers every major codegen subsystem: type mapping, function generation, class/struct layout, fat pointers, closures, RAII, virtual dispatch, interface dispatch, pattern matching, pipe expressions, and operator overloading.

**Source files:**
- `include/mingus/codegen/IRGenerator.h` -- header with all state, caches, and method declarations
- `src/mingus/codegen/IRGenerator.cpp` -- full codegen implementation (~3800 lines)
- `tools/mingus_ir_tool.cpp` -- driver: parse, sema, codegen, optimization, clang invocation

**LLVM target:** LLVM 21.1.8, `x86_64-pc-windows-msvc` triple.

---

## Table of Contents

1. [Overview](#1-overview)
2. [State Management](#2-state-management)
3. [Type Mapping](#3-type-mapping)
4. [Function Codegen](#4-function-codegen)
5. [Class and Struct Codegen](#5-class-and-struct-codegen)
6. [Fat Pointers](#6-fat-pointers)
7. [Closure Codegen](#7-closure-codegen)
8. [RAII System](#8-raii-system)
9. [Member Access and Bare Fields](#9-member-access-and-bare-fields)
10. [Pipe Expression](#10-pipe-expression)
11. [Match Expression](#11-match-expression)
12. [Delete Statement](#12-delete-statement)
13. [Interface Dispatch](#13-interface-dispatch)
14. [Operator Overloading](#14-operator-overloading)
15. [Optimization](#15-optimization)
16. [Entry Point Wrapper](#16-entry-point-wrapper)
17. [Key Invariants](#17-key-invariants)

---

## 1. Overview

### Architecture

`IRGenerator` is an `ASTVisitor` that walks the semantically-annotated AST produced by all four sema passes and emits LLVM IR via the LLVM C++ API (`IRBuilder`, `Module`, `Function`, etc.). It operates in two phases:

**Phase A -- Forward Declarations** (called explicitly before visitation):
1. `declareStructTypes()` -- create opaque LLVM `StructType` for all structs/classes, then set their bodies
2. `declareExternFunctions()` -- create LLVM `Function` declarations for extern functions
3. `declareFunctions()` -- create LLVM `Function` declarations for all non-extern, non-abstract functions (module-level, methods, constructors, destructors, operators)
4. `declareVtables()` -- create vtable global arrays for concrete classes with virtual methods
5. `declareItables()` -- create itable global arrays for classes implementing interfaces

**Phase B -- Body Generation** (visitor pattern):
```
program.accept(*this);  // walks ProgramNode -> ModuleNode -> declarations -> statements -> expressions
```

Each visitor method sets `lastValue_` as the result of expression evaluation. Statements use `lastValue_` from sub-expressions. The `builder_` (`llvm::IRBuilder<>`) tracks the current insertion point.

### V2 Improvements Over V1

- **No scope navigation stack**: Each AST node carries `astScopeNode` directly; no `childIndexStack_` to track.
- **No `scanForParamSymbols`**: `ParameterNode::resolvedSymbol` links parameter symbols directly -- eliminates the bug-prone scanner that had to cover every AST node type.
- **Unified `mapType(TypeSymbol*)`**: Single entry point for type mapping (V1 had a separate `Type*` hierarchy).
- **`mapParamType()`**: Handles struct-by-ptr, ref-by-ptr, interface-by-fat-ptr uniformly.
- **`ArgumentsNode::isReference[]`**: Per-argument ref tracking populated by the type checker for all call types.
- **`CallExpression::resolvedCallee`**: Pre-resolved function symbol for direct dispatch.

---

## 2. State Management

### Core State Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `context_` | `llvm::LLVMContext` | Owns all LLVM types and constants |
| `module_` | `unique_ptr<llvm::Module>` | The single LLVM module (`"mingus_module"`) |
| `builder_` | `llvm::IRBuilder<>` | Current insertion point for IR instructions |
| `currentFunction_` | `llvm::Function*` | The LLVM function currently being generated |
| `currentThisPtr_` | `llvm::Value*` | The `this` pointer inside methods/constructors/destructors |
| `lastValue_` | `llvm::Value*` | Result of the last expression visitor |
| `currentModuleName_` | `std::string` | Current Mingus module name (for name mangling) |
| `currentClassSym_` | `ClassSymbol*` | The class being visited (set during `ClassDeclaration`) |
| `currentStructSym_` | `StructSymbol*` | The struct being visited (set during `StructDeclaration`) |

### Symbol-to-Value Maps

| Cache | Type | Purpose |
|-------|------|---------|
| `namedValues_` | `Symbol* -> Value*` | Maps variable/parameter symbols to their LLVM allocas. Flat map (no nesting). Saved/restored on function entry/exit. |
| `functionCache_` | `Symbol* -> Function*` | Maps function symbols to their LLVM function declarations. Populated during Phase A. |
| `stringConstants_` | `string -> GlobalVariable*` | Deduplicates string literal constants. |
| `structTypeCache_` | `TypeSymbol* -> StructType*` | Maps struct/class symbols to LLVM struct types. Essential for GEP with opaque pointers. |
| `vtableCache_` | `ClassSymbol* -> GlobalVariable*` | Maps classes to their vtable global arrays. |
| `itableCache_` | `(ClassSymbol*, InterfaceSymbol*) -> GlobalVariable*` | Maps class-interface pairs to itable globals. |
| `structCleanupCache_` | `string -> Function*` | Per-struct-type cleanup functions for closure-typed fields. |

### Loop Context

| Variable | Purpose |
|----------|---------|
| `loopExitBlock_` | Target basic block for `break` |
| `loopIterBlock_` | Target basic block for `continue` |
| `loopRAIIScopeDepth_` | RAII stack depth when entering the loop body (for `emitBreakDestructors()`) |

### namedValues_ Lifecycle

`namedValues_` is saved and cleared at each function boundary (including lambdas). Inside a function body, parameters and local variables are added to it. On function exit, the saved map is restored. This ensures no cross-function symbol leakage.

```cpp
auto savedNamedValues = namedValues_;
namedValues_.clear();
// ... populate params, generate body ...
namedValues_ = savedNamedValues;
```

---

## 3. Type Mapping

### mapType(TypeSymbol*) -- Mingus Types to LLVM Types

| Mingus Type | LLVM Type | Notes |
|-------------|-----------|-------|
| `int` | `i32` | |
| `double` | `double` | |
| `float` | `float` | |
| `byte`, `char` | `i8` | |
| `bool` | `i1` | |
| `void` | `void` | |
| `string` | `ptr` | C-style `char*` |
| `Pointer<T>` | `ptr` | Opaque pointer (LLVM 21) |
| `Pointer<Interface>` | `{ ptr, ptr }` | Fat pointer for interface dispatch |
| `Array<T, N>` | `[N x T]` | Fixed-size array |
| `Array<T, 0>` | `ptr` | Dynamic array (heap pointer) |
| `Tuple<A, B, ...>` | `{ A, B, ... }` | Anonymous LLVM struct |
| `enum` | underlying type (default `i32`) | |
| `struct`, `class` | cached `StructType*` | Created in Phase A |
| `FunctionType` | `{ ptr, ptr }` | Fat pointer (fnPtr + envPtr) |
| `ReferenceType` | `ptr` | Only seen if TypeResolver failed to unwrap |
| `Interface` | `{ ptr, ptr }` | Fat pointer (objPtr + itablePtr) |
| `null`, `error` | `ptr` | Fallback |

### mapParamType(TypeSymbol*, bool isReference)

Unified parameter type mapping that handles three special cases:

```
isReference=true    --> ptr    (pass pointer to caller's alloca)
class/struct type   --> ptr    (pass by pointer, not by value)
interface type      --> {ptr, ptr}  (fat pointer)
everything else     --> mapType(type)
```

This function fixes three HIGH-severity bugs from V1 where struct params, ref params, and interface params were not consistently mapped to pointer types.

### getFatPtrType()

Returns the canonical `{ ptr, ptr }` struct type used for both closures and interfaces:

```llvm
%fat_ptr = type { ptr, ptr }
```

---

## 4. Function Codegen

### Free Functions

```cpp
void IRGenerator::visit(FunctionDeclaration& node)
```

1. Look up the pre-declared `llvm::Function` from `functionCache_`
2. Create entry basic block
3. Save/restore `currentFunction_`, `currentThisPtr_`, `namedValues_`
4. Map parameters to allocas using `ParameterNode::resolvedSymbol`:
   - **Reference params** (`isReference=true`): Use the LLVM arg value directly as a pointer (no alloca)
   - **Struct/class params**: Load from the pointer arg into a local alloca
   - **All other params**: Create alloca, store arg value
5. Push RAII scope
6. Visit body statements
7. If no terminator exists, emit scope destructors and return (void or undef)
8. Pop RAII scope, restore state

### Methods (with `this` as first param)

Methods are generated by the same `FunctionDeclaration` visitor. The `FunctionSymbol::hasThisParam` flag causes the first LLVM argument to be treated as `this`:

```cpp
if (funcSym->hasThisParam) {
    currentThisPtr_ = fn->getArg(argIdx++);
}
```

The `this` pointer is set on `currentThisPtr_` so that bare field accesses (without `this.` prefix) can resolve through `emitFieldGEP()`.

### Extern Functions (with varargs)

Extern functions are declared in `declareExternFunctions()` during Phase A. If `FunctionSymbol::isVariadic` is true, the LLVM function type is created with `isVarArg=true`:

```llvm
; extern func printf(string fmt, ...) => int;
declare i32 @printf(ptr, ...)
```

At call sites, variadic arguments undergo C calling convention promotion:
- Small integers (`i1`, `i8`) are sign-extended to `i32`
- `float` values are extended to `double`

### Static Methods

Static methods have `isStatic=true` on the `FunctionSymbol`. When dispatching a call through `MemberAccessExpression`, the codegen checks `isStaticAccess` and does NOT pass a `this` pointer:

```cpp
if (calleeFuncSym->isStatic) thisPtr = nullptr;
```

### Name Mangling

Function names are mangled using `mangleName()`:

| Symbol Type | Mangled Name | Example |
|-------------|-------------|---------|
| Extern function | Raw name (no mangling) | `printf` |
| Module function | `ModuleName_funcName` | `Main_compute` |
| Method | `ClassName_methodName` | `Vector_magnitude` |
| Constructor | `ClassName_constructor` | `Vector_constructor` |
| Destructor | `ClassName_destructor` | `Vector_destructor` |
| Operator | `TypeName_operator_op` | `Complex_operator_add` |

### buildFunctionType()

Builds the LLVM `FunctionType` for a `FunctionSymbol`:

```
ReturnType FunctionSymbol(
    ptr %this,           // if hasThisParam
    <mapped param types> // via mapParamType()
)
```

Constructors and destructors always return `void`.

---

## 5. Class and Struct Codegen

### Struct Type Generation (Phase A)

Two-pass approach in `declareStructTypes()`:

**Pass 1**: Create opaque LLVM `StructType` for every struct/class, ensuring base classes are created before derived classes.

**Pass 2**: Set the struct body with field types:

For structs:
```
%StructName = type { <field0_type>, <field1_type>, ... }
```

For classes (with vtable):
```
%ClassName = type { ptr, <inherited_field_types>, <own_field_types> }
                    ^--- vtable pointer at index 0
```

For classes (without vtable):
```
%ClassName = type { <inherited_field_types>, <own_field_types> }
```

The class uses `allFields` (inherited + own) for its layout, which means inherited fields come first and own fields follow.

Empty struct/class bodies get a single `i8` field to ensure non-zero size.

### Vtable Layout

```
vtable[0] = destructor function pointer     (ALWAYS slot 0 if class has vtable)
vtable[1] = first virtual method
vtable[2] = second virtual method
...
```

**Slot 0 is always the destructor.** All method `vtableIndex` values start at 1+. This is essential for `DeleteStatement` dispatch.

Vtables are global constant arrays of pointer type:

```llvm
@ClassName_vtable = internal constant [N x ptr] [
    ptr @ClassName_destructor,
    ptr @ClassName_method1,
    ptr @ClassName_method2,
    ...
]
```

Abstract classes (`isAbstract=true`) do not get vtable globals.

### Field GEP Indexing

`getFieldGEPIndex(ClassSymbol* cls, VariableSymbol* field)` computes the GEP index for a field:

```
offset = cls->hasVtable() ? 1 : 0    // skip vtable pointer slot
index  = offset + position_in_allFields
```

This correctly handles inheritance: `allFields` contains base class fields followed by own fields, so a field from the base class will have a lower index than a field from the derived class.

For structs, field indexing is simpler: `VariableSymbol::fieldIndex` is used directly (no vtable offset).

### Constructor Generation

```llvm
define void @ClassName_constructor(ptr %this, <params>) {
entry:
    ; 1. Call base constructor (if inheriting with super() call)
    call void @BaseClass_constructor(ptr %this, <superArgs>)

    ; 2. Store vtable pointer (overwrites base class vtable)
    %vt.slot = getelementptr %ClassName, ptr %this, i32 0, i32 0
    store ptr @ClassName_vtable, ptr %vt.slot

    ; 3. Zero-init closure-typed fields
    %field.ptr = getelementptr %ClassName, ptr %this, i32 0, i32 <idx>
    store { ptr, ptr } zeroinitializer, ptr %field.ptr

    ; 4. User constructor body
    ; ...

    ret void
}
```

The constructor always stores the vtable pointer AFTER calling the base constructor, ensuring the derived class's vtable overwrites the base class's.

### Destructor Generation

```llvm
define void @ClassName_destructor(ptr %this) {
entry:
    ; 1. User destructor body
    ; ...

    ; 2. Auto-generated epilogue: release closure-typed fields
    %field.ptr = getelementptr %ClassName, ptr %this, i32 0, i32 <idx>
    %fat = load { ptr, ptr }, ptr %field.ptr
    %env = extractvalue { ptr, ptr } %fat, 1
    call void @__mingus_closure_release(ptr %env)

    ; 3. Chain to base destructor (if base class has destructor)
    call void @BaseClass_destructor(ptr %this)

    ret void
}
```

The epilogue releases all closure-typed fields (calling `__mingus_closure_release` on each env pointer), then chains to the base class destructor. This ordering ensures derived-class cleanup happens before base-class cleanup.

---

## 6. Fat Pointers

Fat pointers are `{ ptr, ptr }` struct values used for two distinct purposes:

### Closures: `{ fnPtr, envPtr }`

```llvm
; field 0: pointer to the lambda's LLVM function
; field 1: pointer to the heap-allocated capture environment (or null)
%closure = type { ptr, ptr }
```

Every lambda produces a fat pointer, even non-capturing ones (which use `null` for envPtr). This uniform representation allows all function-typed values to be called through the same interface.

### Interfaces: `{ objPtr, itablePtr }`

```llvm
; field 0: pointer to the concrete class instance
; field 1: pointer to the class's itable for this interface
%interface_ptr = type { ptr, ptr }
```

Used when a `Pointer<InterfaceType>` value is created from a concrete class pointer.

### Why the Same Layout?

Both use `{ ptr, ptr }` because `mapType()` returns `getFatPtrType()` for both `FunctionTypeSymbol` and `InterfaceSymbol`. The distinction is semantic: the codegen knows which interpretation to use based on the source-level type.

---

## 7. Closure Codegen

### Lambda Function Generation

Each lambda creates a new internal LLVM function with a globally unique name:

```llvm
define internal <retTy> @__lambda_N(<params>, ptr %env) {
    ; ... body ...
}
```

ALL lambdas receive `ptr %env` as their **last parameter**, even non-capturing ones (value will be `null`). This uniform calling convention allows all function-typed values to be called through the same fat pointer interface.

### Capture Environment Struct Layout

```
{ i64 refcount, ptr cleanup_fn, <capture_0>, <capture_1>, ... }
  ^--- header offset = 2       ^--- captures start at index 2
```

| Field | Type | Purpose |
|-------|------|---------|
| `refcount` | `i64` | Reference count, initialized to 1 |
| `cleanup_fn` | `ptr` | Per-closure cleanup function (releases inner closures), or `null` |
| captures... | varies | By-value: actual value. By-reference: `ptr` to outer alloca |

### By-Value Capture (`[=]` or `[x]`)

The captured variable's current value is loaded and copied into the env struct:

```llvm
; At capture site (outer function):
%val = load double, ptr %scale.alloca
%cap.slot = getelementptr %closure_ty, ptr %env.ptr, i32 0, i32 2
store double %val, ptr %cap.slot

; Inside lambda (loading from env):
%cap.ptr = getelementptr %closure_ty, ptr %env, i32 0, i32 2
%cap.val = load double, ptr %cap.ptr
%cap.alloca = alloca double
store double %cap.val, ptr %cap.alloca
; subsequent reads/writes use %cap.alloca
```

### By-Reference Capture (`[&]` or `[&x]`)

The env struct stores a **pointer to the outer variable's alloca**, not the value:

```llvm
; At capture site:
%cap.slot = getelementptr %closure_ty, ptr %env.ptr, i32 0, i32 2
store ptr %counter.alloca, ptr %cap.slot    ; store the ALLOCA POINTER itself

; Inside lambda:
%ref.ptr = getelementptr %closure_ty, ptr %env, i32 0, i32 2
%ref = load ptr, ptr %ref.ptr              ; get pointer to outer alloca
; reads/writes go directly through %ref
%val = load i32, ptr %ref
%inc = add i32 %val, 1
store i32 %inc, ptr %ref                   ; persists to outer scope!
```

By-reference captures skip RC cleanup since they don't own the referenced value.

### Reference Counting

**Retain** (`__mingus_closure_retain`):
```
if (envPtr != null) { envPtr->refcount += 1; }
```

**Release** (`__mingus_closure_release`):
```
if (envPtr != null) {
    envPtr->refcount -= 1;
    if (envPtr->refcount == 0) {
        if (envPtr->cleanup_fn != null) {
            envPtr->cleanup_fn(envPtr);    // release nested closures
        }
        free(envPtr);
    }
}
```

Both functions are generated as internal LLVM functions on first use and cached in `closureRetainFn_` / `closureReleaseFn_`.

### Closure Cleanup Functions

When a closure captures other closures by value, a per-closure cleanup function is generated:

```llvm
define internal void @__closure_cleanup_N(ptr %env) {
    ; For each by-value captured variable that is FunctionType:
    %slot = getelementptr %closure_ty, ptr %env, i32 0, i32 <idx>
    %fat = load { ptr, ptr }, ptr %slot
    %inner_env = extractvalue { ptr, ptr } %fat, 1
    call void @__mingus_closure_release(ptr %inner_env)
    ; ...
    ret void
}
```

The cleanup function pointer is stored in `closure_ty.cleanup_fn` (field 1).

### RAII Wrapper for Closure Variables

When a local variable is of function type, it is registered for RAII cleanup via `__mingus_closure_release_wrapper`. This wrapper takes a pointer to the alloca (not the env directly), loads the fat pointer, extracts the env, and calls release:

```llvm
define internal void @__mingus_closure_release_wrapper(ptr %alloca) {
    %fat = load { ptr, ptr }, ptr %alloca
    %env = extractvalue { ptr, ptr } %fat, 1
    call void @__mingus_closure_release(ptr %env)
    ret void
}
```

### Struct Cleanup Functions

For structs with closure-typed fields, a per-struct cleanup function is generated:

```llvm
define internal void @__struct_cleanup_StructName(ptr %struct_ptr) {
    ; For each FunctionType field:
    %field = getelementptr %StructName, ptr %struct_ptr, i32 0, i32 <fieldIndex>
    %fat = load { ptr, ptr }, ptr %field
    %env = extractvalue { ptr, ptr } %fat, 1
    call void @__mingus_closure_release(ptr %env)
    ; ...
    ret void
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

The `env` is always passed as the **last** argument to the function pointer.

### State Saved/Restored During Lambda Codegen

| Variable | Action |
|----------|--------|
| `currentFunction_` | Saved, set to lambda function |
| `currentThisPtr_` | Saved, set to `nullptr` |
| `namedValues_` | Saved, cleared (captures are added under lambda's symbol keys) |
| `raiiScopeStack_` | **Moved out and cleared** (lambda RAII isolation -- CRITICAL) |
| `builder_` insert point | Saved (block + iterator), restored after lambda body |

After the lambda body is generated, all state is restored and the fat pointer is constructed in the **caller's** context.

### Self-Capturing Closures

When a variable declaration's initializer is a lambda that captures the variable itself (`selfCapture=true`), the codegen patches the env struct after the initial store:

```cpp
// After storing the fat pointer to the variable's alloca:
auto* envPtr = builder_.CreateExtractValue(lastValue_, {1}, "self.env");
auto* selfSlot = builder_.CreateStructGEP(closureTy, envPtr,
    headerOffset + selfCaptureIdx, "self.capture.slot");
builder_.CreateStore(lastValue_, selfSlot);  // store fat ptr into its own env
```

---

## 8. RAII System

### Scope Stack

```cpp
struct RAIIScope {
    std::vector<std::pair<llvm::Value*, llvm::Function*>> destructibles;
    std::set<Symbol*> returnedVars;  // vars to skip cleanup (being returned)
};
std::vector<RAIIScope> raiiScopeStack_;
```

Each `destructibles` entry is a `(pointer, destructor_function)` pair.

### Lifecycle

| Operation | When |
|-----------|------|
| `pushRAIIScope()` | Block entry, function entry |
| `registerRAII(ptr, dtor)` | Variable declarations (class instances, structs with closures, closure variables, string concat results) |
| `emitScopeDestructors()` | Block exit (before `popRAIIScope`) |
| `emitReturnDestructors()` | Before `ret` instruction |
| `emitBreakDestructors()` | Before `break`/`continue` branches |
| `popRAIIScope()` | After scope destructors emitted |

### emitScopeDestructors()

Destroys only the **current (top) scope** in LIFO order:

```cpp
auto& scope = raiiScopeStack_.back();
for (auto it = scope.destructibles.rbegin(); it != scope.destructibles.rend(); ++it) {
    // Skip if this variable is being returned (suppress cleanup)
    if (!skip) builder_.CreateCall(it->second, {it->first});
}
```

### emitReturnDestructors()

Destroys **ALL scopes** from top to bottom in LIFO order:

```cpp
for (auto scopeIt = raiiScopeStack_.rbegin(); scopeIt != raiiScopeStack_.rend(); ++scopeIt) {
    for (auto it = scopeIt->destructibles.rbegin(); it != scopeIt->destructibles.rend(); ++it) {
        if (!skip) builder_.CreateCall(it->second, {it->first});
    }
}
```

### emitBreakDestructors()

Destroys only scopes **created inside the loop body**, using `loopRAIIScopeDepth_` as the boundary:

```cpp
for (size_t i = raiiScopeStack_.size(); i > loopRAIIScopeDepth_; --i) {
    auto& scope = raiiScopeStack_[i - 1];
    for (auto it = scope.destructibles.rbegin(); it != scope.destructibles.rend(); ++it) {
        builder_.CreateCall(it->second, {it->first});
    }
}
```

This prevents `break`/`continue` from destroying outer scopes that should survive the loop.

### Return Value Suppression

When a variable is returned from a function, its destructor should NOT be called (the caller now owns it). The `ReturnStatement` visitor marks the returned variable:

```cpp
if (auto* ident = node.value->as<IdentifierExpression>()) {
    scope.returnedVars.insert(ident->resolvedSymbol.get());
}
```

The destructor emission loops check `returnedVars` and skip matching entries.

### What Gets Registered

| Variable Type | RAII Destructor |
|---------------|----------------|
| Class instance (stack) with destructor | `ClassName_destructor` |
| Struct with closure-typed fields | `__struct_cleanup_StructName` |
| Closure variable (FunctionType) | `__mingus_closure_release_wrapper` |
| Heap-allocated string (concat/interpolation) | `__mingus_string_free` |
| Temporary closure argument | `__mingus_closure_release_wrapper` |

### BlockStatement Guard

`BlockStatementNode` must check for a terminator before emitting scope destructors:

```cpp
if (!builder_.GetInsertBlock()->getTerminator()) {
    emitScopeDestructors();
}
```

Without this guard, a block ending with `return` (which already calls `emitReturnDestructors()`) would emit duplicate destructor calls.

---

## 9. Member Access and Bare Fields

### MemberAccessExpression

`visit(MemberAccessExpression&)` handles three cases:

1. **Enum member access** (`Color.Red`): Emit constant int or global string pointer
2. **Static access** (`ClassName.staticMethod`): No runtime value (resolved at call site)
3. **Field/method access** (`obj.field`): Emit via `emitLValue()` then load

For closure-typed fields, the fat pointer is loaded as `{ ptr, ptr }`:
```cpp
if (fieldSym->getType()->is<FunctionTypeSymbol>()) {
    lastValue_ = builder_.CreateLoad(getFatPtrType(), fieldPtr, name);
    return;
}
```

For method references (used by `CallExpression`), the GEP pointer is passed through without loading.

### emitFieldGEP()

Handles bare field names (without `this.` prefix) inside methods:

```cpp
llvm::Value* emitFieldGEP(VariableSymbol* fieldSym) {
    if (!fieldSym || !currentThisPtr_) return nullptr;
    if (fieldSym->role != VariableRole::Field) return nullptr;

    if (currentClassSym_) {
        // Uses getFieldGEPIndex (vtable + inheritance aware)
        int gepIdx = getFieldGEPIndex(currentClassSym_, fieldSym);
        return builder_.CreateStructGEP(structTy, currentThisPtr_, gepIdx, ...);
    }

    if (currentStructSym_) {
        // Uses direct fieldIndex (no vtable)
        return builder_.CreateStructGEP(structTy, currentThisPtr_, fieldSym->fieldIndex, ...);
    }
    return nullptr;
}
```

### Field Fallback in IdentifierExpression

When `visit(IdentifierExpression&)` cannot find a symbol in `namedValues_`, it falls back to `emitFieldGEP()`:

```cpp
// Variable symbol -> load from alloca (locals and params)
auto it = namedValues_.find(node.resolvedSymbol.get());
if (it != namedValues_.end()) { ... }

// Field fallback: bare field name resolved by sema -> access via this pointer
if (auto* varSym = node.resolvedSymbol->as<VariableSymbol>()) {
    llvm::Value* fieldPtr = emitFieldGEP(varSym);
    if (fieldPtr) { lastValue_ = builder_.CreateLoad(...); return; }
}
```

The same fallback exists in `emitLValue()`, enabling bare field names as assignment targets.

### emitLValue()

Returns a pointer (not a loaded value) for use in assignments and ref-param passing. Handles:

| Expression Type | LValue Strategy |
|----------------|-----------------|
| `IdentifierExpression` | Look up in `namedValues_`, fallback to `emitFieldGEP()` |
| `MemberAccessExpression` | Emit object pointer, then `CreateStructGEP` to field |
| `UnaryExpression(Dereference)` | Emit operand (the pointer value) |
| `IndexExpression` | Emit array/pointer GEP to element |

For arrow access (`obj->field`), the object is evaluated as an rvalue (loading the pointer). For dot access (`obj.field`), the object's lvalue is used directly.

---

## 10. Pipe Expression

`x |> f |> g(a, b)` lowers to sequential function calls where each result feeds as the **first argument** to the next stage.

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
; Stage 1: multiply(3.0, 2.0)  -- piped value is first arg
%pipe.0 = call double @multiply(double 3.0, double 2.0)
; Stage 2: clamp(pipe.0, 0.0, 10.0)  -- piped value is first arg
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

### Method Pipe

If the stage is a member access (`x |> obj.method(args)`), the piped value is passed as the first real argument after `this`:

```llvm
; x |> processor->transform(scale)
; Emits: processor->transform(x, scale)
; args = [this=processor, piped=x, extra=scale]
```

Virtual dispatch is supported: if the method has a vtable index, the call goes through the vtable.

---

## 11. Match Expression

### Conditional Branch Chain

`visit(MatchExpression&)` generates a chain of conditional branches, one per arm:

```
[test arm 0] --false--> [test arm 1] --false--> [test arm 2] --false--> [merge]
      |                       |                       |
   true                    true                    true
      |                       |                       |
  [arm 0 body]          [arm 1 body]          [arm 2 body]
      |                       |                       |
      +----------- all branch to [merge] -----------+
```

### Pattern Types

**Literal pattern**: Compare subject to constant value.
```llvm
; Integer:
%cmp = icmp eq i32 %subject, 42
br i1 %cmp, label %arm.body, label %arm.test.next

; Float:
%cmp = fcmp oeq double %subject, 3.14
br i1 %cmp, label %arm.body, label %arm.test.next
```

**Wildcard pattern** (`_`): Unconditional branch to arm body.
```llvm
br label %arm.body
```

**Identifier pattern** (`var x`): Bind subject to a new variable, optionally guarded.
```llvm
%x = alloca i32
store i32 %subject, ptr %x
; If guarded (var x if x > 0):
%guard = icmp sgt i32 %subject, 0
br i1 %guard, label %arm.body, label %arm.test.next
; If unguarded:
br label %arm.body      ; unconditional
```

**Range pattern** (`1..10`): Check lower and upper bounds.
```llvm
%ge = icmp sge i32 %subject, 1
%le = icmp sle i32 %subject, 10
%in_range = and i1 %ge, %le
br i1 %in_range, label %arm.body, label %arm.test.next
```

### Result PHI Node

If the match expression produces a value (non-void result type), a PHI node merges all arm results:

```llvm
match.merge:
    %result = phi double [ %val0, %arm0.end ],
                         [ %val1, %arm1.end ],
                         [ %val2, %arm2.end ]
```

Type mismatches in PHI incoming values fall back to `undef`.

---

## 12. Delete Statement

`delete ptr` performs:

1. **Interface pointer**: Extract `objPtr` from fat pointer, free it (no destructor dispatch -- interface doesn't know concrete type's destructor).

2. **Class pointer with virtual destructor**: Dispatch through vtable slot 0.

```llvm
; Virtual destructor dispatch through vtable[0]:
%vtable.ptr = getelementptr %ClassName, ptr %obj, i32 0, i32 0
%vtable = load ptr, ptr %vtable.ptr
%dtor.slot = getelementptr ptr, ptr %vtable, i32 0       ; slot 0 = destructor
%dtor.fn = load ptr, ptr %dtor.slot
call void (ptr) %dtor.fn(ptr %obj)
```

3. **Class pointer with non-virtual destructor**: Direct call.

```llvm
call void @ClassName_destructor(ptr %obj)
```

4. **All cases**: Call `free(ptr)` after destructor.

```llvm
call void @free(ptr %obj)
```

---

## 13. Interface Dispatch

### Itable Construction (Phase A)

For each concrete class implementing an interface, an itable is created:

```llvm
@ClassName.InterfaceName.itable = internal constant [N x ptr] [
    ptr @ClassName_method1,   ; slot matches interface method order
    ptr @ClassName_method2,
    ...
]
```

Method slot ordering matches the interface's `methods` vector. Each slot points to the class's implementation of that interface method.

### Fat Pointer Creation

`emitWrapToInterfacePtr()` wraps a class pointer into an interface fat pointer:

```llvm
%fat.obj = insertvalue { ptr, ptr } undef, ptr %objPtr, 0
%fat.itable = insertvalue { ptr, ptr } %fat.obj, ptr @ClassName.InterfaceName.itable, 1
```

This wrapping happens:
- At variable declarations: `Drawable* d = new Circle(...);`
- At call sites: when an argument type is `Pointer<Interface>` but the actual is `Pointer<Class>`
- At interface dispatch call arguments (recursive wrapping for nested interface params)

### Virtual Call Through Itable

```llvm
; iface->method(args)
%fat = load { ptr, ptr }, ptr %iface_var

; Extract object pointer and itable
%obj = extractvalue { ptr, ptr } %fat, 0
%itable = extractvalue { ptr, ptr } %fat, 1

; Load method from itable slot
%fn.slot = getelementptr ptr, ptr %itable, i32 <methodIndex>
%fn = load ptr, ptr %fn.slot

; Call with object pointer as first arg (this)
%result = call <retTy> (<paramTys>) %fn(ptr %obj, <args>)
```

The itable dispatch uses `FunctionSymbol::vtableIndex` (which for interface methods indicates the slot position within that interface's method list).

---

## 14. Operator Overloading

### Binary Operators

When `BinaryExpression::isOperatorOverload` is true and `resolvedOperatorFunction` is set:

```cpp
// Get LHS as pointer (this for the operator method)
llvm::Value* lhsPtr = emitLValue(*node.left);
// Get RHS as value or pointer (for struct types)
node.right->accept(*this);
llvm::Value* rhsArg = lastValue_;

// Call: TypeName_operator_op(this, rhs)
lastValue_ = builder_.CreateCall(operatorFn, {lhsPtr, rhsArg});
```

If the LHS expression yields a struct value (not a pointer), a temporary alloca is created to hold it, since operator methods require `this` as a pointer.

### Index Operator

`IndexExpression::isOperatorOverload` triggers similar dispatch:

```llvm
; arr[i] with operator[] overload
%result = call <retTy> @TypeName_operator_index(ptr %arr.ptr, i32 %idx)
```

### Unary Operator (Negate)

The unary negate operator is treated like a method with no arguments beyond `this`.

### Operator Name Mangling

| Op | Mangled Suffix |
|----|---------------|
| `+` | `add` |
| `-` | `sub` |
| `*` | `mul` |
| `/` | `div` |
| `%` | `mod` |
| `[]` | `index` |
| `==` | `eq` |
| `!=` | `ne` |
| `<` | `lt` |
| `>` | `gt` |
| `<=` | `le` |
| `>=` | `ge` |
| unary `-` | `neg` |

Full mangled name: `TypeName_operator_<suffix>` (e.g., `Complex_operator_add`).

---

## 15. Optimization

Optimization runs in `mingus_ir_tool.cpp` using LLVM's new pass manager:

```cpp
static void optimizeModule(llvm::Module& module, int optLevel) {
    if (optLevel <= 0) return;

    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;

    llvm::PassBuilder PB;
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    // --opt 1 -> O1, --opt 2 -> O2
    auto level = (optLevel == 1) ? llvm::OptimizationLevel::O1
                                 : llvm::OptimizationLevel::O2;
    llvm::ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(level);
    MPM.run(module, MAM);
}
```

All test suite runs use `--opt 2`, which applies the full LLVM O2 pipeline (inlining, SROA, GVN, loop optimizations, etc.). The optimization runs **after** the main wrapper injection but **before** LLVM verification.

---

## 16. Entry Point Wrapper

The `--entry` (or `--run`) flag injects a C-compatible `main()` that calls the specified Mingus function:

```llvm
; For integer-returning entry functions:
define i32 @main() {
    %ret = call i32 @ModuleName_entryFunction()
    ret i32 %ret
}

; For void-returning entry functions:
define i32 @main() {
    call void @ModuleName_entryFunction()
    ret i32 0
}
```

This bridges Mingus's module naming convention to the C runtime's expected `main` symbol.

---

## 17. Key Invariants

### Lambda RAII Isolation

**Lambda visitor MUST save/restore `raiiScopeStack_`**: Lambdas create new LLVM functions but share the same `IRGenerator` instance. Without save/restore, `emitReturnDestructors()` inside a lambda would walk the parent function's RAII scope, causing cross-function IR references and an LLVM verification crash.

```cpp
auto savedRAIIStack = std::move(raiiScopeStack_);
raiiScopeStack_.clear();
// ... generate lambda body ...
raiiScopeStack_ = std::move(savedRAIIStack);
```

### FunctionDeclaration Pushes RAII Scope

Without the `pushRAIIScope()` at function entry, variables declared directly in the function body (not inside nested blocks) would never get RAII cleanup.

### Universal Struct Zero-Init

ALL struct construction uses `zeroinitializer` (not `undef`). This prevents undefined value propagation when struct fields are read before assignment (e.g., accumulator patterns like `mix = mix + osc`).

```cpp
if (varSym->getType()->is<StructSymbol>() || varSym->getType()->is<ClassSymbol>()) {
    builder_.CreateStore(llvm::Constant::getNullValue(varTy), alloca);
}
```

### Class Constructors Zero-Init Closure Fields

After `storeVtablePtr()` and before the user constructor body, all closure-typed fields are zero-initialized:

```cpp
builder_.CreateStore(llvm::ConstantAggregateZero::get(fatPtrTy), fieldPtr);
```

This prevents reading uninitialized fat pointers before user code assigns them.

### Retain-on-Field-Store

When assigning a closure to a struct/class field (detected via `MemberAccessExpression` target or `VariableRole::Field`), the new env pointer must be **retained**:

```cpp
if (isFieldStore) {
    auto* newEnv = builder_.CreateExtractValue(lastValue_, {1}, "new.env");
    builder_.CreateCall(getOrCreateClosureRetainFn(), {newEnv});
}
```

Local variable reassignment does NOT need this -- RAII handles local cleanup. The distinction: field stores create a new ownership reference (the struct/class now co-owns the closure), while local reassignment is just updating the single owning variable.

Before any closure field store, the **old** env pointer is released:

```cpp
auto* oldFat = builder_.CreateLoad(getFatPtrType(), targetPtr, "old.fat");
auto* oldEnv = builder_.CreateExtractValue(oldFat, {1}, "old.env");
builder_.CreateCall(getOrCreateClosureReleaseFn(), {oldEnv});
```

### Vtable Slot 0 = Destructor

`declareVtables()` puts the destructor at index 0. All method `vtableIndex` values start at 1+. `DeleteStatement` dispatches through `vtable[0]` without needing to know the destructor's index.

### BlockStatement Must Guard emitScopeDestructors()

Check for terminator before emitting scope destructors:

```cpp
if (!builder_.GetInsertBlock()->getTerminator()) {
    emitScopeDestructors();
}
```

A return statement already calls `emitReturnDestructors()`. Without the guard, destructors would be emitted twice after a return.

### Nested Capture Propagation

`SemanticValidator::checkLambdaCapture()` walks the **entire** lambda stack from inner to outer, adding the variable to each lambda's captures. Only checking the innermost lambda (`lambdaStack_.back()`) would cause outer lambdas to have null env pointers, leading to segfaults.

### Null-to-Zero Fat Pointer Conversion

When assigning `null` to a `FunctionType` variable, the `ConstantPointerNull` is converted to a `ConstantAggregateZero` of the fat pointer type. This ensures the stored value has the correct `{ ptr, ptr }` type rather than a bare `ptr`:

```cpp
if (isFunctionKind(targetType) && llvm::isa<llvm::ConstantPointerNull>(lastValue_)) {
    lastValue_ = llvm::ConstantAggregateZero::get(getFatPtrType());
}
```

### Fat Pointer Null Comparison

Comparing a closure to `null` extracts the `fnPtr` field (index 0) and compares against null pointer:

```llvm
%fnptr = extractvalue { ptr, ptr } %closure, 0
%is_null = icmp eq ptr %fnptr, null
```

### String Concat RAII Registration

Every `emitStringConcat()` result (a `malloc`'d buffer) is immediately registered for RAII cleanup:

```cpp
registerRAII(buf, getOrCreateStringFreeFn());
```

The same applies to interpolated string results.

### Temporary Closure Arguments

Closures passed directly as function arguments (lambda literals at call site) are RAII-wrapped in temporary allocas to ensure cleanup:

```cpp
if (arg->resolvedType->is<FunctionTypeSymbol>() && arg->is<LambdaExpression>()) {
    auto* tmpAlloca = createEntryBlockAlloca(..., "tmp.closure.arg");
    builder_.CreateStore(lastValue_, tmpAlloca);
    registerRAII(tmpAlloca, getOrCreateClosureReleaseWrapper());
}
```

Without this, temporary closures would leak one refcount.
