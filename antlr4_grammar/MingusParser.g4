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
    Lambdas & Closures              [=](int x) => { return x * 2; }
    Explicit Capture Lists           [x, &y](int z) => x + y + z
    Reference Parameters             func swap(int& a, int& b) => void
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
    | unionDeclaration
    | taggedUnionDeclaration
    | enumDeclaration
    | interfaceDeclaration
    | functionDeclaration
    | externDeclaration
    | variableDeclaration
    | typedefDeclaration
    | importDefinition
    ;

// ============================================================================
// Typedef / Type Alias
// ============================================================================

typedefDeclaration
    : DeclareTypedef typeIdentifier Identifier SemicolonSeparator
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
    | externLinkDirective
    | externOpaqueTypeDeclaration
    | externVariableDeclaration
    | CURLY_L externMember* CURLY_R
    ;

externMember
    : externFunctionDeclaration
    | externLinkDirective
    | externOpaqueTypeDeclaration
    | externStructDeclaration
    | externUnionDeclaration
    | externEnumDeclaration
    | externVariableDeclaration
    ;

externFunctionDeclaration
    : attribute* DeclareFunction Identifier
      OpeningRoundBracket parameterList? ( CommaSeparator Ellipsis )? ClosingRoundBracket
      ArrowOperator returnType SemicolonSeparator
    ;

externLinkDirective
    : LinkKeyword string SemicolonSeparator
    ;

externOpaqueTypeDeclaration
    : OpaqueKeyword Identifier SemicolonSeparator
    ;

externStructDeclaration
    : attribute* DeclareStruct Identifier CURLY_L externFieldDeclaration* CURLY_R
    ;

externUnionDeclaration
    : attribute* DeclareUnion Identifier CURLY_L externFieldDeclaration* CURLY_R
    ;

externFieldDeclaration
    : typeIdentifier Identifier SemicolonSeparator
    ;

externEnumDeclaration
    : DeclareEnum Identifier ( ColonOperator typeIdentifier )?
      CURLY_L enumMember ( CommaSeparator enumMember )* CommaSeparator? CURLY_R
    ;

externVariableDeclaration
    : typeIdentifier Identifier SemicolonSeparator
    ;

// ============================================================================
// Class System
// ============================================================================

classDeclaration
    : accessModifier? ( staticModifier | abstractModifier )?
      DeclareClass Identifier typeParameterList? ( ColonOperator inheritance )?
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
    : inheritanceItem ( CommaSeparator inheritanceItem )*
    ;

inheritanceItem
    : qualifiedName typeArgumentList?
    ;

// ============================================================================
// Interface — Method contracts, Go-style fat pointer dispatch
// ============================================================================

interfaceDeclaration
    : accessModifier? DeclareInterface Identifier typeParameterList?
      ( interfaceBlock | SemicolonSeparator )
    ;

interfaceBlock
    : CURLY_L interfaceMember* CURLY_R
    ;

interfaceMember
    : functionDeclaration
    ;

// ============================================================================
// Attributes — Layout annotations (@packed, @align(N))
// ============================================================================

attribute
    : AtSign Identifier ( OpeningRoundBracket IntegerLiteral ClosingRoundBracket )?
    ;

// ============================================================================
// Struct — Value types with methods, no inheritance
// ============================================================================

structDeclaration
    : attribute* accessModifier?
      DeclareStruct Identifier typeParameterList? ( structBlock | SemicolonSeparator )
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
// Union
// ============================================================================

unionDeclaration
    : attribute* accessModifier?
      DeclareUnion Identifier ( unionBlock | SemicolonSeparator )
    ;

unionBlock
    : CURLY_L unionMember* CURLY_R
    ;

unionMember
    : functionDeclaration
    | variableDeclaration
    ;

// ============================================================================
// Tagged Union (Discriminated Union / Sum Type)
// ============================================================================

taggedUnionDeclaration
    : accessModifier? TaggedKeyword DeclareUnion Identifier
      CURLY_L taggedUnionVariant (CommaSeparator taggedUnionVariant)* CommaSeparator? CURLY_R
    ;

taggedUnionVariant
    : Identifier (OpeningRoundBracket taggedUnionField (CommaSeparator taggedUnionField)* ClosingRoundBracket)?
    ;

taggedUnionField
    : typeIdentifier Identifier
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
      DeclareFunction Identifier typeParameterList?
      definitionParameters ArrowOperator returnType
      ( block | ArrowOperator expression SemicolonSeparator | SemicolonSeparator )
    ;

