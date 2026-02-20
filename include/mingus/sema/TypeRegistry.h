//================================================================================
// MINGUS v1 - Type Registry
// Interns types for pointer-equality comparison and provides factory methods
//================================================================================

#pragma once

#include "mingus/ast/Type.h"

#include <map>
#include <memory>
#include <vector>

namespace mingus {
namespace sema {

using namespace mingus::ast;

//================================================================================
// TypeRegistry - singleton type interning and type compatibility
//================================================================================
class TypeRegistry {
public:
    TypeRegistry();

    // Primitive singletons
    TypePtr<PrimitiveType> getInt() const { return INT_; }
    TypePtr<PrimitiveType> getDouble() const { return DOUBLE_; }
    TypePtr<PrimitiveType> getFloat() const { return FLOAT_; }
    TypePtr<PrimitiveType> getByte() const { return BYTE_; }
    TypePtr<PrimitiveType> getChar() const { return CHAR_; }
    TypePtr<PrimitiveType> getString() const { return STRING_; }
    TypePtr<PrimitiveType> getBool() const { return BOOL_; }
    TypePtr<PrimitiveType> getVoid() const { return VOID_; }

    // Special singletons
    TypePtr<ErrorType> getErrorType() const { return ERROR_; }
    TypePtr<NullType> getNullType() const { return NULL_TYPE_; }

    // Factory methods — return cached/interned instances
    TypePtr<PointerType> getPointerTo(TypePtr<Type> base);
    TypePtr<ArrayType> getArrayOf(TypePtr<Type> elementType, int size = -1);
    TypePtr<TupleType> getTupleOf(TypeList<Type> elementTypes);
    TypePtr<FunctionType> getFunctionType(TypeList<Type> paramTypes,
                                          TypePtr<Type> returnType);
    TypePtr<UserType> getUserType(const std::string& name, Type::Kind kind,
                                  void* symbol);
    TypePtr<ReferenceType> getReferenceTo(TypePtr<Type> base);

    // Resolve a PrimitiveKind to its singleton
    TypePtr<PrimitiveType> getPrimitive(PrimitiveType::PrimitiveKind kind) const;

    // Type compatibility checking
    bool isCompatible(const Type* from, const Type* to) const;
    bool isNumericWidening(const Type* from, const Type* to) const;
    TypePtr<Type> getWiderType(const Type* a, const Type* b) const;
    bool isNumericType(const Type* t) const;
    bool isIntegerType(const Type* t) const;
    bool isPointerType(const Type* t) const;
    bool isSubclassOf(const Type* derived, const Type* base) const;
    bool isImplements(const Type* classType, const Type* ifaceType) const;

private:
    // Primitive singletons
    TypePtr<PrimitiveType> INT_;
    TypePtr<PrimitiveType> DOUBLE_;
    TypePtr<PrimitiveType> FLOAT_;
    TypePtr<PrimitiveType> BYTE_;
    TypePtr<PrimitiveType> CHAR_;
    TypePtr<PrimitiveType> STRING_;
    TypePtr<PrimitiveType> BOOL_;
    TypePtr<PrimitiveType> VOID_;

    // Special types
    TypePtr<ErrorType> ERROR_;
    TypePtr<NullType> NULL_TYPE_;

    // Interning caches
    std::map<Type*, TypePtr<PointerType>> pointerCache_;
    std::map<std::pair<Type*, int>, TypePtr<ArrayType>> arrayCache_;
    std::vector<TypePtr<TupleType>> tupleCache_;
    std::vector<TypePtr<FunctionType>> functionCache_;
    std::map<void*, TypePtr<UserType>> userTypeCache_;
};

} // namespace sema
} // namespace mingus
