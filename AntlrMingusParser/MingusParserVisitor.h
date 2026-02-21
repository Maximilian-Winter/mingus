
// Generated from antlr4_grammar/MingusParser.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "MingusParser.h"


namespace AntlrMingusParser {

/**
 * This class defines an abstract visitor for a parse tree
 * produced by MingusParser.
 */
class  MingusParserVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by MingusParser.
   */
    virtual std::any visitProgram(MingusParser::ProgramContext *context) = 0;

    virtual std::any visitModule(MingusParser::ModuleContext *context) = 0;

    virtual std::any visitModuleBlock(MingusParser::ModuleBlockContext *context) = 0;

    virtual std::any visitModuleDeclaration(MingusParser::ModuleDeclarationContext *context) = 0;

    virtual std::any visitTypedefDeclaration(MingusParser::TypedefDeclarationContext *context) = 0;

    virtual std::any visitImportDefinition(MingusParser::ImportDefinitionContext *context) = 0;

    virtual std::any visitImportTarget(MingusParser::ImportTargetContext *context) = 0;

    virtual std::any visitExternDeclaration(MingusParser::ExternDeclarationContext *context) = 0;

    virtual std::any visitExternBody(MingusParser::ExternBodyContext *context) = 0;

    virtual std::any visitExternFunctionDeclaration(MingusParser::ExternFunctionDeclarationContext *context) = 0;

    virtual std::any visitClassDeclaration(MingusParser::ClassDeclarationContext *context) = 0;

    virtual std::any visitClassBlock(MingusParser::ClassBlockContext *context) = 0;

    virtual std::any visitClassMember(MingusParser::ClassMemberContext *context) = 0;

    virtual std::any visitConstructorDeclaration(MingusParser::ConstructorDeclarationContext *context) = 0;

    virtual std::any visitDestructorDeclaration(MingusParser::DestructorDeclarationContext *context) = 0;

    virtual std::any visitOperatorDeclaration(MingusParser::OperatorDeclarationContext *context) = 0;

    virtual std::any visitOverloadableOperator(MingusParser::OverloadableOperatorContext *context) = 0;

    virtual std::any visitInheritance(MingusParser::InheritanceContext *context) = 0;

    virtual std::any visitInterfaceDeclaration(MingusParser::InterfaceDeclarationContext *context) = 0;

    virtual std::any visitInterfaceBlock(MingusParser::InterfaceBlockContext *context) = 0;

    virtual std::any visitInterfaceMember(MingusParser::InterfaceMemberContext *context) = 0;

    virtual std::any visitStructDeclaration(MingusParser::StructDeclarationContext *context) = 0;

    virtual std::any visitStructBlock(MingusParser::StructBlockContext *context) = 0;

    virtual std::any visitStructMember(MingusParser::StructMemberContext *context) = 0;

    virtual std::any visitEnumDeclaration(MingusParser::EnumDeclarationContext *context) = 0;

    virtual std::any visitEnumMember(MingusParser::EnumMemberContext *context) = 0;

    virtual std::any visitFunctionDeclaration(MingusParser::FunctionDeclarationContext *context) = 0;

    virtual std::any visitReturnType(MingusParser::ReturnTypeContext *context) = 0;

    virtual std::any visitDefinitionParameters(MingusParser::DefinitionParametersContext *context) = 0;

    virtual std::any visitParameterList(MingusParser::ParameterListContext *context) = 0;

    virtual std::any visitParameter(MingusParser::ParameterContext *context) = 0;

    virtual std::any visitVariableDeclaration(MingusParser::VariableDeclarationContext *context) = 0;

    virtual std::any visitConstVariableDeclaration(MingusParser::ConstVariableDeclarationContext *context) = 0;

    virtual std::any visitTypedVariableDeclaration(MingusParser::TypedVariableDeclarationContext *context) = 0;

    virtual std::any visitInferredVariableDeclaration(MingusParser::InferredVariableDeclarationContext *context) = 0;

    virtual std::any visitTupleDestructuring(MingusParser::TupleDestructuringContext *context) = 0;

    virtual std::any visitTupleDestructureElement(MingusParser::TupleDestructureElementContext *context) = 0;

