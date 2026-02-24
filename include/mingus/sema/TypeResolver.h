#pragma once

// ============================================================================
// TypeResolver.h — Pass 2: Resolve type annotations and infer types
//
// Responsibilities:
//   - Resolve TypeNode AST annotations → TypeSymbol instances
//   - Set VariableSymbol::type for all variables (declared or inferred)
//   - Set FunctionSymbol::returnType from return type annotations
//   - Resolve function parameter types (with ReferenceType unwrapping)
//   - Set EnumSymbol::underlyingType
//   - Populate ClassSymbol/StructSymbol field types
//
// Critical invariant from V1:
//   ReferenceType (T&) in parameters must be UNWRAPPED:
//     VariableSymbol::type = base type, isReference = true
//   This ensures codegen's mapType() gets the correct LLVM type.
//
// Error recovery: unresolved types become ErrorTypeSymbol.
// ============================================================================

#include "mingus/AstNode.h"
#include "mingus/Expressions.h"
#include "mingus/Statements.h"
#include "mingus/Declarations.h"
#include "mingus/Symbols.h"
#include "mingus/SymbolTable.h"
#include "mingus/sema/ErrorReporter.h"

namespace mingus {

class TypeResolver : public ASTVisitor {
public:
    TypeResolver(SymbolTable& symbolTable, ErrorReporter& errors);

    // Entry point
    void resolve(ProgramNode& program);

    // ---- Visitor overrides ----
    void visit(ProgramNode& node) override;
    void visit(ModuleNode& node) override;
    void visit(BlockStatementNode& node) override;

    // Declarations (resolve type annotations)
    void visit(VariableDeclaration& node) override;
    void visit(TupleDestructuringDeclaration& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(ConstructorDeclaration& node) override;
    void visit(DestructorDeclaration& node) override;
    void visit(ExternFunctionDeclaration& node) override;
    void visit(ExternVariableDeclaration& node) override;
    void visit(OperatorDeclaration& node) override;
    void visit(EnumDeclaration& node) override;
    void visit(StructDeclaration& node) override;
    void visit(ClassDeclaration& node) override;
    void visit(InterfaceDeclaration& node) override;
    void visit(ImportDeclaration& node) override;
    void visit(ExternStructDeclaration& node) override;
    void visit(UnionDeclaration& node) override;
    void visit(ExternUnionDeclaration& node) override;

    // Statements (recurse)
    void visit(ExpressionStatement& node) override;
    void visit(ReturnStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(TypedefDeclaration& node) override;
    void visit(DoWhileStatement& node) override;
    void visit(LabeledStatement& node) override;
    void visit(BreakStatement& node) override;
    void visit(ContinueStatement& node) override;
    void visit(DeleteStatement& node) override;
    void visit(SwitchStatement& node) override;

    // Expressions (recurse for nested lambdas/var-decl-exprs)
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

    // ---- Core resolution helpers ----

    // Resolve a TypeNode annotation → TypeSymbol.
    // Returns ErrorTypeSymbol on failure.
    TypeSymbolPtr resolveTypeNode(TypeNode* typeNode);
    TypeSymbolPtr resolveTypeNode(const std::shared_ptr<TypeNode>& typeNode);

    // Resolve a named type by looking up in scope chain
    TypeSymbolPtr resolveNamedType(const std::vector<std::string>& qualifiedName,
                                   Scope* scope,
                                   const std::shared_ptr<DebugInfo>& loc);

    // Resolve function parameters: set type on VariableSymbol, unwrap refs
    void resolveParameters(
        const std::vector<std::shared_ptr<ParameterNode>>& params,
        const std::shared_ptr<FunctionSymbol>& funcSym);

    // Resolve return type annotation and set on FunctionSymbol
    void resolveReturnType(
        const std::shared_ptr<TypeNode>& returnTypeNode,
        const std::shared_ptr<FunctionSymbol>& funcSym);

    // Walk statements recursively
    void visitStatements(std::vector<std::shared_ptr<StatementBaseNode>>& stmts);
};

} // namespace mingus
