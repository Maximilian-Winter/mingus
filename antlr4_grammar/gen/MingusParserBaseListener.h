
// Generated from antlr4_grammar/MingusParser.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "MingusParserListener.h"


namespace AntlrMingusParser {

/**
 * This class provides an empty implementation of MingusParserListener,
 * which can be extended to create a listener which only needs to handle a subset
 * of the available methods.
 */
class  MingusParserBaseListener : public MingusParserListener {
public:

  virtual void enterProgram(MingusParser::ProgramContext * /*ctx*/) override { }
  virtual void exitProgram(MingusParser::ProgramContext * /*ctx*/) override { }

  virtual void enterModule(MingusParser::ModuleContext * /*ctx*/) override { }
  virtual void exitModule(MingusParser::ModuleContext * /*ctx*/) override { }

  virtual void enterModuleBlock(MingusParser::ModuleBlockContext * /*ctx*/) override { }
  virtual void exitModuleBlock(MingusParser::ModuleBlockContext * /*ctx*/) override { }

  virtual void enterModuleDeclaration(MingusParser::ModuleDeclarationContext * /*ctx*/) override { }
  virtual void exitModuleDeclaration(MingusParser::ModuleDeclarationContext * /*ctx*/) override { }

  virtual void enterTypedefDeclaration(MingusParser::TypedefDeclarationContext * /*ctx*/) override { }
  virtual void exitTypedefDeclaration(MingusParser::TypedefDeclarationContext * /*ctx*/) override { }

  virtual void enterImportDefinition(MingusParser::ImportDefinitionContext * /*ctx*/) override { }
  virtual void exitImportDefinition(MingusParser::ImportDefinitionContext * /*ctx*/) override { }

  virtual void enterImportTarget(MingusParser::ImportTargetContext * /*ctx*/) override { }
  virtual void exitImportTarget(MingusParser::ImportTargetContext * /*ctx*/) override { }

  virtual void enterExternDeclaration(MingusParser::ExternDeclarationContext * /*ctx*/) override { }
  virtual void exitExternDeclaration(MingusParser::ExternDeclarationContext * /*ctx*/) override { }

  virtual void enterExternBody(MingusParser::ExternBodyContext * /*ctx*/) override { }
  virtual void exitExternBody(MingusParser::ExternBodyContext * /*ctx*/) override { }

  virtual void enterExternMember(MingusParser::ExternMemberContext * /*ctx*/) override { }
  virtual void exitExternMember(MingusParser::ExternMemberContext * /*ctx*/) override { }

  virtual void enterExternFunctionDeclaration(MingusParser::ExternFunctionDeclarationContext * /*ctx*/) override { }
  virtual void exitExternFunctionDeclaration(MingusParser::ExternFunctionDeclarationContext * /*ctx*/) override { }

  virtual void enterExternLinkDirective(MingusParser::ExternLinkDirectiveContext * /*ctx*/) override { }
  virtual void exitExternLinkDirective(MingusParser::ExternLinkDirectiveContext * /*ctx*/) override { }

  virtual void enterExternOpaqueTypeDeclaration(MingusParser::ExternOpaqueTypeDeclarationContext * /*ctx*/) override { }
  virtual void exitExternOpaqueTypeDeclaration(MingusParser::ExternOpaqueTypeDeclarationContext * /*ctx*/) override { }

  virtual void enterExternStructDeclaration(MingusParser::ExternStructDeclarationContext * /*ctx*/) override { }
  virtual void exitExternStructDeclaration(MingusParser::ExternStructDeclarationContext * /*ctx*/) override { }

  virtual void enterExternUnionDeclaration(MingusParser::ExternUnionDeclarationContext * /*ctx*/) override { }
  virtual void exitExternUnionDeclaration(MingusParser::ExternUnionDeclarationContext * /*ctx*/) override { }

  virtual void enterExternFieldDeclaration(MingusParser::ExternFieldDeclarationContext * /*ctx*/) override { }
  virtual void exitExternFieldDeclaration(MingusParser::ExternFieldDeclarationContext * /*ctx*/) override { }

  virtual void enterExternEnumDeclaration(MingusParser::ExternEnumDeclarationContext * /*ctx*/) override { }
  virtual void exitExternEnumDeclaration(MingusParser::ExternEnumDeclarationContext * /*ctx*/) override { }

