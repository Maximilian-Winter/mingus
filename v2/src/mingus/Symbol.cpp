// ============================================================================
// Symbol.cpp — BaseSymbol, TypedSymbol, SymbolWithScope implementations
// ============================================================================

#include "mingus/Symbol.h"

namespace mingus {

// ============================================================================
// BaseSymbol
// ============================================================================

BaseSymbol::BaseSymbol(std::string name)
    : name_(std::move(name)) {}

std::string BaseSymbol::getName() const {
    return name_;
}

ScopePtr BaseSymbol::getSymbolScope() const {
    return scope_;
}

void BaseSymbol::setSymbolScope(const ScopePtr& scope) {
    scope_ = scope;
}

int BaseSymbol::getInsertionOrderNumber() const {
    return insertionOrder_;
}

void BaseSymbol::setInsertionOrderNumber(int value) {
    insertionOrder_ = value;
}

// ============================================================================
// TypedSymbol
// ============================================================================

TypedSymbol::TypedSymbol(const std::string& name)
    : BaseSymbol(name) {}

// ============================================================================
// SymbolWithScope
// ============================================================================

SymbolWithScope::SymbolWithScope(std::string name)
    : name_(std::move(name)) {
    // BaseScope::name_ is inherited — set it to match the symbol name
    BaseScope::name_ = name_;
}

// ---- Symbol interface ----

std::string SymbolWithScope::getName() const {
    return name_;
}

ScopePtr SymbolWithScope::getSymbolScope() const {
    return symbolScope_;
}

void SymbolWithScope::setSymbolScope(const ScopePtr& scope) {
    symbolScope_ = scope;
}

int SymbolWithScope::getInsertionOrderNumber() const {
    return insertionOrder_;
}

void SymbolWithScope::setInsertionOrderNumber(int value) {
    insertionOrder_ = value;
}

// ---- Qualified name support ----

std::string SymbolWithScope::getQualifiedName() const {
    return name_;
}

std::string SymbolWithScope::getFullyQualifiedName(const std::string& separator) const {
    std::string result = name_;
    auto current = symbolScope_;
    while (current) {
        // Walk up through SymbolWithScope ancestors only
        auto* symScope = dynamic_cast<SymbolWithScope*>(current.get());
        if (symScope) {
            result = symScope->getName() + separator + result;
            current = symScope->getSymbolScope();
        } else {
            break;
        }
    }
    return result;
}

} // namespace mingus
