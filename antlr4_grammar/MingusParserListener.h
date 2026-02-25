
// Generated from MingusParser.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "MingusParser.h"


namespace AntlrMingusParser {

/**
 * This interface defines an abstract listener for a parse tree produced by MingusParser.
 */
class  MingusParserListener : public antlr4::tree::ParseTreeListener {
public:

  virtual void enterProgram(MingusParser::ProgramContext *ctx) = 0;
  virtual void exitProgram(MingusParser::ProgramContext *ctx) = 0;

  virtual void enterModule(MingusParser::ModuleContext *ctx) = 0;
  virtual void exitModule(MingusParser::ModuleContext *ctx) = 0;

  virtual void enterModuleBlock(MingusParser::ModuleBlockContext *ctx) = 0;
  virtual void exitModuleBlock(MingusParser::ModuleBlockContext *ctx) = 0;

  virtual void enterModuleDeclaration(MingusParser::ModuleDeclarationContext *ctx) = 0;
  virtual void exitModuleDeclaration(MingusParser::ModuleDeclarationContext *ctx) = 0;

  virtual void enterTypedefDeclaration(MingusParser::TypedefDeclarationContext *ctx) = 0;
  virtual void exitTypedefDeclaration(MingusParser::TypedefDeclarationContext *ctx) = 0;

  virtual void enterImportDefinition(MingusParser::ImportDefinitionContext *ctx) = 0;
  virtual void exitImportDefinition(MingusParser::ImportDefinitionContext *ctx) = 0;

  virtual void enterImportTarget(MingusParser::ImportTargetContext *ctx) = 0;
  virtual void exitImportTarget(MingusParser::ImportTargetContext *ctx) = 0;

  virtual void enterExternDeclaration(MingusParser::ExternDeclarationContext *ctx) = 0;
  virtual void exitExternDeclaration(MingusParser::ExternDeclarationContext *ctx) = 0;

  virtual void enterExternBody(MingusParser::ExternBodyContext *ctx) = 0;
  virtual void exitExternBody(MingusParser::ExternBodyContext *ctx) = 0;

  virtual void enterExternMember(MingusParser::ExternMemberContext *ctx) = 0;
  virtual void exitExternMember(MingusParser::ExternMemberContext *ctx) = 0;

  virtual void enterExternFunctionDeclaration(MingusParser::ExternFunctionDeclarationContext *ctx) = 0;
  virtual void exitExternFunctionDeclaration(MingusParser::ExternFunctionDeclarationContext *ctx) = 0;

  virtual void enterExternLinkDirective(MingusParser::ExternLinkDirectiveContext *ctx) = 0;
  virtual void exitExternLinkDirective(MingusParser::ExternLinkDirectiveContext *ctx) = 0;

  virtual void enterExternOpaqueTypeDeclaration(MingusParser::ExternOpaqueTypeDeclarationContext *ctx) = 0;
  virtual void exitExternOpaqueTypeDeclaration(MingusParser::ExternOpaqueTypeDeclarationContext *ctx) = 0;

  virtual void enterExternStructDeclaration(MingusParser::ExternStructDeclarationContext *ctx) = 0;
  virtual void exitExternStructDeclaration(MingusParser::ExternStructDeclarationContext *ctx) = 0;

  virtual void enterExternUnionDeclaration(MingusParser::ExternUnionDeclarationContext *ctx) = 0;
  virtual void exitExternUnionDeclaration(MingusParser::ExternUnionDeclarationContext *ctx) = 0;

  virtual void enterExternFieldDeclaration(MingusParser::ExternFieldDeclarationContext *ctx) = 0;
  virtual void exitExternFieldDeclaration(MingusParser::ExternFieldDeclarationContext *ctx) = 0;

  virtual void enterExternEnumDeclaration(MingusParser::ExternEnumDeclarationContext *ctx) = 0;
  virtual void exitExternEnumDeclaration(MingusParser::ExternEnumDeclarationContext *ctx) = 0;

  virtual void enterExternVariableDeclaration(MingusParser::ExternVariableDeclarationContext *ctx) = 0;
  virtual void exitExternVariableDeclaration(MingusParser::ExternVariableDeclarationContext *ctx) = 0;

