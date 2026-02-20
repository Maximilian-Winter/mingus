/*
  Mingus Parser Grammar — v1
  A statically typed programming language in the spirit of C++, tamed.
  Named after Charles Mingus — fierce, precise, uncompromising.

  Features:
    Modules                         module Name { }
    Classes & Inheritance            class Name : Base { }
    Structs                         struct Name { }
    Enums                           enum Name : int { }
    Abstract & Static modifiers     abstract class, static func
    Access modifiers                public, private, protected
    Constructors & Destructors      constructor(), destructor
    Functions                       func name(params) => ReturnType { }
    Operator Overloading            func operator+(T other) => T { }
    Lambdas & Closures              (int x) => { return x * 2; }
    First-class Function Types      (int, int) => double
    Type Inference                  var x = 42;
    Tuples & Destructuring          (int, string), (var a, var b) = foo()
    Pattern Matching                match expr { pattern => body, }
    Switch Statements               switch (expr) { case val: ... }
    Pipe Operator                   x |> f |> g
    Raw Blocks                      raw { unsafe_code(); }
    Extern Declarations             extern func printf(string fmt) => int;
    Memory Management               new, delete
    Arrays                          int[], double[16]
    Pointers                        int*, byte*
    String Interpolation            "hello ${name}"
    Full C#-style Operator Precedence

  Operator Precedence (highest to lowest):
    Postfix              ++, --
    Prefix/Unary         ++, --, +, -, !, ~, &, *, sizeof, alignof
    Cast                 (type)expr
    Multiplicative       *, /, %
    Additive             +, -
    Shift                <<, >>
    Relational           <, <=, >, >=
    Equality             ==, !=
    Bitwise AND          &
    Bitwise XOR          ^
    Bitwise OR           |
    Logical AND          &&
    Logical OR           ||
    Ternary              ?:
    Pipe                 |>
    Assignment           =, +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=
*/

parser grammar MingusParser;

options
{
    tokenVocab = 'MingusLexer';
}

// ============================================================================
// Program Structure
// ============================================================================

program
    : module* EOF
    ;

module
    : DeclareModule Identifier moduleBlock
    ;

moduleBlock
    : CURLY_L moduleDeclaration* CURLY_R
    ;

moduleDeclaration
    : classDeclaration
    | structDeclaration
    | enumDeclaration
    | interfaceDeclaration
    | functionDeclaration
    | externDeclaration
    | variableDeclaration
    | importDefinition
    ;

// ============================================================================
// Import System
// ============================================================================

importDefinition
    : ImportDirective importTarget ( CommaSeparator importTarget )*
      ( FromDirective qualifiedName )? SemicolonSeparator
    ;

importTarget
    : Identifier ( AsKeyword Identifier )?
    ;

// ============================================================================
// Extern Declarations — C Interop
// ============================================================================

externDeclaration
    : ExternKeyword externBody
    ;

externBody
    : externFunctionDeclaration
    | CURLY_L externFunctionDeclaration* CURLY_R
    ;

externFunctionDeclaration
    : DeclareFunction Identifier definitionParameters ArrowOperator returnType SemicolonSeparator
    ;

// ============================================================================
// Class System
// ============================================================================

classDeclaration
    : accessModifier? ( staticModifier | abstractModifier )?
      DeclareClass Identifier ( ColonOperator inheritance )?
      ( classBlock | SemicolonSeparator )
    ;

classBlock
    : CURLY_L classMember* CURLY_R
    ;

classMember
    : constructorDeclaration
    | destructorDeclaration
    | operatorDeclaration
    | functionDeclaration
    | variableDeclaration
    ;

constructorDeclaration
    : accessModifier?
      DeclareConstructor definitionParameters
      ( ColonOperator SuperKeyword callArguments )?
      block
    ;

destructorDeclaration
    : DeclareDestructor block
    ;

operatorDeclaration
    : accessModifier?
      DeclareFunction DeclareOperator overloadableOperator
      definitionParameters ArrowOperator returnType
      ( block | ArrowOperator expression SemicolonSeparator )
    ;

overloadableOperator
    : PlusOperator
    | MinusOperator
    | StarOperator
    | DivideOperator
    | ModuloOperator
    | EqualOperator
    | UnequalOperator
    | SmallerOperator
    | SmallerEqualOperator
    | GreaterOperator
    | GreaterEqualOperator
    | SquareBracketLeft SquareBracketRight
    ;