    virtual std::any visitStatement(MingusParser::StatementContext *context) = 0;

    virtual std::any visitBlock(MingusParser::BlockContext *context) = 0;

    virtual std::any visitExprStatement(MingusParser::ExprStatementContext *context) = 0;

    virtual std::any visitRawBlock(MingusParser::RawBlockContext *context) = 0;

    virtual std::any visitIfStatement(MingusParser::IfStatementContext *context) = 0;

    virtual std::any visitElseIfClause(MingusParser::ElseIfClauseContext *context) = 0;

    virtual std::any visitElseClause(MingusParser::ElseClauseContext *context) = 0;

    virtual std::any visitSwitchStatement(MingusParser::SwitchStatementContext *context) = 0;

    virtual std::any visitSwitchCase(MingusParser::SwitchCaseContext *context) = 0;

    virtual std::any visitSwitchDefault(MingusParser::SwitchDefaultContext *context) = 0;

    virtual std::any visitMatchStatement(MingusParser::MatchStatementContext *context) = 0;

    virtual std::any visitMatchExpression(MingusParser::MatchExpressionContext *context) = 0;

    virtual std::any visitMatchArm(MingusParser::MatchArmContext *context) = 0;

    virtual std::any visitMatchBody(MingusParser::MatchBodyContext *context) = 0;

    virtual std::any visitPattern(MingusParser::PatternContext *context) = 0;

    virtual std::any visitGuardedPattern(MingusParser::GuardedPatternContext *context) = 0;

    virtual std::any visitBasePattern(MingusParser::BasePatternContext *context) = 0;

    virtual std::any visitLiteralPattern(MingusParser::LiteralPatternContext *context) = 0;

    virtual std::any visitRangePattern(MingusParser::RangePatternContext *context) = 0;

    virtual std::any visitWildcardPattern(MingusParser::WildcardPatternContext *context) = 0;

    virtual std::any visitBindingPattern(MingusParser::BindingPatternContext *context) = 0;

    virtual std::any visitTuplePattern(MingusParser::TuplePatternContext *context) = 0;

    virtual std::any visitForStatement(MingusParser::ForStatementContext *context) = 0;

    virtual std::any visitForInitializer(MingusParser::ForInitializerContext *context) = 0;

    virtual std::any visitLocalVarInitializer(MingusParser::LocalVarInitializerContext *context) = 0;

    virtual std::any visitLocalVarDeclaration(MingusParser::LocalVarDeclarationContext *context) = 0;

    virtual std::any visitForIterator(MingusParser::ForIteratorContext *context) = 0;

    virtual std::any visitWhileStatement(MingusParser::WhileStatementContext *context) = 0;

    virtual std::any visitDoWhileStatement(MingusParser::DoWhileStatementContext *context) = 0;

    virtual std::any visitReturnStatement(MingusParser::ReturnStatementContext *context) = 0;

    virtual std::any visitLabeledStatement(MingusParser::LabeledStatementContext *context) = 0;

    virtual std::any visitBreakStatement(MingusParser::BreakStatementContext *context) = 0;

    virtual std::any visitContinueStatement(MingusParser::ContinueStatementContext *context) = 0;

    virtual std::any visitDeleteStatement(MingusParser::DeleteStatementContext *context) = 0;

    virtual std::any visitExpression(MingusParser::ExpressionContext *context) = 0;

    virtual std::any visitAssignment(MingusParser::AssignmentContext *context) = 0;

    virtual std::any visitAssignmentOperator(MingusParser::AssignmentOperatorContext *context) = 0;

    virtual std::any visitLambdaExpression(MingusParser::LambdaExpressionContext *context) = 0;

    virtual std::any visitCaptureList(MingusParser::CaptureListContext *context) = 0;

    virtual std::any visitCaptureDefault(MingusParser::CaptureDefaultContext *context) = 0;

    virtual std::any visitCaptureItem(MingusParser::CaptureItemContext *context) = 0;

    virtual std::any visitLambdaParameterList(MingusParser::LambdaParameterListContext *context) = 0;

    virtual std::any visitLambdaParameter(MingusParser::LambdaParameterContext *context) = 0;