  virtual void enterExternVariableDeclaration(MingusParser::ExternVariableDeclarationContext * /*ctx*/) override { }
  virtual void exitExternVariableDeclaration(MingusParser::ExternVariableDeclarationContext * /*ctx*/) override { }

  virtual void enterClassDeclaration(MingusParser::ClassDeclarationContext * /*ctx*/) override { }
  virtual void exitClassDeclaration(MingusParser::ClassDeclarationContext * /*ctx*/) override { }

  virtual void enterClassBlock(MingusParser::ClassBlockContext * /*ctx*/) override { }
  virtual void exitClassBlock(MingusParser::ClassBlockContext * /*ctx*/) override { }

  virtual void enterClassMember(MingusParser::ClassMemberContext * /*ctx*/) override { }
  virtual void exitClassMember(MingusParser::ClassMemberContext * /*ctx*/) override { }

  virtual void enterConstructorDeclaration(MingusParser::ConstructorDeclarationContext * /*ctx*/) override { }
  virtual void exitConstructorDeclaration(MingusParser::ConstructorDeclarationContext * /*ctx*/) override { }

  virtual void enterDestructorDeclaration(MingusParser::DestructorDeclarationContext * /*ctx*/) override { }
  virtual void exitDestructorDeclaration(MingusParser::DestructorDeclarationContext * /*ctx*/) override { }

  virtual void enterOperatorDeclaration(MingusParser::OperatorDeclarationContext * /*ctx*/) override { }
  virtual void exitOperatorDeclaration(MingusParser::OperatorDeclarationContext * /*ctx*/) override { }

  virtual void enterOverloadableOperator(MingusParser::OverloadableOperatorContext * /*ctx*/) override { }
  virtual void exitOverloadableOperator(MingusParser::OverloadableOperatorContext * /*ctx*/) override { }

  virtual void enterInheritance(MingusParser::InheritanceContext * /*ctx*/) override { }
  virtual void exitInheritance(MingusParser::InheritanceContext * /*ctx*/) override { }

  virtual void enterInterfaceDeclaration(MingusParser::InterfaceDeclarationContext * /*ctx*/) override { }
  virtual void exitInterfaceDeclaration(MingusParser::InterfaceDeclarationContext * /*ctx*/) override { }

  virtual void enterInterfaceBlock(MingusParser::InterfaceBlockContext * /*ctx*/) override { }
  virtual void exitInterfaceBlock(MingusParser::InterfaceBlockContext * /*ctx*/) override { }

  virtual void enterInterfaceMember(MingusParser::InterfaceMemberContext * /*ctx*/) override { }
  virtual void exitInterfaceMember(MingusParser::InterfaceMemberContext * /*ctx*/) override { }

  virtual void enterAttribute(MingusParser::AttributeContext * /*ctx*/) override { }
  virtual void exitAttribute(MingusParser::AttributeContext * /*ctx*/) override { }

  virtual void enterStructDeclaration(MingusParser::StructDeclarationContext * /*ctx*/) override { }
  virtual void exitStructDeclaration(MingusParser::StructDeclarationContext * /*ctx*/) override { }

  virtual void enterStructBlock(MingusParser::StructBlockContext * /*ctx*/) override { }
  virtual void exitStructBlock(MingusParser::StructBlockContext * /*ctx*/) override { }

  virtual void enterStructMember(MingusParser::StructMemberContext * /*ctx*/) override { }
  virtual void exitStructMember(MingusParser::StructMemberContext * /*ctx*/) override { }

  virtual void enterUnionDeclaration(MingusParser::UnionDeclarationContext * /*ctx*/) override { }
  virtual void exitUnionDeclaration(MingusParser::UnionDeclarationContext * /*ctx*/) override { }

  virtual void enterUnionBlock(MingusParser::UnionBlockContext * /*ctx*/) override { }
  virtual void exitUnionBlock(MingusParser::UnionBlockContext * /*ctx*/) override { }

  virtual void enterUnionMember(MingusParser::UnionMemberContext * /*ctx*/) override { }
  virtual void exitUnionMember(MingusParser::UnionMemberContext * /*ctx*/) override { }

  virtual void enterTaggedUnionDeclaration(MingusParser::TaggedUnionDeclarationContext * /*ctx*/) override { }
  virtual void exitTaggedUnionDeclaration(MingusParser::TaggedUnionDeclarationContext * /*ctx*/) override { }

