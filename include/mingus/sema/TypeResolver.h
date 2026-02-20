//================================================================================
// MINGUS v1 - Type Resolver (Pass 2)
// Walks declaration-level TypeNodes and converts them to concrete Type objects
// via TypeRegistry. After this pass, all Symbol type fields are populated.
// Does NOT enter function bodies — that is Pass 3's job.
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

namespace mingus {
namespace sema {

using namespace mingus::ast;

//================================================================================
// TypeResolver — ASTVisitor that resolves declaration-level types (Pass 2)
//================================================================================
class TypeResolver : public ASTVisitor {
public:
    TypeResolver(SymbolTable& table, TypeRegistry& registry, ErrorReporter& errors);

    // Entry point: resolve all declaration-level types
    void resolve(ProgramNode& program);

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

    // Statements (no-op — Pass 2 does not enter function bodies)
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

    // Expressions (no-op — Pass 2 does not resolve expressions)
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

    // Patterns (no-op)
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

    // Core helper: convert a TypeNode AST node to a resolved Type object.
    // Sets node->resolvedType as a side-effect.
    TypePtr<Type> resolveTypeNode(TypeNode* node);

    // Scope navigation helpers
    void enterScope(Scope* scope);
    void leaveScope();

    // Resolve function/operator parameter types from AST ParameterNodes
    void resolveParameters(NodeList<ParameterNode>& params,
                           std::vector<VariableSymbol*>& symbols);
};

} // namespace sema
} // namespace mingus
