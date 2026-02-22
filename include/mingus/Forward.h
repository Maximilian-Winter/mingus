#pragma once

// ============================================================================
// Forward.h — Forward declarations for all Mingus V2 types
//
// Include this header when you need shared_ptr<T> declarations without
// pulling in the full type definitions.
// ============================================================================

#include <memory>
#include <string>
#include <vector>

namespace mingus {

// ---- Scope hierarchy ----
class Scope;
class BaseScope;
class GlobalScope;
class BlockScope;

// ---- Symbol hierarchy ----
class Symbol;
class BaseSymbol;
class TypedSymbol;
class SymbolWithScope;

// ---- TypeSymbol hierarchy (types ARE symbols) ----
class TypeSymbol;
class PrimitiveTypeSymbol;
class PointerTypeSymbol;
class ArrayTypeSymbol;
class TupleTypeSymbol;
class FunctionTypeSymbol;
class ReferenceTypeSymbol;
class ErrorTypeSymbol;
class NullTypeSymbol;

// ---- Concrete symbol types ----
class VariableSymbol;
class FunctionSymbol;
class MethodSymbol;
class ConstructorSymbol;
class DestructorSymbol;
class OperatorSymbol;
class ClassSymbol;
class StructSymbol;
class EnumSymbol;
class InterfaceSymbol;
class TypeAliasSymbol;
class ModuleSymbol;

// ---- Infrastructure ----
class SymbolTable;
class DebugInfo;

// ---- AST base nodes ----
class AstBaseNode;
class ExpressionBaseNode;
class StatementBaseNode;
class DeclarationBaseNode;
class BlockStatementNode;
class ArgumentsNode;
class ParameterNode;
class TypeNode;
class ModifiersNode;
class ModuleNode;
class ProgramNode;

// ---- AST expression nodes ----
class IntegerLiteral;
class FloatLiteral;
class BoolLiteral;
class CharLiteral;
class StringLiteral;
class NullLiteral;
class InterpolatedStringExpression;
class IdentifierExpression;
class QualifiedNameExpression;
class ThisExpression;
class MemberAccessExpression;
class BinaryExpression;
class UnaryExpression;
class AssignmentExpression;
class TernaryExpression;
class IndexExpression;
class CallExpression;
class CastExpression;
class NewExpression;
class SizeOfExpression;
class TupleExpression;
class MatchExpression;
class PipeExpression;
class LambdaExpression;
class VariableDeclarationExpression;
class MoveExpression;

// ---- AST statement nodes ----
class ExpressionStatement;
class ReturnStatement;
class IfStatement;
class ForStatement;
class WhileStatement;
class DoWhileStatement;
class LabeledStatement;
class BreakStatement;
class ContinueStatement;
class DeleteStatement;
class SwitchStatement;

// ---- AST declaration nodes ----
class VariableDeclaration;
class TupleDestructuringDeclaration;
class FunctionDeclaration;
class ConstructorDeclaration;
class DestructorDeclaration;
class ExternFunctionDeclaration;
class OperatorDeclaration;
class EnumDeclaration;
class StructDeclaration;
class ClassDeclaration;
class InterfaceDeclaration;
class ImportDeclaration;
class TypedefDeclaration;
class EnumMemberNode;

// ---- AST pattern nodes (for match expressions) ----
class PatternNode;
class LiteralPattern;
class IdentifierPattern;
class WildcardPattern;
class RangePattern;

// ---- Common type aliases ----
using SymbolPtr     = std::shared_ptr<Symbol>;
using ScopePtr      = std::shared_ptr<Scope>;
using TypeSymbolPtr = std::shared_ptr<TypeSymbol>;

// ============================================================================
// Enums
// ============================================================================

enum class AccessModifier {
    Public,
    Protected,
    Private
};

enum class VariableRole {
    Local,
    Parameter,
    Field
};

enum class PrimitiveKind {
    Int,
    Double,
    Float,
    Byte,
    Char,
    String,
    Bool,
    Void
};

enum class OverloadableOp {
    Add,        // +
    Sub,        // -
    Mul,        // *
    Div,        // /
    Mod,        // %
    Equal,      // ==
    NotEqual,   // !=
    Less,       // <
    Greater,    // >
    LessEq,    // <=
    GreaterEq,  // >=
    Negate,     // unary -
    Index       // []
};

enum class CaptureDefault {
    None,       // []
    ByCopy,     // [=]
    ByRef       // [&]
};

enum class CaptureMode {
    ByValue,
    ByReference,
    Weak
};

// ---- Expression operator enums ----

enum class BinaryOp {
    Add, Sub, Mul, Div, Mod,
    Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual,
    LogicalAnd, LogicalOr,
    BitwiseAnd, BitwiseOr, BitwiseXor,
    ShiftLeft, ShiftRight
};

enum class UnaryOp {
    Negate,
    LogicalNot,
    BitwiseNot,
    AddressOf,
    Dereference,
    PreIncrement,
    PreDecrement,
    PostIncrement,
    PostDecrement
};

enum class AssignOp {
    Assign,
    AddAssign, SubAssign, MulAssign, DivAssign, ModAssign,
    AndAssign, OrAssign, XorAssign,
    ShiftLeftAssign, ShiftRightAssign
};

enum class InterpolatedPartKind {
    Text,
    Expression
};

// ---- Modifier types ----

enum class ModifierType {
    Public,
    Private,
    Protected,
    Static,
    Abstract,
    Extern,
    Virtual,
    Override
};

} // namespace mingus
