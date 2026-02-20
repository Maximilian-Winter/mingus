#pragma once

// ============================================================================
// AstNode.h — AST base classes for Mingus V2
//
// Every AST node carries:
//   - AstScopeNode: pointer to the scope this node lives in (set by Pass 1)
//   - DebugInfoNode: full source range for diagnostics and debug info
//
// This eliminates childIndexStack_ and positional scope navigation.
// Each pass reads node->AstScopeNode directly.
//
// Node hierarchy:
//   AstBaseNode
//   +-- ExpressionBaseNode (has resolvedType, resolvedSymbol)
//   +-- StatementBaseNode
//   |   +-- DeclarationBaseNode
//   +-- BlockStatementNode (owns child statements, scope = BlockScope)
//   +-- ArgumentsNode (per-argument IsReference tracking)
//   +-- ParameterNode (linked to VariableSymbol by Pass 1)
//   +-- TypeNode (type annotation in source)
//   +-- ModuleNode (top-level module)
//   +-- ProgramNode (root: owns all modules)
// ============================================================================

#include "Forward.h"
#include "DebugInfo.h"

#include <memory>
#include <string>
#include <vector>

namespace mingus {

// ============================================================================
// ASTVisitor — Forward declaration for visitor pattern
// ============================================================================

class ASTVisitor;

// ============================================================================
// AST Node Type Enums
// ============================================================================

enum class ExpressionNodeType {
    Literal,
    Identifier,
    QualifiedName,
    BinaryOperation,
    UnaryOperation,
    Call,
    MemberAccess,
    IndexAccess,
    Assignment,
    Lambda,
    Match,
    Pipe,
    Cast,
    New,
    StringInterpolation,
    TupleLiteral,
    ArrayLiteral,
    Null,
    VariableDeclaration,  // var x = expr (dual: expression in statement context)
    Ternary
};

enum class StatementType {
    Expression,
    Block,
    If,
    For,
    While,
    Return,
    Break,
    Continue,
    Delete,
    Declaration
};

enum class DeclarationNodeType {
    Function,
    Class,
    Struct,
    Enum,
    Interface,
    Module,
    Import,
    Extern,
    Operator
};

// ============================================================================
// AstBaseNode — Root of all AST nodes
//
// Every node has a scope reference and debug info. No exceptions.
// ============================================================================

class AstBaseNode {
public:
    virtual ~AstBaseNode() = default;
    virtual void accept(ASTVisitor& visitor) = 0;

    // The scope this node lives in (set by Pass 1 — SymbolTableBuilder)
    ScopePtr astScopeNode;

    // Source location with full range
    std::shared_ptr<DebugInfo> debugInfo;

    // RTTI-lite
    template<typename T> T* as() { return dynamic_cast<T*>(this); }
    template<typename T> const T* as() const { return dynamic_cast<const T*>(this); }
    template<typename T> bool is() const { return dynamic_cast<const T*>(this) != nullptr; }
};

// ============================================================================
// ExpressionBaseNode — Base for all expression nodes
//
// After type checking (Pass 3), resolvedType holds the expression's type.
// For identifier/member expressions, resolvedSymbol links to the symbol.
// ============================================================================

class ExpressionBaseNode : public AstBaseNode {
public:
    ExpressionNodeType expressionType;

    // Set by Pass 3 (TypeChecker)
    TypeSymbolPtr resolvedType;

    // Set by Pass 2/3 for identifiers and member access
    SymbolPtr resolvedSymbol;
};

// ============================================================================
// StatementBaseNode — Base for all statement nodes
// ============================================================================

class StatementBaseNode : public AstBaseNode {
public:
    StatementType statementType;
};

// ============================================================================
// DeclarationBaseNode — Base for all declaration nodes
// ============================================================================

class DeclarationBaseNode : public StatementBaseNode {
public:
    DeclarationNodeType declarationType;
};

// ============================================================================
// BlockStatementNode — A { } block containing statements
//
// The astScopeNode for this node points to the BlockScope created for it.
// ============================================================================

class BlockStatementNode : public StatementBaseNode {
public:
    BlockStatementNode() { statementType = StatementType::Block; }

