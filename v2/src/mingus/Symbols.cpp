// ============================================================================
// Symbols.cpp — Concrete symbol type implementations
// ============================================================================

#include "mingus/Symbols.h"

namespace mingus {

// ============================================================================
// VariableSymbol
// ============================================================================

VariableSymbol::VariableSymbol(const std::string& name, TypeSymbolPtr type)
    : TypedSymbol(name)
    , type_(std::move(type)) {}

TypeSymbolPtr VariableSymbol::getType() const {
    return type_;
}

void VariableSymbol::setType(const TypeSymbolPtr& type) {
    type_ = type;
}

// ============================================================================
// FunctionSymbol
// ============================================================================

FunctionSymbol::FunctionSymbol(const std::string& name)
    : SymbolWithScope(name) {}

std::shared_ptr<FunctionTypeSymbol> FunctionSymbol::buildFunctionType() const {
    std::vector<FunctionTypeSymbol::ParameterInfo> paramInfos;
    paramInfos.reserve(parameters.size());
    for (const auto& param : parameters) {
        FunctionTypeSymbol::ParameterInfo info;
        info.type = param->getType();
        info.name = param->getName();
        info.isReference = param->isReference;
        paramInfos.push_back(std::move(info));
    }
    return std::make_shared<FunctionTypeSymbol>(
        std::move(paramInfos), returnType, /*isVariadic=*/false);
}

std::string FunctionSymbol::getQualifiedName() const {
    // Walk up to find module name for mangling: Module_FuncName
    auto enclosing = getSymbolScope();
    while (enclosing) {
        auto* modSym = dynamic_cast<ModuleSymbol*>(enclosing.get());
        if (modSym) {
            return modSym->getName() + "_" + getName();
        }
        // Check if enclosing scope is a SymbolWithScope
        auto* symScope = dynamic_cast<SymbolWithScope*>(enclosing.get());
        if (symScope) {
            enclosing = symScope->getSymbolScope();
        } else {
            enclosing = enclosing->getEnclosingScope();
        }
    }
    return getName();
}

// ============================================================================
// MethodSymbol
// ============================================================================

MethodSymbol::MethodSymbol(const std::string& name)
    : FunctionSymbol(name)
{
    isMethod = true;
    hasThisParam = true;
}

// ============================================================================
// ConstructorSymbol
// ============================================================================

ConstructorSymbol::ConstructorSymbol(const std::string& className)
    : FunctionSymbol("constructor")
{
    isMethod = true;
    hasThisParam = true;
    // Note: the className is used for qualified name mangling (ClassName_constructor)
    // We'll store it via the enclosing ClassSymbol scope
}

// ============================================================================
// DestructorSymbol
// ============================================================================

DestructorSymbol::DestructorSymbol(const std::string& className)
    : FunctionSymbol("destructor")
{
    isMethod = true;
    hasThisParam = true;
}

// ============================================================================
// OperatorSymbol
// ============================================================================

OperatorSymbol::OperatorSymbol(const std::string& name, OverloadableOp op)
    : FunctionSymbol(name)
    , op(op) {}

// ============================================================================
// ClassSymbol
// ============================================================================

ClassSymbol::ClassSymbol(const std::string& className, ScopePtr enclosingScope)
    : TypeSymbol(className, /*isPrimaryType=*/false)
{
    setEnclosingScope(enclosingScope);
}

SymbolPtr ClassSymbol::resolve(const std::string& name) const {
    // 1. Own members (fields, methods, etc.)
    auto it = symbols_.find(name);
    if (it != symbols_.end()) {
        return it->second;
    }
    // 2. Inherited members (walk base class chain)
    if (resolvedBaseClass) {
        auto found = resolvedBaseClass->resolve(name);
        if (found) return found;
    }
    // 3. Enclosing scope (module-level, for non-member lookups)
    if (enclosingScope_) {
        return enclosingScope_->resolve(name);
    }
    return nullptr;
}

// ============================================================================
// StructSymbol
// ============================================================================

StructSymbol::StructSymbol(const std::string& name)
    : TypeSymbol(name, /*isPrimaryType=*/false) {}

bool StructSymbol::needsCleanup() const {
    for (const auto& field : fields) {
        if (field && field->getType() && field->getType()->is<FunctionTypeSymbol>()) {
            return true;  // closure-typed field needs RC release
        }
    }
    return false;
}

// ============================================================================
// EnumSymbol
// ============================================================================

EnumSymbol::EnumSymbol(const std::string& name)
    : TypeSymbol(name, /*isPrimaryType=*/false) {}

const EnumSymbol::MemberInfo* EnumSymbol::findMember(const std::string& name) const {
    for (const auto& member : members) {
        if (member.name == name) {
            return &member;
        }
    }
    return nullptr;
}

// ============================================================================
// InterfaceSymbol
// ============================================================================

InterfaceSymbol::InterfaceSymbol(const std::string& name)
    : TypeSymbol(name, /*isPrimaryType=*/false) {}

std::shared_ptr<FunctionSymbol> InterfaceSymbol::findMethod(
    const std::string& name) const
{
    for (const auto& method : methods) {
        if (method && method->getName() == name) {
            return method;
        }
    }
    return nullptr;
}

// ============================================================================
// ModuleSymbol
// ============================================================================

ModuleSymbol::ModuleSymbol(const std::string& name)
    : SymbolWithScope(name) {}

} // namespace mingus
