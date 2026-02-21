# Mingus Type System and Dispatch

This document describes how types are represented in the Mingus compiler, how they are resolved and checked, how they map to LLVM IR, and how method dispatch (virtual, interface, operator) is implemented.

**Source files:**
- `include/mingus/TypeSymbol.h` -- TypeSymbol hierarchy (types ARE symbols)
- `include/mingus/Symbol.h` -- Symbol and SymbolWithScope abstractions
- `include/mingus/Symbols.h` -- Concrete symbols (VariableSymbol, ClassSymbol, etc.)
- `include/mingus/SymbolTable.h` -- Type interning, `isCompatible()`, scope navigation
- `include/mingus/Forward.h` -- Enums (PrimitiveKind, OverloadableOp, AccessModifier, etc.)
- `include/mingus/AstNode.h` -- AST base nodes, TypeNode hierarchy
- `src/mingus/sema/TypeResolver.cpp` -- Pass 2: resolve type annotations to TypeSymbol
- `src/mingus/sema/TypeChecker.cpp` -- Pass 3: expression type inference and dispatch
- `src/mingus/SymbolTable.cpp` -- Type interning and compatibility implementation
- `src/mingus/codegen/IRGenerator.cpp` -- `mapType()`, vtable/itable codegen

---

## Table of Contents

