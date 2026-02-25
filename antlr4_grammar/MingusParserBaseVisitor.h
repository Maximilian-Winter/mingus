
// Generated from MingusParser.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "MingusParserVisitor.h"


namespace AntlrMingusParser {

/**
 * This class provides an empty implementation of MingusParserVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  MingusParserBaseVisitor : public MingusParserVisitor {
public:

  virtual std::any visitProgram(MingusParser::ProgramContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitModule(MingusParser::ModuleContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitModuleBlock(MingusParser::ModuleBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitModuleDeclaration(MingusParser::ModuleDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTypedefDeclaration(MingusParser::TypedefDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitImportDefinition(MingusParser::ImportDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitImportTarget(MingusParser::ImportTargetContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExternDeclaration(MingusParser::ExternDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExternBody(MingusParser::ExternBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExternMember(MingusParser::ExternMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExternFunctionDeclaration(MingusParser::ExternFunctionDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExternLinkDirective(MingusParser::ExternLinkDirectiveContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExternOpaqueTypeDeclaration(MingusParser::ExternOpaqueTypeDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExternStructDeclaration(MingusParser::ExternStructDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExternUnionDeclaration(MingusParser::ExternUnionDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExternFieldDeclaration(MingusParser::ExternFieldDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExternEnumDeclaration(MingusParser::ExternEnumDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExternVariableDeclaration(MingusParser::ExternVariableDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitClassDeclaration(MingusParser::ClassDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitClassBlock(MingusParser::ClassBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitClassMember(MingusParser::ClassMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstructorDeclaration(MingusParser::ConstructorDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDestructorDeclaration(MingusParser::DestructorDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOperatorDeclaration(MingusParser::OperatorDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOverloadableOperator(MingusParser::OverloadableOperatorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInheritance(MingusParser::InheritanceContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInterfaceDeclaration(MingusParser::InterfaceDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInterfaceBlock(MingusParser::InterfaceBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInterfaceMember(MingusParser::InterfaceMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAttribute(MingusParser::AttributeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStructDeclaration(MingusParser::StructDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStructBlock(MingusParser::StructBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStructMember(MingusParser::StructMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnionDeclaration(MingusParser::UnionDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnionBlock(MingusParser::UnionBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnionMember(MingusParser::UnionMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTaggedUnionDeclaration(MingusParser::TaggedUnionDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTaggedUnionVariant(MingusParser::TaggedUnionVariantContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTaggedUnionField(MingusParser::TaggedUnionFieldContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEnumDeclaration(MingusParser::EnumDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEnumMember(MingusParser::EnumMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFunctionDeclaration(MingusParser::FunctionDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReturnType(MingusParser::ReturnTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDefinitionParameters(MingusParser::DefinitionParametersContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitParameterList(MingusParser::ParameterListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitParameter(MingusParser::ParameterContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVariableDeclaration(MingusParser::VariableDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstVariableDeclaration(MingusParser::ConstVariableDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTypedVariableDeclaration(MingusParser::TypedVariableDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInferredVariableDeclaration(MingusParser::InferredVariableDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTupleDestructuring(MingusParser::TupleDestructuringContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTupleDestructureElement(MingusParser::TupleDestructureElementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatement(MingusParser::StatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBlock(MingusParser::BlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExprStatement(MingusParser::ExprStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRawBlock(MingusParser::RawBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIfStatement(MingusParser::IfStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitElseIfClause(MingusParser::ElseIfClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitElseClause(MingusParser::ElseClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSwitchStatement(MingusParser::SwitchStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSwitchCase(MingusParser::SwitchCaseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSwitchDefault(MingusParser::SwitchDefaultContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMatchStatement(MingusParser::MatchStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMatchExpression(MingusParser::MatchExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMatchArm(MingusParser::MatchArmContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMatchBody(MingusParser::MatchBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPattern(MingusParser::PatternContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGuardedPattern(MingusParser::GuardedPatternContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBasePattern(MingusParser::BasePatternContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLiteralPattern(MingusParser::LiteralPatternContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRangePattern(MingusParser::RangePatternContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWildcardPattern(MingusParser::WildcardPatternContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBindingPattern(MingusParser::BindingPatternContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTuplePattern(MingusParser::TuplePatternContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVariantPatternField(MingusParser::VariantPatternFieldContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitForStatement(MingusParser::ForStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitForInitializer(MingusParser::ForInitializerContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLocalVarInitializer(MingusParser::LocalVarInitializerContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLocalVarDeclaration(MingusParser::LocalVarDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitForIterator(MingusParser::ForIteratorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWhileStatement(MingusParser::WhileStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDoWhileStatement(MingusParser::DoWhileStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReturnStatement(MingusParser::ReturnStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLabeledStatement(MingusParser::LabeledStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBreakStatement(MingusParser::BreakStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitContinueStatement(MingusParser::ContinueStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDeleteStatement(MingusParser::DeleteStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExpression(MingusParser::ExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAssignment(MingusParser::AssignmentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAssignmentOperator(MingusParser::AssignmentOperatorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLambdaExpression(MingusParser::LambdaExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCaptureList(MingusParser::CaptureListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCaptureDefault(MingusParser::CaptureDefaultContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCaptureItem(MingusParser::CaptureItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLambdaParameterList(MingusParser::LambdaParameterListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLambdaParameter(MingusParser::LambdaParameterContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPipe(MingusParser::PipeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPipeTarget(MingusParser::PipeTargetContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTernary(MingusParser::TernaryContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLogicOr(MingusParser::LogicOrContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLogicAnd(MingusParser::LogicAndContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBitwiseOr(MingusParser::BitwiseOrContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBitwiseXor(MingusParser::BitwiseXorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBitwiseAnd(MingusParser::BitwiseAndContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEquality(MingusParser::EqualityContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRelational(MingusParser::RelationalContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShift(MingusParser::ShiftContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAdditive(MingusParser::AdditiveContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMultiplicative(MingusParser::MultiplicativeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCastExpression(MingusParser::CastExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryExpression(MingusParser::UnaryExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPostfixExpression(MingusParser::PostfixExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrimaryExpression(MingusParser::PrimaryExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitArrayLiteral(MingusParser::ArrayLiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPostfixOperation(MingusParser::PostfixOperationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNewExpression(MingusParser::NewExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCallArguments(MingusParser::CallArgumentsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitArgumentList(MingusParser::ArgumentListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitElementAccess(MingusParser::ElementAccessContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMemberAccess(MingusParser::MemberAccessContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTupleExpression(MingusParser::TupleExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTypeIdentifier(MingusParser::TypeIdentifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrimitiveType(MingusParser::PrimitiveTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFunctionType(MingusParser::FunctionTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTypeList(MingusParser::TypeListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTupleType(MingusParser::TupleTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTypeModifier(MingusParser::TypeModifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRvalueReferenceLevel(MingusParser::RvalueReferenceLevelContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReferenceLevel(MingusParser::ReferenceLevelContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitArrayDimension(MingusParser::ArrayDimensionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPointerLevel(MingusParser::PointerLevelContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAccessModifier(MingusParser::AccessModifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStaticModifier(MingusParser::StaticModifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAbstractModifier(MingusParser::AbstractModifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitQualifiedName(MingusParser::QualifiedNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrefixOperator(MingusParser::PrefixOperatorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIncrementDecrementOperator(MingusParser::IncrementDecrementOperatorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTypeSizeOrAlign(MingusParser::TypeSizeOrAlignContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitString(MingusParser::StringContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStringPart(MingusParser::StringPartContext *ctx) override {
    return visitChildren(ctx);
  }


};

}  // namespace AntlrMingusParser
