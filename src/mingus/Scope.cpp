// ============================================================================
// Scope.cpp — BaseScope, GlobalScope, BlockScope implementations
//
// Note: resolveOperator() is implemented in Symbols.cpp where OperatorSymbol
// is fully defined. This file handles the core scope mechanics.
// ============================================================================

#include "mingus/Scope.h"
#include "mingus/Symbols.h"  // needed for OperatorSymbol::op in resolveOperator

namespace mingus {

// ============================================================================
// BaseScope
// ============================================================================

BaseScope::BaseScope(ScopePtr enclosingScope)
    : enclosingScope_(std::move(enclosingScope)) {}

SymbolPtr BaseScope::resolve(const std::string& name) const {
    // 1. Look in our own symbols
    auto it = symbols_.find(name);
    if (it != symbols_.end()) {
        return it->second;
    }
    // 2. Check function overloads (return first match for backward compat)
    auto fit = functionOverloads_.find(name);
    if (fit != functionOverloads_.end() && !fit->second.empty()) {
        return fit->second[0];
    }
    // 3. Walk up the enclosing scope chain
    if (enclosingScope_) {
        return enclosingScope_->resolve(name);
    }
    return nullptr;
}

void BaseScope::define(const SymbolPtr& sym) {
    if (!sym) return;

    // Function overloading: track all FunctionSymbol definitions per name
    // (excludes constructors/destructors which use dedicated ClassSymbol slots)
    if (auto funcSym = std::dynamic_pointer_cast<FunctionSymbol>(sym)) {
        if (!funcSym->is<ConstructorSymbol>() && !funcSym->is<DestructorSymbol>()) {
            auto& overloads = functionOverloads_[sym->getName()];
            overloads.push_back(funcSym);
            // Mark all overloads when multiple functions share a name
            if (overloads.size() > 1) {
                for (auto& f : overloads) f->hasOverloads = true;
            }
            // Still store in symbols_ for backward compat (resolve returns first)
            if (overloads.size() == 1) {
                symbols_[sym->getName()] = sym;
            }
            return;
        }
    }

    // Non-function symbols or constructors/destructors: single entry
    symbols_[sym->getName()] = sym;
}

std::vector<SymbolPtr> BaseScope::getAllSymbols() const {
    std::vector<SymbolPtr> result;
    // Non-function symbols (variables, types, ctors, dtors) from symbols_ map
    for (const auto& [name, sym] : symbols_) {
        // Skip functions that are tracked in functionOverloads_
        if (functionOverloads_.count(name)) continue;
        result.push_back(sym);
    }
    // All function overloads (includes single-definition functions)
    for (const auto& [name, funcs] : functionOverloads_) {
        for (const auto& func : funcs) {
            result.push_back(func);
        }
    }
    return result;
}

ScopePtr BaseScope::getEnclosingScope() const {
    return enclosingScope_;
}

void BaseScope::setEnclosingScope(const ScopePtr& scope) {
    enclosingScope_ = scope;
}

std::vector<ScopePtr> BaseScope::getEnclosingPathToRoot() const {
    std::vector<ScopePtr> path;
    auto current = enclosingScope_;
    while (current) {
        path.push_back(current);
        current = current->getEnclosingScope();
    }
    return path;
}

std::string BaseScope::getName() const {
    return name_;
}

void BaseScope::nest(const ScopePtr& childScope) {
    if (!childScope) return;
    nestedScopes_.push_back(childScope);
}

void BaseScope::defineOperator(const std::shared_ptr<OperatorSymbol>& op) {
    if (!op) return;
    operators_.push_back(op);
}

std::shared_ptr<OperatorSymbol> BaseScope::resolveOperator(OverloadableOp opKind) const {
    for (const auto& opSym : operators_) {
        if (opSym && opSym->op == opKind) {
            return opSym;
        }
    }
    // Walk up the chain for operator resolution
    if (enclosingScope_) {
        return enclosingScope_->resolveOperator(opKind);
    }
    return nullptr;
}

const std::vector<std::shared_ptr<OperatorSymbol>>& BaseScope::getAllOperators() const {
    return operators_;
}

bool BaseScope::isDefined(const std::string& name) const {
    return symbols_.find(name) != symbols_.end() ||
           functionOverloads_.find(name) != functionOverloads_.end();
}

std::vector<std::shared_ptr<FunctionSymbol>> BaseScope::resolveFunctions(
    const std::string& name) const
{
    auto it = functionOverloads_.find(name);
    if (it != functionOverloads_.end() && !it->second.empty()) {
        return it->second;
    }
    // Walk up scope chain
    if (enclosingScope_) {
        auto* baseScope = dynamic_cast<BaseScope*>(enclosingScope_.get());
        if (baseScope) {
            return baseScope->resolveFunctions(name);
        }
    }
    return {};
}

std::string BaseScope::toString() const {
    std::string result = "Scope[" + getName() + "] {\n";
    for (const auto& [name, sym] : symbols_) {
        if (!functionOverloads_.count(name))
            result += "  " + name + "\n";
    }
    for (const auto& [name, funcs] : functionOverloads_) {
        result += "  " + name + " (" + std::to_string(funcs.size()) + " overload(s))\n";
    }
    result += "}";
    return result;
}

// ============================================================================
// GlobalScope
// ============================================================================

GlobalScope::GlobalScope() {
    name_ = "global";
}

std::string GlobalScope::getName() const {
    return "global";
}

// ============================================================================
// BlockScope
// ============================================================================

BlockScope::BlockScope(ScopePtr enclosingScope)
    : BaseScope(std::move(enclosingScope)) {
    name_ = "block";
}

std::string BlockScope::getName() const {
    return label.empty() ? "block" : label;
}

} // namespace mingus
