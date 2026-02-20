//================================================================================
// MINGUS v1 - Symbol Table Builder (Pass 1)
// Walks the AST top-down, creates symbols for every declaration,
// and builds the scope tree. Does NOT resolve types.
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
#include "mingus/sema/ErrorReporter.h"

namespace mingus {
namespace sema {

using namespace mingus::ast;

//================================================================================
// SymbolTableBuilder — ASTVisitor that builds the symbol table (Pass 1)
//================================================================================
class SymbolTableBuilder : public ASTVisitor {
public:
    SymbolTableBuilder(SymbolTable& table, ErrorReporter& errors);

    // Entry point: build the symbol table from a program AST
    void build(ProgramNode& program);

    // Program structure
    void visit(ProgramNode& node) override;
    void visit(ModuleNode& node) override;
    void visit(ImportNode& node) override;

    // Type nodes (no-op in Pass 1 — types resolved in Pass 2)
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

    // Expressions (mostly no-op in Pass 1, but we walk bodies)
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

    // Patterns (mostly no-op in Pass 1)
    void visit(LiteralPattern& node) override;
    void visit(RangePattern& node) override;
    void visit(WildcardPattern& node) override;
    void visit(BindingPattern& node) override;
    void visit(TuplePattern& node) override;
    void visit(GuardedPattern& node) override;

private:
    SymbolTable& symbolTable_;
    ErrorReporter& errors_;
    Scope* currentScope_;

    // Helper to define a symbol in the current scope with redefinition checking
    bool defineSymbol(Symbol* symbol);

    // Push/pop scope helpers
    Scope* pushScope(ScopeKind kind, Symbol* owner = nullptr);
    void popScope();

    // Walk statement lists (for blocks and similar)
    void visitStatements(NodeList<StatementNode>& stmts);

    // Determine if current scope is TypeMembers (we're inside a struct/class)
    bool isInTypeScope() const;

    // Pass 1b: resolve import statements after all module scopes are built
    void resolveAllImports(ProgramNode& node);

    // Build vtable and allFields for a class symbol
    void buildVtable(ClassSymbol* sym);
};

} // namespace sema
} // namespace mingus
