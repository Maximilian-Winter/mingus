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
    // 2. Walk up the enclosing scope chain
    if (enclosingScope_) {
        return enclosingScope_->resolve(name);
    }
    return nullptr;
}

void BaseScope::define(const SymbolPtr& sym) {
    if (!sym) return;
    // Note: silently overwrites on redefinition (sema should catch duplicates)
    symbols_[sym->getName()] = sym;
}

std::vector<SymbolPtr> BaseScope::getAllSymbols() const {
    std::vector<SymbolPtr> result;
    result.reserve(symbols_.size());
    for (const auto& [name, sym] : symbols_) {
        result.push_back(sym);
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
    return symbols_.find(name) != symbols_.end();
}

std::string BaseScope::toString() const {
    std::string result = "Scope[" + getName() + "] {\n";
    for (const auto& [name, sym] : symbols_) {
        result += "  " + name + "\n";
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