inheritance
    : qualifiedName ( CommaSeparator qualifiedName )*
    ;

// ============================================================================
// Interface — Method contracts, Go-style fat pointer dispatch
// ============================================================================

interfaceDeclaration
    : accessModifier? DeclareInterface Identifier
      ( interfaceBlock | SemicolonSeparator )
    ;

interfaceBlock
    : CURLY_L interfaceMember* CURLY_R
    ;

interfaceMember
    : functionDeclaration
    ;

// ============================================================================
// Struct — Value types with methods, no inheritance
// ============================================================================

structDeclaration
    : accessModifier?
      DeclareStruct Identifier ( structBlock | SemicolonSeparator )
    ;

structBlock
    : CURLY_L structMember* CURLY_R
    ;

structMember
    : operatorDeclaration
    | functionDeclaration
    | variableDeclaration
    ;

// ============================================================================
// Enum
// ============================================================================

enumDeclaration
    : accessModifier?
      DeclareEnum Identifier ( ColonOperator typeIdentifier )?
      CURLY_L enumMember ( CommaSeparator enumMember )* CommaSeparator? CURLY_R
    ;

enumMember
    : Identifier ( AssignOperator expression )?
    ;

// ============================================================================
// Functions
// ============================================================================

functionDeclaration
    : accessModifier? ( staticModifier | abstractModifier )?
      DeclareFunction Identifier definitionParameters ArrowOperator returnType
      ( block | ArrowOperator expression SemicolonSeparator | SemicolonSeparator )
    ;

returnType
    : typeIdentifier
    | tupleType
    ;

definitionParameters
    : OpeningRoundBracket parameterList? ClosingRoundBracket
    ;

parameterList
    : parameter ( CommaSeparator parameter )*
    ;

parameter
    : typeIdentifier Identifier ( AssignOperator expression )?
    ;

// ============================================================================
// Variable Declarations
// ============================================================================

variableDeclaration
    : accessModifier? staticModifier? typedVariableDeclaration
    | accessModifier? staticModifier? inferredVariableDeclaration
    | accessModifier? staticModifier? tupleDestructuring
    ;

typedVariableDeclaration
    : typeIdentifier Identifier
      ( SemicolonSeparator
      | AssignOperator exprStatement
      | callArguments SemicolonSeparator
      )
    ;

inferredVariableDeclaration
    : DeclareVariable Identifier AssignOperator exprStatement
    ;

tupleDestructuring
    : OpeningRoundBracket tupleDestructureElement
      ( CommaSeparator tupleDestructureElement )+ ClosingRoundBracket
      AssignOperator exprStatement
    ;

tupleDestructureElement
    : typeIdentifier Identifier
    | DeclareVariable Identifier
    ;

// ============================================================================
// Statements
// ============================================================================

statement
    : exprStatement
    | variableDeclaration
    | forStatement
    | whileStatement
    | ifStatement
    | switchStatement
    | matchStatement
    | returnStatement
    | breakStatement
    | continueStatement
    | deleteStatement
    | rawBlock
    | block
    ;

block
    : CURLY_L statement* CURLY_R
    ;

exprStatement
    : expression SemicolonSeparator
    ;

// ============================================================================
// Raw Block — Honest danger, no safety checks
// ============================================================================

rawBlock
    : RawKeyword block
    ;

// ============================================================================
// Control Flow — If / Else-If / Else
// ============================================================================

ifStatement
    : ControlFlowIf OpeningRoundBracket expression ClosingRoundBracket
      trueBody=statement
      elseIfClause*
      elseClause?
    ;

elseIfClause
    : ControlFlowElse ControlFlowIf OpeningRoundBracket expression ClosingRoundBracket
      statement
    ;

elseClause
    : ControlFlowElse statement
    ;

// ============================================================================
// Control Flow — Switch (Classic C-style)
// ============================================================================

switchStatement
    : ControlFlowSwitch OpeningRoundBracket expression ClosingRoundBracket
      CURLY_L switchCase* switchDefault? CURLY_R
    ;

switchCase
    : ControlFlowCase expression ColonOperator statement*
    ;

switchDefault
    : ControlFlowDefault ColonOperator statement*
    ;

