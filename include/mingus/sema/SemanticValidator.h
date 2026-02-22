#pragma once

// ============================================================================
// SemanticValidator.h — Pass 4: Semantic validation and capture analysis
//
// Responsibilities:
//   - Lambda capture analysis (determine captured variables, capture modes)
//   - Self-capture detection (letrec patterns)
//   - Non-escaping lambda detection (lambdas passed directly as arguments)
//   - RAII tracking (identify variables needing destructor calls per scope)
//   - Return completeness checking (all code paths return)
//   - Break/continue validation (must be inside loops)
//   - Abstract class method implementation checking
//   - Interface implementation completeness checking
//   - Match expression exhaustiveness checking
//
// Requires Pass 1 (scope tree), Pass 2 (types), Pass 3 (type checking).
// ============================================================================

#include "mingus/AstNode.h"
#include "mingus/Expressions.h"
#include "mingus/Statements.h"
#include "mingus/Declarations.h"
#include "mingus/Symbols.h"
#include "mingus/SymbolTable.h"
#include "mingus/sema/ErrorReporter.h"

#include <set>
#include <unordered_map>

namespace mingus {

// ============================================================================
// ScopeRAIIInfo — Per-scope RAII tracking
//
// Stored for each scope that contains variables requiring destructor calls.
// Codegen reverses the vector for LIFO cleanup.
// ============================================================================

struct ScopeRAIIInfo {
    struct Destructible {
        VariableSymbol* variable;
        DestructorSymbol* destructor;
    };
    std::vector<Destructible> destructibles;
};

// ============================================================================
// Reachability — Return-path analysis classification
// ============================================================================

enum class Reachability {
    AlwaysReturns,      // All code paths reach a return
    SometimesReturns,   // Some paths return, some don't
    NeverReturns        // No return statement reached
};

// ============================================================================
// SemanticValidator — Pass 4 visitor
// ============================================================================

class SemanticValidator : public ASTVisitor {
public:
    SemanticValidator(SymbolTable& symbolTable, ErrorReporter& errors);

    // Entry point
    void validate(ProgramNode& program);

    // ---- RAII info accessor (used by codegen) ----
    const std::unordered_map<Scope*, ScopeRAIIInfo>& getRAIIInfo() const;

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
    void visit(MoveExpression& node) override;
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
    // Loop label stack (replaces simple loopDepth_ for labeled break/continue)
    std::vector<std::string> loopLabelStack_;  // "" for unlabeled loops
    std::string pendingLabel_;  // set by LabeledStatement, consumed by next loop
    TypeSymbolPtr currentReturnType_;

    // ---- RAII tracking ----
    std::unordered_map<Scope*, ScopeRAIIInfo> raiiInfo_;
    Scope* currentScope_ = nullptr;

    // ---- Lambda capture analysis ----
    struct LambdaContext {
        LambdaExpression* lambda;
        std::set<Symbol*> localSymbols;  // params + declarations owned by this lambda
    };
    std::vector<LambdaContext> lambdaStack_;

    // ---- By-reference capture escape tracking ----
    std::set<Symbol*> refCaptureVars_;  // variables holding closures with by-ref captures

    // ---- Helpers ----
    void visitStatements(std::vector<std::shared_ptr<StatementBaseNode>>& stmts);

    // RAII: check if a variable needs destructor tracking
    void trackRAIIVariable(VariableSymbol* var);

    // Lambda captures: check if an identifier needs capturing
    void checkLambdaCapture(IdentifierExpression& node);

    // Lambda: detect self-capture in variable initializer
    void checkSelfCapture(VariableDeclaration& node);

    // Return completeness analysis
    Reachability classifyStatement(StatementBaseNode* stmt);
    Reachability classifyBlock(BlockStatementNode* block);

    // Check function body return completeness
    void checkReturnCompleteness(const std::string& funcName,
                                  TypeSymbolPtr returnType,
                                  BlockStatementNode* body,
                                  const std::shared_ptr<DebugInfo>& loc);

    // Check class implements all abstract base methods
    void checkAbstractImplementation(ClassSymbol* cls,
                                      const std::shared_ptr<DebugInfo>& loc);

    // Check class implements all interface methods
    void checkInterfaceImplementation(ClassSymbol* cls,
                                       InterfaceSymbol* iface,
                                       const std::shared_ptr<DebugInfo>& loc);

    // Check override return types are covariant
    void checkOverrideCovariance(ClassSymbol* cls,
                                  const std::shared_ptr<DebugInfo>& loc);

    // Match exhaustiveness
    void checkExhaustiveness(MatchExpression& node);
};

} // namespace mingus
