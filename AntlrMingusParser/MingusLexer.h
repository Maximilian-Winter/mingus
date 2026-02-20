
// Generated from ./antlr4_grammar/MingusLexer.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"


namespace AntlrMingusParser {


class  MingusLexer : public antlr4::Lexer {
public:
  enum {
    DeclareModule = 1, DeclareClass = 2, DeclareStruct = 3, DeclareEnum = 4, 
    DeclareFunction = 5, DeclareConstructor = 6, DeclareDestructor = 7, 
    SuperKeyword = 8, DeclareOperator = 9, DeclareForLoop = 10, DeclareWhileLoop = 11, 
    DeclareStatic = 12, DeclareAbstract = 13, DeclareInterface = 14, DeclarePublic = 15, 
    DeclarePrivate = 16, DeclareProtected = 17, ExternKeyword = 18, RawKeyword = 19, 
    ControlFlowIf = 20, ControlFlowElse = 21, ControlFlowSwitch = 22, ControlFlowCase = 23, 
    ControlFlowDefault = 24, ControlFlowMatch = 25, FunctionReturn = 26, 
    Break = 27, Continue = 28, DeclareVariable = 29, NewKeyword = 30, DeleteKeyword = 31, 
    NullReference = 32, ThisReference = 33, ImportDirective = 34, FromDirective = 35, 
    AsKeyword = 36, SizeOfKeyword = 37, AlignOfKeyword = 38, IntegerType = 39, 
    DoubleType = 40, FloatType = 41, ByteType = 42, StringType = 43, CharType = 44, 
    BoolType = 45, VoidType = 46, BooleanLiteral = 47, AssignOperator = 48, 
    PlusAssignOperator = 49, MinusAssignOperator = 50, MultiplyAssignOperator = 51, 
    DivideAssignOperator = 52, ModuloAssignOperator = 53, BitwiseAndAssignOperator = 54, 
    BitwiseOrAssignOperator = 55, BitwiseXorAssignOperator = 56, BitwiseLeftShiftAssignOperator = 57, 
    BitwiseRightShiftAssignOperator = 58, LogicalOrOperator = 59, LogicalAndOperator = 60, 
    UnequalOperator = 61, EqualOperator = 62, GreaterEqualOperator = 63, 
    SmallerEqualOperator = 64, GreaterOperator = 65, SmallerOperator = 66, 
    ShiftLeftOperator = 67, ShiftRightOperator = 68, PlusPlusOperator = 69, 
    MinusMinusOperator = 70, PlusOperator = 71, MinusOperator = 72, StarOperator = 73, 
    DivideOperator = 74, ModuloOperator = 75, LogicalNegationOperator = 76, 
    ComplimentOperator = 77, SingleAndOperator = 78, BitwiseXorOperator = 79, 
    BitwiseOrOperator = 80, PipeOperator = 81, ArrowOperator = 82, ReferenceAccessOperator = 83, 
    DotOperator = 84, DotDotOperator = 85, QuestionMarkOperator = 86, ColonOperator = 87, 
    SemicolonSeparator = 88, CommaSeparator = 89, UnderscoreWildcard = 90, 
    OpeningRoundBracket = 91, ClosingRoundBracket = 92, SquareBracketLeft = 93, 
    SquareBracketRight = 94, FloatingLiteral = 95, IntegerLiteral = 96, 
    CharLiteral = 97, Identifier = 98, BLOCK_COMMENT = 99, LINE_COMMENT = 100, 
    WS = 101, DQUOTE = 102, CURLY_L = 103, CURLY_R = 104, TEXT = 105, BACKSLASH_PAREN = 106, 
    ESCAPE_SEQUENCE = 107
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
