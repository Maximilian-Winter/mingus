//================================================================================
// MINGUS v1 - Type Registry Implementation
//================================================================================

#include "mingus/sema/TypeRegistry.h"
#include "mingus/sema/Symbol.h"

namespace mingus {
namespace sema {

TypeRegistry::TypeRegistry()
    : INT_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int))
    , DOUBLE_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Double))
    , FLOAT_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Float))
    , BYTE_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Byte))
    , CHAR_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Char))
    , STRING_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::String))
    , BOOL_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Bool))
    , VOID_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Void))
    , ERROR_(std::make_shared<ErrorType>())
    , NULL_TYPE_(std::make_shared<NullType>())
{
}

TypePtr<PrimitiveType> TypeRegistry::getPrimitive(PrimitiveType::PrimitiveKind kind) const {
    switch (kind) {
        case PrimitiveType::PrimitiveKind::Int:    return INT_;
        case PrimitiveType::PrimitiveKind::Double: return DOUBLE_;
        case PrimitiveType::PrimitiveKind::Float:  return FLOAT_;
        case PrimitiveType::PrimitiveKind::Byte:   return BYTE_;
        case PrimitiveType::PrimitiveKind::Char:   return CHAR_;
        case PrimitiveType::PrimitiveKind::String: return STRING_;
        case PrimitiveType::PrimitiveKind::Bool:   return BOOL_;
        case PrimitiveType::PrimitiveKind::Void:   return VOID_;
    }
    return INT_; // unreachable
}

TypePtr<ReferenceType> TypeRegistry::getReferenceTo(TypePtr<Type> base) {
    return std::make_shared<ReferenceType>(base);
}

TypePtr<PointerType> TypeRegistry::getPointerTo(TypePtr<Type> base) {
    auto* key = base.get();
    auto it = pointerCache_.find(key);
    if (it != pointerCache_.end()) {
        return it->second;
    }
    auto ptr = std::make_shared<PointerType>(base);
    pointerCache_[key] = ptr;
    return ptr;
}

TypePtr<ArrayType> TypeRegistry::getArrayOf(TypePtr<Type> elementType, int size) {
    auto key = std::make_pair(elementType.get(), size);
    auto it = arrayCache_.find(key);
    if (it != arrayCache_.end()) {
        return it->second;
    }
    auto arr = std::make_shared<ArrayType>(elementType, size);
    arrayCache_[key] = arr;
    return arr;
}

TypePtr<TupleType> TypeRegistry::getTupleOf(TypeList<Type> elementTypes) {
    // Search existing tuples for structural match
    for (auto& existing : tupleCache_) {
        if (existing->elementTypes.size() != elementTypes.size()) continue;
        bool match = true;
        for (size_t i = 0; i < elementTypes.size(); ++i) {
            if (existing->elementTypes[i].get() != elementTypes[i].get()) {
                match = false;
                break;
            }
        }
        if (match) return existing;
    }
    auto tuple = std::make_shared<TupleType>(std::move(elementTypes));
    tupleCache_.push_back(tuple);
    return tuple;
}

TypePtr<FunctionType> TypeRegistry::getFunctionType(TypeList<Type> paramTypes,
                                                     TypePtr<Type> returnType) {
    // Search existing function types for structural match
    for (auto& existing : functionCache_) {
        if (existing->returnType.get() != returnType.get()) continue;
        if (existing->parameterTypes.size() != paramTypes.size()) continue;
        bool match = true;
        for (size_t i = 0; i < paramTypes.size(); ++i) {
            if (existing->parameterTypes[i].get() != paramTypes[i].get()) {
                match = false;
                break;
            }
        }
        if (match) return existing;
    }
    auto fn = std::make_shared<FunctionType>(std::move(paramTypes), returnType);
    functionCache_.push_back(fn);
    return fn;
}

TypePtr<UserType> TypeRegistry::getUserType(const std::string& name,
                                             Type::Kind kind,
                                             void* symbol) {
    auto it = userTypeCache_.find(symbol);
    if (it != userTypeCache_.end()) {
        return it->second;
    }
    auto ut = std::make_shared<UserType>(name, kind, symbol);
    userTypeCache_[symbol] = ut;
    return ut;
}

//================================================================================
// Type compatibility
//================================================================================