  virtual void enterClassDeclaration(MingusParser::ClassDeclarationContext *ctx) = 0;
  virtual void exitClassDeclaration(MingusParser::ClassDeclarationContext *ctx) = 0;

  virtual void enterClassBlock(MingusParser::ClassBlockContext *ctx) = 0;
  virtual void exitClassBlock(MingusParser::ClassBlockContext *ctx) = 0;

  virtual void enterClassMember(MingusParser::ClassMemberContext *ctx) = 0;
  virtual void exitClassMember(MingusParser::ClassMemberContext *ctx) = 0;

  virtual void enterConstructorDeclaration(MingusParser::ConstructorDeclarationContext *ctx) = 0;
  virtual void exitConstructorDeclaration(MingusParser::ConstructorDeclarationContext *ctx) = 0;

  virtual void enterDestructorDeclaration(MingusParser::DestructorDeclarationContext *ctx) = 0;
  virtual void exitDestructorDeclaration(MingusParser::DestructorDeclarationContext *ctx) = 0;

  virtual void enterOperatorDeclaration(MingusParser::OperatorDeclarationContext *ctx) = 0;
  virtual void exitOperatorDeclaration(MingusParser::OperatorDeclarationContext *ctx) = 0;

  virtual void enterOverloadableOperator(MingusParser::OverloadableOperatorContext *ctx) = 0;
  virtual void exitOverloadableOperator(MingusParser::OverloadableOperatorContext *ctx) = 0;

  virtual void enterInheritance(MingusParser::InheritanceContext *ctx) = 0;
  virtual void exitInheritance(MingusParser::InheritanceContext *ctx) = 0;

  virtual void enterInterfaceDeclaration(MingusParser::InterfaceDeclarationContext *ctx) = 0;
  virtual void exitInterfaceDeclaration(MingusParser::InterfaceDeclarationContext *ctx) = 0;

  virtual void enterInterfaceBlock(MingusParser::InterfaceBlockContext *ctx) = 0;
  virtual void exitInterfaceBlock(MingusParser::InterfaceBlockContext *ctx) = 0;

  virtual void enterInterfaceMember(MingusParser::InterfaceMemberContext *ctx) = 0;
  virtual void exitInterfaceMember(MingusParser::InterfaceMemberContext *ctx) = 0;

  virtual void enterAttribute(MingusParser::AttributeContext *ctx) = 0;
  virtual void exitAttribute(MingusParser::AttributeContext *ctx) = 0;

  virtual void enterStructDeclaration(MingusParser::StructDeclarationContext *ctx) = 0;
  virtual void exitStructDeclaration(MingusParser::StructDeclarationContext *ctx) = 0;

  virtual void enterStructBlock(MingusParser::StructBlockContext *ctx) = 0;
  virtual void exitStructBlock(MingusParser::StructBlockContext *ctx) = 0;

  virtual void enterStructMember(MingusParser::StructMemberContext *ctx) = 0;
  virtual void exitStructMember(MingusParser::StructMemberContext *ctx) = 0;

  virtual void enterUnionDeclaration(MingusParser::UnionDeclarationContext *ctx) = 0;
  virtual void exitUnionDeclaration(MingusParser::UnionDeclarationContext *ctx) = 0;

  virtual void enterUnionBlock(MingusParser::UnionBlockContext *ctx) = 0;
  virtual void exitUnionBlock(MingusParser::UnionBlockContext *ctx) = 0;

  virtual void enterUnionMember(MingusParser::UnionMemberContext *ctx) = 0;
  virtual void exitUnionMember(MingusParser::UnionMemberContext *ctx) = 0;

  virtual void enterTaggedUnionDeclaration(MingusParser::TaggedUnionDeclarationContext *ctx) = 0;
  virtual void exitTaggedUnionDeclaration(MingusParser::TaggedUnionDeclarationContext *ctx) = 0;

  virtual void enterTaggedUnionVariant(MingusParser::TaggedUnionVariantContext *ctx) = 0;
  virtual void exitTaggedUnionVariant(MingusParser::TaggedUnionVariantContext *ctx) = 0;