  virtual void enterTaggedUnionVariant(MingusParser::TaggedUnionVariantContext * /*ctx*/) override { }
  virtual void exitTaggedUnionVariant(MingusParser::TaggedUnionVariantContext * /*ctx*/) override { }

  virtual void enterTaggedUnionField(MingusParser::TaggedUnionFieldContext * /*ctx*/) override { }
  virtual void exitTaggedUnionField(MingusParser::TaggedUnionFieldContext * /*ctx*/) override { }

  virtual void enterEnumDeclaration(MingusParser::EnumDeclarationContext * /*ctx*/) override { }
  virtual void exitEnumDeclaration(MingusParser::EnumDeclarationContext * /*ctx*/) override { }

  virtual void enterEnumMember(MingusParser::EnumMemberContext * /*ctx*/) override { }
  virtual void exitEnumMember(MingusParser::EnumMemberContext * /*ctx*/) override { }

  virtual void enterFunctionDeclaration(MingusParser::FunctionDeclarationContext * /*ctx*/) override { }
  virtual void exitFunctionDeclaration(MingusParser::FunctionDeclarationContext * /*ctx*/) override { }

  virtual void enterReturnType(MingusParser::ReturnTypeContext * /*ctx*/) override { }
  virtual void exitReturnType(MingusParser::ReturnTypeContext * /*ctx*/) override { }

  virtual void enterDefinitionParameters(MingusParser::DefinitionParametersContext * /*ctx*/) override { }
  virtual void exitDefinitionParameters(MingusParser::DefinitionParametersContext * /*ctx*/) override { }

  virtual void enterParameterList(MingusParser::ParameterListContext * /*ctx*/) override { }
  virtual void exitParameterList(MingusParser::ParameterListContext * /*ctx*/) override { }

  virtual void enterParameter(MingusParser::ParameterContext * /*ctx*/) override { }
  virtual void exitParameter(MingusParser::ParameterContext * /*ctx*/) override { }

  virtual void enterVariableDeclaration(MingusParser::VariableDeclarationContext * /*ctx*/) override { }
  virtual void exitVariableDeclaration(MingusParser::VariableDeclarationContext * /*ctx*/) override { }

  virtual void enterConstVariableDeclaration(MingusParser::ConstVariableDeclarationContext * /*ctx*/) override { }
  virtual void exitConstVariableDeclaration(MingusParser::ConstVariableDeclarationContext * /*ctx*/) override { }

  virtual void enterTypedVariableDeclaration(MingusParser::TypedVariableDeclarationContext * /*ctx*/) override { }
  virtual void exitTypedVariableDeclaration(MingusParser::TypedVariableDeclarationContext * /*ctx*/) override { }

  virtual void enterInferredVariableDeclaration(MingusParser::InferredVariableDeclarationContext * /*ctx*/) override { }
  virtual void exitInferredVariableDeclaration(MingusParser::InferredVariableDeclarationContext * /*ctx*/) override { }

  virtual void enterTupleDestructuring(MingusParser::TupleDestructuringContext * /*ctx*/) override { }
  virtual void exitTupleDestructuring(MingusParser::TupleDestructuringContext * /*ctx*/) override { }

  virtual void enterTupleDestructureElement(MingusParser::TupleDestructureElementContext * /*ctx*/) override { }
  virtual void exitTupleDestructureElement(MingusParser::TupleDestructureElementContext * /*ctx*/) override { }

  virtual void enterStatement(MingusParser::StatementContext * /*ctx*/) override { }
  virtual void exitStatement(MingusParser::StatementContext * /*ctx*/) override { }

  virtual void enterBlock(MingusParser::BlockContext * /*ctx*/) override { }
  virtual void exitBlock(MingusParser::BlockContext * /*ctx*/) override { }

  virtual void enterExprStatement(MingusParser::ExprStatementContext * /*ctx*/) override { }
  virtual void exitExprStatement(MingusParser::ExprStatementContext * /*ctx*/) override { }

  virtual void enterRawBlock(MingusParser::RawBlockContext * /*ctx*/) override { }
  virtual void exitRawBlock(MingusParser::RawBlockContext * /*ctx*/) override { }

  virtual void enterIfStatement(MingusParser::IfStatementContext * /*ctx*/) override { }
  virtual void exitIfStatement(MingusParser::IfStatementContext * /*ctx*/) override { }

