
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
    DeclareStatic = 12, DeclareAbstract = 13, DeclareInterface = 14, DeclarePublic = 15, 
    DeclarePrivate = 16, DeclareProtected = 17, ExternKeyword = 18, RawKeyword = 19, 
    ControlFlowIf = 20, ControlFlowElse = 21, ControlFlowSwitch = 22, ControlFlowCase = 23, 
    ControlFlowDefault = 24, ControlFlowMatch = 25, FunctionReturn = 26, 
    Break = 27, Continue = 28, DeclareVariable = 29, DeclareConst = 30, 
    NewKeyword = 31, DeleteKeyword = 32, NullReference = 33, ThisReference = 34, 
    ImportDirective = 35, FromDirective = 36, AsKeyword = 37, SizeOfKeyword = 38, 
    AlignOfKeyword = 39, IntegerType = 40, DoubleType = 41, FloatType = 42, 
    ByteType = 43, StringType = 44, CharType = 45, BoolType = 46, VoidType = 47, 
    BooleanLiteral = 48, AssignOperator = 49, PlusAssignOperator = 50, MinusAssignOperator = 51, 
    MultiplyAssignOperator = 52, DivideAssignOperator = 53, ModuloAssignOperator = 54, 
    BitwiseAndAssignOperator = 55, BitwiseOrAssignOperator = 56, BitwiseXorAssignOperator = 57, 
    BitwiseLeftShiftAssignOperator = 58, BitwiseRightShiftAssignOperator = 59, 
    LogicalOrOperator = 60, LogicalAndOperator = 61, UnequalOperator = 62, 
    EqualOperator = 63, GreaterEqualOperator = 64, SmallerEqualOperator = 65, 
    GreaterOperator = 66, SmallerOperator = 67, ShiftLeftOperator = 68, 
    ShiftRightOperator = 69, PlusPlusOperator = 70, MinusMinusOperator = 71, 
    PlusOperator = 72, MinusOperator = 73, StarOperator = 74, DivideOperator = 75, 
    ModuloOperator = 76, LogicalNegationOperator = 77, ComplimentOperator = 78, 
    SingleAndOperator = 79, BitwiseXorOperator = 80, BitwiseOrOperator = 81, 
    PipeOperator = 82, ArrowOperator = 83, ReferenceAccessOperator = 84, 
    DotOperator = 85, Ellipsis = 86, DotDotOperator = 87, QuestionMarkOperator = 88, 
    ColonOperator = 89, SemicolonSeparator = 90, CommaSeparator = 91, UnderscoreWildcard = 92, 
    OpeningRoundBracket = 93, ClosingRoundBracket = 94, SquareBracketLeft = 95, 
    SquareBracketRight = 96, FloatingLiteral = 97, IntegerLiteral = 98, 
    CharLiteral = 99, Identifier = 100, BLOCK_COMMENT = 101, LINE_COMMENT = 102, 
    WS = 103, DQUOTE = 104, CURLY_L = 105, CURLY_R = 106, TEXT = 107, BACKSLASH_PAREN = 108, 
    ESCAPE_SEQUENCE = 109
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
