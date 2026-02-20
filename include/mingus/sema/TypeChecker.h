//================================================================================
// MINGUS v1 - Type Checker (Pass 3)
// Walks every expression and statement. Sets resolvedType on all ExpressionNodes.
// Checks type compatibility for assignments, calls, returns, etc.
// Resolves symbols for IdentifierExpression, QualifiedNameExpression, etc.
//================================================================================

#pragma once

#include "mingus/ast/ASTVisitor.h"
#include "mingus/ast/ASTNode.h"
#include "mingus/ast/Program.h"
#include "mingus/ast/Declarations.h"
#include "mingus/ast/Statements.h"
#include "mingus/ast/Expressions.h"
#include "mingus/ast/Patterns.h"
#include "mingus/ast/TypeNode.h"
#include "mingus/sema/SymbolTable.h"
#include "mingus/sema/TypeRegistry.h"
#include "mingus/sema/ErrorReporter.h"

#include <optional>
#include <vector>

namespace mingus {
namespace sema {

using namespace mingus::ast;

//================================================================================
// TypeChecker — ASTVisitor that type-checks the full program (Pass 3)
//================================================================================
class TypeChecker : public ASTVisitor {
public:
    TypeChecker(SymbolTable& table, TypeRegistry& registry, ErrorReporter& errors);

    // Entry point
    void check(ProgramNode& program);

    // Program structure
    void visit(ProgramNode& node) override;
    void visit(ModuleNode& node) override;
    void visit(ImportNode& node) override;

    // Type nodes (no-op — resolved via resolveTypeNode helper)
    void visit(TypeNode& node) override;
    void visit(PrimitiveTypeNode& node) override;
    void visit(NamedTypeNode& node) override;
    void visit(PointerTypeNode& node) override;
    void visit(ArrayTypeNode& node) override;
    void visit(TupleTypeNode& node) override;
    void visit(FunctionTypeNode& node) override;

    // Declarations
    void visit(VariableDeclaration& node) override;
    void visit(TupleDestructuringDeclaration& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(ConstructorDeclaration& node) override;
    void visit(DestructorDeclaration& node) override;
    void visit(OperatorDeclaration& node) override;
    void visit(ExternFunctionDeclaration& node) override;
    void visit(EnumMemberNode& node) override;
    void visit(EnumDeclaration& node) override;
    void visit(StructDeclaration& node) override;
    void visit(ClassDeclaration& node) override;
    void visit(InterfaceDeclaration& node) override;
    void visit(ParameterNode& node) override;

    // Statements
    void visit(BlockStatement& node) override;
    void visit(ExpressionStatement& node) override;
    void visit(ReturnStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(SwitchStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(BreakStatement& node) override;
    void visit(ContinueStatement& node) override;
    void visit(DeleteStatement& node) override;
    void visit(RawBlock& node) override;

    // Expressions
    void visit(IntegerLiteral& node) override;
    void visit(FloatLiteral& node) override;
    void visit(BoolLiteral& node) override;
    void visit(CharLiteral& node) override;
    void visit(StringLiteral& node) override;
    void visit(InterpolatedString& node) override;
    void visit(NullLiteral& node) override;
    void visit(IdentifierExpression& node) override;
    void visit(QualifiedNameExpression& node) override;
    void visit(MemberAccessExpression& node) override;
    void visit(ThisExpression& node) override;
    void visit(BinaryExpression& node) override;
    void visit(UnaryExpression& node) override;
    void visit(AssignmentExpression& node) override;
    void visit(TernaryExpression& node) override;
    void visit(CallExpression& node) override;
    void visit(NewExpression& node) override;
    void visit(IndexExpression& node) override;
    void visit(CastExpression& node) override;
    void visit(SizeOfExpression& node) override;
    void visit(AlignOfExpression& node) override;
    void visit(PipeExpression& node) override;
    void visit(MatchExpression& node) override;
    void visit(TupleExpression& node) override;
    void visit(LambdaExpression& node) override;

    // Patterns
    void visit(LiteralPattern& node) override;
    void visit(RangePattern& node) override;
    void visit(WildcardPattern& node) override;
    void visit(BindingPattern& node) override;
    void visit(TuplePattern& node) override;
    void visit(GuardedPattern& node) override;

private:
    SymbolTable& symbolTable_;
    TypeRegistry& registry_;
    ErrorReporter& errors_;
    Scope* currentScope_;

    // Track the current function for parameter info
    FunctionSymbol* currentFunction_;

    // Expected return type for the current callable body
    // (works for functions, constructors, destructors, and operators)
    TypePtr<Type> currentReturnType_;

    // Track the current type (struct/class) for 'this' expression
    TypeSymbol* currentType_;

    // Track the match subject type for binding pattern inference
    TypePtr<Type> matchSubjectType_;

    // Child scope index counter per scope level for anonymous scopes
    std::vector<size_t> childIndexStack_;

    // TypeNode resolution (same as TypeResolver)
    TypePtr<Type> resolveTypeNode(TypeNode* node);

    // Scope navigation
    void enterNamedScope(Scope* scope);
    void leaveNamedScope();
    void enterNextChildScope();
    void leaveChildScope();

    // Walk a list of statements
    void visitStatements(NodeList<StatementNode>& stmts);

    // Check that 'from' type is compatible with 'to' type; report error if not
    void checkAssignability(const Type* from, const Type* to,
                            const SourceLocation& loc,
                            const std::string& context = "");

    // Check that an expression is a valid lvalue
    bool isLValue(ExpressionNode* expr);

    // Get the type of a symbol
    TypePtr<Type> getSymbolType(Symbol* sym);

    // Map BinaryOp to OverloadableOp (nullopt if not overloadable)
    std::optional<OverloadableOp> binaryOpToOverloadableOp(BinaryOp op);

    // Check if a type is void
    bool isVoidType(const Type* t) const;
};

} // namespace sema
} // namespace mingus
