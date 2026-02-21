#pragma once

// ============================================================================
// Symbols.h — All concrete symbol types for Mingus V2
//
// This file defines the symbols that represent user-declared entities:
//
// VariableSymbol     — locals, parameters, struct/class fields
// FunctionSymbol     — functions (IS scope for body, IS typed via return type)
// MethodSymbol       — instance methods (has ClassOfThisMethod)
// ConstructorSymbol  — class constructors
// DestructorSymbol   — class destructors
// OperatorSymbol     — operator overloads
// ClassSymbol        — class declarations (IS scope for members, IS TypeSymbol)
// StructSymbol       — struct declarations
// EnumSymbol         — enum declarations
// InterfaceSymbol    — interface declarations
// ModuleSymbol       — module declarations (IS scope for module-level symbols)
//
// Dependency chain:
//   TypeSymbol.h -> Symbol.h -> Scope.h -> Forward.h
//   This file includes TypeSymbol.h and adds everything else.
// ============================================================================

#include "Forward.h"
#include "TypeSymbol.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mingus {

// ============================================================================
// VariableSymbol — Locals, parameters, struct/class fields
//
// Extends TypedSymbol (has getType/setType returning TypeSymbolPtr).
// ============================================================================

class VariableSymbol : public TypedSymbol {
public:
    VariableSymbol(const std::string& name, TypeSymbolPtr type);

    TypeSymbolPtr getType() const override;
    void setType(const TypeSymbolPtr& type) override;

    // What role does this variable play?
    VariableRole role = VariableRole::Local;

    // Mutability (for future const enforcement)
    bool isMutable = true;

    // Was the type inferred (var x = expr)?
    bool isInferred = false;

    // Has a definite assignment?
    bool isInitialized = false;

    // For struct/class fields: GEP index (O(1) field access)
    int fieldIndex = -1;

    // For reference parameters (T&): pass-by-pointer semantics
    bool isReference = false;

    // For class fields: initial value expression (optional, for future use)
    // std::shared_ptr<ExpressionBaseNode> defaultValue;

private:
    TypeSymbolPtr type_;
};

// ============================================================================
// FunctionSymbol — Named function declarations
//
// Dual nature: IS both a SymbolWithScope (scope for its body) and a
// TypedSymbol (its "type" is the return type). This means FunctionSymbol
// has diamond inheritance through Symbol — resolved via explicit overrides.
//
// Parameters are VariableSymbol instances defined inside this function's scope.
// They are also stored in the Parameters vector for ordered access.
// ============================================================================

class FunctionSymbol : public SymbolWithScope {
public:
    explicit FunctionSymbol(const std::string& name);

    // Return type (what TypedSymbol::getType() would return)
    TypeSymbolPtr returnType;

    // Ordered parameter list with full metadata
    std::vector<std::shared_ptr<VariableSymbol>> parameters;

    // ---- Function properties ----
    bool isMethod = false;
    bool isExtern = false;
    bool isStatic = false;
    bool isAbstract = false;
    bool isVirtual = false;
    bool isVariadic = false;
    bool hasThisParam = false;

    // Virtual dispatch index (-1 = not virtual)
    int vtableIndex = -1;

    // Build the corresponding FunctionTypeSymbol for this function
    // (used when storing a function reference in a variable)
    std::shared_ptr<FunctionTypeSymbol> buildFunctionType() const;

    // ---- Qualified name (Module_FuncName for LLVM) ----
    std::string getQualifiedName() const override;
};

// ============================================================================
// MethodSymbol — Instance methods (belong to a class)
// ============================================================================

class MethodSymbol : public FunctionSymbol {
public:
    explicit MethodSymbol(const std::string& name);

    // The class this method belongs to
    std::shared_ptr<TypeSymbol> classOfThisMethod;
};

// ============================================================================
// ConstructorSymbol — Class constructors
// ============================================================================

class ConstructorSymbol : public FunctionSymbol {
public:
    explicit ConstructorSymbol(const std::string& className);

    // Always: isMethod = true, hasThisParam = true, name = "constructor"
    bool isCopyConstructor = false;  // Set by SymbolTableBuilder for copy ctors
};

// ============================================================================
// DestructorSymbol — Class destructors
// ============================================================================

class DestructorSymbol : public FunctionSymbol {
public:
    explicit DestructorSymbol(const std::string& className);

    // Always: isMethod = true, hasThisParam = true, name = "destructor"
};

// ============================================================================
// OperatorSymbol — Operator overloads
// ============================================================================

class OperatorSymbol : public FunctionSymbol {
public:
    OperatorSymbol(const std::string& name, OverloadableOp op);

