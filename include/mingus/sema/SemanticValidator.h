//================================================================================
// MINGUS v1 - Semantic Validator (Pass 4)
// Performs final semantic validation after type checking:
//   4a: Control flow analysis (return completeness, break/continue validity)
//   4b: RAII analysis (destructor injection points per scope)
//   4c: Raw block safety (pointer ops only inside raw blocks)
//   4d: Pattern exhaustiveness (match expressions cover all cases)
//   4e: Lambda capture analysis (which variables are captured)
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

#include <set>
#include <unordered_map>
#include <vector>

namespace mingus {
namespace sema {

using namespace mingus::ast;

//================================================================================
// Reachability — classification for return completeness analysis
//================================================================================
enum class Reachability {
    AlwaysReturns,      // Guaranteed to hit a return on every path
    SometimesReturns,   // Some paths return, others do not
    NeverReturns        // No return statement reached
};

//================================================================================
// ScopeRAIIInfo — RAII-active variables in a scope, for codegen
//================================================================================
struct ScopeRAIIInfo {
    // Variables needing destructor calls at scope exit.
    // Ordered by declaration order; codegen reverses for LIFO destruction.
    std::vector<std::pair<VariableSymbol*, DestructorSymbol*>> destructibles;
};

//================================================================================
// SemanticValidator — ASTVisitor that performs Pass 4
//================================================================================
class SemanticValidator : public ASTVisitor {
public:
    SemanticValidator(SymbolTable& table, TypeRegistry& registry, ErrorReporter& errors);

    // Entry point
    void validate(ProgramNode& program);

    // RAII info accessor for codegen
    const std::unordered_map<Scope*, ScopeRAIIInfo>& getRAIIInfo() const;

    // Program structure
    void visit(ProgramNode& node) override;
    void visit(ModuleNode& node) override;
    void visit(ImportNode& node) override;

    // Type nodes (no-op)
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

    // Scope navigation
    std::vector<size_t> childIndexStack_;

    // 4a: Control flow
    int loopDepth_;
    TypePtr<Type> currentReturnType_;

    // 4b: RAII
    std::unordered_map<Scope*, ScopeRAIIInfo> raiiInfo_;

    // 4c: Raw block safety
    int rawDepth_;

    // 4e: Lambda captures
    struct LambdaContext {
        LambdaExpression* lambda;
        std::set<Symbol*> localSymbols;
    };
    std::vector<LambdaContext> lambdaStack_;

    // Scope navigation helpers
    void enterNamedScope(Scope* scope);
    void leaveNamedScope();
    void enterNextChildScope();
    void leaveChildScope();
    void visitStatements(NodeList<StatementNode>& stmts);

    // 4a: Reachability analysis (standalone, uses dynamic_cast)
    Reachability classifyStatement(StatementNode* stmt);
    Reachability classifyBlock(const NodeList<StatementNode>& stmts);
    bool isVoidReturn() const;

    // 4b: RAII helpers
    void trackRAIIVariable(VariableSymbol* var);

    // 4d: Pattern exhaustiveness
    void checkExhaustiveness(MatchExpression& node);

    // 4e: Lambda capture helpers
    void checkLambdaCapture(IdentifierExpression& node);
};

} // namespace sema
} // namespace mingus
