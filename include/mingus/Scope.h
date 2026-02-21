#pragma once

// ============================================================================
// Scope.h — Scope hierarchy for Mingus V2
//
// Scope (abstract interface)
// +-- BaseScope (concrete: symbol map, enclosing chain, nesting)
//     +-- GlobalScope (root scope, one per compilation)
//     +-- BlockScope (anonymous { } blocks, for bodies, match arms)
//
// SymbolWithScope also extends BaseScope — see Symbol.h.
// That's the key MI pattern: types, functions, modules ARE scopes.
// ============================================================================

#include "Forward.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mingus {

// ============================================================================
// Scope — Abstract interface
// ============================================================================

class Scope {
public:
    virtual ~Scope() = default;

    // Name resolution: walk up enclosing chain
    virtual SymbolPtr resolve(const std::string& name) const = 0;

    // Define a symbol in this scope
    virtual void define(const SymbolPtr& sym) = 0;

    // All symbols directly in this scope
    virtual std::vector<SymbolPtr> getAllSymbols() const = 0;

    // Scope chain navigation
    virtual ScopePtr getEnclosingScope() const = 0;
    virtual void setEnclosingScope(const ScopePtr& scope) = 0;

    // Walk to root, collecting all enclosing scopes
    virtual std::vector<ScopePtr> getEnclosingPathToRoot() const = 0;

    // Human-readable scope name (for diagnostics)
    virtual std::string getName() const = 0;

    // Nest a child scope (block scopes that are NOT symbols)
    virtual void nest(const ScopePtr& childScope) = 0;

    // Operator overload management (separate namespace from regular symbols)
    virtual void defineOperator(const std::shared_ptr<OperatorSymbol>& op) = 0;
    virtual std::shared_ptr<OperatorSymbol> resolveOperator(OverloadableOp op) const = 0;
    virtual const std::vector<std::shared_ptr<OperatorSymbol>>& getAllOperators() const = 0;
};

// ============================================================================
// BaseScope — Concrete implementation of Scope
//
// Owns a symbol map, an enclosing scope pointer, nested scopes, and operators.
// SymbolWithScope inherits from this.
// ============================================================================

class BaseScope : public Scope {
public:
    BaseScope() = default;
    explicit BaseScope(ScopePtr enclosingScope);

    // ---- Scope interface ----
    SymbolPtr resolve(const std::string& name) const override;
    void define(const SymbolPtr& sym) override;
    std::vector<SymbolPtr> getAllSymbols() const override;
    ScopePtr getEnclosingScope() const override;
    void setEnclosingScope(const ScopePtr& scope) override;
    std::vector<ScopePtr> getEnclosingPathToRoot() const override;
    std::string getName() const override;
    void nest(const ScopePtr& childScope) override;

    // ---- Operator overloads ----
    void defineOperator(const std::shared_ptr<OperatorSymbol>& op) override;
    std::shared_ptr<OperatorSymbol> resolveOperator(OverloadableOp op) const override;
    const std::vector<std::shared_ptr<OperatorSymbol>>& getAllOperators() const override;

    // ---- Diagnostics ----
    virtual std::string toString() const;

    // Check if a name is defined directly in this scope (no chain walk)
    bool isDefined(const std::string& name) const;

    // Overload resolution: return all functions with a given name
    // Walks enclosing scope chain. Returns empty vector if none found.
    std::vector<std::shared_ptr<FunctionSymbol>> resolveFunctions(
        const std::string& name) const;

protected:
    std::unordered_map<std::string, SymbolPtr> symbols_;
    ScopePtr enclosingScope_;
    std::vector<ScopePtr> nestedScopes_;  // child block scopes (not SymbolWithScope)
    std::vector<std::shared_ptr<OperatorSymbol>> operators_;
    std::string name_;

    // Parallel storage for function overloads (functions only, not ctors/dtors)
    std::unordered_map<std::string,
        std::vector<std::shared_ptr<FunctionSymbol>>> functionOverloads_;
};

// ============================================================================
// GlobalScope — Root scope, one per compilation unit
// ============================================================================

class GlobalScope : public BaseScope {
public:
    GlobalScope();
    std::string getName() const override;
};

// ============================================================================
// BlockScope — Anonymous { } blocks, for/while bodies, match arms
// ============================================================================

class BlockScope : public BaseScope {
public:
    explicit BlockScope(ScopePtr enclosingScope);
    std::string getName() const override;

    // Unique label for this block (for diagnostics/debug)
    std::string label;
};

} // namespace mingus
