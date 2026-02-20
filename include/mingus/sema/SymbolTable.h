//================================================================================
// MINGUS v1 - Symbol Table
// Owns all symbols and the root scope. Central container for semantic analysis.
//================================================================================

#pragma once

#include "mingus/sema/Symbol.h"
#include "mingus/sema/Scope.h"

#include <memory>
#include <vector>

namespace mingus {
namespace sema {

//================================================================================
// SymbolTable — owns all symbols and the global scope tree
//================================================================================
class SymbolTable {
public:
    SymbolTable() = default;

    // Create a symbol of type T and take ownership.
    // Returns a raw (non-owning) pointer for use in scopes.
    template<typename T, typename... Args>
    T* createSymbol(Args&&... args) {
        auto sym = std::make_unique<T>(std::forward<Args>(args)...);
        auto* ptr = sym.get();
        symbols_.push_back(std::move(sym));
        return ptr;
    }

    // Create the global scope (only call once)
    Scope* createGlobalScope() {
        globalScope_ = std::make_unique<Scope>(ScopeKind::Global);
        return globalScope_.get();
    }

    // Get the global scope
    Scope* getGlobalScope() const { return globalScope_.get(); }

    // Get all symbols (for debugging/inspection)
    const std::vector<std::unique_ptr<Symbol>>& getSymbols() const { return symbols_; }

private:
    std::unique_ptr<Scope> globalScope_;
    std::vector<std::unique_ptr<Symbol>> symbols_;
};

} // namespace sema
} // namespace mingus
