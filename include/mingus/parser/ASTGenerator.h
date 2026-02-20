//================================================================================
// MINGUS v1 - AST Generator
// ANTLR4 Visitor that converts parse trees to AST nodes
//================================================================================

#pragma once

#include "mingus/AST.h"
#include "MingusParserBaseVisitor.h"

#include <any>
#include <memory>
#include <string>
#include <vector>

namespace mingus {
namespace parser {

using namespace AntlrMingusParser;

//================================================================================
// SourceLocation helper
//================================================================================
ast::SourceLocation getSourceLocation(antlr4::ParserRuleContext* ctx);

//================================================================================
// ASTGenerator - Converts ANTLR4 parse trees to Mingus AST
//================================================================================
class ASTGenerator : public MingusParserBaseVisitor {
public:
    // Error tracking
    struct Error {
        ast::SourceLocation location;
        std::string message;
    };
    std::vector<Error> errors;

    // Entry point
    std::shared_ptr<ast::ProgramNode> generate(MingusParser::ProgramContext* ctx);

    // Helper methods for error reporting
    void reportError(antlr4::ParserRuleContext* ctx, const std::string& message);
    bool hasErrors() const { return !errors.empty(); }

    // Visitor overrides - Program Structure
    virtual std::any visitProgram(MingusParser::ProgramContext* ctx) override;
    virtual std::any visitModule(MingusParser::ModuleContext* ctx) override;
    virtual std::any visitModuleBlock(MingusParser::ModuleBlockContext* ctx) override;
    virtual std::any visitImportDefinition(MingusParser::ImportDefinitionContext* ctx) override;
    virtual std::any visitImportTarget(MingusParser::ImportTargetContext* ctx) override;

    // Visitor overrides - Declarations
    virtual std::any visitClassDeclaration(MingusParser::ClassDeclarationContext* ctx) override;
    virtual std::any visitInterfaceDeclaration(MingusParser::InterfaceDeclarationContext* ctx) override;
    virtual std::any visitStructDeclaration(MingusParser::StructDeclarationContext* ctx) override;
    virtual std::any visitEnumDeclaration(MingusParser::EnumDeclarationContext* ctx) override;
    virtual std::any visitEnumMember(MingusParser::EnumMemberContext* ctx) override;
    virtual std::any visitFunctionDeclaration(MingusParser::FunctionDeclarationContext* ctx) override;
    virtual std::any visitConstructorDeclaration(MingusParser::ConstructorDeclarationContext* ctx) override;
    virtual std::any visitDestructorDeclaration(MingusParser::DestructorDeclarationContext* ctx) override;
    virtual std::any visitOperatorDeclaration(MingusParser::OperatorDeclarationContext* ctx) override;
    virtual std::any visitExternFunctionDeclaration(MingusParser::ExternFunctionDeclarationContext* ctx) override;
    virtual std::any visitParameter(MingusParser::ParameterContext* ctx) override;
    virtual std::any visitVariableDeclaration(MingusParser::VariableDeclarationContext* ctx) override;
    virtual std::any visitTypedVariableDeclaration(MingusParser::TypedVariableDeclarationContext* ctx) override;
    virtual std::any visitInferredVariableDeclaration(MingusParser::InferredVariableDeclarationContext* ctx) override;
    virtual std::any visitTupleDestructuring(MingusParser::TupleDestructuringContext* ctx) override;
    virtual std::any visitTupleDestructureElement(MingusParser::TupleDestructureElementContext* ctx) override;

    // Visitor overrides - Statements
    virtual std::any visitBlock(MingusParser::BlockContext* ctx) override;
    virtual std::any visitStatement(MingusParser::StatementContext* ctx) override;
    virtual std::any visitExprStatement(MingusParser::ExprStatementContext* ctx) override;
    virtual std::any visitReturnStatement(MingusParser::ReturnStatementContext* ctx) override;
    virtual std::any visitIfStatement(MingusParser::IfStatementContext* ctx) override;
    virtual std::any visitElseIfClause(MingusParser::ElseIfClauseContext* ctx) override;
    virtual std::any visitElseClause(MingusParser::ElseClauseContext* ctx) override;
    virtual std::any visitSwitchStatement(MingusParser::SwitchStatementContext* ctx) override;
    virtual std::any visitSwitchCase(MingusParser::SwitchCaseContext* ctx) override;
    virtual std::any visitSwitchDefault(MingusParser::SwitchDefaultContext* ctx) override;
    virtual std::any visitForStatement(MingusParser::ForStatementContext* ctx) override;
    virtual std::any visitWhileStatement(MingusParser::WhileStatementContext* ctx) override;
    virtual std::any visitBreakStatement(MingusParser::BreakStatementContext* ctx) override;
    virtual std::any visitContinueStatement(MingusParser::ContinueStatementContext* ctx) override;
    virtual std::any visitDeleteStatement(MingusParser::DeleteStatementContext* ctx) override;
    virtual std::any visitRawBlock(MingusParser::RawBlockContext* ctx) override;

