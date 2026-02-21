
// Generated from antlr4_grammar/MingusLexer.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"


namespace AntlrMingusParser {


class  MingusLexer : public antlr4::Lexer {
public:
  enum {
    DeclareModule = 1, DeclareClass = 2, DeclareStruct = 3, DeclareEnum = 4, 
    DeclareFunction = 5, DeclareConstructor = 6, DeclareDestructor = 7, 
    SuperKeyword = 8, DeclareOperator = 9, DeclareForLoop = 10, DeclareWhileLoop = 11, 
    DeclareDoLoop = 12, DeclareStatic = 13, DeclareAbstract = 14, DeclareInterface = 15, 
    DeclareTypedef = 16, DeclarePublic = 17, DeclarePrivate = 18, DeclareProtected = 19, 
    ExternKeyword = 20, RawKeyword = 21, ControlFlowIf = 22, ControlFlowElse = 23, 
    ControlFlowSwitch = 24, ControlFlowCase = 25, ControlFlowDefault = 26, 
    ControlFlowMatch = 27, FunctionReturn = 28, Break = 29, Continue = 30, 
    DeclareVariable = 31, DeclareConst = 32, NewKeyword = 33, DeleteKeyword = 34, 
    NullReference = 35, ThisReference = 36, MoveKeyword = 37, ImportDirective = 38, 
    FromDirective = 39, AsKeyword = 40, SizeOfKeyword = 41, AlignOfKeyword = 42, 
    IntegerType = 43, DoubleType = 44, FloatType = 45, ByteType = 46, StringType = 47, 
    CharType = 48, BoolType = 49, VoidType = 50, BooleanLiteral = 51, AssignOperator = 52, 
    PlusAssignOperator = 53, MinusAssignOperator = 54, MultiplyAssignOperator = 55, 
    DivideAssignOperator = 56, ModuloAssignOperator = 57, BitwiseAndAssignOperator = 58, 
    BitwiseOrAssignOperator = 59, BitwiseXorAssignOperator = 60, BitwiseLeftShiftAssignOperator = 61, 
    BitwiseRightShiftAssignOperator = 62, LogicalOrOperator = 63, LogicalAndOperator = 64, 
    UnequalOperator = 65, EqualOperator = 66, GreaterEqualOperator = 67, 
    SmallerEqualOperator = 68, GreaterOperator = 69, SmallerOperator = 70, 
    ShiftLeftOperator = 71, ShiftRightOperator = 72, PlusPlusOperator = 73, 
    MinusMinusOperator = 74, PlusOperator = 75, MinusOperator = 76, StarOperator = 77, 
    DivideOperator = 78, ModuloOperator = 79, LogicalNegationOperator = 80, 
    ComplimentOperator = 81, SingleAndOperator = 82, BitwiseXorOperator = 83, 
    BitwiseOrOperator = 84, PipeOperator = 85, ArrowOperator = 86, ReferenceAccessOperator = 87, 
    DotOperator = 88, Ellipsis = 89, DotDotOperator = 90, QuestionMarkOperator = 91, 
    ColonOperator = 92, SemicolonSeparator = 93, CommaSeparator = 94, UnderscoreWildcard = 95, 
    OpeningRoundBracket = 96, ClosingRoundBracket = 97, SquareBracketLeft = 98, 
    SquareBracketRight = 99, FloatingLiteral = 100, IntegerLiteral = 101, 
    CharLiteral = 102, Identifier = 103, BLOCK_COMMENT = 104, LINE_COMMENT = 105, 
    WS = 106, DQUOTE = 107, CURLY_L = 108, CURLY_R = 109, TEXT = 110, BACKSLASH_PAREN = 111, 
    ESCAPE_SEQUENCE = 112
  };

  enum {
    IN_STRING = 1
  };

  explicit MingusLexer(antlr4::CharStream *input);

  ~MingusLexer() override;


  std::string getGrammarFileName() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const std::vector<std::string>& getChannelNames() const override;

  const std::vector<std::string>& getModeNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  const antlr4::atn::ATN& getATN() const override;

  // By default the static state used to implement the lexer is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:

  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

};

}  // namespace AntlrMingusParser