  virtual void enterTaggedUnionField(MingusParser::TaggedUnionFieldContext *ctx) = 0;
  virtual void exitTaggedUnionField(MingusParser::TaggedUnionFieldContext *ctx) = 0;

  virtual void enterEnumDeclaration(MingusParser::EnumDeclarationContext *ctx) = 0;
  virtual void exitEnumDeclaration(MingusParser::EnumDeclarationContext *ctx) = 0;

  virtual void enterEnumMember(MingusParser::EnumMemberContext *ctx) = 0;
  virtual void exitEnumMember(MingusParser::EnumMemberContext *ctx) = 0;

  virtual void enterFunctionDeclaration(MingusParser::FunctionDeclarationContext *ctx) = 0;
  virtual void exitFunctionDeclaration(MingusParser::FunctionDeclarationContext *ctx) = 0;

  virtual void enterReturnType(MingusParser::ReturnTypeContext *ctx) = 0;
  virtual void exitReturnType(MingusParser::ReturnTypeContext *ctx) = 0;

  virtual void enterTypeParameterList(MingusParser::TypeParameterListContext *ctx) = 0;
  virtual void exitTypeParameterList(MingusParser::TypeParameterListContext *ctx) = 0;

  virtual void enterTypeArgumentList(MingusParser::TypeArgumentListContext *ctx) = 0;
  virtual void exitTypeArgumentList(MingusParser::TypeArgumentListContext *ctx) = 0;

  virtual void enterDefinitionParameters(MingusParser::DefinitionParametersContext *ctx) = 0;
  virtual void exitDefinitionParameters(MingusParser::DefinitionParametersContext *ctx) = 0;

  virtual void enterParameterList(MingusParser::ParameterListContext *ctx) = 0;
  virtual void exitParameterList(MingusParser::ParameterListContext *ctx) = 0;

  virtual void enterParameter(MingusParser::ParameterContext *ctx) = 0;
  virtual void exitParameter(MingusParser::ParameterContext *ctx) = 0;

  virtual void enterVariableDeclaration(MingusParser::VariableDeclarationContext *ctx) = 0;
  virtual void exitVariableDeclaration(MingusParser::VariableDeclarationContext *ctx) = 0;

  virtual void enterConstVariableDeclaration(MingusParser::ConstVariableDeclarationContext *ctx) = 0;
  virtual void exitConstVariableDeclaration(MingusParser::ConstVariableDeclarationContext *ctx) = 0;

  virtual void enterTypedVariableDeclaration(MingusParser::TypedVariableDeclarationContext *ctx) = 0;
  virtual void exitTypedVariableDeclaration(MingusParser::TypedVariableDeclarationContext *ctx) = 0;

  virtual void enterInferredVariableDeclaration(MingusParser::InferredVariableDeclarationContext *ctx) = 0;
  virtual void exitInferredVariableDeclaration(MingusParser::InferredVariableDeclarationContext *ctx) = 0;

  virtual void enterTupleDestructuring(MingusParser::TupleDestructuringContext *ctx) = 0;
  virtual void exitTupleDestructuring(MingusParser::TupleDestructuringContext *ctx) = 0;

  virtual void enterTupleDestructureElement(MingusParser::TupleDestructureElementContext *ctx) = 0;
  virtual void exitTupleDestructureElement(MingusParser::TupleDestructureElementContext *ctx) = 0;

  virtual void enterStatement(MingusParser::StatementContext *ctx) = 0;
  virtual void exitStatement(MingusParser::StatementContext *ctx) = 0;

  virtual void enterBlock(MingusParser::BlockContext *ctx) = 0;
  virtual void exitBlock(MingusParser::BlockContext *ctx) = 0;

  virtual void enterExprStatement(MingusParser::ExprStatementContext *ctx) = 0;
  virtual void exitExprStatement(MingusParser::ExprStatementContext *ctx) = 0;

  virtual void enterRawBlock(MingusParser::RawBlockContext *ctx) = 0;
  virtual void exitRawBlock(MingusParser::RawBlockContext *ctx) = 0;

