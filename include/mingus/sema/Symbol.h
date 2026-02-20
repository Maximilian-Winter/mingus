//================================================================================
// MINGUS v1 - Symbol Hierarchy
// Every declaration creates a symbol. Symbols carry metadata needed by
// type checking and code generation.
//================================================================================

#pragma once

#include "mingus/ast/ASTNode.h"
#include "mingus/ast/Type.h"
#include "mingus/ast/Declarations.h"

#include <memory>
#include <string>
#include <vector>

// Re-export AccessModifier from Declarations.h for convenience
using mingus::ast::AccessModifier;

namespace mingus {
namespace sema {

// Forward declarations
class Scope;
class OperatorSymbol;

using namespace mingus::ast;

//================================================================================
// SymbolKind — discriminator for safe downcasting
//================================================================================
enum class SymbolKind {
    Variable,
    Function,
    Constructor,
    Destructor,
    Operator,
    Struct,
    Class,
    Enum,
    Interface,
    Module
};

//================================================================================
// VariableRole — where the variable lives
//================================================================================
enum class VariableRole {
    Local,      // Declared inside a function/method body
    Parameter,  // Function/method parameter
    Field       // Struct/class member variable
};

//================================================================================
// OverloadableOp — operators that can be overloaded
//================================================================================
enum class OverloadableOp {
    Plus,
    Minus,
    Star,
    Slash,
    Modulo,
    Index,
    Equals,
    NotEquals,
    Less,
    Greater,
    LessEqual,
    GreaterEqual
};

OverloadableOp operatorKindToOverloadableOp(OperatorKind kind);

//================================================================================
// Symbol — abstract base for all symbols
//================================================================================
class Symbol {
public:
    std::string name;
    SymbolKind kind;
    SourceLocation location;
    Symbol* owner;          // Enclosing symbol (module, struct, class)
    AccessModifier accessLevel;

    // Backward-compatible accessor: anything not Private is "public" in the old sense
    bool isPublic() const { return accessLevel != AccessModifier::Private; }

    Symbol(const std::string& n, SymbolKind k,
           const SourceLocation& loc = SourceLocation(),
           Symbol* own = nullptr, bool pub = true)
        : name(n), kind(k), location(loc), owner(own)
        , accessLevel(pub ? AccessModifier::Public : AccessModifier::Private) {}

    virtual ~Symbol() = default;

    template<typename T>
    T* as() { return dynamic_cast<T*>(this); }

    template<typename T>
    const T* as() const { return dynamic_cast<const T*>(this); }

    template<typename T>
    bool is() const { return dynamic_cast<const T*>(this) != nullptr; }
};

//================================================================================
// VariableSymbol — local variables, parameters, struct/class fields
//================================================================================
class VariableSymbol : public Symbol {
public:
    TypePtr<Type> type;         // Resolved type (null until Pass 2)
    VariableRole role;
    bool isMutable;
    bool isInferred;            // Declared with 'var'
    bool isInitialized;         // Has initializer expression
    int fieldIndex;             // Position in struct/class layout (-1 if not a field)
    bool isReference = false;   // true for int& x reference parameters

    VariableSymbol(const std::string& n,
                   VariableRole r,
                   const SourceLocation& loc = SourceLocation(),
                   Symbol* own = nullptr)
        : Symbol(n, SymbolKind::Variable, loc, own)
        , type(nullptr)
        , role(r)
        , isMutable(true)
        , isInferred(false)
        , isInitialized(false)
        , fieldIndex(-1)
    {}
};

//================================================================================
// FunctionSymbol — free functions, methods, extern declarations
//================================================================================
class FunctionSymbol : public Symbol {
public:
    std::vector<VariableSymbol*> parameters;  // Owned by function scope
    TypePtr<Type> returnType;                 // Resolved return type
    bool isMethod;
    bool isExtern;
    bool isStatic;
    bool isAbstract;
    bool hasThisParam;
    int vtableIndex;                          // Slot in owning class's vtable (-1 = not virtual)
    Scope* bodyScope;                         // Function's local scope

    FunctionSymbol(const std::string& n,
                   const SourceLocation& loc = SourceLocation(),
                   Symbol* own = nullptr)
        : Symbol(n, SymbolKind::Function, loc, own)
        , returnType(nullptr)
        , isMethod(false)
        , isExtern(false)
        , isStatic(false)
        , isAbstract(false)
        , hasThisParam(false)
        , vtableIndex(-1)
        , bodyScope(nullptr)
    {}
};

//================================================================================
// ConstructorSymbol — extends FunctionSymbol
// Return type is implicitly the owning class. Name is always "constructor".
//================================================================================
class ConstructorSymbol : public FunctionSymbol {
public:
    explicit ConstructorSymbol(const SourceLocation& loc = SourceLocation(),
                               Symbol* own = nullptr)
        : FunctionSymbol("constructor", loc, own)
    {
        kind = SymbolKind::Constructor;
        isMethod = true;
        hasThisParam = true;
    }
};

//================================================================================
// DestructorSymbol — extends FunctionSymbol
// No user-defined parameters. Name is always "destructor".
//================================================================================
class DestructorSymbol : public FunctionSymbol {
public:
    explicit DestructorSymbol(const SourceLocation& loc = SourceLocation(),
                              Symbol* own = nullptr)
        : FunctionSymbol("destructor", loc, own)
    {
        kind = SymbolKind::Destructor;
        isMethod = true;
        hasThisParam = true;
    }
};

//================================================================================
// OperatorSymbol — operator overloads
//================================================================================
class OperatorSymbol : public Symbol {
public:
    OverloadableOp op;
    std::vector<VariableSymbol*> parameters;  // rhs operand(s)
    TypePtr<Type> returnType;
    Symbol* ownerType;  // TypeSymbol* that owns this operator
    Scope* bodyScope;

