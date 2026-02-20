# Mingus Type System and Dispatch

This document describes how Mingus types are represented in LLVM IR, how virtual dispatch works (vtables and interface tables), and how operator overloading is implemented.

**Source files:**
- `src/mingus/codegen/IRGenerator.cpp` — Type mapping, dispatch codegen
- `include/mingus/codegen/IRGenerator.h` — Caches and helpers
- `include/mingus/ast/Type.h` — Type hierarchy
- `include/mingus/sema/Symbol.h` — Symbol hierarchy

---

## Table of Contents

1. [LLVM IR Type Mapping](#1-llvm-ir-type-mapping)
2. [Fat Pointer System](#2-fat-pointer-system)
3. [Virtual Dispatch (Vtables)](#3-virtual-dispatch-vtables)
4. [Interface Dispatch (Itables)](#4-interface-dispatch-itables)
5. [Operator Overloading](#5-operator-overloading)
6. [Dispatch Limitations](#6-dispatch-limitations)

---

## 1. LLVM IR Type Mapping

### Primitive Types

| Mingus Type | LLVM IR Type | Notes |
|------------|-------------|-------|
| `int` | `i32` | Signed 32-bit |
| `double` | `double` | 64-bit IEEE |
| `float` | `float` | 32-bit IEEE |
| `byte` | `i8` | Unsigned 8-bit |
| `char` | `i8` | Same as byte in IR |
| `bool` | `i1` | Single bit |
| `void` | `void` | |
| `string` | `ptr` | Null-terminated C string |

All pointers use LLVM opaque pointers (`ptr`), following LLVM 14+ conventions.

### Struct Types

Named LLVM struct types, fields in declaration order:

```llvm
%Vec3 = type { double, double, double }
```

Field access via GEP with `fieldIndex`:
```llvm
%x_ptr = getelementptr %Vec3, ptr %v, i32 0, i32 0
%x = load double, ptr %x_ptr
```

### Class Types

Named struct types with optional vtable pointer at index 0:

```llvm
; Class with virtual methods:
%DynamicArray = type { ptr,     ; vtable pointer (GEP index 0)
                       ptr,     ; field: data (GEP index 1)
                       i32,     ; field: size (GEP index 2)
                       i32 }    ; field: capacity (GEP index 3)

; Inherited class layout:
%Derived = type { ptr,          ; vtable pointer
                  i32,          ; inherited field from Base (allFields[0])
                  double,       ; inherited field from Base (allFields[1])
                  i32 }         ; own field (allFields[2])
```

Field GEP index formula: `(vtableSize > 0 ? 1 : 0) + index_in_allFields`

`allFields` includes base class fields first (copied in `buildVtable`), then own fields. This ensures correct ABI compatibility for upcasting — a `Base*` can access its fields at the same GEP indices whether the object is actually a `Base` or a `Derived`.

### Enum Types

Enums use their underlying type (default `i32`):

```llvm
; enum Color : int { Red = 0, Green = 1, Blue = 2 }
; Color values are just i32 constants

; String enums use global string pointers:
@Color_str_Red = private unnamed_addr constant [4 x i8] c"Red\00"
```

### Pointer Types

```llvm
; int*    → ptr
; byte*   → ptr
; Class*  → ptr
; T[]     → ptr (dynamic array = raw pointer)
```

### Array Types

```llvm
; int[16]   → [16 x i32]
; double[4] → [4 x double]
; int[]     → ptr  (unsized = raw pointer)
```

Fixed-size array access: `getelementptr [16 x i32], ptr %arr, i32 0, i32 %idx`

### Tuple Types

Anonymous LLVM struct types:

```llvm
; (int, double) → { i32, double }
; (int, string, bool) → { i32, ptr, i1 }
```

### Function Types (Closures)

All function-typed values are fat pointers: `{ ptr, ptr }` = `{ fnPtr, envPtr }`.

### Reference Types

`T&` → `ptr` (transparent alias to caller's alloca). The `isReference` flag on `VariableSymbol` distinguishes from regular pointers at codegen time.

### Complete Type Layout Summary

```
Primitive int:       i32
Primitive double:    double
Primitive bool:      i1
Primitive char:      i8
Primitive string:    ptr → null-terminated C string

Struct:              %Name = type { field0, field1, ... }
Class (virtual):     %Name = type { ptr, inherited..., own... }
Class (no virtual):  %Name = type { inherited..., own... }
Enum:                i32 (or underlying type)
Tuple:               { T0, T1, ... }  (anonymous)

FunctionType:        { ptr, ptr }  (fat pointer: fnPtr + envPtr)
InterfaceType*:      { ptr, ptr }  (fat pointer: objPtr + itablePtr)
PointerType*:        ptr
ReferenceType&:      ptr

Fixed array T[N]:    [N x T]
Dynamic array T[]:   ptr

Closure env:         { i64, ptr, T0, T1, ... }  (refcount + cleanup + captures)
```

---

## 2. Fat Pointer System

The `{ ptr, ptr }` struct type is shared by two concepts:

### Closures: `{ fnPtr, envPtr }`

- **fnPtr**: pointer to the lambda's LLVM function (`@__lambda_N`)
- **envPtr**: pointer to heap-allocated capture environment (or `null` for non-capturing)

All lambda functions receive `ptr %env` as their last parameter, even non-capturing ones (the value is just `null`).

### Interfaces: `{ objPtr, itablePtr }`

- **objPtr**: raw pointer to the concrete class object
- **itablePtr**: pointer to the itable global (`@ClassName.InterfaceName.itable`)

This is a Go-style interface representation — the dispatch table travels with the pointer, not inside the object.

### Assembly

```llvm
; Building a fat pointer:
%fat = insertvalue { ptr, ptr } undef, ptr %component0, 0
%fat1 = insertvalue { ptr, ptr } %fat, ptr %component1, 1

; Extracting:
%fn = extractvalue { ptr, ptr } %fat, 0
%env = extractvalue { ptr, ptr } %fat, 1
```

---

## 3. Virtual Dispatch (Vtables)

### Vtable Structure

Global constant array of function pointers:

```llvm
@DynamicArray_vtable = internal constant [3 x ptr] [
    ptr @DynamicArray_destructor,   ; slot 0: always destructor
    ptr @DynamicArray_push,         ; slot 1: virtual method
    ptr @DynamicArray_get           ; slot 2: virtual method
]
```

**Slot 0 is always the destructor** for virtual destructor support.

For derived classes, slots are inherited from the base and overridden by name:

```llvm
; If class Dog overrides Animal.speak():
@Dog_vtable = internal constant [3 x ptr] [
    ptr @Dog_destructor,     ; slot 0: overridden destructor
    ptr @Dog_speak,          ; slot 1: overridden from Animal
    ptr @Dog_fetch           ; slot 2: new method
]
```

### Vtable Pointer Storage

Stored at GEP index 0 of every class instance:

```llvm
; In constructor:
%vtable.slot = getelementptr %Dog, ptr %this, i32 0, i32 0
store ptr @Dog_vtable, ptr %vtable.slot
```

For derived classes: base constructor runs first (sets base vtable), then derived constructor overwrites with derived vtable. This ensures correct polymorphism at every point during construction.

### Virtual Method Call

When `methodSym->vtableIndex >= 0` and `classSym->vtableSize > 0`:

```llvm
; obj->method(args)
; 1. Load vtable pointer
%vt.ptr = getelementptr %Class, ptr %obj, i32 0, i32 0
%vt = load ptr, ptr %vt.ptr

; 2. Index into vtable
%slot = getelementptr ptr, ptr %vt, i32 <vtableIndex>

; 3. Load function pointer
%fn = load ptr, ptr %slot

; 4. Indirect call
%result = call <retTy> %fn(ptr %obj, <args>)
```

### Virtual Destructor via `delete`

`delete p` dispatches through vtable slot 0 to ensure the most-derived destructor runs:

```llvm
; delete animal_ptr;  — may actually be a Dog
%vt.ptr = getelementptr %Animal, ptr %p, i32 0, i32 0
%vt = load ptr, ptr %vt.ptr
%dtor.slot = getelementptr ptr, ptr %vt, i32 0
%dtor = load ptr, ptr %dtor.slot
call void %dtor(ptr %p)
call void @free(ptr %p)
```

### Destructor Chaining

Derived destructors explicitly call the base destructor at the end:

```
Dog_destructor(ptr %this):
    ; ... user destructor body ...
    ; ... release closure fields (auto-generated epilogue) ...
    call void @Animal_destructor(ptr %this)   ; chain to base
```

LIFO order: derived cleanup first, then base cleanup.

---

## 4. Interface Dispatch (Itables)

### Itable Structure

Per `(Class, Interface)` pair, a global constant array:

```llvm
@Circle.Drawable.itable = internal constant [1 x ptr] [
    ptr @Circle_draw       ; slot 0: implements Drawable.draw()
]

@Circle.Resizable.itable = internal constant [1 x ptr] [
    ptr @Circle_resize     ; slot 0: implements Resizable.resize()
]
```

Slots are ordered by the interface's method declaration order. Each slot holds a pointer to the concrete implementation.

### Wrapping to Interface Fat Pointer

When a `Dog*` is assigned to a `Drawable*`:

```llvm
; Drawable* d = dog_ptr;
%fat.obj = insertvalue { ptr, ptr } undef, ptr %dog_ptr, 0
%fat.itable = insertvalue { ptr, ptr } %fat.obj, ptr @Dog.Drawable.itable, 1
store { ptr, ptr } %fat.itable, ptr %d_alloca
```

### Interface Method Call

```llvm
; d->draw()  where d is Drawable*
; 1. Load fat pointer
%fat = load { ptr, ptr }, ptr %d_alloca

; 2. Extract components
%obj = extractvalue { ptr, ptr } %fat, 0
%itable = extractvalue { ptr, ptr } %fat, 1

; 3. Index into itable
%slot = getelementptr ptr, ptr %itable, i32 <methodIndex>

; 4. Load function pointer
%fn = load ptr, ptr %slot

; 5. Call with objPtr as this
%result = call void %fn(ptr %obj)
```

### Interface delete

When deleting through an interface pointer, the object pointer is extracted from field 0:

```llvm
; delete drawable_ptr;
%fat = load { ptr, ptr }, ptr %d_alloca
%obj = extractvalue { ptr, ptr } %fat, 0
; ... call destructor ...
call void @free(ptr %obj)
```

---

## 5. Operator Overloading

### Name Mangling

Operator functions are mangled as `<TypeName>_operator_<opStr>`:

| Operator | Mangled Name |
|----------|-------------|
| `+` | `TypeName_operator_add` |
| `-` | `TypeName_operator_sub` |
| `*` | `TypeName_operator_mul` |
| `/` | `TypeName_operator_div` |
| `%` | `TypeName_operator_mod` |
| `==` | `TypeName_operator_eq` |
| `!=` | `TypeName_operator_neq` |
| `<` | `TypeName_operator_lt` |
| `<=` | `TypeName_operator_lte` |
| `>` | `TypeName_operator_gt` |
| `>=` | `TypeName_operator_gte` |
| `[]` | `TypeName_operator_index` |

### Function Signature

```llvm
define %Vec3 @Vec3_operator_add(ptr %this, ptr %rhs) {
    ; %this = pointer to left operand
    ; %rhs = pointer to right operand (struct passed by pointer)
    ; returns struct value
}
```

### Call Site

When `node.isOperatorOverload` is true in `BinaryExpression`:

```llvm
; a + b  where a, b are Vec3
; Get pointer to lhs (alloca or temp)
; Get pointer to rhs (alloca or temp)
%result = call %Vec3 @Vec3_operator_add(ptr %a_ptr, ptr %b_ptr)
```

For `IndexExpression` with operator overload:
```llvm
; arr[i]  where arr has operator[]
%result = call i32 @DynamicArray_operator_index(ptr %arr_ptr, i32 %i)
```

### Resolution

Operator overload resolution happens in Pass 3 (TypeChecker). When a `BinaryExpression` has a `UserType` left operand, Pass 3 checks for a matching operator in the type's member scope. If found, sets `isOperatorOverload = true` and `resolvedOperatorFunction`.

---

## 6. Dispatch Limitations

### Interface Parameters to Methods

Passing an interface fat pointer as a function argument has a calling convention mismatch. The function type expects `ptr` but the caller has a `{ ptr, ptr }` fat pointer. This makes interface-typed parameters unreliable.

### Vtable Ordering

New virtual methods added by a derived class are ordered alphabetically (from `std::map` iteration in SymbolTableBuilder), not in source declaration order. This is stable but surprising.

### No Multiple Inheritance

Only single class inheritance is supported. Multiple interfaces can be implemented, but multiple base classes cause errors.

### No Covariant Return Types

Overriding a virtual method requires the exact same return type. Covariant returns (e.g., returning `Dog*` where `Animal*` is expected) are not supported at the vtable level.

### Operator Overload by Left Operand Only

Operator resolution checks only the left operand's type. `42 + vec` will not find `Vec3::operator+` because the left operand is `int`, not `Vec3`.