  virtual void enterIfStatement(MingusParser::IfStatementContext *ctx) = 0;
  virtual void exitIfStatement(MingusParser::IfStatementContext *ctx) = 0;

  virtual void enterElseIfClause(MingusParser::ElseIfClauseContext *ctx) = 0;
  virtual void exitElseIfClause(MingusParser::ElseIfClauseContext *ctx) = 0;

  virtual void enterElseClause(MingusParser::ElseClauseContext *ctx) = 0;
  virtual void exitElseClause(MingusParser::ElseClauseContext *ctx) = 0;

  virtual void enterSwitchStatement(MingusParser::SwitchStatementContext *ctx) = 0;
  virtual void exitSwitchStatement(MingusParser::SwitchStatementContext *ctx) = 0;

  virtual void enterSwitchCase(MingusParser::SwitchCaseContext *ctx) = 0;
  virtual void exitSwitchCase(MingusParser::SwitchCaseContext *ctx) = 0;

  virtual void enterSwitchDefault(MingusParser::SwitchDefaultContext *ctx) = 0;
  virtual void exitSwitchDefault(MingusParser::SwitchDefaultContext *ctx) = 0;

  virtual void enterMatchStatement(MingusParser::MatchStatementContext *ctx) = 0;
  virtual void exitMatchStatement(MingusParser::MatchStatementContext *ctx) = 0;

  virtual void enterMatchExpression(MingusParser::MatchExpressionContext *ctx) = 0;
  virtual void exitMatchExpression(MingusParser::MatchExpressionContext *ctx) = 0;

  virtual void enterMatchArm(MingusParser::MatchArmContext *ctx) = 0;
  virtual void exitMatchArm(MingusParser::MatchArmContext *ctx) = 0;

  virtual void enterMatchBody(MingusParser::MatchBodyContext *ctx) = 0;
  virtual void exitMatchBody(MingusParser::MatchBodyContext *ctx) = 0;

  virtual void enterPattern(MingusParser::PatternContext *ctx) = 0;
  virtual void exitPattern(MingusParser::PatternContext *ctx) = 0;

  virtual void enterGuardedPattern(MingusParser::GuardedPatternContext *ctx) = 0;
  virtual void exitGuardedPattern(MingusParser::GuardedPatternContext *ctx) = 0;

  virtual void enterBasePattern(MingusParser::BasePatternContext *ctx) = 0;
  virtual void exitBasePattern(MingusParser::BasePatternContext *ctx) = 0;

  virtual void enterLiteralPattern(MingusParser::LiteralPatternContext *ctx) = 0;
  virtual void exitLiteralPattern(MingusParser::LiteralPatternContext *ctx) = 0;

  virtual void enterRangePattern(MingusParser::RangePatternContext *ctx) = 0;
  virtual void exitRangePattern(MingusParser::RangePatternContext *ctx) = 0;

  virtual void enterWildcardPattern(MingusParser::WildcardPatternContext *ctx) = 0;
  virtual void exitWildcardPattern(MingusParser::WildcardPatternContext *ctx) = 0;

  virtual void enterBindingPattern(MingusParser::BindingPatternContext *ctx) = 0;
  virtual void exitBindingPattern(MingusParser::BindingPatternContext *ctx) = 0;

  virtual void enterTuplePattern(MingusParser::TuplePatternContext *ctx) = 0;
  virtual void exitTuplePattern(MingusParser::TuplePatternContext *ctx) = 0;

  virtual void enterVariantPatternField(MingusParser::VariantPatternFieldContext *ctx) = 0;
  virtual void exitVariantPatternField(MingusParser::VariantPatternFieldContext *ctx) = 0;

  virtual void enterForStatement(MingusParser::ForStatementContext *ctx) = 0;
  virtual void exitForStatement(MingusParser::ForStatementContext *ctx) = 0;

  virtual void enterForInitializer(MingusParser::ForInitializerContext *ctx) = 0;
  virtual void exitForInitializer(MingusParser::ForInitializerContext *ctx) = 0;