  virtual void enterElseIfClause(MingusParser::ElseIfClauseContext * /*ctx*/) override { }
  virtual void exitElseIfClause(MingusParser::ElseIfClauseContext * /*ctx*/) override { }

  virtual void enterElseClause(MingusParser::ElseClauseContext * /*ctx*/) override { }
  virtual void exitElseClause(MingusParser::ElseClauseContext * /*ctx*/) override { }

  virtual void enterSwitchStatement(MingusParser::SwitchStatementContext * /*ctx*/) override { }
  virtual void exitSwitchStatement(MingusParser::SwitchStatementContext * /*ctx*/) override { }

  virtual void enterSwitchCase(MingusParser::SwitchCaseContext * /*ctx*/) override { }
  virtual void exitSwitchCase(MingusParser::SwitchCaseContext * /*ctx*/) override { }

  virtual void enterSwitchDefault(MingusParser::SwitchDefaultContext * /*ctx*/) override { }
  virtual void exitSwitchDefault(MingusParser::SwitchDefaultContext * /*ctx*/) override { }

  virtual void enterMatchStatement(MingusParser::MatchStatementContext * /*ctx*/) override { }
  virtual void exitMatchStatement(MingusParser::MatchStatementContext * /*ctx*/) override { }

  virtual void enterMatchExpression(MingusParser::MatchExpressionContext * /*ctx*/) override { }
  virtual void exitMatchExpression(MingusParser::MatchExpressionContext * /*ctx*/) override { }

  virtual void enterMatchArm(MingusParser::MatchArmContext * /*ctx*/) override { }
  virtual void exitMatchArm(MingusParser::MatchArmContext * /*ctx*/) override { }

  virtual void enterMatchBody(MingusParser::MatchBodyContext * /*ctx*/) override { }
  virtual void exitMatchBody(MingusParser::MatchBodyContext * /*ctx*/) override { }

  virtual void enterPattern(MingusParser::PatternContext * /*ctx*/) override { }
  virtual void exitPattern(MingusParser::PatternContext * /*ctx*/) override { }

  virtual void enterGuardedPattern(MingusParser::GuardedPatternContext * /*ctx*/) override { }
  virtual void exitGuardedPattern(MingusParser::GuardedPatternContext * /*ctx*/) override { }

  virtual void enterBasePattern(MingusParser::BasePatternContext * /*ctx*/) override { }
  virtual void exitBasePattern(MingusParser::BasePatternContext * /*ctx*/) override { }

  virtual void enterLiteralPattern(MingusParser::LiteralPatternContext * /*ctx*/) override { }
  virtual void exitLiteralPattern(MingusParser::LiteralPatternContext * /*ctx*/) override { }

  virtual void enterRangePattern(MingusParser::RangePatternContext * /*ctx*/) override { }
  virtual void exitRangePattern(MingusParser::RangePatternContext * /*ctx*/) override { }

  virtual void enterWildcardPattern(MingusParser::WildcardPatternContext * /*ctx*/) override { }
  virtual void exitWildcardPattern(MingusParser::WildcardPatternContext * /*ctx*/) override { }

  virtual void enterBindingPattern(MingusParser::BindingPatternContext * /*ctx*/) override { }
  virtual void exitBindingPattern(MingusParser::BindingPatternContext * /*ctx*/) override { }

  virtual void enterTuplePattern(MingusParser::TuplePatternContext * /*ctx*/) override { }
  virtual void exitTuplePattern(MingusParser::TuplePatternContext * /*ctx*/) override { }

  virtual void enterVariantPatternField(MingusParser::VariantPatternFieldContext * /*ctx*/) override { }
  virtual void exitVariantPatternField(MingusParser::VariantPatternFieldContext * /*ctx*/) override { }

  virtual void enterForStatement(MingusParser::ForStatementContext * /*ctx*/) override { }
  virtual void exitForStatement(MingusParser::ForStatementContext * /*ctx*/) override { }

  virtual void enterForInitializer(MingusParser::ForInitializerContext * /*ctx*/) override { }
  virtual void exitForInitializer(MingusParser::ForInitializerContext * /*ctx*/) override { }

  virtual void enterLocalVarInitializer(MingusParser::LocalVarInitializerContext * /*ctx*/) override { }
  virtual void exitLocalVarInitializer(MingusParser::LocalVarInitializerContext * /*ctx*/) override { }

