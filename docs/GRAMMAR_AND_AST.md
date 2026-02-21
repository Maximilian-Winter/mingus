# Mingus Grammar and AST Reference

This document describes the Mingus grammar (ANTLR4) and the AST node types produced by the parser. It covers syntax rules, operator precedence, and the complete AST node inventory.

**Source files:**
- `grammar/MingusLexer.g4` — Lexer rules
- `grammar/MingusParser.g4` — Parser rules
- `src/mingus/parser/ASTGenerator.cpp` — Parse tree to AST conversion
- `include/mingus/AstNode.h`, `Expressions.h`, `Statements.h`, `Declarations.h` — AST node headers

---

## Table of Contents

1. [Lexer Overview](#1-lexer-overview)
2. [Grammar Rules](#2-grammar-rules)
3. [Operator Precedence](#3-operator-precedence)
4. [AST Node Inventory](#4-ast-node-inventory)
5. [ASTGenerator Mapping](#5-astgenerator-mapping)
6. [Grammar Limitations](#6-grammar-limitations)

---

## 1. Lexer Overview

The lexer uses **modes** to handle string interpolation:

- **Default mode:** Keywords, operators, identifiers, numeric literals
- **String mode (`IN_STRING`):** Entered on `"`, processes text, escape sequences, and `${` interpolation openers
- **Interpolation mode (`IN_INTERPOLATION`):** Entered on `${`, parses a full expression, exits on `}`

### Keywords

```
module  import  from  as  extern  func  return  var
struct  class   interface  enum  constructor  destructor  operator
if  else  for  while  switch  case  default  break  continue
new  delete  raw  match  null  this  true  false
public  private  protected  static  abstract
sizeof  alignof
```

### Numeric Literals

| Format | Example | Notes |
|--------|---------|-------|
| Decimal | `42`, `1000` | Standard integer |
| Hex | `0xFF`, `0x1A2B` | Prefix `0x` or `0X` |
| Binary | `0b1010`, `0B1111` | Prefix `0b` or `0B` |
| Octal | `0o777`, `0O644` | Prefix `0o` or `0O` |
| Float | `3.14`, `1.0e-5` | Decimal point or exponent |

### String Literals

```
"hello"                    // plain string
"value = ${x + 1}"        // interpolated string
"name: ${person.name}"    // member access in interpolation
```

Escape sequences: `\n`, `\t`, `\r`, `\\`, `\"`, `\$`.

---

## 2. Grammar Rules

### 2.1 Program Structure

```
program    : module* EOF
module     : 'module' Identifier '{' moduleBody '}'
moduleBody : (importDefinition | declaration)*
```

A program is one or more modules. Each module contains imports and declarations.

### 2.2 Imports

```
importDefinition : 'import' importTarget (',' importTarget)* 'from' qualifiedName ';'
                 | 'import' qualifiedName ';'
importTarget     : Identifier ('as' Identifier)?
```

Two forms:
- **Selective:** `import Foo, Bar as Baz from ModuleName;`
- **Whole-module:** `import ModuleName;`

### 2.3 Declarations

```
declaration : externFunctionDeclaration
            | functionDeclaration
            | classDeclaration
            | structDeclaration
            | interfaceDeclaration
            | enumDeclaration
            | variableDeclaration
```

### 2.4 Functions

```
functionDeclaration : accessModifier? staticModifier? abstractModifier?
                      'func' Identifier '(' parameterList? ')' '=>' returnType
                      (block | ';')
```

Expression body shorthand: `func square(int x) => int { return x * x; }` — but note the grammar always requires a block. The ASTGenerator converts single-expression bodies (where the block contains just `return expr;`) transparently.

### 2.5 Parameters

```
parameter     : typeIdentifier Identifier ('=' expression)?
parameterList : parameter (',' parameter)*
```

Reference parameters use the type modifier: `int& x` → `typeIdentifier` includes the `&`.

### 2.6 Variables

```
variableDeclaration : typedVariableDeclaration | inferredVariableDeclaration | tupleDestructuring

typedVariableDeclaration    : accessModifier? staticModifier? typeIdentifier Identifier
                              ('=' expression | callArguments)? ';'
inferredVariableDeclaration : 'var' Identifier '=' expression ';'
tupleDestructuring          : '(' destructureElement (',' destructureElement)+ ')'
                              '=' expression ';'
destructureElement          : 'var' Identifier | typeIdentifier Identifier
```

Construction syntax: `Point p(1.0, 2.0);` — typed variable with call arguments.

### 2.7 Structs

```
structDeclaration : accessModifier? 'struct' Identifier '{'
                    (variableDeclaration | functionDeclaration | operatorDeclaration)*
                    '}'
```

### 2.8 Classes

```
classDeclaration : accessModifier? abstractModifier? 'class' Identifier
                   (':' qualifiedName (',' qualifiedName)*)?
                   '{' classMember* '}'
classMember      : variableDeclaration | functionDeclaration | operatorDeclaration
                 | constructorDeclaration | destructorDeclaration
```

Inheritance: `class Dog : Animal, Drawable` — single base class + multiple interfaces.

### 2.9 Constructors and Destructors

```
constructorDeclaration : accessModifier? 'constructor' '(' parameterList? ')'
                         (':' 'super' '(' argumentList? ')')? block
destructorDeclaration  : 'destructor' block
```

Super call: `constructor(int x) : super(x) { ... }`

### 2.10 Operator Overloading

```
operatorDeclaration : 'func' 'operator' overloadableOp '(' parameterList? ')'
                      '=>' returnType (block | ';')
overloadableOp      : '+' | '-' | '*' | '/' | '%' | '==' | '!='
                    | '<' | '<=' | '>' | '>=' | '[]'
```

### 2.11 Interfaces

```
interfaceDeclaration : accessModifier? 'interface' Identifier '{'
                       functionDeclaration*
                       '}'
```

Interface methods are abstract by default (no body).

### 2.12 Enums

```
enumDeclaration : accessModifier? 'enum' Identifier (':' typeIdentifier)?
                  '{' enumMember (',' enumMember)* ','? '}'
enumMember      : Identifier ('=' expression)?
```

### 2.13 Extern Functions

```
externFunctionDeclaration : 'extern' 'func' Identifier '(' parameterList? ')'
                            '=>' returnType ';'
```

### 2.14 Control Flow

```
ifStatement     : 'if' '(' expression ')' statement elseIfClause* elseClause?
forStatement    : 'for' '(' (localVarDeclaration | exprList)? ';' expression? ';' exprList? ')' (block | statement)
whileStatement  : 'while' '(' expression ')' (block | statement)
switchStatement : 'switch' '(' expression ')' '{' switchCase* switchDefault? '}'
```

### 2.15 Pattern Matching

```
matchExpression : 'match' expression '{' matchArm (',' matchArm)* ','? '}'
matchArm        : pattern '=>' (expression | block)
pattern         : basePattern ('if' expression)?
basePattern     : literalPattern | rangePattern | wildcardPattern | bindingPattern | tuplePattern
```

Pattern types:
- **Literal:** `42`, `"hello"`, `Color.Red`, `null`
- **Range:** `1..10` (integer only, inclusive)
- **Wildcard:** `_`
- **Binding:** `var x` (binds the subject to `x`)
- **Tuple:** `(var a, var b)`
- **Guarded:** any pattern + `if condition`

### 2.16 Raw Blocks

```
rawBlock : 'raw' block
```

Required for pointer dereference assignment and pointer arithmetic.

### 2.17 Lambda Expressions

```
lambdaExpression  : captureList '(' lambdaParamList? ')' '=>' (block | expression)
captureList       : '[' ']' | '[' captureDefault ']'
                  | '[' captureDefault (',' captureItem)+ ']'
                  | '[' captureItem (',' captureItem)* ']'
captureDefault    : '=' | '&'
captureItem       : '&'? Identifier
```

Capture list forms:
- `[]` — no captures
- `[=]` — all by value
- `[&]` — all by reference
- `[x, y]` — explicit by value
- `[&x, &y]` — explicit by reference
- `[=, &x]` — all by value except `x` by reference
- `[&, x]` — all by reference except `x` by value

### 2.18 Pipe Expression

```
pipe       : ternary ('|>' pipeTarget)*
pipeTarget : qualifiedName callArguments?
```

`x |> f |> g(a, b)` — passes result as the first argument at each stage.

### 2.19 New and Delete

```
newExpression   : 'new' typeIdentifier callArguments?
                | 'new' typeIdentifier '[' expression ']'
deleteStatement : 'delete' expression ';'
```

---

## 3. Operator Precedence

From highest to lowest:

| Level | Operators | Associativity |
|-------|-----------|---------------|
| Postfix | `()` `[]` `.` `->` `++` `--` | Left |
| Prefix/Unary | `++` `--` `+` `-` `!` `~` `&` `*` `sizeof` `alignof` | Right |
| Cast | `(type)expr` | Right |
| Multiplicative | `*` `/` `%` | Left |
| Additive | `+` `-` | Left |
| Shift | `<<` `>>` | Left |
| Relational | `<` `<=` `>` `>=` | Left |
| Equality | `==` `!=` | Left |
| Bitwise AND | `&` | Left |
| Bitwise XOR | `^` | Left |
| Bitwise OR | `\|` | Left |
| Logical AND | `&&` | Left |
| Logical OR | `\|\|` | Left |
| Ternary | `? :` | Right |
| Pipe | `\|>` | Left |
| Assignment | `=` `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` | Right |
| Lambda | `[captures](params) => body` | — |

---

## 4. AST Node Inventory

### 4.1 Base Classes

**File:** `include/mingus/ast/ASTNode.h`

| Class | Base | Purpose |
|-------|------|---------|
| `AstBaseNode` | — | Root with `DebugInfo` (V2: replaces `SourceLocation`), `astScopeNode` (baked scope pointer) |
| `ExpressionNode` | `AstBaseNode` | Has `resolvedType`, `resolvedSymbol` (filled by sema) |
| `StatementNode` | `AstBaseNode` | Statements |
| `DeclarationNode` | `AstBaseNode` | Top-level declarations |

All nodes support `accept(ASTVisitor&)` for the visitor pattern and `as<T>()`/`is<T>()` for downcasting.

V2 additions: `CallExpression::resolvedCallee` (FunctionSymbol for direct calls), `ArgumentsNode::isReference` (per-argument ref flags), `ParameterNode::resolvedSymbol` (eliminates scanForParamSymbols).

### 4.2 Program Nodes

**File:** `include/mingus/ast/Program.h`

| Node | Key Fields |
|------|------------|
| `ProgramNode` | `modules: NodeList<ModuleNode>` |
| `ModuleNode` | `name`, `imports`, `declarations` |
| `ImportNode` | `targets: vector<ImportTarget>`, `source: QualifiedName` |

### 4.3 Declaration Nodes

**File:** `include/mingus/ast/Declarations.h`

| Node | Key Fields |
|------|------------|
| `VariableDeclaration` | `name`, `type`, `isInferred`, `initializer`, `accessModifier`, `isStatic` |
| `TupleDestructuringDeclaration` | `elements: vector<DestructureElement>`, `initializer` |
| `FunctionDeclaration` | `name`, `parameters`, `returnType`, `body`, `accessModifier`, `isStatic`, `isAbstract` |
| `ConstructorDeclaration` | `parameters`, `body`, `superArgs`, `hasSuperCall`, `accessModifier` |
| `DestructorDeclaration` | `body` |
| `OperatorDeclaration` | `op: OperatorKind`, `parameters`, `returnType`, `body` |
| `ExternFunctionDeclaration` | `name`, `parameters`, `returnType` |
| `EnumDeclaration` | `name`, `underlyingType`, `members: NodeList<EnumMemberNode>` |
| `StructDeclaration` | `name`, `fields`, `methods`, `operators` |
| `ClassDeclaration` | `name`, `baseClasses`, `constructor`, `destructor`, `fields`, `methods`, `operators` |
| `InterfaceDeclaration` | `name`, `methods` |

**OperatorKind** enum: `Plus`, `Minus`, `Star`, `Divide`, `Modulo`, `Equal`, `NotEqual`, `Less`, `LessEqual`, `Greater`, `GreaterEqual`, `Index`

### 4.4 Statement Nodes

**File:** `include/mingus/ast/Statements.h`

| Node | Key Fields |
|------|------------|
| `BlockStatement` | `statements: NodeList<StatementNode>` |
| `ExpressionStatement` | `expression` |
| `ReturnStatement` | `value` (nullable for void) |
| `IfStatement` | `condition`, `thenBody`, `elseIfClauses`, `elseBody` |
| `ForStatement` | `initDeclaration`, `condition`, `iterators`, `body` |
| `WhileStatement` | `condition`, `body` |
| `SwitchStatement` | `subject`, `cases`, `defaultCase` |
| `BreakStatement` | — |
| `ContinueStatement` | — |
| `DeleteStatement` | `target` |
| `RawBlock` | `body: BlockStatement` |

### 4.5 Expression Nodes

**File:** `include/mingus/ast/Expressions.h`

#### Literals

| Node | Value Type |
|------|------------|
| `IntegerLiteral` | `int64_t` |
| `FloatLiteral` | `double` |
| `BoolLiteral` | `bool` |
| `CharLiteral` | `char` |
| `StringLiteral` | `string` |
| `NullLiteral` | — |
| `InterpolatedString` | `vector<InterpolatedPart>` (text + expressions) |

#### Reference/Identity

| Node | Key Fields |
|------|------------|
| `IdentifierExpression` | `name`, `resolvedSymbol` |
| `QualifiedNameExpression` | `qualifiedName`, `resolvedSymbol` |
| `ThisExpression` | — |

#### Operators

| Node | Key Fields |
|------|------------|
| `BinaryExpression` | `left`, `op: BinaryOp`, `right`, `isOperatorOverload`, `resolvedOperatorFunction` |
| `UnaryExpression` | `op: UnaryOp`, `operand` |
| `AssignmentExpression` | `target`, `op: AssignOp`, `value` |
| `TernaryExpression` | `condition`, `thenExpr`, `elseExpr` |

#### Compound

| Node | Key Fields |
|------|------------|
| `CallExpression` | `callee`, `arguments` |
| `MemberAccessExpression` | `object`, `memberName`, `isArrow`, + semantic flags |
| `IndexExpression` | `object`, `index`, `isOperatorOverload` |
| `NewExpression` | `type`, `arguments` or `isArray` + `arraySize` |
| `CastExpression` | `targetType`, `operand` |
| `SizeOfExpression` | `targetType` |
| `AlignOfExpression` | `targetType` |
| `TupleExpression` | `elements` |
| `MatchExpression` | `subject`, `arms: vector<MatchArm>` |
| `PipeExpression` | `input`, `stages: vector<PipeStage>` |
| `LambdaExpression` | `parameters`, `body`, `captureDefault`, `captureItems`, `capturedVariables`, `captureModesResolved`, `escapes`, `selfCapture` |

**MemberAccessExpression** semantic flags (filled by sema):
- `resolvedField`, `resolvedEnumValue`, `isEnumAccess`, `isStringEnumAccess`
- `isStringBuiltinMethod`, `isStaticAccess`

### 4.6 Pattern Nodes

**File:** `include/mingus/ast/Patterns.h`

| Node | Key Fields | Matches |
|------|------------|---------|
| `LiteralPattern` | `value: ExpressionNode` | Exact literal or enum member |
| `RangePattern` | `low: int64_t`, `high: int64_t` | Integer range `[low, high]` |
| `WildcardPattern` | — | Anything (`_`) |
| `BindingPattern` | `name: string` | Anything, binds to name (`var x`) |
| `TuplePattern` | `elements: PatternList` | Tuple with sub-patterns |
| `GuardedPattern` | `innerPattern`, `guard: ExpressionNode` | Inner pattern AND guard is truthy |

### 4.7 Type Nodes

**File:** `include/mingus/ast/TypeNode.h`

| Node | Key Fields | Represents |
|------|------------|------------|
| `PrimitiveTypeNode` | `kind` | `int`, `double`, `float`, `byte`, `string`, `char`, `bool`, `void` |
| `NamedTypeNode` | `qualifiedName: vector<string>` | User type: `Point`, `Module.Class` |
| `PointerTypeNode` | `baseType`, `isReference` | `T*` or `T&` |
| `ArrayTypeNode` | `elementType`, `size` | `T[]` or `T[N]` |
| `TupleTypeNode` | `elementTypes` | `(T0, T1, ...)` |
| `FunctionTypeNode` | `parameterTypes`, `returnType` | `(T0, T1) => R` |

Type modifiers stack left-to-right: `int[16]*` → `PointerTypeNode(ArrayTypeNode(int, 16))`.

---

## 5. ASTGenerator Mapping

### Key Grammar-to-AST Conversions

| Grammar Rule | AST Node | Notes |
|-------------|----------|-------|
| `program` | `ProgramNode` | |
| `module` | `ModuleNode` | |
| `functionDeclaration` | `FunctionDeclaration` | Expression body → `Block{Return}` |
| `variableDeclaration` (typed) | `VariableDeclaration` | Construction `T x(args)` → `CallExpression` init |
| `variableDeclaration` (var) | `VariableDeclaration` (isInferred=true) | |
| `tupleDestructuring` | `TupleDestructuringDeclaration` | |
| `matchStatement` | `ExpressionStatement{MatchExpression}` | Match-as-statement wrapped |
| `assignment` with lambda RHS | `AssignmentExpression` | `f = [=](x) => {...};` |
| `pipe` with stages | `PipeExpression` | |
| `binary operators` | `BinaryExpression` chain | Left-associative |
| `postfixExpression` | Chained `Call`/`Index`/`MemberAccess` | |
| `primaryExpression` (integer) | `IntegerLiteral` | Handles decimal/hex/binary/octal |
| `string` with interpolation | `InterpolatedString` | Plain strings → `StringLiteral` |

### Special Cases

- **Variable-as-statement**: Local `var` declarations inside function bodies are wrapped in `ExpressionStatement` (a structural quirk).
- **Integer literal parsing**: `0b`/`0B` (binary), `0o`/`0O` (octal), `0x`/`0X` (hex) — uses `std::stoll` with explicit bases.
- **String escape processing**: `\n` → newline, `\t` → tab, `\r` → return, others take literal character after backslash.
- **Lambda in assignment**: `visitAssignment` checks for `lambdaExpression` as RHS, calls `visitLambdaExpression` directly.
- **Postfix chaining**: `visitPostfixExpression` iterates operations left-to-right, wrapping the accumulator at each step.

---

## 6. Grammar Limitations

- **No varargs in extern:** Each `extern func printf(...)` must declare exact parameter count.
- **For loop init:** Only the first `localVarDeclaration` is used; `for (int i = 0, int j = 0; ...)` silently drops `j`.
- **Range pattern:** Only integer literal ranges (`1..10`); no float or char ranges.
- **Pipe target:** Only `qualifiedName`, not arbitrary expressions or member access chains.
- **No `do-while` loop.**
- **No labeled break/continue.**
- **No generics/templates.**
- **No `typedef` or `using` alias.**
- **No `const` modifier** on variables or parameters.
- **Char literal escapes:** ASTGenerator reads `text[1]` directly without processing escape sequences in char literals.
- **Lambda params can omit types:** Untyped lambda params produce `ParameterNode` with null type; type inference is not supported (sema will error).