// ============================================================================
// Control Flow — Pattern Matching
// ============================================================================

matchStatement
    : matchExpression SemicolonSeparator
    ;

matchExpression
    : ControlFlowMatch expression
      CURLY_L matchArm ( CommaSeparator matchArm )* CommaSeparator? CURLY_R
    ;

matchArm
    : pattern ArrowOperator matchBody
    ;

matchBody
    : expression
    | block
    ;

pattern
    : guardedPattern
    ;

guardedPattern
    : basePattern ( ControlFlowIf expression )?
    ;

basePattern
    : literalPattern
    | rangePattern
    | wildcardPattern
    | bindingPattern
    | tuplePattern
    ;

literalPattern
    : IntegerLiteral
    | FloatingLiteral
    | BooleanLiteral
    | CharLiteral
    | string
    | NullReference
    | qualifiedName
    ;

rangePattern
    : IntegerLiteral DotDotOperator IntegerLiteral
    ;

wildcardPattern
    : UnderscoreWildcard
    ;

bindingPattern
    : DeclareVariable Identifier
    ;

tuplePattern
    : OpeningRoundBracket pattern ( CommaSeparator pattern )+ ClosingRoundBracket
    ;

// ============================================================================
// Loops
// ============================================================================

forStatement
    : DeclareForLoop OpeningRoundBracket
      forInitializer? SemicolonSeparator
      expression? SemicolonSeparator
      forIterator?
      ClosingRoundBracket statement
    ;

forInitializer
    : localVarInitializer
    | expression ( CommaSeparator expression )*
    ;

localVarInitializer
    : localVarDeclaration ( CommaSeparator localVarDeclaration )*
    ;

localVarDeclaration
    : typeIdentifier Identifier ( AssignOperator expression )?
    | DeclareVariable Identifier AssignOperator expression
    ;

forIterator
    : expression ( CommaSeparator expression )*
    ;

whileStatement
    : DeclareWhileLoop OpeningRoundBracket expression ClosingRoundBracket
      ( block | statement )
    ;

// ============================================================================
// Jump & Memory Statements
// ============================================================================

returnStatement
    : FunctionReturn expression? SemicolonSeparator
    ;

breakStatement
    : Break SemicolonSeparator
    ;

continueStatement
    : Continue SemicolonSeparator
    ;

deleteStatement
    : DeleteKeyword expression SemicolonSeparator
    ;

// ============================================================================
// Expressions — Precedence Climbing
// ============================================================================

expression
    : assignment
    | lambdaExpression
    ;

assignment
    : unaryExpression assignmentOperator assignment
    | pipe
    ;

assignmentOperator
    : AssignOperator
    | PlusAssignOperator
    | MinusAssignOperator
    | MultiplyAssignOperator
    | DivideAssignOperator
    | ModuloAssignOperator
    | BitwiseAndAssignOperator
    | BitwiseOrAssignOperator
    | BitwiseXorAssignOperator
    | BitwiseLeftShiftAssignOperator
    | BitwiseRightShiftAssignOperator
    ;

lambdaExpression
    : OpeningRoundBracket lambdaParameterList? ClosingRoundBracket ArrowOperator
      ( block | expression )
    ;

lambdaParameterList
    : lambdaParameter ( CommaSeparator lambdaParameter )*
    ;

lambdaParameter
    : typeIdentifier Identifier
    | Identifier
    ;

// ============================================================================
// Pipe — Composition operator, lower precedence than ternary
// ============================================================================

pipe
    : ternary ( PipeOperator pipeTarget )*
    ;

pipeTarget
    : qualifiedName callArguments?
    ;

// ============================================================================
// Ternary & Binary Operators
// ============================================================================

ternary
    : logicOr ( QuestionMarkOperator expression ColonOperator expression )?
    ;

logicOr
    : logicAnd ( LogicalOrOperator logicAnd )*
    ;

logicAnd
    : bitwiseOr ( LogicalAndOperator bitwiseOr )*
    ;

bitwiseOr
    : bitwiseXor ( BitwiseOrOperator bitwiseXor )*
    ;

bitwiseXor
    : bitwiseAnd ( BitwiseXorOperator bitwiseAnd )*
    ;

bitwiseAnd
    : equality ( SingleAndOperator equality )*
    ;