returnType
    : typeIdentifier
    | tupleType
    ;

typeParameterList
    : SmallerOperator typeParameter (CommaSeparator typeParameter)* GreaterOperator
    ;

typeParameter
    : Identifier ( ColonOperator typeParameterBound ( PlusOperator typeParameterBound )* )?
    ;

typeParameterBound
    : qualifiedName typeArgumentList?
    ;

typeArgumentList
    : SmallerOperator typeIdentifier (CommaSeparator typeIdentifier)* GreaterOperator
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
    | accessModifier? staticModifier? constVariableDeclaration
    | accessModifier? staticModifier? tupleDestructuring
    ;

constVariableDeclaration
    : DeclareConst typeIdentifier Identifier AssignOperator exprStatement
    | DeclareConst Identifier AssignOperator exprStatement
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
    | doWhileStatement
    | ifStatement
    | switchStatement
    | matchStatement
    | returnStatement
    | labeledStatement
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
    | qualifiedName ( OpeningRoundBracket variantPatternField ( CommaSeparator variantPatternField )* ClosingRoundBracket )?
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

variantPatternField
    : bindingPattern
    | wildcardPattern
    | literalPattern
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
    | DeclareConst typeIdentifier Identifier AssignOperator expression
    | DeclareConst Identifier AssignOperator expression
    ;

forIterator
    : expression ( CommaSeparator expression )*
    ;

whileStatement
    : DeclareWhileLoop OpeningRoundBracket expression ClosingRoundBracket
      ( block | statement )
    ;

doWhileStatement
    : DeclareDoLoop ( block | statement ) DeclareWhileLoop
      OpeningRoundBracket expression ClosingRoundBracket SemicolonSeparator
    ;

// ============================================================================
// Jump & Memory Statements
// ============================================================================

returnStatement
    : FunctionReturn expression? SemicolonSeparator
    ;

labeledStatement
    : Identifier ColonOperator ( forStatement | whileStatement | doWhileStatement )
    ;

breakStatement
    : Break Identifier? SemicolonSeparator
    ;

continueStatement
    : Continue Identifier? SemicolonSeparator
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
    : unaryExpression assignmentOperator ( lambdaExpression | assignment )
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
    : captureList
      OpeningRoundBracket lambdaParameterList? ClosingRoundBracket ArrowOperator
      ( block | expression )
    ;

captureList
    : SquareBracketLeft SquareBracketRight
    | SquareBracketLeft captureDefault SquareBracketRight
    | SquareBracketLeft captureDefault ( CommaSeparator captureItem )+ SquareBracketRight
    | SquareBracketLeft captureItem ( CommaSeparator captureItem )* SquareBracketRight
    ;

captureDefault
    : AssignOperator
    | SingleAndOperator
    ;

captureItem
    : SingleAndOperator Identifier
    | WeakKeyword Identifier
    | Identifier
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
    : qualifiedName ( ( DotOperator | ReferenceAccessOperator ) Identifier )* callArguments?
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
    : MoveKeyword OpeningRoundBracket expression ClosingRoundBracket
    | prefixOperator unaryExpression
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
    | arrayLiteral
    | OpeningRoundBracket expression ClosingRoundBracket
    ;

arrayLiteral
    : SquareBracketLeft expression (CommaSeparator expression)* SquareBracketRight
    ;

postfixOperation
    : TurbofishOperator typeIdentifier (CommaSeparator typeIdentifier)* GreaterOperator callArguments
    | callArguments
    | elementAccess
    | memberAccess
    | incrementDecrementOperator
    ;

// ============================================================================
// New Expression — Heap allocation
// ============================================================================

newExpression
    : NewKeyword SharedKeyword typeIdentifier callArguments?
    | NewKeyword typeIdentifier callArguments?
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
    : DeclareConst primitiveType typeModifier*
    | DeclareConst qualifiedName typeArgumentList? typeModifier*
    | SharedKeyword qualifiedName typeArgumentList? typeModifier*
    | primitiveType typeModifier*
    | qualifiedName typeArgumentList? typeModifier*
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
    | ShortType
    | UShortType
    | UIntType
    | LongType
    | ULongType
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
    | rvalueReferenceLevel
    | referenceLevel
    ;

rvalueReferenceLevel
    : LogicalAndOperator
    ;

referenceLevel
    : SingleAndOperator
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
