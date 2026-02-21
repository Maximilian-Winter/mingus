# Mingus Grammar and AST Reference

This document is the authoritative reference for the Mingus language grammar and AST node hierarchy. It covers the ANTLR4 lexer and parser grammar, the complete AST node inventory with all fields, and the ASTGenerator that bridges the two.

**Source files:**

| File | Purpose |
|------|---------|
| `antlr4_grammar/MingusLexer.g4` | Lexer rules (tokens, keywords, operators, literals) |
| `antlr4_grammar/MingusParser.g4` | Parser rules (syntax structure) |
| `src/mingus/parser/ASTGenerator.cpp` | ANTLR4 parse tree to AST conversion |
| `include/mingus/AstNode.h` | Base AST nodes, type nodes, pattern nodes, supporting structs |
| `include/mingus/Expressions.h` | All expression AST nodes |
| `include/mingus/Statements.h` | All statement AST nodes |
| `include/mingus/Declarations.h` | All declaration AST nodes |
| `include/mingus/Forward.h` | Forward declarations and all enums |
| `include/mingus/DebugInfo.h` | Source range tracking for AST nodes |

---

## Table of Contents

1. [Grammar Overview](#1-grammar-overview)
2. [Lexer Rules](#2-lexer-rules)
3. [Parser Rules](#3-parser-rules)
4. [Operator Precedence](#4-operator-precedence)
5. [AST Node Hierarchy](#5-ast-node-hierarchy)
6. [ASTGenerator Mapping](#6-astgenerator-mapping)
7. [Key Design Decisions](#7-key-design-decisions)
8. [Grammar Limitations](#8-grammar-limitations)

---

## 1. Grammar Overview

Mingus uses ANTLR4 to define its grammar, split across two files:

- **MingusLexer.g4** -- Defines all tokens: keywords, operators, punctuation, numeric and string literals, identifiers, comments, and whitespace. Uses a lexer **mode** (`IN_STRING`) for string interpolation support.

- **MingusParser.g4** -- Defines the syntactic structure. The parser references the lexer via `tokenVocab = 'MingusLexer'`. Grammar rules are organized into program structure, declarations (classes, structs, enums, interfaces, functions, extern, imports), statements (control flow, loops, jumps, memory), and expressions (with full operator precedence climbing).

The compilation pipeline is:

```
Source (.mingus)
  --> ANTLR4 Lexer (MingusLexer.g4)
    --> Token stream
      --> ANTLR4 Parser (MingusParser.g4)
        --> Parse tree (CST)
          --> ASTGenerator (ASTGenerator.cpp)
            --> AST (AstNode.h, Expressions.h, Statements.h, Declarations.h)
              --> 4 semantic analysis passes
                --> LLVM IR (IRGenerator)
                  --> clang --> native executable
```

The ASTGenerator is implemented as an ANTLR4 visitor (`MingusParserBaseVisitor`) that walks the parse tree and constructs AST nodes. The AST is a cleaner, more compact representation of the program than the raw parse tree, suitable for semantic analysis and code generation.

---

## 2. Lexer Rules

### 2.1 Keywords

The lexer defines keywords in several categories:

**Language Structure:**
```
module  class  struct  enum  func  constructor  destructor
super   operator  for  while  static  abstract  interface
public  private  protected  extern  raw
```

**Control Flow:**
```
if  else  switch  case  default  match  return  break  continue
```

**Memory and References:**
```
var  const  new  delete  null  this
```

**Module System:**
```
import  from  as
```

**Compile-Time Operators:**
```
sizeof  alignof
```

**Built-In Types:**
```
int  double  float  byte  string  char  bool  void
```

**Boolean Literals:**
```
true  false
```

### 2.2 Operators

**Assignment Operators:**

| Token | Symbol |
|-------|--------|
| `AssignOperator` | `=` |
| `PlusAssignOperator` | `+=` |
| `MinusAssignOperator` | `-=` |
| `MultiplyAssignOperator` | `*=` |
| `DivideAssignOperator` | `/=` |
| `ModuloAssignOperator` | `%=` |
| `BitwiseAndAssignOperator` | `&=` |
| `BitwiseOrAssignOperator` | `\|=` |
| `BitwiseXorAssignOperator` | `^=` |
| `BitwiseLeftShiftAssignOperator` | `<<=` |
| `BitwiseRightShiftAssignOperator` | `>>=` |

**Logical Operators:**

| Token | Symbol |
|-------|--------|
| `LogicalOrOperator` | `\|\|` |
| `LogicalAndOperator` | `&&` |

**Comparison Operators:**

| Token | Symbol |
|-------|--------|
| `EqualOperator` | `==` |
| `UnequalOperator` | `!=` |
| `GreaterEqualOperator` | `>=` |
| `SmallerEqualOperator` | `<=` |
| `GreaterOperator` | `>` |
| `SmallerOperator` | `<` |

**Shift Operators:**

| Token | Symbol |
|-------|--------|
| `ShiftLeftOperator` | `<<` |
| `ShiftRightOperator` | `>>` |

**Arithmetic and Unary Operators:**

| Token | Symbol |
|-------|--------|
| `PlusPlusOperator` | `++` |
| `MinusMinusOperator` | `--` |
| `PlusOperator` | `+` |
| `MinusOperator` | `-` |
| `StarOperator` | `*` |
| `DivideOperator` | `/` |
| `ModuloOperator` | `%` |

**Bitwise and Logical Unary:**

| Token | Symbol |
|-------|--------|
| `LogicalNegationOperator` | `!` |
| `ComplimentOperator` | `~` |
| `SingleAndOperator` | `&` |
| `BitwiseXorOperator` | `^` |
| `BitwiseOrOperator` | `\|` |

**Punctuation and Delimiters:**

| Token | Symbol | Purpose |
|-------|--------|---------|
| `PipeOperator` | `\|>` | Pipe composition |
| `ArrowOperator` | `=>` | Return type, lambda body, match arm |
| `ReferenceAccessOperator` | `->` | Pointer member access |
| `DotOperator` | `.` | Value member access, qualified names |
| `Ellipsis` | `...` | Varargs |
| `DotDotOperator` | `..` | Range patterns |
| `QuestionMarkOperator` | `?` | Ternary conditional |
| `ColonOperator` | `:` | Ternary, inheritance, enum underlying type, super call |
| `SemicolonSeparator` | `;` | Statement terminator |
| `CommaSeparator` | `,` | List separator |
| `UnderscoreWildcard` | `_` | Wildcard pattern |

**Brackets:**

| Token | Symbol |
|-------|--------|
| `OpeningRoundBracket` / `ClosingRoundBracket` | `(` `)` |
| `SquareBracketLeft` / `SquareBracketRight` | `[` `]` |
| `CURLY_L` / `CURLY_R` | `{` `}` |

Note: `CURLY_L` and `CURLY_R` participate in mode management for string interpolation (see below).

### 2.3 Numeric Literals

| Format | Prefix | Example | Base |
|--------|--------|---------|------|
| Decimal | (none) | `42`, `1000` | 10 |
| Hexadecimal | `0x` or `0X` | `0xFF`, `0x1A2B` | 16 |
| Binary | `0b` or `0B` | `0b1010`, `0B1111` | 2 |
| Octal | `0o` or `0O` | `0o777`, `0O644` | 8 |
| Floating | (none) | `3.14`, `1.0e-5`, `2.5E3` | 10 |

Integer literals are parsed by `parseIntegerLiteral()` in ASTGenerator, which dispatches on the prefix:
- `0b`/`0B` -- strips prefix, parses with base 2
- `0o`/`0O` -- strips prefix, parses with base 8
- All others -- uses `std::stoll(..., nullptr, 0)` which handles `0x` natively

Floating literals support fractional constants (`3.14`, `0.5`) and exponent notation (`1e5`, `2.5E-3`). Digit sequences may contain single-quote digit separators (e.g., `1'000'000`).

### 2.4 Character Literals

Character literals are enclosed in single quotes: `'a'`, `'\n'`. Supported escape sequences:

```
\n  \r  \t  \f  \b  \0  \'  \\
```

Note: The ASTGenerator extracts the character by reading `text[1]` from the raw token text, without escape-sequence processing. This means escape characters inside char literals (e.g., `'\n'`) will be treated as the literal backslash character rather than newline. This is a known limitation.

### 2.5 String Literals and Interpolation

The lexer uses **mode-based scanning** for string interpolation:

1. When `"` is encountered in default mode, the lexer pushes `IN_STRING` mode.
2. Inside `IN_STRING`:
   - `TEXT` matches any characters that are not `$`, `\`, or `"`.
   - `ESCAPE_SEQUENCE` matches `\` followed by any character.
   - `BACKSLASH_PAREN` (`${`) pushes back to `DEFAULT_MODE`, allowing a full expression.
   - `DQUOTE_IN_STRING` (`"`) pops back to the previous mode.
3. `CURLY_L` (`{`) pushes `DEFAULT_MODE`; `CURLY_R` (`}`) pops a mode.

This mechanism allows arbitrarily nested expressions inside string interpolation:

```
"hello"                         // plain string
"value = ${x + 1}"             // interpolated with expression
"name: ${person.name}"         // member access in interpolation
"result: ${compute(a, b)}"     // function call in interpolation
```

Supported escape sequences in strings: `\n`, `\t`, `\r`, `\\`, `\"`, `\'`, `\0`. Other `\X` sequences take the literal character after the backslash.

The ASTGenerator determines the node type based on content: a string with only text parts produces a `StringLiteral`; a string with any `${...}` interpolation produces an `InterpolatedStringExpression`.

### 2.6 Identifiers

```
Identifier: [a-zA-Z_][a-zA-Z0-9_]*
```

Standard C-style identifiers: letters, digits, and underscores, starting with a letter or underscore.

### 2.7 Comments and Whitespace

```
BLOCK_COMMENT: '/*' .*? '*/'  -> skip
LINE_COMMENT:  '//' ~[\r\n]*  -> skip
WS:            [ \r\t\n]+     -> skip
```

Block comments (`/* ... */`), line comments (`// ...`), and whitespace are all discarded by the lexer.

---

## 3. Parser Rules

### 3.1 Program Structure

```
program : module* EOF
module  : 'module' Identifier moduleBlock
```

A Mingus program consists of one or more **modules**. Each module has a name and a body enclosed in braces:

```mingus
module Main {
    // declarations here
}
```

**Module declarations** (the top-level items inside a module):

```
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
```

### 3.2 Import System

```
importDefinition
    : 'import' importTarget (',' importTarget)* ('from' qualifiedName)? ';'
    ;

importTarget
    : Identifier ('as' Identifier)?
    ;
```

Two forms:

- **Selective import:** `import Foo, Bar as Baz from ModuleName;` -- imports specific symbols from a module, with optional aliasing.
- **Whole-module import:** `import ModuleName;` -- imports an entire module. The `from` clause is absent.

Examples:

```mingus
import MathLib;                          // whole module
import sin, cos from MathLib;            // selective
import DynamicArray as DA from Collections;  // with alias
```

### 3.3 Extern Declarations (C Interop)

```
externDeclaration
    : 'extern' externBody
    ;

externBody
    : externFunctionDeclaration
    | '{' externFunctionDeclaration* '}'
    ;

externFunctionDeclaration
    : 'func' Identifier '(' parameterList? (',' '...')? ')' '=>' returnType ';'
    ;
```

Extern functions declare C-linkage functions for FFI. They support **varargs** via `...`:

```mingus
extern func printf(string fmt, ...) => int;
extern func sin(double x) => double;

extern {
    func malloc(int size) => byte*;
    func free(byte* ptr) => void;
}
```

The `...` (Ellipsis) must appear after the parameter list, separated by a comma. The ASTGenerator sets `ExternFunctionDeclaration::isVariadic = true` when an Ellipsis is present.

Extern declarations can appear individually or grouped in an `extern { ... }` block.

### 3.4 Functions

```
functionDeclaration
    : accessModifier? (staticModifier | abstractModifier)?
      'func' Identifier definitionParameters '=>' returnType
      (block | '=>' expression ';' | ';')
    ;

definitionParameters
    : '(' parameterList? ')'
    ;

parameterList
    : parameter (',' parameter)*
    ;

parameter
    : typeIdentifier Identifier ('=' expression)?
    ;
```

Functions have three body forms:

1. **Block body:** `func add(int a, int b) => int { return a + b; }`
2. **Expression body:** `func add(int a, int b) => int => a + b;` -- the ASTGenerator wraps this in a `Block{ReturnStatement{expr}}`.
3. **Abstract (no body):** `func draw() => void;` -- used in interfaces and abstract classes.

Functions can carry modifiers:
- **Access modifiers:** `public`, `private`, `protected`
- **Static:** `static func create() => MyClass { ... }`
- **Abstract:** `abstract func draw() => void;`

Parameters support **default values**: `func greet(string name = "world") => void { ... }`

**Reference parameters** use the `&` type modifier on the type: `func swap(int& a, int& b) => void`. The ASTGenerator detects the reference modifier and sets `ParameterNode::isReference = true`.

### 3.5 Classes

```
classDeclaration
    : accessModifier? (staticModifier | abstractModifier)?
      'class' Identifier (':' inheritance)?
      (classBlock | ';')
    ;

classBlock
    : '{' classMember* '}'
    ;

classMember
    : constructorDeclaration
    | destructorDeclaration
    | operatorDeclaration
    | functionDeclaration
    | variableDeclaration
    ;

inheritance
    : qualifiedName (',' qualifiedName)*
    ;
```

Classes support:
- **Inheritance:** `class Dog : Animal` -- single base class. Multiple names after `:` represent base class + interfaces.
- **Access modifiers:** `public`, `private`, `protected` (on the class itself and on members)
- **Abstract classes:** `abstract class Shape { abstract func area() => double; }`
- **Static classes:** `static class Utils { ... }`

```mingus
class Dog : Animal {
    private string breed;

    constructor(string name, string breed) : super(name) {
        this.breed = breed;
    }

    destructor {
        // cleanup
    }

    func bark() => void {
        printf("Woof!\n");
    }
}
```

### 3.6 Constructors and Destructors

```
constructorDeclaration
    : accessModifier?
      'constructor' definitionParameters
      (':' 'super' callArguments)?
      block
    ;

destructorDeclaration
    : 'destructor' block
    ;
```

Constructors support an optional **super call**: `constructor(int x) : super(x) { ... }`. The super arguments are parsed from the `callArguments` after `: super`.

Destructors take no parameters. When a class lacks an explicit constructor or destructor, the `SymbolTableBuilder` (Pass 1) auto-generates synthetic ones with empty bodies.

### 3.7 Structs

```
structDeclaration
    : accessModifier?
      'struct' Identifier (structBlock | ';')
    ;

structBlock
    : '{' structMember* '}'
    ;

structMember
    : operatorDeclaration
    | functionDeclaration
    | variableDeclaration
    ;
```

Structs are value types with fields, methods, and operator overloads. They do not support inheritance, constructors, or destructors.

```mingus
struct Vec2 {
    double x;
    double y;

    func length() => double {
        return sqrt(x * x + y * y);
    }

    func operator+(Vec2 other) => Vec2 => Vec2(x + other.x, y + other.y);
}
```

### 3.8 Operator Overloading

```
operatorDeclaration
    : accessModifier?
      'func' 'operator' overloadableOperator
      definitionParameters '=>' returnType
      (block | '=>' expression ';')
    ;

overloadableOperator
    : '+' | '-' | '*' | '/' | '%'
    | '==' | '!=' | '<' | '<=' | '>' | '>='
    | '[' ']'
    ;
```

Twelve operators can be overloaded: the six arithmetic/modulo operators, six comparison operators, and the indexing operator `[]`. Operator declarations support both block and expression bodies.

### 3.9 Enums

```
enumDeclaration
    : accessModifier?
      'enum' Identifier (':' typeIdentifier)?
      '{' enumMember (',' enumMember)* ','? '}'
    ;

enumMember
    : Identifier ('=' expression)?
    ;
```

Enums support an optional **underlying type** (defaults to `int`). Members can have explicit values or be auto-assigned:

```mingus
enum Color : int {
    Red = 0,
    Green = 1,
    Blue = 2,
}

enum Status {
    Ok,         // auto-assigned 0
    Error,      // auto-assigned 1
}
```

Trailing commas are allowed.

### 3.10 Interfaces

```
interfaceDeclaration
    : accessModifier? 'interface' Identifier
      (interfaceBlock | ';')
    ;

interfaceBlock
    : '{' interfaceMember* '}'
    ;

interfaceMember
    : functionDeclaration
    ;
```

Interfaces declare method contracts. All interface methods are implicitly abstract (no body):

```mingus
interface Drawable {
    func draw() => void;
    func getColor() => int;
}
```

At runtime, interfaces use fat-pointer dispatch: `{ objPtr, itablePtr }`.

### 3.11 Variable Declarations

```
variableDeclaration
    : accessModifier? staticModifier? typedVariableDeclaration
    | accessModifier? staticModifier? inferredVariableDeclaration
    | accessModifier? staticModifier? constVariableDeclaration
    | accessModifier? staticModifier? tupleDestructuring
    ;
```

Four forms of variable declaration:

**Typed declaration:**
```
typeIdentifier Identifier (';' | '=' exprStatement | callArguments ';')
```

```mingus
int x;                  // uninitialized
int x = 42;             // with initializer
Point p(1.0, 2.0);     // construction syntax (struct init via call args)
```

**Inferred declaration:**
```
'var' Identifier '=' exprStatement
```

```mingus
var x = 42;             // type inferred as int
var name = "hello";     // type inferred as string
```

**Const declaration:**
```
'const' typeIdentifier Identifier '=' exprStatement
'const' Identifier '=' exprStatement
```

```mingus
const int MAX = 100;     // typed const
const PI = 3.14159;      // inferred const
```

**Tuple destructuring:**
```
'(' tupleDestructureElement (',' tupleDestructureElement)+ ')' '=' exprStatement
```

```mingus
(int a, int b) = getTuple();
(var x, var y) = computePair();
```

### 3.12 Statements

```
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
```

#### If / Else-If / Else

```
ifStatement
    : 'if' '(' expression ')' statement elseIfClause* elseClause?
    ;

elseIfClause
    : 'else' 'if' '(' expression ')' statement
    ;

elseClause
    : 'else' statement
    ;
```

```mingus
if (x > 0) {
    printf("positive\n");
} else if (x == 0) {
    printf("zero\n");
} else {
    printf("negative\n");
}
```

#### For Loops (with multi-init)

```
forStatement
    : 'for' '(' forInitializer? ';' expression? ';' forIterator? ')' statement
    ;

forInitializer
    : localVarInitializer
    | expression (',' expression)*
    ;

localVarInitializer
    : localVarDeclaration (',' localVarDeclaration)*
    ;

localVarDeclaration
    : typeIdentifier Identifier ('=' expression)?
    | 'var' Identifier '=' expression
    | 'const' typeIdentifier Identifier '=' expression
    | 'const' Identifier '=' expression
    ;

forIterator
    : expression (',' expression)*
    ;
```

For loops support **multiple variable declarations** in the initializer:

```mingus
for (int i = 0, int j = 10; i < j; i++, j--) {
    printf("i=%d j=%d\n", i, j);
}

for (var k = 0; k < 100; k++) {
    // inferred type in for-init
}

for (;;) {
    // infinite loop (all three clauses optional)
}
```

The initializer can be either a list of `localVarDeclaration` (which may include `const` declarations) or a list of expressions. The iterator clause also supports multiple comma-separated expressions.

#### While Loops

```
whileStatement
    : 'while' '(' expression ')' (block | statement)
    ;
```

```mingus
while (running) {
    tick();
}
```

#### Switch Statements

```
switchStatement
    : 'switch' '(' expression ')' '{' switchCase* switchDefault? '}'
    ;

switchCase
    : 'case' expression ':' statement*
    ;

switchDefault
    : 'default' ':' statement*
    ;
```

Classic C-style switch:

```mingus
switch (color) {
    case Color.Red:
        printf("red\n");
    case Color.Blue:
        printf("blue\n");
    default:
        printf("other\n");
}
```

#### Return, Break, Continue

```
returnStatement    : 'return' expression? ';'
breakStatement     : 'break' ';'
continueStatement  : 'continue' ';'
```

#### Delete Statement

```
deleteStatement : 'delete' expression ';'
```

Frees heap-allocated objects. Dispatches through `vtable[0]` (virtual destructor), then frees memory.

```mingus
var dog = new Dog("Rex");
delete dog;
```

#### Raw Blocks

```
rawBlock : 'raw' block
```

Raw blocks signal that the enclosed code may perform unsafe operations (pointer arithmetic, dereference assignment, etc.). In V2, the ASTGenerator treats raw blocks as regular blocks (no separate `RawBlock` AST node).

```mingus
raw {
    *ptr = 42;
}
```

### 3.13 Expressions

The expression grammar uses **precedence climbing** (separate rules per precedence level) to enforce operator precedence without left recursion.

```
expression
    : assignment
    | lambdaExpression
    ;
```

#### Assignment

```
assignment
    : unaryExpression assignmentOperator (lambdaExpression | assignment)
    | pipe
    ;

assignmentOperator
    : '=' | '+=' | '-=' | '*=' | '/=' | '%='
    | '&=' | '|=' | '^=' | '<<=' | '>>='
    ;
```

Assignment is right-associative. The RHS can be a lambda expression (enabling `f = [=](int x) => x * 2;`) or another assignment (enabling `a = b = c`).

#### Lambda Expressions

```
lambdaExpression
    : captureList '(' lambdaParameterList? ')' '=>' (block | expression)
    ;

captureList
    : '[' ']'
    | '[' captureDefault ']'
    | '[' captureDefault (',' captureItem)+ ']'
    | '[' captureItem (',' captureItem)* ']'
    ;

captureDefault
    : '='     // capture all by value
    | '&'     // capture all by reference
    ;

captureItem
    : '&' Identifier    // capture by reference
    | Identifier         // capture by value
    ;

lambdaParameterList
    : lambdaParameter (',' lambdaParameter)*
    ;

lambdaParameter
    : typeIdentifier Identifier
    | Identifier                   // untyped (sema will error)
    ;
```

Lambdas require **mandatory explicit capture lists** (C++ style). Capture list forms:

| Syntax | Meaning |
|--------|---------|
| `[]` | No captures |
| `[=]` | All by value (copy) |
| `[&]` | All by reference |
| `[x, y]` | Explicit by-value captures |
| `[&x, &y]` | Explicit by-reference captures |
| `[=, &x]` | All by value, except `x` by reference |
| `[&, x]` | All by reference, except `x` by value |

Lambda bodies can be either a block or a single expression:

```mingus
var double_it = [=](int x) => x * 2;

var accumulate = [&sum](int x) => {
    sum = sum + x;
    return sum;
};
```

Lambda parameters can optionally include types. Untyped lambda parameters produce a `ParameterNode` with null type; type inference for lambda params is not supported.

#### Pipe Expression

```
pipe
    : ternary ('|>' pipeTarget)*
    ;

pipeTarget
    : qualifiedName (('.' | '->') Identifier)* callArguments?
    ;
```

The pipe operator `|>` passes the left-hand value as the first argument to the right-hand function. Pipe targets can be:

- Simple function names: `x |> f`
- Qualified names: `x |> Math.abs`
- Member access chains: `x |> obj.method` or `x |> ptr->method`
- With extra arguments: `x |> f(a, b)` (the piped value becomes the first arg, `a` and `b` are appended)

```mingus
input |> parse |> validate |> transform(config) |> output;
```

#### Ternary

```
ternary
    : logicOr ('?' expression ':' expression)?
    ;
```

```mingus
var result = (x > 0) ? "positive" : "non-positive";
```

#### Binary Operators

Each precedence level has its own rule. All binary operators are left-associative and build left-leaning chains of `BinaryExpression` nodes:

```
logicOr        : logicAnd ('||' logicAnd)*
logicAnd       : bitwiseOr ('&&' bitwiseOr)*
bitwiseOr      : bitwiseXor ('|' bitwiseXor)*
bitwiseXor     : bitwiseAnd ('^' bitwiseAnd)*
bitwiseAnd     : equality ('&' equality)*
equality       : relational (('==' | '!=') relational)*
relational     : shift (('<' | '<=' | '>' | '>=') shift)*
shift          : additive (('<<' | '>>') additive)*
additive       : multiplicative (('+' | '-') multiplicative)*
multiplicative : castExpression (('*' | '/' | '%') castExpression)*
```

#### Cast Expressions

```
castExpression
    : '(' typeIdentifier ')' castExpression
    | unaryExpression
    ;
```

C-style cast syntax: `(double)x`, `(int)3.14`.

#### Unary Expressions

```
unaryExpression
    : prefixOperator unaryExpression
    | incrementDecrementOperator unaryExpression
    | typeSizeOrAlign '(' typeIdentifier ')'
    | postfixExpression
    ;

prefixOperator
    : '!' | '-' | '+' | '~' | '&' | '*'
    ;

incrementDecrementOperator
    : '++' | '--'
    ;

typeSizeOrAlign
    : 'sizeof' | 'alignof'
    ;
```

Prefix unary operators: `!` (logical not), `-` (negate), `+` (positive), `~` (bitwise complement), `&` (address-of), `*` (dereference), `++`/`--` (pre-increment/decrement), `sizeof(Type)`, `alignof(Type)`.

#### Postfix Expressions

```
postfixExpression
    : primaryExpression postfixOperation*
    ;

postfixOperation
    : callArguments                       // f(args)
    | elementAccess                       // arr[index]
    | memberAccess                        // obj.field or ptr->field
    | incrementDecrementOperator          // x++ or x--
    ;

callArguments : '(' argumentList? ')'
elementAccess : '[' expression ']'
memberAccess  : '.' Identifier | '->' Identifier
```

Postfix operations are chained left-to-right. Each operation wraps the accumulator:

```mingus
obj.method(args)[0].field++
// Parsed as: ((((obj).method)(args))[0]).field)++
```

#### Primary Expressions

```
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
    | '(' expression ')'
    ;
```

Primary expressions are the "atoms" of the expression grammar: literals, identifiers, `this`, `null`, tuples, `new`, `match`, and parenthesized sub-expressions.

#### Tuple Expressions

```
tupleExpression
    : '(' expression ',' expression (',' expression)* ')'
    ;
```

Tuples require at least two elements: `(1, 2)`, `("hello", 42, true)`.

#### New Expressions

```
newExpression
    : 'new' typeIdentifier callArguments?
    | 'new' typeIdentifier '[' expression ']'
    ;
```

Two forms:
- **Object allocation:** `new Dog("Rex")` -- heap-allocates and calls constructor.
- **Array allocation:** `new int[100]` -- heap-allocates an array.

#### Match Expressions

```
matchExpression
    : 'match' expression '{' matchArm (',' matchArm)* ','? '}'
    ;

matchArm
    : pattern '=>' matchBody
    ;

matchBody
    : expression
    | block
    ;

pattern
    : guardedPattern
    ;

guardedPattern
    : basePattern ('if' expression)?
    ;

basePattern
    : literalPattern
    | rangePattern
    | wildcardPattern
    | bindingPattern
    | tuplePattern
    ;
```

Match can be used as both expression and statement. Pattern types:

| Pattern | Syntax | Example |
|---------|--------|---------|
| Literal | `value` | `42`, `"hello"`, `Color.Red`, `null` |
| Range | `low..high` | `1..10` (integers only, inclusive) |
| Wildcard | `_` | `_` (matches anything) |
| Binding | `var name` | `var x` (binds matched value to `x`) |
| Tuple | `(pat, pat, ...)` | `(var a, var b)` |
| Guarded | `pattern if condition` | `var x if x > 0` |

```mingus
var result = match status {
    0 => "ok",
    1..10 => "warning",
    var x if x > 100 => "critical",
    _ => "unknown",
};
```

### 3.14 Type System Grammar

```
typeIdentifier
    : primitiveType typeModifier*
    | qualifiedName typeModifier*
    | tupleType typeModifier*
    | functionType typeModifier*
    ;

primitiveType : 'int' | 'double' | 'float' | 'byte' | 'string' | 'char' | 'bool' | 'void'

typeModifier
    : arrayDimension       // [N] or []
    | pointerLevel         // *
    | referenceLevel       // &
    ;

arrayDimension : '[' IntegerLiteral? ']'
pointerLevel   : '*'
referenceLevel : '&'

functionType : '(' typeList? ')' '=>' returnType
tupleType    : '(' typeIdentifier ',' typeIdentifier (',' typeIdentifier)* ')'

returnType : typeIdentifier | tupleType
```

Type modifiers stack left-to-right on the base type:

| Source | Parsed As |
|--------|-----------|
| `int` | `PrimitiveTypeNode(Int)` |
| `int*` | `PointerTypeNode(PrimitiveTypeNode(Int))` |
| `int&` | `PointerTypeNode(PrimitiveTypeNode(Int), isReference=true)` |
| `int[]` | `ArrayTypeNode(PrimitiveTypeNode(Int), unsized)` |
| `int[16]` | `ArrayTypeNode(PrimitiveTypeNode(Int), size=16)` |
| `int[]*` | `PointerTypeNode(ArrayTypeNode(PrimitiveTypeNode(Int)))` |
| `MyStruct` | `NamedTypeNode(["MyStruct"])` |
| `Module.Type` | `NamedTypeNode(["Module", "Type"])` |
| `(int, string)` | `TupleTypeNode([int, string])` |
| `(int) => double` | `FunctionTypeNode([int], double)` |
| `() => void` | `FunctionTypeNode([], void)` |

### 3.15 Modifiers

```
accessModifier   : 'public' | 'private' | 'protected'
staticModifier   : 'static'
abstractModifier : 'abstract'
```

### 3.16 Qualified Names

```
qualifiedName : Identifier ('.' Identifier)*
```

Used for module-qualified references: `Math.sin`, `Color.Red`, `ModuleName`.

---

## 4. Operator Precedence

From highest to lowest:

| Level | Category | Operators | Associativity |
|-------|----------|-----------|---------------|
| 1 | Postfix | `()` `[]` `.` `->` `++` `--` | Left |
| 2 | Prefix / Unary | `++` `--` `+` `-` `!` `~` `&` `*` `sizeof` `alignof` | Right |
| 3 | Cast | `(type)expr` | Right |
| 4 | Multiplicative | `*` `/` `%` | Left |
| 5 | Additive | `+` `-` | Left |
| 6 | Shift | `<<` `>>` | Left |
| 7 | Relational | `<` `<=` `>` `>=` | Left |
| 8 | Equality | `==` `!=` | Left |
| 9 | Bitwise AND | `&` | Left |
| 10 | Bitwise XOR | `^` | Left |
| 11 | Bitwise OR | `\|` | Left |
| 12 | Logical AND | `&&` | Left |
| 13 | Logical OR | `\|\|` | Left |
| 14 | Ternary | `? :` | Right |
| 15 | Pipe | `\|>` | Left |
| 16 | Assignment | `=` `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` | Right |
| 17 | Lambda | `[captures](params) => body` | -- |

This matches C# operator precedence. The pipe operator sits between ternary and assignment, which means `x |> f = y` parses as `(x |> f) = y` and `a ? b : c |> f` parses as `a ? b : (c |> f)`.

---

## 5. AST Node Hierarchy

### 5.1 Base Classes

Defined in `include/mingus/AstNode.h`:

```
AstBaseNode                          // Root of all AST nodes
+-- ExpressionBaseNode               // Base for all expressions
+-- StatementBaseNode                // Base for all statements
|   +-- DeclarationBaseNode          // Base for all declarations (IS-A statement)
+-- BlockStatementNode               // { stmt; stmt; ... }
+-- TypeNode                         // Base for type annotations
|   +-- PrimitiveTypeNode            // int, double, etc.
|   +-- NamedTypeNode                // User types, qualified names
|   +-- PointerTypeNode              // T* or T& (isReference flag)
|   +-- ArrayTypeNode                // T[] or T[N]
|   +-- TupleTypeNode                // (T1, T2, ...)
|   +-- FunctionTypeNode             // (T1, T2) => R
+-- PatternNode                      // Base for match patterns
|   +-- LiteralPattern               // 42, "hello", Color.Red
|   +-- IdentifierPattern            // var x, with optional guard
|   +-- WildcardPattern              // _
|   +-- RangePattern                 // 1..10
+-- ParameterNode                    // Function/lambda parameter
+-- ArgumentsNode                    // Call argument list
+-- ModifiersNode                    // Access/static/abstract modifiers
+-- ModuleNode                       // module Name { ... }
+-- ProgramNode                      // Root (owns all modules)
```

#### AstBaseNode

Every AST node carries:

| Field | Type | Purpose |
|-------|------|---------|
| `astScopeNode` | `ScopePtr` | Pointer to the scope this node lives in (set by Pass 1) |
| `debugInfo` | `shared_ptr<DebugInfo>` | Full source range for diagnostics and debug info |

Helper methods: `as<T>()` for downcasting, `is<T>()` for type checking, `accept(ASTVisitor&)` for the visitor pattern.

#### ExpressionBaseNode

Adds to AstBaseNode:

| Field | Type | Purpose |
|-------|------|---------|
| `resolvedType` | `TypeSymbolPtr` | Result type (set by Pass 3, TypeChecker) |
| `resolvedSymbol` | `SymbolPtr` | Resolved symbol (set by Pass 2/3 for identifiers, member access) |

#### DeclarationBaseNode : StatementBaseNode

Key design: declarations extend `StatementBaseNode`. This means declarations can appear wherever statements can, without needing wrapper nodes. A `VariableDeclaration` inside a function body is directly a statement in the block's `statements` vector.

### 5.2 DebugInfo

Defined in `include/mingus/DebugInfo.h`:

| Field | Type |
|-------|------|
| `lineNumber`, `columnNumber` | `int` | Primary location (for error messages) |
| `lineNumberStart`, `columnNumberStart` | `int` | Range start |
| `lineNumberEnd`, `columnNumberEnd` | `int` | Range end |
| `sourceFile` | `string` | Source file path |

Factory methods: `fromToken(line, col, length)` creates a single-line range; `merge(a, b)` computes the union of two ranges.

### 5.3 Expression Nodes

Defined in `include/mingus/Expressions.h`:

#### Literals

| Node | Fields | Description |
|------|--------|-------------|
| `IntegerLiteral` | `int64_t value` | Integer constant (decimal, hex, binary, octal) |
| `FloatLiteral` | `double value` | Floating-point constant |
| `BoolLiteral` | `bool value` | `true` or `false` |
| `CharLiteral` | `char value` | Character constant |
| `StringLiteral` | `string value` | Plain string (no interpolation) |
| `NullLiteral` | -- | The `null` keyword |
| `InterpolatedStringExpression` | `vector<InterpolatedPart> parts` | String with `${expr}` interpolation |

`InterpolatedPart` has `kind` (`Text` or `Expression`), `text` (for text parts), and `expression` (for expression parts).

#### Identifiers and Names

| Node | Fields | Description |
|------|--------|-------------|
| `IdentifierExpression` | `string name` | Simple identifier: `x`, `foo` |
| `QualifiedNameExpression` | `vector<string> parts`, `isEnumAccess`, `resolvedEnumValue`, `resolvedEnumStringValue`, `isStringEnumAccess` | Dotted name: `Module.func`, `Color.Red` |
| `ThisExpression` | -- | The `this` keyword |

`QualifiedNameExpression` carries fields for enum access resolution, set by the TypeChecker when the qualified name resolves to an enum member.

#### Member Access

| Node | Fields | Description |
|------|--------|-------------|
| `MemberAccessExpression` | `object`, `string memberName`, `bool isArrow` | `obj.field` or `ptr->field` |

Sema-resolved annotations on `MemberAccessExpression`:

| Field | Type | Purpose |
|-------|------|---------|
| `resolvedEnumValue` | `int64_t` | Enum member integer value |
| `resolvedEnumStringValue` | `string` | Enum member string value |
| `isEnumAccess` | `bool` | True if this is an enum member access |
| `isStringEnumAccess` | `bool` | True if enum has string underlying type |
| `isStringBuiltinMethod` | `bool` | True for string builtins (length, substring) |
| `isStaticAccess` | `bool` | True for static method dispatch |

#### Operators

| Node | Fields | Description |
|------|--------|-------------|
| `BinaryExpression` | `left`, `BinaryOp op`, `right`, `bool isOperatorOverload`, `resolvedOperatorFunction` | `a + b`, `x == y` |
| `UnaryExpression` | `UnaryOp op`, `operand` | `-x`, `!flag`, `*ptr`, `&val`, `++i`, `i--` |
| `AssignmentExpression` | `target`, `AssignOp op`, `value` | `x = 42`, `y += 1` |
| `TernaryExpression` | `condition`, `thenExpr`, `elseExpr` | `cond ? a : b` |

**BinaryOp enum:** `Add`, `Sub`, `Mul`, `Div`, `Mod`, `Equal`, `NotEqual`, `Less`, `LessEqual`, `Greater`, `GreaterEqual`, `LogicalAnd`, `LogicalOr`, `BitwiseAnd`, `BitwiseOr`, `BitwiseXor`, `ShiftLeft`, `ShiftRight`

**UnaryOp enum:** `Negate`, `LogicalNot`, `BitwiseNot`, `AddressOf`, `Dereference`, `PreIncrement`, `PreDecrement`, `PostIncrement`, `PostDecrement`

**AssignOp enum:** `Assign`, `AddAssign`, `SubAssign`, `MulAssign`, `DivAssign`, `ModAssign`, `AndAssign`, `OrAssign`, `XorAssign`, `ShiftLeftAssign`, `ShiftRightAssign`

#### Call and Index

| Node | Fields | Description |
|------|--------|-------------|
| `CallExpression` | `callee`, `shared_ptr<ArgumentsNode> arguments`, `shared_ptr<FunctionSymbol> resolvedCallee` | `f(a, b)`, `obj.method(x)` |
| `IndexExpression` | `object`, `index`, `bool isOperatorOverload`, `resolvedOperatorFunction` | `arr[i]` |

`CallExpression::resolvedCallee` is set by Pass 3 (TypeChecker) for direct function calls. It is null for indirect/closure calls.

`ArgumentsNode` carries:
- `vector<shared_ptr<ExpressionBaseNode>> expressions` -- the argument expressions
- `vector<bool> isReference` -- per-argument reference flag (set by TypeChecker, read by codegen)

#### Type Operations

| Node | Fields | Description |
|------|--------|-------------|
| `CastExpression` | `shared_ptr<TypeNode> targetType`, `operand` | `(double)x` |
| `NewExpression` | `shared_ptr<TypeNode> type`, `shared_ptr<ArgumentsNode> arguments`, `bool isArray`, `arraySize` | `new Dog("Rex")`, `new int[100]` |
| `SizeOfExpression` | `shared_ptr<TypeNode> targetType` | `sizeof(int)` |

#### Tuple, Match, Pipe, Lambda

| Node | Fields | Description |
|------|--------|-------------|
| `TupleExpression` | `vector<shared_ptr<ExpressionBaseNode>> elements` | `(1, "hello")` |
| `MatchExpression` | `subject`, `vector<MatchArm> arms` | `match x { ... }` |
| `PipeExpression` | `input`, `vector<PipeStage> stages` | `x \|> f \|> g(a)` |
| `LambdaExpression` | see below | `[=](int x) => x * 2` |

**MatchArm struct:** `shared_ptr<PatternNode> pattern`, `shared_ptr<AstBaseNode> body` (expression or block).

**PipeStage struct:** `shared_ptr<ExpressionBaseNode> function`, `vector<shared_ptr<ExpressionBaseNode>> extraArguments`.

**LambdaExpression fields:**

| Field | Type | Set By | Description |
|-------|------|--------|-------------|
| `parameters` | `vector<shared_ptr<ParameterNode>>` | Parser | Lambda parameters |
| `body` | `shared_ptr<AstBaseNode>` | Parser | Block or expression body |
| `captureDefault` | `CaptureDefault` | Parser | `None`, `ByCopy`, or `ByRef` |
| `captureItems` | `vector<CaptureItem>` | Parser | Explicit capture items |
| `capturedVariables` | `vector<SymbolPtr>` | Pass 4 | Resolved captured variable symbols |
| `captureModesResolved` | `vector<CaptureMode>` | Pass 4 | Per-variable capture mode |
| `escapes` | `bool` | Pass 4 | False for non-escaping lambdas (stack-allocated) |
| `selfCapture` | `bool` | Pass 4 | True for letrec pattern (lambda captures itself) |

**CaptureDefault enum:** `None` (for `[]`), `ByCopy` (for `[=]`), `ByRef` (for `[&]`).

**CaptureMode enum:** `ByValue`, `ByReference`.

#### VariableDeclarationExpression

| Node | Fields | Description |
|------|--------|-------------|
| `VariableDeclarationExpression` | `name`, `accessModifier`, `isStatic`, `type`, `isInferred`, `initializer`, `resolvedVariable` | Variable declaration in expression context |

This is the expression-level variant of `VariableDeclaration`. Both produce a `VariableSymbol` during Pass 1.

### 5.4 Statement Nodes

Defined in `include/mingus/Statements.h` (with `BlockStatementNode` in `AstNode.h`):

| Node | Fields | Description |
|------|--------|-------------|
| `BlockStatementNode` | `vector<shared_ptr<StatementBaseNode>> statements` | `{ stmt; stmt; ... }` |
| `ExpressionStatement` | `shared_ptr<ExpressionBaseNode> expression` | Expression used as statement |
| `ReturnStatement` | `shared_ptr<ExpressionBaseNode> value` | `return expr;` or `return;` (value is null for void) |
| `IfStatement` | `condition`, `thenBody`, `vector<ElseIfClause> elseIfClauses`, `elseBody` | `if/else if/else` |
| `ForStatement` | `initDeclarations`, `initExpressions`, `condition`, `iterators`, `body` | `for (...;...;...) { }` |
| `WhileStatement` | `condition`, `body` | `while (cond) { }` |
| `BreakStatement` | -- | `break;` |
| `ContinueStatement` | -- | `continue;` |
| `DeleteStatement` | `shared_ptr<ExpressionBaseNode> target` | `delete expr;` |
| `SwitchStatement` | `subject`, `vector<SwitchCase> cases`, `vector<shared_ptr<StatementBaseNode>> defaultCase` | `switch { case: ... }` |

**ForStatement detail:** `initDeclarations` and `initExpressions` are mutually exclusive. If the for-init uses variable declarations, `initDeclarations` is populated; if it uses expressions, `initExpressions` is populated. The condition can be null (infinite loop).

**Supporting structs:**
- `ElseIfClause`: `condition`, `body`
- `SwitchCase`: `value` (null for default), `vector<StatementBaseNode> body`

### 5.5 Declaration Nodes

Defined in `include/mingus/Declarations.h` (with `ModuleNode` in `AstNode.h`):

| Node | Fields | Description |
|------|--------|-------------|
| `VariableDeclaration` | `name`, `accessModifier`, `isStatic`, `isConst`, `type`, `isInferred`, `initializer`, `resolvedVariable` | Variable/field/const declaration |
| `TupleDestructuringDeclaration` | `vector<DestructureElement> elements`, `initializer`, `resolvedVariables` | `(var a, var b) = expr;` |
| `FunctionDeclaration` | `name`, `accessModifier`, `isStatic`, `isAbstract`, `isVirtual`, `isOverride`, `parameters`, `returnType`, `body`, `resolvedFunction` | Function/method declaration |
| `ConstructorDeclaration` | `accessModifier`, `parameters`, `body`, `superArgs`, `hasSuperCall`, `resolvedConstructor` | `constructor(params) : super(args) { }` |
| `DestructorDeclaration` | `body`, `resolvedDestructor` | `destructor { }` |
| `ExternFunctionDeclaration` | `name`, `parameters`, `returnType`, `bool isVariadic`, `resolvedFunction` | `extern func f(...) => T;` |
| `OperatorDeclaration` | `OverloadableOp op`, `parameters`, `returnType`, `body`, `resolvedOperator` | `func operator+(T) => T { }` |
| `EnumMemberNode` | `string name`, `shared_ptr<ExpressionBaseNode> value` | `Red = 0` |
| `EnumDeclaration` | `name`, `accessModifier`, `underlyingType`, `vector<EnumMemberNode> members`, `resolvedEnum` | `enum Color : int { }` |
| `StructDeclaration` | `name`, `accessModifier`, `fields`, `methods`, `operators`, `resolvedStruct` | `struct Vec2 { }` |
| `ClassDeclaration` | `name`, `accessModifier`, `isStatic`, `isAbstract`, `baseClasses`, `fields`, `methods`, `operators`, `constructor`, `destructor`, `resolvedClass` | `class Dog : Animal { }` |
| `InterfaceDeclaration` | `name`, `accessModifier`, `methods`, `resolvedInterface` | `interface Drawable { }` |
| `ImportDeclaration` | `vector<ImportTarget> targets`, `vector<string> sourcePath`, `bool isWholeModule` | `import X from M;` |

**OverloadableOp enum:** `Add`, `Sub`, `Mul`, `Div`, `Mod`, `Equal`, `NotEqual`, `Less`, `Greater`, `LessEq`, `GreaterEq`, `Negate`, `Index`

**DestructureElement struct:** `string name`, `shared_ptr<TypeNode> type`, `bool isInferred`

**ImportTarget struct:** `string name`, `optional<string> alias`

### 5.6 Type Nodes

Defined in `include/mingus/AstNode.h`:

| Node | Fields | Description |
|------|--------|-------------|
| `TypeNode` | `TypeSymbolPtr resolvedType` | Base type annotation (resolved by Pass 2) |
| `PrimitiveTypeNode` | `PrimitiveKind kind` | `int`, `double`, `float`, `byte`, `string`, `char`, `bool`, `void` |
| `NamedTypeNode` | `vector<string> qualifiedName` | `MyStruct`, `Module.Type` |
| `PointerTypeNode` | `shared_ptr<TypeNode> baseType`, `bool isReference` | `T*` (isReference=false) or `T&` (isReference=true) |
| `ArrayTypeNode` | `shared_ptr<TypeNode> elementType`, `shared_ptr<ExpressionBaseNode> sizeExpr` | `T[]` (sizeExpr=null) or `T[16]` |
| `TupleTypeNode` | `vector<shared_ptr<TypeNode>> elementTypes` | `(int, string)` |
| `FunctionTypeNode` | `vector<shared_ptr<TypeNode>> parameterTypes`, `shared_ptr<TypeNode> returnType` | `(int) => double` |

**PrimitiveKind enum:** `Int`, `Double`, `Float`, `Byte`, `Char`, `String`, `Bool`, `Void`

Note: `PointerTypeNode` serves double duty for both pointer types (`T*`) and reference types (`T&`). The `isReference` flag distinguishes them. The TypeResolver (Pass 2) unwraps reference types during parameter resolution.

### 5.7 Pattern Nodes

Defined in `include/mingus/AstNode.h`:

| Node | Fields | Description |
|------|--------|-------------|
| `LiteralPattern` | `shared_ptr<ExpressionBaseNode> value` | Matches an exact value |
| `IdentifierPattern` | `string name`, `shared_ptr<ExpressionBaseNode> guard`, `shared_ptr<VariableSymbol> resolvedSymbol` | Binds matched value; optional guard |
| `WildcardPattern` | -- | Matches anything (`_`) |
| `RangePattern` | `shared_ptr<ExpressionBaseNode> low`, `shared_ptr<ExpressionBaseNode> high` | Integer range `low..high` |

The grammar defines a `tuplePattern` rule but the V2 ASTGenerator reports an error for tuple patterns (not yet implemented in V2).

Guard expressions are attached to `IdentifierPattern` only. In the grammar, any `basePattern` can have a guard (`if condition`), but the ASTGenerator only sets the guard on `IdentifierPattern` nodes.

### 5.8 ParameterNode

Defined in `include/mingus/AstNode.h`:

| Field | Type | Description |
|-------|------|-------------|
| `name` | `string` | Parameter name |
| `type` | `shared_ptr<TypeNode>` | Parameter type (null for untyped lambda params) |
| `isReference` | `bool` | True for `T&` parameters |
| `defaultValue` | `shared_ptr<ExpressionBaseNode>` | Default value (null if none) |
| `resolvedSymbol` | `shared_ptr<VariableSymbol>` | Linked to the parameter's variable symbol (set by Pass 1) |

The `resolvedSymbol` link eliminates the need for `scanForParamSymbols` in codegen -- a key V2 improvement over V1.

### 5.9 Supporting Structs

Defined in `include/mingus/AstNode.h`:

| Struct | Fields | Used By |
|--------|--------|---------|
| `CaptureItem` | `string name`, `CaptureMode mode` | `LambdaExpression::captureItems` |
| `InterpolatedPart` | `InterpolatedPartKind kind`, `string text`, `ExpressionBaseNode expression` | `InterpolatedStringExpression::parts` |
| `MatchArm` | `PatternNode pattern`, `AstBaseNode body` | `MatchExpression::arms` |
| `PipeStage` | `ExpressionBaseNode function`, `vector<ExpressionBaseNode> extraArguments` | `PipeExpression::stages` |
| `ElseIfClause` | `ExpressionBaseNode condition`, `StatementBaseNode body` | `IfStatement::elseIfClauses` |
| `SwitchCase` | `ExpressionBaseNode value`, `vector<StatementBaseNode> body` | `SwitchStatement::cases` |
| `DestructureElement` | `string name`, `TypeNode type`, `bool isInferred` | `TupleDestructuringDeclaration::elements` |
| `ImportTarget` | `string name`, `optional<string> alias` | `ImportDeclaration::targets` |

### 5.10 Complete AST Node Count

The V2 AST has **62 named node types** (including base classes, type nodes, pattern nodes, and supporting nodes):

- 4 base classes: `AstBaseNode`, `ExpressionBaseNode`, `StatementBaseNode`, `DeclarationBaseNode`
- 3 structural: `ProgramNode`, `ModuleNode`, `BlockStatementNode`
- 3 support: `ParameterNode`, `ArgumentsNode`, `ModifiersNode`
- 7 type nodes: `TypeNode`, `PrimitiveTypeNode`, `NamedTypeNode`, `PointerTypeNode`, `ArrayTypeNode`, `TupleTypeNode`, `FunctionTypeNode`
- 4 pattern nodes: `LiteralPattern`, `IdentifierPattern`, `WildcardPattern`, `RangePattern`
- 7 literal expressions: `IntegerLiteral`, `FloatLiteral`, `BoolLiteral`, `CharLiteral`, `StringLiteral`, `NullLiteral`, `InterpolatedStringExpression`
- 18 other expressions: `IdentifierExpression`, `QualifiedNameExpression`, `ThisExpression`, `MemberAccessExpression`, `BinaryExpression`, `UnaryExpression`, `AssignmentExpression`, `TernaryExpression`, `IndexExpression`, `CallExpression`, `CastExpression`, `NewExpression`, `SizeOfExpression`, `TupleExpression`, `MatchExpression`, `PipeExpression`, `LambdaExpression`, `VariableDeclarationExpression`
- 8 statements: `ExpressionStatement`, `ReturnStatement`, `IfStatement`, `ForStatement`, `WhileStatement`, `BreakStatement`, `ContinueStatement`, `DeleteStatement`, `SwitchStatement`
- 13 declarations: `VariableDeclaration`, `TupleDestructuringDeclaration`, `FunctionDeclaration`, `ConstructorDeclaration`, `DestructorDeclaration`, `ExternFunctionDeclaration`, `OperatorDeclaration`, `EnumMemberNode`, `EnumDeclaration`, `StructDeclaration`, `ClassDeclaration`, `InterfaceDeclaration`, `ImportDeclaration`

---

## 6. ASTGenerator Mapping

The `ASTGenerator` class (`src/mingus/parser/ASTGenerator.cpp`) extends `MingusParserBaseVisitor` and implements visitor methods for every grammar rule. It produces V2 AST nodes using default-constructed shared pointers with field assignment.

### 6.1 Entry Point

```cpp
std::shared_ptr<ProgramNode> ASTGenerator::generate(MingusParser::ProgramContext* ctx)
```

Calls `visitProgram()`, extracts the `ProgramNode`, and collects any errors.

### 6.2 Key Grammar-to-AST Conversions

| Grammar Rule | AST Node | Notes |
|--------------|----------|-------|
| `program` | `ProgramNode` | Contains `vector<ModuleNode>` |
| `module` | `ModuleNode` | Name + declarations |
| `importDefinition` | `ImportDeclaration` | Targets + source path + isWholeModule |
| `classDeclaration` | `ClassDeclaration` | Parses members, inheritance |
| `structDeclaration` | `StructDeclaration` | Fields, methods, operators |
| `enumDeclaration` | `EnumDeclaration` | Members with optional values |
| `interfaceDeclaration` | `InterfaceDeclaration` | Abstract methods |
| `functionDeclaration` | `FunctionDeclaration` | Expression body wrapped in `Block{Return}` |
| `constructorDeclaration` | `ConstructorDeclaration` | Super call args extracted |
| `destructorDeclaration` | `DestructorDeclaration` | Body only |
| `operatorDeclaration` | `OperatorDeclaration` | Expression body wrapped in `Block{Return}` |
| `externFunctionDeclaration` | `ExternFunctionDeclaration` | isVariadic from Ellipsis token |
| `typedVariableDeclaration` | `VariableDeclaration` | Construction `T x(args)` becomes `CallExpression` init |
| `inferredVariableDeclaration` | `VariableDeclaration` | `isInferred = true` |
| `constVariableDeclaration` | `VariableDeclaration` | `isConst = true` |
| `tupleDestructuring` | `TupleDestructuringDeclaration` | |
| `forStatement` | `ForStatement` | Multi-init via `localVarInitializer` |
| `whileStatement` | `WhileStatement` | |
| `ifStatement` | `IfStatement` | Else-if clauses chained |
| `switchStatement` | `SwitchStatement` | Cases + optional default |
| `matchStatement` | `ExpressionStatement { MatchExpression }` | Match-as-statement wrapped |
| `rawBlock` | `BlockStatementNode` | No separate RawBlock node in V2 |
| `assignment` (with lambda RHS) | `AssignmentExpression` | `f = [=](x) => {...}` works |
| `lambdaExpression` | `LambdaExpression` | Captures parsed, body as expression or block |
| `pipe` | `PipeExpression` | Stages with optional member access chains |
| Binary operator rules | `BinaryExpression` chain | Left-associative chaining |
| `castExpression` | `CastExpression` | C-style cast |
| `unaryExpression` | `UnaryExpression` | Prefix ops, sizeof |
| `postfixExpression` | Chained nodes | `Call`/`Index`/`MemberAccess`/`UnaryExpression` |
| `primaryExpression` (integer) | `IntegerLiteral` | Handles decimal/hex/binary/octal |
| `string` with interpolation | `InterpolatedStringExpression` | Plain strings become `StringLiteral` |
| `newExpression` | `NewExpression` | Object or array form |
| `matchExpression` | `MatchExpression` | Arms with patterns and bodies |
| `tupleExpression` | `TupleExpression` | |

### 6.3 Special Conversions

#### Expression-Bodied Functions and Operators

When `functionDeclaration` or `operatorDeclaration` uses the expression body form (`=> expression ;`), the ASTGenerator wraps the expression in a `BlockStatementNode` containing a `ReturnStatement`:

```
func square(int x) => int => x * x;
// Becomes: FunctionDeclaration { body: Block { Return { BinaryExpression{x * x} } } }
```

#### Constructor-Style Variable Initialization

`Type name(args);` is converted to a `VariableDeclaration` with a `CallExpression` initializer:

```
Point p(1.0, 2.0);
// Becomes: VariableDeclaration { name: "p", type: NamedTypeNode("Point"),
//          initializer: CallExpression { callee: IdentifierExpression("Point"),
//                                        arguments: [1.0, 2.0] } }
```

#### Integer Literal Parsing

The `parseIntegerLiteral()` function handles all numeric bases:

```cpp
static int64_t parseIntegerLiteral(const std::string& text) {
    if (text.size() >= 2 && text[0] == '0') {
        char p = text[1];
        if (p == 'b' || p == 'B') return std::stoll(text.substr(2), nullptr, 2);
        if (p == 'o' || p == 'O') return std::stoll(text.substr(2), nullptr, 8);
    }
    return std::stoll(text, nullptr, 0);  // handles 0x natively
}
```

#### String Processing

The `visitString()` method accumulates text parts and interpolation parts, then decides the node type:
- All text, no interpolation: `StringLiteral`
- Any `${...}` parts: `InterpolatedStringExpression`

Escape sequences are processed during accumulation: `\n` becomes newline, `\t` becomes tab, `\r` becomes carriage return, `\\` becomes backslash, `\"` becomes quote, `\'` becomes single quote, `\0` becomes null character, and other `\X` sequences become the literal character `X`.

#### Postfix Chaining

`visitPostfixExpression()` starts with a primary expression and iterates over postfix operations, wrapping the accumulator at each step:

```
obj.method(args)[0].field++
// Step 0: result = IdentifierExpression("obj")
// Step 1: result = MemberAccessExpression(result, "method")
// Step 2: result = CallExpression(result, [args])
// Step 3: result = IndexExpression(result, 0)
// Step 4: result = MemberAccessExpression(result, "field")
// Step 5: result = UnaryExpression(PostIncrement, result)
```

#### For Loop Multi-Init

The ASTGenerator iterates over all `localVarDeclaration` children in the `localVarInitializer`, creating a `VariableDeclaration` for each. These are stored in `ForStatement::initDeclarations`:

```mingus
for (int i = 0, int j = 10; i < j; i++, j--)
// ForStatement {
//   initDeclarations: [VariableDeclaration("i", int, 0), VariableDeclaration("j", int, 10)]
//   condition: BinaryExpression(i < j)
//   iterators: [UnaryExpression(i++), UnaryExpression(j--)]
// }
```

#### Pipe Target Member Access

When a pipe target includes member access (e.g., `x |> obj.method` or `x |> ptr->method`), the ASTGenerator:

1. Builds the base expression from the `qualifiedName` (as `IdentifierExpression` for single names, `QualifiedNameExpression` for dotted names).
2. Walks the children of the `pipeTarget` node to pair `.`/`->` operators with their corresponding `Identifier` tokens.
3. Chains `MemberAccessExpression` nodes, setting `isArrow` based on whether the operator was `->` or `.`.

#### Lambda Capture Parsing

The ASTGenerator reads the `captureList` context and populates:
- `lambda->captureDefault`: `ByCopy` for `=`, `ByRef` for `&`, `None` for empty brackets.
- `lambda->captureItems`: Each item has a `name` and `mode` (`ByReference` if prefixed with `&`, else `ByValue`).

#### Match-as-Statement

When a `match` expression is used as a statement (via `matchStatement`), the ASTGenerator wraps the `MatchExpression` in an `ExpressionStatement`:

```mingus
match x { ... };
// Becomes: ExpressionStatement { expression: MatchExpression { ... } }
```

#### Raw Block

In V2, raw blocks produce a plain `BlockStatementNode` -- there is no separate `RawBlock` AST node. The `raw` keyword is consumed but not represented in the AST.

#### Declaration as Statement

Since `DeclarationBaseNode` extends `StatementBaseNode`, variable declarations inside function bodies flow directly into the block's `statements` vector without any wrapper node. The `visitStatement()` method calls `visitVariableDeclaration()` directly, and the returned `DeclarationBaseNode` is a valid `StatementBaseNode`.

---

## 7. Key Design Decisions

### 7.1 Mandatory Capture Lists

Unlike C++ where lambdas can optionally have capture lists, Mingus **requires** explicit capture lists on all lambdas. This makes closure behavior explicit and avoids surprising implicit captures. The syntax mirrors C++:

```mingus
[=](int x) => x + captured_var      // all captures by value
[&count](int x) => { count++; }     // explicit reference capture
[]() => 42                           // no captures (pure function)
```

### 7.2 Pipe Target as Expression

The pipe target grammar allows member access chains:

```
pipeTarget : qualifiedName (('.' | '->') Identifier)* callArguments?
```

This enables method-style piping: `x |> obj.method(extra_args)`. The qualified name plus member access chain is converted to nested `MemberAccessExpression` nodes, which the `PipeStage` stores as its `function` field. Extra arguments (beyond the piped value) are stored in `extraArguments`.

### 7.3 QualifiedNameExpression

Dotted names like `Module.func` or `Color.Red` are represented as `QualifiedNameExpression` with a `parts` vector. This avoids prematurely deciding whether the dotted name is a module-qualified function, an enum member access, or a static method call -- that determination is made during semantic analysis.

### 7.4 Declarations ARE Statements

`DeclarationBaseNode` extends `StatementBaseNode`, not `AstBaseNode` directly. This means variable declarations, function declarations, and other declarations are valid statements. A `VariableDeclaration` inside a function body appears directly in the `BlockStatementNode::statements` vector without any wrapper. This simplifies the AST compared to designs that require separate "declaration statement" wrapper nodes.

### 7.5 ArgumentsNode with Per-Arg isReference

`CallExpression` uses `ArgumentsNode` instead of a plain expression vector. Each argument slot has a corresponding `isReference` flag set by the TypeChecker (Pass 3). This means codegen can read `arguments->isReference[i]` directly without re-resolving the callee function -- a significant simplification over V1.

### 7.6 ParameterNode::resolvedSymbol

Each `ParameterNode` carries a `resolvedSymbol` pointer set by the SymbolTableBuilder (Pass 1). This eliminates the V1 `scanForParamSymbols()` pass that had to scan lambda bodies to map parameter names to allocas. Now codegen reads `param->resolvedSymbol` directly.

### 7.7 PointerTypeNode for Both Pointers and References

Rather than having separate `PointerTypeNode` and `ReferenceTypeNode` in the AST, the parser produces `PointerTypeNode` for both `T*` and `T&`, distinguished by the `isReference` flag. The semantic passes unwrap reference types: the TypeResolver sets `VariableSymbol::isReference = true` and stores the base type, not the reference wrapper.

### 7.8 Expression Body Desugaring

Expression-bodied functions and operators (`=> expr ;`) are desugared by the ASTGenerator into `Block { Return { expr } }`. This means the rest of the compiler (sema, codegen) only ever sees block-bodied functions, simplifying downstream processing.

### 7.9 Lambda Body Duality

Lambda bodies can be either `ExpressionBaseNode` or `BlockStatementNode`, stored in a `shared_ptr<AstBaseNode>`. The `LambdaExpression` provides `hasExpressionBody()` and `hasBlockBody()` helpers using `is<T>()` checks. Unlike functions, expression-bodied lambdas are NOT desugared to blocks -- codegen handles both forms.

---

## 8. Grammar Limitations

- **No do-while loop.** Only `for` and `while` loops are supported.
- **No labeled break/continue.** Breaking out of nested loops requires restructuring.
- **No generics/templates.** All types are concrete.
- **No typedef or using alias.** Type aliases are not supported.
- **No multiple return types.** Functions return a single type (tuples can be used as a workaround).
- **Range patterns are integer-only.** `1..10` works, but `'a'..'z'` or `0.0..1.0` do not.
- **Char literal escapes are raw.** The ASTGenerator reads `text[1]` without processing escape sequences in character literals, so `'\n'` produces `\` instead of a newline character.
- **Lambda params can omit types.** Untyped lambda params produce `ParameterNode` with null type; type inference for lambda parameters is not supported and sema will reject them.
- **Tuple patterns not implemented in V2.** The grammar rule exists but the ASTGenerator reports an error.
- **Guard patterns only on bindings.** The grammar allows guards on any pattern, but the ASTGenerator only attaches guards to `IdentifierPattern` nodes.
- **No `for-each` or range-based for.** Only C-style `for(init; cond; iter)` is supported.
- **No string escape in char literals.** Escape sequences like `'\n'` are not properly decoded.
- **Single inheritance only.** Classes support one base class (plus interfaces via the comma-separated list after `:`).
