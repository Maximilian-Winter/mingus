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

void SymbolTable::registerPlatformTypes(unsigned pointerWidthBytes) {
    if (pointerWidthBytes == 8) {
        // 64-bit target
        types_["size_t"]    = ulongType_;   // uint64
        types_["ssize_t"]   = longType_;    // int64
        types_["intptr_t"]  = longType_;    // int64
        types_["uintptr_t"] = ulongType_;   // uint64
        types_["ptrdiff_t"] = longType_;    // int64
    } else {
        // 32-bit target
        types_["size_t"]    = uintType_;    // uint32
        types_["ssize_t"]   = intType_;     // int32
        types_["intptr_t"]  = intType_;     // int32
        types_["uintptr_t"] = uintType_;    // uint32
        types_["ptrdiff_t"] = intType_;     // int32
    }
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

std::shared_ptr<PointerTypeSymbol> SymbolTable::getConstPointerType(TypeSymbolPtr baseType) {
    auto key = "const " + baseType->getInterningKey() + "*";
    auto it = types_.find(key);
    if (it != types_.end()) {
        return std::dynamic_pointer_cast<PointerTypeSymbol>(it->second);
    }
    auto ptrType = std::make_shared<PointerTypeSymbol>(std::move(baseType),
                                                        /*isShared=*/false,
                                                        /*isConst=*/true);
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

    // 2b. TypeParameterSymbol → compatible with anything (deferred to monomorphization)
    if (from->is<TypeParameterSymbol>() || to->is<TypeParameterSymbol>()) return true;

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

    // 4c. string ↔ byte* / const byte* (string is effectively char*/byte* at LLVM level)
    if (auto* fromPrim3 = from->as<PrimitiveTypeSymbol>()) {
        if (fromPrim3->primitiveKind == PrimitiveKind::String) {
            if (auto* toPtr = to->as<PointerTypeSymbol>()) {
                if (auto* toBase = toPtr->baseType->as<PrimitiveTypeSymbol>()) {
                    if (toBase->primitiveKind == PrimitiveKind::Byte) return true;
                }
            }
        }
    }
    if (auto* toPrim3 = to->as<PrimitiveTypeSymbol>()) {
        if (toPrim3->primitiveKind == PrimitiveKind::String) {
            if (auto* fromPtr3 = from->as<PointerTypeSymbol>()) {
                if (!fromPtr3->isConst) {  // byte* → string OK, const byte* → string rejected
                    if (auto* fromBase = fromPtr3->baseType->as<PrimitiveTypeSymbol>()) {
                        if (fromBase->primitiveKind == PrimitiveKind::Byte) return true;
                    }
                }
            }
        }
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

    // 5b. Const pointer safety
    //     const T* → T* is REJECTED (cannot strip const)
    //     T* → const T* is OK (widening — check base compatibility)
    if (auto* fromPtr = from->as<PointerTypeSymbol>()) {
        if (auto* toPtr = to->as<PointerTypeSymbol>()) {
            if (fromPtr->isConst && !toPtr->isConst) {
                return false;  // Cannot strip const
            }
            if (!fromPtr->isConst && toPtr->isConst) {
                return isCompatible(fromPtr->baseType.get(), toPtr->baseType.get());
            }
        }
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

// ============================================================================
// Generics — Monomorphization cache
// ============================================================================

static std::string buildMonoKey(FunctionSymbol* tmpl,
                                 const std::vector<TypeSymbolPtr>& typeArgs) {
    std::string key = tmpl->getQualifiedName();
    for (auto& ta : typeArgs) {
        key += "$" + ta->getInterningKey();
    }
    return key;
}

std::shared_ptr<FunctionSymbol> SymbolTable::getOrCreateMonomorphization(
    FunctionSymbol* tmpl, const std::vector<TypeSymbolPtr>& typeArgs)
{
    auto key = buildMonoKey(tmpl, typeArgs);
    auto it = monomorphizationCache_.find(key);
    if (it != monomorphizationCache_.end()) return it->second;

    // Build type substitution map
    std::unordered_map<std::string, TypeSymbolPtr> subst;
    for (size_t i = 0; i < tmpl->typeParameterNames.size(); i++) {
        subst[tmpl->typeParameterNames[i]] = typeArgs[i];
    }

    // Create specialized FunctionSymbol
    auto mono = std::make_shared<FunctionSymbol>(tmpl->getName());
    mono->genericTemplate = tmpl;
    mono->typeArguments = typeArgs;
    mono->genericASTNode = tmpl->genericASTNode;

    // Substitute parameter types
    for (auto& origParam : tmpl->parameters) {
        auto newParam = std::make_shared<VariableSymbol>(
            origParam->getName(), substituteType(origParam->getType(), subst));
        newParam->isReference = origParam->isReference;
        mono->parameters.push_back(newParam);
    }

    // Substitute return type
    mono->returnType = substituteType(tmpl->returnType, subst);

    // Copy other properties
    mono->isMethod = tmpl->isMethod;
    mono->isStatic = tmpl->isStatic;
    mono->hasThisParam = tmpl->hasThisParam;
    mono->isVariadic = tmpl->isVariadic;

    monomorphizationCache_[key] = mono;
    return mono;
}

bool SymbolTable::hasMonomorphization(
    FunctionSymbol* tmpl, const std::vector<TypeSymbolPtr>& typeArgs) const
{
    auto key = buildMonoKey(tmpl, typeArgs);
    return monomorphizationCache_.find(key) != monomorphizationCache_.end();
}

std::vector<std::shared_ptr<FunctionSymbol>> SymbolTable::getAllMonomorphizations() const {
    std::vector<std::shared_ptr<FunctionSymbol>> result;
    result.reserve(monomorphizationCache_.size());
    for (auto& kv : monomorphizationCache_) {
        result.push_back(kv.second);
    }
    return result;
}

TypeSymbolPtr SymbolTable::substituteType(
    TypeSymbolPtr type,
    const std::unordered_map<std::string, TypeSymbolPtr>& subst) const
{
    if (!type) return nullptr;

    // Direct substitution: T → concrete type
    if (auto* tp = type->as<TypeParameterSymbol>()) {
        auto it = subst.find(tp->getName());
        if (it != subst.end()) return it->second;
        return type;
    }

    // Recursive substitution for compound types
    if (auto* ptr = type->as<PointerTypeSymbol>()) {
        auto newBase = substituteType(ptr->baseType, subst);
        if (newBase == ptr->baseType) return type;
        if (ptr->isShared) {
            // Need to cast away const for the non-const factory method
            return const_cast<SymbolTable*>(this)->getSharedPointerType(newBase);
        }
        if (ptr->isConst) {
            return const_cast<SymbolTable*>(this)->getConstPointerType(newBase);
        }
        return const_cast<SymbolTable*>(this)->getPointerType(newBase);
    }

    if (auto* arr = type->as<ArrayTypeSymbol>()) {
        auto newElem = substituteType(arr->elementType, subst);
        if (newElem == arr->elementType) return type;
        return const_cast<SymbolTable*>(this)->getArrayType(newElem, arr->arraySize);
    }

    if (auto* tup = type->as<TupleTypeSymbol>()) {
        std::vector<TypeSymbolPtr> newElems;
        bool changed = false;
        for (auto& e : tup->elementTypes) {
            auto ne = substituteType(e, subst);
            newElems.push_back(ne);
            if (ne != e) changed = true;
        }
        if (!changed) return type;
        return const_cast<SymbolTable*>(this)->getTupleType(std::move(newElems));
    }

    if (auto* fn = type->as<FunctionTypeSymbol>()) {
        std::vector<FunctionTypeSymbol::ParameterInfo> newParams;
        bool changed = false;
        for (auto& p : fn->parameters) {
            auto nt = substituteType(p.type, subst);
            newParams.push_back({nt, p.name, p.isReference});
            if (nt != p.type) changed = true;
        }
        auto newRet = substituteType(fn->returnType, subst);
        if (newRet != fn->returnType) changed = true;
        if (!changed) return type;
        return const_cast<SymbolTable*>(this)->getFunctionType(
            std::move(newParams), newRet, fn->isVariadic);
    }

    if (auto* ref = type->as<ReferenceTypeSymbol>()) {
        auto newBase = substituteType(ref->baseType, subst);
        if (newBase == ref->baseType) return type;
        return const_cast<SymbolTable*>(this)->getReferenceType(newBase);
    }

    // Primitive, class, struct, enum, etc. — no substitution needed
    return type;
}

// ============================================================================
// Generics — Struct monomorphization
// ============================================================================

std::shared_ptr<StructSymbol> SymbolTable::getOrCreateStructMonomorphization(
    StructSymbol* tmpl, const std::vector<TypeSymbolPtr>& typeArgs)
{
    // Build cache key
    std::string key = "struct:" + tmpl->getName();
    for (auto& ta : typeArgs) key += "$" + ta->getInterningKey();

    auto it = structMonoCache_.find(key);
    if (it != structMonoCache_.end()) return it->second;

    // Build substitution map
    std::unordered_map<std::string, TypeSymbolPtr> subst;
    for (size_t i = 0; i < tmpl->typeParameterNames.size(); i++) {
        subst[tmpl->typeParameterNames[i]] = typeArgs[i];
    }

    // Build mangled name: Pair$G_int_double
    std::string monoName = tmpl->getName() + "$G";
    for (auto& ta : typeArgs) monoName += "_" + ta->getName();

    auto mono = std::make_shared<StructSymbol>(monoName);
    mono->genericTemplate = tmpl;
    mono->typeArguments = typeArgs;
    mono->genericASTNode = tmpl->genericASTNode;
    mono->isPacked = tmpl->isPacked;
    mono->alignment = tmpl->alignment;

    // Substitute field types and add to scope for resolve()
    for (auto& origField : tmpl->fields) {
        auto newField = std::make_shared<VariableSymbol>(
            origField->getName(), substituteType(origField->getType(), subst));
        newField->fieldIndex = origField->fieldIndex;
        mono->fields.push_back(newField);
        mono->define(newField);
    }

    registerType(monoName, mono);
    structMonoCache_[key] = mono;
    return mono;
}

std::vector<std::shared_ptr<StructSymbol>> SymbolTable::getAllStructMonomorphizations() const {
    std::vector<std::shared_ptr<StructSymbol>> result;
    result.reserve(structMonoCache_.size());
    for (auto& kv : structMonoCache_) result.push_back(kv.second);
    return result;
}

// ============================================================================
// Generics — Class monomorphization
// ============================================================================

std::shared_ptr<ClassSymbol> SymbolTable::getOrCreateClassMonomorphization(
    ClassSymbol* tmpl, const std::vector<TypeSymbolPtr>& typeArgs)
{
    std::string key = "class:" + tmpl->getName();
    for (auto& ta : typeArgs) key += "$" + ta->getInterningKey();

    auto it = classMonoCache_.find(key);
    if (it != classMonoCache_.end()) return it->second;

    std::unordered_map<std::string, TypeSymbolPtr> subst;
    for (size_t i = 0; i < tmpl->typeParameterNames.size(); i++) {
        subst[tmpl->typeParameterNames[i]] = typeArgs[i];
    }

    std::string monoName = tmpl->getName() + "$G";
    for (auto& ta : typeArgs) monoName += "_" + ta->getName();

    auto mono = std::make_shared<ClassSymbol>(monoName, tmpl->getEnclosingScope());
    mono->genericTemplate = tmpl;
    mono->typeArguments = typeArgs;
    mono->genericASTNode = tmpl->genericASTNode;
    mono->isAbstract = tmpl->isAbstract;

    // Set symbolScope_ so getQualifiedName() can walk Module_Class_member
    mono->setSymbolScope(tmpl->getSymbolScope());

    // Substitute fields
    for (auto& origField : tmpl->fields) {
        auto newField = std::make_shared<VariableSymbol>(
            origField->getName(), substituteType(origField->getType(), subst));
        newField->fieldIndex = origField->fieldIndex;
        mono->fields.push_back(newField);
        mono->allFields.push_back(newField);
        mono->define(newField);
    }

    // Monomorphize constructors
    for (auto& origCtor : tmpl->constructors) {
        auto monoCtor = std::make_shared<ConstructorSymbol>(monoName);
        monoCtor->isMethod = true;
        monoCtor->hasThisParam = true;
        monoCtor->setSymbolScope(mono);  // For getQualifiedName() → "Module_Class_constructor"
        for (auto& origParam : origCtor->parameters) {
            auto newParam = std::make_shared<VariableSymbol>(
                origParam->getName(), substituteType(origParam->getType(), subst));
            newParam->isReference = origParam->isReference;
            monoCtor->parameters.push_back(newParam);
        }
        mono->constructors.push_back(monoCtor);
    }

    // Monomorphize destructor
    if (tmpl->destructor) {
        auto monoDtor = std::make_shared<DestructorSymbol>(monoName);
        monoDtor->isMethod = true;
        monoDtor->hasThisParam = true;
        monoDtor->setSymbolScope(mono);  // For getQualifiedName() → "Module_Class_destructor"
        mono->destructor = monoDtor;
    }

    // Monomorphize methods — also add to function mono cache for body emission
    for (auto& sym : tmpl->getAllSymbols()) {
        auto* origMethod = sym->as<FunctionSymbol>();
        if (!origMethod || !origMethod->isMethod) continue;
        if (origMethod->is<ConstructorSymbol>() || origMethod->is<DestructorSymbol>()) continue;

        auto monoMethod = std::make_shared<MethodSymbol>(origMethod->getName());
        monoMethod->genericTemplate = origMethod;
        monoMethod->typeArguments = typeArgs;
        monoMethod->classOfThisMethod = mono;
        monoMethod->isMethod = true;
        monoMethod->hasThisParam = true;
        monoMethod->returnType = substituteType(origMethod->returnType, subst);
        monoMethod->genericASTNode = origMethod->genericASTNode;
        for (auto& origParam : origMethod->parameters) {
            auto newParam = std::make_shared<VariableSymbol>(
                origParam->getName(), substituteType(origParam->getType(), subst));
            newParam->isReference = origParam->isReference;
            monoMethod->parameters.push_back(newParam);
        }
        mono->define(monoMethod);

        // Add to function monomorphization cache for body emission
        std::string methodKey = origMethod->getQualifiedName();
        for (auto& ta : typeArgs) methodKey += "$" + ta->getInterningKey();
        monomorphizationCache_[methodKey] = monoMethod;
    }

    // Build vtable (slot 0 = destructor, then methods)
    mono->vtable.clear();
    if (mono->destructor) mono->vtable.push_back(mono->destructor);
    for (auto& sym : mono->getAllSymbols()) {
        auto* m = sym->as<FunctionSymbol>();
        if (m && m->isMethod && !m->is<ConstructorSymbol>() && !m->is<DestructorSymbol>()) {
            m->vtableIndex = static_cast<int>(mono->vtable.size());
            mono->vtable.push_back(std::dynamic_pointer_cast<FunctionSymbol>(sym));
        }
    }
    mono->vtableSize = static_cast<int>(mono->vtable.size());

    registerType(monoName, mono);
    classMonoCache_[key] = mono;
    return mono;
}

std::vector<std::shared_ptr<ClassSymbol>> SymbolTable::getAllClassMonomorphizations() const {
    std::vector<std::shared_ptr<ClassSymbol>> result;
    result.reserve(classMonoCache_.size());
    for (auto& kv : classMonoCache_) result.push_back(kv.second);
    return result;
}

//================================================================================
// Interface generic monomorphization
//================================================================================

std::shared_ptr<InterfaceSymbol> SymbolTable::getOrCreateInterfaceMonomorphization(
    InterfaceSymbol* tmpl, const std::vector<TypeSymbolPtr>& typeArgs)
{
    // Build cache key
    std::string key = "iface:" + tmpl->getName();
    for (auto& ta : typeArgs) key += "$" + ta->getInterningKey();

    auto it = interfaceMonoCache_.find(key);
    if (it != interfaceMonoCache_.end()) return it->second;

    // Build substitution map
    std::unordered_map<std::string, TypeSymbolPtr> subst;
    for (size_t i = 0; i < tmpl->typeParameterNames.size(); i++) {
        subst[tmpl->typeParameterNames[i]] = typeArgs[i];
    }

    // Mangled name: Getter$G_int
    std::string monoName = tmpl->getName() + "$G";
    for (auto& ta : typeArgs) monoName += "_" + ta->getName();

    auto mono = std::make_shared<InterfaceSymbol>(monoName);
    mono->genericTemplate = tmpl;
    mono->typeArguments = typeArgs;
    mono->genericASTNode = tmpl->genericASTNode;

    // Substitute method signatures
    for (size_t mi = 0; mi < tmpl->methods.size(); mi++) {
        auto& origMethod = tmpl->methods[mi];
        auto monoMethod = std::make_shared<FunctionSymbol>(origMethod->getName());
        monoMethod->isMethod = true;
        monoMethod->isAbstract = true;
        monoMethod->hasThisParam = origMethod->hasThisParam;  // Preserve this-param flag
        monoMethod->returnType = substituteType(origMethod->returnType, subst);
        monoMethod->vtableIndex = origMethod->vtableIndex;  // Preserve slot index
        for (auto& origParam : origMethod->parameters) {
            auto newParam = std::make_shared<VariableSymbol>(
                origParam->getName(), substituteType(origParam->getType(), subst));
            newParam->isReference = origParam->isReference;
            monoMethod->parameters.push_back(newParam);
        }
        mono->methods.push_back(monoMethod);
        mono->define(monoMethod);  // Add to scope for resolve()
    }

    registerType(monoName, mono);
    interfaceMonoCache_[key] = mono;
    return mono;
}

std::vector<std::shared_ptr<InterfaceSymbol>> SymbolTable::getAllInterfaceMonomorphizations() const {
    std::vector<std::shared_ptr<InterfaceSymbol>> result;
    result.reserve(interfaceMonoCache_.size());
    for (auto& kv : interfaceMonoCache_) result.push_back(kv.second);
    return result;
}

} // namespace mingus
