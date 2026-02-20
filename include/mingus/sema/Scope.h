//================================================================================
// MINGUS v1 - Scope System
// Symbols live inside scopes. Scopes form a tree that mirrors lexical nesting.
// Name lookup walks up the tree from the current scope until a match is found.
//================================================================================

#pragma once

#include "mingus/sema/Symbol.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace mingus {
namespace sema {

//================================================================================
// ScopeKind — what kind of scope this is
//================================================================================
enum class ScopeKind {
    Global,       // Program root, contains modules
    Module,       // module X { ... }
    TypeMembers,  // struct/class member scope (fields, methods, operators)
    Function,     // function body
    Block,        // { ... } inside a function (if/for/while bodies)
    RawBlock      // raw { ... } — same as Block but codegen skips safety checks
};

//================================================================================
// Scope — lexical scope containing symbols
//================================================================================
class Scope {
public:
    ScopeKind kind;
    Scope* parent;                              // Enclosing scope (null for global)
    std::vector<std::unique_ptr<Scope>> children;
    Symbol* ownerSymbol;                        // Symbol that created this scope (module, function, type)

    // Insert symbol into this scope's map.
    // Returns false if name already exists in THIS scope (redefinition error).
    // Does NOT check parent scopes — shadowing is legal.
    bool define(Symbol* symbol);

    // Insert symbol under a different name (for import aliases).
    // Returns false if aliasName already exists in THIS scope.
    bool defineAs(const std::string& aliasName, Symbol* symbol);

    // Check this scope. If found, return it.
    // If not found, recurse into parent.
    // Returns nullptr if not found in any scope.
    Symbol* lookup(const std::string& name) const;

    // Check only this scope. Used to detect redefinitions.
    Symbol* lookupLocal(const std::string& name) const;

    // Search this scope's operator list for a matching OverloadableOp.
    // Does NOT walk up — operators belong to their declaring type only.
    OperatorSymbol* lookupOperator(OverloadableOp op) const;

    // Create a child scope
    Scope* createChild(ScopeKind childKind, Symbol* owner = nullptr);

    // Read-only access to symbols and operators (for dump tools)
    const std::map<std::string, Symbol*>& getSymbolMap() const { return symbols_; }
    const std::vector<OperatorSymbol*>& getOperatorList() const { return operators_; }

    explicit Scope(ScopeKind k, Scope* par = nullptr, Symbol* owner = nullptr)
        : kind(k), parent(par), ownerSymbol(owner) {}

private:
    std::map<std::string, Symbol*> symbols_;
    std::vector<OperatorSymbol*> operators_;
};

} // namespace sema
} // namespace mingus
