
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
    NullReference = 35, ThisReference = 36, ImportDirective = 37, FromDirective = 38, 
    AsKeyword = 39, SizeOfKeyword = 40, AlignOfKeyword = 41, IntegerType = 42, 
    DoubleType = 43, FloatType = 44, ByteType = 45, StringType = 46, CharType = 47, 
    BoolType = 48, VoidType = 49, BooleanLiteral = 50, AssignOperator = 51, 
    PlusAssignOperator = 52, MinusAssignOperator = 53, MultiplyAssignOperator = 54, 
    DivideAssignOperator = 55, ModuloAssignOperator = 56, BitwiseAndAssignOperator = 57, 
    BitwiseOrAssignOperator = 58, BitwiseXorAssignOperator = 59, BitwiseLeftShiftAssignOperator = 60, 
    BitwiseRightShiftAssignOperator = 61, LogicalOrOperator = 62, LogicalAndOperator = 63, 
    UnequalOperator = 64, EqualOperator = 65, GreaterEqualOperator = 66, 
    SmallerEqualOperator = 67, GreaterOperator = 68, SmallerOperator = 69, 
    ShiftLeftOperator = 70, ShiftRightOperator = 71, PlusPlusOperator = 72, 
    MinusMinusOperator = 73, PlusOperator = 74, MinusOperator = 75, StarOperator = 76, 
    DivideOperator = 77, ModuloOperator = 78, LogicalNegationOperator = 79, 
    ComplimentOperator = 80, SingleAndOperator = 81, BitwiseXorOperator = 82, 
    BitwiseOrOperator = 83, PipeOperator = 84, ArrowOperator = 85, ReferenceAccessOperator = 86, 
    DotOperator = 87, Ellipsis = 88, DotDotOperator = 89, QuestionMarkOperator = 90, 
    ColonOperator = 91, SemicolonSeparator = 92, CommaSeparator = 93, UnderscoreWildcard = 94, 
    OpeningRoundBracket = 95, ClosingRoundBracket = 96, SquareBracketLeft = 97, 
    SquareBracketRight = 98, FloatingLiteral = 99, IntegerLiteral = 100, 
    CharLiteral = 101, Identifier = 102, BLOCK_COMMENT = 103, LINE_COMMENT = 104, 
    WS = 105, DQUOTE = 106, CURLY_L = 107, CURLY_R = 108, TEXT = 109, BACKSLASH_PAREN = 110, 
    ESCAPE_SEQUENCE = 111
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