bool TypeRegistry::isNumericType(const Type* t) const {
    if (!t) return false;
    if (t->is<UserType>() && t->as<UserType>()->underlyingKind == Type::Kind::Enum) {
        auto* enumSym = static_cast<EnumSymbol*>(t->as<UserType>()->symbol);
        if (enumSym && enumSym->underlyingType)
            return isNumericType(enumSym->underlyingType.get());
        return true;  // default (int) is numeric
    }
    if (!t->is<PrimitiveType>()) return false;
    auto kind = t->as<PrimitiveType>()->kind;
    return kind == PrimitiveType::PrimitiveKind::Int
        || kind == PrimitiveType::PrimitiveKind::Double
        || kind == PrimitiveType::PrimitiveKind::Float
        || kind == PrimitiveType::PrimitiveKind::Byte
        || kind == PrimitiveType::PrimitiveKind::Char;
}

bool TypeRegistry::isIntegerType(const Type* t) const {
    if (!t) return false;
    if (t->is<UserType>() && t->as<UserType>()->underlyingKind == Type::Kind::Enum) {
        auto* enumSym = static_cast<EnumSymbol*>(t->as<UserType>()->symbol);
        if (enumSym && enumSym->underlyingType)
            return isIntegerType(enumSym->underlyingType.get());
        return true;  // default (int) is integer
    }
    if (!t->is<PrimitiveType>()) return false;
    auto kind = t->as<PrimitiveType>()->kind;
    return kind == PrimitiveType::PrimitiveKind::Int
        || kind == PrimitiveType::PrimitiveKind::Byte
        || kind == PrimitiveType::PrimitiveKind::Char;
}

bool TypeRegistry::isPointerType(const Type* t) const {
    return t && t->is<PointerType>();
}

bool TypeRegistry::isNumericWidening(const Type* from, const Type* to) const {
    if (!from || !to) return false;
    if (!from->is<PrimitiveType>() || !to->is<PrimitiveType>()) return false;

    auto fk = from->as<PrimitiveType>()->kind;
    auto tk = to->as<PrimitiveType>()->kind;

    using PK = PrimitiveType::PrimitiveKind;

    // Int -> Double, Int -> Float
    if (fk == PK::Int && (tk == PK::Double || tk == PK::Float)) return true;
    // Float -> Double
    if (fk == PK::Float && tk == PK::Double) return true;
    // Byte -> Int
    if (fk == PK::Byte && tk == PK::Int) return true;
    // Char -> Int
    if (fk == PK::Char && tk == PK::Int) return true;

    return false;
}

TypePtr<Type> TypeRegistry::getWiderType(const Type* a, const Type* b) const {
    if (!a || !b) return nullptr;
    if (a->is<ErrorType>()) return std::const_pointer_cast<Type>(b->shared_from_this());
    if (b->is<ErrorType>()) return std::const_pointer_cast<Type>(a->shared_from_this());

    if (a->equals(*b)) return std::const_pointer_cast<Type>(a->shared_from_this());

    // Unwrap enum types to their underlying type for widening comparison
    const Type* aUnderlying = a;
    const Type* bUnderlying = b;
    if (auto* ua = a->as<UserType>()) {
        if (ua->underlyingKind == Type::Kind::Enum) {
            auto* enumSym = static_cast<EnumSymbol*>(ua->symbol);
            if (enumSym && enumSym->underlyingType)
                aUnderlying = enumSym->underlyingType.get();
        }
    }
    if (auto* ub = b->as<UserType>()) {
        if (ub->underlyingKind == Type::Kind::Enum) {
            auto* enumSym = static_cast<EnumSymbol*>(ub->symbol);
            if (enumSym && enumSym->underlyingType)
                bUnderlying = enumSym->underlyingType.get();
        }
    }

    if (isNumericWidening(aUnderlying, bUnderlying))
        return std::const_pointer_cast<Type>(b->shared_from_this());
    if (isNumericWidening(bUnderlying, aUnderlying))
        return std::const_pointer_cast<Type>(a->shared_from_this());

    // Original non-unwrapped check (handles cases where neither is enum)
    if (isNumericWidening(a, b)) return std::const_pointer_cast<Type>(b->shared_from_this());
    if (isNumericWidening(b, a)) return std::const_pointer_cast<Type>(a->shared_from_this());

    return nullptr;
}

