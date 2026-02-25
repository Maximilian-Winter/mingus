
// Generated from antlr4_grammar/MingusLexer.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"


namespace AntlrMingusParser {


class  MingusLexer : public antlr4::Lexer {
public:
  enum {
    DeclareModule = 1, DeclareClass = 2, DeclareStruct = 3, DeclareUnion = 4, 
    TaggedKeyword = 5, DeclareEnum = 6, DeclareFunction = 7, DeclareConstructor = 8, 
    DeclareDestructor = 9, SuperKeyword = 10, DeclareOperator = 11, DeclareForLoop = 12, 
    DeclareWhileLoop = 13, DeclareDoLoop = 14, DeclareStatic = 15, DeclareAbstract = 16, 
    DeclareInterface = 17, DeclareTypedef = 18, DeclarePublic = 19, DeclarePrivate = 20, 
    DeclareProtected = 21, ExternKeyword = 22, RawKeyword = 23, LinkKeyword = 24, 
    OpaqueKeyword = 25, ControlFlowIf = 26, ControlFlowElse = 27, ControlFlowSwitch = 28, 
    ControlFlowCase = 29, ControlFlowDefault = 30, ControlFlowMatch = 31, 
    FunctionReturn = 32, Break = 33, Continue = 34, DeclareVariable = 35, 
    DeclareConst = 36, NewKeyword = 37, DeleteKeyword = 38, NullReference = 39, 
    ThisReference = 40, MoveKeyword = 41, WeakKeyword = 42, SharedKeyword = 43, 
    ImportDirective = 44, FromDirective = 45, AsKeyword = 46, SizeOfKeyword = 47, 
    AlignOfKeyword = 48, IntegerType = 49, DoubleType = 50, FloatType = 51, 
    ByteType = 52, StringType = 53, CharType = 54, BoolType = 55, VoidType = 56, 
    ShortType = 57, UShortType = 58, UIntType = 59, LongType = 60, ULongType = 61, 
    BooleanLiteral = 62, AssignOperator = 63, PlusAssignOperator = 64, MinusAssignOperator = 65, 
    MultiplyAssignOperator = 66, DivideAssignOperator = 67, ModuloAssignOperator = 68, 
    BitwiseAndAssignOperator = 69, BitwiseOrAssignOperator = 70, BitwiseXorAssignOperator = 71, 
    BitwiseLeftShiftAssignOperator = 72, BitwiseRightShiftAssignOperator = 73, 
    LogicalOrOperator = 74, LogicalAndOperator = 75, UnequalOperator = 76, 
    EqualOperator = 77, GreaterEqualOperator = 78, SmallerEqualOperator = 79, 
    GreaterOperator = 80, SmallerOperator = 81, ShiftLeftOperator = 82, 
    ShiftRightOperator = 83, PlusPlusOperator = 84, MinusMinusOperator = 85, 
    PlusOperator = 86, MinusOperator = 87, StarOperator = 88, DivideOperator = 89, 
    ModuloOperator = 90, LogicalNegationOperator = 91, ComplimentOperator = 92, 
    SingleAndOperator = 93, BitwiseXorOperator = 94, BitwiseOrOperator = 95, 
    PipeOperator = 96, ArrowOperator = 97, ReferenceAccessOperator = 98, 
    DotOperator = 99, Ellipsis = 100, DotDotOperator = 101, QuestionMarkOperator = 102, 
    ColonOperator = 103, SemicolonSeparator = 104, CommaSeparator = 105, 
    UnderscoreWildcard = 106, AtSign = 107, OpeningRoundBracket = 108, ClosingRoundBracket = 109, 
    SquareBracketLeft = 110, SquareBracketRight = 111, FloatingLiteral = 112, 
    IntegerLiteral = 113, CharLiteral = 114, Identifier = 115, BLOCK_COMMENT = 116, 
    LINE_COMMENT = 117, WS = 118, DQUOTE = 119, CURLY_L = 120, CURLY_R = 121, 
    TEXT = 122, BACKSLASH_PAREN = 123, ESCAPE_SEQUENCE = 124
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