  virtual void enterLocalVarDeclaration(MingusParser::LocalVarDeclarationContext * /*ctx*/) override { }
  virtual void exitLocalVarDeclaration(MingusParser::LocalVarDeclarationContext * /*ctx*/) override { }

  virtual void enterForIterator(MingusParser::ForIteratorContext * /*ctx*/) override { }
  virtual void exitForIterator(MingusParser::ForIteratorContext * /*ctx*/) override { }

  virtual void enterWhileStatement(MingusParser::WhileStatementContext * /*ctx*/) override { }
  virtual void exitWhileStatement(MingusParser::WhileStatementContext * /*ctx*/) override { }

  virtual void enterDoWhileStatement(MingusParser::DoWhileStatementContext * /*ctx*/) override { }
  virtual void exitDoWhileStatement(MingusParser::DoWhileStatementContext * /*ctx*/) override { }

  virtual void enterReturnStatement(MingusParser::ReturnStatementContext * /*ctx*/) override { }
  virtual void exitReturnStatement(MingusParser::ReturnStatementContext * /*ctx*/) override { }

  virtual void enterLabeledStatement(MingusParser::LabeledStatementContext * /*ctx*/) override { }
  virtual void exitLabeledStatement(MingusParser::LabeledStatementContext * /*ctx*/) override { }

  virtual void enterBreakStatement(MingusParser::BreakStatementContext * /*ctx*/) override { }
  virtual void exitBreakStatement(MingusParser::BreakStatementContext * /*ctx*/) override { }

  virtual void enterContinueStatement(MingusParser::ContinueStatementContext * /*ctx*/) override { }
  virtual void exitContinueStatement(MingusParser::ContinueStatementContext * /*ctx*/) override { }

  virtual void enterDeleteStatement(MingusParser::DeleteStatementContext * /*ctx*/) override { }
  virtual void exitDeleteStatement(MingusParser::DeleteStatementContext * /*ctx*/) override { }

  virtual void enterExpression(MingusParser::ExpressionContext * /*ctx*/) override { }
  virtual void exitExpression(MingusParser::ExpressionContext * /*ctx*/) override { }

  virtual void enterAssignment(MingusParser::AssignmentContext * /*ctx*/) override { }
  virtual void exitAssignment(MingusParser::AssignmentContext * /*ctx*/) override { }

  virtual void enterAssignmentOperator(MingusParser::AssignmentOperatorContext * /*ctx*/) override { }
  virtual void exitAssignmentOperator(MingusParser::AssignmentOperatorContext * /*ctx*/) override { }

  virtual void enterLambdaExpression(MingusParser::LambdaExpressionContext * /*ctx*/) override { }
  virtual void exitLambdaExpression(MingusParser::LambdaExpressionContext * /*ctx*/) override { }

  virtual void enterCaptureList(MingusParser::CaptureListContext * /*ctx*/) override { }
  virtual void exitCaptureList(MingusParser::CaptureListContext * /*ctx*/) override { }

  virtual void enterCaptureDefault(MingusParser::CaptureDefaultContext * /*ctx*/) override { }
  virtual void exitCaptureDefault(MingusParser::CaptureDefaultContext * /*ctx*/) override { }

  virtual void enterCaptureItem(MingusParser::CaptureItemContext * /*ctx*/) override { }
  virtual void exitCaptureItem(MingusParser::CaptureItemContext * /*ctx*/) override { }

  virtual void enterLambdaParameterList(MingusParser::LambdaParameterListContext * /*ctx*/) override { }
  virtual void exitLambdaParameterList(MingusParser::LambdaParameterListContext * /*ctx*/) override { }

  virtual void enterLambdaParameter(MingusParser::LambdaParameterContext * /*ctx*/) override { }
  virtual void exitLambdaParameter(MingusParser::LambdaParameterContext * /*ctx*/) override { }

  virtual void enterPipe(MingusParser::PipeContext * /*ctx*/) override { }
  virtual void exitPipe(MingusParser::PipeContext * /*ctx*/) override { }

  virtual void enterPipeTarget(MingusParser::PipeTargetContext * /*ctx*/) override { }
  virtual void exitPipeTarget(MingusParser::PipeTargetContext * /*ctx*/) override { }

  virtual void enterTernary(MingusParser::TernaryContext * /*ctx*/) override { }
  virtual void exitTernary(MingusParser::TernaryContext * /*ctx*/) override { }

