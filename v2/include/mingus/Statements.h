#pragma once

// ============================================================================
// Statements.h — All concrete statement AST nodes for Mingus V2
//
// BlockStatementNode is defined in AstNode.h (used broadly).
// All other statements are here.
// ============================================================================

#include "AstNode.h"

#include <memory>
#include <vector>

namespace mingus {

// ============================================================================
// ExpressionStatement — Wraps an expression used as a statement
//
// This includes: function calls, assignments, variable declarations (via
// VariableDeclarationExpression), match expressions used as statements.
// ============================================================================

class ExpressionStatement : public StatementBaseNode {
public:
    void accept(ASTVisitor& visitor) override;
    std::shared_ptr<ExpressionBaseNode> expression;
};

// ============================================================================
// ReturnStatement — return expr; or return;
// ============================================================================

class ReturnStatement : public StatementBaseNode {
public:
    void accept(ASTVisitor& visitor) override;
    std::shared_ptr<ExpressionBaseNode> value;  // nullptr for void return
};

// ============================================================================
// IfStatement — if/else-if/else chains
// ============================================================================

class IfStatement : public StatementBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::shared_ptr<ExpressionBaseNode> condition;
    std::shared_ptr<StatementBaseNode> thenBody;
    std::vector<ElseIfClause> elseIfClauses;
    std::shared_ptr<StatementBaseNode> elseBody;  // nullptr if no else
};

// ============================================================================
// ForStatement — for (init; condition; iterator) body
//
// initDeclaration and initExpressions are mutually exclusive.
// Condition can be null (infinite loop).
// ============================================================================

class ForStatement : public StatementBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    // Initialization: one of these (or neither)
    std::shared_ptr<DeclarationBaseNode> initDeclaration;
    std::vector<std::shared_ptr<ExpressionBaseNode>> initExpressions;

    // Condition (nullptr = infinite loop)
    std::shared_ptr<ExpressionBaseNode> condition;

    // Iterators (executed after each iteration)
    std::vector<std::shared_ptr<ExpressionBaseNode>> iterators;

    // Loop body
    std::shared_ptr<StatementBaseNode> body;
};

// ============================================================================
// WhileStatement — while (condition) body
// ============================================================================

class WhileStatement : public StatementBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::shared_ptr<ExpressionBaseNode> condition;
    std::shared_ptr<StatementBaseNode> body;
};

// ============================================================================
// BreakStatement — break;
// ============================================================================

class BreakStatement : public StatementBaseNode {
public:
    void accept(ASTVisitor& visitor) override;
};

// ============================================================================
// ContinueStatement — continue;
// ============================================================================

class ContinueStatement : public StatementBaseNode {
public:
    void accept(ASTVisitor& visitor) override;
};

// ============================================================================
// DeleteStatement — delete expr;
//
// For heap-allocated objects (via new). Dispatches through vtable[0]
// (virtual destructor) then frees memory.
// ============================================================================

class DeleteStatement : public StatementBaseNode {
public:
    void accept(ASTVisitor& visitor) override;
    std::shared_ptr<ExpressionBaseNode> target;
};

// ============================================================================
// SwitchStatement — switch (subject) { case val: stmts; default: stmts; }
// ============================================================================

class SwitchStatement : public StatementBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::shared_ptr<ExpressionBaseNode> subject;
    std::vector<SwitchCase> cases;
    std::vector<std::shared_ptr<StatementBaseNode>> defaultCase;  // empty if none
};

} // namespace mingus
