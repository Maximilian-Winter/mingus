//================================================================================
// MINGUS v1 - Scope Implementation
//================================================================================

#include "mingus/sema/Scope.h"

namespace mingus {
namespace sema {

bool Scope::define(Symbol* symbol) {
    if (!symbol) return false;

    // Operators are stored separately — not looked up by name
    if (auto* opSym = dynamic_cast<OperatorSymbol*>(symbol)) {
        operators_.push_back(opSym);
        return true;
    }

    // Check for redefinition in this scope
    if (symbols_.find(symbol->name) != symbols_.end()) {
        return false;  // Redefinition error
    }

    symbols_[symbol->name] = symbol;
    return true;
}

bool Scope::defineAs(const std::string& aliasName, Symbol* symbol) {
    if (!symbol) return false;

    // Operators are stored separately — aliases not supported for operators
    if (dynamic_cast<OperatorSymbol*>(symbol)) {
        return false;
    }

    // Check for redefinition in this scope
    if (symbols_.find(aliasName) != symbols_.end()) {
        return false;  // Redefinition error
    }

    symbols_[aliasName] = symbol;
    return true;
}

Symbol* Scope::lookup(const std::string& name) const {
    // Check this scope first
    auto it = symbols_.find(name);
    if (it != symbols_.end()) {
        return it->second;
    }

    // Walk up the scope chain
    if (parent) {
        return parent->lookup(name);
    }

    return nullptr;  // Not found
}

Symbol* Scope::lookupLocal(const std::string& name) const {
    auto it = symbols_.find(name);
    if (it != symbols_.end()) {
        return it->second;
    }
    return nullptr;
}

OperatorSymbol* Scope::lookupOperator(OverloadableOp op) const {
    for (auto* opSym : operators_) {
        if (opSym->op == op) {
            return opSym;
        }
    }
    return nullptr;
}

Scope* Scope::createChild(ScopeKind childKind, Symbol* owner) {
    auto child = std::make_unique<Scope>(childKind, this, owner);
    auto* ptr = child.get();
    children.push_back(std::move(child));
    return ptr;
}

} // namespace sema
} // namespace mingus
