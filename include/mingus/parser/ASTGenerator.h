#pragma once

//================================================================================
// MINGUS V2 - AST Generator
//
// ANTLR4 Visitor that converts parse trees to V2 AST nodes.
// Reuses V1 grammar (MingusLexer.g4 / MingusParser.g4) unchanged.
// Produces V2 AST nodes (different hierarchy from V1):
//   - Default-constructed shared_ptr with field assignment
//   - DebugInfo instead of SourceLocation
//   - V2 enums: BinaryOp, UnaryOp, AssignOp, OverloadableOp, etc.
//================================================================================

#include "mingus/AstNode.h"
#include "mingus/Expressions.h"
#include "mingus/Statements.h"
#include "mingus/Declarations.h"
#include "MingusParserBaseVisitor.h"

#include <any>
#include <memory>
#include <string>
#include <vector>

namespace mingus {
namespace parser {

using namespace AntlrMingusParser;

//================================================================================
// ASTGenerator — Converts ANTLR4 parse trees to V2 AST
//================================================================================
class ASTGenerator : public MingusParserBaseVisitor {
public:
    // Error tracking
    struct Error {
        int line = 0;
        int column = 0;
        std::string message;
    };
    std::vector<Error> errors;

    // Entry point
    std::shared_ptr<ProgramNode> generate(MingusParser::ProgramContext* ctx);

    // Error reporting
    void reportError(antlr4::ParserRuleContext* ctx, const std::string& message);
    bool hasErrors() const { return !errors.empty(); }

    // Visitor overrides — Program Structure
    std::any visitProgram(MingusParser::ProgramContext* ctx) override;
    std::any visitModule(MingusParser::ModuleContext* ctx) override;
    std::any visitModuleBlock(MingusParser::ModuleBlockContext* ctx) override;
    std::any visitImportDefinition(MingusParser::ImportDefinitionContext* ctx) override;
    std::any visitImportTarget(MingusParser::ImportTargetContext* ctx) override;

    // Visitor overrides — Declarations
    std::any visitClassDeclaration(MingusParser::ClassDeclarationContext* ctx) override;
    std::any visitInterfaceDeclaration(MingusParser::InterfaceDeclarationContext* ctx) override;
    std::any visitStructDeclaration(MingusParser::StructDeclarationContext* ctx) override;
    std::any visitEnumDeclaration(MingusParser::EnumDeclarationContext* ctx) override;
    std::any visitEnumMember(MingusParser::EnumMemberContext* ctx) override;
    std::any visitFunctionDeclaration(MingusParser::FunctionDeclarationContext* ctx) override;
    std::any visitConstructorDeclaration(MingusParser::ConstructorDeclarationContext* ctx) override;
    std::any visitDestructorDeclaration(MingusParser::DestructorDeclarationContext* ctx) override;
    std::any visitOperatorDeclaration(MingusParser::OperatorDeclarationContext* ctx) override;
    std::any visitExternFunctionDeclaration(MingusParser::ExternFunctionDeclarationContext* ctx) override;
    std::any visitParameter(MingusParser::ParameterContext* ctx) override;
    std::any visitVariableDeclaration(MingusParser::VariableDeclarationContext* ctx) override;
    std::any visitTypedVariableDeclaration(MingusParser::TypedVariableDeclarationContext* ctx) override;
    std::any visitInferredVariableDeclaration(MingusParser::InferredVariableDeclarationContext* ctx) override;
    std::any visitConstVariableDeclaration(MingusParser::ConstVariableDeclarationContext* ctx) override;
    std::any visitTupleDestructuring(MingusParser::TupleDestructuringContext* ctx) override;
    std::any visitTupleDestructureElement(MingusParser::TupleDestructureElementContext* ctx) override;
    std::any visitTypedefDeclaration(MingusParser::TypedefDeclarationContext* ctx) override;