equality
    : relational ( ( EqualOperator | UnequalOperator ) relational )*
    ;

relational
    : shift ( ( SmallerOperator | SmallerEqualOperator
              | GreaterOperator | GreaterEqualOperator ) shift )*
    ;

shift
    : additive ( ( ShiftLeftOperator | ShiftRightOperator ) additive )*
    ;

additive
    : multiplicative ( ( PlusOperator | MinusOperator ) multiplicative )*
    ;

multiplicative
    : castExpression ( ( StarOperator | DivideOperator | ModuloOperator ) castExpression )*
    ;

castExpression
    : OpeningRoundBracket typeIdentifier ClosingRoundBracket castExpression
    | unaryExpression
    ;

unaryExpression
    : prefixOperator unaryExpression
    | incrementDecrementOperator unaryExpression
    | typeSizeOrAlign OpeningRoundBracket typeIdentifier ClosingRoundBracket
    | postfixExpression
    ;

postfixExpression
    : primaryExpression postfixOperation*
    ;

primaryExpression
    : BooleanLiteral
    | NullReference
    | ThisReference
    | IntegerLiteral
    | FloatingLiteral
    | CharLiteral
    | string
    | Identifier
    | tupleExpression
    | newExpression
    | matchExpression
    | OpeningRoundBracket expression ClosingRoundBracket
    ;

postfixOperation
    : callArguments
    | elementAccess
    | memberAccess
    | incrementDecrementOperator
    ;

// ============================================================================
// New Expression — Heap allocation
// ============================================================================

newExpression
    : NewKeyword typeIdentifier callArguments?
    | NewKeyword typeIdentifier SquareBracketLeft expression SquareBracketRight
    ;

// ============================================================================
// Call, Access, Index
// ============================================================================

callArguments
    : OpeningRoundBracket argumentList? ClosingRoundBracket
    ;

argumentList
    : expression ( CommaSeparator expression )*
    ;

elementAccess
    : SquareBracketLeft expression SquareBracketRight
    ;

memberAccess
    : DotOperator Identifier
    | ReferenceAccessOperator Identifier
    ;

// ============================================================================
// Tuple Expressions
// ============================================================================

tupleExpression
    : OpeningRoundBracket expression CommaSeparator expression
      ( CommaSeparator expression )* ClosingRoundBracket
    ;

// ============================================================================
// Type System
// ============================================================================

typeIdentifier
    : primitiveType typeModifier*
    | qualifiedName typeModifier*
    | tupleType typeModifier*
    | functionType typeModifier*
    ;

primitiveType
    : IntegerType
    | DoubleType
    | FloatType
    | ByteType
    | StringType
    | CharType
    | BoolType
    | VoidType
    ;

functionType
    : OpeningRoundBracket typeList? ClosingRoundBracket ArrowOperator returnType
    ;

typeList
    : typeIdentifier ( CommaSeparator typeIdentifier )*
    ;

tupleType
    : OpeningRoundBracket typeIdentifier CommaSeparator typeIdentifier
      ( CommaSeparator typeIdentifier )* ClosingRoundBracket
    ;

typeModifier
    : arrayDimension
    | pointerLevel
    ;

arrayDimension
    : SquareBracketLeft IntegerLiteral? SquareBracketRight
    ;

pointerLevel
    : StarOperator
    ;

// ============================================================================
// Modifiers
// ============================================================================

accessModifier
    : DeclarePublic
    | DeclarePrivate
    | DeclareProtected
    ;

staticModifier
    : DeclareStatic
    ;

abstractModifier
    : DeclareAbstract
    ;

// ============================================================================
// Shared Rules
// ============================================================================

qualifiedName
    : Identifier ( DotOperator Identifier )*
    ;

prefixOperator
    : LogicalNegationOperator
    | MinusOperator
    | PlusOperator
    | ComplimentOperator
    | SingleAndOperator
    | StarOperator
    ;

incrementDecrementOperator
    : PlusPlusOperator
    | MinusMinusOperator
    ;

typeSizeOrAlign
    : SizeOfKeyword
    | AlignOfKeyword
    ;

string
    : DQUOTE stringPart* DQUOTE
    ;

stringPart
    : TEXT
    | ESCAPE_SEQUENCE
    | BACKSLASH_PAREN expression CURLY_R
    ;