    // Visitor overrides - Expressions
    virtual std::any visitExpression(MingusParser::ExpressionContext* ctx) override;
    virtual std::any visitAssignment(MingusParser::AssignmentContext* ctx) override;
    virtual std::any visitLambdaExpression(MingusParser::LambdaExpressionContext* ctx) override;
    virtual std::any visitPipe(MingusParser::PipeContext* ctx) override;
    virtual std::any visitTernary(MingusParser::TernaryContext* ctx) override;
    virtual std::any visitLogicOr(MingusParser::LogicOrContext* ctx) override;
    virtual std::any visitLogicAnd(MingusParser::LogicAndContext* ctx) override;
    virtual std::any visitBitwiseOr(MingusParser::BitwiseOrContext* ctx) override;
    virtual std::any visitBitwiseXor(MingusParser::BitwiseXorContext* ctx) override;
    virtual std::any visitBitwiseAnd(MingusParser::BitwiseAndContext* ctx) override;
    virtual std::any visitEquality(MingusParser::EqualityContext* ctx) override;
    virtual std::any visitRelational(MingusParser::RelationalContext* ctx) override;
    virtual std::any visitShift(MingusParser::ShiftContext* ctx) override;
    virtual std::any visitAdditive(MingusParser::AdditiveContext* ctx) override;
    virtual std::any visitMultiplicative(MingusParser::MultiplicativeContext* ctx) override;
    virtual std::any visitCastExpression(MingusParser::CastExpressionContext* ctx) override;
    virtual std::any visitUnaryExpression(MingusParser::UnaryExpressionContext* ctx) override;
    virtual std::any visitPostfixExpression(MingusParser::PostfixExpressionContext* ctx) override;
    virtual std::any visitPrimaryExpression(MingusParser::PrimaryExpressionContext* ctx) override;
    virtual std::any visitNewExpression(MingusParser::NewExpressionContext* ctx) override;
    virtual std::any visitTupleExpression(MingusParser::TupleExpressionContext* ctx) override;
    virtual std::any visitMatchExpression(MingusParser::MatchExpressionContext* ctx) override;
    virtual std::any visitMatchArm(MingusParser::MatchArmContext* ctx) override;
    virtual std::any visitCallArguments(MingusParser::CallArgumentsContext* ctx) override;
    virtual std::any visitElementAccess(MingusParser::ElementAccessContext* ctx) override;
    virtual std::any visitMemberAccess(MingusParser::MemberAccessContext* ctx) override;

    // Visitor overrides - Patterns
    virtual std::any visitPattern(MingusParser::PatternContext* ctx) override;
    virtual std::any visitGuardedPattern(MingusParser::GuardedPatternContext* ctx) override;
    virtual std::any visitLiteralPattern(MingusParser::LiteralPatternContext* ctx) override;
    virtual std::any visitRangePattern(MingusParser::RangePatternContext* ctx) override;
    virtual std::any visitWildcardPattern(MingusParser::WildcardPatternContext* ctx) override;
    virtual std::any visitBindingPattern(MingusParser::BindingPatternContext* ctx) override;
    virtual std::any visitTuplePattern(MingusParser::TuplePatternContext* ctx) override;

    // Visitor overrides - Types
    virtual std::any visitTypeIdentifier(MingusParser::TypeIdentifierContext* ctx) override;
    virtual std::any visitPrimitiveType(MingusParser::PrimitiveTypeContext* ctx) override;
    virtual std::any visitTupleType(MingusParser::TupleTypeContext* ctx) override;
    virtual std::any visitFunctionType(MingusParser::FunctionTypeContext* ctx) override;
    virtual std::any visitArrayDimension(MingusParser::ArrayDimensionContext* ctx) override;
    virtual std::any visitPointerLevel(MingusParser::PointerLevelContext* ctx) override;
    virtual std::any visitReturnType(MingusParser::ReturnTypeContext* ctx) override;

    // Visitor overrides - Utilities
    virtual std::any visitQualifiedName(MingusParser::QualifiedNameContext* ctx) override;
    virtual std::any visitAccessModifier(MingusParser::AccessModifierContext* ctx) override;
    virtual std::any visitStaticModifier(MingusParser::StaticModifierContext* ctx) override;
    virtual std::any visitAbstractModifier(MingusParser::AbstractModifierContext* ctx) override;
    virtual std::any visitOverloadableOperator(MingusParser::OverloadableOperatorContext* ctx) override;
    virtual std::any visitString(MingusParser::StringContext* ctx) override;

private:
    // Helper methods
    ast::QualifiedName parseQualifiedName(MingusParser::QualifiedNameContext* ctx);
    ast::AccessModifier parseAccessModifier(MingusParser::AccessModifierContext* ctx);
    ast::AssignOp parseAssignmentOperator(MingusParser::AssignmentOperatorContext* ctx);
    ast::BinaryOp parseBinaryOperator(const std::string& op);
    ast::UnaryOp parseUnaryOperator(const std::string& op);
    ast::OperatorKind parseOperatorKind(MingusParser::OverloadableOperatorContext* ctx);
    
    // Template helper to extract shared_ptr from std::any
    // Tries exact match first, then dynamic cast from ASTNode
    template<typename T>
    std::shared_ptr<T> anyToNode(const std::any& a) {
        if (!a.has_value()) return nullptr;
        try {
            // First try exact type match
            return std::any_cast<std::shared_ptr<T>>(a);
        } catch (...) {
            // If that fails, try casting from ASTNode base class
            try {
                auto base = std::any_cast<std::shared_ptr<ast::ASTNode>>(a);
                return std::dynamic_pointer_cast<T>(base);
            } catch (...) {
                return nullptr;
            }
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
