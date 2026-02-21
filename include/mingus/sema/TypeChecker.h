#pragma once

// ============================================================================
// TypeChecker.h — Pass 3: Expression type inference and compatibility checking
//
// Responsibilities:
//   - Infer resolvedType on every ExpressionBaseNode (bottom-up)
//   - Resolve IdentifierExpression → scope lookup → resolvedSymbol
//   - Resolve MemberAccessExpression (fields, methods, enum, static)
//   - Resolve CallExpression::resolvedCallee + ArgumentsNode::isReference
//   - Infer var x = expr; types (isInferred)
//   - Resolve operator overloads (BinaryExpression, IndexExpression)
//   - Check type compatibility at assignments, returns, calls
//   - Type-check conditions (if, while, for → bool)
//
// Requires Pass 1 (scope tree) and Pass 2 (type annotations) to be complete.
// ============================================================================

#include "mingus/AstNode.h"
#include "mingus/Expressions.h"
#include "mingus/Statements.h"
#include "mingus/Declarations.h"
#include "mingus/Symbols.h"
#include "mingus/SymbolTable.h"
#include "mingus/sema/ErrorReporter.h"

namespace mingus {

class TypeChecker : public ASTVisitor {
public:
    TypeChecker(SymbolTable& symbolTable, ErrorReporter& errors);

    // Entry point
    void check(ProgramNode& program);

    // ---- Visitor overrides ----
    void visit(ProgramNode& node) override;
    void visit(ModuleNode& node) override;
    void visit(BlockStatementNode& node) override;

    // Declarations
    void visit(VariableDeclaration& node) override;
    void visit(TupleDestructuringDeclaration& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(ConstructorDeclaration& node) override;
    void visit(DestructorDeclaration& node) override;
    void visit(ExternFunctionDeclaration& node) override;
    void visit(OperatorDeclaration& node) override;
    void visit(EnumDeclaration& node) override;
    void visit(StructDeclaration& node) override;
    void visit(ClassDeclaration& node) override;
    void visit(InterfaceDeclaration& node) override;
    void visit(ImportDeclaration& node) override;
    void visit(TypedefDeclaration& node) override;

    // Statements
    void visit(ExpressionStatement& node) override;
    void visit(ReturnStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(DoWhileStatement& node) override;
    void visit(LabeledStatement& node) override;
    void visit(BreakStatement& node) override;
    void visit(ContinueStatement& node) override;
    void visit(DeleteStatement& node) override;
    void visit(SwitchStatement& node) override;

    // Expressions
    void visit(IntegerLiteral& node) override;
    void visit(FloatLiteral& node) override;
    void visit(BoolLiteral& node) override;
    void visit(CharLiteral& node) override;
    void visit(StringLiteral& node) override;
    void visit(NullLiteral& node) override;
    void visit(InterpolatedStringExpression& node) override;
    void visit(IdentifierExpression& node) override;
    void visit(QualifiedNameExpression& node) override;
    void visit(ThisExpression& node) override;
    void visit(MemberAccessExpression& node) override;
    void visit(BinaryExpression& node) override;
    void visit(UnaryExpression& node) override;
    void visit(AssignmentExpression& node) override;
    void visit(TernaryExpression& node) override;
    void visit(IndexExpression& node) override;
    void visit(CallExpression& node) override;
    void visit(CastExpression& node) override;
    void visit(NewExpression& node) override;
    void visit(SizeOfExpression& node) override;
    void visit(TupleExpression& node) override;
    void visit(MatchExpression& node) override;
    void visit(PipeExpression& node) override;
    void visit(LambdaExpression& node) override;
    void visit(VariableDeclarationExpression& node) override;

private:
    SymbolTable& symbolTable_;
    ErrorReporter& errors_;

    // ---- Context tracking ----
    std::shared_ptr<FunctionSymbol> currentFunction_;
    TypeSymbolPtr currentReturnType_;
    TypeSymbol* currentClass_ = nullptr;  // ClassSymbol* or StructSymbol*

    // ---- Helpers ----
    void visitStatements(std::vector<std::shared_ptr<StatementBaseNode>>& stmts);
    void visitFunctionBody(const std::shared_ptr<FunctionSymbol>& funcSym,
                           BlockStatementNode& body);

    // Get the type of a symbol (variable type, function type, etc.)
    TypeSymbolPtr getSymbolType(const SymbolPtr& sym);

    // Check if a type is compatible with a target type
    bool checkAssignability(TypeSymbol* from, TypeSymbol* to,
                            const std::shared_ptr<DebugInfo>& loc,
                            const std::string& context = "");

    // Get the wider numeric type for binary arithmetic
    TypeSymbolPtr getWiderType(TypeSymbol* a, TypeSymbol* b);

    // Resolve operator overload on a type
    std::shared_ptr<OperatorSymbol> findOperatorOverload(
        TypeSymbol* type, OverloadableOp op);

    // Map BinaryOp to OverloadableOp (for overload resolution)
    OverloadableOp binaryOpToOverloadable(BinaryOp op);

    // Check that an expression is a valid lvalue
    bool isLValue(ExpressionBaseNode* expr);

    // Overload resolution: pick the best-matching function from candidates
    // Returns nullptr if no match or ambiguous
    std::shared_ptr<FunctionSymbol> resolveOverload(
        const std::vector<std::shared_ptr<FunctionSymbol>>& candidates,
        const std::shared_ptr<ArgumentsNode>& args);

    // Score how well argument types match parameter types (lower = better, -1 = no match)
    int scoreOverloadMatch(FunctionSymbol* func,
                           const std::shared_ptr<ArgumentsNode>& args);
};

} // namespace mingus
