# Mingus Semantic Analysis Pipeline

The Mingus compiler performs four sequential semantic analysis passes over the AST before code generation. Each pass reads data deposited by prior passes and adds new information.

**Source files:**
- `src/mingus/sema/SymbolTableBuilder.cpp` — Pass 1
- `src/mingus/sema/TypeResolver.cpp` — Pass 2
- `src/mingus/sema/TypeChecker.cpp` — Pass 3
- `src/mingus/sema/SemanticValidator.cpp` — Pass 4
- `include/mingus/sema/Symbol.h` — Symbol types
- `src/mingus/sema/TypeRegistry.cpp` — Type registry and compatibility

---

## Table of Contents

1. [Pipeline Overview](#1-pipeline-overview)
2. [Type System](#2-type-system)
3. [Pass 1: Symbol Table Builder](#3-pass-1-symbol-table-builder)
4. [Pass 2: Type Resolver](#4-pass-2-type-resolver)
5. [Pass 3: Type Checker](#5-pass-3-type-checker)
6. [Pass 4: Semantic Validator](#6-pass-4-semantic-validator)
7. [Pass Interaction Summary](#7-pass-interaction-summary)

---

## 1. Pipeline Overview

```
AST (from ANTLR4/ASTGenerator)
  │
  ▼
Pass 1: SymbolTableBuilder
  → Scope tree, symbol objects, vtables, auto ctor/dtor, imports
  │
  ▼
Pass 2: TypeResolver
  → Resolves TypeNode → Type for all declarations (does NOT enter function bodies)
  │
  ▼
Pass 3: TypeChecker
  → Types all expressions, resolves identifiers, enforces compatibility
  │
  ▼
Pass 4: SemanticValidator
  → RAII tracking, lambda captures, control flow, raw block safety, exhaustiveness
  │
  ▼
IRGenerator (codegen)
```

---

## 2. Type System

### Type Hierarchy

**File:** `include/mingus/ast/Type.h`

```
Type (abstract)
├── PrimitiveType  — int, double, float, byte, char, bool, string, void
├── PointerType    — T*
├── ReferenceType  — T& (for parameters)
├── ArrayType      — T[N] or T[]
├── FunctionType   — (T1, T2) => R
├── TupleType      — (T1, T2, ...)
├── UserType       — structs, classes, enums, interfaces
├── NullType       — the type of `null`
└── ErrorType      — placeholder for unresolvable types
```

### Symbol Hierarchy

**File:** `include/mingus/sema/Symbol.h`

```
Symbol (abstract)
├── VariableSymbol    — variables, parameters, fields
├── FunctionSymbol    — functions, methods
├── ConstructorSymbol — class constructors
├── DestructorSymbol  — class destructors
├── OperatorSymbol    — operator overloads
├── TypeSymbol        — base for type-defining symbols
│   ├── ClassSymbol   — classes (+ vtable, allFields, interfaces)
│   ├── StructSymbol  — structs
│   ├── EnumSymbol    — enums
│   └── InterfaceSymbol — interfaces
└── ModuleSymbol      — modules
```

Key `VariableSymbol` fields:
- `role`: `Field` (struct/class member) or `Local` (function-local)
- `fieldIndex`: GEP index for struct/class fields
- `isReference`: true for `T&` parameters
- `isInferred`: true for `var` declarations
- `isMutable`: exists but not currently enforced

Key `ClassSymbol` fields:
- `vtable: vector<FunctionSymbol*>` — virtual method slots (slot 0 = destructor)
- `allFields: vector<VariableSymbol*>` — base fields + own fields
- `implementedInterfaces: vector<InterfaceSymbol*>`
- `baseClass: ClassSymbol*`
- `constructor`, `destructor`

### Type Compatibility Rules

**File:** `src/mingus/sema/TypeRegistry.cpp`

`isCompatible(from, to)` is the core compatibility check, used for assignments, arguments, and returns:

1. **Same type**: always compatible
2. **Pointer equality**: if both resolve to the same `Type*`
3. **UserType name match**: if both are `UserType` with the same `name` and `underlyingKind`
4. **Numeric widening** (one-directional): `byte` → `int`, `char` → `int`, `int` → `float`/`double`, `float` → `double`
5. **Null literal**: `NullType` compatible with any `PointerType` or `FunctionType`
6. **Enum ↔ underlying**: both directions allowed
7. **Interface pointer**: `Dog*` → `Drawable*` if Dog implements Drawable
8. **byte* universal pointer**: `byte*` compatible both ways with any `T*` (like `void*`)
9. **Inheritance pointer**: `Derived*` → `Base*`
10. **Inheritance value**: `Derived` → `Base`
11. **Reference**: `T&` ↔ `T&`, `T` → `T&` (implicit address-of at call site)

---

## 3. Pass 1: Symbol Table Builder

### Purpose

Creates all named symbols, builds the scope tree, resolves imports, builds vtables, and auto-generates constructors/destructors.

### Execution Structure

```
build(ProgramNode):
  Pass 1a: Visit all modules (create scopes, symbols — no imports resolved yet)
  Pass 1b: resolveAllImports() (cross-module symbol injection)
```

All module scopes must exist before any import can resolve, hence the two sub-passes.

### Scope Creation

| Construct | Scope Kind |
|-----------|-----------|
| Program (global) | `Global` |
| `module X { }` | `Module` |
| `struct/class/interface` | `TypeMembers` |
| `function/constructor/destructor/operator` | `Function` |
| `{ }` block | `Block` |
| `for` statement | `Block` (for loop variable) |
| `raw { }` | `RawBlock` |
| Lambda expression | `Function` |
| Match arm | `Block` |

### Symbol Registration

- **Functions**: Symbol defined in current scope before pushing function scope. Parameters defined inside function scope.
- **Struct fields**: `role = Field`, ascending `fieldIndex` (0-based).
- **Class fields**: Same as struct fields. Vtable and `allFields` built after all members processed.
- **Variables**: `role = Field` if inside TypeMembers scope, otherwise `Local`.
- **Enum members**: Values evaluated eagerly (auto-increment from last explicit value).
- **Lambda expressions**: Creates a `Function` scope with parameter symbols.
- **Binding patterns** (`var x` in match): Creates `Local` variable in the arm's `Block` scope.

### Auto-Generated Constructors and Destructors

If a `ClassDeclaration` lacks an explicit constructor, Pass 1 injects:
1. A synthetic `ConstructorDeclaration` AST node (empty body, no parameters, public)
2. A `ConstructorSymbol` in the class's TypeMembers scope

Same for missing destructors. This ensures `classSym->hasRAII()` always returns `true`, so all class instances get automatic destructor calls at scope exit.

### Import Resolution (Pass 1b)

**Selective import** (`import Foo, Bar from Mod`):
1. Look up source module in global scope
2. For each target name, look up in source module's scope
3. Call `moduleScope->defineAs(name_or_alias, sym)` in the importing module

**Whole-module import** (`import Mod`):
1. Iterate source module's symbol map
2. Copy all public symbols into importing module's scope
3. Skip existing names (silent override suppression)
4. Operators are NOT imported (resolved via type, not name)

### Vtable Building

`buildVtable(ClassSymbol*)` runs after all class members are registered:

1. Inherit base class vtable and `allFields` (copy base vectors)
2. Add own fields to `allFields`
3. Slot 0 = destructor (root class: insert; derived: override)
4. For each non-static method:
   - If base has a slot with same name: override → record `vtableIndex`
   - Otherwise: append as new slot

**Note:** Vtable ordering for new methods is alphabetical (from `std::map` iteration), not source declaration order.

### Pass 1 Limitations

- **No forward declarations**: Base class must be defined before derived class.
- **Operator imports**: Whole-module import does not transfer operator overloads.
- **Single constructor/destructor**: Only one per class; multiple definitions silently overwrite.
- **Vtable ordering**: New virtual methods added by derived classes are in alphabetical order.

---

## 4. Pass 2: Type Resolver

### Purpose

Converts all **declaration-level** `TypeNode` AST nodes into concrete `Type` objects. Does NOT enter function bodies.

### Core Resolution

| TypeNode | Resolution |
|----------|-----------|
| `PrimitiveTypeNode` | `TypeRegistry::getPrimitive(kind)` |
| `NamedTypeNode` | Scope lookup → must be `TypeSymbol` → `getUserType(name, kind, sym)` |
| `PointerTypeNode` | Recursive resolve → `getPointerTo` or `getReferenceTo` |
| `ArrayTypeNode` | Recursive resolve → `getArrayOf(elementType, size)` |
| `TupleTypeNode` | Resolve all elements → `getTupleOf(types)` |
| `FunctionTypeNode` | Resolve params + return → `getFunctionType(params, ret)` |

### Reference Parameter Unwrapping

When a parameter is `int& x`:
- `VariableSymbol::type` is set to `int` (the base type)
- `VariableSymbol::isReference = true` carries the reference semantic
- Codegen uses `isReference` to decide pointer vs value passing

### What Pass 2 Does NOT Do

- Does NOT enter function bodies or statements
- Does NOT resolve `var`-declared types (deferred to Pass 3)
- Does NOT resolve expression types

### Pass 2 Limitations

- **No forward reference for types**: Types must be defined before use in type annotations.
- **Array size must be integer literal**: Constant expressions are not evaluated.

---

## 5. Pass 3: Type Checker

### Purpose

The most complex pass. Enters all function bodies and:
- Sets `resolvedType` on every expression
- Resolves identifiers to symbols
- Enforces type compatibility
- Handles type inference for `var`
- Resolves operator overloads
- Enforces access modifiers
- Type-checks lambda expressions

### Scope Navigation

Pass 3 uses a different navigation model. Named scopes (functions, type members) are entered by reference. Anonymous child scopes (blocks, match arms, for loops, lambdas) are entered sequentially via `enterNextChildScope()` using a child index counter.

### Expression Type Rules

| Expression | Result Type |
|-----------|-------------|
| `IntegerLiteral` | `int` |
| `FloatLiteral` | `double` |
| `BoolLiteral` | `bool` |
| `CharLiteral` | `char` |
| `StringLiteral` | `string` |
| `NullLiteral` | `NullType` |
| `IdentifierExpression` | Looked up via scope chain |
| `ThisExpression` | Current class/struct `UserType` |
| `BinaryExpression` | Operator overload check → arithmetic rules → widened type |
| `UnaryExpression` | Negate/Not/BitwiseNot/AddressOf/Dereference |
| `CallExpression` | Return type of called function/constructor |
| `MemberAccessExpression` | Field type, method type, enum value, or string builtin |
| `IndexExpression` | Element type (array/pointer/string/operator[]) |
| `NewExpression` | `pointer_to<T>` |
| `CastExpression` | Target type (validated for legality) |
| `TernaryExpression` | Wider of then/else branches |
| `PipeExpression` | Output type of final stage |
| `MatchExpression` | Wider type across all arm bodies |
| `TupleExpression` | `TupleType` of element types |
| `LambdaExpression` | `FunctionType(params, inferredReturn)` |

### MemberAccessExpression (Most Complex)

Resolves in this priority order:
1. Arrow (`->`) requires and dereferences pointer
2. Dot auto-dereferences one level of pointer
3. String built-in methods (`length`, `charAt`, `substring`)
4. Enum member access
5. Static access (type name + static method)
6. Field access (by name)
7. Method access (by name)
8. Access modifier enforcement (public/private/protected)

### Type Inference for `var`

```
var x = expr;
→ Evaluate expr → get resolvedType → set varSym->type
```

Errors: `var` without initializer, `var = null` (unknown type), `var = voidFunc()`.

### Access Modifier Enforcement

- `public`/none: always accessible
- `private`: only from within the same type
- `protected`: accessible from the same type or subclasses

### Pass 3 Limitations

- **No function overloading**: Only one function per name. Operator overloads are the only overloading.
- **Lambda return inference**: Block-bodied lambdas infer from first `return` statement. Conflicting return types across branches are not cross-checked.
- **No mutability enforcement**: `isMutable` flag exists but is never checked.
- **No definite assignment analysis**: Variables can be read before assignment.
- **No null safety**: Pointers can be dereferenced without null checks.
- **Float literal always `double`**: No `1.0f` suffix.

---

## 6. Pass 4: Semantic Validator

### Purpose

Final pass performing: reachability analysis, RAII tracking, raw block safety, pattern exhaustiveness, lambda capture analysis, and abstract/interface checking.

### 6a: Reachability Analysis

Three-state lattice: `NeverReturns` < `SometimesReturns` < `AlwaysReturns`

| Statement | Classification |
|-----------|---------------|
| `return` | `AlwaysReturns` |
| `if/else` (all branches return) | `AlwaysReturns` |
| `if` (no else) | At best `SometimesReturns` |
| `switch` (all cases + default return) | `AlwaysReturns` |
| `for`/`while` | `NeverReturns` (may not execute) |
| `break`/`continue`/`delete`/`expression` | `NeverReturns` |

Non-void functions must have `AlwaysReturns` reachability. Unreachable statements after a `return` emit warnings.

`break`/`continue` are validated to be inside a loop (`loopDepth_` counter).

### 6b: RAII Tracking

```cpp
// Per-scope RAII info
std::unordered_map<Scope*, ScopeRAIIInfo> raiiInfo_;
// Each entry: vector of {VariableSymbol*, DestructorSymbol*} pairs
```

A variable is tracked for RAII only if:
1. Its type is a `UserType` backed by a `ClassSymbol`
2. The class has a destructor (`hasRAII()` — always true due to auto-generation)
3. Its role is `Local` (not `Field`)

Fields are handled by the class destructor epilogue in codegen. Closure RAII is handled separately by the IRGenerator.

### 6c: Raw Block Safety

Two constructs require `raw { }` context:
- **Pointer arithmetic**: `+`/`-` with a pointer operand
- **Pointer casts**: Different pointer types, or pointer ↔ integer

The `rawDepth_` counter tracks nesting. Error if either construct appears at `rawDepth_ == 0`.

### 6d: Pattern Exhaustiveness

1. If any arm has an unguarded `WildcardPattern` or `BindingPattern` → exhaustive
2. Guarded patterns do NOT count (guard might be false)
3. For enum subjects: check all enum members are covered
4. For non-enum subjects without wildcard → error

### 6e: Lambda Capture Analysis

Populates `LambdaExpression::capturedVariables` and `captureModesResolved`.

Algorithm: When an identifier is visited inside a lambda, walk from innermost to outermost lambda. At each level, if the variable is not locally owned, it must be captured. The capture mode is determined from:
1. Explicit capture list items matching by name
2. Default capture mode (`[=]` or `[&]`)

**Propagation**: If a variable used in an inner lambda must pass through an outer lambda, it is captured at each level in the chain.

**Self-capture detection**: After visiting a `VariableDeclaration` whose initializer is a lambda, checks if the lambda captured the variable being defined. Sets `selfCapture = true` if so.

**Escape analysis**: Lambda literals passed directly as function arguments are marked `escapes = false`. All others default to `true`.

### 6f: Abstract and Interface Checking

- **Abstract methods**: Every non-abstract class must override all abstract vtable slots.
- **Interface methods**: Every non-abstract class must implement all methods from each interface it declares.

### Pass 4 Limitations

- **No escape analysis for non-trivial cases**: Only direct lambda-as-argument is marked non-escaping.
- **Enum exhaustiveness is name-based**: Does not handle numeric or complex patterns.
- **No control flow through loops**: Loops always classified as `NeverReturns`, even if they always return.
- **RAII only for class-typed locals**: Structs with closure fields and function-typed variables are handled by codegen, not Pass 4.
- **No dangling reference detection**: `[&x]` captures that outlive the captured variable are not detected.
- **Capture propagation stops at disallowed boundary**: If an intermediate lambda lacks the right capture mode, propagation halts.

---

## 7. Pass Interaction Summary

```
Pass 1 produces:
  ├── Scope tree (Global → Module → TypeMembers → Function → Block)
  ├── Symbol objects for all declarations
  ├── Vtable slots and allFields for classes
  ├── Auto-generated ctor/dtor for classes
  └── Import aliases in module scopes

Pass 2 reads Pass 1 scopes/symbols, produces:
  ├── VariableSymbol::type for explicitly-typed fields and parameters
  ├── FunctionSymbol::returnType and parameter types
  ├── EnumSymbol::underlyingType
  └── TypeNode::resolvedType at declaration sites

Pass 3 reads Pass 1+2 data, produces:
  ├── ExpressionNode::resolvedType on ALL expressions
  ├── ExpressionNode::resolvedSymbol on identifiers
  ├── VariableSymbol::type for 'var'-inferred variables
  ├── BinaryExpression::isOperatorOverload + resolvedOperatorFunction
  ├── MemberAccessExpression flags
  └── LambdaExpression::resolvedType (FunctionType)

Pass 4 reads Pass 1+2+3 data, produces:
  ├── raiiInfo_ map (scope → RAII variables)
  ├── LambdaExpression::capturedVariables, captureModesResolved
  ├── LambdaExpression::selfCapture, escapes
  └── Control flow / safety / exhaustiveness errors
```

Each pass is strictly additive — no pass modifies data written by a prior pass.