  virtual void enterLogicOr(MingusParser::LogicOrContext * /*ctx*/) override { }
  virtual void exitLogicOr(MingusParser::LogicOrContext * /*ctx*/) override { }

  virtual void enterLogicAnd(MingusParser::LogicAndContext * /*ctx*/) override { }
  virtual void exitLogicAnd(MingusParser::LogicAndContext * /*ctx*/) override { }

  virtual void enterBitwiseOr(MingusParser::BitwiseOrContext * /*ctx*/) override { }
  virtual void exitBitwiseOr(MingusParser::BitwiseOrContext * /*ctx*/) override { }

  virtual void enterBitwiseXor(MingusParser::BitwiseXorContext * /*ctx*/) override { }
  virtual void exitBitwiseXor(MingusParser::BitwiseXorContext * /*ctx*/) override { }

  virtual void enterBitwiseAnd(MingusParser::BitwiseAndContext * /*ctx*/) override { }
  virtual void exitBitwiseAnd(MingusParser::BitwiseAndContext * /*ctx*/) override { }

  virtual void enterEquality(MingusParser::EqualityContext * /*ctx*/) override { }
  virtual void exitEquality(MingusParser::EqualityContext * /*ctx*/) override { }

  virtual void enterRelational(MingusParser::RelationalContext * /*ctx*/) override { }
  virtual void exitRelational(MingusParser::RelationalContext * /*ctx*/) override { }

  virtual void enterShift(MingusParser::ShiftContext * /*ctx*/) override { }
  virtual void exitShift(MingusParser::ShiftContext * /*ctx*/) override { }

  virtual void enterAdditive(MingusParser::AdditiveContext * /*ctx*/) override { }
  virtual void exitAdditive(MingusParser::AdditiveContext * /*ctx*/) override { }

  virtual void enterMultiplicative(MingusParser::MultiplicativeContext * /*ctx*/) override { }
  virtual void exitMultiplicative(MingusParser::MultiplicativeContext * /*ctx*/) override { }

  virtual void enterCastExpression(MingusParser::CastExpressionContext * /*ctx*/) override { }
  virtual void exitCastExpression(MingusParser::CastExpressionContext * /*ctx*/) override { }

  virtual void enterUnaryExpression(MingusParser::UnaryExpressionContext * /*ctx*/) override { }
  virtual void exitUnaryExpression(MingusParser::UnaryExpressionContext * /*ctx*/) override { }

  virtual void enterPostfixExpression(MingusParser::PostfixExpressionContext * /*ctx*/) override { }
  virtual void exitPostfixExpression(MingusParser::PostfixExpressionContext * /*ctx*/) override { }

  virtual void enterPrimaryExpression(MingusParser::PrimaryExpressionContext * /*ctx*/) override { }
  virtual void exitPrimaryExpression(MingusParser::PrimaryExpressionContext * /*ctx*/) override { }

  virtual void enterArrayLiteral(MingusParser::ArrayLiteralContext * /*ctx*/) override { }
  virtual void exitArrayLiteral(MingusParser::ArrayLiteralContext * /*ctx*/) override { }

  virtual void enterPostfixOperation(MingusParser::PostfixOperationContext * /*ctx*/) override { }
  virtual void exitPostfixOperation(MingusParser::PostfixOperationContext * /*ctx*/) override { }

  virtual void enterNewExpression(MingusParser::NewExpressionContext * /*ctx*/) override { }
  virtual void exitNewExpression(MingusParser::NewExpressionContext * /*ctx*/) override { }

  virtual void enterCallArguments(MingusParser::CallArgumentsContext * /*ctx*/) override { }
  virtual void exitCallArguments(MingusParser::CallArgumentsContext * /*ctx*/) override { }

  virtual void enterArgumentList(MingusParser::ArgumentListContext * /*ctx*/) override { }
  virtual void exitArgumentList(MingusParser::ArgumentListContext * /*ctx*/) override { }

  virtual void enterElementAccess(MingusParser::ElementAccessContext * /*ctx*/) override { }
  virtual void exitElementAccess(MingusParser::ElementAccessContext * /*ctx*/) override { }

  virtual void enterMemberAccess(MingusParser::MemberAccessContext * /*ctx*/) override { }
  virtual void exitMemberAccess(MingusParser::MemberAccessContext * /*ctx*/) override { }

