#pragma once

// ============================================================================
// TypeSymbol.h — Type-as-Symbol unification for Mingus V2
//
// TypeSymbol (extends SymbolWithScope — every type IS a scope)
// +-- PrimitiveTypeSymbol (int, double, float, byte, char, string, bool, void)
// +-- PointerTypeSymbol (T*)
// +-- ArrayTypeSymbol (T[N])
// +-- TupleTypeSymbol ((T1, T2, ...))
// +-- FunctionTypeSymbol ((T1) => R — closure/function pointer types)
// +-- ReferenceTypeSymbol (T& — transient, unwrapped during type resolution)
// +-- ErrorTypeSymbol (sentinel for error recovery)
// +-- NullTypeSymbol (compatible with pointers and closures)
//
// ClassSymbol, StructSymbol, EnumSymbol, InterfaceSymbol also extend
// TypeSymbol but are defined in Symbols.h (they depend on VariableSymbol
// and FunctionSymbol which are also in Symbols.h).
//
// The key design: there is NO separate Type hierarchy. TypeSymbol IS the type.
// VariableSymbol::getType() returns shared_ptr<TypeSymbol>.
// mapType(TypeSymbol*) is the single entry point for LLVM type mapping.
// ============================================================================

#include "Forward.h"
#include "Symbol.h"

#include <memory>
#include <string>
#include <vector>

namespace mingus {

// ============================================================================
// TypeSymbol — Base for ALL types
//
// Extends SymbolWithScope: every type IS a scope. For primitive types, the
// scope is empty. For class/struct types, the scope holds fields and methods.
// typeSymbol->resolve("member") is always valid — returns null for leaf types.
// ============================================================================

class TypeSymbol : public SymbolWithScope {
public:
    explicit TypeSymbol(const std::string& name, bool isPrimaryType = false);

    // Is this a built-in primitive type (int, double, etc.)?
    bool isPrimaryType;

    // Size in bytes (for layout calculations, 0 = unsized/abstract)
    int sizeInBytes = 0;

    // Human-readable type description
    virtual std::string getTypeDescription() const;

    // Structural equality key (for type interning)
    virtual std::string getInterningKey() const;
};

// ============================================================================
// PrimitiveTypeSymbol — int, double, float, byte, char, string, bool, void
// ============================================================================

class PrimitiveTypeSymbol : public TypeSymbol {
public:
    PrimitiveTypeSymbol(const std::string& name, PrimitiveKind kind, int sizeInBytes);

    PrimitiveKind primitiveKind;

    std::string getTypeDescription() const override;
    std::string getInterningKey() const override;

    // Convenience queries
    bool isNumeric() const;
    bool isIntegral() const;
    bool isFloating() const;
    bool isUnsigned() const;
};

// ============================================================================
// PointerTypeSymbol — T* or shared T*
// ============================================================================

class PointerTypeSymbol : public TypeSymbol {
public:
    explicit PointerTypeSymbol(TypeSymbolPtr baseType, bool isShared = false);

    TypeSymbolPtr baseType;
    bool isShared = false;  // true for shared T* (RC-managed)

    std::string getTypeDescription() const override;
    std::string getInterningKey() const override;
};

// ============================================================================
// ArrayTypeSymbol — T[N]
//
// size == -1 means dynamic/unsized array (pointer to heap).
// size > 0 means fixed-size stack array.
// ============================================================================

class ArrayTypeSymbol : public TypeSymbol {
public:
    ArrayTypeSymbol(TypeSymbolPtr elementType, int size);

    TypeSymbolPtr elementType;
    int arraySize;  // -1 = dynamic

    std::string getTypeDescription() const override;
    std::string getInterningKey() const override;
};

// ============================================================================
// TupleTypeSymbol — (T1, T2, ...)
// ============================================================================

class TupleTypeSymbol : public TypeSymbol {
public:
    explicit TupleTypeSymbol(std::vector<TypeSymbolPtr> elementTypes);

    std::vector<TypeSymbolPtr> elementTypes;

    std::string getTypeDescription() const override;
    std::string getInterningKey() const override;
};

// ============================================================================
// FunctionTypeSymbol — (T1, T2) => R
//
// This is the type of closure/function pointer values. Carries full per-param
// metadata via ParameterInfo, which is what fixes the three HIGH-severity bugs.
//
// FunctionTypeSymbol is NOT the same as FunctionSymbol:
// - FunctionSymbol = a named function declaration (IS a scope for its body)
// - FunctionTypeSymbol = the TYPE of a function/closure value
//
// A FunctionSymbol's corresponding FunctionTypeSymbol is derived from its
// parameter list. A VariableSymbol holding a closure has type FunctionTypeSymbol.
// ============================================================================

class FunctionTypeSymbol : public TypeSymbol {
public:
    struct ParameterInfo {
        TypeSymbolPtr type;
        std::string name;
        bool isReference = false;
    };

    FunctionTypeSymbol(
        std::vector<ParameterInfo> parameters,
        TypeSymbolPtr returnType,
        bool isVariadic = false
    );

    std::vector<ParameterInfo> parameters;
    TypeSymbolPtr returnType;
    bool isVariadic;

    std::string getTypeDescription() const override;
    std::string getInterningKey() const override;
};

// ============================================================================
// ReferenceTypeSymbol — T&
//
// Transient: created during type annotation parsing, unwrapped by TypeResolver
// in resolveParameters(). The base type goes onto VariableSymbol::type, and
// isReference is set on the VariableSymbol.
// ============================================================================

class ReferenceTypeSymbol : public TypeSymbol {
public:
    explicit ReferenceTypeSymbol(TypeSymbolPtr baseType);

    TypeSymbolPtr baseType;

    std::string getTypeDescription() const override;
    std::string getInterningKey() const override;
};

// ============================================================================
// ErrorTypeSymbol — Sentinel for error recovery
//
// isCompatible() returns true when either side is ErrorType, preventing
// cascading type errors after the first failure.
// ============================================================================

class ErrorTypeSymbol : public TypeSymbol {
public:
    ErrorTypeSymbol();

    std::string getTypeDescription() const override;
    std::string getInterningKey() const override;
};

// ============================================================================
// NullTypeSymbol — The type of `null`
//
// Compatible with PointerTypeSymbol and FunctionTypeSymbol (fat pointer zeroed).
// ============================================================================

class NullTypeSymbol : public TypeSymbol {
public:
    NullTypeSymbol();

    std::string getTypeDescription() const override;
    std::string getInterningKey() const override;
};

// ============================================================================
// StringObjectSymbol — Built-in String value type { ptr, i32, i32 }
//
// A managed string with length tracking. Layout: { data, len, cap }.
// Distinct from PrimitiveKind::String (which is raw char*).
// Implicit conversion both directions for C interop.
// ============================================================================

class StringObjectSymbol : public TypeSymbol {
public:
    StringObjectSymbol();

    std::string getTypeDescription() const override;
    std::string getInterningKey() const override;
};

// ============================================================================
// OpaqueTypeSymbol — Extern opaque type (FILE, HANDLE, SDL_Window, etc.)
//
// Used only through pointers. Has no fields, no size, no layout.
// mapType() produces an opaque LLVM StructType (no body set).
// ============================================================================

class OpaqueTypeSymbol : public TypeSymbol {
public:
    explicit OpaqueTypeSymbol(const std::string& name);

    std::string getTypeDescription() const override;
    std::string getInterningKey() const override;
};

} // namespace mingus
