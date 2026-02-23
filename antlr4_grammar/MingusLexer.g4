/*
  Mingus Lexer Grammar — v1
  A statically typed programming language in the spirit of C++, tamed.
  Named after Charles Mingus — fierce, precise, uncompromising.

  Core philosophy:
    - Pipes for composition
    - Raw blocks for honest danger
    - Pattern matching for exhaustive logic
    - RAII for resource safety
    - Extern C for real-world interop
    - No runtime, no garbage collector, pure LLVM native code
*/

lexer grammar MingusLexer;

// ============================================================================
// Keywords — Language Structure
// ============================================================================

DeclareModule:       'module';
DeclareClass:        'class';
DeclareStruct:       'struct';
DeclareEnum:         'enum';
DeclareFunction:     'func';
DeclareConstructor:  'constructor';
DeclareDestructor:   'destructor';
SuperKeyword:        'super';
DeclareOperator:     'operator';
DeclareForLoop:      'for';
DeclareWhileLoop:    'while';
DeclareDoLoop:       'do';
DeclareStatic:       'static';
DeclareAbstract:     'abstract';
DeclareInterface:    'interface';
DeclareTypedef:      'typedef';
DeclarePublic:       'public';
DeclarePrivate:      'private';
DeclareProtected:    'protected';
ExternKeyword:       'extern';
RawKeyword:          'raw';
LinkKeyword:         'link';
OpaqueKeyword:       'opaque';

// ============================================================================
// Keywords — Control Flow
// ============================================================================

ControlFlowIf:       'if';
ControlFlowElse:     'else';
ControlFlowSwitch:   'switch';
ControlFlowCase:     'case';
ControlFlowDefault:  'default';
ControlFlowMatch:    'match';
FunctionReturn:      'return';
Break:               'break';
Continue:            'continue';

// ============================================================================
// Keywords — Memory & References
// ============================================================================

DeclareVariable:     'var';
DeclareConst:        'const';
NewKeyword:          'new';
DeleteKeyword:       'delete';
NullReference:       'null';
ThisReference:       'this';
MoveKeyword:         'move';
WeakKeyword:         'weak';
SharedKeyword:       'shared';

// ============================================================================
// Keywords — Module System
// ============================================================================

ImportDirective:      'import';
FromDirective:        'from';
AsKeyword:            'as';

// ============================================================================
// Keywords — Compile-time Operators
// ============================================================================

SizeOfKeyword:        'sizeof';
AlignOfKeyword:       'alignof';

// ============================================================================
// Built-in Type Keywords
// ============================================================================

IntegerType:          'int';
DoubleType:           'double';
FloatType:            'float';
ByteType:             'byte';
StringType:           'string';
CharType:             'char';
BoolType:             'bool';
VoidType:             'void';

// ============================================================================
// Boolean Literals
// ============================================================================

BooleanLiteral: 'false' | 'true';

// ============================================================================
// Assignment Operators
// ============================================================================

AssignOperator:                  '=';
PlusAssignOperator:              '+=';
MinusAssignOperator:             '-=';
MultiplyAssignOperator:          '*=';
DivideAssignOperator:            '/=';
ModuloAssignOperator:            '%=';
BitwiseAndAssignOperator:        '&=';
BitwiseOrAssignOperator:         '|=';
BitwiseXorAssignOperator:        '^=';
BitwiseLeftShiftAssignOperator:  '<<=';
BitwiseRightShiftAssignOperator: '>>=';

// ============================================================================
// Logical Operators
// ============================================================================

LogicalOrOperator:   '||';
LogicalAndOperator:  '&&';

// ============================================================================
// Comparison Operators
// ============================================================================

UnequalOperator:      '!=';
EqualOperator:        '==';
GreaterEqualOperator: '>=';
SmallerEqualOperator: '<=';
GreaterOperator:      '>';
SmallerOperator:      '<';

// ============================================================================
// Shift Operators
// ============================================================================

ShiftLeftOperator:    '<<';
ShiftRightOperator:   '>>';

// ============================================================================
// Arithmetic & Unary Operators
// ============================================================================

PlusPlusOperator:     '++';
MinusMinusOperator:   '--';
PlusOperator:         '+';
MinusOperator:        '-';
StarOperator:         '*';
DivideOperator:       '/';
ModuloOperator:       '%';

// ============================================================================
// Bitwise & Logical Unary
// ============================================================================

LogicalNegationOperator: '!';
ComplimentOperator:      '~';
SingleAndOperator:       '&';
BitwiseXorOperator:      '^';
BitwiseOrOperator:       '|';

// ============================================================================
// Punctuation & Delimiters
// ============================================================================

PipeOperator:           '|>';
ArrowOperator:          '=>';
ReferenceAccessOperator: '->';
DotOperator:            '.';
Ellipsis:               '...';
DotDotOperator:         '..';
QuestionMarkOperator:   '?';
ColonOperator:          ':';
SemicolonSeparator:     ';';
CommaSeparator:         ',';
UnderscoreWildcard:     '_';

OpeningRoundBracket:    '(';
ClosingRoundBracket:    ')';
SquareBracketLeft:      '[';
SquareBracketRight:     ']';

// ============================================================================
// Numeric Literals
// ============================================================================

FloatingLiteral:
    Fractionalconstant Exponentpart? [fF]?
    | Digitsequence Exponentpart [fF]?
    ;

IntegerLiteral:
    HexLiteral
    | BinaryLiteral
    | OctalLiteral
    | DecimalLiteral
    ;

fragment HexLiteral:      '0' [xX] [0-9a-fA-F]+;
fragment BinaryLiteral:   '0' [bB] [01]+;
fragment OctalLiteral:    '0' [oO] [0-7]+;
fragment DecimalLiteral:  DIGIT+;
fragment DIGIT:           [0-9];

fragment Fractionalconstant:
    Digitsequence? '.' Digitsequence
    | Digitsequence '.'
    ;

fragment Exponentpart:
    [eE] SIGN? Digitsequence
    ;

fragment SIGN: [+-];
fragment Digitsequence: DIGIT ('\''? DIGIT)*;

// ============================================================================
// Character Literal
// ============================================================================

CharLiteral: '\'' ( CHAR_ESCAPE | ~['\\\r\n] ) '\'';
fragment CHAR_ESCAPE: '\\' [nrtfb0'\\];

// ============================================================================
// Identifiers
// ============================================================================

Identifier: [a-zA-Z_][a-zA-Z0-9_]*;

// ============================================================================
// Comments & Whitespace
// ============================================================================

BLOCK_COMMENT: '/*' .*? '*/'    -> skip;
LINE_COMMENT:  '//' ~[\r\n]*    -> skip;
WS:            [ \r\t\n\u000C]+ -> skip;

// ============================================================================
// String Handling — Mode-based for interpolation support
// ============================================================================

DQUOTE:  '"' -> pushMode(IN_STRING);
CURLY_L: '{' -> pushMode(DEFAULT_MODE);
CURLY_R: '}' -> popMode;

// ============================================================================
// String Interpolation Mode
// ============================================================================

mode IN_STRING;

TEXT:             ~[$\\"]+;
BACKSLASH_PAREN:  '${' -> pushMode(DEFAULT_MODE);
ESCAPE_SEQUENCE:  '\\' .;
DQUOTE_IN_STRING: '"' -> type(DQUOTE), popMode;
