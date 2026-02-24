
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
