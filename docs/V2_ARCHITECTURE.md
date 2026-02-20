# Mingus V2 Architecture Design

**Date:** February 2026
**Status:** Design specification — clean rewrite, no backward compatibility
**Approach:** Modeled on the proven `SymbolWithScope` multiple-inheritance pattern

---

## Table of Contents

1. [Design Goals](#1-design-goals)
2. [Symbol and Scope Hierarchy](#2-symbol-and-scope-hierarchy)
3. [TypeSymbol Unification](#3-typesymbol-unification)
4. [AST Node Design](#4-ast-node-design)
5. [Sema Passes](#5-sema-passes)
6. [LLVM IR Codegen Mapping](#6-llvm-ir-codegen-mapping)
7. [What This Fixes](#7-what-this-fixes)

---

## 1. Design Goals

This is a clean rewrite. The language is five days old — no backward compatibility needed. The goal is a principled foundation that supports future features (generics, overloading, const, error recovery) without another rewrite.

**Core insight from the old compiler:** Don't maintain two parallel hierarchies (Symbol + Type). Instead, `TypeSymbol` IS the type. `VariableSymbol::getType()` returns a `TypeSymbol*`. The scope tree IS the symbol table. A `ClassSymbol` IS its own member scope.

**What must work for LLVM IR:**
- `mapType(TypeSymbol*)` → single entry point for all LLVM type mapping
- `FunctionSymbol::Parameters` carries full metadata (name, type, isReference) — indirect calls and closures read this directly
- Fat pointer `{ ptr, ptr }` representation for closures and interfaces — unchanged
- RAII scope stack, vtable dispatch, capture analysis — unchanged in logic

---

## 2. Symbol and Scope Hierarchy

### 2.1 Scope — Abstract Interface

```cpp
// Scope.h

class Scope {
public:
    virtual ~Scope() = default;

    virtual std::shared_ptr<Symbol> resolve(const std::string& name) = 0;
    virtual void define(const std::shared_ptr<Symbol>& sym) = 0;
    virtual std::vector<std::shared_ptr<Symbol>> getAllSymbols() const = 0;
    virtual std::shared_ptr<Scope> getEnclosingScope() const = 0;
    virtual void setEnclosingScope(const std::shared_ptr<Scope>& value) = 0;
    virtual std::vector<std::shared_ptr<Scope>> getEnclosingPathToRoot() = 0;
    virtual std::string getName() const = 0;
    virtual void nest(const std::shared_ptr<Scope>& scope) = 0;
};
```

### 2.2 Symbol — Abstract Interface

```cpp
// Symbol.h

class Symbol {
public:
    virtual ~Symbol() = default;

    virtual int getInsertionOrderNumber() const = 0;
    virtual void setInsertionOrderNumber(int value) = 0;
    virtual std::string getName() const = 0;
    virtual std::shared_ptr<Scope> getSymbolScope() const = 0;
    virtual void setSymbolScope(const std::shared_ptr<Scope>& value) = 0;

    // Every place in the source where this symbol is referenced
    std::vector<std::shared_ptr<AstBaseNode>> SymbolUsageNodes;
};
```

The `SymbolUsageNodes` list tracks every AST node that references this symbol. This enables IDE features (find-all-references, rename) and diagnostics.

### 2.3 BaseScope — Concrete Scope Implementation

```cpp
// BaseScope.h

class BaseScope : public Scope, public std::enable_shared_from_this<BaseScope> {
public:
    std::shared_ptr<Symbol> resolve(const std::string& name) override;
    void define(const std::shared_ptr<Symbol>& sym) override;
    std::vector<std::shared_ptr<Symbol>> getAllSymbols() const override;
    std::shared_ptr<Scope> getEnclosingScope() const override;
    void setEnclosingScope(const std::shared_ptr<Scope>& value) override;
    std::vector<std::shared_ptr<Scope>> getEnclosingPathToRoot() override;
    void nest(const std::shared_ptr<Scope>& scope) override;
    virtual std::string ToString();

    // Operator overloads — separate namespace
    void defineOperator(const std::shared_ptr<OperatorSymbol>& op);
    std::shared_ptr<OperatorSymbol> resolveOperator(OverloadableOp op) const;
    const std::vector<std::shared_ptr<OperatorSymbol>>& getAllOperators() const;

protected:
    BaseScope() = default;
    BaseScope(std::shared_ptr<Scope> enclosingScope);

    std::unordered_map<std::string, std::shared_ptr<Symbol>> symbols;
    std::shared_ptr<Scope> enclosingScope;
    std::vector<std::shared_ptr<Scope>> nestedScopesNotSymbols;  // block scopes
    std::vector<std::shared_ptr<OperatorSymbol>> operators;
};
```

The `resolve()` implementation walks up through `enclosingScope`:

```cpp
std::shared_ptr<Symbol> BaseScope::resolve(const std::string& name) {
    auto it = symbols.find(name);
    if (it != symbols.end()) return it->second;
    if (enclosingScope) return enclosingScope->resolve(name);
    return nullptr;
}
```

### 2.4 BaseSymbol — Leaf Symbols (No Scope)

```cpp
// BaseSymbol.h

class BaseSymbol : public Symbol {
public:
    BaseSymbol(std::string name);

    std::string getName() const override;
    std::shared_ptr<Scope> getSymbolScope() const override;
    void setSymbolScope(const std::shared_ptr<Scope>& value) override;
    int getInsertionOrderNumber() const override;
    void setInsertionOrderNumber(int value) override;

    // The defining AST node for this symbol
    void setDefBaseNode(const std::shared_ptr<AstBaseNode>& value);
    std::shared_ptr<AstBaseNode> getDefBaseNode() const;

    int lexicalOrder;
    std::shared_ptr<AstBaseNode> defBaseNode;
    const std::string name;
    std::shared_ptr<Scope> scope;    // the scope this symbol was defined IN
};
```

### 2.5 SymbolWithScope — The Core Dual-Nature Abstraction

```cpp
// SymbolWithScope.h

class SymbolWithScope : public BaseScope, public Symbol {
public:
    SymbolWithScope(std::string name);

    // From Symbol:
    std::string getName() const override;
    std::shared_ptr<Scope> getSymbolScope() const override;
    void setSymbolScope(const std::shared_ptr<Scope>& value) override;
    int getInsertionOrderNumber() const override;
    void setInsertionOrderNumber(int value) override;

    // From Scope (via BaseScope):
    std::shared_ptr<Scope> getEnclosingScope() const override;

    // Qualified name support for diagnostics and name mangling
    virtual std::string getQualifiedName() const;
    virtual std::string getFullyQualifiedName(const std::string& scopePathSeparator);

    bool IsExternal = false;
    int index;
    const std::string name;
};
```

**This is the key pattern.** A `SymbolWithScope` is simultaneously:
- A **Symbol** (it has a name, lives in some enclosing scope)
- A **Scope** (it contains other symbols, can be resolved into)

A `ClassSymbol` IS a `SymbolWithScope`. You call `classSym->resolve("field")` to find a field. You call `classSym->define(fieldSym)` to register a field. The class symbol and its member scope are the same object.

### 2.6 Complete Hierarchy

```
Symbol (abstract)
├── BaseSymbol : Symbol                              (leaf — no scope of its own)
│   └── TypedSymbol : BaseSymbol                     (abstract — getType()/setType())
│       └── VariableSymbol : TypedSymbol             (locals, params, fields)
│
└── SymbolWithScope : BaseScope, Symbol              (dual nature: IS both)
    ├── TypeSymbol : SymbolWithScope                 (all type-defining symbols)
    │   ├── PrimitiveTypeSymbol : TypeSymbol         (int, double, float, byte, char, string, bool, void)
    │   ├── PointerTypeSymbol : TypeSymbol           (T*)
    │   ├── ArrayTypeSymbol : TypeSymbol             (T[N])
    │   ├── TupleTypeSymbol : TypeSymbol             ((T1, T2, ...))
    │   ├── FunctionTypeSymbol : TypeSymbol          ((T1) => R — closure/function pointer types)
    │   ├── ReferenceTypeSymbol : TypeSymbol         (T& — transient, used during resolution)
    │   ├── ClassSymbol : TypeSymbol                 (class decls with vtable, inheritance)
    │   ├── StructSymbol : TypeSymbol                (struct decls with fields, operators)
    │   ├── EnumSymbol : TypeSymbol                  (enum decls with members)
    │   └── InterfaceSymbol : TypeSymbol             (interface decls with method signatures)
    ├── FunctionSymbol : SymbolWithScope, TypedSymbol (functions — IS scope for body)
    │   ├── MethodSymbol : FunctionSymbol            (instance methods)
    │   ├── ConstructorSymbol : FunctionSymbol       (class constructors)
    │   ├── DestructorSymbol : FunctionSymbol        (class destructors)
    │   └── OperatorSymbol : FunctionSymbol          (operator overloads)
    └── ModuleSymbol : SymbolWithScope               (modules — IS the module scope)

Scope (abstract)
├── BaseScope : Scope                                (concrete map + children)
│   ├── GlobalScope : BaseScope                      (root; one per compilation)
│   └── BlockScope : BaseScope                       (anonymous { } blocks, for, match arms)
└── SymbolWithScope : BaseScope, Symbol              (see above)
```

### 2.7 GlobalScope

```cpp
class GlobalScope : public BaseScope {
public:
    GlobalScope();
    std::string getName() const override;
    void DefineModule(const std::shared_ptr<ModuleSymbol>& moduleSymbol);
};
```

### 2.8 SymbolTable — Owns Everything

```cpp
class SymbolTable : public std::enable_shared_from_this<SymbolTable> {
public:
    SymbolTable();

    std::shared_ptr<GlobalScope> getRootScope() const;
    std::shared_ptr<Scope> getCurrentScope() const;
    void setCurrentScope(const std::shared_ptr<Scope>& value);
    void DefineSymbol(const std::shared_ptr<Symbol>& symbol);
    void PopScope();
    void PushScope(const std::shared_ptr<Scope>& s);

    std::shared_ptr<GlobalScope> RootScope;
    std::shared_ptr<Scope> CurrentScope;

    // Type interning: canonical instances of all types
    std::unordered_map<std::string, std::shared_ptr<TypeSymbol>> Types;
};
```

The `Types` map serves as the type registry. Primitives are pre-registered. Compound types (pointer, array, tuple, function types) are interned by their string key. User-defined types (class, struct, enum, interface) are registered when declared.

---

## 3. TypeSymbol Unification

### 3.1 Design: TypeSymbol IS the Type

In V1, there were two parallel hierarchies:
- **Symbol hierarchy** (`ClassSymbol`, `StructSymbol`, etc.) — held real structural data
- **Type hierarchy** (`ClassType`, `StructType`, `UserType`, etc.) — mostly dead shells

In V2, there is **one hierarchy**. `TypeSymbol` IS the type. `VariableSymbol::getType()` returns a `std::shared_ptr<TypeSymbol>`.

```cpp
// TypedSymbol.h — abstract base for symbols that have a type

class TypedSymbol : public BaseSymbol {
public:
    TypedSymbol(const std::string& name);
    virtual std::shared_ptr<TypeSymbol> getType() const = 0;
    virtual void setType(const std::shared_ptr<TypeSymbol>& value) = 0;
};
```

### 3.2 TypeSymbol — Base for All Types

```cpp
// TypeSymbol.h

class TypeSymbol : public SymbolWithScope {
public:
    TypeSymbol(const std::string& name, bool isPrimaryType);

    bool IsPrimaryType;       // true for int, double, float, byte, char, string, bool, void
    int SizeInBytes = 0;      // for layout calculations
};
```

Since `TypeSymbol` extends `SymbolWithScope`, every type IS a scope. For primitive types, the scope is empty. For class/struct types, the scope holds fields and methods. This uniformity means `typeSymbol->resolve("member")` is always valid — it just returns null for types with no members.

### 3.3 PrimitiveTypeSymbol

```cpp
class PrimitiveTypeSymbol : public TypeSymbol {
public:
    PrimitiveTypeSymbol(const std::string& name, PrimitiveKind kind, int sizeInBytes);

    enum class PrimitiveKind { Int, Double, Float, Byte, Char, String, Bool, Void };
    PrimitiveKind primitiveKind;
};
```

Pre-registered in `SymbolTable::Types`:
```
"int"    → PrimitiveTypeSymbol("int",    Int,    4)
"double" → PrimitiveTypeSymbol("double", Double, 8)
"float"  → PrimitiveTypeSymbol("float",  Float,  4)
"byte"   → PrimitiveTypeSymbol("byte",   Byte,   1)
"char"   → PrimitiveTypeSymbol("char",   Char,   1)
"string" → PrimitiveTypeSymbol("string", String, 8)   // ptr-sized
"bool"   → PrimitiveTypeSymbol("bool",   Bool,   1)
"void"   → PrimitiveTypeSymbol("void",   Void,   0)
```

### 3.4 VariableSymbol

```cpp
class VariableSymbol : public TypedSymbol {
public:
    VariableSymbol(const std::string& name, std::shared_ptr<TypeSymbol> type);

    std::shared_ptr<TypeSymbol> getType() const override;

    VariableRole role;             // Local | Parameter | Field
    bool isMutable = true;
    bool isInferred = false;
    bool isInitialized = false;
    int fieldIndex = -1;           // GEP index for struct/class fields
    bool isReference = false;      // true for T& parameters

private:
    void setType(const std::shared_ptr<TypeSymbol>& value) override;
    std::shared_ptr<TypeSymbol> type;
};
```

### 3.5 FunctionSymbol

```cpp
class FunctionSymbol : public SymbolWithScope, public TypedSymbol {
public:
    FunctionSymbol(const std::string& name);

    // TypedSymbol interface — type is the return type
    std::shared_ptr<TypeSymbol> getType() const override;           // returns ReturnType
    void setType(const std::shared_ptr<TypeSymbol>& value) override;

    // Diamond resolution (SymbolWithScope + TypedSymbol both extend Symbol)
    std::string getName() const override;
    int getInsertionOrderNumber() const override;
    void setInsertionOrderNumber(int value) override;
    std::shared_ptr<Scope> getSymbolScope() const override;
    void setSymbolScope(const std::shared_ptr<Scope>& value) override;

    // Function-specific:
    std::shared_ptr<TypeSymbol> ReturnType;
    std::vector<std::shared_ptr<VariableSymbol>> Parameters;

    bool isMethod = false;
    bool isExtern = false;
    bool isStatic = false;
    bool isAbstract = false;
    bool hasThisParam = false;
    int vtableIndex = -1;
};
```

**Key for LLVM codegen**: `Parameters` carries `VariableSymbol*` with full metadata — `getType()` gives the parameter's `TypeSymbol`, `isReference` tells you whether it's by-ref, `name` gives the debug name. This is the complete information needed for `buildFunctionType()`.

### 3.6 MethodSymbol, ConstructorSymbol, DestructorSymbol

```cpp
class MethodSymbol : public FunctionSymbol {
public:
    MethodSymbol(const std::string& name);
    std::shared_ptr<TypeSymbol> ClassOfThisMethod;
};

class ConstructorSymbol : public FunctionSymbol {
public:
    ConstructorSymbol(const std::string& name);
    // isMethod = true, hasThisParam = true, name = "constructor"
};

class DestructorSymbol : public FunctionSymbol {
public:
    DestructorSymbol(const std::string& name);
    // isMethod = true, hasThisParam = true, name = "destructor"
};

class OperatorSymbol : public FunctionSymbol {
public:
    OperatorSymbol(const std::string& name);
    OverloadableOp op;
    std::shared_ptr<TypeSymbol> ownerType;
};
```

### 3.7 ClassSymbol

```cpp
class ClassSymbol : public TypeSymbol {
public:
    ClassSymbol(const std::string& className, const std::shared_ptr<Scope>& enclosingScope);

    // Inheritance
    std::vector<std::string> BaseClasses;
    ClassSymbol* resolvedBaseClass = nullptr;
    bool isAbstract = false;

    // Members (also accessible via scope resolve/define)
    std::vector<std::shared_ptr<VariableSymbol>> fields;       // own fields
    std::vector<std::shared_ptr<VariableSymbol>> allFields;    // inherited + own (LLVM GEP layout)
    std::shared_ptr<ConstructorSymbol> constructor;
    std::shared_ptr<DestructorSymbol> destructor;

    // Virtual dispatch
    std::vector<std::shared_ptr<FunctionSymbol>> vtable;       // slot 0 = destructor
    int vtableSize = 0;

    // Interfaces
    std::vector<std::shared_ptr<InterfaceSymbol>> implementedInterfaces;

    bool hasRAII() const { return destructor != nullptr; }

    // Override resolve() to walk inheritance chain
    std::shared_ptr<Symbol> resolve(const std::string& name) override;

    void define(const std::shared_ptr<Symbol>& sym) override;
};
```

The overridden `resolve()`:

```cpp
std::shared_ptr<Symbol> ClassSymbol::resolve(const std::string& name) {
    // 1. Own members
    auto it = symbols.find(name);
    if (it != symbols.end()) return it->second;
    // 2. Inherited members (walk base class chain)
    if (resolvedBaseClass) {
        auto found = resolvedBaseClass->resolve(name);
        if (found) return found;
    }
    // 3. Enclosing scope (module-level)
    if (enclosingScope) return enclosingScope->resolve(name);
    return nullptr;
}
```

### 3.8 StructSymbol, EnumSymbol, InterfaceSymbol

```cpp
class StructSymbol : public TypeSymbol {
public:
    StructSymbol(const std::string& name);
    std::vector<std::shared_ptr<VariableSymbol>> fields;
};

class EnumSymbol : public TypeSymbol {
public:
    EnumSymbol(const std::string& name);
    struct MemberInfo {
        std::string name;
        int64_t intValue;
        std::string stringValue;
        bool hasExplicitValue;
    };
    std::vector<MemberInfo> members;
    std::shared_ptr<TypeSymbol> underlyingType;   // int, byte, or string
};

class InterfaceSymbol : public TypeSymbol {
public:
    InterfaceSymbol(const std::string& name);
    std::vector<std::shared_ptr<FunctionSymbol>> methods;   // abstract method decls
};
```

### 3.9 Compound Type Symbols

These represent structural types (not declared by the user, but constructed by the type system):

```cpp
class PointerTypeSymbol : public TypeSymbol {
public:
    PointerTypeSymbol(std::shared_ptr<TypeSymbol> baseType);
    std::shared_ptr<TypeSymbol> baseType;
};

class ArrayTypeSymbol : public TypeSymbol {
public:
    ArrayTypeSymbol(std::shared_ptr<TypeSymbol> elementType, int size);
    std::shared_ptr<TypeSymbol> elementType;
    int size;   // -1 = unsized/dynamic
};

class TupleTypeSymbol : public TypeSymbol {
public:
    TupleTypeSymbol(std::vector<std::shared_ptr<TypeSymbol>> elementTypes);
    std::vector<std::shared_ptr<TypeSymbol>> elementTypes;
};

class ReferenceTypeSymbol : public TypeSymbol {
public:
    ReferenceTypeSymbol(std::shared_ptr<TypeSymbol> baseType);
    std::shared_ptr<TypeSymbol> baseType;
    // Transient: unwrapped during resolveParameters, sets VariableSymbol::isReference
};
```

### 3.10 FunctionTypeSymbol — Critical for Closure Correctness

```cpp
class FunctionTypeSymbol : public TypeSymbol {
public:
    FunctionTypeSymbol(
        std::vector<ParameterInfo> parameters,
        std::shared_ptr<TypeSymbol> returnType,
        bool isVariadic = false
    );

    struct ParameterInfo {
        std::shared_ptr<TypeSymbol> type;
        std::string name;
        bool isReference = false;
    };

    std::vector<ParameterInfo> parameters;
    std::shared_ptr<TypeSymbol> returnType;
    bool isVariadic = false;
};
```

**This is what fixes the three HIGH bugs.** When a closure variable has type `FunctionTypeSymbol`, its `parameters` carry `isReference` and the actual `TypeSymbol` (which can be checked for `ClassSymbol`/`StructSymbol` to decide pointer-passing). The LLVM codegen reads this directly for indirect calls.

### 3.11 ErrorTypeSymbol and NullTypeSymbol

```cpp
class ErrorTypeSymbol : public TypeSymbol {
public:
    ErrorTypeSymbol();
    // Sentinel — isCompatible returns true if either side is ErrorType
};

class NullTypeSymbol : public TypeSymbol {
public:
    NullTypeSymbol();
    // Compatible with PointerTypeSymbol and FunctionTypeSymbol
};
```

### 3.12 ModuleSymbol

```cpp
class ModuleSymbol : public SymbolWithScope {
public:
    ModuleSymbol(const std::string& name);
    // IS the module scope. Module-level functions, types, externs all defined here.
};
```

### 3.13 Type Interning

Compound types are interned by string key in `SymbolTable::Types`:

```
"int*"           → PointerTypeSymbol(intSym)
"int[10]"        → ArrayTypeSymbol(intSym, 10)
"(int,double)"   → TupleTypeSymbol([intSym, doubleSym])
"(int,int)=>int" → FunctionTypeSymbol([{intSym,"",false},{intSym,"",false}], intSym)
"int&"           → ReferenceTypeSymbol(intSym)  // transient
```

The key is synthetic (for debugging/lookup). Identity is by `shared_ptr` — two references to the same `TypeSymbol` instance are the same type.

### 3.14 isCompatible()

Type compatibility is checked via a utility function (or method on `TypeSymbol`):

```cpp
bool isCompatible(TypeSymbol* from, TypeSymbol* to);
```

Rules (same as V1, adapted for TypeSymbol):
1. Same pointer → true
2. Either is `ErrorTypeSymbol` → true
3. Numeric widening: byte→int, char→int, int→float/double, float→double
4. `NullTypeSymbol` → `PointerTypeSymbol` or `FunctionTypeSymbol`
5. Enum ↔ underlying (bidirectional)
6. Interface upcast: `Dog*` → `Drawable*` if Dog implements Drawable
7. `byte*` universal pointer (both directions)
8. Inheritance: `Derived*` → `Base*`
9. `T` → `T&` (implicit address-of at call site)

---

## 4. AST Node Design

### 4.1 DebugInfo — Full Source Ranges

```cpp
// DebugInfo.h

class DebugInfo : public std::enable_shared_from_this<DebugInfo> {
public:
    int LineNumber;            // primary line (for quick reference)
    int ColumnNumber;          // primary column
    int LineNumberStart;       // range start
    int LineNumberEnd;         // range end
    int ColumnNumberStart;
    int ColumnNumberEnd;
};
```

### 4.2 AstBaseNode — Every Node Has Scope + DebugInfo

```cpp
// AstBaseNode.h

class AstBaseNode {
public:
    virtual ~AstBaseNode() = default;
    virtual void accept(ASTVisitor&) = 0;

    std::shared_ptr<Scope> AstScopeNode;            // the scope this node lives in
    std::shared_ptr<DebugInfo> DebugInfoAstNode;     // full source range

    template<typename T> T* as();
    template<typename T> bool is() const;
};
```

**Key change from V1:** Every AST node carries `AstScopeNode`. After Pass 1, this pointer is set. Later passes read it directly — no `childIndexStack_`, no positional scope navigation.

### 4.3 ExpressionBaseNode

```cpp
class ExpressionBaseNode : public AstBaseNode {
public:
    ExpressionNodeType ExpressionType;

    // Sema-resolved:
    std::shared_ptr<TypeSymbol> resolvedType;       // filled by Pass 3
    std::shared_ptr<Symbol> resolvedSymbol;         // for identifiers, member access
};
```

### 4.4 ArgumentsBaseNode — Per-Argument Reference Tracking

```cpp
// ArgumentsBaseNode.h

class ArgumentsBaseNode : public AstBaseNode {
public:
    std::vector<std::shared_ptr<ExpressionBaseNode>> Expressions;
    std::vector<bool> IsReference;   // per-argument: true if this arg is passed by ref
};
```

**This is the old design's pattern for call sites.** The `IsReference` vector is set by the TypeChecker when it matches arguments against the callee's `FunctionSymbol::Parameters`. This way, codegen doesn't need to re-resolve the callee type — it reads `IsReference[i]` directly.

For Mingus, `CallExpression` would use `ArgumentsBaseNode` (or carry the same fields):

```cpp
class CallExpression : public ExpressionBaseNode {
public:
    std::shared_ptr<ExpressionBaseNode> callee;
    std::shared_ptr<ArgumentsBaseNode> arguments;   // with IsReference per arg

    // Sema-resolved:
    std::shared_ptr<FunctionSymbol> resolvedCallee;  // null for indirect calls
};
```

### 4.5 Other Key AST Nodes

```cpp
class StatementBaseNode : public AstBaseNode {
public:
    StatementType NodeType;
};

class DeclarationBaseNode : public StatementBaseNode {
public:
    DeclarationNodeType DeclarationType;
};

class BlockStatementBaseNode : public StatementBaseNode {
public:
    std::vector<std::shared_ptr<StatementBaseNode>> Statements;
    // AstScopeNode points to this block's BlockScope
};

class ModifiersBaseNode : public AstBaseNode {
public:
    std::vector<ModifierTypes> Modifiers;   // public, private, protected, static, abstract, extern
};

class ParameterNode : public AstBaseNode {
public:
    std::string name;
    std::shared_ptr<TypeNode> type;
    bool isReference = false;
    std::shared_ptr<VariableSymbol> resolvedSymbol;   // set by Pass 1
};

class FunctionDeclaration : public DeclarationBaseNode {
public:
    std::string name;
    ModifiersBaseNode modifiers;
    std::vector<std::shared_ptr<ParameterNode>> parameters;
    std::shared_ptr<TypeNode> returnType;
    std::shared_ptr<BlockStatementBaseNode> body;
};

class ClassDeclarationBaseNode : public DeclarationBaseNode {
public:
    std::string ClassId;
    std::vector<std::string> Inheritance;
    ModifiersBaseNode ModifiersBase;
    std::shared_ptr<BlockStatementBaseNode> Block;
    bool IsDefined;
};

class LambdaExpression : public ExpressionBaseNode {
public:
    std::vector<std::shared_ptr<ParameterNode>> parameters;
    std::shared_ptr<AstBaseNode> body;

    // Capture specification (parsed):
    CaptureDefault captureDefault;
    std::vector<CaptureItem> captureItems;

    // Sema-resolved:
    std::vector<std::shared_ptr<Symbol>> capturedVariables;
    std::vector<CaptureMode> captureModesResolved;
    bool escapes = true;
    bool selfCapture = false;
};
```

### 4.6 Identifier and QualifiedName

```cpp
class Identifier : public AstBaseNode {
public:
    Identifier(const std::string& id);
    std::string Id;
};

class ModuleIdentifier : public AstBaseNode {
public:
    ModuleIdentifier(const std::string& id, std::vector<std::string>& parentModules);
    std::shared_ptr<Identifier> ModuleId;
    std::vector<std::shared_ptr<Identifier>> ParentModules;
    std::string ToString();
};
```

### 4.7 InterpolatedStringPart

```cpp
class InterpolatedStringPart {
public:
    InterpolatedStringPart(const std::string& textBeforeExpression,
                           const ExpressionBaseNode& expressionBaseNode);
    std::string TextBeforeExpression;
    ExpressionBaseNode ExpressionBase;
};
```

---

## 5. Sema Passes

The four-pass architecture is preserved. What changes is HOW they interact with the symbol/scope system.

### 5.1 Pass 1: SymbolTableBuilder

Creates all symbols and scopes. Sets `AstScopeNode` on every AST node. Sets `ParameterNode::resolvedSymbol`.

```cpp
void SymbolTableBuilder::visit(ClassDeclarationBaseNode& node) {
    auto classSym = std::make_shared<ClassSymbol>(node.ClassId, symbolTable.getCurrentScope());
    symbolTable.DefineSymbol(classSym);

    // ClassSymbol IS the scope — push it
    symbolTable.PushScope(classSym);
    node.AstScopeNode = classSym;    // the class decl lives in its own scope

    // Visit fields, methods, ctor, dtor...
    // Each field/method call symbolTable.DefineSymbol() into classSym

    buildVtable(classSym);
    symbolTable.PopScope();
}

void SymbolTableBuilder::visit(FunctionDeclaration& node) {
    auto funcSym = std::make_shared<FunctionSymbol>(node.name);
    symbolTable.DefineSymbol(funcSym);

    // FunctionSymbol IS the scope — push it
    symbolTable.PushScope(funcSym);
    node.AstScopeNode = funcSym;

    for (auto& param : node.parameters) {
        auto paramSym = std::make_shared<VariableSymbol>(param->name, nullptr);
        paramSym->role = VariableRole::Parameter;
        paramSym->isReference = param->isReference;
        symbolTable.DefineSymbol(paramSym);
        param->resolvedSymbol = paramSym;   // direct link
    }

    visit(node.body);
    symbolTable.PopScope();
}

void SymbolTableBuilder::visit(BlockStatementBaseNode& node) {
    auto blockScope = std::make_shared<BlockScope>(symbolTable.getCurrentScope());
    symbolTable.getCurrentScope()->nest(blockScope);
    symbolTable.PushScope(blockScope);
    node.AstScopeNode = blockScope;

    for (auto& stmt : node.Statements) stmt->accept(*this);

    symbolTable.PopScope();
}

// For all other nodes:
void SymbolTableBuilder::visit(BinaryOperationBaseNode& node) {
    node.AstScopeNode = symbolTable.getCurrentScope();
    node.LeftOperand->accept(*this);
    node.RightOperand->accept(*this);
}
```

### 5.2 Pass 2: TypeResolver

Resolves all type annotations. Builds `FunctionTypeSymbol` instances with full `ParameterInfo`. Uses `node.AstScopeNode` for scope navigation (no childIndexStack).

```cpp
void TypeResolver::visit(FunctionDeclaration& node) {
    currentScope = node.AstScopeNode;   // direct — the FunctionSymbol IS the scope
    auto funcSym = std::dynamic_pointer_cast<FunctionSymbol>(
        currentScope->resolve(node.name));

    // Resolve return type
    funcSym->ReturnType = resolveTypeNode(node.returnType.get());

    // Resolve parameters with full metadata
    for (size_t i = 0; i < node.parameters.size(); i++) {
        auto& param = node.parameters[i];
        auto paramSym = param->resolvedSymbol;
        auto resolvedType = resolveTypeNode(param->type.get());

        // Unwrap ReferenceType
        if (param->isReference) {
            paramSym->isReference = true;
            if (auto refType = std::dynamic_pointer_cast<ReferenceTypeSymbol>(resolvedType)) {
                resolvedType = refType->baseType;
            }
        }
        paramSym->setType(resolvedType);
    }
}
```

### 5.3 Pass 3: TypeChecker

Types all expressions. Sets `CallExpression::resolvedCallee`. Sets `ArgumentsBaseNode::IsReference`. Uses `node.AstScopeNode` everywhere.

```cpp
void TypeChecker::visit(CallExpression& node) {
    node.callee->accept(*this);
    auto calleeType = node.callee->resolvedType;

    // Visit arguments
    for (auto& arg : node.arguments->Expressions) arg->accept(*this);

    if (auto funcSym = std::dynamic_pointer_cast<FunctionSymbol>(node.callee->resolvedSymbol)) {
        // Direct function call
        node.resolvedCallee = funcSym;

        // Set IsReference per argument from function parameters
        node.arguments->IsReference.resize(funcSym->Parameters.size(), false);
        for (size_t i = 0; i < funcSym->Parameters.size(); i++) {
            node.arguments->IsReference[i] = funcSym->Parameters[i]->isReference;
            checkAssignability(argTypes[i], funcSym->Parameters[i]->getType(), ...);
            if (funcSym->Parameters[i]->isReference && !isLValue(node.arguments->Expressions[i])) {
                error("argument '" + funcSym->Parameters[i]->getName()
                      + "' requires an lvalue for reference parameter");
            }
        }
        node.resolvedType = funcSym->ReturnType;

    } else if (auto funcTypeSym = std::dynamic_pointer_cast<FunctionTypeSymbol>(calleeType)) {
        // Indirect call (closure / function pointer)
        node.arguments->IsReference.resize(funcTypeSym->parameters.size(), false);
        for (size_t i = 0; i < funcTypeSym->parameters.size(); i++) {
            node.arguments->IsReference[i] = funcTypeSym->parameters[i].isReference;
            checkAssignability(argTypes[i], funcTypeSym->parameters[i].type, ...);
        }
        node.resolvedType = funcTypeSym->returnType;
    }
}
```

### 5.4 Pass 4: SemanticValidator

RAII tracking, capture analysis, escape analysis. Logic unchanged — just uses `node.AstScopeNode` instead of `childIndexStack_`.

---

## 6. LLVM IR Codegen Mapping

### 6.1 `mapType(TypeSymbol*)` — Single Entry Point

```cpp
llvm::Type* IRGenerator::mapType(TypeSymbol* type) {
    if (auto* prim = type->as<PrimitiveTypeSymbol>()) {
        switch (prim->primitiveKind) {
            case PrimitiveKind::Int:    return builder_.getInt32Ty();
            case PrimitiveKind::Double: return builder_.getDoubleTy();
            case PrimitiveKind::Float:  return builder_.getFloatTy();
            case PrimitiveKind::Byte:   return builder_.getInt8Ty();
            case PrimitiveKind::Char:   return builder_.getInt8Ty();
            case PrimitiveKind::Bool:   return builder_.getInt1Ty();
            case PrimitiveKind::String: return ptrTy;
            case PrimitiveKind::Void:   return builder_.getVoidTy();
        }
    }
    if (type->is<PointerTypeSymbol>()) {
        if (type->as<PointerTypeSymbol>()->baseType->is<InterfaceSymbol>())
            return getFatPtrType();          // interface* → { ptr, ptr }
        return ptrTy;                        // all other pointers → ptr
    }
    if (type->is<ClassSymbol>() || type->is<StructSymbol>())
        return getOrCreateStructType(type);  // named LLVM struct type
    if (type->is<EnumSymbol>())
        return mapType(type->as<EnumSymbol>()->underlyingType.get());
    if (type->is<ArrayTypeSymbol>()) {
        auto* arr = type->as<ArrayTypeSymbol>();
        if (arr->size > 0) return llvm::ArrayType::get(mapType(arr->elementType.get()), arr->size);
        return ptrTy;
    }
    if (type->is<TupleTypeSymbol>()) {
        auto* tup = type->as<TupleTypeSymbol>();
        std::vector<llvm::Type*> elems;
        for (auto& e : tup->elementTypes) elems.push_back(mapType(e.get()));
        return llvm::StructType::get(context_, elems);
    }
    if (type->is<FunctionTypeSymbol>())
        return getFatPtrType();              // closures → { ptr, ptr }
    if (type->is<ReferenceTypeSymbol>())
        return ptrTy;                        // T& → ptr
    // ErrorType, NullType
    return ptrTy;
}
```

### 6.2 `mapParamType()` — Uses FunctionSymbol Parameters Directly

For named functions, codegen reads `FunctionSymbol::Parameters`:

```cpp
llvm::FunctionType* IRGenerator::buildFunctionType(FunctionSymbol* sym) {
    llvm::Type* retTy = mapType(sym->ReturnType.get());
    std::vector<llvm::Type*> paramTypes;

    if (sym->hasThisParam) paramTypes.push_back(ptrTy);

    for (auto& param : sym->Parameters) {
        paramTypes.push_back(mapParamType(param.get()));
    }
    return llvm::FunctionType::get(retTy, paramTypes, false);
}

llvm::Type* IRGenerator::mapParamType(VariableSymbol* param) {
    if (param->isReference)
        return ptrTy;                                         // T& → ptr
    auto* type = param->getType().get();
    if (type->is<ClassSymbol>() || type->is<StructSymbol>())
        return ptrTy;                                         // struct/class → ptr
    if (type->is<InterfaceSymbol>())
        return getFatPtrType();                               // interface → { ptr, ptr }
    return mapType(type);
}
```

For **indirect calls** (closures), codegen reads `FunctionTypeSymbol::parameters`:

```cpp
llvm::Type* IRGenerator::mapParamType(const FunctionTypeSymbol::ParameterInfo& param) {
    if (param.isReference)
        return ptrTy;
    auto* type = param.type.get();
    if (type->is<ClassSymbol>() || type->is<StructSymbol>())
        return ptrTy;
    if (type->is<InterfaceSymbol>())
        return getFatPtrType();
    return mapType(type);
}
```

**Both paths use the same logic.** Named functions have `VariableSymbol*` parameters. Closure types have `ParameterInfo` parameters. Both carry `isReference` and a `TypeSymbol*` for type checking. No more divergence.

### 6.3 Indirect Call (Closures) — Fixed

```cpp
void IRGenerator::emitIndirectCall(CallExpression& node, llvm::Value* fatPtr) {
    llvm::Value* fnPtr = builder_.CreateExtractValue(fatPtr, {0});
    llvm::Value* envPtr = builder_.CreateExtractValue(fatPtr, {1});

    auto funcTypeSym = std::dynamic_pointer_cast<FunctionTypeSymbol>(
        node.callee->resolvedType);

    llvm::Type* retTy = mapType(funcTypeSym->returnType.get());
    std::vector<llvm::Type*> paramTypes;
    for (auto& param : funcTypeSym->parameters) {
        paramTypes.push_back(mapParamType(param));   // FULL metadata available
    }
    paramTypes.push_back(ptrTy);  // env pointer

    auto* fnTy = llvm::FunctionType::get(retTy, paramTypes, false);

    std::vector<llvm::Value*> args;
    for (size_t i = 0; i < node.arguments->Expressions.size(); i++) {
        node.arguments->Expressions[i]->accept(*this);
        llvm::Value* argVal = lastValue_;

        // Use ArgumentsBaseNode::IsReference to decide pass-by-ref
        if (node.arguments->IsReference[i]) {
            argVal = emitLValue(node.arguments->Expressions[i].get());
        }
        // Use the parameter type to decide struct pass-by-pointer
        else if (funcTypeSym->parameters[i].type->is<ClassSymbol>()
              || funcTypeSym->parameters[i].type->is<StructSymbol>()) {
            argVal = emitLValue(node.arguments->Expressions[i].get());
        }
        args.push_back(argVal);
    }
    args.push_back(envPtr);

    lastValue_ = builder_.CreateCall(fnTy, fnPtr, args);
}
```

### 6.4 Lambda Codegen — Parameter Mapping Simplified

```cpp
void IRGenerator::visit(LambdaExpression& node) {
    auto funcTypeSym = std::dynamic_pointer_cast<FunctionTypeSymbol>(node.resolvedType);

    // Build LLVM function type from FunctionTypeSymbol::parameters
    llvm::Type* retTy = mapType(funcTypeSym->returnType.get());
    std::vector<llvm::Type*> paramTypes;
    for (auto& param : funcTypeSym->parameters) {
        paramTypes.push_back(mapParamType(param));
    }
    paramTypes.push_back(ptrTy);  // env

    auto* fnTy = llvm::FunctionType::get(retTy, paramTypes, false);
    auto* lambdaFn = llvm::Function::Create(fnTy, ...);

    // Map parameters using ParameterNode::resolvedSymbol — NO scanForParamSymbols
    for (size_t i = 0; i < node.parameters.size(); i++) {
        auto* paramSym = node.parameters[i]->resolvedSymbol.get();
        namedValues_[paramSym] = lambdaFn->getArg(i);
    }

    // ... rest of lambda codegen (captures, body, allocation) unchanged ...
}
```

### 6.5 Scope Navigation — Direct

```cpp
void IRGenerator::visit(BlockStatementBaseNode& node) {
    auto savedScope = currentScope_;
    currentScope_ = node.AstScopeNode;   // direct pointer
    for (auto& stmt : node.Statements) stmt->accept(*this);
    currentScope_ = savedScope;
}
```

No `childIndexStack_`. No `enterNextChildScope()`. No positional desync risk.

### 6.6 Field Access — Direct

```cpp
void IRGenerator::visit(MemberAccessExpression& node) {
    // resolvedSymbol was set by TypeChecker
    auto* fieldSym = node.resolvedSymbol->as<VariableSymbol>();
    int gepIdx = fieldSym->fieldIndex;   // O(1) — set during Pass 1
    // Add vtable offset: +1 if class has vtable
    if (classSym->vtableSize > 0) gepIdx++;
    // ... GEP and load ...
}
```

---

## 7. What This Fixes

### 7.1 Three HIGH-Severity Bugs — All Fixed

| Bug | Root Cause | Fix |
|-----|-----------|-----|
| **Closures with struct params** | `FunctionType::parameterTypes` had no struct-kind info. `mapType(struct)` returned LLVM struct, not `ptr`. | `FunctionTypeSymbol::parameters` has `TypeSymbol*` which can be checked with `is<ClassSymbol>()`/`is<StructSymbol>()`. `mapParamType` returns `ptr`. |
| **Closures with reference params** | `isReference` on `VariableSymbol`, not on `FunctionType`. Lost during indirect call. | `FunctionTypeSymbol::ParameterInfo::isReference` carries it. `mapParamType` returns `ptr`. |
| **Interface parameter passing** | Interface `{ ptr, ptr }` passed as arg, function expected `ptr`. | `mapParamType` checks `is<InterfaceSymbol>()` → returns `getFatPtrType()`. |

### 7.2 Structural Debt — All Eliminated

| Issue | Fix |
|-------|-----|
| **Dead `ClassType`/`StructType`/`EnumType`** | Eliminated entirely. `TypeSymbol` IS the type. |
| **`UserType` with `void*`** | Eliminated. No more `void*` casting. |
| **Separate Type hierarchy** | Gone. One hierarchy: `TypeSymbol`. |
| **`childIndexStack_` fragility** | Gone. `AstScopeNode` on every node. |
| **`scanForParamSymbols`** | Gone. `ParameterNode::resolvedSymbol` set in Pass 1. |
| **`resolveTypeNode` duplicated** | Shared utility, resolves against `TypeSymbol` instances. |
| **`FieldInfo*` dangling pointer** | `Symbol*` on `MemberAccessExpression` — stable lifetime. |
| **O(n) `getFieldGEPIndex`** | `VariableSymbol::fieldIndex` is O(1). |
| **Point-only source locations** | `DebugInfo` with full ranges. |
| **No `IsReference` at call sites** | `ArgumentsBaseNode::IsReference` per argument. |

### 7.3 Foundation for Future Features

| Feature | How V2 Enables It |
|---------|-------------------|
| **Generics** | `TypeSymbol` unification means monomorphized types are just new `TypeSymbol` instances in the registry. `ClassSymbol<int>` → `ClassSymbol("Vector_int")` with concrete fields. |
| **Function overloading** | `FunctionSymbol::Parameters` carries full type info. Overload resolution can compare `TypeSymbol*` lists directly. |
| **Const modifier** | `VariableSymbol::isMutable` already exists. TypeChecker can enforce it using the rich `AstScopeNode` for context. |
| **Error recovery** | `DebugInfo` with source ranges enables precise error positioning. `ErrorTypeSymbol` prevents cascading. |
| **IDE integration** | `Symbol::SymbolUsageNodes` tracks all references. `AstScopeNode` enables find-definition. `DebugInfo` ranges enable hover info. |

---

*碼道無形，生育萬程。一統則簡，簡則能變。*
*The Way of Code is formless, giving birth to all programs. Unification brings simplicity. Simplicity enables evolution.*
