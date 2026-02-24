
// Generated from antlr4_grammar/MingusLexer.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"


namespace AntlrMingusParser {


class  MingusLexer : public antlr4::Lexer {
public:
  enum {
    DeclareModule = 1, DeclareClass = 2, DeclareStruct = 3, DeclareUnion = 4, 
    DeclareEnum = 5, DeclareFunction = 6, DeclareConstructor = 7, DeclareDestructor = 8, 
    SuperKeyword = 9, DeclareOperator = 10, DeclareForLoop = 11, DeclareWhileLoop = 12, 
    DeclareDoLoop = 13, DeclareStatic = 14, DeclareAbstract = 15, DeclareInterface = 16, 
    DeclareTypedef = 17, DeclarePublic = 18, DeclarePrivate = 19, DeclareProtected = 20, 
    ExternKeyword = 21, RawKeyword = 22, LinkKeyword = 23, OpaqueKeyword = 24, 
    ControlFlowIf = 25, ControlFlowElse = 26, ControlFlowSwitch = 27, ControlFlowCase = 28, 
    ControlFlowDefault = 29, ControlFlowMatch = 30, FunctionReturn = 31, 
    Break = 32, Continue = 33, DeclareVariable = 34, DeclareConst = 35, 
    NewKeyword = 36, DeleteKeyword = 37, NullReference = 38, ThisReference = 39, 
    MoveKeyword = 40, WeakKeyword = 41, SharedKeyword = 42, ImportDirective = 43, 
    FromDirective = 44, AsKeyword = 45, SizeOfKeyword = 46, AlignOfKeyword = 47, 
    IntegerType = 48, DoubleType = 49, FloatType = 50, ByteType = 51, StringType = 52, 
    CharType = 53, BoolType = 54, VoidType = 55, ShortType = 56, UShortType = 57, 
    UIntType = 58, LongType = 59, ULongType = 60, BooleanLiteral = 61, AssignOperator = 62, 
    PlusAssignOperator = 63, MinusAssignOperator = 64, MultiplyAssignOperator = 65, 
    DivideAssignOperator = 66, ModuloAssignOperator = 67, BitwiseAndAssignOperator = 68, 
    BitwiseOrAssignOperator = 69, BitwiseXorAssignOperator = 70, BitwiseLeftShiftAssignOperator = 71, 
    BitwiseRightShiftAssignOperator = 72, LogicalOrOperator = 73, LogicalAndOperator = 74, 
    UnequalOperator = 75, EqualOperator = 76, GreaterEqualOperator = 77, 
    SmallerEqualOperator = 78, GreaterOperator = 79, SmallerOperator = 80, 
    ShiftLeftOperator = 81, ShiftRightOperator = 82, PlusPlusOperator = 83, 
    MinusMinusOperator = 84, PlusOperator = 85, MinusOperator = 86, StarOperator = 87, 
    DivideOperator = 88, ModuloOperator = 89, LogicalNegationOperator = 90, 
    ComplimentOperator = 91, SingleAndOperator = 92, BitwiseXorOperator = 93, 
    BitwiseOrOperator = 94, PipeOperator = 95, ArrowOperator = 96, ReferenceAccessOperator = 97, 
    DotOperator = 98, Ellipsis = 99, DotDotOperator = 100, QuestionMarkOperator = 101, 
    ColonOperator = 102, SemicolonSeparator = 103, CommaSeparator = 104, 
    UnderscoreWildcard = 105, OpeningRoundBracket = 106, ClosingRoundBracket = 107, 
    SquareBracketLeft = 108, SquareBracketRight = 109, FloatingLiteral = 110, 
    IntegerLiteral = 111, CharLiteral = 112, Identifier = 113, BLOCK_COMMENT = 114, 
    LINE_COMMENT = 115, WS = 116, DQUOTE = 117, CURLY_L = 118, CURLY_R = 119, 
    TEXT = 120, BACKSLASH_PAREN = 121, ESCAPE_SEQUENCE = 122
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