    void accept(ASTVisitor& visitor) override;

    std::vector<std::shared_ptr<StatementBaseNode>> statements;
};

// ============================================================================
// TypeNode — Type annotation in source code (int, MyClass, T[10], etc.)
//
// Resolved to a TypeSymbol by Pass 2 (TypeResolver).
// ============================================================================

class TypeNode : public AstBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::string name;                         // "int", "MyClass", etc.
    bool isArray = false;
    int arraySize = -1;                       // for T[N] annotations
    bool isPointer = false;
    bool isReference = false;                 // for T& annotations
    std::vector<std::shared_ptr<TypeNode>> typeArguments;  // for future generics

    // Resolved by Pass 2
    TypeSymbolPtr resolvedType;
};

// ============================================================================
// ParameterNode — Function/lambda parameter declaration
//
// resolvedSymbol is set by Pass 1 (SymbolTableBuilder). This eliminates
// the need for scanForParamSymbols — the link is established at declaration
// time, not patched later.
// ============================================================================

class ParameterNode : public AstBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::string name;
    std::shared_ptr<TypeNode> type;
    bool isReference = false;                  // T& parameter

    // Set by Pass 1: direct link to the parameter's VariableSymbol
    std::shared_ptr<VariableSymbol> resolvedSymbol;
};

// ============================================================================
// ArgumentsNode — Call arguments with per-argument reference tracking
//
// IsReference[i] is set by Pass 3 (TypeChecker) when matching arguments
// against the callee's parameters. Codegen reads this directly — no need
// to re-resolve the callee type.
//
// This is the "ArgumentsBaseNode" pattern from the old compiler.
// ============================================================================

class ArgumentsNode : public AstBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    // The argument expressions
    std::vector<std::shared_ptr<ExpressionBaseNode>> expressions;

    // Per-argument: true if this argument should be passed by reference
    // Set by TypeChecker, read by codegen
    std::vector<bool> isReference;
};

// ============================================================================
// ModifiersNode — Access modifiers, static, abstract, extern, virtual
// ============================================================================

enum class ModifierType {
    Public,
    Private,
    Protected,
    Static,
    Abstract,
    Extern,
    Virtual,
    Override
};

class ModifiersNode : public AstBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::vector<ModifierType> modifiers;

    bool hasModifier(ModifierType mod) const {
        for (auto m : modifiers) {
            if (m == mod) return true;
        }
        return false;
    }
};

// ============================================================================
// CaptureItem — Explicit capture in lambda expressions
// ============================================================================

struct CaptureItem {
    std::string name;
    CaptureMode mode = CaptureMode::ByValue;
};

// ============================================================================
// ModuleNode — A module declaration (top-level container)
// ============================================================================

class ModuleNode : public DeclarationBaseNode {
public:
    ModuleNode() { declarationType = DeclarationNodeType::Module; }

    void accept(ASTVisitor& visitor) override;

    std::string name;
    std::vector<std::shared_ptr<DeclarationBaseNode>> declarations;

    // Resolved in Pass 1
    std::shared_ptr<ModuleSymbol> resolvedModule;
};

// ============================================================================
// ProgramNode — Root AST node (owns all modules)
// ============================================================================

class ProgramNode : public AstBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::vector<std::shared_ptr<ModuleNode>> modules;
    std::string entryPoint;  // --entry flag value
};

// ============================================================================
// ASTVisitor — Abstract visitor interface
//
// Concrete visitors: SymbolTableBuilder, TypeResolver, TypeChecker,
// SemanticValidator, IRGenerator
// ============================================================================

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    // Base nodes
    virtual void visit(BlockStatementNode& node) = 0;
    virtual void visit(TypeNode& node) {}
    virtual void visit(ParameterNode& node) {}
    virtual void visit(ArgumentsNode& node) {}
    virtual void visit(ModifiersNode& node) {}
    virtual void visit(ModuleNode& node) = 0;
    virtual void visit(ProgramNode& node) = 0;

    // Expressions and statements will be added as concrete node types
    // are defined (in separate headers like Expressions.h, Statements.h).
    // The visitor can be extended with additional visit() overloads.
};

} // namespace mingus
