#pragma once

// ============================================================================
// SymbolTable.h — Root owner of the scope tree and type registry
//
// The SymbolTable owns:
// 1. The GlobalScope (root of the scope tree)
// 2. The current scope pointer (for incremental construction during Pass 1)
// 3. The type interning map (canonical TypeSymbol instances)
//
// Primitive types (int, double, etc.) are pre-registered on construction.
// Compound types (T*, T[], (T1,T2)=>R, etc.) are interned by structural key.
// ============================================================================

#include "Forward.h"
#include "Scope.h"
#include "Symbol.h"
#include "TypeSymbol.h"
#include "Symbols.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace mingus {

class SymbolTable {
public:
    SymbolTable();

    // ---- Root scope ----
    std::shared_ptr<GlobalScope> getRootScope() const;

    // ---- Scope navigation (used during construction in Pass 1) ----
    ScopePtr getCurrentScope() const;
    void setCurrentScope(const ScopePtr& scope);
    void pushScope(const ScopePtr& scope);
    void popScope();

    // ---- Symbol definition (convenience: defines in currentScope) ----
    void defineSymbol(const SymbolPtr& symbol);

    // ---- Type registry ----
    // Get or create canonical type instances.
    // All types are interned: same structural type = same shared_ptr.

    // Retrieve pre-registered primitive types
    std::shared_ptr<PrimitiveTypeSymbol> getIntType() const;
    std::shared_ptr<PrimitiveTypeSymbol> getDoubleType() const;
    std::shared_ptr<PrimitiveTypeSymbol> getFloatType() const;
    std::shared_ptr<PrimitiveTypeSymbol> getByteType() const;
    std::shared_ptr<PrimitiveTypeSymbol> getCharType() const;
    std::shared_ptr<PrimitiveTypeSymbol> getStringType() const;
    std::shared_ptr<PrimitiveTypeSymbol> getBoolType() const;
    std::shared_ptr<PrimitiveTypeSymbol> getVoidType() const;
    std::shared_ptr<PrimitiveTypeSymbol> getShortType() const;
    std::shared_ptr<PrimitiveTypeSymbol> getUShortType() const;
    std::shared_ptr<PrimitiveTypeSymbol> getUIntType() const;
    std::shared_ptr<PrimitiveTypeSymbol> getLongType() const;
    std::shared_ptr<PrimitiveTypeSymbol> getULongType() const;

    // Sentinel types
    std::shared_ptr<ErrorTypeSymbol> getErrorType() const;
    std::shared_ptr<NullTypeSymbol> getNullType() const;

    // Built-in String object type
    std::shared_ptr<StringObjectSymbol> getStringObjectType() const;

    // Intern compound types (get-or-create)
    std::shared_ptr<PointerTypeSymbol> getPointerType(TypeSymbolPtr baseType);
    std::shared_ptr<PointerTypeSymbol> getSharedPointerType(TypeSymbolPtr baseType);
    std::shared_ptr<PointerTypeSymbol> getConstPointerType(TypeSymbolPtr baseType);
    std::shared_ptr<ArrayTypeSymbol> getArrayType(TypeSymbolPtr elementType, int size);
    std::shared_ptr<TupleTypeSymbol> getTupleType(std::vector<TypeSymbolPtr> elementTypes);
    std::shared_ptr<FunctionTypeSymbol> getFunctionType(
        std::vector<FunctionTypeSymbol::ParameterInfo> params,
        TypeSymbolPtr returnType,
        bool isVariadic = false);
    std::shared_ptr<ReferenceTypeSymbol> getReferenceType(TypeSymbolPtr baseType);

    // Register a user-defined type (class, struct, enum, interface)
    void registerType(const std::string& name, TypeSymbolPtr type);

    // Look up a type by name
    TypeSymbolPtr resolveType(const std::string& name) const;

    // ---- Type compatibility ----
    bool isCompatible(TypeSymbol* from, TypeSymbol* to) const;

    // ---- Raw access to type registry (for diagnostics/tests) ----
    const std::unordered_map<std::string, TypeSymbolPtr>& getAllTypes() const;

    // ---- Generics monomorphization ----
    std::shared_ptr<FunctionSymbol> getOrCreateMonomorphization(
        FunctionSymbol* genericTemplate,
        const std::vector<TypeSymbolPtr>& typeArgs);
    bool hasMonomorphization(FunctionSymbol* genericTemplate,
                              const std::vector<TypeSymbolPtr>& typeArgs) const;
    std::vector<std::shared_ptr<FunctionSymbol>> getAllMonomorphizations() const;

    // Type substitution for monomorphization (replaces TypeParameterSymbol)
    TypeSymbolPtr substituteType(TypeSymbolPtr type,
        const std::unordered_map<std::string, TypeSymbolPtr>& subst) const;

private:
    std::shared_ptr<GlobalScope> rootScope_;
    ScopePtr currentScope_;

    // Type interning map: key -> canonical TypeSymbol instance
    std::unordered_map<std::string, TypeSymbolPtr> types_;

    // Pre-registered primitive types (fast access)
    std::shared_ptr<PrimitiveTypeSymbol> intType_;
    std::shared_ptr<PrimitiveTypeSymbol> doubleType_;
    std::shared_ptr<PrimitiveTypeSymbol> floatType_;
    std::shared_ptr<PrimitiveTypeSymbol> byteType_;
    std::shared_ptr<PrimitiveTypeSymbol> charType_;
    std::shared_ptr<PrimitiveTypeSymbol> stringType_;
    std::shared_ptr<PrimitiveTypeSymbol> boolType_;
    std::shared_ptr<PrimitiveTypeSymbol> voidType_;
    std::shared_ptr<PrimitiveTypeSymbol> shortType_;
    std::shared_ptr<PrimitiveTypeSymbol> ushortType_;
    std::shared_ptr<PrimitiveTypeSymbol> uintType_;
    std::shared_ptr<PrimitiveTypeSymbol> longType_;
    std::shared_ptr<PrimitiveTypeSymbol> ulongType_;
    std::shared_ptr<ErrorTypeSymbol> errorType_;
    std::shared_ptr<NullTypeSymbol> nullType_;
    std::shared_ptr<StringObjectSymbol> stringObjectType_;

    // Scope stack for push/pop during construction
    std::vector<ScopePtr> scopeStack_;

    // Monomorphization cache: key -> specialized FunctionSymbol
    std::unordered_map<std::string, std::shared_ptr<FunctionSymbol>> monomorphizationCache_;

    // Initialize primitive types
    void registerPrimitives();

public:
    // Register platform-sized type aliases (size_t, intptr_t, etc.)
    // Must be called after construction, once target pointer width is known.
    void registerPlatformTypes(unsigned pointerWidthBytes);
};

} // namespace mingus