    virtual std::any visitPipe(MingusParser::PipeContext *context) = 0;

    virtual std::any visitPipeTarget(MingusParser::PipeTargetContext *context) = 0;

    virtual std::any visitTernary(MingusParser::TernaryContext *context) = 0;

    virtual std::any visitLogicOr(MingusParser::LogicOrContext *context) = 0;

    virtual std::any visitLogicAnd(MingusParser::LogicAndContext *context) = 0;

    virtual std::any visitBitwiseOr(MingusParser::BitwiseOrContext *context) = 0;

    virtual std::any visitBitwiseXor(MingusParser::BitwiseXorContext *context) = 0;

    virtual std::any visitBitwiseAnd(MingusParser::BitwiseAndContext *context) = 0;

    virtual std::any visitEquality(MingusParser::EqualityContext *context) = 0;

    virtual std::any visitRelational(MingusParser::RelationalContext *context) = 0;

    virtual std::any visitShift(MingusParser::ShiftContext *context) = 0;

    virtual std::any visitAdditive(MingusParser::AdditiveContext *context) = 0;

    virtual std::any visitMultiplicative(MingusParser::MultiplicativeContext *context) = 0;

    virtual std::any visitCastExpression(MingusParser::CastExpressionContext *context) = 0;

    virtual std::any visitUnaryExpression(MingusParser::UnaryExpressionContext *context) = 0;

    virtual std::any visitPostfixExpression(MingusParser::PostfixExpressionContext *context) = 0;

    virtual std::any visitPrimaryExpression(MingusParser::PrimaryExpressionContext *context) = 0;

    virtual std::any visitPostfixOperation(MingusParser::PostfixOperationContext *context) = 0;

    virtual std::any visitNewExpression(MingusParser::NewExpressionContext *context) = 0;

    virtual std::any visitCallArguments(MingusParser::CallArgumentsContext *context) = 0;

    virtual std::any visitArgumentList(MingusParser::ArgumentListContext *context) = 0;

    virtual std::any visitElementAccess(MingusParser::ElementAccessContext *context) = 0;

    virtual std::any visitMemberAccess(MingusParser::MemberAccessContext *context) = 0;

    virtual std::any visitTupleExpression(MingusParser::TupleExpressionContext *context) = 0;

    virtual std::any visitTypeIdentifier(MingusParser::TypeIdentifierContext *context) = 0;

    virtual std::any visitPrimitiveType(MingusParser::PrimitiveTypeContext *context) = 0;

    virtual std::any visitFunctionType(MingusParser::FunctionTypeContext *context) = 0;

    virtual std::any visitTypeList(MingusParser::TypeListContext *context) = 0;

    virtual std::any visitTupleType(MingusParser::TupleTypeContext *context) = 0;

    virtual std::any visitTypeModifier(MingusParser::TypeModifierContext *context) = 0;

    virtual std::any visitRvalueReferenceLevel(MingusParser::RvalueReferenceLevelContext *context) = 0;

    virtual std::any visitReferenceLevel(MingusParser::ReferenceLevelContext *context) = 0;

    virtual std::any visitArrayDimension(MingusParser::ArrayDimensionContext *context) = 0;

    virtual std::any visitPointerLevel(MingusParser::PointerLevelContext *context) = 0;

    virtual std::any visitAccessModifier(MingusParser::AccessModifierContext *context) = 0;

    virtual std::any visitStaticModifier(MingusParser::StaticModifierContext *context) = 0;

    virtual std::any visitAbstractModifier(MingusParser::AbstractModifierContext *context) = 0;

    virtual std::any visitQualifiedName(MingusParser::QualifiedNameContext *context) = 0;

    virtual std::any visitPrefixOperator(MingusParser::PrefixOperatorContext *context) = 0;

    virtual std::any visitIncrementDecrementOperator(MingusParser::IncrementDecrementOperatorContext *context) = 0;

    virtual std::any visitTypeSizeOrAlign(MingusParser::TypeSizeOrAlignContext *context) = 0;

    virtual std::any visitString(MingusParser::StringContext *context) = 0;

    virtual std::any visitStringPart(MingusParser::StringPartContext *context) = 0;


};

}  // namespace AntlrMingusParser