    OverloadableOp op;

    // The type that owns this operator (for resolution)
    TypeSymbolPtr ownerType;
};

// ============================================================================
// ClassSymbol — Class declarations
//
// Extends TypeSymbol (IS a type) and inherits SymbolWithScope (IS a scope
// for its members). Fields, methods, ctor, dtor are all symbols defined
// within this scope.
//
// Overrides resolve() to walk the inheritance chain.
// ============================================================================

class ClassSymbol : public TypeSymbol {
public:
    ClassSymbol(const std::string& className, ScopePtr enclosingScope);

    // ---- Inheritance ----
    std::vector<std::string> baseClassNames;  // unresolved names from parse
    ClassSymbol* resolvedBaseClass = nullptr;  // resolved in Pass 1
    bool isAbstract = false;

    // ---- Members ----
    // Fields are also accessible via scope resolve/define
    std::vector<std::shared_ptr<VariableSymbol>> fields;     // own fields only
    std::vector<std::shared_ptr<VariableSymbol>> allFields;  // inherited + own (LLVM GEP order)

    std::shared_ptr<ConstructorSymbol> constructor;
    std::shared_ptr<ConstructorSymbol> copyConstructor;  // constructor(ClassName& other)
    std::shared_ptr<DestructorSymbol> destructor;

    // ---- Virtual dispatch ----
    // Slot 0 = destructor. User methods start at index 1.
    std::vector<std::shared_ptr<FunctionSymbol>> vtable;
    int vtableSize = 0;

    // ---- Interface implementation ----
    std::vector<std::shared_ptr<InterfaceSymbol>> implementedInterfaces;

    // Does this class need RAII cleanup?
    bool hasRAII() const { return destructor != nullptr; }

    // Has a vtable (virtual methods or inheritance)?
    bool hasVtable() const { return vtableSize > 0; }

    // ---- Scope overrides ----
    // Walk inheritance chain for name resolution
    SymbolPtr resolve(const std::string& name) const override;
};

// ============================================================================
// StructSymbol — Struct declarations
//
// Simpler than ClassSymbol: no inheritance, no vtable, no constructor/destructor
// (though cleanup functions may be synthesized for struct fields that are closures).
// ============================================================================

class StructSymbol : public TypeSymbol {
public:
    explicit StructSymbol(const std::string& name);

    std::vector<std::shared_ptr<VariableSymbol>> fields;

    // Does any field require cleanup (closure-typed)?
    bool needsCleanup() const;
};

// ============================================================================
// EnumSymbol — Enum declarations
// ============================================================================

class EnumSymbol : public TypeSymbol {
public:
    explicit EnumSymbol(const std::string& name);

    struct MemberInfo {
        std::string name;
        int64_t intValue = 0;
        std::string stringValue;
        bool hasExplicitValue = false;
    };

    std::vector<MemberInfo> members;

    // Underlying storage type (int, byte, or string)
    TypeSymbolPtr underlyingType;

    // Find a member by name
    const MemberInfo* findMember(const std::string& name) const;
};

// ============================================================================
// InterfaceSymbol — Interface declarations
//
// Defines abstract method signatures. Classes that implement an interface
// must provide all methods. Fat pointer: { objPtr, itablePtr }.
// ============================================================================

class InterfaceSymbol : public TypeSymbol {
public:
    explicit InterfaceSymbol(const std::string& name);

    // Abstract method declarations (no bodies)
    std::vector<std::shared_ptr<FunctionSymbol>> methods;

    // Find a method by name
    std::shared_ptr<FunctionSymbol> findMethod(const std::string& name) const;
};

// ============================================================================
// TypeAliasSymbol — Type alias declarations (typedef)
//
// Not a TypeSymbol — a typedef is not a new type, just an alias.
// During type resolution, encountering this symbol unwraps immediately
// to the underlying aliasedType.
// ============================================================================

class TypeAliasSymbol : public BaseSymbol {
public:
    explicit TypeAliasSymbol(const std::string& name);

    // The type this alias refers to (resolved in Pass 2)
    TypeSymbolPtr aliasedType;
};

// ============================================================================
// ModuleSymbol — Module declarations
//
// IS the module scope. All module-level functions, types, externs are
// defined within this scope. Used for name mangling (Module_FuncName).
// ============================================================================

class ModuleSymbol : public SymbolWithScope {
public:
    explicit ModuleSymbol(const std::string& name);

    // Modules this module imports from
    std::vector<std::shared_ptr<ModuleSymbol>> importedModules;
};

} // namespace mingus
