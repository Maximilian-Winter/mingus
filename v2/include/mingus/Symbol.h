#pragma once

// ============================================================================
// Symbol.h — Core symbol abstractions for Mingus V2
//
// Symbol (abstract interface)
// +-- BaseSymbol (leaf symbols — no scope of their own)
// |   +-- TypedSymbol (abstract — getType()/setType() returning TypeSymbol)
// |       +-- VariableSymbol (see Symbols.h)
// +-- SymbolWithScope (MI: BaseScope + Symbol — the core dual-nature pattern)
//     +-- TypeSymbol (see TypeSymbol.h)
//     +-- FunctionSymbol (see Symbols.h)
//     +-- ModuleSymbol (see Symbols.h)
//
// The key insight: SymbolWithScope IS both a Symbol and a Scope.
// A ClassSymbol IS its own member scope. A FunctionSymbol IS its body scope.
// ============================================================================

#include "Forward.h"
#include "Scope.h"

#include <memory>
#include <string>
#include <vector>

namespace mingus {

// Forward — needed for SymbolUsageNodes
class AstBaseNode;

// ============================================================================
// Symbol — Abstract interface
//
// A named entity in the program: variable, function, type, module, etc.
// ============================================================================

class Symbol {
public:
    virtual ~Symbol() = default;

    // The symbol's name
    virtual std::string getName() const = 0;

    // The scope this symbol was defined IN (not the scope it creates)
    virtual ScopePtr getSymbolScope() const = 0;
    virtual void setSymbolScope(const ScopePtr& scope) = 0;

    // Insertion order within a scope (for deterministic iteration)
    virtual int getInsertionOrderNumber() const = 0;
    virtual void setInsertionOrderNumber(int value) = 0;

    // Every AST node that references this symbol (for IDE: find-all-refs, rename)
    std::vector<std::shared_ptr<AstBaseNode>> symbolUsageNodes;

    // Access control
    AccessModifier accessLevel = AccessModifier::Public;

    // RTTI-lite: dynamic type checking without dynamic_cast overhead
    template<typename T> T* as() { return dynamic_cast<T*>(this); }
    template<typename T> const T* as() const { return dynamic_cast<const T*>(this); }
    template<typename T> bool is() const { return dynamic_cast<const T*>(this) != nullptr; }
};

// ============================================================================
// BaseSymbol — Leaf symbols (no scope of their own)
//
// Variables, parameters, fields — things that have a name and live in a scope
// but don't create their own scope.
// ============================================================================

class BaseSymbol : public Symbol {
public:
    explicit BaseSymbol(std::string name);

    std::string getName() const override;
    ScopePtr getSymbolScope() const override;
    void setSymbolScope(const ScopePtr& scope) override;
    int getInsertionOrderNumber() const override;
    void setInsertionOrderNumber(int value) override;

    // The defining AST node for this symbol
    std::shared_ptr<AstBaseNode> defBaseNode;

protected:
    const std::string name_;
    ScopePtr scope_;         // the scope this symbol was defined IN
    int insertionOrder_ = 0;
};

// ============================================================================
// TypedSymbol — Abstract base for symbols that carry a type
//
// Extends BaseSymbol with getType()/setType() returning TypeSymbol.
// VariableSymbol extends this.
// ============================================================================

class TypedSymbol : public BaseSymbol {
public:
    explicit TypedSymbol(const std::string& name);

    virtual TypeSymbolPtr getType() const = 0;
    virtual void setType(const TypeSymbolPtr& type) = 0;
};

// ============================================================================
// SymbolWithScope — The core dual-nature abstraction
//
// Multiple inheritance: IS both a BaseScope (has symbols, enclosing chain)
// AND a Symbol (has a name, lives in an enclosing scope).
//
// ClassSymbol IS a SymbolWithScope: classSym->resolve("field") works.
// FunctionSymbol IS a SymbolWithScope: funcSym->define(paramSym) works.
// ModuleSymbol IS a SymbolWithScope: moduleSym->resolve("func") works.
// ============================================================================

class SymbolWithScope : public BaseScope, public Symbol {
public:
    explicit SymbolWithScope(std::string name);

    // ---- Symbol interface ----
    std::string getName() const override;
    ScopePtr getSymbolScope() const override;
    void setSymbolScope(const ScopePtr& scope) override;
    int getInsertionOrderNumber() const override;
    void setInsertionOrderNumber(int value) override;

    // ---- Scope overrides (getName from both interfaces) ----
    // BaseScope::getName() returns the symbol's name
    // BaseScope::getEnclosingScope() returns the enclosing scope

    // ---- Qualified name support (for diagnostics and LLVM name mangling) ----
    virtual std::string getQualifiedName() const;
    virtual std::string getFullyQualifiedName(const std::string& separator = "::") const;

    // Is this symbol defined externally (extern keyword)?
    bool isExternal = false;

    // The defining AST node for this symbol
    std::shared_ptr<AstBaseNode> defBaseNode;

protected:
    const std::string name_;
    ScopePtr symbolScope_;   // the scope this symbol was defined IN
    int insertionOrder_ = 0;
};

} // namespace mingus