1. [Type System Overview](#1-type-system-overview)
2. [Primitive Types](#2-primitive-types)
3. [User-Defined Types](#3-user-defined-types)
4. [Function Types](#4-function-types)
5. [Composite Types](#5-composite-types)
6. [Type Compatibility](#6-type-compatibility)
7. [Type Interning](#7-type-interning)
8. [LLVM Type Mapping](#8-llvm-type-mapping)
9. [Method Dispatch](#9-method-dispatch)
10. [Operator Overloading](#10-operator-overloading)
11. [Access Modifiers](#11-access-modifiers)
12. [Const System](#12-const-system)
13. [Typedef / Type Aliases](#13-typedef--type-aliases)
14. [Function Overloading](#14-function-overloading)
15. [Covariant Return Types](#15-covariant-return-types)
16. [Copy and Move Constructor Type Matching](#16-copy-and-move-constructor-type-matching)

---

## 1. Type System Overview

### Design Principle: Types ARE Symbols

The central design decision in Mingus is that there is **no separate Type hierarchy**. Every type is a `TypeSymbol`, which itself extends `SymbolWithScope`. This means every type is simultaneously:

- A **Symbol**: has a name, lives in a scope, carries access modifiers
- A **Scope**: can contain members (fields, methods, operators)

For primitive types, the scope is empty (no members). For class and struct types, the scope holds fields, methods, constructors, and operators. Name resolution through a type -- `classSym->resolve("fieldName")` -- is always valid syntax; it returns `nullptr` for types without members.

### Inheritance Hierarchy

```
Scope (abstract)
  +-- BaseScope (symbol map, enclosing chain)
      +-- GlobalScope
      +-- BlockScope

Symbol (abstract)
  +-- BaseSymbol (leaf symbols, no scope)
  |   +-- TypedSymbol (getType()/setType() -> TypeSymbol)
  |       +-- VariableSymbol
  +-- SymbolWithScope (MI: BaseScope + Symbol)
      +-- TypeSymbol (base for ALL types)
      |   +-- PrimitiveTypeSymbol
      |   +-- PointerTypeSymbol
      |   +-- ArrayTypeSymbol
      |   +-- TupleTypeSymbol
      |   +-- FunctionTypeSymbol
      |   +-- ReferenceTypeSymbol
      |   +-- ErrorTypeSymbol
      |   +-- NullTypeSymbol
      |   +-- ClassSymbol
      |   +-- StructSymbol
      |   +-- EnumSymbol
      |   +-- InterfaceSymbol
      +-- FunctionSymbol
      |   +-- MethodSymbol
      |   +-- ConstructorSymbol
      |   +-- DestructorSymbol
      |   +-- OperatorSymbol
      +-- ModuleSymbol
```

The key multiple-inheritance pattern is `SymbolWithScope`, which inherits from both `BaseScope` (it IS a scope) and `Symbol` (it IS a named entity). A `ClassSymbol` IS its own member scope. A `FunctionSymbol` IS the scope for its body. A `ModuleSymbol` IS the scope for its top-level declarations.

### Resolution Pipeline

Types flow through four compiler passes:

1. **Pass 1 (SymbolTableBuilder)**: Creates all `TypeSymbol` instances (classes, structs, enums, interfaces) and registers them in the `SymbolTable` type registry. Creates `VariableSymbol`, `FunctionSymbol`, etc. Links `ParameterNode::resolvedSymbol` to its `VariableSymbol`.

2. **Pass 2 (TypeResolver)**: Resolves `TypeNode` annotations in the AST to `TypeSymbol` instances. Sets `VariableSymbol::type`, `FunctionSymbol::returnType`, and parameter types. Critically, unwraps `ReferenceType` on parameters (stores base type + `isReference=true` flag).

3. **Pass 3 (TypeChecker)**: Bottom-up expression type inference. Every `ExpressionBaseNode` gets a `resolvedType`. Resolves identifiers via scope chain. Checks type compatibility at assignments, returns, and call sites. Sets `CallExpression::resolvedCallee` and `ArgumentsNode::isReference` per-argument flags.

4. **Pass 4 (SemanticValidator)**: Lambda capture analysis, RAII tracking, return completeness, access modifier enforcement. Does not modify types but validates their correct usage.

---

## 2. Primitive Types

Mingus has eight primitive types, all represented as `PrimitiveTypeSymbol` instances with a `PrimitiveKind` enum:

| Mingus Type | `PrimitiveKind` | Size (bytes) | LLVM IR Type | Description |
|-------------|-----------------|-------------|-------------|-------------|
| `int`       | `Int`           | 4           | `i32`        | Signed 32-bit integer |
| `double`    | `Double`        | 8           | `double`     | 64-bit IEEE 754 floating-point |
| `float`     | `Float`         | 4           | `float`      | 32-bit IEEE 754 floating-point |
| `byte`      | `Byte`          | 1           | `i8`         | Unsigned 8-bit integer |
| `char`      | `Char`          | 1           | `i8`         | Character (same as byte in IR) |
| `string`    | `String`        | 8           | `ptr`        | Null-terminated C string (pointer to `i8`) |
| `bool`      | `Bool`          | 1           | `i1`         | Boolean (single bit) |
| `void`      | `Void`          | 0           | `void`       | No value |

All eight primitives are pre-registered in the `SymbolTable` constructor (via `registerPrimitives()`). They are canonical singletons: `symbolTable.getIntType()` always returns the same `shared_ptr`.

### Convenience Queries

`PrimitiveTypeSymbol` provides:
- `isNumeric()` -- true for int, double, float, byte, char
- `isIntegral()` -- true for int, byte, char
- `isFloating()` -- true for double, float

### Sentinel Types

Two special types exist for compiler-internal purposes:

- **`ErrorTypeSymbol`**: Returned when type resolution fails. `isCompatible()` returns `true` whenever either operand is `ErrorType`, preventing cascading type errors after the first failure.

- **`NullTypeSymbol`**: The type of the `null` literal. Compatible with `PointerTypeSymbol` and `FunctionTypeSymbol` (the fat pointer is zeroed). Not compatible with primitives. Attempting `var x = null` is a compile error ("cannot infer type from null").

---

## 3. User-Defined Types

### ClassSymbol

Classes are the most complex type. A `ClassSymbol` extends `TypeSymbol` and provides:

```
ClassSymbol
  +-- baseClassNames: vector<string>         (unresolved names from parse)
  +-- resolvedBaseClass: ClassSymbol*         (resolved in Pass 1)
  +-- isAbstract: bool
  +-- fields: vector<VariableSymbol>          (own fields only)
  +-- allFields: vector<VariableSymbol>       (inherited + own, in LLVM GEP order)
  +-- constructor: ConstructorSymbol
  +-- destructor: DestructorSymbol
  +-- vtable: vector<FunctionSymbol>          (slot 0 = destructor)
  +-- vtableSize: int
  +-- implementedInterfaces: vector<InterfaceSymbol>
```

Key behaviors:
- **`resolve()` walks the inheritance chain**: If a name is not found in the class scope, resolution continues in the base class, then its base class, etc. This is an override of `BaseScope::resolve()`.
- **`hasRAII()`**: True if the class has a destructor (every class does, because Pass 1 auto-generates one if missing).
- **`hasVtable()`**: True if `vtableSize > 0` (any virtual/override methods, or inherits from a class with a vtable).

### StructSymbol

Structs are simpler: no inheritance, no vtable, no constructor/destructor.

```
StructSymbol
  +-- fields: vector<VariableSymbol>
  +-- needsCleanup(): bool     (true if any field is closure-typed)
```

Structs can have methods and operator overloads (defined in their scope), but no virtual dispatch. Cleanup functions (`__struct_cleanup_<Name>`) may be synthesized by codegen if any field is closure-typed.

### EnumSymbol

```
EnumSymbol
  +-- members: vector<MemberInfo>
  +-- underlyingType: TypeSymbolPtr    (int, byte, or string)
  +-- findMember(name): MemberInfo*
```

Each `MemberInfo` carries:
- `name`: the member name (e.g., "Red")
- `intValue`: integer value for int-backed enums
- `stringValue`: string value for string-backed enums
- `hasExplicitValue`: whether the value was explicitly assigned

Enums default to `int` underlying type. String-backed enums store global constant strings in codegen. Enum values are fully interchangeable with their underlying type via `isCompatible()`.

### InterfaceSymbol

```
InterfaceSymbol
  +-- methods: vector<FunctionSymbol>    (abstract method signatures)
  +-- findMethod(name): FunctionSymbol
```

Interfaces declare abstract method signatures with no bodies. Classes that implement an interface must provide implementations for all methods. At runtime, interface values are fat pointers `{ objPtr, itablePtr }`. See [Interface Dispatch](#interface-dispatch-itables) for details.

---

## 4. Function Types

### FunctionTypeSymbol vs. FunctionSymbol

These two are distinct concepts:

- **`FunctionSymbol`**: A named function declaration. IS a scope for its body. Contains parameter `VariableSymbol` instances, a return type, and flags (isMethod, isStatic, isVirtual, etc.).

- **`FunctionTypeSymbol`**: The TYPE of a function or closure value. Carries parameter info and return type but no body. This is what a `VariableSymbol` holds when it stores a closure.

A `FunctionSymbol` can produce its corresponding `FunctionTypeSymbol` via `buildFunctionType()`.

### ParameterInfo

`FunctionTypeSymbol` carries full per-parameter metadata:

```cpp
struct ParameterInfo {
    TypeSymbolPtr type;       // The parameter's type (base type, NOT ReferenceType)
    std::string name;         // Parameter name (empty for type annotations)
    bool isReference = false; // T& parameter: pass-by-pointer semantics
};
```

The `isReference` flag is critical. When a function type is `(int&) => void`, the `ParameterInfo` stores `type=int, isReference=true`, NOT `type=ReferenceType(int)`. This ensures `mapType()` produces `i32` (not `ptr`) for the base type, while codegen generates pointer-passing calling convention at the call site.

### Closure Representation

All function-typed values use a fat pointer in LLVM IR: `{ ptr, ptr }` = `{ fnPtr, envPtr }`.

- **fnPtr**: Pointer to the LLVM function (e.g., `@__lambda_0`)
- **envPtr**: Pointer to the heap-allocated capture environment, or `null` for non-capturing lambdas

Every lambda function receives `ptr %env` as its **last** parameter, even non-capturing ones. The capture environment struct layout is:

```
{ i64 refcount, ptr cleanup_fn, T0 capture0, T1 capture1, ... }
```

For by-reference captures (`[&x]`), the environment stores a pointer to the original alloca:

```
{ i64 refcount, ptr cleanup_fn, ptr ref_to_x, i32 val_capture_y, ... }
```

### Null Closures

Assigning `null` to a closure variable produces a zeroed fat pointer:

```llvm
store { ptr, ptr } zeroinitializer, ptr %closure_var
```

Calling a null closure is undefined behavior (the programmer must check).

---

## 5. Composite Types

### TupleTypeSymbol

```
TupleTypeSymbol
  +-- elementTypes: vector<TypeSymbolPtr>
```

Tuples map to anonymous LLVM struct types:

```llvm
; (int, double, bool)  ->  { i32, double, i1 }
```

Tuple elements are accessed by index (0-based). Tuple destructuring declarations (`var (a, b, c) = tuple_expr`) have their variable types inferred from the tuple's element types during Pass 3.

### PointerTypeSymbol

```
PointerTypeSymbol
  +-- baseType: TypeSymbolPtr
```

All pointer types map to opaque `ptr` in LLVM IR (following LLVM 14+ conventions), with one exception: `PointerTypeSymbol(InterfaceSymbol)` maps to the fat pointer `{ ptr, ptr }` because interface pointers carry an itable alongside the object pointer.

### ArrayTypeSymbol

```
ArrayTypeSymbol
  +-- elementType: TypeSymbolPtr
  +-- arraySize: int            (-1 = dynamic, >0 = fixed)
```

LLVM mapping:
- Fixed-size `int[16]` maps to `[16 x i32]`
- Dynamic/unsized `int[]` maps to `ptr` (raw heap pointer)

### ReferenceTypeSymbol

```
ReferenceTypeSymbol
  +-- baseType: TypeSymbolPtr
```

**Transient**: `ReferenceTypeSymbol` is created during type annotation parsing (e.g., `int&`) but is immediately unwrapped by TypeResolver in `resolveParameters()`. The base type goes onto `VariableSymbol::type` and `isReference=true` is set on the `VariableSymbol`. After Pass 2, no `VariableSymbol` should have `ReferenceTypeSymbol` as its type.

This unwrapping is critical: without it, `mapType(ReferenceType(int))` would return `ptr` instead of `i32`, causing LLVM verification failures.

If `ReferenceTypeSymbol` survives to codegen (which should not happen), `mapType()` produces `ptr` as a fallback.

---

## 6. Type Compatibility

The `SymbolTable::isCompatible(TypeSymbol* from, TypeSymbol* to)` function implements all type compatibility rules. It is called by the TypeChecker at assignments, returns, call arguments, and ternary branches.

### Compatibility Rules (in priority order)

1. **Identity**: `from == to` (same pointer) -- always compatible.

2. **Error type**: If either side is `ErrorTypeSymbol`, return `true`. This prevents cascading errors after an initial type resolution failure.

3. **Null compatibility**: `NullTypeSymbol` is compatible with `PointerTypeSymbol` and `FunctionTypeSymbol`. Not compatible with primitives, structs, classes, or enums.

4. **Numeric widening** (implicit, lossless direction):
   - `byte` -> `int`
   - `char` -> `int`
   - `int` -> `float`
   - `int` -> `double`
   - `float` -> `double`

   These are one-directional: `double` is NOT implicitly compatible with `int`. Use explicit casts for narrowing conversions.

5. **Enum interoperability**: Bidirectional compatibility between an enum and its underlying type. An `enum Color : int` value can be used where `int` is expected, and vice versa. Widening rules also apply transitively (e.g., a byte-backed enum can widen to int).

6. **Interface upcast**: `PointerTypeSymbol(ClassSymbol)` -> `PointerTypeSymbol(InterfaceSymbol)` if the class implements the interface. Checked by walking `ClassSymbol::implementedInterfaces`.

7. **Universal byte pointer**: `byte*` is compatible with any `T*` in both directions. This serves as a `void*` equivalent for raw memory operations.

8. **Inheritance upcast**: `Derived*` -> `Base*`. Checked by walking `ClassSymbol::resolvedBaseClass` chain from `from` toward the root. Does NOT work in the reverse direction (no implicit downcasting).

9. **Implicit reference**: `T` -> `T&`. When a function expects a reference parameter, passing a value of the base type is compatible (the call site takes the address implicitly).

### Wider Type Resolution

For binary expressions with numeric operands, `TypeChecker::getWiderType()` determines the result type using a rank system:

```
byte(1) < char(2) < int(3) < float(4) < double(5)
```

The higher-ranked type wins. Enum operands are unwrapped to their underlying type before ranking.

---

## 7. Type Interning

The `SymbolTable` maintains a type interning map (`types_`: `unordered_map<string, TypeSymbolPtr>`) that ensures structural type identity: two types with the same structure share the same `TypeSymbolPtr` instance.

### Pre-registered Types

On construction, `SymbolTable::registerPrimitives()` creates and registers all eight primitive types, plus `ErrorTypeSymbol` and `NullTypeSymbol`:

```
"int"     -> PrimitiveTypeSymbol("int", Int, 4)
"double"  -> PrimitiveTypeSymbol("double", Double, 8)
"float"   -> PrimitiveTypeSymbol("float", Float, 4)
"byte"    -> PrimitiveTypeSymbol("byte", Byte, 1)
"char"    -> PrimitiveTypeSymbol("char", Char, 1)
"string"  -> PrimitiveTypeSymbol("string", String, 8)
"bool"    -> PrimitiveTypeSymbol("bool", Bool, 1)
"void"    -> PrimitiveTypeSymbol("void", Void, 0)
"<error>" -> ErrorTypeSymbol
"null"    -> NullTypeSymbol
```

### Compound Type Interning

Compound types are interned by structural key (get-or-create pattern):

| Type | Key Format | Example |
|------|-----------|---------|
| `PointerTypeSymbol` | `<base>*` | `int*` |
| `ArrayTypeSymbol` | `<elem>[<size>]` | `int[16]` |
| `TupleTypeSymbol` | `(<elem1>,<elem2>,...)` | `(int,double)` |
| `FunctionTypeSymbol` | `(<p1>,<p2>&,...)=><ret>` | `(int,double&)=>int` |
| `ReferenceTypeSymbol` | `<base>&` | `int&` |

For function types, the `&` suffix on a parameter indicates `isReference=true`. The interning key includes this flag to distinguish `(int) => void` from `(int&) => void`.

### User-Defined Type Registration

Classes, structs, enums, and interfaces are registered by name via `SymbolTable::registerType(name, type)` during Pass 1. They are NOT interned by structural key -- they are nominal types identified by their declared name.

### Type Resolution

`SymbolTable::resolveType(name)` looks up the interning map by name. For user-defined types, this returns the registered `TypeSymbol`. For compound types that were interned, the structural key must be used.

The TypeResolver in Pass 2 uses scope-chain resolution for named types: it first checks the `SymbolTable` type registry, then falls back to `scope->resolve(name)` for types not yet registered. Qualified names like `Module.TypeName` resolve by first finding the module symbol, then resolving the type name within the module's scope.

---

## 8. LLVM Type Mapping

The `IRGenerator::mapType(TypeSymbol*)` function translates every Mingus type to its LLVM IR representation. This is the single entry point for all type mapping in codegen.

### Complete Mapping Table

| Mingus Type | LLVM IR Type | Notes |
|------------|-------------|-------|
| `int` | `i32` | Signed 32-bit |
| `double` | `double` | 64-bit IEEE 754 |
| `float` | `float` | 32-bit IEEE 754 |
| `byte` | `i8` | Unsigned 8-bit |
| `char` | `i8` | Same representation as byte |
| `bool` | `i1` | Single bit |
| `void` | `void` | |
| `string` | `ptr` | Pointer to null-terminated C string |
| `T*` | `ptr` | Opaque pointer |
| `Interface*` | `{ ptr, ptr }` | Fat pointer: `{ objPtr, itablePtr }` |
| `int[16]` | `[16 x i32]` | Fixed-size array |
| `int[]` | `ptr` | Dynamic array = raw pointer |
| `(int, double)` | `{ i32, double }` | Anonymous struct |
| `enum Color : int` | `i32` | Maps to underlying type |
| `enum Status : string` | `ptr` | String-backed enum |
| Struct | `%Name = type { ... }` | Named struct, fields in order |
| Class (virtual) | `%Name = type { ptr, ... }` | Vtable pointer at index 0, then fields |
| Class (no virtual) | `%Name = type { ... }` | Fields only |
| `(int) => double` | `{ ptr, ptr }` | Fat pointer: `{ fnPtr, envPtr }` |
| `T&` | `ptr` | Transparent pointer (fallback only) |
| Error/Null | `ptr` | Fallback for sentinel types |

### Struct and Class Layout

Struct and class LLVM types are cached in `structTypeCache_` to ensure consistent layout. They are created as opaque named types first, then their bodies are set after all types are declared (supporting forward references and mutual recursion).

**Struct layout**: Fields in declaration order.

```llvm
%Vec3 = type { double, double, double }
```

**Class layout**: Vtable pointer (if `hasVtable()`) at index 0, then `allFields` (inherited first, then own).

```llvm
; class Animal { int age; }  -- with virtual methods
%Animal = type { ptr, i32 }    ; { vtable_ptr, age }

; class Dog : Animal { string name; }
%Dog = type { ptr, i32, ptr }  ; { vtable_ptr, age (inherited), name (own) }
```

The field GEP index formula: `(hasVtable ? 1 : 0) + index_in_allFields`

### Parameter Type Mapping

`mapParamType(TypeSymbol*, bool isReference)` is a specialized variant for function parameters:

- If `isReference=true`, always returns `ptr` (pointer to caller's alloca)
- If the type is a class or struct, returns `ptr` (passed by pointer)
- If the type is an interface, returns `{ ptr, ptr }` (fat pointer)
- Otherwise, delegates to `mapType()`

### Fat Pointer Type

`getFatPtrType()` returns `{ ptr, ptr }` -- a two-element struct of opaque pointers. This type is shared by closures and interface pointers:

```llvm
; Closure:   { fnPtr,  envPtr   }
; Interface: { objPtr, itablePtr }
```

---

## 9. Method Dispatch

Mingus supports three kinds of method dispatch: static (direct), virtual (vtable), and interface (itable).

### Static Dispatch

Non-virtual methods and static methods are dispatched directly by function name:

```llvm
; obj.nonVirtualMethod(args)
call <retTy> @ClassName_methodName(ptr %obj, <args>)

; ClassName.staticMethod(args)  -- no 'this' pointer
call <retTy> @ClassName_staticMethod(<args>)
```

Static methods have `FunctionSymbol::isStatic = true`. When `isStatic` is true, the call site does not pass a `this` pointer.

### Virtual Dispatch (Vtables)

#### Vtable Structure

Each non-abstract class with virtual methods has a global vtable: a constant array of function pointers.

```llvm
@Animal_vtable = internal constant [3 x ptr] [
    ptr @Animal_destructor,   ; slot 0: always the destructor
    ptr @Animal_speak,        ; slot 1: virtual method
    ptr @Animal_move          ; slot 2: virtual method
]
```

**Slot 0 is always the destructor.** User methods start at index 1. This is a critical invariant: `delete` dispatches through vtable slot 0 to ensure the most-derived destructor runs.

For derived classes, slots are inherited from the base and overridden:

```llvm
@Dog_vtable = internal constant [4 x ptr] [
    ptr @Dog_destructor,      ; slot 0: overridden destructor
    ptr @Dog_speak,           ; slot 1: overridden from Animal
    ptr @Animal_move,         ; slot 2: inherited (not overridden)
    ptr @Dog_fetch            ; slot 3: new virtual method
]
```

Vtable globals are created by `IRGenerator::declareVtables()`, which iterates over all class symbols with vtables and populates entries from the `functionCache_`.

#### Vtable Pointer Storage

The vtable pointer is stored at GEP index 0 of every class instance. It is written in the constructor:

```llvm
define void @Dog_constructor(ptr %this) {
    %vtable.slot = getelementptr %Dog, ptr %this, i32 0, i32 0
    store ptr @Dog_vtable, ptr %vtable.slot
    ; ... rest of constructor ...
}
```

For inheritance: the base constructor runs first (sets base vtable), then the derived constructor overwrites with the derived vtable. This ensures correct polymorphism at every point during construction.

#### Virtual Method Call

When `calleeFuncSym->vtableIndex >= 0` and `classSym->hasVtable()`:

```llvm
; animal->speak()   -- may actually be a Dog
; 1. Load vtable pointer from object
%vt.ptr = getelementptr %Animal, ptr %obj, i32 0, i32 0
%vt = load ptr, ptr %vt.ptr

; 2. Index into vtable at the method's slot
%slot = getelementptr ptr, ptr %vt, i32 1   ; speak is slot 1

; 3. Load function pointer
%fn = load ptr, ptr %slot

; 4. Indirect call with this pointer
%result = call <retTy> %fn(ptr %obj, <args>)
```

#### Virtual Destructor via delete

`delete ptr` dispatches the destructor through vtable slot 0:

```llvm
; delete animal_ptr;   -- may be a Dog, Cat, etc.
%vt.ptr = getelementptr %Animal, ptr %p, i32 0, i32 0
%vt = load ptr, ptr %vt.ptr
%dtor.slot = getelementptr ptr, ptr %vt, i32 0     ; slot 0 = destructor
%dtor = load ptr, ptr %dtor.slot
call void %dtor(ptr %p)
call void @free(ptr %p)
```

#### Destructor Chaining

Derived destructors call the base destructor at the end:

```
Dog_destructor(ptr %this):
    ; ... user destructor body ...
    ; ... release closure fields (auto-generated epilogue) ...
    call void @Animal_destructor(ptr %this)   ; chain to base
```

Order is LIFO: derived cleanup first, then base cleanup.

### Interface Dispatch (Itables)

#### Itable Structure

For each `(Class, Interface)` pair where the class implements the interface, a global constant itable is created:

```llvm
@Circle.Drawable.itable = internal constant [1 x ptr] [
    ptr @Circle_draw       ; slot 0: implements Drawable.draw()
]

@Circle.Resizable.itable = internal constant [2 x ptr] [
    ptr @Circle_resize,    ; slot 0: implements Resizable.resize()
    ptr @Circle_scale      ; slot 1: implements Resizable.scale()
]
```

Slots are ordered by the interface's method declaration order. Each slot holds a pointer to the concrete implementation. The itable is built by `IRGenerator::declareItables()`, which resolves each interface method name in the class scope.

#### Wrapping to Interface Fat Pointer

When a concrete class pointer is assigned to an interface pointer:

```llvm
; Drawable* d = circle_ptr;
%fat.obj = insertvalue { ptr, ptr } undef, ptr %circle_ptr, 0
%fat.itable = insertvalue { ptr, ptr } %fat.obj, ptr @Circle.Drawable.itable, 1
store { ptr, ptr } %fat.itable, ptr %d_alloca
```

This wrapping is performed by `emitWrapToInterfacePtr()`, which looks up the itable in the `itableCache_` using the `{ClassSymbol*, InterfaceSymbol*}` key.

#### Interface Method Call

```llvm
; d->draw()   where d is Drawable*
; 1. Load fat pointer
%fat = load { ptr, ptr }, ptr %d_alloca

; 2. Extract components
%obj = extractvalue { ptr, ptr } %fat, 0        ; objPtr
%itable = extractvalue { ptr, ptr } %fat, 1     ; itablePtr

; 3. Index into itable
%slot = getelementptr ptr, ptr %itable, i32 0   ; draw is slot 0

; 4. Load function pointer
%fn = load ptr, ptr %slot

; 5. Call with objPtr as this
call void %fn(ptr %obj)
```

This is a Go-style interface representation: the dispatch table travels with the pointer, not inside the object. This means a single object can have different itables depending on which interface it is viewed as.

#### Interface Argument Wrapping

When passing a class pointer as an interface parameter in a method call, the TypeChecker detects the mismatch (via `isCompatible()`) and codegen wraps the argument using `emitWrapToInterfacePtr()`. This happens at the call site, not at the declaration site.

---

## 10. Operator Overloading

### Supported Operators

Mingus supports overloading the following operators via the `OverloadableOp` enum:

| Operator | `OverloadableOp` | Mangled Suffix |
|----------|------------------|----------------|
| `+` | `Add` | `operator_add` |
| `-` (binary) | `Sub` | `operator_sub` |
| `*` | `Mul` | `operator_mul` |
| `/` | `Div` | `operator_div` |
| `%` | `Mod` | `operator_mod` |
| `==` | `Equal` | `operator_eq` |
| `!=` | `NotEqual` | `operator_neq` |
| `<` | `Less` | `operator_lt` |
| `>` | `Greater` | `operator_gt` |
| `<=` | `LessEq` | `operator_lte` |
| `>=` | `GreaterEq` | `operator_gte` |
| `-` (unary) | `Negate` | `operator_neg` |
| `[]` | `Index` | `operator_index` |

### Operator Symbol

```cpp
class OperatorSymbol : public FunctionSymbol {
    OverloadableOp op;
    TypeSymbolPtr ownerType;    // the type that defines this operator
};
```

Operators are stored in a separate namespace within each scope (via `Scope::defineOperator()` and `Scope::resolveOperator()`), distinct from regular symbol definitions.

### Resolution (Pass 3)

When the TypeChecker encounters a `BinaryExpression`:

1. Visit left and right children (bottom-up type inference)
2. Get the left operand's type
3. Call `findOperatorOverload(leftType, binaryOpToOverloadable(op))`
4. This casts the type to a `Scope*` and calls `resolveOperator(op)`
5. If found, set `node.isOperatorOverload = true` and `node.resolvedOperatorFunction = opSym`
6. The expression's result type is `opSym->returnType`

For `IndexExpression`, the same resolution applies but checks `OverloadableOp::Index` on the object type.

**Important limitation**: Operator resolution checks only the **left** operand's type. `42 + vec` will NOT find `Vec3::operator+` because the left operand is `int`, not `Vec3`. Write `vec + 42` instead.

### Codegen

When `node.isOperatorOverload` is true in a `BinaryExpression`, codegen:

1. Gets the `llvm::Function*` from `functionCache_` using the resolved `OperatorSymbol`
2. Emits the left operand as an lvalue (pointer). If it is a struct value (not addressable), creates a temporary alloca
3. Emits the right operand. If it is a struct type, also ensures it is passed by pointer
4. Calls the operator function: `call %RetTy @TypeName_operator_add(ptr %lhs, ptr %rhs)`

```llvm
; a + b  where a, b are Vec3
%a_ptr = ...                ; pointer to left operand
%b_ptr = ...                ; pointer to right operand
%result = call %Vec3 @Vec3_operator_add(ptr %a_ptr, ptr %b_ptr)
```

For `IndexExpression` operator overloads:
```llvm
; arr[i]  where arr has operator[]
%result = call i32 @DynamicArray_operator_index(ptr %arr_ptr, i32 %i)
```

### Operator Function Signature

```mingus
struct Vec3 {
    double x;
    double y;
    double z;

    operator+(Vec3 rhs) Vec3 {
        return Vec3(this.x + rhs.x, this.y + rhs.y, this.z + rhs.z);
    }
}
```

The left operand is always `this` (passed as the first pointer parameter). The right operand is the explicit parameter.

---

## 11. Access Modifiers

### AccessModifier Enum

```cpp
enum class AccessModifier {
    Public,       // accessible from anywhere
    Protected,    // accessible from class and subclasses
    Private       // accessible from class only
};
```

Every `Symbol` carries an `accessLevel` field, defaulting to `AccessModifier::Public`.

### ModifierType Enum

Source-level modifiers are parsed as `ModifierType` values on `ModifiersNode`:

```cpp
enum class ModifierType {
    Public,
    Private,
    Protected,
    Static,
    Abstract,
    Extern,
    Virtual,
    Override
};
```

Pass 1 (SymbolTableBuilder) reads these modifiers and sets the appropriate flags:
- `Public/Private/Protected` -> `Symbol::accessLevel`
- `Static` -> `FunctionSymbol::isStatic`
- `Abstract` -> `FunctionSymbol::isAbstract`, `ClassSymbol::isAbstract`
- `Extern` -> `FunctionSymbol::isExtern`
- `Virtual` -> `FunctionSymbol::isVirtual`
- `Override` -> validated against base class method

### Enforcement

Access modifiers are enforced during semantic analysis (Pass 4, SemanticValidator). When resolving a member access, the validator checks whether the current context (class, subclass, or external code) has permission to access the target symbol. Violations produce compile errors.

---

## 12. Const System

### Variable Mutability

`VariableSymbol` carries a `isMutable` flag:

```cpp
class VariableSymbol : public TypedSymbol {
    bool isMutable = true;     // true for 'var', false for 'const'
    bool isInferred = false;   // was type inferred (var x = expr)?
    bool isInitialized = false; // has a definite assignment?
};
```

### Const Declaration

In Mingus source:
```mingus
const int MAX_SIZE = 100;    // isMutable = false
var count = 0;               // isMutable = true
```

### Sema Enforcement

The TypeChecker (Pass 3) enforces const-correctness in `visit(AssignmentExpression)`:

```cpp
if (auto* varSym = node.target->resolvedSymbol->as<VariableSymbol>()) {
    if (!varSym->isMutable) {
        errors_.error("assignment to const variable '" + varSym->getName() + "'",
            node.debugInfo);
    }
}
```

This check runs for every assignment expression. If the target resolves to a `VariableSymbol` with `isMutable=false`, the assignment is rejected with a compile error.

### LValue Validation

The TypeChecker also validates that assignment targets are valid lvalues:

- `IdentifierExpression` -- local/parameter/field
- `MemberAccessExpression` -- `obj.field`
- `IndexExpression` -- `arr[i]`
- `UnaryExpression` with `Dereference` -- `*ptr`

Anything else (literals, function calls, binary expressions) produces "assignment to non-lvalue".

---

## 13. Typedef / Type Aliases

### Overview

Typedefs create transparent name aliases for existing types. They do NOT introduce new types — they are pure name aliases that resolve to an existing `TypeSymbol`.

### Syntax

```mingus
typedef int Count;
typedef (int) => bool Predicate;
typedef Vec3 Position;
```

### Implementation

During Pass 1, the SymbolTableBuilder processes typedef declarations by:

1. Resolving the target type name to its `TypeSymbol`
2. Registering the alias name in the current scope, pointing directly to the underlying `TypeSymbol`

Because the alias maps to the same `TypeSymbol` instance as the original type, no new `TypeSymbol` is created. The alias is simply another name for the same object.

### Type Compatibility

Since a typedef resolves to the exact same `TypeSymbol` as its target, `isCompatible()` treats typedef aliases identically to their underlying type. The identity check (`from == to`) passes because both names resolve to the same `shared_ptr` instance. No special compatibility logic is needed.

### Scope

Typedefs work with all types: primitives, structs, classes, function types, pointers, arrays, and tuples. The alias is visible within the scope where it is declared and any nested scopes.

---

## 14. Function Overloading

### Overview

Function overloading allows multiple functions with the same name but different parameter signatures to coexist in the same scope. This applies to both free functions and class methods.

### Symbol Registration

The SymbolTableBuilder tracks overloads via a `functionOverloads_` parallel map in each scope. When a function name is already defined in a scope, the new definition is added to the overload set rather than producing a redefinition error.

### Overload Resolution

The TypeChecker performs scoring-based overload resolution when a `CallExpression` target has multiple candidates:

1. **Parameter count**: Must match exactly. Candidates with wrong arity are eliminated.
2. **Exact type match**: Each argument that matches its parameter type exactly contributes the highest score.
3. **Compatible type**: An argument that is compatible but not identical (e.g., `int` passed to `double`) contributes a lower score.
4. The candidate with the highest total score wins.

If no candidate matches or multiple candidates tie, a compile error is produced.

### Name Mangling

Overloaded functions get a `$_type` suffix appended to their mangled name for LLVM IR disambiguation. The suffix encodes the parameter types:

```
func add(int a, int b) => int;      // Module_add$_int_int
func add(double a, double b) => double;  // Module_add$_double_double
```

This ensures each overload has a unique LLVM function name while preserving the shared source-level name for resolution.

### Codegen

After the TypeChecker resolves the correct overload and sets `CallExpression::resolvedCallee`, codegen uses the mangled name to look up the correct `llvm::Function*` in the `functionCache_`. No additional disambiguation is needed at the IR level.

---

## 15. Covariant Return Types

### Overview

When overriding a virtual method, the return type can be a more derived pointer type than the base method's return type. This is known as covariant return.

### Example

```mingus
class Animal {
    virtual func clone() => Animal* {
        return new Animal();
    }
}

class Dog : Animal {
    override func clone() => Dog* {    // covariant: Dog* narrows Animal*
        return new Dog();
    }
}
```

### Validation

The TypeChecker validates covariant returns during override checking. The override's return type must satisfy one of:

- **Identical type** to the base method's return type, OR
- **A pointer to a subclass** of the base method's return pointer type. The subclass relationship is verified by walking the `ClassSymbol::resolvedBaseClass` inheritance chain.

Non-pointer covariant returns (e.g., base returns `int`, override returns `byte`) and unrelated pointer types are rejected with a compile error.

### LLVM Representation

At the LLVM level, all pointer types map to opaque `ptr` (following LLVM 14+ conventions). Because both the base return type (`Animal*`) and the covariant return type (`Dog*`) map to `ptr`, no special codegen is needed. The vtable slot holds the overriding function, and the caller receives a `ptr` regardless of which concrete type is returned.

---

## 16. Copy and Move Constructor Type Matching

### Copy Constructor Detection

The ASTGenerator detects copy constructors by examining constructor parameters. A constructor is a copy constructor if it has a single parameter that is a reference (`&`) to the enclosing class type.

The detection requires a key subtlety: the parameter's `TypeNode` is a `PointerTypeNode` with `isReference=true`. The underlying `NamedTypeNode` is nested inside this wrapper. The ASTGenerator must unwrap the `PointerTypeNode` first to find the `NamedTypeNode`, then compare the type name against the enclosing class name.

```mingus
class Foo {
    constructor(Foo& other) {    // copy constructor
        // PointerTypeNode(isReference=true) wraps NamedTypeNode("Foo")
    }
}
```

The mangled name is `ClassName_copy_constructor`.

### Move Constructor Detection

Move constructors follow the same pattern but with an rvalue reference (`&&`). The `PointerTypeNode` has both `isReference=true` and `isRvalueReference=true`.

A critical parsing subtlety: the `&&` token is lexed as a single `LogicalAndOperator` token, NOT two `SingleAndOperator` (`&`) tokens. This is due to ANTLR's longest-match rule. The grammar and ASTGenerator handle this by recognizing `LogicalAndOperator` in the parameter type position as an rvalue reference marker.

```mingus
class Foo {
    constructor(Foo&& other) {   // move constructor
        // PointerTypeNode(isReference=true, isRvalueReference=true)
        // wraps NamedTypeNode("Foo")
    }
}
```

The move constructor is invoked via `move(expr)`, which produces a `MoveExpression` AST node.

---

## Appendix: Type System Invariants

These invariants must be maintained for correct compilation:

1. **ReferenceType unwrapping**: After Pass 2, no `VariableSymbol` should have `ReferenceTypeSymbol` as its type. The base type is stored, with `isReference=true`. Violating this causes `mapType()` to produce `ptr` instead of the base type's LLVM representation.

2. **FunctionTypeSymbol ParameterInfo carries isReference**: When building a `FunctionTypeSymbol` (via `buildFunctionType()` or `getFunctionType()`), `isReference` must be propagated from the `VariableSymbol`. Without this, call sites cannot determine whether to pass values or pointers for closure calls.

3. **Vtable slot 0 = destructor**: All vtable construction must place the destructor at index 0. User methods start at index 1. `delete` dispatches through vtable[0].

4. **allFields ordering**: `ClassSymbol::allFields` must contain inherited fields first, then own fields. This ensures that a `Base*` can access fields at the same GEP indices regardless of whether the actual object is `Base` or `Derived`.

5. **Type identity via interning**: Two types that are structurally identical must resolve to the same `shared_ptr` instance. The `isCompatible()` identity check (`from == to`) relies on this.

6. **ErrorType absorbs errors**: `isCompatible()` must always return `true` when either operand is `ErrorTypeSymbol`. Otherwise, a single unresolved type causes an avalanche of downstream type errors.

7. **Null compatibility is limited**: `NullTypeSymbol` is only compatible with `PointerTypeSymbol` and `FunctionTypeSymbol`. Allowing null compatibility with value types (int, struct, etc.) would produce invalid IR.