    OperatorSymbol(OverloadableOp o,
                   const SourceLocation& loc = SourceLocation(),
                   Symbol* own = nullptr)
        : Symbol("operator", SymbolKind::Operator, loc, own)
        , op(o)
        , returnType(nullptr)
        , ownerType(own)
        , bodyScope(nullptr)
    {}
};

//================================================================================
// TypeSymbol — abstract base for user-defined types
//================================================================================
class TypeSymbol : public Symbol {
public:
    Scope* memberScope;  // Contains fields, methods, operators

    TypeSymbol(const std::string& n, SymbolKind k,
               const SourceLocation& loc = SourceLocation(),
               Symbol* own = nullptr)
        : Symbol(n, k, loc, own)
        , memberScope(nullptr)
    {}

    VariableSymbol* findField(const std::string& fieldName) const;
    FunctionSymbol* findMethod(const std::string& methodName) const;
    OperatorSymbol* findOperator(OverloadableOp op) const;
};

//================================================================================
// StructSymbol — value type, no constructor/destructor
//================================================================================
class StructSymbol : public TypeSymbol {
public:
    std::vector<VariableSymbol*> fields;  // Ordered for layout

    explicit StructSymbol(const std::string& n,
                          const SourceLocation& loc = SourceLocation(),
                          Symbol* own = nullptr)
        : TypeSymbol(n, SymbolKind::Struct, loc, own)
    {}
};

//================================================================================
// InterfaceSymbol — abstract contract, Go-style fat pointer dispatch
//================================================================================
class InterfaceSymbol : public TypeSymbol {
public:
    std::vector<FunctionSymbol*> methods;  // Abstract methods in declaration order

    explicit InterfaceSymbol(const std::string& n,
                              const SourceLocation& loc = SourceLocation(),
                              Symbol* own = nullptr)
        : TypeSymbol(n, SymbolKind::Interface, loc, own)
    {}
};

//================================================================================
// ClassSymbol — reference type, RAII-capable
//================================================================================
class ClassSymbol : public TypeSymbol {
public:
    std::vector<VariableSymbol*> fields;      // Own fields only (declared in this class)
    ConstructorSymbol* constructor;            // At most one in v1
    DestructorSymbol* destructor;              // At most one in v1

    // Inheritance
    ClassSymbol* baseClass = nullptr;          // Single-inheritance parent (null if root)
    bool isAbstract = false;                   // Declared 'abstract'
    std::vector<FunctionSymbol*> vtable;       // Ordered virtual method slots
    int vtableSize = 0;                        // Total vtable entries (including inherited)
    std::vector<VariableSymbol*> allFields;    // Inherited + own fields (for LLVM layout)

    // Interface implementation
    std::vector<InterfaceSymbol*> implementedInterfaces;  // Interfaces this class implements

    explicit ClassSymbol(const std::string& n,
                         const SourceLocation& loc = SourceLocation(),
                         Symbol* own = nullptr)
        : TypeSymbol(n, SymbolKind::Class, loc, own)
        , constructor(nullptr)
        , destructor(nullptr)
    {}

    bool hasRAII() const { return destructor != nullptr; }
};

//================================================================================
// EnumMemberInfo — resolved enum member
//================================================================================
struct EnumMemberInfo {
    std::string name;
    int64_t value;
    std::string stringValue;
    bool isString;

    EnumMemberInfo(const std::string& n, int64_t v)
        : name(n), value(v), isString(false) {}
    EnumMemberInfo(const std::string& n, const std::string& sv)
        : name(n), value(0), stringValue(sv), isString(true) {}
};

//================================================================================
// EnumSymbol — discrete named integer constants
//================================================================================
class EnumSymbol : public TypeSymbol {
public:
    std::vector<EnumMemberInfo> members;
    TypePtr<Type> underlyingType;  // Resolved underlying type (nullptr → defaults to int)

    explicit EnumSymbol(const std::string& n,
                        const SourceLocation& loc = SourceLocation(),
                        Symbol* own = nullptr)
        : TypeSymbol(n, SymbolKind::Enum, loc, own)
        , underlyingType(nullptr)
    {}
};

//================================================================================
// ModuleSymbol — scoping container for top-level declarations
//================================================================================
class ModuleSymbol : public Symbol {
public:
    Scope* moduleScope;

    explicit ModuleSymbol(const std::string& n,
                          const SourceLocation& loc = SourceLocation())
        : Symbol(n, SymbolKind::Module, loc, nullptr, true)
        , moduleScope(nullptr)
    {}
};

} // namespace sema
} // namespace mingus
