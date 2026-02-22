
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
    ImportDirective = 39, FromDirective = 40, AsKeyword = 41, SizeOfKeyword = 42, 
    AlignOfKeyword = 43, IntegerType = 44, DoubleType = 45, FloatType = 46, 
    ByteType = 47, StringType = 48, CharType = 49, BoolType = 50, VoidType = 51, 
    BooleanLiteral = 52, AssignOperator = 53, PlusAssignOperator = 54, MinusAssignOperator = 55, 
    MultiplyAssignOperator = 56, DivideAssignOperator = 57, ModuloAssignOperator = 58, 
    BitwiseAndAssignOperator = 59, BitwiseOrAssignOperator = 60, BitwiseXorAssignOperator = 61, 
    BitwiseLeftShiftAssignOperator = 62, BitwiseRightShiftAssignOperator = 63, 
    LogicalOrOperator = 64, LogicalAndOperator = 65, UnequalOperator = 66, 
    EqualOperator = 67, GreaterEqualOperator = 68, SmallerEqualOperator = 69, 
    GreaterOperator = 70, SmallerOperator = 71, ShiftLeftOperator = 72, 
    ShiftRightOperator = 73, PlusPlusOperator = 74, MinusMinusOperator = 75, 
    PlusOperator = 76, MinusOperator = 77, StarOperator = 78, DivideOperator = 79, 
    ModuloOperator = 80, LogicalNegationOperator = 81, ComplimentOperator = 82, 
    SingleAndOperator = 83, BitwiseXorOperator = 84, BitwiseOrOperator = 85, 
    PipeOperator = 86, ArrowOperator = 87, ReferenceAccessOperator = 88, 
    DotOperator = 89, Ellipsis = 90, DotDotOperator = 91, QuestionMarkOperator = 92, 
    ColonOperator = 93, SemicolonSeparator = 94, CommaSeparator = 95, UnderscoreWildcard = 96, 
    OpeningRoundBracket = 97, ClosingRoundBracket = 98, SquareBracketLeft = 99, 
    SquareBracketRight = 100, FloatingLiteral = 101, IntegerLiteral = 102, 
    CharLiteral = 103, Identifier = 104, BLOCK_COMMENT = 105, LINE_COMMENT = 106, 
    WS = 107, DQUOTE = 108, CURLY_L = 109, CURLY_R = 110, TEXT = 111, BACKSLASH_PAREN = 112, 
    ESCAPE_SEQUENCE = 113
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
