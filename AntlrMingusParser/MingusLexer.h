
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
    NullReference = 35, ThisReference = 36, MoveKeyword = 37, WeakKeyword = 38, 
    SharedKeyword = 39, ImportDirective = 40, FromDirective = 41, AsKeyword = 42, 
    SizeOfKeyword = 43, AlignOfKeyword = 44, IntegerType = 45, DoubleType = 46, 
    FloatType = 47, ByteType = 48, StringType = 49, CharType = 50, BoolType = 51, 
    VoidType = 52, BooleanLiteral = 53, AssignOperator = 54, PlusAssignOperator = 55, 
    MinusAssignOperator = 56, MultiplyAssignOperator = 57, DivideAssignOperator = 58, 
    ModuloAssignOperator = 59, BitwiseAndAssignOperator = 60, BitwiseOrAssignOperator = 61, 
    BitwiseXorAssignOperator = 62, BitwiseLeftShiftAssignOperator = 63, 
    BitwiseRightShiftAssignOperator = 64, LogicalOrOperator = 65, LogicalAndOperator = 66, 
    UnequalOperator = 67, EqualOperator = 68, GreaterEqualOperator = 69, 
    SmallerEqualOperator = 70, GreaterOperator = 71, SmallerOperator = 72, 
    ShiftLeftOperator = 73, ShiftRightOperator = 74, PlusPlusOperator = 75, 
    MinusMinusOperator = 76, PlusOperator = 77, MinusOperator = 78, StarOperator = 79, 
    DivideOperator = 80, ModuloOperator = 81, LogicalNegationOperator = 82, 
    ComplimentOperator = 83, SingleAndOperator = 84, BitwiseXorOperator = 85, 
    BitwiseOrOperator = 86, PipeOperator = 87, ArrowOperator = 88, ReferenceAccessOperator = 89, 
    DotOperator = 90, Ellipsis = 91, DotDotOperator = 92, QuestionMarkOperator = 93, 
    ColonOperator = 94, SemicolonSeparator = 95, CommaSeparator = 96, UnderscoreWildcard = 97, 
    OpeningRoundBracket = 98, ClosingRoundBracket = 99, SquareBracketLeft = 100, 
    SquareBracketRight = 101, FloatingLiteral = 102, IntegerLiteral = 103, 
    CharLiteral = 104, Identifier = 105, BLOCK_COMMENT = 106, LINE_COMMENT = 107, 
    WS = 108, DQUOTE = 109, CURLY_L = 110, CURLY_R = 111, TEXT = 112, BACKSLASH_PAREN = 113, 
    ESCAPE_SEQUENCE = 114
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