    // Visitor overrides — Statements
    std::any visitBlock(MingusParser::BlockContext* ctx) override;
    std::any visitStatement(MingusParser::StatementContext* ctx) override;
    std::any visitExprStatement(MingusParser::ExprStatementContext* ctx) override;
    std::any visitReturnStatement(MingusParser::ReturnStatementContext* ctx) override;
    std::any visitIfStatement(MingusParser::IfStatementContext* ctx) override;
    std::any visitElseIfClause(MingusParser::ElseIfClauseContext* ctx) override;
    std::any visitElseClause(MingusParser::ElseClauseContext* ctx) override;
    std::any visitSwitchStatement(MingusParser::SwitchStatementContext* ctx) override;
    std::any visitSwitchCase(MingusParser::SwitchCaseContext* ctx) override;
    std::any visitSwitchDefault(MingusParser::SwitchDefaultContext* ctx) override;
    std::any visitForStatement(MingusParser::ForStatementContext* ctx) override;
    std::any visitWhileStatement(MingusParser::WhileStatementContext* ctx) override;
    std::any visitDoWhileStatement(MingusParser::DoWhileStatementContext* ctx) override;
    std::any visitBreakStatement(MingusParser::BreakStatementContext* ctx) override;
    std::any visitContinueStatement(MingusParser::ContinueStatementContext* ctx) override;
    std::any visitDeleteStatement(MingusParser::DeleteStatementContext* ctx) override;
    std::any visitRawBlock(MingusParser::RawBlockContext* ctx) override;

    // Visitor overrides — Expressions
    std::any visitExpression(MingusParser::ExpressionContext* ctx) override;
    std::any visitAssignment(MingusParser::AssignmentContext* ctx) override;
    std::any visitLambdaExpression(MingusParser::LambdaExpressionContext* ctx) override;
    std::any visitPipe(MingusParser::PipeContext* ctx) override;
    std::any visitTernary(MingusParser::TernaryContext* ctx) override;
    std::any visitLogicOr(MingusParser::LogicOrContext* ctx) override;
    std::any visitLogicAnd(MingusParser::LogicAndContext* ctx) override;
    std::any visitBitwiseOr(MingusParser::BitwiseOrContext* ctx) override;
    std::any visitBitwiseXor(MingusParser::BitwiseXorContext* ctx) override;
    std::any visitBitwiseAnd(MingusParser::BitwiseAndContext* ctx) override;
    std::any visitEquality(MingusParser::EqualityContext* ctx) override;
    std::any visitRelational(MingusParser::RelationalContext* ctx) override;
    std::any visitShift(MingusParser::ShiftContext* ctx) override;
    std::any visitAdditive(MingusParser::AdditiveContext* ctx) override;
    std::any visitMultiplicative(MingusParser::MultiplicativeContext* ctx) override;
    std::any visitCastExpression(MingusParser::CastExpressionContext* ctx) override;
    std::any visitUnaryExpression(MingusParser::UnaryExpressionContext* ctx) override;
    std::any visitPostfixExpression(MingusParser::PostfixExpressionContext* ctx) override;
    std::any visitPrimaryExpression(MingusParser::PrimaryExpressionContext* ctx) override;
    std::any visitNewExpression(MingusParser::NewExpressionContext* ctx) override;
    std::any visitTupleExpression(MingusParser::TupleExpressionContext* ctx) override;
    std::any visitMatchExpression(MingusParser::MatchExpressionContext* ctx) override;
    std::any visitMatchArm(MingusParser::MatchArmContext* ctx) override;
    std::any visitCallArguments(MingusParser::CallArgumentsContext* ctx) override;
    std::any visitElementAccess(MingusParser::ElementAccessContext* ctx) override;
    std::any visitMemberAccess(MingusParser::MemberAccessContext* ctx) override;

    // Visitor overrides — Patterns
    std::any visitPattern(MingusParser::PatternContext* ctx) override;
    std::any visitGuardedPattern(MingusParser::GuardedPatternContext* ctx) override;
    std::any visitLiteralPattern(MingusParser::LiteralPatternContext* ctx) override;
    std::any visitRangePattern(MingusParser::RangePatternContext* ctx) override;
    std::any visitWildcardPattern(MingusParser::WildcardPatternContext* ctx) override;
    std::any visitBindingPattern(MingusParser::BindingPatternContext* ctx) override;
    std::any visitTuplePattern(MingusParser::TuplePatternContext* ctx) override;

