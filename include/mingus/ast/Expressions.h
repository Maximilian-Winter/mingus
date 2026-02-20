//================================================================================
// MINGUS v1 - Expression Nodes
// Literals, identifiers, operators, calls, and Mingus-specific expressions
//================================================================================

#pragma once

#include "mingus/ast/ASTNode.h"
#include "mingus/ast/TypeNode.h"
#include "mingus/ast/Program.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// Forward declaration for semantic analysis symbols
namespace mingus { namespace sema { class Symbol; } }

namespace mingus {
namespace ast {

// Forward declarations
class StatementNode;
class BlockStatement;
class ParameterNode;
class PatternNode;

//================================================================================
// Binary Operator kinds
//================================================================================
enum class BinaryOp {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    LogicalAnd,
    LogicalOr,
    BitwiseAnd,
    BitwiseOr,
    BitwiseXor,
    ShiftLeft,
    ShiftRight
};

std::string binaryOpToString(BinaryOp op);
bool binaryOpIsComparison(BinaryOp op);
bool binaryOpIsLogical(BinaryOp op);
bool binaryOpIsBitwise(BinaryOp op);
bool binaryOpIsArithmetic(BinaryOp op);

//================================================================================
// Unary Operator kinds
//================================================================================
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

std::string unaryOpToString(UnaryOp op);
bool unaryOpIsPrefix(UnaryOp op);
bool unaryOpIsPostfix(UnaryOp op);

//================================================================================
// Assignment Operator kinds
//================================================================================
enum class AssignOp {
    Assign,
    AddAssign,
    SubAssign,
    MulAssign,
    DivAssign,
    ModAssign,
    AndAssign,
    OrAssign,
    XorAssign,
    ShiftLeftAssign,
    ShiftRightAssign
};

std::string assignOpToString(AssignOp op);
bool assignOpIsCompound(AssignOp op);
BinaryOp assignOpToBinaryOp(AssignOp op);

//================================================================================
// IntegerLiteral - integer constant
//================================================================================
class IntegerLiteral : public ExpressionNode {
public:
    int64_t value;

    explicit IntegerLiteral(int64_t val,
                            const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), value(val) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// FloatLiteral - floating point constant
//================================================================================
class FloatLiteral : public ExpressionNode {
public:
    double value;

    explicit FloatLiteral(double val,
                          const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), value(val) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// BoolLiteral - boolean constant
//================================================================================
class BoolLiteral : public ExpressionNode {
public:
    bool value;

    explicit BoolLiteral(bool val,
                         const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), value(val) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// CharLiteral - character constant
//================================================================================
class CharLiteral : public ExpressionNode {
public:
    char value;

    explicit CharLiteral(char val,
                         const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), value(val) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// StringLiteral - string constant
//================================================================================
class StringLiteral : public ExpressionNode {
public:
    std::string value;

    explicit StringLiteral(std::string val,
                           const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), value(std::move(val)) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// InterpolatedString - "hello ${name}!"
//================================================================================
enum class InterpolatedPartKind {
    Text,
    Expression
};

struct InterpolatedPart {
    InterpolatedPartKind kind;
    std::string text;                      // For Text parts
    NodePtr<ExpressionNode> expression;    // For Expression parts

    static InterpolatedPart makeText(const std::string& txt) {
        InterpolatedPart part;
        part.kind = InterpolatedPartKind::Text;
        part.text = txt;
        return part;
    }

    static InterpolatedPart makeExpression(NodePtr<ExpressionNode> expr) {
        InterpolatedPart part;
        part.kind = InterpolatedPartKind::Expression;
        part.expression = expr;
        return part;
    }
};

class InterpolatedString : public ExpressionNode {
public:
    std::vector<InterpolatedPart> parts;