  virtual void enterTupleExpression(MingusParser::TupleExpressionContext * /*ctx*/) override { }
  virtual void exitTupleExpression(MingusParser::TupleExpressionContext * /*ctx*/) override { }

  virtual void enterTypeIdentifier(MingusParser::TypeIdentifierContext * /*ctx*/) override { }
  virtual void exitTypeIdentifier(MingusParser::TypeIdentifierContext * /*ctx*/) override { }

  virtual void enterPrimitiveType(MingusParser::PrimitiveTypeContext * /*ctx*/) override { }
  virtual void exitPrimitiveType(MingusParser::PrimitiveTypeContext * /*ctx*/) override { }

  virtual void enterFunctionType(MingusParser::FunctionTypeContext * /*ctx*/) override { }
  virtual void exitFunctionType(MingusParser::FunctionTypeContext * /*ctx*/) override { }

  virtual void enterTypeList(MingusParser::TypeListContext * /*ctx*/) override { }
  virtual void exitTypeList(MingusParser::TypeListContext * /*ctx*/) override { }

  virtual void enterTupleType(MingusParser::TupleTypeContext * /*ctx*/) override { }
  virtual void exitTupleType(MingusParser::TupleTypeContext * /*ctx*/) override { }

  virtual void enterTypeModifier(MingusParser::TypeModifierContext * /*ctx*/) override { }
  virtual void exitTypeModifier(MingusParser::TypeModifierContext * /*ctx*/) override { }

  virtual void enterRvalueReferenceLevel(MingusParser::RvalueReferenceLevelContext * /*ctx*/) override { }
  virtual void exitRvalueReferenceLevel(MingusParser::RvalueReferenceLevelContext * /*ctx*/) override { }

  virtual void enterReferenceLevel(MingusParser::ReferenceLevelContext * /*ctx*/) override { }
  virtual void exitReferenceLevel(MingusParser::ReferenceLevelContext * /*ctx*/) override { }

  virtual void enterArrayDimension(MingusParser::ArrayDimensionContext * /*ctx*/) override { }
  virtual void exitArrayDimension(MingusParser::ArrayDimensionContext * /*ctx*/) override { }

  virtual void enterPointerLevel(MingusParser::PointerLevelContext * /*ctx*/) override { }
  virtual void exitPointerLevel(MingusParser::PointerLevelContext * /*ctx*/) override { }

  virtual void enterAccessModifier(MingusParser::AccessModifierContext * /*ctx*/) override { }
  virtual void exitAccessModifier(MingusParser::AccessModifierContext * /*ctx*/) override { }

  virtual void enterStaticModifier(MingusParser::StaticModifierContext * /*ctx*/) override { }
  virtual void exitStaticModifier(MingusParser::StaticModifierContext * /*ctx*/) override { }

  virtual void enterAbstractModifier(MingusParser::AbstractModifierContext * /*ctx*/) override { }
  virtual void exitAbstractModifier(MingusParser::AbstractModifierContext * /*ctx*/) override { }

  virtual void enterQualifiedName(MingusParser::QualifiedNameContext * /*ctx*/) override { }
  virtual void exitQualifiedName(MingusParser::QualifiedNameContext * /*ctx*/) override { }

  virtual void enterPrefixOperator(MingusParser::PrefixOperatorContext * /*ctx*/) override { }
  virtual void exitPrefixOperator(MingusParser::PrefixOperatorContext * /*ctx*/) override { }

  virtual void enterIncrementDecrementOperator(MingusParser::IncrementDecrementOperatorContext * /*ctx*/) override { }
  virtual void exitIncrementDecrementOperator(MingusParser::IncrementDecrementOperatorContext * /*ctx*/) override { }

  virtual void enterTypeSizeOrAlign(MingusParser::TypeSizeOrAlignContext * /*ctx*/) override { }
  virtual void exitTypeSizeOrAlign(MingusParser::TypeSizeOrAlignContext * /*ctx*/) override { }

  virtual void enterString(MingusParser::StringContext * /*ctx*/) override { }
  virtual void exitString(MingusParser::StringContext * /*ctx*/) override { }

  virtual void enterStringPart(MingusParser::StringPartContext * /*ctx*/) override { }
  virtual void exitStringPart(MingusParser::StringPartContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

}  // namespace AntlrMingusParser
