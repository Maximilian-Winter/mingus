# Mingus V2 Architecture Design

**Date:** February 2026
**Status:** Design specification — not yet implemented
**Scope:** Symbol/Scope/Type hierarchy redesign, AST enrichment, sema/codegen adaptation

---

## Table of Contents

1. [Motivation and Design Goals](#1-motivation-and-design-goals)
2. [V2 Scope System](#2-v2-scope-system)
3. [V2 Symbol Hierarchy](#3-v2-symbol-hierarchy)
4. [V2 Type System](#4-v2-type-system)
5. [V2 AST Node Design](#5-v2-ast-node-design)
6. [V2 Sema Pass Architecture](#6-v2-sema-pass-architecture)
7. [V2 Codegen Interactions](#7-v2-codegen-interactions)
8. [Migration Path](#8-migration-path)
9. [Current Limitations Resolved](#9-current-limitations-resolved)

---

## 1. Motivation and Design Goals

### 1.1 What V1 Got Right

The V1 compiler proves the language works. 51 tests pass, 9 example programs run, closures have reference counting, RAII is solid, virtual dispatch works. The codegen is battle-tested.

### 1.2 Where V1 Struggles

Three HIGH-severity bugs trace to the same root cause: **`FunctionType` carries no per-parameter metadata**.

| Bug | Root Cause | Severity |
|-----|-----------|----------|
| Closures with struct params | Indirect call builds signature from `FunctionType::parameterTypes` via `mapType()` — returns LLVM struct type, not `ptr`. But named functions pass structs by pointer. ABI mismatch. | HIGH |
| Closures with reference params | `isReference` lives on `VariableSymbol`, not on `FunctionType`. After `TypeResolver` unwraps `ReferenceType`, `FunctionType` stores the base type. Indirect call passes `i32`, lambda expects `ptr`. | HIGH |
| Interface parameter passing | Interface fat pointer `{ ptr, ptr }` passed as argument has calling convention mismatch — function expects `ptr`, receives struct. | HIGH |

Beyond the three critical bugs, V1 has structural debt:

| Issue | Impact |
|-------|--------|
| **`ClassType`/`StructType`/`EnumType` are dead** | `fields`/`methods` vectors never populated. All real data lives in symbols. These types exist but carry no useful information. |
| **`UserType` with `void* symbol`** | Every usage site does `static_cast<TypeSymbol*>(user->symbol)`. No type safety at the AST-sema bridge. |
| **Scope not on AST nodes** | Every pass maintains its own `currentScope_` + `childIndexStack_`, synchronized to traversal order. Deviation → silent desync → wrong scope entered. |
| **Positional child-scope indexing** | Block scopes have no direct pointer. All passes navigate via `enterNextChildScope()` using a counter. The `scanForParamSymbols` bug was a direct consequence. |
| **Dual field representation** | `ClassSymbol::fields`/`allFields` in sema, `ClassType::fields` (empty) in types. Codegen ignores type-level fields entirely, re-queries symbols. |
| **`resolveTypeNode` duplicated** | Identical code in TypeResolver.cpp and TypeChecker.cpp. |
| **No `ParameterInfo` on `FunctionType`** | Parameter names, reference flags, and struct-kind hints are lost when building `FunctionType`. |

### 1.3 Design Goals

```
道生一  一生二  二生三  三生萬物
The Dao generates One: SymbolWithScope (dual-nature abstraction)
One generates Two: Symbol (names things) + Scope (contains things)
Two generates Three: Type (describes shape) + Symbol (names) + Scope (nests)
Three generates all features
```

1. **Fix the three HIGH bugs** by enriching `FunctionType` with `ParameterInfo`.
2. **Eliminate `UserType`** — use properly-populated `ClassType`/`StructType`/`EnumType` with back-references to their `TypeSymbol`.
3. **Scope-as-Symbol** — `TypeSymbol` and `FunctionSymbol` ARE their own scopes via `SymbolWithScope` multiple inheritance. `classSym->resolve("field")` works directly.
4. **Scope on AST nodes** — every node carries a `Scope*` reference. No more positional child-index navigation.
5. **Source ranges on all nodes** — `SourceLocation` becomes `SourceRange` (start + end).
6. **Single `resolveTypeNode` path** — extract into a shared utility.

### 1.4 Design Principles

**Principle 1: Types carry enough metadata for codegen.** No re-querying symbols during IR emission. `FunctionType::parameters` has everything needed to build an LLVM function signature.

**Principle 2: The scope tree is the symbol table.** A `ClassSymbol` IS a scope. `resolve("method")` walks the inheritance chain naturally through the scope's `enclosingScope` mechanism.

**Principle 3: AST nodes know their scope.** After Pass 1, every node has a `Scope*`. Later passes never need `childIndexStack_`.

**Principle 4: Types are populated, not shells.** `ClassType` carries its `FieldInfo` list. `InterfaceType` carries its method signatures. Types are the single source of structural truth after sema completes.

---

## 2. V2 Scope System

### 2.1 Abstract Interface

```cpp
// include/mingus/sema/Scope.h

class Scope {
public:
    virtual ~Scope() = default;

    // Core operations
    virtual Symbol* resolve(const std::string& name) const = 0;     // walks up chain
    virtual Symbol* resolveLocal(const std::string& name) const = 0; // this scope only
    virtual bool define(Symbol* symbol) = 0;
    virtual bool defineAs(const std::string& alias, Symbol* symbol) = 0;

    // Hierarchy navigation
    virtual Scope* getEnclosingScope() const = 0;     // parent scope
    virtual std::string getScopeName() const = 0;      // for diagnostics
    virtual ScopeKind getScopeKind() const = 0;

    // Operator overloads (separate namespace)
    virtual OperatorSymbol* resolveOperator(OverloadableOp op) const = 0;

    // Enumeration
    virtual const std::map<std::string, Symbol*>& getAllSymbols() const = 0;
    virtual const std::vector<OperatorSymbol*>& getAllOperators() const = 0;

    // Child scope management
    virtual Scope* createChild(ScopeKind kind, Symbol* owner = nullptr) = 0;
    virtual const std::vector<std::unique_ptr<Scope>>& getChildren() const = 0;
};
```

### 2.2 Concrete Hierarchy

```
Scope (abstract interface)
├── BaseScope : Scope                  (concrete — owns symbol map + children)
│   ├── GlobalScope : BaseScope        (root scope; one per compilation)
│   └── BlockScope : BaseScope         (for { } blocks, for-loops, match arms, raw blocks)
└── SymbolWithScope : BaseScope, Symbol  (dual nature — IS both scope and symbol)
    ├── ModuleScope : SymbolWithScope   (≡ ModuleSymbol)
    ├── TypeScope : SymbolWithScope     (≡ TypeSymbol)
    │   ├── ClassScope                  (≡ ClassSymbol — fields, methods, vtable)
    │   ├── StructScope                 (≡ StructSymbol — fields, methods)
    │   ├── EnumScope                   (≡ EnumSymbol — members)
    │   └── InterfaceScope              (≡ InterfaceSymbol — method signatures)
    └── FunctionScope : SymbolWithScope (≡ FunctionSymbol — params + body)
        ├── MethodScope                 (≡ MethodSymbol)
        ├── ConstructorScope            (≡ ConstructorSymbol)
        ├── DestructorScope             (≡ DestructorSymbol)
        └── LambdaScope                 (≡ anonymous — captures + params)
```

Note: The scope hierarchy merges with the symbol hierarchy (Section 3) through `SymbolWithScope`. The names above are conceptual — in practice, `ClassSymbol` extends `SymbolWithScope` and IS a `ClassScope`.

### 2.3 BaseScope Implementation

```cpp
class BaseScope : public virtual Scope {
protected:
    ScopeKind kind_;
    Scope* enclosingScope_;                      // raw non-owning pointer (parent always outlives child)
    Symbol* ownerSymbol_;                        // the Symbol that created this scope (nullable)
    std::map<std::string, Symbol*> symbols_;     // name → symbol
    std::vector<OperatorSymbol*> operators_;      // separate namespace for overloads
    std::vector<std::unique_ptr<Scope>> children_; // owned child scopes

public:
    Symbol* resolve(const std::string& name) const override {
        auto it = symbols_.find(name);
        if (it != symbols_.end()) return it->second;
        if (enclosingScope_) return enclosingScope_->resolve(name);
        return nullptr;
    }

    Symbol* resolveLocal(const std::string& name) const override {
        auto it = symbols_.find(name);
        return (it != symbols_.end()) ? it->second : nullptr;
    }

    Scope* getEnclosingScope() const override { return enclosingScope_; }
};
```

### 2.4 Name Resolution Through Inheritance

With `ClassSymbol` inheriting from `SymbolWithScope`, name resolution for class members becomes natural:

```cpp
// ClassScope overrides resolve() to walk the inheritance chain:
Symbol* ClassScope::resolve(const std::string& name) const {
    // 1. Check own members
    if (auto* sym = resolveLocal(name)) return sym;
    // 2. Walk base class chain (inheritance lookup)
    if (baseClass_) {
        if (auto* sym = baseClass_->resolve(name)) return sym;
    }
    // 3. Walk enclosing scope (module-level lookup)
    if (getEnclosingScope()) return getEnclosingScope()->resolve(name);
    return nullptr;
}
```

This replaces the current `TypeSymbol::findField()`/`findMethod()` helpers with standard scope resolution. Field lookup, method lookup, and inherited member lookup all use the same mechanism.

### 2.5 Key Design: TypeSymbol IS Its Member Scope

**V1 pattern (two separate objects):**
```cpp
// V1: TypeSymbol has a pointer to its member scope
classSymbol->memberScope->lookupLocal("field");   // indirect
```

**V2 pattern (single object, dual nature):**
```cpp
// V2: ClassSymbol IS a scope
classSymbol->resolve("field");                     // direct
classSymbol->define(fieldSymbol);                  // direct
```

This eliminates the `memberScope` pointer, the `findField()`/`findMethod()`/`findOperator()` helpers, and the constant `static_cast` dance. The type symbol and its member scope are unified.

### 2.6 BlockScope

`BlockScope` extends `BaseScope` for anonymous scopes (if/for/while bodies, match arms, raw blocks). These are the only scopes that remain as separate `Scope` objects — they are not symbols because they have no name.

```cpp
class BlockScope : public BaseScope {
public:
    BlockScope(ScopeKind kind, Scope* enclosing)
        : BaseScope(kind, enclosing, /*owner=*/nullptr) {}
};
```

**V2 improvement**: BlockScopes are still children of their enclosing scope, but they are no longer navigated by position. Instead, each AST `BlockStatement` carries a `Scope*` that points directly to its `BlockScope`. See Section 5.

---

## 3. V2 Symbol Hierarchy

### 3.1 Complete Hierarchy

```
Symbol (abstract)
├── BaseSymbol : Symbol                          (leaf symbols — no scope of their own)
│   └── TypedSymbol : BaseSymbol                 (abstract — carries getType()/setType())
│       └── VariableSymbol : TypedSymbol         (locals, params, fields)
│
└── SymbolWithScope : BaseScope, Symbol          (dual nature: IS both scope and symbol)
    ├── TypeSymbol : SymbolWithScope             (abstract base for type-defining symbols)
    │   ├── ClassSymbol : TypeSymbol             (class decls with vtable, inheritance)
    │   ├── StructSymbol : TypeSymbol            (struct decls with fields, operators)
    │   ├── EnumSymbol : TypeSymbol              (enum decls with members)
    │   └── InterfaceSymbol : TypeSymbol         (interface decls with method signatures)
    ├── FunctionSymbol : SymbolWithScope, TypedSymbol  (functions — IS a scope for its body)
    │   ├── MethodSymbol : FunctionSymbol        (instance methods — hasThisParam)
    │   ├── ConstructorSymbol : FunctionSymbol   (class constructors)
    │   ├── DestructorSymbol : FunctionSymbol    (class destructors)
    │   └── OperatorSymbol : FunctionSymbol      (operator overloads)
    └── ModuleSymbol : SymbolWithScope           (module decls — IS the module scope)
```

### 3.2 Symbol Base Class

```cpp
class Symbol {
public:
    virtual ~Symbol() = default;

    std::string name;
    SymbolKind kind;               // discriminator for safe downcast
    SourceRange location;          // full range: start line/col + end line/col
    Symbol* owner;                 // enclosing symbol (module, class, struct)
    AccessModifier accessLevel;    // Public | Private | Protected

    // Safe downcasting
    template<typename T> T* as();
    template<typename T> const T* as() const;
    template<typename T> bool is() const;
};
```

### 3.3 BaseSymbol and TypedSymbol

```cpp
// Leaf symbols that don't define their own scope
class BaseSymbol : public Symbol {};

// Abstract: symbols that carry a Type
class TypedSymbol : public virtual BaseSymbol {
public:
    virtual TypePtr<Type> getType() const = 0;
    virtual void setType(TypePtr<Type> type) = 0;
};
```

### 3.4 VariableSymbol

```cpp
class VariableSymbol : public TypedSymbol {
public:
    TypePtr<Type> type_;            // resolved in Pass 2 (null until then)
    VariableRole role;              // Local | Parameter | Field
    bool isMutable = true;
    bool isInferred = false;        // declared with 'var'
    bool isInitialized = false;     // has initializer expression
    int fieldIndex = -1;            // GEP index for struct/class fields
    bool isReference = false;       // true for T& parameters

    TypePtr<Type> getType() const override { return type_; }
    void setType(TypePtr<Type> t) override { type_ = t; }
};
```

Unchanged from V1 in substance. The `isReference` flag remains here because it is a property of the *binding site* (this parameter is passed by reference), not the type itself.

### 3.5 SymbolWithScope — The Multiple Inheritance Core

```cpp
// The key abstraction: a symbol that IS also a scope
class SymbolWithScope : public BaseScope, public virtual Symbol {
public:
    // From Symbol: name, kind, location, owner, accessLevel
    // From BaseScope: symbols_, operators_, children_, enclosingScope_

    // Override getScopeName to return the symbol name
    std::string getScopeName() const override { return name; }
};
```

C++ virtual inheritance ensures the `Symbol` base is shared when `FunctionSymbol` inherits from both `SymbolWithScope` and `TypedSymbol`:

```
         Symbol (virtual)
          /         \
   BaseSymbol    SymbolWithScope : BaseScope, Symbol
      |               |
  TypedSymbol    FunctionSymbol : SymbolWithScope, TypedSymbol
```

### 3.6 FunctionSymbol

```cpp
class FunctionSymbol : public SymbolWithScope, public TypedSymbol {
public:
    std::vector<VariableSymbol*> parameters;  // ordered params (owned by this scope)
    TypePtr<Type> returnType;
    bool isMethod = false;
    bool isExtern = false;
    bool isStatic = false;
    bool isAbstract = false;
    bool hasThisParam = false;
    int vtableIndex = -1;                     // >=0 means virtual

    // TypedSymbol interface — returns FunctionType built from params + return
    TypePtr<Type> getType() const override;
    void setType(TypePtr<Type> t) override;

    // SymbolWithScope: this scope holds parameter symbols + body child scopes
    ScopeKind getScopeKind() const override { return ScopeKind::Function; }
};
```

**V1 → V2 change**: `bodyScope` pointer is eliminated. The `FunctionSymbol` IS the scope. `funcSym->resolve("paramName")` works directly. `funcSym->getChildren()` returns the body's child block scopes.

### 3.7 ClassSymbol

```cpp
class ClassSymbol : public TypeSymbol {
public:
    // Inheritance
    ClassSymbol* baseClass = nullptr;
    bool isAbstract = false;

    // Members (all accessible via scope resolution too)
    std::vector<VariableSymbol*> fields;       // own fields only (ordered for layout)
    std::vector<VariableSymbol*> allFields;    // inherited + own (for LLVM GEP layout)
    ConstructorSymbol* constructor = nullptr;
    DestructorSymbol* destructor = nullptr;

    // Virtual dispatch
    std::vector<FunctionSymbol*> vtable;       // slot 0 = destructor
    int vtableSize = 0;

    // Interfaces
    std::vector<InterfaceSymbol*> implementedInterfaces;

    // Type association
    TypePtr<ClassType> classType;              // back-reference to the populated ClassType

    // Convenience
    bool hasRAII() const { return destructor != nullptr; }

    // Scope overrides for inheritance-aware resolution
    Symbol* resolve(const std::string& name) const override;
    ScopeKind getScopeKind() const override { return ScopeKind::TypeMembers; }
};
```

### 3.8 StructSymbol

```cpp
class StructSymbol : public TypeSymbol {
public:
    std::vector<VariableSymbol*> fields;       // ordered for layout
    TypePtr<StructType> structType;            // back-reference to populated StructType
    ScopeKind getScopeKind() const override { return ScopeKind::TypeMembers; }
};
```

### 3.9 EnumSymbol

```cpp
class EnumSymbol : public TypeSymbol {
public:
    struct MemberInfo {
        std::string name;
        int64_t intValue;
        std::string stringValue;
        bool hasExplicitValue;
    };
    std::vector<MemberInfo> members;
    TypePtr<Type> underlyingType;              // int, byte, or string
    TypePtr<EnumType> enumType;                // back-reference to populated EnumType
    ScopeKind getScopeKind() const override { return ScopeKind::TypeMembers; }
};
```

### 3.10 InterfaceSymbol

```cpp
class InterfaceSymbol : public TypeSymbol {
public:
    std::vector<FunctionSymbol*> methods;      // abstract method declarations
    TypePtr<InterfaceType> interfaceType;      // back-reference to populated InterfaceType
    ScopeKind getScopeKind() const override { return ScopeKind::TypeMembers; }
};
```

### 3.11 ModuleSymbol

```cpp
class ModuleSymbol : public SymbolWithScope {
public:
    ScopeKind getScopeKind() const override { return ScopeKind::Module; }
};
```

**V1 → V2 change**: `moduleScope` pointer is eliminated. `ModuleSymbol` IS the module scope.

### 3.12 OperatorSymbol

In V2, `OperatorSymbol` extends `FunctionSymbol` rather than `Symbol` directly. This means it inherits the scope (for its body) and `TypedSymbol` (for its type). Operators are stored in the parent scope's `operators_` list (separate from `symbols_`), preserving the V1 lookup semantics.

```cpp
class OperatorSymbol : public FunctionSymbol {
public:
    OverloadableOp op;             // Plus, Minus, Star, etc.
    Symbol* ownerType;             // the TypeSymbol that declares this operator
};
```

---

## 4. V2 Type System

### 4.1 Design: Eliminate `UserType`, Populate Real Types

V1's `UserType` exists because `ClassType`/`StructType`/`EnumType` were never populated. V2 reverses this: the real type classes carry all structural metadata, and `UserType` is eliminated.

```
V1 path:  NamedTypeNode → UserType(name, void* symbol)  →  cast to ClassSymbol* at usage
V2 path:  NamedTypeNode → ClassType(fields, methods, backRef)  →  classSym via backRef
```

### 4.2 Complete Hierarchy

```
Type (abstract)
├── PrimitiveType              (int, double, float, byte, char, string, bool, void)
├── PointerType                (T*)
├── ReferenceType              (T& — for parameters and captures)
├── ArrayType                  (T[N] or T[])
├── TupleType                  ((T1, T2, ...))
├── FunctionType               (enriched: ParameterInfo per param)
├── ClassType                  (populated: fields, methods, layout, baseClass)
├── StructType                 (populated: fields, methods)
├── EnumType                   (populated: members, underlying type)
├── InterfaceType              (populated: method signatures)
├── ErrorType                  (sentinel for unresolvable types)
└── NullType                   (type of null literal)
```

**Removed**: `UserType`. All places that created `UserType` now create the appropriate `ClassType`/`StructType`/`EnumType`/`InterfaceType`.

### 4.3 ParameterInfo — The Key New Type

```cpp
struct ParameterInfo {
    TypePtr<Type> type;            // the parameter's semantic type
    std::string name;              // parameter name (for diagnostics and debug info)
    bool isReference = false;      // true for T& params
};
```

This is the critical addition that fixes the three HIGH-severity bugs. `ParameterInfo` carries all the metadata that `VariableSymbol` has but that V1's `FunctionType` lacked.

### 4.4 FunctionType (Enriched)

```cpp
class FunctionType : public Type {
public:
    std::vector<ParameterInfo> parameters;   // full metadata per parameter
    TypePtr<Type> returnType;
    bool isVariadic = false;                  // for extern varargs functions

    // Convenience: extract just the types (for compatibility checks)
    std::vector<TypePtr<Type>> getParameterTypes() const;
};
```

**V1 → V2 change**: `parameterTypes` (flat type list) → `parameters` (vector of `ParameterInfo`). This means:

- **Closure indirect calls** can check `param.isReference` to decide `ptr` vs value passing.
- **Closure struct params** can check `param.type->is<ClassType>() || param.type->is<StructType>()` to decide pointer passing.
- **Debug info** can use `param.name` for `DILocalVariable`.
- **Diagnostics** can report "parameter 'x' has type mismatch" instead of "parameter 2".

### 4.5 ClassType (Populated)

```cpp
class ClassType : public Type {
public:
    std::string name;
    std::string module;
    TypeSymbol* symbol;                         // back-reference (typed, not void*)

    // Structural metadata — populated during Pass 2
    std::vector<FieldInfo> fields;              // ALL fields including inherited
    std::vector<MethodInfo> methods;            // ALL methods including inherited
    TypePtr<ClassType> baseClass;               // null for root classes
    bool hasDestructor = false;
    bool isAbstract = false;

    // Layout
    int vtableSize = 0;

    Kind getKind() const override { return Kind::Class; }
    bool equals(const Type& other) const override {
        if (auto* o = other.as<ClassType>()) return symbol == o->symbol;
        return false;
    }
};
```

Where `FieldInfo` and `MethodInfo` are:

```cpp
struct FieldInfo {
    std::string name;
    TypePtr<Type> type;
    AccessModifier access;
    int fieldIndex;                // GEP index in struct layout
    bool isReference = false;      // for reference-typed fields (future)
};

struct MethodInfo {
    std::string name;
    TypePtr<FunctionType> type;    // enriched FunctionType with ParameterInfo
    AccessModifier access;
    bool isStatic;
    bool isAbstract;
    bool isVirtual;
    int vtableIndex;               // -1 = not virtual; >=0 = slot number
};
```

### 4.6 StructType (Populated)

```cpp
class StructType : public Type {
public:
    std::string name;
    TypeSymbol* symbol;                     // back-reference

    std::vector<FieldInfo> fields;          // ordered for layout
    std::vector<MethodInfo> methods;

    Kind getKind() const override { return Kind::Struct; }
    bool equals(const Type& other) const override {
        if (auto* o = other.as<StructType>()) return symbol == o->symbol;
        return false;
    }
};
```

### 4.7 EnumType (Populated)

```cpp
class EnumType : public Type {
public:
    std::string name;
    TypeSymbol* symbol;

    TypePtr<Type> underlyingType;           // int, byte, or string
    struct MemberInfo {
        std::string name;
        int64_t intValue;
        std::string stringValue;
        bool hasExplicitValue;
    };
    std::vector<MemberInfo> members;

    Kind getKind() const override { return Kind::Enum; }
    bool equals(const Type& other) const override {
        if (auto* o = other.as<EnumType>()) return symbol == o->symbol;
        return false;
    }
};
```

### 4.8 InterfaceType (Populated)

```cpp
class InterfaceType : public Type {
public:
    std::string name;
    TypeSymbol* symbol;

    std::vector<MethodInfo> methods;        // abstract method signatures

    Kind getKind() const override { return Kind::Interface; }
    bool equals(const Type& other) const override {
        if (auto* o = other.as<InterfaceType>()) return symbol == o->symbol;
        return false;
    }
};
```

### 4.9 TypeRegistry Changes

The `TypeRegistry` changes:

```
V1:  userTypeCache_: map<void*, TypePtr<UserType>>    →  one cache for all user types
V2:  classTypeCache_: map<ClassSymbol*, TypePtr<ClassType>>
     structTypeCache_: map<StructSymbol*, TypePtr<StructType>>
     enumTypeCache_: map<EnumSymbol*, TypePtr<EnumType>>
     interfaceTypeCache_: map<InterfaceSymbol*, TypePtr<InterfaceType>>
```

Factory methods change:

```
V1:  getUserType(name, kind, void* symbol)
V2:  getClassType(ClassSymbol*)      → ClassType*   (creates + populates from symbol)
     getStructType(StructSymbol*)    → StructType*  (creates + populates from symbol)
     getEnumType(EnumSymbol*)        → EnumType*    (creates + populates from symbol)
     getInterfaceType(InterfaceSymbol*) → InterfaceType*
```

`FunctionType` factory changes:

```
V1:  getFunctionType(TypeList<Type> paramTypes, TypePtr<Type> retType)
V2:  getFunctionType(std::vector<ParameterInfo> params, TypePtr<Type> retType, bool isVariadic = false)
```

### 4.10 `isCompatible()` Updates

Most rules remain identical. Changes:

| Rule | V1 | V2 |
|------|----|----|
| User type name match | `UserType` name + underlyingKind | `ClassType`/`StructType`/`EnumType` pointer identity via `symbol` field |
| Function type match | Pointer equality on cached `FunctionType` | Structural: compare parameter types (ignoring names), return type, and `isVariadic` |
| Null compat | `NullType` → `PointerType` or `FunctionType` | Same (now uses `ClassType`/`StructType` for pointer-to-class checks) |
| Inheritance | Cast `void*` to `ClassSymbol*` | Direct `ClassType::baseClass` traversal or `ClassType::symbol->baseClass` |

### 4.11 ReferenceType — Unchanged

`ReferenceType` remains a transient wrapper. `TypeResolver::resolveParameters()` still unwraps it to the base type on `VariableSymbol::type` and sets `isReference = true`. The difference is that `ParameterInfo::isReference` now preserves this information on `FunctionType`.

---

## 5. V2 AST Node Design

### 5.1 Design Goal

Every AST node carries:
1. **`SourceRange`** — full source range (start line/col, end line/col), replacing V1's point-only `SourceLocation`.
2. **`Scope*`** — pointer to the scope this node lives in, set during Pass 1.

### 5.2 SourceRange

```cpp
struct SourceRange {
    std::string file;
    int startLine;
    int startColumn;
    int endLine;
    int endColumn;

    // Convenience: point location for backward compat
    int line() const { return startLine; }
    int column() const { return startColumn; }
};
```

V1's `SourceLocation` had `file`, `line`, `column`. V2 extends to a full range. This enables:
- Precise error underlines in future error recovery
- Better DIBuilder debug info (function body ranges, not just start points)
- IDE integration (hover-to-find-definition with exact spans)

### 5.3 ASTNode Base Enrichment

```cpp
class ASTNode {
public:
    SourceRange location;                    // V2: full range (was SourceLocation)
    Scope* enclosingScope = nullptr;         // V2: set by Pass 1

    virtual void accept(ASTVisitor&) = 0;
    virtual void accept(ConstASTVisitor&) const = 0;
    template<typename T> T* as();
    template<typename T> bool is() const;
};

class ExpressionNode : public ASTNode {
public:
    TypePtr<Type> resolvedType;              // unchanged: filled by Pass 3
};
```

The `enclosingScope` pointer is set during `SymbolTableBuilder` (Pass 1) for every node. This is the scope that was current when the node was visited.

**Key consequence**: Passes 2–4 and codegen no longer need `childIndexStack_`. To enter the scope of a `BlockStatement`, they read `block->enclosingScope` directly.

### 5.4 Scope Assignment During Pass 1

In `SymbolTableBuilder`, every `visit()` method sets `node.enclosingScope = currentScope_` before doing anything else. For scope-creating nodes:

```cpp
void SymbolTableBuilder::visit(BlockStatement& node) {
    auto* childScope = currentScope_->createChild(ScopeKind::Block);
    node.enclosingScope = childScope;    // the block IS this scope
    pushScope(childScope);
    visitStatements(node.statements);
    popScope();
}

void SymbolTableBuilder::visit(ClassDeclaration& node) {
    auto* classSym = createSymbol<ClassSymbol>(node.name);
    // ClassSymbol IS a scope — no separate scope needed
    node.enclosingScope = classSym;      // the class decl's scope IS the class symbol
    pushScope(classSym);
    // ... visit fields, methods, etc. ...
    popScope();
}
```

For non-scope-creating expression/statement nodes:

```cpp
void SymbolTableBuilder::visit(BinaryExpression& node) {
    node.enclosingScope = currentScope_;
    node.left->accept(*this);
    node.right->accept(*this);
}
```

### 5.5 ParameterNode Enrichment

```cpp
class ParameterNode : public ASTNode {
public:
    std::string name;
    NodePtr<TypeNode> type;
    NodePtr<ExpressionNode> defaultValue;
    bool isReference = false;

    // V2 addition:
    VariableSymbol* resolvedSymbol = nullptr;   // set by Pass 1
};
```

This eliminates the `scanForParamSymbols` hack in lambda codegen. After Pass 1, `param->resolvedSymbol` points directly to the `VariableSymbol*` in the function's scope. Codegen reads it directly.

### 5.6 CallExpression Enrichment

```cpp
class CallExpression : public ExpressionNode {
public:
    NodePtr<ExpressionNode> callee;
    NodeList<ExpressionNode> arguments;

    // V2 additions:
    FunctionSymbol* resolvedCallee = nullptr;   // set by Pass 3 (null for indirect calls)
};
```

When `resolvedCallee` is non-null, codegen uses `buildFunctionType(resolvedCallee)` — the correct path with full parameter metadata. When null (indirect call through closure variable), codegen uses the `FunctionType` from `callee->resolvedType`, which now has `ParameterInfo` — also correct.

### 5.7 MemberAccessExpression Cleanup

```cpp
class MemberAccessExpression : public ExpressionNode {
public:
    NodePtr<ExpressionNode> object;
    std::string memberName;
    bool isArrow;

    // Sema-resolved (V2: streamlined)
    Symbol* resolvedSymbol = nullptr;        // the resolved field or method Symbol
    bool isEnumAccess = false;
    int64_t resolvedEnumValue = 0;
    std::string resolvedEnumStringValue;
    bool isStringEnumAccess = false;
    bool isStringBuiltinMethod = false;
    bool isStaticAccess = false;
};
```

V1 had `FieldInfo* resolvedField` (a pointer into `ClassType::fields`, which was always empty). V2 replaces it with `Symbol* resolvedSymbol`, which can be a `VariableSymbol` (field) or `FunctionSymbol` (method). Codegen reads the field index from `resolvedSymbol->as<VariableSymbol>()->fieldIndex` directly.

### 5.8 Nodes That Are Fine As-Is

Most expression and statement nodes need only the base-class `enclosingScope` addition. No structural changes needed for:

| Node | Reason |
|------|--------|
| `IdentifierExpression` | Already has `resolvedSymbol` — works correctly |
| `BinaryExpression` | Already has `resolvedOperatorFunction` — works correctly |
| `LambdaExpression` | Already has `capturedVariables`/`captureModesResolved` — works correctly |
| `IfStatement`, `ForStatement`, `WhileStatement` | Only need `enclosingScope` on base |
| `ReturnStatement`, `BreakStatement`, `ContinueStatement` | Only need `enclosingScope` |
| `VariableDeclaration` | Already functional; `resolvedSymbol` could be added but is low priority |
| `BlockStatement` | Only needs `enclosingScope` (which IS the block's scope) |

---

## 6. V2 Sema Pass Architecture

### 6.1 Pass 1: SymbolTableBuilder

**Changes:**
- Creates `SymbolWithScope` instances instead of separate Symbol + Scope pairs.
- Sets `node.enclosingScope = currentScope_` on every visited node.
- Sets `param.resolvedSymbol` on every `ParameterNode`.
- Still runs in two sub-passes: 1a (build all modules), 1b (resolve imports).

**Scope creation flow:**

```
V1:                                          V2:
createSymbol<ClassSymbol>("Dog")             createSymbol<ClassSymbol>("Dog")
pushScope(ScopeKind::TypeMembers, sym)       pushScope(classSym)        // classSym IS the scope
sym->memberScope = currentScope_             // no pointer needed — it IS the scope
                                             node.enclosingScope = classSym
visit fields, methods                        visit fields, methods
popScope()                                   popScope()
```

**Import resolution**: Unchanged conceptually. `moduleScope->defineAs(alias, sym)` works the same way because `ModuleSymbol` IS a scope.

**Vtable building**: Unchanged. `buildVtable(ClassSymbol*)` iterates the class's own scope (`classSym->getAllSymbols()`) and the base class's vtable.

### 6.2 Pass 2: TypeResolver

**Changes:**
- Populates `ClassType`/`StructType`/`EnumType`/`InterfaceType` with structural data.
- Builds `ParameterInfo` vectors for `FunctionType`.
- `resolveTypeNode()` extracted into a shared utility class (eliminates duplication with TypeChecker).

**Type population flow:**

After resolving all field types and method signatures in a class, Pass 2 populates the `ClassType`:

```cpp
void TypeResolver::visit(ClassDeclaration& node) {
    auto* classSym = ...; // look up the ClassSymbol
    enterScope(classSym); // classSym IS the scope

    // Visit fields and methods (resolve their types)
    for (auto& field : node.fields) field->accept(*this);
    for (auto& method : node.methods) method->accept(*this);
    if (node.constructor) node.constructor->accept(*this);

    // Populate ClassType from resolved symbol data
    auto classType = registry_.getClassType(classSym);
    classType->fields.clear();
    for (auto* f : classSym->allFields) {
        classType->fields.push_back({f->name, f->type_, f->accessLevel, f->fieldIndex});
    }
    for (auto* m : classSym->vtable) {
        auto fnType = buildFunctionTypeFromSymbol(m);
        classType->methods.push_back({m->name, fnType, m->accessLevel,
                                       m->isStatic, m->isAbstract, true, m->vtableIndex});
    }
    classType->baseClass = classSym->baseClass
        ? registry_.getClassType(classSym->baseClass) : nullptr;
    classType->hasDestructor = classSym->hasRAII();
    classType->vtableSize = classSym->vtableSize;

    classSym->classType = classType;  // back-link

    leaveScope();
}
```

**`buildFunctionTypeFromSymbol`:**

```cpp
TypePtr<FunctionType> buildFunctionTypeFromSymbol(FunctionSymbol* sym) {
    std::vector<ParameterInfo> params;
    for (auto* p : sym->parameters) {
        params.push_back({p->type_, p->name, p->isReference});
    }
    return registry_.getFunctionType(params, sym->returnType, sym->isExtern && isVarargs(sym));
}
```

This is the key change: `FunctionType` is built with full `ParameterInfo` from the `FunctionSymbol`'s resolved parameters.

**Shared `resolveTypeNode`:**

Extract into a standalone utility (or a base class shared by TypeResolver and TypeChecker):

```cpp
class TypeNodeResolver {
protected:
    TypeRegistry& registry_;
    Scope* currentScope_;

    TypePtr<Type> resolveTypeNode(TypeNode* node);
    // Implementation identical to V1's TypeResolver::resolveTypeNode
    // but now shared between Pass 2 and Pass 3
};

class TypeResolver : public ASTVisitor, protected TypeNodeResolver { ... };
class TypeChecker : public ASTVisitor, protected TypeNodeResolver { ... };
```

### 6.3 Pass 3: TypeChecker

**Changes:**
- Uses `node.enclosingScope` instead of `childIndexStack_` for scope navigation.
- Uses enriched `FunctionType` with `ParameterInfo` for argument checking.
- Sets `CallExpression::resolvedCallee` when a direct function call is detected.
- Uses shared `TypeNodeResolver::resolveTypeNode()`.

**Scope navigation simplification:**

```cpp
// V1:
void TypeChecker::visit(BlockStatement& node) {
    enterNextChildScope();      // uses childIndexStack_ — fragile
    visitStatements(node.statements);
    leaveChildScope();
}

// V2:
void TypeChecker::visit(BlockStatement& node) {
    auto* savedScope = currentScope_;
    currentScope_ = node.enclosingScope;    // direct pointer — robust
    visitStatements(node.statements);
    currentScope_ = savedScope;
}
```

No more `childIndexStack_`. No more positional desync risk.

**Argument checking with ParameterInfo:**

```cpp
// V1: type-only check
for (size_t i = 0; i < fnType->parameterTypes.size(); i++) {
    checkAssignability(argTypes[i], fnType->parameterTypes[i], ...);
}

// V2: metadata-aware check
for (size_t i = 0; i < fnType->parameters.size(); i++) {
    auto& param = fnType->parameters[i];
    checkAssignability(argTypes[i], param.type, ...);
    if (param.isReference) {
        // Verify argument is an lvalue (identifier, member access, index)
        if (!isLValue(node.arguments[i].get())) {
            error("argument '" + param.name + "' requires an lvalue for reference parameter");
        }
    }
}
```

### 6.4 Pass 4: SemanticValidator

**Changes:**
- Uses `node.enclosingScope` instead of `childIndexStack_`.
- RAII tracking, capture analysis, escape analysis, exhaustiveness checking — all unchanged in logic.

The capture analysis algorithm (`checkLambdaCapture`) is unchanged. It already works correctly with the scope chain.

---

## 7. V2 Codegen Interactions

### 7.1 `mapType()` Changes

| V1 Type | V1 LLVM Type | V2 Type | V2 LLVM Type |
|---------|-------------|---------|-------------|
| `UserType(Class)` | lookup `structTypeCache_` | `ClassType` | same — `structTypeCache_[classSym]` |
| `UserType(Struct)` | lookup `structTypeCache_` | `StructType` | same |
| `UserType(Enum)` | `mapType(underlying)` | `EnumType` | `mapType(enumType->underlyingType)` |
| `FunctionType` | `{ ptr, ptr }` | `FunctionType` | same `{ ptr, ptr }` |
| `UserType(Interface)` via ptr | `{ ptr, ptr }` | `InterfaceType` via ptr | same `{ ptr, ptr }` |

The `mapType` switch changes from `type->as<UserType>()` + switch on `underlyingKind` to direct `type->is<ClassType>()` / `type->is<StructType>()` / etc.

### 7.2 `mapParamType(ParameterInfo)` — The Key New Helper

```cpp
llvm::Type* IRGenerator::mapParamType(const ParameterInfo& param) {
    if (param.isReference)
        return llvm::PointerType::get(context_, 0);     // ref → ptr
    if (param.type->is<ClassType>() || param.type->is<StructType>())
        return llvm::PointerType::get(context_, 0);     // struct → ptr (by-pointer convention)
    if (param.type->is<InterfaceType>())
        return getFatPtrType();                          // interface → { ptr, ptr }
    return mapType(param.type.get());                    // everything else: direct mapping
}
```

This single function replaces ALL ad-hoc parameter type decision logic in V1's codegen:
- `buildFunctionType` had manual `param->isReference` checks
- The indirect call path had no checks at all (root cause of the bugs)
- Interface dispatch had its own logic

### 7.3 Lambda Codegen — Fixed

**V1 problem**: Lambda function signature built from `FunctionType::parameterTypes` via `mapType()`.

**V2 fix**: Lambda function signature built from `FunctionType::parameters` via `mapParamType()`:

```cpp
void IRGenerator::visit(LambdaExpression& node) {
    auto* fnType = node.resolvedType->as<FunctionType>();

    // Build LLVM function type from enriched FunctionType
    llvm::Type* retTy = mapType(fnType->returnType.get());
    std::vector<llvm::Type*> paramTypes;
    for (auto& param : fnType->parameters) {
        paramTypes.push_back(mapParamType(param));    // V2: uses ParameterInfo
    }
    paramTypes.push_back(ptrTy);  // env pointer

    auto* llvmFnType = llvm::FunctionType::get(retTy, paramTypes, false);
    // ... rest of lambda codegen unchanged ...
}
```

### 7.4 Indirect Call Path — Fixed

```cpp
// V1 (broken for struct/ref params):
for (auto& pt : funcType->parameterTypes) {
    paramTypes.push_back(mapType(pt.get()));    // no ref/struct awareness
}

// V2 (correct):
for (auto& param : funcType->parameters) {
    paramTypes.push_back(mapParamType(param));   // full awareness
}
```

### 7.5 Interface Dispatch — Fixed

When passing an interface fat pointer as a function argument:

```cpp
// V2: mapParamType handles InterfaceType
// param.type->is<InterfaceType>() → getFatPtrType()
// The caller passes the { ptr, ptr } fat pointer, the callee receives it as { ptr, ptr }
```

### 7.6 Scope Navigation — Simplified

```cpp
// V1:
void IRGenerator::visit(BlockStatement& node) {
    enterNextChildScope();     // childIndexStack_ based
    // ...
    leaveChildScope();
}

// V2:
void IRGenerator::visit(BlockStatement& node) {
    auto* savedScope = currentScope_;
    currentScope_ = node.enclosingScope;   // direct
    // ...
    currentScope_ = savedScope;
}
```

### 7.7 Lambda Parameter Mapping — Simplified

```cpp
// V1: scanForParamSymbols — manual AST walk matching parameter names to IdentifierExpression nodes
// V2: direct ParameterNode::resolvedSymbol

void IRGenerator::visit(LambdaExpression& node) {
    // ...
    for (size_t i = 0; i < node.parameters.size(); i++) {
        auto* paramSym = node.parameters[i]->resolvedSymbol;
        // Map directly to the LLVM arg:
        namedValues_[paramSym] = llvmFunc->getArg(i);    // or create alloca as needed
    }
    // No more scanForParamSymbols!
}
```

### 7.8 Field Access — Simplified

```cpp
// V1:
auto* typeSym = static_cast<TypeSymbol*>(static_cast<UserType*>(type)->symbol);
auto* fieldSym = typeSym->findField(memberName);
int gepIdx = getFieldGEPIndex(classSym, fieldSym);  // O(n) linear scan

// V2:
auto* fieldSym = node.resolvedSymbol->as<VariableSymbol>();
int gepIdx = fieldSym->fieldIndex;                   // direct: O(1)
// (add vtable offset if classSym->vtableSize > 0)
```

---

## 8. Migration Path

### 8.1 Phase Dependencies

```
Phase A: Scope + Symbol hierarchy (SymbolWithScope MI)
    │
    ├──► Phase B: Type enrichment (FunctionType params, populate ClassType/StructType)
    │       │
    │       └──► Phase D: Sema updates (adapt 4 passes)
    │               │
    │               └──► Phase E: Codegen fixes (mapParamType, indirect calls)
    │                       │
    │                       └──► Phase F: New tests
    │
    └──► Phase C: AST enrichment (scope refs, SourceRange, ParameterNode::resolvedSymbol)
            │
            └──► Phase D: Sema updates (uses scope refs)
```

Phases A, B, and C can be developed in parallel to some extent, but they all feed into Phase D.

### 8.2 Phase A: Symbol/Scope Hierarchy

**Goal**: Implement the `SymbolWithScope` multiple inheritance pattern.

**Files to modify:**
- `include/mingus/sema/Symbol.h` — restructure hierarchy
- `include/mingus/sema/Scope.h` — extract `Scope` interface, `BaseScope`, `BlockScope`
- `src/mingus/sema/Scope.cpp` — implementations

**Steps:**
1. Define `Scope` as an abstract interface with pure virtual methods.
2. Implement `BaseScope` with the concrete symbol map, operator list, and children.
3. Define `SymbolWithScope` extending both `BaseScope` and `Symbol` (virtual inheritance).
4. Restructure `ClassSymbol`, `StructSymbol`, `EnumSymbol`, `InterfaceSymbol`, `FunctionSymbol`, `ModuleSymbol` to extend `SymbolWithScope` (or its `TypeSymbol` subclass).
5. Remove `memberScope`, `bodyScope`, `moduleScope` pointer fields.
6. Update `ClassSymbol::resolve()` to walk the base class chain.

**Verification**: All existing `TypeSymbol::findField()`, `findMethod()`, `findOperator()` calls should still compile by routing through `resolve()`/`resolveLocal()` or by providing compatibility shims during migration.

**Risk**: This is the most structurally invasive change. Every file that references `memberScope` or `bodyScope` must be updated. Recommend implementing behind a feature flag or on a branch with incremental testing.

### 8.3 Phase B: Type Enrichment

**Goal**: `FunctionType` carries `ParameterInfo`; `ClassType`/`StructType`/`EnumType`/`InterfaceType` are populated; `UserType` is eliminated.

**Files to modify:**
- `include/mingus/ast/Type.h` — add `ParameterInfo`, populate type classes, remove `UserType`
- `src/mingus/sema/TypeRegistry.cpp` — new factory methods, split caches
- `include/mingus/sema/TypeRegistry.h` — new API

**Steps:**
1. Add `ParameterInfo` struct to `Type.h`.
2. Change `FunctionType::parameterTypes` → `FunctionType::parameters` (vector of `ParameterInfo`).
3. Add `TypeSymbol* symbol` field to `ClassType`, `StructType`, `EnumType`, `InterfaceType`.
4. Change `ClassType`/`StructType` `equals()` to use `symbol` pointer identity.
5. Add factory methods `getClassType(ClassSymbol*)`, etc. to `TypeRegistry`.
6. Update `isCompatible()` to handle new type classes directly.
7. Remove `UserType` class. Find and replace all `UserType` references.

**Verification**: TypeRegistry unit tests (if any) or the full test suite. The tricky part is `isCompatible` — must verify all 11 compatibility rules still work.

**Risk**: `UserType` removal touches every file that does `type->as<UserType>()`. This is a sweeping search-and-replace, but each site's migration is mechanical: replace `userType->symbol` with `classType->symbol` (or `structType->symbol`, etc.).

### 8.4 Phase C: AST Enrichment

**Goal**: Every node has `Scope*`, `SourceRange`, `ParameterNode::resolvedSymbol`.

**Files to modify:**
- `include/mingus/ast/ASTNode.h` — `SourceRange`, `enclosingScope`
- `include/mingus/ast/Declarations.h` — `ParameterNode::resolvedSymbol`
- `include/mingus/ast/Expressions.h` — `CallExpression::resolvedCallee`, `MemberAccessExpression::resolvedSymbol`
- `src/mingus/parser/ASTGenerator.cpp` — populate `SourceRange` (start + end from ANTLR tokens)

**Steps:**
1. Replace `SourceLocation` with `SourceRange` on `ASTNode`.
2. Add `Scope* enclosingScope = nullptr` to `ASTNode`.
3. Add `VariableSymbol* resolvedSymbol` to `ParameterNode`.
4. Add `FunctionSymbol* resolvedCallee` to `CallExpression`.
5. Change `MemberAccessExpression::resolvedField` (was `FieldInfo*`) to `Symbol* resolvedSymbol`.
6. Update `ASTGenerator` to set `SourceRange` from ANTLR `ParserRuleContext::start` / `stop` tokens.

**Verification**: All existing tests should still pass since the new fields default to `nullptr` / zero-initialized. The fields become populated only after Pass 1 (scope) and Pass 3 (resolved symbols) are updated in Phase D.

**Risk**: Low — purely additive changes. The `SourceLocation` → `SourceRange` rename requires updating all constructor call sites in `ASTGenerator`.

### 8.5 Phase D: Sema Updates

**Goal**: All 4 passes adapted to use the new hierarchy.

**Files to modify:**
- `src/mingus/sema/SymbolTableBuilder.cpp` — create `SymbolWithScope` instances, set `node.enclosingScope`, set `param.resolvedSymbol`
- `src/mingus/sema/TypeResolver.cpp` — populate `ClassType`/`StructType` from symbols, build `FunctionType` with `ParameterInfo`, extract shared `resolveTypeNode`
- `src/mingus/sema/TypeChecker.cpp` — use `node.enclosingScope` instead of `childIndexStack_`, set `CallExpression::resolvedCallee`, use shared `resolveTypeNode`
- `src/mingus/sema/SemanticValidator.cpp` — use `node.enclosingScope` instead of `childIndexStack_`

**Steps:**
1. **SymbolTableBuilder**: Replace all `pushScope(ScopeKind, sym)` with scope creation through `SymbolWithScope`. Assign `node.enclosingScope` in every visitor. Set `ParameterNode::resolvedSymbol` when creating parameter symbols.
2. **TypeResolver**: After resolving all members of a class/struct, populate the corresponding `ClassType`/`StructType` via the registry factory. Build `FunctionType` with `ParameterInfo` vectors. Extract `resolveTypeNode` into `TypeNodeResolver` base class.
3. **TypeChecker**: Remove `childIndexStack_` and all `enterNextChildScope`/`leaveChildScope`. Replace with `currentScope_ = node.enclosingScope` pattern. Set `CallExpression::resolvedCallee` in `visit(CallExpression&)` when the callee resolves to a named function. Inherit `TypeNodeResolver` for shared type resolution.
4. **SemanticValidator**: Same `childIndexStack_` removal. Lambda capture analysis logic is unchanged.

**Verification**: All 51 tests must pass after this phase. Run the full suite after each sub-step.

**Risk**: Medium-high. The `childIndexStack_` removal must be done atomically with the scope-on-AST-node addition — if any node lacks its `enclosingScope`, the pass will crash.

### 8.6 Phase E: Codegen Fixes

**Goal**: IRGenerator uses `mapParamType(ParameterInfo)`, fixes the three HIGH bugs.

**Files to modify:**
- `include/mingus/codegen/IRGenerator.h` — add `mapParamType` declaration, remove `childIndexStack_`
- `src/mingus/codegen/IRGenerator.cpp` — implement `mapParamType`, update `mapType` for new type classes, update lambda codegen, update indirect call path, remove `scanForParamSymbols`, remove `childIndexStack_` navigation

**Steps:**
1. Implement `mapParamType(ParameterInfo)` as described in Section 7.2.
2. Update `mapType()` to handle `ClassType`/`StructType`/`EnumType`/`InterfaceType` directly (no more `UserType` switch).
3. Update lambda codegen to use `FunctionType::parameters` + `mapParamType`.
4. Update indirect call path to use `FunctionType::parameters` + `mapParamType`.
5. Update interface dispatch to use enriched `InterfaceType`.
6. Remove `scanForParamSymbols` — use `ParameterNode::resolvedSymbol` instead.
7. Remove `childIndexStack_` — use `node.enclosingScope` instead.
8. Update `isUserStructKind()` helper to check `ClassType`/`StructType` directly.

**Verification**: All 51 existing tests pass. Then add new tests for the previously-broken patterns (Phase F).

### 8.7 Phase F: New Tests

**Goal**: Verify the three HIGH bugs are fixed.

**New test files:**

| Test | What It Tests |
|------|---------------|
| `test_31_closure_struct_param` | Lambda taking a struct parameter, called through fat pointer. `var f = [=](Vec2 v) => { return v.x + v.y; }; f(myVec);` |
| `test_32_closure_ref_param` | Lambda taking a reference parameter. `var f = [=](int& x) => { x = x + 1; }; f(myVar);` |
| `test_33_interface_param` | Passing an interface fat pointer as a function argument. `func render(Drawable* d) => void { d->draw(); }` called with polymorphic interface value. |

These tests would crash or produce LLVM verification errors in V1. They should pass in V2.

---

## 9. Current Limitations Resolved

### 9.1 HIGH-Severity Bugs

| Limitation | V1 Root Cause | V2 Resolution |
|-----------|--------------|---------------|
| **Closures with struct params** | Indirect call uses `mapType(StructType)` → LLVM struct type. Lambda expects `ptr`. ABI mismatch. | `mapParamType()` checks `param.type->is<StructType>()` → returns `ptr`. Indirect call uses `FunctionType::parameters` with `mapParamType`. |
| **Closures with reference params** | `isReference` on `VariableSymbol`, not on `FunctionType`. Indirect call passes `i32`, lambda expects `ptr`. | `ParameterInfo::isReference` on `FunctionType`. `mapParamType()` checks `param.isReference` → returns `ptr`. |
| **Interface parameter passing** | Interface fat pointer `{ ptr, ptr }` passed as arg, function expects `ptr`. | `mapParamType()` checks `param.type->is<InterfaceType>()` → returns `getFatPtrType()` i.e. `{ ptr, ptr }`. |

### 9.2 MEDIUM-Severity Issues

| Limitation | V2 Resolution |
|-----------|---------------|
| **`UserType` with `void*` symbol** | Eliminated. `ClassType`/`StructType`/`EnumType`/`InterfaceType` carry `TypeSymbol* symbol` (typed pointer). |
| **Positional child-scope indexing** | Eliminated. All nodes carry `Scope* enclosingScope`. No more `childIndexStack_`. |
| **`resolveTypeNode` duplicated** | Extracted into shared `TypeNodeResolver` base class used by both `TypeResolver` and `TypeChecker`. |
| **No `isVariadic` on `FunctionType`** | Added `bool isVariadic` field. `printf`/`snprintf` no longer need name-based special-casing. |
| **`FieldInfo*` dangling pointer risk** | Replaced with `Symbol* resolvedSymbol` on `MemberAccessExpression`. Symbols are lifetime-stable in the `SymbolTable`. |

### 9.3 Structural Improvements

| Issue | V2 Resolution |
|-------|---------------|
| **Dead `ClassType::fields`/`methods`** | Now populated during Pass 2. Single source of structural truth. |
| **Dual field representation** | `ClassSymbol::allFields` (layout) and `ClassType::fields` (metadata) are populated from the same source. Codegen uses `ClassType::fields` for metadata, `ClassSymbol::allFields` for GEP indices. |
| **`scanForParamSymbols`** | Eliminated. `ParameterNode::resolvedSymbol` set during Pass 1. |
| **`getFieldGEPIndex` O(n) scan** | Can use `VariableSymbol::fieldIndex` directly (O(1)). Vtable offset still added separately. |
| **No scope on AST nodes** | Every node has `Scope* enclosingScope`. |
| **Point-only source locations** | `SourceRange` with start/end line/column. |

### 9.4 Limitations NOT Resolved by V2

These require separate efforts beyond the scope of this architecture redesign:

| Limitation | Why Not V2 |
|-----------|------------|
| No generics/templates | Requires monomorphization or type erasure — orthogonal to hierarchy redesign |
| No function overloading | Requires name mangling changes — can layer on top of V2 |
| No error recovery | Parser-level work — orthogonal |
| No definite assignment | Requires data flow analysis pass — orthogonal |
| No move semantics | Requires ownership model — orthogonal |
| No copy constructors | Requires deep-copy codegen — orthogonal |
| Reference cycle detection | Requires GC or weak-ref support — orthogonal |
| No labeled break/continue | Grammar + codegen change — orthogonal |
| No const modifier | Requires immutability tracking — can layer on V2's `isMutable` |
| Duplicate cross-module externs | Requires linker-level dedup — orthogonal |

---

## Appendix A: File Impact Matrix

Every file that touches Symbol, Scope, or Type and must be modified:

| File | Phase | Changes |
|------|-------|---------|
| `include/mingus/sema/Symbol.h` | A | Full hierarchy restructure |
| `include/mingus/sema/Scope.h` | A | Extract interface, BaseScope, BlockScope |
| `src/mingus/sema/Scope.cpp` | A | Implement new scope classes |
| `include/mingus/sema/SymbolTable.h` | A | Owns SymbolWithScope instances |
| `include/mingus/ast/Type.h` | B | ParameterInfo, populate ClassType/StructType, remove UserType |
| `include/mingus/sema/TypeRegistry.h` | B | New factory methods, split caches |
| `src/mingus/sema/TypeRegistry.cpp` | B | isCompatible, new factories, remove getUserType |
| `include/mingus/ast/ASTNode.h` | C | SourceRange, enclosingScope |
| `include/mingus/ast/Declarations.h` | C | ParameterNode::resolvedSymbol |
| `include/mingus/ast/Expressions.h` | C | CallExpression::resolvedCallee, MemberAccess::resolvedSymbol |
| `src/mingus/parser/ASTGenerator.cpp` | C | SourceRange from ANTLR tokens |
| `src/mingus/sema/SymbolTableBuilder.cpp` | D | SymbolWithScope creation, scope assignment, param resolution |
| `include/mingus/sema/SymbolTableBuilder.h` | D | Updated method signatures |
| `src/mingus/sema/TypeResolver.cpp` | D | Populate types, ParameterInfo, shared TypeNodeResolver |
| `include/mingus/sema/TypeResolver.h` | D | Inherit TypeNodeResolver |
| `src/mingus/sema/TypeChecker.cpp` | D | Remove childIndexStack_, use enclosingScope, set resolvedCallee |
| `include/mingus/sema/TypeChecker.h` | D | Remove childIndexStack_, inherit TypeNodeResolver |
| `src/mingus/sema/SemanticValidator.cpp` | D | Remove childIndexStack_, use enclosingScope |
| `include/mingus/sema/SemanticValidator.h` | D | Remove childIndexStack_ |
| `include/mingus/codegen/IRGenerator.h` | E | Add mapParamType, remove childIndexStack_, scanForParamSymbols |
| `src/mingus/codegen/IRGenerator.cpp` | E | mapParamType, mapType update, lambda fix, indirect call fix |

**Total: 22 files modified across 6 phases.**

---

## Appendix B: V1 → V2 Migration Cheat Sheet

### Common Patterns

```cpp
// V1: Access class member scope
classSym->memberScope->lookupLocal("field")
// V2: ClassSymbol IS the scope
classSym->resolveLocal("field")

// V1: Access function body scope
funcSym->bodyScope->lookupLocal("param")
// V2: FunctionSymbol IS the scope
funcSym->resolveLocal("param")

// V1: Access module scope
moduleSym->moduleScope->lookup("func")
// V2: ModuleSymbol IS the scope
moduleSym->resolve("func")

// V1: Get UserType from TypeSymbol
registry_.getUserType(sym->name, kind, sym)
// V2: Get specific type
registry_.getClassType(classSym)
registry_.getStructType(structSym)

// V1: Cast void* to get symbol from type
static_cast<ClassSymbol*>(static_cast<UserType*>(type)->symbol)
// V2: Typed back-reference
type->as<ClassType>()->symbol

// V1: FunctionType parameter access
fnType->parameterTypes[i]
// V2: ParameterInfo access
fnType->parameters[i].type
fnType->parameters[i].isReference
fnType->parameters[i].name

// V1: Navigate to child scope
enterNextChildScope();   // uses childIndexStack_
// V2: Direct scope pointer
currentScope_ = node.enclosingScope;

// V1: scanForParamSymbols (manual AST walk for lambda params)
// V2: Direct access
node.parameters[i]->resolvedSymbol
```

---

*碼道無形，生育萬程。此設計既成，V2之路已明。*
*The Way of Code is formless, giving birth to all programs. This design is complete — the path to V2 is clear.*