    explicit InterpolatedString(std::vector<InterpolatedPart> prts,
                                const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), parts(std::move(prts)) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// NullLiteral - null pointer
//================================================================================
class NullLiteral : public ExpressionNode {
public:
    explicit NullLiteral(const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// IdentifierExpression - variable or function name reference
//================================================================================
class IdentifierExpression : public ExpressionNode {
public:
    std::string name;
    mingus::sema::Symbol* resolvedSymbol = nullptr;  // Filled during symbol resolution

    explicit IdentifierExpression(const std::string& n,
                                  const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), name(n) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// QualifiedNameExpression - Module.ClassName reference
//================================================================================
class QualifiedNameExpression : public ExpressionNode {
public:
    QualifiedName qualifiedName;
    mingus::sema::Symbol* resolvedSymbol = nullptr;  // Filled during symbol resolution

    explicit QualifiedNameExpression(QualifiedName name,
                                     const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), qualifiedName(std::move(name)) {}

    std::string getName() const { return qualifiedName.toString(); }

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// MemberAccessExpression - obj.member or obj->member
//================================================================================
class MemberAccessExpression : public ExpressionNode {
public:
    NodePtr<ExpressionNode> object;
    std::string memberName;
    bool isArrow;  // true for ->, false for .
    FieldInfo* resolvedField = nullptr;  // Filled during type checking
    int64_t resolvedEnumValue = 0;           // Filled when this is an enum member access
    std::string resolvedEnumStringValue;     // Filled when this is a string enum member access
    bool isEnumAccess = false;               // True when this resolves to an enum member
    bool isStringEnumAccess = false;         // True when this resolves to a string enum member
    bool isStringBuiltinMethod = false;      // True for string built-in methods (length, charAt, substring)
    bool isStaticAccess = false;             // True for ClassName.staticMethod() syntax

    MemberAccessExpression(NodePtr<ExpressionNode> obj,
                           const std::string& member,
                           bool arrow,
                           const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), object(obj), memberName(member), isArrow(arrow) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// ThisExpression - 'this' keyword
//================================================================================
class ThisExpression : public ExpressionNode {
public:
    explicit ThisExpression(const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// BinaryExpression - left op right
//================================================================================
class BinaryExpression : public ExpressionNode {
public:
    NodePtr<ExpressionNode> left;
    BinaryOp op;
    NodePtr<ExpressionNode> right;
    bool isOperatorOverload;           // Resolved during type checking
    mingus::sema::Symbol* resolvedOperatorFunction = nullptr;  // If overloaded

    BinaryExpression(NodePtr<ExpressionNode> lhs,
                     BinaryOp oper,
                     NodePtr<ExpressionNode> rhs,
                     const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), left(lhs), op(oper), right(rhs), 
          isOperatorOverload(false) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// UnaryExpression - op operand or operand op (for postfix)
//================================================================================
class UnaryExpression : public ExpressionNode {
public:
    UnaryOp op;
    NodePtr<ExpressionNode> operand;

    UnaryExpression(UnaryOp oper,
                    NodePtr<ExpressionNode> expr,
                    const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), op(oper), operand(expr) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// AssignmentExpression - target op value
//================================================================================
class AssignmentExpression : public ExpressionNode {
public:
    NodePtr<ExpressionNode> target;  // Must be an lvalue
    AssignOp op;
    NodePtr<ExpressionNode> value;

    AssignmentExpression(NodePtr<ExpressionNode> tgt,
                         AssignOp oper,
                         NodePtr<ExpressionNode> val,
                         const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), target(tgt), op(oper), value(val) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// TernaryExpression - condition ? thenExpr : elseExpr
//================================================================================
class TernaryExpression : public ExpressionNode {
public:
    NodePtr<ExpressionNode> condition;
    NodePtr<ExpressionNode> thenExpr;
    NodePtr<ExpressionNode> elseExpr;

    TernaryExpression(NodePtr<ExpressionNode> cond,
                      NodePtr<ExpressionNode> thenEx,
                      NodePtr<ExpressionNode> elseEx,
                      const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), condition(cond), thenExpr(thenEx), elseExpr(elseEx) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// CallExpression - callee(arguments...)
//================================================================================
class CallExpression : public ExpressionNode {
public:
    NodePtr<ExpressionNode> callee;  // identifier, member access, or function value
    NodeList<ExpressionNode> arguments;

    CallExpression(NodePtr<ExpressionNode> call,
                   NodeList<ExpressionNode> args,
                   const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), callee(call), arguments(std::move(args)) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// NewExpression - new Type(args) or new Type[size]
//================================================================================
class NewExpression : public ExpressionNode {
public:
    NodePtr<TypeNode> type;
    NodeList<ExpressionNode> arguments;  // Constructor args
    bool isArray;
    NodePtr<ExpressionNode> arraySize;   // For new Type[size]

    // Object construction: new Type(args)
    NewExpression(NodePtr<TypeNode> tp,
                  NodeList<ExpressionNode> args,
                  const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), type(tp), arguments(std::move(args)), 
          isArray(false), arraySize(nullptr) {}

    // Array construction: new Type[size]
    NewExpression(NodePtr<TypeNode> tp,
                  NodePtr<ExpressionNode> size,
                  const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), type(tp), isArray(true), arraySize(size) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// IndexExpression - object[index]
//================================================================================
class IndexExpression : public ExpressionNode {
public:
    NodePtr<ExpressionNode> object;
    NodePtr<ExpressionNode> index;
    bool isOperatorOverload;  // Resolved during type checking
    mingus::sema::Symbol* resolvedOperatorFunction = nullptr;  // If overloaded

    IndexExpression(NodePtr<ExpressionNode> obj,
                    NodePtr<ExpressionNode> idx,
                    const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), object(obj), index(idx), isOperatorOverload(false) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// CastExpression - (Type)operand
//================================================================================
class CastExpression : public ExpressionNode {
public:
    NodePtr<TypeNode> targetType;
    NodePtr<ExpressionNode> operand;

    CastExpression(NodePtr<TypeNode> tp,
                   NodePtr<ExpressionNode> expr,
                   const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), targetType(tp), operand(expr) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// SizeOfExpression - sizeof(Type)
//================================================================================
class SizeOfExpression : public ExpressionNode {
public:
    NodePtr<TypeNode> targetType;

    explicit SizeOfExpression(NodePtr<TypeNode> tp,
                              const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), targetType(tp) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// AlignOfExpression - alignof(Type)
//================================================================================
class AlignOfExpression : public ExpressionNode {
public:
    NodePtr<TypeNode> targetType;

    explicit AlignOfExpression(NodePtr<TypeNode> tp,
                               const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), targetType(tp) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// PipeStage - single stage in a pipe expression
//================================================================================
struct PipeStage {
    NodePtr<QualifiedNameExpression> function;
    NodeList<ExpressionNode> extraArguments;

    PipeStage(NodePtr<QualifiedNameExpression> fn,
              NodeList<ExpressionNode> extraArgs = {})
        : function(fn), extraArguments(std::move(extraArgs)) {}
};

//================================================================================
// PipeExpression - input |> f |> g(a, b)
//================================================================================
class PipeExpression : public ExpressionNode {
public:
    NodePtr<ExpressionNode> input;
    std::vector<PipeStage> stages;

    PipeExpression(NodePtr<ExpressionNode> inp,
                   std::vector<PipeStage> stgs,
                   const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), input(inp), stages(std::move(stgs)) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// MatchArm - single arm in a match expression
//================================================================================
struct MatchArm {
    NodePtr<PatternNode> pattern;
    NodePtr<ASTNode> body;  // ExpressionNode or BlockStatement

    MatchArm(NodePtr<PatternNode> pat, NodePtr<ASTNode> bdy)
        : pattern(pat), body(bdy) {}
};

//================================================================================
// MatchExpression - match subject { pattern => body, ... }
//================================================================================
class MatchExpression : public ExpressionNode {
public:
    NodePtr<ExpressionNode> subject;
    std::vector<MatchArm> arms;

    MatchExpression(NodePtr<ExpressionNode> subj,
                    std::vector<MatchArm> arms_list,
                    const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), subject(subj), arms(std::move(arms_list)) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// TupleExpression - (expr0, expr1, ...)
//================================================================================
class TupleExpression : public ExpressionNode {
public:
    NodeList<ExpressionNode> elements;

    explicit TupleExpression(NodeList<ExpressionNode> elems,
                             const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), elements(std::move(elems)) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// Capture list types for C++-style lambda capture specifications
//================================================================================
enum class CaptureMode {
    ByValue,        // [x]  — copy the value into the env struct
    ByReference     // [&x] — store pointer to original alloca in env struct
};

enum class CaptureDefault {
    None,           // [] or explicit list only — no auto-capture
    AllByValue,     // [=] — capture all referenced outer vars by value
    AllByReference  // [&] — capture all referenced outer vars by reference
};

struct CaptureItem {
    std::string name;
    CaptureMode mode;
    CaptureItem(const std::string& n, CaptureMode m) : name(n), mode(m) {}
};

//================================================================================
// LambdaExpression - [captures](params) => body
//================================================================================
class LambdaExpression : public ExpressionNode {
public:
    NodeList<ParameterNode> parameters;
    NodePtr<ASTNode> body;  // ExpressionNode or BlockStatement

    // === Explicit capture list (parsed from source) ===
    CaptureDefault captureDefault = CaptureDefault::None;
    std::vector<CaptureItem> captureItems;  // Named captures: [x, &y, z]

    // === Resolved captures (filled during semantic analysis Pass 4) ===
    std::vector<mingus::sema::Symbol*> capturedVariables;
    std::vector<CaptureMode> captureModesResolved;  // Parallel to capturedVariables

    bool escapes = true;       // Escape analysis: true = heap-alloc, false = stack-alloc
    bool selfCapture = false;  // True if lambda captures its own variable (letrec pattern)

    LambdaExpression(NodeList<ParameterNode> params,
                     NodePtr<ASTNode> bdy,
                     const SourceLocation& loc = SourceLocation())
        : ExpressionNode(loc), parameters(std::move(params)), body(bdy) {}

    bool hasExpressionBody() const;
    bool hasBlockBody() const;
    NodePtr<ExpressionNode> getExpressionBody() const;
    NodePtr<BlockStatement> getBlockBody() const;

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

} // namespace ast
} // namespace mingus