bool TypeRegistry::isCompatible(const Type* from, const Type* to) const {
    if (!from || !to) return false;

    // ErrorType is compatible with everything (error recovery)
    if (from->is<ErrorType>() || to->is<ErrorType>()) return true;

    // Exact match
    if (from->equals(*to)) return true;

    // Numeric widening
    if (isNumericWidening(from, to)) return true;

    // Null is compatible with any pointer type or function type (nullable callbacks)
    if (from->is<NullType>() && (to->is<PointerType>() || to->is<FunctionType>())) return true;

    // Enum is compatible with its underlying type (both directions)
    if (from->is<UserType>() && from->as<UserType>()->underlyingKind == Type::Kind::Enum) {
        auto* enumSym = static_cast<EnumSymbol*>(from->as<UserType>()->symbol);
        const Type* underlying = (enumSym && enumSym->underlyingType)
            ? enumSym->underlyingType.get() : INT_.get();
        if (to->equals(*underlying) || isCompatible(underlying, to)) {
            return true;
        }
    }
    if (to->is<UserType>() && to->as<UserType>()->underlyingKind == Type::Kind::Enum) {
        auto* enumSym = static_cast<EnumSymbol*>(to->as<UserType>()->symbol);
        const Type* underlying = (enumSym && enumSym->underlyingType)
            ? enumSym->underlyingType.get() : INT_.get();
        if (from->equals(*underlying) || isCompatible(from, underlying)) {
            return true;
        }
    }

    // Interface pointer compatibility: Dog* → Drawable* if Dog implements Drawable
    if (from->is<PointerType>() && to->is<PointerType>()) {
        auto* toBase = to->as<PointerType>()->baseType.get();
        auto* fromBase = from->as<PointerType>()->baseType.get();
        if (auto* toUser = toBase->as<UserType>()) {
            if (toUser->underlyingKind == Type::Kind::Interface) {
                if (auto* fromUser = fromBase->as<UserType>()) {
                    if (fromUser->underlyingKind == Type::Kind::Class) {
                        return isImplements(fromBase, toBase);
                    }
                }
            }
        }
    }

    // byte* is the universal pointer (like void* in C)
    if (from->is<PointerType>() && to->is<PointerType>()) {
        auto* toBase = to->as<PointerType>()->baseType.get();
        auto* fromBase = from->as<PointerType>()->baseType.get();
        if (toBase->is<PrimitiveType>() &&
            toBase->as<PrimitiveType>()->kind == PrimitiveType::PrimitiveKind::Byte) {
            return true;
        }
        if (fromBase->is<PrimitiveType>() &&
            fromBase->as<PrimitiveType>()->kind == PrimitiveType::PrimitiveKind::Byte) {
            return true;
        }
        // Inheritance: Derived* is compatible with Base*
        if (isSubclassOf(fromBase, toBase)) return true;
    }

    // Inheritance: Derived value compatible with Base value
    if (isSubclassOf(from, to)) return true;

    // ReferenceType compatibility
    if (from->is<ReferenceType>() && to->is<ReferenceType>()) {
        return isCompatible(from->as<ReferenceType>()->baseType.get(),
                           to->as<ReferenceType>()->baseType.get());
    }
    // T is compatible with T& (implicit address-of at call site)
    if (to->is<ReferenceType>()) {
        return isCompatible(from, to->as<ReferenceType>()->baseType.get());
    }

    return false;
}

bool TypeRegistry::isImplements(const Type* classType, const Type* ifaceType) const {
    auto* cu = classType->as<UserType>();
    auto* iu = ifaceType->as<UserType>();
    if (!cu || !iu) return false;
    if (cu->underlyingKind != Type::Kind::Class) return false;
    if (iu->underlyingKind != Type::Kind::Interface) return false;

    auto* cls = static_cast<ClassSymbol*>(cu->symbol);
    auto* iface = static_cast<InterfaceSymbol*>(iu->symbol);
    while (cls) {
        for (auto* impl : cls->implementedInterfaces) {
            if (impl == iface) return true;
        }
        cls = cls->baseClass;
    }
    return false;
}

bool TypeRegistry::isSubclassOf(const Type* derived, const Type* base) const {
    auto* derivedUser = derived->as<UserType>();
    auto* baseUser = base->as<UserType>();
    if (!derivedUser || !baseUser) return false;
    if (derivedUser->underlyingKind != Type::Kind::Class ||
        baseUser->underlyingKind != Type::Kind::Class) return false;

    auto* derivedClass = static_cast<ClassSymbol*>(derivedUser->symbol);
    auto* baseClass = static_cast<ClassSymbol*>(baseUser->symbol);

    // Walk the inheritance chain
    auto* current = derivedClass->baseClass;
    while (current) {
        if (current == baseClass) return true;
        current = current->baseClass;
    }
    return false;
}

} // namespace sema
} // namespace mingus
