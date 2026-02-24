
// Generated from antlr4_grammar/MingusParser.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"


namespace AntlrMingusParser {


class  MingusParser : public antlr4::Parser {
public:
  enum {
    DeclareModule = 1, DeclareClass = 2, DeclareStruct = 3, DeclareEnum = 4, 
    DeclareFunction = 5, DeclareConstructor = 6, DeclareDestructor = 7, 
    SuperKeyword = 8, DeclareOperator = 9, DeclareForLoop = 10, DeclareWhileLoop = 11, 
    DeclareDoLoop = 12, DeclareStatic = 13, DeclareAbstract = 14, DeclareInterface = 15, 
    DeclareTypedef = 16, DeclarePublic = 17, DeclarePrivate = 18, DeclareProtected = 19, 
    ExternKeyword = 20, RawKeyword = 21, LinkKeyword = 22, OpaqueKeyword = 23, 
    ControlFlowIf = 24, ControlFlowElse = 25, ControlFlowSwitch = 26, ControlFlowCase = 27, 
    ControlFlowDefault = 28, ControlFlowMatch = 29, FunctionReturn = 30, 
    Break = 31, Continue = 32, DeclareVariable = 33, DeclareConst = 34, 
    NewKeyword = 35, DeleteKeyword = 36, NullReference = 37, ThisReference = 38, 
    MoveKeyword = 39, WeakKeyword = 40, SharedKeyword = 41, ImportDirective = 42, 
    FromDirective = 43, AsKeyword = 44, SizeOfKeyword = 45, AlignOfKeyword = 46, 
    IntegerType = 47, DoubleType = 48, FloatType = 49, ByteType = 50, StringType = 51, 
    CharType = 52, BoolType = 53, VoidType = 54, ShortType = 55, UShortType = 56, 
    UIntType = 57, LongType = 58, ULongType = 59, BooleanLiteral = 60, AssignOperator = 61, 
    PlusAssignOperator = 62, MinusAssignOperator = 63, MultiplyAssignOperator = 64, 
    DivideAssignOperator = 65, ModuloAssignOperator = 66, BitwiseAndAssignOperator = 67, 
    BitwiseOrAssignOperator = 68, BitwiseXorAssignOperator = 69, BitwiseLeftShiftAssignOperator = 70, 
    BitwiseRightShiftAssignOperator = 71, LogicalOrOperator = 72, LogicalAndOperator = 73, 
    UnequalOperator = 74, EqualOperator = 75, GreaterEqualOperator = 76, 
    SmallerEqualOperator = 77, GreaterOperator = 78, SmallerOperator = 79, 
    ShiftLeftOperator = 80, ShiftRightOperator = 81, PlusPlusOperator = 82, 
    MinusMinusOperator = 83, PlusOperator = 84, MinusOperator = 85, StarOperator = 86, 
    DivideOperator = 87, ModuloOperator = 88, LogicalNegationOperator = 89, 
    ComplimentOperator = 90, SingleAndOperator = 91, BitwiseXorOperator = 92, 
    BitwiseOrOperator = 93, PipeOperator = 94, ArrowOperator = 95, ReferenceAccessOperator = 96, 
    DotOperator = 97, Ellipsis = 98, DotDotOperator = 99, QuestionMarkOperator = 100, 
    ColonOperator = 101, SemicolonSeparator = 102, CommaSeparator = 103, 
    UnderscoreWildcard = 104, OpeningRoundBracket = 105, ClosingRoundBracket = 106, 
    SquareBracketLeft = 107, SquareBracketRight = 108, FloatingLiteral = 109, 
    IntegerLiteral = 110, CharLiteral = 111, Identifier = 112, BLOCK_COMMENT = 113, 
    LINE_COMMENT = 114, WS = 115, DQUOTE = 116, CURLY_L = 117, CURLY_R = 118, 
    TEXT = 119, BACKSLASH_PAREN = 120, ESCAPE_SEQUENCE = 121
  };

  enum {
    RuleProgram = 0, RuleModule = 1, RuleModuleBlock = 2, RuleModuleDeclaration = 3, 
    RuleTypedefDeclaration = 4, RuleImportDefinition = 5, RuleImportTarget = 6, 
    RuleExternDeclaration = 7, RuleExternBody = 8, RuleExternMember = 9, 
    RuleExternFunctionDeclaration = 10, RuleExternLinkDirective = 11, RuleExternOpaqueTypeDeclaration = 12, 
    RuleExternStructDeclaration = 13, RuleExternFieldDeclaration = 14, RuleExternEnumDeclaration = 15, 
    RuleClassDeclaration = 16, RuleClassBlock = 17, RuleClassMember = 18, 
    RuleConstructorDeclaration = 19, RuleDestructorDeclaration = 20, RuleOperatorDeclaration = 21, 
    RuleOverloadableOperator = 22, RuleInheritance = 23, RuleInterfaceDeclaration = 24, 
    RuleInterfaceBlock = 25, RuleInterfaceMember = 26, RuleStructDeclaration = 27, 
    RuleStructBlock = 28, RuleStructMember = 29, RuleEnumDeclaration = 30, 
    RuleEnumMember = 31, RuleFunctionDeclaration = 32, RuleReturnType = 33, 
    RuleDefinitionParameters = 34, RuleParameterList = 35, RuleParameter = 36, 
    RuleVariableDeclaration = 37, RuleConstVariableDeclaration = 38, RuleTypedVariableDeclaration = 39, 
    RuleInferredVariableDeclaration = 40, RuleTupleDestructuring = 41, RuleTupleDestructureElement = 42, 
    RuleStatement = 43, RuleBlock = 44, RuleExprStatement = 45, RuleRawBlock = 46, 
    RuleIfStatement = 47, RuleElseIfClause = 48, RuleElseClause = 49, RuleSwitchStatement = 50, 
    RuleSwitchCase = 51, RuleSwitchDefault = 52, RuleMatchStatement = 53, 
    RuleMatchExpression = 54, RuleMatchArm = 55, RuleMatchBody = 56, RulePattern = 57, 
    RuleGuardedPattern = 58, RuleBasePattern = 59, RuleLiteralPattern = 60, 
    RuleRangePattern = 61, RuleWildcardPattern = 62, RuleBindingPattern = 63, 
    RuleTuplePattern = 64, RuleForStatement = 65, RuleForInitializer = 66, 
    RuleLocalVarInitializer = 67, RuleLocalVarDeclaration = 68, RuleForIterator = 69, 
    RuleWhileStatement = 70, RuleDoWhileStatement = 71, RuleReturnStatement = 72, 
    RuleLabeledStatement = 73, RuleBreakStatement = 74, RuleContinueStatement = 75, 
    RuleDeleteStatement = 76, RuleExpression = 77, RuleAssignment = 78, 
    RuleAssignmentOperator = 79, RuleLambdaExpression = 80, RuleCaptureList = 81, 
    RuleCaptureDefault = 82, RuleCaptureItem = 83, RuleLambdaParameterList = 84, 
    RuleLambdaParameter = 85, RulePipe = 86, RulePipeTarget = 87, RuleTernary = 88, 
    RuleLogicOr = 89, RuleLogicAnd = 90, RuleBitwiseOr = 91, RuleBitwiseXor = 92, 
    RuleBitwiseAnd = 93, RuleEquality = 94, RuleRelational = 95, RuleShift = 96, 
    RuleAdditive = 97, RuleMultiplicative = 98, RuleCastExpression = 99, 
    RuleUnaryExpression = 100, RulePostfixExpression = 101, RulePrimaryExpression = 102, 
    RuleArrayLiteral = 103, RulePostfixOperation = 104, RuleNewExpression = 105, 
    RuleCallArguments = 106, RuleArgumentList = 107, RuleElementAccess = 108, 
    RuleMemberAccess = 109, RuleTupleExpression = 110, RuleTypeIdentifier = 111, 
    RulePrimitiveType = 112, RuleFunctionType = 113, RuleTypeList = 114, 
    RuleTupleType = 115, RuleTypeModifier = 116, RuleRvalueReferenceLevel = 117, 
    RuleReferenceLevel = 118, RuleArrayDimension = 119, RulePointerLevel = 120, 
    RuleAccessModifier = 121, RuleStaticModifier = 122, RuleAbstractModifier = 123, 
    RuleQualifiedName = 124, RulePrefixOperator = 125, RuleIncrementDecrementOperator = 126, 
    RuleTypeSizeOrAlign = 127, RuleString = 128, RuleStringPart = 129
  };

