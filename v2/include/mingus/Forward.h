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

// ---- Common type aliases ----
using SymbolPtr     = std::shared_ptr<Symbol>;
using ScopePtr      = std::shared_ptr<Scope>;
using TypeSymbolPtr = std::shared_ptr<TypeSymbol>;

// ---- Enums ----

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
    Negate      // unary -
};

enum class CaptureDefault {
    None,       // []
    ByCopy,     // [=]
    ByRef       // [&]
};

enum class CaptureMode {
    ByValue,
    ByReference
};

} // namespace mingus