  virtual void enterLocalVarInitializer(MingusParser::LocalVarInitializerContext *ctx) = 0;
  virtual void exitLocalVarInitializer(MingusParser::LocalVarInitializerContext *ctx) = 0;

  virtual void enterLocalVarDeclaration(MingusParser::LocalVarDeclarationContext *ctx) = 0;
  virtual void exitLocalVarDeclaration(MingusParser::LocalVarDeclarationContext *ctx) = 0;

  virtual void enterForIterator(MingusParser::ForIteratorContext *ctx) = 0;
  virtual void exitForIterator(MingusParser::ForIteratorContext *ctx) = 0;

  virtual void enterWhileStatement(MingusParser::WhileStatementContext *ctx) = 0;
  virtual void exitWhileStatement(MingusParser::WhileStatementContext *ctx) = 0;

  virtual void enterDoWhileStatement(MingusParser::DoWhileStatementContext *ctx) = 0;
  virtual void exitDoWhileStatement(MingusParser::DoWhileStatementContext *ctx) = 0;

  virtual void enterReturnStatement(MingusParser::ReturnStatementContext *ctx) = 0;
  virtual void exitReturnStatement(MingusParser::ReturnStatementContext *ctx) = 0;

  virtual void enterLabeledStatement(MingusParser::LabeledStatementContext *ctx) = 0;
  virtual void exitLabeledStatement(MingusParser::LabeledStatementContext *ctx) = 0;

  virtual void enterBreakStatement(MingusParser::BreakStatementContext *ctx) = 0;
  virtual void exitBreakStatement(MingusParser::BreakStatementContext *ctx) = 0;

  virtual void enterContinueStatement(MingusParser::ContinueStatementContext *ctx) = 0;
  virtual void exitContinueStatement(MingusParser::ContinueStatementContext *ctx) = 0;

  virtual void enterDeleteStatement(MingusParser::DeleteStatementContext *ctx) = 0;
  virtual void exitDeleteStatement(MingusParser::DeleteStatementContext *ctx) = 0;

  virtual void enterExpression(MingusParser::ExpressionContext *ctx) = 0;
  virtual void exitExpression(MingusParser::ExpressionContext *ctx) = 0;

  virtual void enterAssignment(MingusParser::AssignmentContext *ctx) = 0;
  virtual void exitAssignment(MingusParser::AssignmentContext *ctx) = 0;

  virtual void enterAssignmentOperator(MingusParser::AssignmentOperatorContext *ctx) = 0;
  virtual void exitAssignmentOperator(MingusParser::AssignmentOperatorContext *ctx) = 0;

  virtual void enterLambdaExpression(MingusParser::LambdaExpressionContext *ctx) = 0;
  virtual void exitLambdaExpression(MingusParser::LambdaExpressionContext *ctx) = 0;

  virtual void enterCaptureList(MingusParser::CaptureListContext *ctx) = 0;
  virtual void exitCaptureList(MingusParser::CaptureListContext *ctx) = 0;

  virtual void enterCaptureDefault(MingusParser::CaptureDefaultContext *ctx) = 0;
  virtual void exitCaptureDefault(MingusParser::CaptureDefaultContext *ctx) = 0;

  virtual void enterCaptureItem(MingusParser::CaptureItemContext *ctx) = 0;
  virtual void exitCaptureItem(MingusParser::CaptureItemContext *ctx) = 0;

  virtual void enterLambdaParameterList(MingusParser::LambdaParameterListContext *ctx) = 0;
  virtual void exitLambdaParameterList(MingusParser::LambdaParameterListContext *ctx) = 0;

  virtual void enterLambdaParameter(MingusParser::LambdaParameterContext *ctx) = 0;
  virtual void exitLambdaParameter(MingusParser::LambdaParameterContext *ctx) = 0;

  virtual void enterPipe(MingusParser::PipeContext *ctx) = 0;
  virtual void exitPipe(MingusParser::PipeContext *ctx) = 0;

  virtual void enterPipeTarget(MingusParser::PipeTargetContext *ctx) = 0;
  virtual void exitPipeTarget(MingusParser::PipeTargetContext *ctx) = 0;

