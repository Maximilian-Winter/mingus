
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
    DeclarePublic = 16, DeclarePrivate = 17, DeclareProtected = 18, ExternKeyword = 19, 
    RawKeyword = 20, ControlFlowIf = 21, ControlFlowElse = 22, ControlFlowSwitch = 23, 
    ControlFlowCase = 24, ControlFlowDefault = 25, ControlFlowMatch = 26, 
    FunctionReturn = 27, Break = 28, Continue = 29, DeclareVariable = 30, 
    DeclareConst = 31, NewKeyword = 32, DeleteKeyword = 33, NullReference = 34, 
    ThisReference = 35, ImportDirective = 36, FromDirective = 37, AsKeyword = 38, 
    SizeOfKeyword = 39, AlignOfKeyword = 40, IntegerType = 41, DoubleType = 42, 
    FloatType = 43, ByteType = 44, StringType = 45, CharType = 46, BoolType = 47, 
    VoidType = 48, BooleanLiteral = 49, AssignOperator = 50, PlusAssignOperator = 51, 
    MinusAssignOperator = 52, MultiplyAssignOperator = 53, DivideAssignOperator = 54, 
    ModuloAssignOperator = 55, BitwiseAndAssignOperator = 56, BitwiseOrAssignOperator = 57, 
    BitwiseXorAssignOperator = 58, BitwiseLeftShiftAssignOperator = 59, 
    BitwiseRightShiftAssignOperator = 60, LogicalOrOperator = 61, LogicalAndOperator = 62, 
    UnequalOperator = 63, EqualOperator = 64, GreaterEqualOperator = 65, 
    SmallerEqualOperator = 66, GreaterOperator = 67, SmallerOperator = 68, 
    ShiftLeftOperator = 69, ShiftRightOperator = 70, PlusPlusOperator = 71, 
    MinusMinusOperator = 72, PlusOperator = 73, MinusOperator = 74, StarOperator = 75, 
    DivideOperator = 76, ModuloOperator = 77, LogicalNegationOperator = 78, 
    ComplimentOperator = 79, SingleAndOperator = 80, BitwiseXorOperator = 81, 
    BitwiseOrOperator = 82, PipeOperator = 83, ArrowOperator = 84, ReferenceAccessOperator = 85, 
    DotOperator = 86, Ellipsis = 87, DotDotOperator = 88, QuestionMarkOperator = 89, 
    ColonOperator = 90, SemicolonSeparator = 91, CommaSeparator = 92, UnderscoreWildcard = 93, 
    OpeningRoundBracket = 94, ClosingRoundBracket = 95, SquareBracketLeft = 96, 
    SquareBracketRight = 97, FloatingLiteral = 98, IntegerLiteral = 99, 
    CharLiteral = 100, Identifier = 101, BLOCK_COMMENT = 102, LINE_COMMENT = 103, 
    WS = 104, DQUOTE = 105, CURLY_L = 106, CURLY_R = 107, TEXT = 108, BACKSLASH_PAREN = 109, 
    ESCAPE_SEQUENCE = 110
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