    // Visitor overrides — Types
    std::any visitTypeIdentifier(MingusParser::TypeIdentifierContext* ctx) override;
    std::any visitPrimitiveType(MingusParser::PrimitiveTypeContext* ctx) override;
    std::any visitTupleType(MingusParser::TupleTypeContext* ctx) override;
    std::any visitFunctionType(MingusParser::FunctionTypeContext* ctx) override;
    std::any visitArrayDimension(MingusParser::ArrayDimensionContext* ctx) override;
    std::any visitPointerLevel(MingusParser::PointerLevelContext* ctx) override;
    std::any visitReturnType(MingusParser::ReturnTypeContext* ctx) override;

    // Visitor overrides — Utilities
    std::any visitQualifiedName(MingusParser::QualifiedNameContext* ctx) override;
    std::any visitAccessModifier(MingusParser::AccessModifierContext* ctx) override;
    std::any visitStaticModifier(MingusParser::StaticModifierContext* ctx) override;
    std::any visitAbstractModifier(MingusParser::AbstractModifierContext* ctx) override;
    std::any visitOverloadableOperator(MingusParser::OverloadableOperatorContext* ctx) override;
    std::any visitString(MingusParser::StringContext* ctx) override;

private:
    // DebugInfo helper
    std::shared_ptr<DebugInfo> makeDebugInfo(antlr4::ParserRuleContext* ctx);

    // Parse helpers
    std::vector<std::string> parseQualifiedName(MingusParser::QualifiedNameContext* ctx);
    AccessModifier parseAccessModifier(MingusParser::AccessModifierContext* ctx);
    AssignOp parseAssignmentOperator(MingusParser::AssignmentOperatorContext* ctx);
    BinaryOp parseBinaryOperator(const std::string& op);
    UnaryOp parseUnaryOperator(const std::string& op);
    OverloadableOp parseOperatorKind(MingusParser::OverloadableOperatorContext* ctx);

    // Template helpers — extract shared_ptr from std::any
    template<typename T>
    std::shared_ptr<T> anyToNode(const std::any& a) {
        if (!a.has_value()) return nullptr;
        try {
            return std::any_cast<std::shared_ptr<T>>(a);
        } catch (...) {
            // Try casting from base types
            try {
                auto base = std::any_cast<std::shared_ptr<ExpressionBaseNode>>(a);
                return std::dynamic_pointer_cast<T>(base);
            } catch (...) {}
            try {
                auto base = std::any_cast<std::shared_ptr<StatementBaseNode>>(a);
                return std::dynamic_pointer_cast<T>(base);
            } catch (...) {}
            try {
                auto base = std::any_cast<std::shared_ptr<DeclarationBaseNode>>(a);
                return std::dynamic_pointer_cast<T>(base);
            } catch (...) {}
            try {
                auto base = std::any_cast<std::shared_ptr<TypeNode>>(a);
                return std::dynamic_pointer_cast<T>(base);
            } catch (...) {}
            try {
                auto base = std::any_cast<std::shared_ptr<PatternNode>>(a);
                return std::dynamic_pointer_cast<T>(base);
            } catch (...) {}
            try {
                auto base = std::any_cast<std::shared_ptr<AstBaseNode>>(a);
                return std::dynamic_pointer_cast<T>(base);
            } catch (...) {}
            return nullptr;
        }
    }

    template<typename T>
    std::vector<std::shared_ptr<T>> anyToNodeList(const std::any& a) {
        if (!a.has_value()) return {};
        try {
            return std::any_cast<std::vector<std::shared_ptr<T>>>(a);
        } catch (...) {
            return {};
        }
    }
};

} // namespace parser
} // namespace mingus
