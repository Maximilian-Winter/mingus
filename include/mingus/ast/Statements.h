//================================================================================
// MINGUS v1 - Statement Nodes
// Block, Expression, Return, If, Switch, For, While, Break, Continue, Delete, Raw
//================================================================================

#pragma once

#include "mingus/ast/ASTNode.h"

#include <memory>
#include <string>
#include <vector>

namespace mingus {
namespace ast {

// Forward declarations
class ExpressionNode;
class DeclarationNode;
class StatementNode;
class BlockStatement;
class TypeNode;

//================================================================================
// BlockStatement - { statement; statement; ... }
//================================================================================
class BlockStatement : public StatementNode {
public:
    NodeList<StatementNode> statements;

    explicit BlockStatement(NodeList<StatementNode> stmts = {},
                            const SourceLocation& loc = SourceLocation())
        : StatementNode(loc), statements(std::move(stmts)) {}

    bool isEmpty() const { return statements.empty(); }

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// ExpressionStatement - expression;
//================================================================================
class ExpressionStatement : public StatementNode {
public:
    NodePtr<ExpressionNode> expression;

    explicit ExpressionStatement(NodePtr<ExpressionNode> expr,
                                 const SourceLocation& loc = SourceLocation())
        : StatementNode(loc), expression(expr) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// ReturnStatement - return; or return value;
//================================================================================
class ReturnStatement : public StatementNode {
public:
    NodePtr<ExpressionNode> value;  // nullptr for void return

    explicit ReturnStatement(NodePtr<ExpressionNode> val = nullptr,
                             const SourceLocation& loc = SourceLocation())
        : StatementNode(loc), value(val) {}

    bool hasValue() const { return value != nullptr; }

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// ElseIfClause - part of IfStatement
//================================================================================
struct ElseIfClause {
    NodePtr<ExpressionNode> condition;
    NodePtr<StatementNode> body;

    ElseIfClause(NodePtr<ExpressionNode> cond, NodePtr<StatementNode> bdy)
        : condition(cond), body(bdy) {}
};

//================================================================================
// IfStatement - if (cond) then [else if ...] [else ...]
//================================================================================
class IfStatement : public StatementNode {
public:
    NodePtr<ExpressionNode> condition;
    NodePtr<StatementNode> thenBody;
    std::vector<ElseIfClause> elseIfClauses;
    NodePtr<StatementNode> elseBody;  // nullptr if no else

    IfStatement(NodePtr<ExpressionNode> cond,
                NodePtr<StatementNode> thenBdy,
                std::vector<ElseIfClause> elseifs = {},
                NodePtr<StatementNode> elseBdy = nullptr,
                const SourceLocation& loc = SourceLocation())
        : StatementNode(loc), condition(cond), thenBody(thenBdy),
          elseIfClauses(std::move(elseifs)), elseBody(elseBdy) {}

    bool hasElseIf() const { return !elseIfClauses.empty(); }
    bool hasElse() const { return elseBody != nullptr; }

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// SwitchCase - single case in a switch statement
//================================================================================
struct SwitchCase {
    NodePtr<ExpressionNode> value;
    NodeList<StatementNode> body;

    SwitchCase(NodePtr<ExpressionNode> val, NodeList<StatementNode> bdy)
        : value(val), body(std::move(bdy)) {}
};

//================================================================================
// SwitchStatement - switch (subject) { case value: ... [default: ...] }
//================================================================================
class SwitchStatement : public StatementNode {
public:
    NodePtr<ExpressionNode> subject;
    std::vector<SwitchCase> cases;
    NodeList<StatementNode> defaultCase;  // empty if no default

    SwitchStatement(NodePtr<ExpressionNode> subj,
                    std::vector<SwitchCase> cs,
                    NodeList<StatementNode> defCase = {},
                    const SourceLocation& loc = SourceLocation())
        : StatementNode(loc), subject(subj), cases(std::move(cs)), 
          defaultCase(std::move(defCase)) {}

    bool hasDefault() const { return !defaultCase.empty(); }

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// ForStatement - for (init; cond; iter) body
//================================================================================
class ForStatement : public StatementNode {
public:
    // initializer can be: variable declaration, expression list, or empty
    NodePtr<DeclarationNode> initDeclaration;  // for var i = 0
    NodeList<ExpressionNode> initExpressions;  // for i = 0, j = 1
    NodePtr<ExpressionNode> condition;  // nullptr means true
    NodeList<ExpressionNode> iterators;
    NodePtr<StatementNode> body;

    // Constructor with declaration initializer
    ForStatement(NodePtr<DeclarationNode> initDecl,
                 NodePtr<ExpressionNode> cond,
                 NodeList<ExpressionNode> iters,
                 NodePtr<StatementNode> bdy,
                 const SourceLocation& loc = SourceLocation())
        : StatementNode(loc), initDeclaration(initDecl), condition(cond),
          iterators(std::move(iters)), body(bdy) {}

    // Constructor with expression initializer
    ForStatement(NodeList<ExpressionNode> initExprs,
                 NodePtr<ExpressionNode> cond,
                 NodeList<ExpressionNode> iters,
                 NodePtr<StatementNode> bdy,
                 const SourceLocation& loc = SourceLocation())
        : StatementNode(loc), initExpressions(std::move(initExprs)), 
          condition(cond), iterators(std::move(iters)), body(bdy) {}

    bool hasInitDeclaration() const { return initDeclaration != nullptr; }
    bool hasInitExpressions() const { return !initExpressions.empty(); }
    bool hasCondition() const { return condition != nullptr; }
    bool hasIterators() const { return !iterators.empty(); }

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// WhileStatement - while (cond) body
//================================================================================
class WhileStatement : public StatementNode {
public:
    NodePtr<ExpressionNode> condition;
    NodePtr<StatementNode> body;

    WhileStatement(NodePtr<ExpressionNode> cond,
                   NodePtr<StatementNode> bdy,
                   const SourceLocation& loc = SourceLocation())
        : StatementNode(loc), condition(cond), body(bdy) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// BreakStatement - break;
//================================================================================
class BreakStatement : public StatementNode {
public:
    explicit BreakStatement(const SourceLocation& loc = SourceLocation())
        : StatementNode(loc) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// ContinueStatement - continue;
//================================================================================
class ContinueStatement : public StatementNode {
public:
    explicit ContinueStatement(const SourceLocation& loc = SourceLocation())
        : StatementNode(loc) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// DeleteStatement - delete target;
//================================================================================
class DeleteStatement : public StatementNode {
public:
    NodePtr<ExpressionNode> target;

    explicit DeleteStatement(NodePtr<ExpressionNode> tgt,
                             const SourceLocation& loc = SourceLocation())
        : StatementNode(loc), target(tgt) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// RawBlock - raw { ... } - disables safety checks
//================================================================================
class RawBlock : public StatementNode {
public:
    NodePtr<BlockStatement> body;

    explicit RawBlock(NodePtr<BlockStatement> bdy,
                      const SourceLocation& loc = SourceLocation())
        : StatementNode(loc), body(bdy) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

} // namespace ast
} // namespace mingus