  explicit MingusParser(antlr4::TokenStream *input);

  MingusParser(antlr4::TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options);

  ~MingusParser() override;

  std::string getGrammarFileName() const override;

  const antlr4::atn::ATN& getATN() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;


  class ProgramContext;
  class ModuleContext;
  class ModuleBlockContext;
  class ModuleDeclarationContext;
  class TypedefDeclarationContext;
  class ImportDefinitionContext;
  class ImportTargetContext;
  class ExternDeclarationContext;
  class ExternBodyContext;
  class ExternMemberContext;
  class ExternFunctionDeclarationContext;
  class ExternLinkDirectiveContext;
  class ExternOpaqueTypeDeclarationContext;
  class ExternStructDeclarationContext;
  class ExternFieldDeclarationContext;
  class ExternEnumDeclarationContext;
  class ClassDeclarationContext;
  class ClassBlockContext;
  class ClassMemberContext;
  class ConstructorDeclarationContext;
  class DestructorDeclarationContext;
  class OperatorDeclarationContext;
  class OverloadableOperatorContext;
  class InheritanceContext;
  class InterfaceDeclarationContext;
  class InterfaceBlockContext;
  class InterfaceMemberContext;
  class StructDeclarationContext;
  class StructBlockContext;
  class StructMemberContext;
  class EnumDeclarationContext;
  class EnumMemberContext;
  class FunctionDeclarationContext;
  class ReturnTypeContext;
  class DefinitionParametersContext;
  class ParameterListContext;
  class ParameterContext;
  class VariableDeclarationContext;
  class ConstVariableDeclarationContext;
  class TypedVariableDeclarationContext;
  class InferredVariableDeclarationContext;
  class TupleDestructuringContext;
  class TupleDestructureElementContext;
  class StatementContext;
  class BlockContext;
  class ExprStatementContext;
  class RawBlockContext;
  class IfStatementContext;
  class ElseIfClauseContext;
  class ElseClauseContext;
  class SwitchStatementContext;
  class SwitchCaseContext;
  class SwitchDefaultContext;
  class MatchStatementContext;
  class MatchExpressionContext;
  class MatchArmContext;
  class MatchBodyContext;
  class PatternContext;
  class GuardedPatternContext;
  class BasePatternContext;
  class LiteralPatternContext;
  class RangePatternContext;
  class WildcardPatternContext;
  class BindingPatternContext;
  class TuplePatternContext;
  class ForStatementContext;
  class ForInitializerContext;
  class LocalVarInitializerContext;
  class LocalVarDeclarationContext;
  class ForIteratorContext;
  class WhileStatementContext;
  class DoWhileStatementContext;
  class ReturnStatementContext;
  class LabeledStatementContext;
  class BreakStatementContext;
  class ContinueStatementContext;
  class DeleteStatementContext;
  class ExpressionContext;
  class AssignmentContext;
  class AssignmentOperatorContext;
  class LambdaExpressionContext;
  class CaptureListContext;
  class CaptureDefaultContext;
  class CaptureItemContext;
  class LambdaParameterListContext;
  class LambdaParameterContext;
  class PipeContext;
  class PipeTargetContext;
  class TernaryContext;
  class LogicOrContext;
  class LogicAndContext;
  class BitwiseOrContext;
  class BitwiseXorContext;
  class BitwiseAndContext;
  class EqualityContext;
  class RelationalContext;
  class ShiftContext;
  class AdditiveContext;
  class MultiplicativeContext;
  class CastExpressionContext;
  class UnaryExpressionContext;
  class PostfixExpressionContext;
  class PrimaryExpressionContext;
  class ArrayLiteralContext;
  class PostfixOperationContext;
  class NewExpressionContext;
  class CallArgumentsContext;
  class ArgumentListContext;
  class ElementAccessContext;
  class MemberAccessContext;
  class TupleExpressionContext;
  class TypeIdentifierContext;
  class PrimitiveTypeContext;
  class FunctionTypeContext;
  class TypeListContext;
  class TupleTypeContext;
  class TypeModifierContext;
  class RvalueReferenceLevelContext;
  class ReferenceLevelContext;
  class ArrayDimensionContext;
  class PointerLevelContext;
  class AccessModifierContext;
  class StaticModifierContext;
  class AbstractModifierContext;
  class QualifiedNameContext;
  class PrefixOperatorContext;
  class IncrementDecrementOperatorContext;
  class TypeSizeOrAlignContext;
  class StringContext;
  class StringPartContext; 