  virtual void enterTernary(MingusParser::TernaryContext *ctx) = 0;
  virtual void exitTernary(MingusParser::TernaryContext *ctx) = 0;

  virtual void enterLogicOr(MingusParser::LogicOrContext *ctx) = 0;
  virtual void exitLogicOr(MingusParser::LogicOrContext *ctx) = 0;

  virtual void enterLogicAnd(MingusParser::LogicAndContext *ctx) = 0;
  virtual void exitLogicAnd(MingusParser::LogicAndContext *ctx) = 0;

  virtual void enterBitwiseOr(MingusParser::BitwiseOrContext *ctx) = 0;
  virtual void exitBitwiseOr(MingusParser::BitwiseOrContext *ctx) = 0;

  virtual void enterBitwiseXor(MingusParser::BitwiseXorContext *ctx) = 0;
  virtual void exitBitwiseXor(MingusParser::BitwiseXorContext *ctx) = 0;

  virtual void enterBitwiseAnd(MingusParser::BitwiseAndContext *ctx) = 0;
  virtual void exitBitwiseAnd(MingusParser::BitwiseAndContext *ctx) = 0;

  virtual void enterEquality(MingusParser::EqualityContext *ctx) = 0;
  virtual void exitEquality(MingusParser::EqualityContext *ctx) = 0;

  virtual void enterRelational(MingusParser::RelationalContext *ctx) = 0;
  virtual void exitRelational(MingusParser::RelationalContext *ctx) = 0;

  virtual void enterShift(MingusParser::ShiftContext *ctx) = 0;
  virtual void exitShift(MingusParser::ShiftContext *ctx) = 0;

  virtual void enterAdditive(MingusParser::AdditiveContext *ctx) = 0;
  virtual void exitAdditive(MingusParser::AdditiveContext *ctx) = 0;

  virtual void enterMultiplicative(MingusParser::MultiplicativeContext *ctx) = 0;
  virtual void exitMultiplicative(MingusParser::MultiplicativeContext *ctx) = 0;

  virtual void enterCastExpression(MingusParser::CastExpressionContext *ctx) = 0;
  virtual void exitCastExpression(MingusParser::CastExpressionContext *ctx) = 0;

  virtual void enterUnaryExpression(MingusParser::UnaryExpressionContext *ctx) = 0;
  virtual void exitUnaryExpression(MingusParser::UnaryExpressionContext *ctx) = 0;

  virtual void enterPostfixExpression(MingusParser::PostfixExpressionContext *ctx) = 0;
  virtual void exitPostfixExpression(MingusParser::PostfixExpressionContext *ctx) = 0;

  virtual void enterPrimaryExpression(MingusParser::PrimaryExpressionContext *ctx) = 0;
  virtual void exitPrimaryExpression(MingusParser::PrimaryExpressionContext *ctx) = 0;

  virtual void enterArrayLiteral(MingusParser::ArrayLiteralContext *ctx) = 0;
  virtual void exitArrayLiteral(MingusParser::ArrayLiteralContext *ctx) = 0;

  virtual void enterPostfixOperation(MingusParser::PostfixOperationContext *ctx) = 0;
  virtual void exitPostfixOperation(MingusParser::PostfixOperationContext *ctx) = 0;

  virtual void enterNewExpression(MingusParser::NewExpressionContext *ctx) = 0;
  virtual void exitNewExpression(MingusParser::NewExpressionContext *ctx) = 0;

  virtual void enterCallArguments(MingusParser::CallArgumentsContext *ctx) = 0;
  virtual void exitCallArguments(MingusParser::CallArgumentsContext *ctx) = 0;

  virtual void enterArgumentList(MingusParser::ArgumentListContext *ctx) = 0;
  virtual void exitArgumentList(MingusParser::ArgumentListContext *ctx) = 0;

  virtual void enterElementAccess(MingusParser::ElementAccessContext *ctx) = 0;
  virtual void exitElementAccess(MingusParser::ElementAccessContext *ctx) = 0;

  virtual void enterMemberAccess(MingusParser::MemberAccessContext *ctx) = 0;
  virtual void exitMemberAccess(MingusParser::MemberAccessContext *ctx) = 0;

