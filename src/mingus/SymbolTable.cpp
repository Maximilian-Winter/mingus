// ============================================================================
// SymbolTable.cpp — Root owner, scope navigation, type interning
// ============================================================================

#include "mingus/SymbolTable.h"

namespace mingus {

// ============================================================================
// Construction — pre-register all primitive types
// ============================================================================

SymbolTable::SymbolTable() {
    rootScope_ = std::make_shared<GlobalScope>();
    currentScope_ = rootScope_;
    registerPrimitives();
}

void SymbolTable::registerPrimitives() {
    // Create canonical primitive type instances
    intType_    = std::make_shared<PrimitiveTypeSymbol>("int",    PrimitiveKind::Int,    4);
    doubleType_ = std::make_shared<PrimitiveTypeSymbol>("double", PrimitiveKind::Double, 8);
    floatType_  = std::make_shared<PrimitiveTypeSymbol>("float",  PrimitiveKind::Float,  4);
    byteType_   = std::make_shared<PrimitiveTypeSymbol>("byte",   PrimitiveKind::Byte,   1);
    charType_   = std::make_shared<PrimitiveTypeSymbol>("char",   PrimitiveKind::Char,   1);
    stringType_ = std::make_shared<PrimitiveTypeSymbol>("string", PrimitiveKind::String, 8);
    boolType_   = std::make_shared<PrimitiveTypeSymbol>("bool",   PrimitiveKind::Bool,   1);
    voidType_   = std::make_shared<PrimitiveTypeSymbol>("void",   PrimitiveKind::Void,   0);
    shortType_  = std::make_shared<PrimitiveTypeSymbol>("short",  PrimitiveKind::Short,  2);
    ushortType_ = std::make_shared<PrimitiveTypeSymbol>("ushort", PrimitiveKind::UShort, 2);
    uintType_   = std::make_shared<PrimitiveTypeSymbol>("uint",   PrimitiveKind::UInt,   4);
    longType_   = std::make_shared<PrimitiveTypeSymbol>("long",   PrimitiveKind::Long,   8);
    ulongType_  = std::make_shared<PrimitiveTypeSymbol>("ulong",  PrimitiveKind::ULong,  8);
    errorType_  = std::make_shared<ErrorTypeSymbol>();
    nullType_   = std::make_shared<NullTypeSymbol>();
    stringObjectType_ = std::make_shared<StringObjectSymbol>();

    // Register in the interning map
    types_["int"]     = intType_;
    types_["double"]  = doubleType_;
    types_["float"]   = floatType_;
    types_["byte"]    = byteType_;
    types_["char"]    = charType_;
    types_["string"]  = stringType_;
    types_["bool"]    = boolType_;
    types_["void"]    = voidType_;
    types_["short"]   = shortType_;
    types_["ushort"]  = ushortType_;
    types_["uint"]    = uintType_;
    types_["long"]    = longType_;
    types_["ulong"]   = ulongType_;
    types_["<error>"] = errorType_;
    types_["null"]    = nullType_;
    types_["String"]  = stringObjectType_;
}

// ============================================================================
// Root scope
// ============================================================================

std::shared_ptr<GlobalScope> SymbolTable::getRootScope() const {
    return rootScope_;
}

// ============================================================================
// Scope navigation
// ============================================================================

ScopePtr SymbolTable::getCurrentScope() const {
    return currentScope_;
}

void SymbolTable::setCurrentScope(const ScopePtr& scope) {
    currentScope_ = scope;
}

void SymbolTable::pushScope(const ScopePtr& scope) {
    scopeStack_.push_back(currentScope_);
    // Wire the enclosing scope if not already set.
    // This ensures SymbolWithScope entries (functions, modules, classes)
    // have their enclosing chain connected for name resolution.
    if (!scope->getEnclosingScope()) {
        scope->setEnclosingScope(currentScope_);
    }
    currentScope_ = scope;
}

void SymbolTable::popScope() {
    if (!scopeStack_.empty()) {
        currentScope_ = scopeStack_.back();
        scopeStack_.pop_back();
    }
}

// ============================================================================
// Symbol definition
// ============================================================================

void SymbolTable::defineSymbol(const SymbolPtr& symbol) {
    if (!symbol || !currentScope_) return;

    // Set the symbol's defining scope
    symbol->setSymbolScope(currentScope_);

    // Set insertion order
    static int globalOrder = 0;
    symbol->setInsertionOrderNumber(globalOrder++);

    // Define in current scope
    currentScope_->define(symbol);
}

// ============================================================================
// Primitive type accessors
// ============================================================================

std::shared_ptr<PrimitiveTypeSymbol> SymbolTable::getIntType() const    { return intType_; }
std::shared_ptr<PrimitiveTypeSymbol> SymbolTable::getDoubleType() const { return doubleType_; }
std::shared_ptr<PrimitiveTypeSymbol> SymbolTable::getFloatType() const  { return floatType_; }
std::shared_ptr<PrimitiveTypeSymbol> SymbolTable::getByteType() const   { return byteType_; }
std::shared_ptr<PrimitiveTypeSymbol> SymbolTable::getCharType() const   { return charType_; }
std::shared_ptr<PrimitiveTypeSymbol> SymbolTable::getStringType() const { return stringType_; }
std::shared_ptr<PrimitiveTypeSymbol> SymbolTable::getBoolType() const   { return boolType_; }
std::shared_ptr<PrimitiveTypeSymbol> SymbolTable::getVoidType() const   { return voidType_; }
std::shared_ptr<PrimitiveTypeSymbol> SymbolTable::getShortType() const  { return shortType_; }
std::shared_ptr<PrimitiveTypeSymbol> SymbolTable::getUShortType() const { return ushortType_; }
std::shared_ptr<PrimitiveTypeSymbol> SymbolTable::getUIntType() const   { return uintType_; }
std::shared_ptr<PrimitiveTypeSymbol> SymbolTable::getLongType() const   { return longType_; }
std::shared_ptr<PrimitiveTypeSymbol> SymbolTable::getULongType() const  { return ulongType_; }
std::shared_ptr<ErrorTypeSymbol> SymbolTable::getErrorType() const      { return errorType_; }
std::shared_ptr<NullTypeSymbol> SymbolTable::getNullType() const        { return nullType_; }
std::shared_ptr<StringObjectSymbol> SymbolTable::getStringObjectType() const { return stringObjectType_; }

// ============================================================================
// Type interning — compound types
// ============================================================================

std::shared_ptr<PointerTypeSymbol> SymbolTable::getPointerType(TypeSymbolPtr baseType) {
    auto key = baseType->getInterningKey() + "*";
    auto it = types_.find(key);
    if (it != types_.end()) {
        return std::dynamic_pointer_cast<PointerTypeSymbol>(it->second);
    }
    auto ptrType = std::make_shared<PointerTypeSymbol>(std::move(baseType));
    types_[key] = ptrType;
    return ptrType;
}

std::shared_ptr<PointerTypeSymbol> SymbolTable::getSharedPointerType(TypeSymbolPtr baseType) {
    auto key = "shared " + baseType->getInterningKey() + "*";
    auto it = types_.find(key);
    if (it != types_.end()) {
        return std::dynamic_pointer_cast<PointerTypeSymbol>(it->second);
    }
    auto ptrType = std::make_shared<PointerTypeSymbol>(std::move(baseType), /*isShared=*/true);
    types_[key] = ptrType;
    return ptrType;
}

std::shared_ptr<ArrayTypeSymbol> SymbolTable::getArrayType(
    TypeSymbolPtr elementType, int size)
{
    auto key = elementType->getInterningKey() + "[" + std::to_string(size) + "]";
    auto it = types_.find(key);
    if (it != types_.end()) {
        return std::dynamic_pointer_cast<ArrayTypeSymbol>(it->second);
    }
    auto arrType = std::make_shared<ArrayTypeSymbol>(std::move(elementType), size);
    types_[key] = arrType;
    return arrType;
}

std::shared_ptr<TupleTypeSymbol> SymbolTable::getTupleType(
    std::vector<TypeSymbolPtr> elementTypes)
{
    // Build interning key
    std::string key = "(";
    for (size_t i = 0; i < elementTypes.size(); i++) {
        if (i > 0) key += ",";
        key += elementTypes[i] ? elementTypes[i]->getInterningKey() : "?";
    }
    key += ")";

    auto it = types_.find(key);
    if (it != types_.end()) {
        return std::dynamic_pointer_cast<TupleTypeSymbol>(it->second);
    }
    auto tupType = std::make_shared<TupleTypeSymbol>(std::move(elementTypes));
    types_[key] = tupType;
    return tupType;
}

std::shared_ptr<FunctionTypeSymbol> SymbolTable::getFunctionType(
    std::vector<FunctionTypeSymbol::ParameterInfo> params,
    TypeSymbolPtr returnType,
    bool isVariadic)
{
    // Build interning key
    std::string key = "(";
    for (size_t i = 0; i < params.size(); i++) {
        if (i > 0) key += ",";
        key += params[i].type ? params[i].type->getInterningKey() : "?";
        if (params[i].isReference) key += "&";
    }
    key += ")=>";
    key += returnType ? returnType->getInterningKey() : "void";

    auto it = types_.find(key);
    if (it != types_.end()) {
        return std::dynamic_pointer_cast<FunctionTypeSymbol>(it->second);
    }
    auto funcType = std::make_shared<FunctionTypeSymbol>(
        std::move(params), std::move(returnType), isVariadic);
    types_[key] = funcType;
    return funcType;
}

std::shared_ptr<ReferenceTypeSymbol> SymbolTable::getReferenceType(TypeSymbolPtr baseType) {
    auto key = baseType->getInterningKey() + "&";
    auto it = types_.find(key);
    if (it != types_.end()) {
        return std::dynamic_pointer_cast<ReferenceTypeSymbol>(it->second);
    }
    auto refType = std::make_shared<ReferenceTypeSymbol>(std::move(baseType));
    types_[key] = refType;
    return refType;
}

// ============================================================================
// User-defined type registration
// ============================================================================

void SymbolTable::registerType(const std::string& name, TypeSymbolPtr type) {
    types_[name] = std::move(type);
}

TypeSymbolPtr SymbolTable::resolveType(const std::string& name) const {
    auto it = types_.find(name);
    if (it != types_.end()) return it->second;
    return nullptr;
}

// ============================================================================
// Type compatibility
// ============================================================================

bool SymbolTable::isCompatible(TypeSymbol* from, TypeSymbol* to) const {
    if (!from || !to) return false;

    // 1. Same pointer → identical type
    if (from == to) return true;

    // 2. ErrorType → always compatible (prevents cascading errors)
    if (from->is<ErrorTypeSymbol>() || to->is<ErrorTypeSymbol>()) return true;

    // 3. Null → pointer or function type
    if (from->is<NullTypeSymbol>()) {
        return to->is<PointerTypeSymbol>() || to->is<FunctionTypeSymbol>();
    }

    // 4. Numeric widening (rank-based)
    if (auto* fromPrim = from->as<PrimitiveTypeSymbol>()) {
        if (auto* toPrim = to->as<PrimitiveTypeSymbol>()) {
            // Integer bit-width rank
            auto intRank = [](PrimitiveKind k) -> int {
                switch (k) {
                    case PrimitiveKind::Byte:   case PrimitiveKind::Char:   return 8;
                    case PrimitiveKind::Short:  case PrimitiveKind::UShort: return 16;
                    case PrimitiveKind::Int:    case PrimitiveKind::UInt:   return 32;
                    case PrimitiveKind::Long:   case PrimitiveKind::ULong:  return 64;
                    default: return 0;
                }
            };
            int fromRank = intRank(fromPrim->primitiveKind);
            int toRank = intRank(toPrim->primitiveKind);

            // Any integer ↔ any integer (widening, narrowing, cross-sign all allowed)
            // Warnings for narrowing/cross-sign can be emitted by TypeChecker
            if (fromRank > 0 && toRank > 0) return true;

            // Any integer → float or double
            if (fromRank > 0 && toPrim->isFloating()) return true;
            // float/double → any integer (explicit-style implicit narrowing)
            if (fromPrim->isFloating() && toRank > 0) return true;
            // float → double
            if (fromPrim->primitiveKind == PrimitiveKind::Float &&
                toPrim->primitiveKind == PrimitiveKind::Double) return true;
        }
    }

    // 4b. StringObject ↔ string primitive (bidirectional implicit conversion)
    if (from->is<StringObjectSymbol>()) {
        if (auto* toPrim = to->as<PrimitiveTypeSymbol>())
            if (toPrim->primitiveKind == PrimitiveKind::String) return true;
    }
    if (auto* fromPrim2 = from->as<PrimitiveTypeSymbol>()) {
        if (fromPrim2->primitiveKind == PrimitiveKind::String)
            if (to->is<StringObjectSymbol>()) return true;
    }

    // 5. Enum ↔ underlying (bidirectional, with widening)
    if (auto* fromEnum = from->as<EnumSymbol>()) {
        auto* underlying = fromEnum->underlyingType
            ? fromEnum->underlyingType.get() : getIntType().get();
        if (underlying == to) return true;
        if (isCompatible(underlying, to)) return true;
    }
    if (auto* toEnum = to->as<EnumSymbol>()) {
        auto* underlying = toEnum->underlyingType
            ? toEnum->underlyingType.get() : getIntType().get();
        if (underlying == from) return true;
        if (isCompatible(from, underlying)) return true;
    }

    // 6. Interface upcast: Dog* → Drawable* if Dog implements Drawable
    //    shared-ness must match (no cross-sharing)
    if (auto* toPtr = to->as<PointerTypeSymbol>()) {
        if (auto* toIface = toPtr->baseType->as<InterfaceSymbol>()) {
            if (auto* fromPtr = from->as<PointerTypeSymbol>()) {
                if (fromPtr->isShared == toPtr->isShared) {
                    if (auto* fromClass = fromPtr->baseType->as<ClassSymbol>()) {
                        for (const auto& iface : fromClass->implementedInterfaces) {
                            if (iface.get() == toIface) return true;
                        }
                    }
                }
            }
        }
    }

    // 7. byte* = universal pointer (both directions) — raw pointers only
    if (auto* fromPtr = from->as<PointerTypeSymbol>()) {
        if (auto* toPtr = to->as<PointerTypeSymbol>()) {
            if (fromPtr->isShared == toPtr->isShared) {
                auto* fromBase = fromPtr->baseType->as<PrimitiveTypeSymbol>();
                auto* toBase = toPtr->baseType->as<PrimitiveTypeSymbol>();
                if ((fromBase && fromBase->primitiveKind == PrimitiveKind::Byte) ||
                    (toBase && toBase->primitiveKind == PrimitiveKind::Byte)) {
                    return true;
                }
            }
        }
    }

    // 8. Inheritance: Derived* → Base* (shared-ness must match)
    if (auto* fromPtr = from->as<PointerTypeSymbol>()) {
        if (auto* toPtr = to->as<PointerTypeSymbol>()) {
            if (fromPtr->isShared == toPtr->isShared) {
                auto* fromClass = fromPtr->baseType->as<ClassSymbol>();
                auto* toClass = toPtr->baseType->as<ClassSymbol>();
                if (fromClass && toClass) {
                    auto* current = fromClass->resolvedBaseClass;
                    while (current) {
                        if (current == toClass) return true;
                        current = current->resolvedBaseClass;
                    }
                }
            }
        }
    }

    // 9. T → T& (implicit address-of at call site)
    if (auto* toRef = to->as<ReferenceTypeSymbol>()) {
        if (isCompatible(from, toRef->baseType.get())) return true;
    }

    return false;
}

// ============================================================================
// Raw type access
// ============================================================================

const std::unordered_map<std::string, TypeSymbolPtr>& SymbolTable::getAllTypes() const {
    return types_;
}

} // namespace mingus