  class  ProgramContext : public antlr4::ParserRuleContext {
  public:
    ProgramContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *EOF();
    std::vector<ModuleContext *> module();
    ModuleContext* module(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ProgramContext* program();

  class  ModuleContext : public antlr4::ParserRuleContext {
  public:
    ModuleContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeclareModule();
    antlr4::tree::TerminalNode *Identifier();
    ModuleBlockContext *moduleBlock();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ModuleContext* module();

  class  ModuleBlockContext : public antlr4::ParserRuleContext {
  public:
    ModuleBlockContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *CURLY_L();
    antlr4::tree::TerminalNode *CURLY_R();
    std::vector<ModuleDeclarationContext *> moduleDeclaration();
    ModuleDeclarationContext* moduleDeclaration(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ModuleBlockContext* moduleBlock();

  class  ModuleDeclarationContext : public antlr4::ParserRuleContext {
  public:
    ModuleDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ClassDeclarationContext *classDeclaration();
    StructDeclarationContext *structDeclaration();
    EnumDeclarationContext *enumDeclaration();
    InterfaceDeclarationContext *interfaceDeclaration();
    FunctionDeclarationContext *functionDeclaration();
    ExternDeclarationContext *externDeclaration();
    VariableDeclarationContext *variableDeclaration();
    TypedefDeclarationContext *typedefDeclaration();
    ImportDefinitionContext *importDefinition();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ModuleDeclarationContext* moduleDeclaration();

  class  TypedefDeclarationContext : public antlr4::ParserRuleContext {
  public:
    TypedefDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeclareTypedef();
    TypeIdentifierContext *typeIdentifier();
    antlr4::tree::TerminalNode *Identifier();
    antlr4::tree::TerminalNode *SemicolonSeparator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TypedefDeclarationContext* typedefDeclaration();

  class  ImportDefinitionContext : public antlr4::ParserRuleContext {
  public:
    ImportDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *ImportDirective();
    std::vector<ImportTargetContext *> importTarget();
    ImportTargetContext* importTarget(size_t i);
    antlr4::tree::TerminalNode *SemicolonSeparator();
    std::vector<antlr4::tree::TerminalNode *> CommaSeparator();
    antlr4::tree::TerminalNode* CommaSeparator(size_t i);
    antlr4::tree::TerminalNode *FromDirective();
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ImportDefinitionContext* importDefinition();

  class  ImportTargetContext : public antlr4::ParserRuleContext {
  public:
    ImportTargetContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> Identifier();
    antlr4::tree::TerminalNode* Identifier(size_t i);
    antlr4::tree::TerminalNode *AsKeyword();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ImportTargetContext* importTarget();

  class  ExternDeclarationContext : public antlr4::ParserRuleContext {
  public:
    ExternDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *ExternKeyword();
    ExternBodyContext *externBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExternDeclarationContext* externDeclaration();

  class  ExternBodyContext : public antlr4::ParserRuleContext {
  public:
    ExternBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExternFunctionDeclarationContext *externFunctionDeclaration();
    ExternLinkDirectiveContext *externLinkDirective();
    ExternOpaqueTypeDeclarationContext *externOpaqueTypeDeclaration();
    antlr4::tree::TerminalNode *CURLY_L();
    antlr4::tree::TerminalNode *CURLY_R();
    std::vector<ExternMemberContext *> externMember();
    ExternMemberContext* externMember(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExternBodyContext* externBody();

  class  ExternMemberContext : public antlr4::ParserRuleContext {
  public:
    ExternMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExternFunctionDeclarationContext *externFunctionDeclaration();
    ExternLinkDirectiveContext *externLinkDirective();
    ExternOpaqueTypeDeclarationContext *externOpaqueTypeDeclaration();
    ExternStructDeclarationContext *externStructDeclaration();
    ExternEnumDeclarationContext *externEnumDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExternMemberContext* externMember();

  class  ExternFunctionDeclarationContext : public antlr4::ParserRuleContext {
  public:
    ExternFunctionDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeclareFunction();
    antlr4::tree::TerminalNode *Identifier();
    antlr4::tree::TerminalNode *OpeningRoundBracket();
    antlr4::tree::TerminalNode *ClosingRoundBracket();
    antlr4::tree::TerminalNode *ArrowOperator();
    ReturnTypeContext *returnType();
    antlr4::tree::TerminalNode *SemicolonSeparator();
    ParameterListContext *parameterList();
    antlr4::tree::TerminalNode *CommaSeparator();
    antlr4::tree::TerminalNode *Ellipsis();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExternFunctionDeclarationContext* externFunctionDeclaration();

  class  ExternLinkDirectiveContext : public antlr4::ParserRuleContext {
  public:
    ExternLinkDirectiveContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *LinkKeyword();
    StringContext *string();
    antlr4::tree::TerminalNode *SemicolonSeparator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExternLinkDirectiveContext* externLinkDirective();

  class  ExternOpaqueTypeDeclarationContext : public antlr4::ParserRuleContext {
  public:
    ExternOpaqueTypeDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *OpaqueKeyword();
    antlr4::tree::TerminalNode *Identifier();
    antlr4::tree::TerminalNode *SemicolonSeparator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExternOpaqueTypeDeclarationContext* externOpaqueTypeDeclaration();

  class  ExternStructDeclarationContext : public antlr4::ParserRuleContext {
  public:
    ExternStructDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeclareStruct();
    antlr4::tree::TerminalNode *Identifier();
    antlr4::tree::TerminalNode *CURLY_L();
    antlr4::tree::TerminalNode *CURLY_R();
    std::vector<ExternFieldDeclarationContext *> externFieldDeclaration();
    ExternFieldDeclarationContext* externFieldDeclaration(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExternStructDeclarationContext* externStructDeclaration();

  class  ExternFieldDeclarationContext : public antlr4::ParserRuleContext {
  public:
    ExternFieldDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    TypeIdentifierContext *typeIdentifier();
    antlr4::tree::TerminalNode *Identifier();
    antlr4::tree::TerminalNode *SemicolonSeparator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExternFieldDeclarationContext* externFieldDeclaration();

  class  ExternEnumDeclarationContext : public antlr4::ParserRuleContext {
  public:
    ExternEnumDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeclareEnum();
    antlr4::tree::TerminalNode *Identifier();
    antlr4::tree::TerminalNode *CURLY_L();
    std::vector<EnumMemberContext *> enumMember();
    EnumMemberContext* enumMember(size_t i);
    antlr4::tree::TerminalNode *CURLY_R();
    antlr4::tree::TerminalNode *ColonOperator();
    TypeIdentifierContext *typeIdentifier();
    std::vector<antlr4::tree::TerminalNode *> CommaSeparator();
    antlr4::tree::TerminalNode* CommaSeparator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExternEnumDeclarationContext* externEnumDeclaration();

  class  ClassDeclarationContext : public antlr4::ParserRuleContext {
  public:
    ClassDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeclareClass();
    antlr4::tree::TerminalNode *Identifier();
    ClassBlockContext *classBlock();
    antlr4::tree::TerminalNode *SemicolonSeparator();
    AccessModifierContext *accessModifier();
    StaticModifierContext *staticModifier();
    AbstractModifierContext *abstractModifier();
    antlr4::tree::TerminalNode *ColonOperator();
    InheritanceContext *inheritance();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ClassDeclarationContext* classDeclaration();

  class  ClassBlockContext : public antlr4::ParserRuleContext {
  public:
    ClassBlockContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *CURLY_L();
    antlr4::tree::TerminalNode *CURLY_R();
    std::vector<ClassMemberContext *> classMember();
    ClassMemberContext* classMember(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ClassBlockContext* classBlock();

  class  ClassMemberContext : public antlr4::ParserRuleContext {
  public:
    ClassMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ConstructorDeclarationContext *constructorDeclaration();
    DestructorDeclarationContext *destructorDeclaration();
    OperatorDeclarationContext *operatorDeclaration();
    FunctionDeclarationContext *functionDeclaration();
    VariableDeclarationContext *variableDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ClassMemberContext* classMember();

  class  ConstructorDeclarationContext : public antlr4::ParserRuleContext {
  public:
    ConstructorDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeclareConstructor();
    DefinitionParametersContext *definitionParameters();
    BlockContext *block();
    AccessModifierContext *accessModifier();
    antlr4::tree::TerminalNode *ColonOperator();
    antlr4::tree::TerminalNode *SuperKeyword();
    CallArgumentsContext *callArguments();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ConstructorDeclarationContext* constructorDeclaration();

  class  DestructorDeclarationContext : public antlr4::ParserRuleContext {
  public:
    DestructorDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeclareDestructor();
    BlockContext *block();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  DestructorDeclarationContext* destructorDeclaration();

  class  OperatorDeclarationContext : public antlr4::ParserRuleContext {
  public:
    OperatorDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeclareFunction();
    antlr4::tree::TerminalNode *DeclareOperator();
    OverloadableOperatorContext *overloadableOperator();
    DefinitionParametersContext *definitionParameters();
    std::vector<antlr4::tree::TerminalNode *> ArrowOperator();
    antlr4::tree::TerminalNode* ArrowOperator(size_t i);
    ReturnTypeContext *returnType();
    BlockContext *block();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *SemicolonSeparator();
    AccessModifierContext *accessModifier();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  OperatorDeclarationContext* operatorDeclaration();

  class  OverloadableOperatorContext : public antlr4::ParserRuleContext {
  public:
    OverloadableOperatorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *PlusOperator();
    antlr4::tree::TerminalNode *MinusOperator();
    antlr4::tree::TerminalNode *StarOperator();
    antlr4::tree::TerminalNode *DivideOperator();
    antlr4::tree::TerminalNode *ModuloOperator();
    antlr4::tree::TerminalNode *EqualOperator();
    antlr4::tree::TerminalNode *UnequalOperator();
    antlr4::tree::TerminalNode *SmallerOperator();
    antlr4::tree::TerminalNode *SmallerEqualOperator();
    antlr4::tree::TerminalNode *GreaterOperator();
    antlr4::tree::TerminalNode *GreaterEqualOperator();
    antlr4::tree::TerminalNode *SquareBracketLeft();
    antlr4::tree::TerminalNode *SquareBracketRight();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  OverloadableOperatorContext* overloadableOperator();

  class  InheritanceContext : public antlr4::ParserRuleContext {
  public:
    InheritanceContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<QualifiedNameContext *> qualifiedName();
    QualifiedNameContext* qualifiedName(size_t i);
    std::vector<antlr4::tree::TerminalNode *> CommaSeparator();
    antlr4::tree::TerminalNode* CommaSeparator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  InheritanceContext* inheritance();

  class  InterfaceDeclarationContext : public antlr4::ParserRuleContext {
  public:
    InterfaceDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeclareInterface();
    antlr4::tree::TerminalNode *Identifier();
    InterfaceBlockContext *interfaceBlock();
    antlr4::tree::TerminalNode *SemicolonSeparator();
    AccessModifierContext *accessModifier();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  InterfaceDeclarationContext* interfaceDeclaration();

  class  InterfaceBlockContext : public antlr4::ParserRuleContext {
  public:
    InterfaceBlockContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *CURLY_L();
    antlr4::tree::TerminalNode *CURLY_R();
    std::vector<InterfaceMemberContext *> interfaceMember();
    InterfaceMemberContext* interfaceMember(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  InterfaceBlockContext* interfaceBlock();

  class  InterfaceMemberContext : public antlr4::ParserRuleContext {
  public:
    InterfaceMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    FunctionDeclarationContext *functionDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  InterfaceMemberContext* interfaceMember();

  class  StructDeclarationContext : public antlr4::ParserRuleContext {
  public:
    StructDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeclareStruct();
    antlr4::tree::TerminalNode *Identifier();
    StructBlockContext *structBlock();
    antlr4::tree::TerminalNode *SemicolonSeparator();
    AccessModifierContext *accessModifier();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  StructDeclarationContext* structDeclaration();

  class  StructBlockContext : public antlr4::ParserRuleContext {
  public:
    StructBlockContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *CURLY_L();
    antlr4::tree::TerminalNode *CURLY_R();
    std::vector<StructMemberContext *> structMember();
    StructMemberContext* structMember(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  StructBlockContext* structBlock();

  class  StructMemberContext : public antlr4::ParserRuleContext {
  public:
    StructMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OperatorDeclarationContext *operatorDeclaration();
    FunctionDeclarationContext *functionDeclaration();
    VariableDeclarationContext *variableDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  StructMemberContext* structMember();

  class  EnumDeclarationContext : public antlr4::ParserRuleContext {
  public:
    EnumDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeclareEnum();
    antlr4::tree::TerminalNode *Identifier();
    antlr4::tree::TerminalNode *CURLY_L();
    std::vector<EnumMemberContext *> enumMember();
    EnumMemberContext* enumMember(size_t i);
    antlr4::tree::TerminalNode *CURLY_R();
    AccessModifierContext *accessModifier();
    antlr4::tree::TerminalNode *ColonOperator();
    TypeIdentifierContext *typeIdentifier();
    std::vector<antlr4::tree::TerminalNode *> CommaSeparator();
    antlr4::tree::TerminalNode* CommaSeparator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  EnumDeclarationContext* enumDeclaration();

  class  EnumMemberContext : public antlr4::ParserRuleContext {
  public:
    EnumMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *Identifier();
    antlr4::tree::TerminalNode *AssignOperator();
    ExpressionContext *expression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  EnumMemberContext* enumMember();

  class  FunctionDeclarationContext : public antlr4::ParserRuleContext {
  public:
    FunctionDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeclareFunction();
    antlr4::tree::TerminalNode *Identifier();
    DefinitionParametersContext *definitionParameters();
    std::vector<antlr4::tree::TerminalNode *> ArrowOperator();
    antlr4::tree::TerminalNode* ArrowOperator(size_t i);
    ReturnTypeContext *returnType();
    BlockContext *block();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *SemicolonSeparator();
    AccessModifierContext *accessModifier();
    StaticModifierContext *staticModifier();
    AbstractModifierContext *abstractModifier();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  FunctionDeclarationContext* functionDeclaration();

  class  ReturnTypeContext : public antlr4::ParserRuleContext {
  public:
    ReturnTypeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    TypeIdentifierContext *typeIdentifier();
    TupleTypeContext *tupleType();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ReturnTypeContext* returnType();

  class  DefinitionParametersContext : public antlr4::ParserRuleContext {
  public:
    DefinitionParametersContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *OpeningRoundBracket();
    antlr4::tree::TerminalNode *ClosingRoundBracket();
    ParameterListContext *parameterList();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  DefinitionParametersContext* definitionParameters();

  class  ParameterListContext : public antlr4::ParserRuleContext {
  public:
    ParameterListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ParameterContext *> parameter();
    ParameterContext* parameter(size_t i);
    std::vector<antlr4::tree::TerminalNode *> CommaSeparator();
    antlr4::tree::TerminalNode* CommaSeparator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ParameterListContext* parameterList();

  class  ParameterContext : public antlr4::ParserRuleContext {
  public:
    ParameterContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    TypeIdentifierContext *typeIdentifier();
    antlr4::tree::TerminalNode *Identifier();
    antlr4::tree::TerminalNode *AssignOperator();
    ExpressionContext *expression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ParameterContext* parameter();

  class  VariableDeclarationContext : public antlr4::ParserRuleContext {
  public:
    VariableDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    TypedVariableDeclarationContext *typedVariableDeclaration();
    AccessModifierContext *accessModifier();
    StaticModifierContext *staticModifier();
    InferredVariableDeclarationContext *inferredVariableDeclaration();
    ConstVariableDeclarationContext *constVariableDeclaration();
    TupleDestructuringContext *tupleDestructuring();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  VariableDeclarationContext* variableDeclaration();

  class  ConstVariableDeclarationContext : public antlr4::ParserRuleContext {
  public:
    ConstVariableDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeclareConst();
    TypeIdentifierContext *typeIdentifier();
    antlr4::tree::TerminalNode *Identifier();
    antlr4::tree::TerminalNode *AssignOperator();
    ExprStatementContext *exprStatement();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ConstVariableDeclarationContext* constVariableDeclaration();

  class  TypedVariableDeclarationContext : public antlr4::ParserRuleContext {
  public:
    TypedVariableDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    TypeIdentifierContext *typeIdentifier();
    antlr4::tree::TerminalNode *Identifier();
    antlr4::tree::TerminalNode *SemicolonSeparator();
    antlr4::tree::TerminalNode *AssignOperator();
    ExprStatementContext *exprStatement();
    CallArgumentsContext *callArguments();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TypedVariableDeclarationContext* typedVariableDeclaration();

  class  InferredVariableDeclarationContext : public antlr4::ParserRuleContext {
  public:
    InferredVariableDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeclareVariable();
    antlr4::tree::TerminalNode *Identifier();
    antlr4::tree::TerminalNode *AssignOperator();
    ExprStatementContext *exprStatement();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  InferredVariableDeclarationContext* inferredVariableDeclaration();

  class  TupleDestructuringContext : public antlr4::ParserRuleContext {
  public:
    TupleDestructuringContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *OpeningRoundBracket();
    std::vector<TupleDestructureElementContext *> tupleDestructureElement();
    TupleDestructureElementContext* tupleDestructureElement(size_t i);
    antlr4::tree::TerminalNode *ClosingRoundBracket();
    antlr4::tree::TerminalNode *AssignOperator();
    ExprStatementContext *exprStatement();
    std::vector<antlr4::tree::TerminalNode *> CommaSeparator();
    antlr4::tree::TerminalNode* CommaSeparator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TupleDestructuringContext* tupleDestructuring();

  class  TupleDestructureElementContext : public antlr4::ParserRuleContext {
  public:
    TupleDestructureElementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    TypeIdentifierContext *typeIdentifier();
    antlr4::tree::TerminalNode *Identifier();
    antlr4::tree::TerminalNode *DeclareVariable();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TupleDestructureElementContext* tupleDestructureElement();

  class  StatementContext : public antlr4::ParserRuleContext {
  public:
    StatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExprStatementContext *exprStatement();
    VariableDeclarationContext *variableDeclaration();
    ForStatementContext *forStatement();
    WhileStatementContext *whileStatement();
    DoWhileStatementContext *doWhileStatement();
    IfStatementContext *ifStatement();
    SwitchStatementContext *switchStatement();
    MatchStatementContext *matchStatement();
    ReturnStatementContext *returnStatement();
    LabeledStatementContext *labeledStatement();
    BreakStatementContext *breakStatement();
    ContinueStatementContext *continueStatement();
    DeleteStatementContext *deleteStatement();
    RawBlockContext *rawBlock();
    BlockContext *block();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  StatementContext* statement();

  class  BlockContext : public antlr4::ParserRuleContext {
  public:
    BlockContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *CURLY_L();
    antlr4::tree::TerminalNode *CURLY_R();
    std::vector<StatementContext *> statement();
    StatementContext* statement(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  BlockContext* block();

  class  ExprStatementContext : public antlr4::ParserRuleContext {
  public:
    ExprStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *SemicolonSeparator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExprStatementContext* exprStatement();

  class  RawBlockContext : public antlr4::ParserRuleContext {
  public:
    RawBlockContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *RawKeyword();
    BlockContext *block();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  RawBlockContext* rawBlock();

  class  IfStatementContext : public antlr4::ParserRuleContext {
  public:
    MingusParser::StatementContext *trueBody = nullptr;
    IfStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *ControlFlowIf();
    antlr4::tree::TerminalNode *OpeningRoundBracket();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *ClosingRoundBracket();
    StatementContext *statement();
    std::vector<ElseIfClauseContext *> elseIfClause();
    ElseIfClauseContext* elseIfClause(size_t i);
    ElseClauseContext *elseClause();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  IfStatementContext* ifStatement();

  class  ElseIfClauseContext : public antlr4::ParserRuleContext {
  public:
    ElseIfClauseContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *ControlFlowElse();
    antlr4::tree::TerminalNode *ControlFlowIf();
    antlr4::tree::TerminalNode *OpeningRoundBracket();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *ClosingRoundBracket();
    StatementContext *statement();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ElseIfClauseContext* elseIfClause();

  class  ElseClauseContext : public antlr4::ParserRuleContext {
  public:
    ElseClauseContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *ControlFlowElse();
    StatementContext *statement();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ElseClauseContext* elseClause();

  class  SwitchStatementContext : public antlr4::ParserRuleContext {
  public:
    SwitchStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *ControlFlowSwitch();
    antlr4::tree::TerminalNode *OpeningRoundBracket();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *ClosingRoundBracket();
    antlr4::tree::TerminalNode *CURLY_L();
    antlr4::tree::TerminalNode *CURLY_R();
    std::vector<SwitchCaseContext *> switchCase();
    SwitchCaseContext* switchCase(size_t i);
    SwitchDefaultContext *switchDefault();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SwitchStatementContext* switchStatement();

  class  SwitchCaseContext : public antlr4::ParserRuleContext {
  public:
    SwitchCaseContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *ControlFlowCase();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *ColonOperator();
    std::vector<StatementContext *> statement();
    StatementContext* statement(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SwitchCaseContext* switchCase();

  class  SwitchDefaultContext : public antlr4::ParserRuleContext {
  public:
    SwitchDefaultContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *ControlFlowDefault();
    antlr4::tree::TerminalNode *ColonOperator();
    std::vector<StatementContext *> statement();
    StatementContext* statement(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SwitchDefaultContext* switchDefault();

  class  MatchStatementContext : public antlr4::ParserRuleContext {
  public:
    MatchStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    MatchExpressionContext *matchExpression();
    antlr4::tree::TerminalNode *SemicolonSeparator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  MatchStatementContext* matchStatement();

  class  MatchExpressionContext : public antlr4::ParserRuleContext {
  public:
    MatchExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *ControlFlowMatch();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *CURLY_L();
    std::vector<MatchArmContext *> matchArm();
    MatchArmContext* matchArm(size_t i);
    antlr4::tree::TerminalNode *CURLY_R();
    std::vector<antlr4::tree::TerminalNode *> CommaSeparator();
    antlr4::tree::TerminalNode* CommaSeparator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  MatchExpressionContext* matchExpression();

  class  MatchArmContext : public antlr4::ParserRuleContext {
  public:
    MatchArmContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    PatternContext *pattern();
    antlr4::tree::TerminalNode *ArrowOperator();
    MatchBodyContext *matchBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  MatchArmContext* matchArm();

  class  MatchBodyContext : public antlr4::ParserRuleContext {
  public:
    MatchBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExpressionContext *expression();
    BlockContext *block();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  MatchBodyContext* matchBody();

  class  PatternContext : public antlr4::ParserRuleContext {
  public:
    PatternContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    GuardedPatternContext *guardedPattern();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PatternContext* pattern();

  class  GuardedPatternContext : public antlr4::ParserRuleContext {
  public:
    GuardedPatternContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    BasePatternContext *basePattern();
    antlr4::tree::TerminalNode *ControlFlowIf();
    ExpressionContext *expression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  GuardedPatternContext* guardedPattern();

  class  BasePatternContext : public antlr4::ParserRuleContext {
  public:
    BasePatternContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    LiteralPatternContext *literalPattern();
    RangePatternContext *rangePattern();
    WildcardPatternContext *wildcardPattern();
    BindingPatternContext *bindingPattern();
    TuplePatternContext *tuplePattern();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  BasePatternContext* basePattern();

  class  LiteralPatternContext : public antlr4::ParserRuleContext {
  public:
    LiteralPatternContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *IntegerLiteral();
    antlr4::tree::TerminalNode *FloatingLiteral();
    antlr4::tree::TerminalNode *BooleanLiteral();
    antlr4::tree::TerminalNode *CharLiteral();
    StringContext *string();
    antlr4::tree::TerminalNode *NullReference();
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  LiteralPatternContext* literalPattern();

  class  RangePatternContext : public antlr4::ParserRuleContext {
  public:
    RangePatternContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> IntegerLiteral();
    antlr4::tree::TerminalNode* IntegerLiteral(size_t i);
    antlr4::tree::TerminalNode *DotDotOperator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  RangePatternContext* rangePattern();

  class  WildcardPatternContext : public antlr4::ParserRuleContext {
  public:
    WildcardPatternContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *UnderscoreWildcard();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  WildcardPatternContext* wildcardPattern();

  class  BindingPatternContext : public antlr4::ParserRuleContext {
  public:
    BindingPatternContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeclareVariable();
    antlr4::tree::TerminalNode *Identifier();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  BindingPatternContext* bindingPattern();

  class  TuplePatternContext : public antlr4::ParserRuleContext {
  public:
    TuplePatternContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *OpeningRoundBracket();
    std::vector<PatternContext *> pattern();
    PatternContext* pattern(size_t i);
    antlr4::tree::TerminalNode *ClosingRoundBracket();
    std::vector<antlr4::tree::TerminalNode *> CommaSeparator();
    antlr4::tree::TerminalNode* CommaSeparator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TuplePatternContext* tuplePattern();

  class  ForStatementContext : public antlr4::ParserRuleContext {
  public:
    ForStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeclareForLoop();
    antlr4::tree::TerminalNode *OpeningRoundBracket();
    std::vector<antlr4::tree::TerminalNode *> SemicolonSeparator();
    antlr4::tree::TerminalNode* SemicolonSeparator(size_t i);
    antlr4::tree::TerminalNode *ClosingRoundBracket();
    StatementContext *statement();
    ForInitializerContext *forInitializer();
    ExpressionContext *expression();
    ForIteratorContext *forIterator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ForStatementContext* forStatement();

  class  ForInitializerContext : public antlr4::ParserRuleContext {
  public:
    ForInitializerContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    LocalVarInitializerContext *localVarInitializer();
    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    std::vector<antlr4::tree::TerminalNode *> CommaSeparator();
    antlr4::tree::TerminalNode* CommaSeparator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ForInitializerContext* forInitializer();

  class  LocalVarInitializerContext : public antlr4::ParserRuleContext {
  public:
    LocalVarInitializerContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<LocalVarDeclarationContext *> localVarDeclaration();
    LocalVarDeclarationContext* localVarDeclaration(size_t i);
    std::vector<antlr4::tree::TerminalNode *> CommaSeparator();
    antlr4::tree::TerminalNode* CommaSeparator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  LocalVarInitializerContext* localVarInitializer();

  class  LocalVarDeclarationContext : public antlr4::ParserRuleContext {
  public:
    LocalVarDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    TypeIdentifierContext *typeIdentifier();
    antlr4::tree::TerminalNode *Identifier();
    antlr4::tree::TerminalNode *AssignOperator();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *DeclareVariable();
    antlr4::tree::TerminalNode *DeclareConst();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  LocalVarDeclarationContext* localVarDeclaration();

  class  ForIteratorContext : public antlr4::ParserRuleContext {
  public:
    ForIteratorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    std::vector<antlr4::tree::TerminalNode *> CommaSeparator();
    antlr4::tree::TerminalNode* CommaSeparator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ForIteratorContext* forIterator();

  class  WhileStatementContext : public antlr4::ParserRuleContext {
  public:
    WhileStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeclareWhileLoop();
    antlr4::tree::TerminalNode *OpeningRoundBracket();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *ClosingRoundBracket();
    BlockContext *block();
    StatementContext *statement();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  WhileStatementContext* whileStatement();

  class  DoWhileStatementContext : public antlr4::ParserRuleContext {
  public:
    DoWhileStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeclareDoLoop();
    antlr4::tree::TerminalNode *DeclareWhileLoop();
    antlr4::tree::TerminalNode *OpeningRoundBracket();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *ClosingRoundBracket();
    antlr4::tree::TerminalNode *SemicolonSeparator();
    BlockContext *block();
    StatementContext *statement();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  DoWhileStatementContext* doWhileStatement();

  class  ReturnStatementContext : public antlr4::ParserRuleContext {
  public:
    ReturnStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *FunctionReturn();
    antlr4::tree::TerminalNode *SemicolonSeparator();
    ExpressionContext *expression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ReturnStatementContext* returnStatement();

  class  LabeledStatementContext : public antlr4::ParserRuleContext {
  public:
    LabeledStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *Identifier();
    antlr4::tree::TerminalNode *ColonOperator();
    ForStatementContext *forStatement();
    WhileStatementContext *whileStatement();
    DoWhileStatementContext *doWhileStatement();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  LabeledStatementContext* labeledStatement();

  class  BreakStatementContext : public antlr4::ParserRuleContext {
  public:
    BreakStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *Break();
    antlr4::tree::TerminalNode *SemicolonSeparator();
    antlr4::tree::TerminalNode *Identifier();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  BreakStatementContext* breakStatement();

  class  ContinueStatementContext : public antlr4::ParserRuleContext {
  public:
    ContinueStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *Continue();
    antlr4::tree::TerminalNode *SemicolonSeparator();
    antlr4::tree::TerminalNode *Identifier();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ContinueStatementContext* continueStatement();

  class  DeleteStatementContext : public antlr4::ParserRuleContext {
  public:
    DeleteStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeleteKeyword();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *SemicolonSeparator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  DeleteStatementContext* deleteStatement();

  class  ExpressionContext : public antlr4::ParserRuleContext {
  public:
    ExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    AssignmentContext *assignment();
    LambdaExpressionContext *lambdaExpression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExpressionContext* expression();

  class  AssignmentContext : public antlr4::ParserRuleContext {
  public:
    AssignmentContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    UnaryExpressionContext *unaryExpression();
    AssignmentOperatorContext *assignmentOperator();
    LambdaExpressionContext *lambdaExpression();
    AssignmentContext *assignment();
    PipeContext *pipe();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AssignmentContext* assignment();

  class  AssignmentOperatorContext : public antlr4::ParserRuleContext {
  public:
    AssignmentOperatorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *AssignOperator();
    antlr4::tree::TerminalNode *PlusAssignOperator();
    antlr4::tree::TerminalNode *MinusAssignOperator();
    antlr4::tree::TerminalNode *MultiplyAssignOperator();
    antlr4::tree::TerminalNode *DivideAssignOperator();
    antlr4::tree::TerminalNode *ModuloAssignOperator();
    antlr4::tree::TerminalNode *BitwiseAndAssignOperator();
    antlr4::tree::TerminalNode *BitwiseOrAssignOperator();
    antlr4::tree::TerminalNode *BitwiseXorAssignOperator();
    antlr4::tree::TerminalNode *BitwiseLeftShiftAssignOperator();
    antlr4::tree::TerminalNode *BitwiseRightShiftAssignOperator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AssignmentOperatorContext* assignmentOperator();

  class  LambdaExpressionContext : public antlr4::ParserRuleContext {
  public:
    LambdaExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    CaptureListContext *captureList();
    antlr4::tree::TerminalNode *OpeningRoundBracket();
    antlr4::tree::TerminalNode *ClosingRoundBracket();
    antlr4::tree::TerminalNode *ArrowOperator();
    BlockContext *block();
    ExpressionContext *expression();
    LambdaParameterListContext *lambdaParameterList();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  LambdaExpressionContext* lambdaExpression();

  class  CaptureListContext : public antlr4::ParserRuleContext {
  public:
    CaptureListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *SquareBracketLeft();
    antlr4::tree::TerminalNode *SquareBracketRight();
    CaptureDefaultContext *captureDefault();
    std::vector<antlr4::tree::TerminalNode *> CommaSeparator();
    antlr4::tree::TerminalNode* CommaSeparator(size_t i);
    std::vector<CaptureItemContext *> captureItem();
    CaptureItemContext* captureItem(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  CaptureListContext* captureList();

  class  CaptureDefaultContext : public antlr4::ParserRuleContext {
  public:
    CaptureDefaultContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *AssignOperator();
    antlr4::tree::TerminalNode *SingleAndOperator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  CaptureDefaultContext* captureDefault();

  class  CaptureItemContext : public antlr4::ParserRuleContext {
  public:
    CaptureItemContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *SingleAndOperator();
    antlr4::tree::TerminalNode *Identifier();
    antlr4::tree::TerminalNode *WeakKeyword();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  CaptureItemContext* captureItem();

  class  LambdaParameterListContext : public antlr4::ParserRuleContext {
  public:
    LambdaParameterListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<LambdaParameterContext *> lambdaParameter();
    LambdaParameterContext* lambdaParameter(size_t i);
    std::vector<antlr4::tree::TerminalNode *> CommaSeparator();
    antlr4::tree::TerminalNode* CommaSeparator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  LambdaParameterListContext* lambdaParameterList();

  class  LambdaParameterContext : public antlr4::ParserRuleContext {
  public:
    LambdaParameterContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    TypeIdentifierContext *typeIdentifier();
    antlr4::tree::TerminalNode *Identifier();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  LambdaParameterContext* lambdaParameter();

  class  PipeContext : public antlr4::ParserRuleContext {
  public:
    PipeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    TernaryContext *ternary();
    std::vector<antlr4::tree::TerminalNode *> PipeOperator();
    antlr4::tree::TerminalNode* PipeOperator(size_t i);
    std::vector<PipeTargetContext *> pipeTarget();
    PipeTargetContext* pipeTarget(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PipeContext* pipe();

  class  PipeTargetContext : public antlr4::ParserRuleContext {
  public:
    PipeTargetContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QualifiedNameContext *qualifiedName();
    std::vector<antlr4::tree::TerminalNode *> Identifier();
    antlr4::tree::TerminalNode* Identifier(size_t i);
    CallArgumentsContext *callArguments();
    std::vector<antlr4::tree::TerminalNode *> DotOperator();
    antlr4::tree::TerminalNode* DotOperator(size_t i);
    std::vector<antlr4::tree::TerminalNode *> ReferenceAccessOperator();
    antlr4::tree::TerminalNode* ReferenceAccessOperator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PipeTargetContext* pipeTarget();

  class  TernaryContext : public antlr4::ParserRuleContext {
  public:
    TernaryContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    LogicOrContext *logicOr();
    antlr4::tree::TerminalNode *QuestionMarkOperator();
    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    antlr4::tree::TerminalNode *ColonOperator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TernaryContext* ternary();

  class  LogicOrContext : public antlr4::ParserRuleContext {
  public:
    LogicOrContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<LogicAndContext *> logicAnd();
    LogicAndContext* logicAnd(size_t i);
    std::vector<antlr4::tree::TerminalNode *> LogicalOrOperator();
    antlr4::tree::TerminalNode* LogicalOrOperator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  LogicOrContext* logicOr();

  class  LogicAndContext : public antlr4::ParserRuleContext {
  public:
    LogicAndContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<BitwiseOrContext *> bitwiseOr();
    BitwiseOrContext* bitwiseOr(size_t i);
    std::vector<antlr4::tree::TerminalNode *> LogicalAndOperator();
    antlr4::tree::TerminalNode* LogicalAndOperator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  LogicAndContext* logicAnd();

  class  BitwiseOrContext : public antlr4::ParserRuleContext {
  public:
    BitwiseOrContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<BitwiseXorContext *> bitwiseXor();
    BitwiseXorContext* bitwiseXor(size_t i);
    std::vector<antlr4::tree::TerminalNode *> BitwiseOrOperator();
    antlr4::tree::TerminalNode* BitwiseOrOperator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  BitwiseOrContext* bitwiseOr();

  class  BitwiseXorContext : public antlr4::ParserRuleContext {
  public:
    BitwiseXorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<BitwiseAndContext *> bitwiseAnd();
    BitwiseAndContext* bitwiseAnd(size_t i);
    std::vector<antlr4::tree::TerminalNode *> BitwiseXorOperator();
    antlr4::tree::TerminalNode* BitwiseXorOperator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  BitwiseXorContext* bitwiseXor();

  class  BitwiseAndContext : public antlr4::ParserRuleContext {
  public:
    BitwiseAndContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<EqualityContext *> equality();
    EqualityContext* equality(size_t i);
    std::vector<antlr4::tree::TerminalNode *> SingleAndOperator();
    antlr4::tree::TerminalNode* SingleAndOperator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  BitwiseAndContext* bitwiseAnd();

  class  EqualityContext : public antlr4::ParserRuleContext {
  public:
    EqualityContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<RelationalContext *> relational();
    RelationalContext* relational(size_t i);
    std::vector<antlr4::tree::TerminalNode *> EqualOperator();
    antlr4::tree::TerminalNode* EqualOperator(size_t i);
    std::vector<antlr4::tree::TerminalNode *> UnequalOperator();
    antlr4::tree::TerminalNode* UnequalOperator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  EqualityContext* equality();

  class  RelationalContext : public antlr4::ParserRuleContext {
  public:
    RelationalContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ShiftContext *> shift();
    ShiftContext* shift(size_t i);
    std::vector<antlr4::tree::TerminalNode *> SmallerOperator();
    antlr4::tree::TerminalNode* SmallerOperator(size_t i);
    std::vector<antlr4::tree::TerminalNode *> SmallerEqualOperator();
    antlr4::tree::TerminalNode* SmallerEqualOperator(size_t i);
    std::vector<antlr4::tree::TerminalNode *> GreaterOperator();
    antlr4::tree::TerminalNode* GreaterOperator(size_t i);
    std::vector<antlr4::tree::TerminalNode *> GreaterEqualOperator();
    antlr4::tree::TerminalNode* GreaterEqualOperator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  RelationalContext* relational();

  class  ShiftContext : public antlr4::ParserRuleContext {
  public:
    ShiftContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<AdditiveContext *> additive();
    AdditiveContext* additive(size_t i);
    std::vector<antlr4::tree::TerminalNode *> ShiftLeftOperator();
    antlr4::tree::TerminalNode* ShiftLeftOperator(size_t i);
    std::vector<antlr4::tree::TerminalNode *> ShiftRightOperator();
    antlr4::tree::TerminalNode* ShiftRightOperator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ShiftContext* shift();

  class  AdditiveContext : public antlr4::ParserRuleContext {
  public:
    AdditiveContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<MultiplicativeContext *> multiplicative();
    MultiplicativeContext* multiplicative(size_t i);
    std::vector<antlr4::tree::TerminalNode *> PlusOperator();
    antlr4::tree::TerminalNode* PlusOperator(size_t i);
    std::vector<antlr4::tree::TerminalNode *> MinusOperator();
    antlr4::tree::TerminalNode* MinusOperator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AdditiveContext* additive();

  class  MultiplicativeContext : public antlr4::ParserRuleContext {
  public:
    MultiplicativeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<CastExpressionContext *> castExpression();
    CastExpressionContext* castExpression(size_t i);
    std::vector<antlr4::tree::TerminalNode *> StarOperator();
    antlr4::tree::TerminalNode* StarOperator(size_t i);
    std::vector<antlr4::tree::TerminalNode *> DivideOperator();
    antlr4::tree::TerminalNode* DivideOperator(size_t i);
    std::vector<antlr4::tree::TerminalNode *> ModuloOperator();
    antlr4::tree::TerminalNode* ModuloOperator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  MultiplicativeContext* multiplicative();

  class  CastExpressionContext : public antlr4::ParserRuleContext {
  public:
    CastExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *OpeningRoundBracket();
    TypeIdentifierContext *typeIdentifier();
    antlr4::tree::TerminalNode *ClosingRoundBracket();
    CastExpressionContext *castExpression();
    UnaryExpressionContext *unaryExpression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  CastExpressionContext* castExpression();

  class  UnaryExpressionContext : public antlr4::ParserRuleContext {
  public:
    UnaryExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *MoveKeyword();
    antlr4::tree::TerminalNode *OpeningRoundBracket();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *ClosingRoundBracket();
    PrefixOperatorContext *prefixOperator();
    UnaryExpressionContext *unaryExpression();
    IncrementDecrementOperatorContext *incrementDecrementOperator();
    TypeSizeOrAlignContext *typeSizeOrAlign();
    TypeIdentifierContext *typeIdentifier();
    PostfixExpressionContext *postfixExpression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  UnaryExpressionContext* unaryExpression();

  class  PostfixExpressionContext : public antlr4::ParserRuleContext {
  public:
    PostfixExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    PrimaryExpressionContext *primaryExpression();
    std::vector<PostfixOperationContext *> postfixOperation();
    PostfixOperationContext* postfixOperation(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PostfixExpressionContext* postfixExpression();

  class  PrimaryExpressionContext : public antlr4::ParserRuleContext {
  public:
    PrimaryExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *BooleanLiteral();
    antlr4::tree::TerminalNode *NullReference();
    antlr4::tree::TerminalNode *ThisReference();
    antlr4::tree::TerminalNode *IntegerLiteral();
    antlr4::tree::TerminalNode *FloatingLiteral();
    antlr4::tree::TerminalNode *CharLiteral();
    StringContext *string();
    antlr4::tree::TerminalNode *Identifier();
    TupleExpressionContext *tupleExpression();
    NewExpressionContext *newExpression();
    MatchExpressionContext *matchExpression();
    ArrayLiteralContext *arrayLiteral();
    antlr4::tree::TerminalNode *OpeningRoundBracket();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *ClosingRoundBracket();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PrimaryExpressionContext* primaryExpression();

  class  ArrayLiteralContext : public antlr4::ParserRuleContext {
  public:
    ArrayLiteralContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *SquareBracketLeft();
    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    antlr4::tree::TerminalNode *SquareBracketRight();
    std::vector<antlr4::tree::TerminalNode *> CommaSeparator();
    antlr4::tree::TerminalNode* CommaSeparator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ArrayLiteralContext* arrayLiteral();

  class  PostfixOperationContext : public antlr4::ParserRuleContext {
  public:
    PostfixOperationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    CallArgumentsContext *callArguments();
    ElementAccessContext *elementAccess();
    MemberAccessContext *memberAccess();
    IncrementDecrementOperatorContext *incrementDecrementOperator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PostfixOperationContext* postfixOperation();

  class  NewExpressionContext : public antlr4::ParserRuleContext {
  public:
    NewExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *NewKeyword();
    antlr4::tree::TerminalNode *SharedKeyword();
    TypeIdentifierContext *typeIdentifier();
    CallArgumentsContext *callArguments();
    antlr4::tree::TerminalNode *SquareBracketLeft();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *SquareBracketRight();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  NewExpressionContext* newExpression();

  class  CallArgumentsContext : public antlr4::ParserRuleContext {
  public:
    CallArgumentsContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *OpeningRoundBracket();
    antlr4::tree::TerminalNode *ClosingRoundBracket();
    ArgumentListContext *argumentList();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  CallArgumentsContext* callArguments();

  class  ArgumentListContext : public antlr4::ParserRuleContext {
  public:
    ArgumentListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    std::vector<antlr4::tree::TerminalNode *> CommaSeparator();
    antlr4::tree::TerminalNode* CommaSeparator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ArgumentListContext* argumentList();

  class  ElementAccessContext : public antlr4::ParserRuleContext {
  public:
    ElementAccessContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *SquareBracketLeft();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *SquareBracketRight();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ElementAccessContext* elementAccess();

  class  MemberAccessContext : public antlr4::ParserRuleContext {
  public:
    MemberAccessContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DotOperator();
    antlr4::tree::TerminalNode *Identifier();
    antlr4::tree::TerminalNode *ReferenceAccessOperator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  MemberAccessContext* memberAccess();

  class  TupleExpressionContext : public antlr4::ParserRuleContext {
  public:
    TupleExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *OpeningRoundBracket();
    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    std::vector<antlr4::tree::TerminalNode *> CommaSeparator();
    antlr4::tree::TerminalNode* CommaSeparator(size_t i);
    antlr4::tree::TerminalNode *ClosingRoundBracket();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TupleExpressionContext* tupleExpression();

  class  TypeIdentifierContext : public antlr4::ParserRuleContext {
  public:
    TypeIdentifierContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *SharedKeyword();
    QualifiedNameContext *qualifiedName();
    std::vector<TypeModifierContext *> typeModifier();
    TypeModifierContext* typeModifier(size_t i);
    PrimitiveTypeContext *primitiveType();
    TupleTypeContext *tupleType();
    FunctionTypeContext *functionType();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TypeIdentifierContext* typeIdentifier();

  class  PrimitiveTypeContext : public antlr4::ParserRuleContext {
  public:
    PrimitiveTypeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *IntegerType();
    antlr4::tree::TerminalNode *DoubleType();
    antlr4::tree::TerminalNode *FloatType();
    antlr4::tree::TerminalNode *ByteType();
    antlr4::tree::TerminalNode *StringType();
    antlr4::tree::TerminalNode *CharType();
    antlr4::tree::TerminalNode *BoolType();
    antlr4::tree::TerminalNode *VoidType();
    antlr4::tree::TerminalNode *ShortType();
    antlr4::tree::TerminalNode *UShortType();
    antlr4::tree::TerminalNode *UIntType();
    antlr4::tree::TerminalNode *LongType();
    antlr4::tree::TerminalNode *ULongType();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PrimitiveTypeContext* primitiveType();

  class  FunctionTypeContext : public antlr4::ParserRuleContext {
  public:
    FunctionTypeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *OpeningRoundBracket();
    antlr4::tree::TerminalNode *ClosingRoundBracket();
    antlr4::tree::TerminalNode *ArrowOperator();
    ReturnTypeContext *returnType();
    TypeListContext *typeList();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  FunctionTypeContext* functionType();

  class  TypeListContext : public antlr4::ParserRuleContext {
  public:
    TypeListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<TypeIdentifierContext *> typeIdentifier();
    TypeIdentifierContext* typeIdentifier(size_t i);
    std::vector<antlr4::tree::TerminalNode *> CommaSeparator();
    antlr4::tree::TerminalNode* CommaSeparator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TypeListContext* typeList();

  class  TupleTypeContext : public antlr4::ParserRuleContext {
  public:
    TupleTypeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *OpeningRoundBracket();
    std::vector<TypeIdentifierContext *> typeIdentifier();
    TypeIdentifierContext* typeIdentifier(size_t i);
    std::vector<antlr4::tree::TerminalNode *> CommaSeparator();
    antlr4::tree::TerminalNode* CommaSeparator(size_t i);
    antlr4::tree::TerminalNode *ClosingRoundBracket();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TupleTypeContext* tupleType();

  class  TypeModifierContext : public antlr4::ParserRuleContext {
  public:
    TypeModifierContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ArrayDimensionContext *arrayDimension();
    PointerLevelContext *pointerLevel();
    RvalueReferenceLevelContext *rvalueReferenceLevel();
    ReferenceLevelContext *referenceLevel();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TypeModifierContext* typeModifier();

  class  RvalueReferenceLevelContext : public antlr4::ParserRuleContext {
  public:
    RvalueReferenceLevelContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *LogicalAndOperator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  RvalueReferenceLevelContext* rvalueReferenceLevel();

  class  ReferenceLevelContext : public antlr4::ParserRuleContext {
  public:
    ReferenceLevelContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *SingleAndOperator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ReferenceLevelContext* referenceLevel();

  class  ArrayDimensionContext : public antlr4::ParserRuleContext {
  public:
    ArrayDimensionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *SquareBracketLeft();
    antlr4::tree::TerminalNode *SquareBracketRight();
    antlr4::tree::TerminalNode *IntegerLiteral();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ArrayDimensionContext* arrayDimension();

  class  PointerLevelContext : public antlr4::ParserRuleContext {
  public:
    PointerLevelContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *StarOperator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PointerLevelContext* pointerLevel();

  class  AccessModifierContext : public antlr4::ParserRuleContext {
  public:
    AccessModifierContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeclarePublic();
    antlr4::tree::TerminalNode *DeclarePrivate();
    antlr4::tree::TerminalNode *DeclareProtected();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AccessModifierContext* accessModifier();

  class  StaticModifierContext : public antlr4::ParserRuleContext {
  public:
    StaticModifierContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeclareStatic();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  StaticModifierContext* staticModifier();

  class  AbstractModifierContext : public antlr4::ParserRuleContext {
  public:
    AbstractModifierContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DeclareAbstract();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AbstractModifierContext* abstractModifier();

  class  QualifiedNameContext : public antlr4::ParserRuleContext {
  public:
    QualifiedNameContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> Identifier();
    antlr4::tree::TerminalNode* Identifier(size_t i);
    std::vector<antlr4::tree::TerminalNode *> DotOperator();
    antlr4::tree::TerminalNode* DotOperator(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  QualifiedNameContext* qualifiedName();

  class  PrefixOperatorContext : public antlr4::ParserRuleContext {
  public:
    PrefixOperatorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *LogicalNegationOperator();
    antlr4::tree::TerminalNode *MinusOperator();
    antlr4::tree::TerminalNode *PlusOperator();
    antlr4::tree::TerminalNode *ComplimentOperator();
    antlr4::tree::TerminalNode *SingleAndOperator();
    antlr4::tree::TerminalNode *StarOperator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PrefixOperatorContext* prefixOperator();

  class  IncrementDecrementOperatorContext : public antlr4::ParserRuleContext {
  public:
    IncrementDecrementOperatorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *PlusPlusOperator();
    antlr4::tree::TerminalNode *MinusMinusOperator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  IncrementDecrementOperatorContext* incrementDecrementOperator();

  class  TypeSizeOrAlignContext : public antlr4::ParserRuleContext {
  public:
    TypeSizeOrAlignContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *SizeOfKeyword();
    antlr4::tree::TerminalNode *AlignOfKeyword();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TypeSizeOrAlignContext* typeSizeOrAlign();

  class  StringContext : public antlr4::ParserRuleContext {
  public:
    StringContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> DQUOTE();
    antlr4::tree::TerminalNode* DQUOTE(size_t i);
    std::vector<StringPartContext *> stringPart();
    StringPartContext* stringPart(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  StringContext* string();

  class  StringPartContext : public antlr4::ParserRuleContext {
  public:
    StringPartContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *TEXT();
    antlr4::tree::TerminalNode *ESCAPE_SEQUENCE();
    antlr4::tree::TerminalNode *BACKSLASH_PAREN();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *CURLY_R();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  StringPartContext* stringPart();


  // By default the static state used to implement the parser is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:
};

}  // namespace AntlrMingusParser
