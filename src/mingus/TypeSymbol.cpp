// ============================================================================
// TypeSymbol.cpp — TypeSymbol hierarchy implementations
// ============================================================================

#include "mingus/TypeSymbol.h"

namespace mingus {

// ============================================================================
// TypeSymbol
// ============================================================================

TypeSymbol::TypeSymbol(const std::string& name, bool isPrimaryType)
    : SymbolWithScope(name)
    , isPrimaryType(isPrimaryType) {}

std::string TypeSymbol::getTypeDescription() const {
    return getName();
}

std::string TypeSymbol::getInterningKey() const {
    return getName();
}

// ============================================================================
// PrimitiveTypeSymbol
// ============================================================================

PrimitiveTypeSymbol::PrimitiveTypeSymbol(
    const std::string& name, PrimitiveKind kind, int size)
    : TypeSymbol(name, /*isPrimaryType=*/true)
    , primitiveKind(kind)
{
    sizeInBytes = size;
}

std::string PrimitiveTypeSymbol::getTypeDescription() const {
    return getName();
}

std::string PrimitiveTypeSymbol::getInterningKey() const {
    return getName();
}

bool PrimitiveTypeSymbol::isNumeric() const {
    return primitiveKind == PrimitiveKind::Int
        || primitiveKind == PrimitiveKind::Double
        || primitiveKind == PrimitiveKind::Float
        || primitiveKind == PrimitiveKind::Byte
        || primitiveKind == PrimitiveKind::Short
        || primitiveKind == PrimitiveKind::UShort
        || primitiveKind == PrimitiveKind::UInt
        || primitiveKind == PrimitiveKind::Long
        || primitiveKind == PrimitiveKind::ULong;
}

bool PrimitiveTypeSymbol::isIntegral() const {
    return primitiveKind == PrimitiveKind::Int
        || primitiveKind == PrimitiveKind::Byte
        || primitiveKind == PrimitiveKind::Char
        || primitiveKind == PrimitiveKind::Short
        || primitiveKind == PrimitiveKind::UShort
        || primitiveKind == PrimitiveKind::UInt
        || primitiveKind == PrimitiveKind::Long
        || primitiveKind == PrimitiveKind::ULong;
}

bool PrimitiveTypeSymbol::isFloating() const {
    return primitiveKind == PrimitiveKind::Double
        || primitiveKind == PrimitiveKind::Float;
}

bool PrimitiveTypeSymbol::isUnsigned() const {
    return primitiveKind == PrimitiveKind::Byte
        || primitiveKind == PrimitiveKind::UShort
        || primitiveKind == PrimitiveKind::UInt
        || primitiveKind == PrimitiveKind::ULong;
}

// ============================================================================
// PointerTypeSymbol
// ============================================================================

PointerTypeSymbol::PointerTypeSymbol(TypeSymbolPtr baseType, bool isShared, bool isConst)
    : TypeSymbol(std::string(isConst ? "const " : "") +
                 std::string(isShared ? "shared " : "") +
                 baseType->getName() + "*",
                 /*isPrimaryType=*/false)
    , baseType(std::move(baseType))
    , isShared(isShared)
    , isConst(isConst)
{
    sizeInBytes = 8;  // ptr-sized (64-bit)
}

std::string PointerTypeSymbol::getTypeDescription() const {
    std::string desc;
    if (isConst) desc += "const ";
    if (isShared) desc += "shared ";
    desc += baseType->getTypeDescription() + "*";
    return desc;
}

std::string PointerTypeSymbol::getInterningKey() const {
    std::string key;
    if (isConst) key += "const ";
    if (isShared) key += "shared ";
    key += baseType->getInterningKey() + "*";
    return key;
}

// ============================================================================
// ArrayTypeSymbol
// ============================================================================

ArrayTypeSymbol::ArrayTypeSymbol(TypeSymbolPtr elementType, int size)
    : TypeSymbol(
        elementType->getName() + "[" + (size > 0 ? std::to_string(size) : "") + "]",
        /*isPrimaryType=*/false)
    , elementType(std::move(elementType))
    , arraySize(size)
{
    if (size > 0 && this->elementType) {
        sizeInBytes = size * this->elementType->sizeInBytes;
    } else {
        sizeInBytes = 8;  // dynamic array = pointer
    }
}

std::string ArrayTypeSymbol::getTypeDescription() const {
    if (arraySize > 0) {
        return elementType->getTypeDescription() + "[" + std::to_string(arraySize) + "]";
    }
    return elementType->getTypeDescription() + "[]";
}

std::string ArrayTypeSymbol::getInterningKey() const {
    return elementType->getInterningKey() + "[" + std::to_string(arraySize) + "]";
}

// ============================================================================
// TupleTypeSymbol
// ============================================================================

TupleTypeSymbol::TupleTypeSymbol(std::vector<TypeSymbolPtr> elementTypes)
    : TypeSymbol("tuple", /*isPrimaryType=*/false)
    , elementTypes(std::move(elementTypes))
{
    // Compute size as sum of element sizes (no padding in this model)
    sizeInBytes = 0;
    for (const auto& et : this->elementTypes) {
        if (et) sizeInBytes += et->sizeInBytes;
    }
}

std::string TupleTypeSymbol::getTypeDescription() const {
    std::string result = "(";
    for (size_t i = 0; i < elementTypes.size(); i++) {
        if (i > 0) result += ", ";
        result += elementTypes[i] ? elementTypes[i]->getTypeDescription() : "?";
    }
    result += ")";
    return result;
}

std::string TupleTypeSymbol::getInterningKey() const {
    std::string result = "(";
    for (size_t i = 0; i < elementTypes.size(); i++) {
        if (i > 0) result += ",";
        result += elementTypes[i] ? elementTypes[i]->getInterningKey() : "?";
    }
    result += ")";
    return result;
}

// ============================================================================
// FunctionTypeSymbol
// ============================================================================

FunctionTypeSymbol::FunctionTypeSymbol(
    std::vector<ParameterInfo> parameters,
    TypeSymbolPtr returnType,
    bool isVariadic)
    : TypeSymbol("func", /*isPrimaryType=*/false)
    , parameters(std::move(parameters))
    , returnType(std::move(returnType))
    , isVariadic(isVariadic)
{
    sizeInBytes = 16;  // fat pointer: { ptr, ptr } = 2 * 8
}

std::string FunctionTypeSymbol::getTypeDescription() const {
    std::string result = "(";
    for (size_t i = 0; i < parameters.size(); i++) {
        if (i > 0) result += ", ";
        if (parameters[i].type) {
            result += parameters[i].type->getTypeDescription();
        } else {
            result += "?";
        }
        if (parameters[i].isReference) result += "&";
    }
    if (isVariadic) {
        if (!parameters.empty()) result += ", ";
        result += "...";
    }
    result += ") => ";
    result += returnType ? returnType->getTypeDescription() : "void";
    return result;
}

std::string FunctionTypeSymbol::getInterningKey() const {
    std::string result = "(";
    for (size_t i = 0; i < parameters.size(); i++) {
        if (i > 0) result += ",";
        if (parameters[i].type) {
            result += parameters[i].type->getInterningKey();
        }
        if (parameters[i].isReference) result += "&";
    }
    result += ")=>";
    result += returnType ? returnType->getInterningKey() : "void";
    return result;
}

// ============================================================================
// ReferenceTypeSymbol
// ============================================================================

ReferenceTypeSymbol::ReferenceTypeSymbol(TypeSymbolPtr baseType)
    : TypeSymbol(baseType->getName() + "&", /*isPrimaryType=*/false)
    , baseType(std::move(baseType))
{
    sizeInBytes = 8;  // reference = pointer at the LLVM level
}

std::string ReferenceTypeSymbol::getTypeDescription() const {
    return baseType->getTypeDescription() + "&";
}

std::string ReferenceTypeSymbol::getInterningKey() const {
    return baseType->getInterningKey() + "&";
}

// ============================================================================
// ErrorTypeSymbol
// ============================================================================

ErrorTypeSymbol::ErrorTypeSymbol()
    : TypeSymbol("<error>", /*isPrimaryType=*/false) {}

std::string ErrorTypeSymbol::getTypeDescription() const {
    return "<error>";
}

std::string ErrorTypeSymbol::getInterningKey() const {
    return "<error>";
}

// ============================================================================
// NullTypeSymbol
// ============================================================================

NullTypeSymbol::NullTypeSymbol()
    : TypeSymbol("null", /*isPrimaryType=*/false) {}

std::string NullTypeSymbol::getTypeDescription() const {
    return "null";
}

std::string NullTypeSymbol::getInterningKey() const {
    return "null";
}

// ============================================================================
// StringObjectSymbol
// ============================================================================

StringObjectSymbol::StringObjectSymbol()
    : TypeSymbol("String", /*isPrimaryType=*/true)
{
    sizeInBytes = 16;  // { ptr(8), i32(4), i32(4) }
}

std::string StringObjectSymbol::getTypeDescription() const {
    return "String";
}

std::string StringObjectSymbol::getInterningKey() const {
    return "String";
}

// ============================================================================
// OpaqueTypeSymbol
// ============================================================================

OpaqueTypeSymbol::OpaqueTypeSymbol(const std::string& name)
    : TypeSymbol(name, /*isPrimaryType=*/false)
{
    sizeInBytes = 0;  // opaque — no known layout
}

std::string OpaqueTypeSymbol::getTypeDescription() const {
    return getName();
}

std::string OpaqueTypeSymbol::getInterningKey() const {
    return "opaque:" + getName();
}

// ============================================================================
// TypeParameterSymbol
// ============================================================================

TypeParameterSymbol::TypeParameterSymbol(const std::string& name)
    : TypeSymbol(name, /*isPrimaryType=*/false)
{
    sizeInBytes = 0;  // placeholder — no known size until monomorphized
}

std::string TypeParameterSymbol::getTypeDescription() const {
    return getName();
}

std::string TypeParameterSymbol::getInterningKey() const {
    return "typeparam:" + getName();
}

} // namespace mingus