  virtual void enterTupleExpression(MingusParser::TupleExpressionContext *ctx) = 0;
  virtual void exitTupleExpression(MingusParser::TupleExpressionContext *ctx) = 0;

  virtual void enterTypeIdentifier(MingusParser::TypeIdentifierContext *ctx) = 0;
  virtual void exitTypeIdentifier(MingusParser::TypeIdentifierContext *ctx) = 0;

  virtual void enterPrimitiveType(MingusParser::PrimitiveTypeContext *ctx) = 0;
  virtual void exitPrimitiveType(MingusParser::PrimitiveTypeContext *ctx) = 0;

  virtual void enterFunctionType(MingusParser::FunctionTypeContext *ctx) = 0;
  virtual void exitFunctionType(MingusParser::FunctionTypeContext *ctx) = 0;

  virtual void enterTypeList(MingusParser::TypeListContext *ctx) = 0;
  virtual void exitTypeList(MingusParser::TypeListContext *ctx) = 0;

  virtual void enterTupleType(MingusParser::TupleTypeContext *ctx) = 0;
  virtual void exitTupleType(MingusParser::TupleTypeContext *ctx) = 0;

  virtual void enterTypeModifier(MingusParser::TypeModifierContext *ctx) = 0;
  virtual void exitTypeModifier(MingusParser::TypeModifierContext *ctx) = 0;

  virtual void enterRvalueReferenceLevel(MingusParser::RvalueReferenceLevelContext *ctx) = 0;
  virtual void exitRvalueReferenceLevel(MingusParser::RvalueReferenceLevelContext *ctx) = 0;

  virtual void enterReferenceLevel(MingusParser::ReferenceLevelContext *ctx) = 0;
  virtual void exitReferenceLevel(MingusParser::ReferenceLevelContext *ctx) = 0;

  virtual void enterArrayDimension(MingusParser::ArrayDimensionContext *ctx) = 0;
  virtual void exitArrayDimension(MingusParser::ArrayDimensionContext *ctx) = 0;

  virtual void enterPointerLevel(MingusParser::PointerLevelContext *ctx) = 0;
  virtual void exitPointerLevel(MingusParser::PointerLevelContext *ctx) = 0;

  virtual void enterAccessModifier(MingusParser::AccessModifierContext *ctx) = 0;
  virtual void exitAccessModifier(MingusParser::AccessModifierContext *ctx) = 0;

  virtual void enterStaticModifier(MingusParser::StaticModifierContext *ctx) = 0;
  virtual void exitStaticModifier(MingusParser::StaticModifierContext *ctx) = 0;

  virtual void enterAbstractModifier(MingusParser::AbstractModifierContext *ctx) = 0;
  virtual void exitAbstractModifier(MingusParser::AbstractModifierContext *ctx) = 0;

  virtual void enterQualifiedName(MingusParser::QualifiedNameContext *ctx) = 0;
  virtual void exitQualifiedName(MingusParser::QualifiedNameContext *ctx) = 0;

  virtual void enterPrefixOperator(MingusParser::PrefixOperatorContext *ctx) = 0;
  virtual void exitPrefixOperator(MingusParser::PrefixOperatorContext *ctx) = 0;

  virtual void enterIncrementDecrementOperator(MingusParser::IncrementDecrementOperatorContext *ctx) = 0;
  virtual void exitIncrementDecrementOperator(MingusParser::IncrementDecrementOperatorContext *ctx) = 0;

  virtual void enterTypeSizeOrAlign(MingusParser::TypeSizeOrAlignContext *ctx) = 0;
  virtual void exitTypeSizeOrAlign(MingusParser::TypeSizeOrAlignContext *ctx) = 0;

  virtual void enterString(MingusParser::StringContext *ctx) = 0;
  virtual void exitString(MingusParser::StringContext *ctx) = 0;

  virtual void enterStringPart(MingusParser::StringPartContext *ctx) = 0;
  virtual void exitStringPart(MingusParser::StringPartContext *ctx) = 0;


};

}  // namespace AntlrMingusParser
