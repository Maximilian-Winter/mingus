
// Generated from MingusLexer.g4 by ANTLR 4.13.2

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
    ExternKeyword = 20, RawKeyword = 21, LinkKeyword = 22, OpaqueKeyword = 23, 
    ControlFlowIf = 24, ControlFlowElse = 25, ControlFlowSwitch = 26, ControlFlowCase = 27, 
    ControlFlowDefault = 28, ControlFlowMatch = 29, FunctionReturn = 30, 
    Break = 31, Continue = 32, DeclareVariable = 33, DeclareConst = 34, 
    NewKeyword = 35, DeleteKeyword = 36, NullReference = 37, ThisReference = 38, 
    MoveKeyword = 39, WeakKeyword = 40, SharedKeyword = 41, ImportDirective = 42, 
    FromDirective = 43, AsKeyword = 44, SizeOfKeyword = 45, AlignOfKeyword = 46, 
    IntegerType = 47, DoubleType = 48, FloatType = 49, ByteType = 50, StringType = 51, 
    CharType = 52, BoolType = 53, VoidType = 54, BooleanLiteral = 55, AssignOperator = 56, 
    PlusAssignOperator = 57, MinusAssignOperator = 58, MultiplyAssignOperator = 59, 
    DivideAssignOperator = 60, ModuloAssignOperator = 61, BitwiseAndAssignOperator = 62, 
    BitwiseOrAssignOperator = 63, BitwiseXorAssignOperator = 64, BitwiseLeftShiftAssignOperator = 65, 
    BitwiseRightShiftAssignOperator = 66, LogicalOrOperator = 67, LogicalAndOperator = 68, 
    UnequalOperator = 69, EqualOperator = 70, GreaterEqualOperator = 71, 
    SmallerEqualOperator = 72, GreaterOperator = 73, SmallerOperator = 74, 
    ShiftLeftOperator = 75, ShiftRightOperator = 76, PlusPlusOperator = 77, 
    MinusMinusOperator = 78, PlusOperator = 79, MinusOperator = 80, StarOperator = 81, 
    DivideOperator = 82, ModuloOperator = 83, LogicalNegationOperator = 84, 
    ComplimentOperator = 85, SingleAndOperator = 86, BitwiseXorOperator = 87, 
    BitwiseOrOperator = 88, PipeOperator = 89, ArrowOperator = 90, ReferenceAccessOperator = 91, 
    DotOperator = 92, Ellipsis = 93, DotDotOperator = 94, QuestionMarkOperator = 95, 
    ColonOperator = 96, SemicolonSeparator = 97, CommaSeparator = 98, UnderscoreWildcard = 99, 
    OpeningRoundBracket = 100, ClosingRoundBracket = 101, SquareBracketLeft = 102, 
    SquareBracketRight = 103, FloatingLiteral = 104, IntegerLiteral = 105, 
    CharLiteral = 106, Identifier = 107, BLOCK_COMMENT = 108, LINE_COMMENT = 109, 
    WS = 110, DQUOTE = 111, CURLY_L = 112, CURLY_R = 113, TEXT = 114, BACKSLASH_PAREN = 115, 
    ESCAPE_SEQUENCE = 116
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
