#pragma once

// ============================================================================
// Expressions.h — All concrete expression AST nodes for Mingus V2
//
// Every expression inherits from ExpressionBaseNode which carries:
//   - resolvedType (set by Pass 3)
//   - resolvedSymbol (set by Pass 2/3 for identifiers, member access)
//   - astScopeNode (set by Pass 1)
//   - debugInfo (set by parser)
//
// Key V2 improvements over V1:
//   - CallExpression uses ArgumentsNode with per-arg isReference
//   - MemberAccessExpression uses inherited resolvedSymbol (no separate FieldInfo*)
//   - IdentifierExpression uses inherited resolvedSymbol
//   - All nodes have scope + debug info
// ============================================================================

#include "AstNode.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mingus {

// ============================================================================
// Literals
// ============================================================================

class IntegerLiteral : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;
    int64_t value = 0;
};

class FloatLiteral : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;
    double value = 0.0;
};

class BoolLiteral : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;
    bool value = false;
};

class CharLiteral : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;
    char value = '\0';
};

class StringLiteral : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;
    std::string value;
};

class NullLiteral : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;
};

// ============================================================================
// Interpolated String — "x=${x}, name=${name}"
// ============================================================================

class InterpolatedStringExpression : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;
    std::vector<InterpolatedPart> parts;
};

// ============================================================================
// Identifiers & Names
// ============================================================================

class IdentifierExpression : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;
    std::string name;
    // resolvedSymbol inherited from ExpressionBaseNode
};

class QualifiedNameExpression : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;
    std::vector<std::string> parts;  // ["Module", "name"] or ["Enum", "Member"]

    // Enum access resolution (set by TypeChecker)
    bool isEnumAccess = false;
    int64_t resolvedEnumValue = 0;

    // resolvedSymbol inherited from ExpressionBaseNode
};

class ThisExpression : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;
};

// ============================================================================
// Member Access — obj.field, obj.method, ptr->field, Enum.Member
// ============================================================================

class MemberAccessExpression : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::shared_ptr<ExpressionBaseNode> object;
    std::string memberName;
    bool isArrow = false;  // true for ptr->field syntax

    // ---- Sema-resolved annotations ----
    // resolvedSymbol (inherited) points to the VariableSymbol/FunctionSymbol

    // Enum access resolved values
    int64_t resolvedEnumValue = 0;
    std::string resolvedEnumStringValue;
    bool isEnumAccess = false;
    bool isStringEnumAccess = false;

    // String builtin methods (length, substring, etc.)
    bool isStringBuiltinMethod = false;

    // Static method access: ClassName.staticMethod()
    bool isStaticAccess = false;
};

// ============================================================================
// Operators
// ============================================================================

class BinaryExpression : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::shared_ptr<ExpressionBaseNode> left;
    BinaryOp op;
    std::shared_ptr<ExpressionBaseNode> right;

    // Sema: resolved to operator overload?
    bool isOperatorOverload = false;
    SymbolPtr resolvedOperatorFunction;  // OperatorSymbol if overloaded
};

class UnaryExpression : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    UnaryOp op;
    std::shared_ptr<ExpressionBaseNode> operand;
};

class AssignmentExpression : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::shared_ptr<ExpressionBaseNode> target;
    AssignOp op;
    std::shared_ptr<ExpressionBaseNode> value;
};

class TernaryExpression : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::shared_ptr<ExpressionBaseNode> condition;
    std::shared_ptr<ExpressionBaseNode> thenExpr;
    std::shared_ptr<ExpressionBaseNode> elseExpr;
};

// ============================================================================
// Index & Call
// ============================================================================

class IndexExpression : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::shared_ptr<ExpressionBaseNode> object;
    std::shared_ptr<ExpressionBaseNode> index;

    // Sema: resolved to operator[] overload?
    bool isOperatorOverload = false;
    SymbolPtr resolvedOperatorFunction;
};

// CallExpression — Uses ArgumentsNode with per-arg isReference tracking.
// This is a key V2 improvement: codegen reads isReference directly.
class CallExpression : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::shared_ptr<ExpressionBaseNode> callee;
    std::shared_ptr<ArgumentsNode> arguments;

    // Sema-resolved: direct function call target (null for indirect/closure calls)
    std::shared_ptr<FunctionSymbol> resolvedCallee;
};

// ============================================================================
// Type Operations
// ============================================================================

class CastExpression : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::shared_ptr<TypeNode> targetType;
    std::shared_ptr<ExpressionBaseNode> operand;
};

class NewExpression : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::shared_ptr<TypeNode> type;
    std::shared_ptr<ArgumentsNode> arguments;  // constructor args (uses ArgumentsNode)
    bool isArray = false;
    std::shared_ptr<ExpressionBaseNode> arraySize;  // for new T[size]
};

class SizeOfExpression : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;
    std::shared_ptr<TypeNode> targetType;
};

// ============================================================================
// Tuple
// ============================================================================

class TupleExpression : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;
    std::vector<std::shared_ptr<ExpressionBaseNode>> elements;
};

// ============================================================================
// Match Expression
//
// Can be used as both expression and statement (via ExpressionStatement wrapper).
// Arms can have expression bodies or block bodies.
// ============================================================================

class MatchExpression : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::shared_ptr<ExpressionBaseNode> subject;
    std::vector<MatchArm> arms;
};

// ============================================================================
// Pipe Expression — x |> f |> g(a, b)
// ============================================================================

class PipeExpression : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::shared_ptr<ExpressionBaseNode> input;
    std::vector<PipeStage> stages;
};

// ============================================================================
// Lambda Expression
//
// Explicit C++ capture lists: [], [=], [&], [x], [&x], [=, &x], [&, x]
// ============================================================================

class LambdaExpression : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    // Parsed
    std::vector<std::shared_ptr<ParameterNode>> parameters;
    std::shared_ptr<AstBaseNode> body;  // ExpressionBaseNode or BlockStatementNode
    CaptureDefault captureDefault = CaptureDefault::None;
    std::vector<CaptureItem> captureItems;

    // Sema-resolved (Pass 4 — SemanticValidator)
    std::vector<SymbolPtr> capturedVariables;
    std::vector<CaptureMode> captureModesResolved;
    bool escapes = true;       // false for non-escaping (stack-allocated)
    bool selfCapture = false;  // letrec pattern

    // Convenience
    bool hasExpressionBody() const { return body && body->is<ExpressionBaseNode>(); }
    bool hasBlockBody() const { return body && body->is<BlockStatementNode>(); }
};

// ============================================================================
// Variable Declaration as Expression
//
// var x = expr; in expression context (wrapped in ExpressionStatement for
// statement context). This dual nature is a V1 quirk we preserve.
// ============================================================================

class VariableDeclarationExpression : public ExpressionBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::string name;
    AccessModifier accessModifier = AccessModifier::Public;
    bool isStatic = false;
    std::shared_ptr<TypeNode> type;  // nullptr if inferred
    bool isInferred = false;
    std::shared_ptr<ExpressionBaseNode> initializer;

    // Resolved in Pass 1
    std::shared_ptr<VariableSymbol> resolvedVariable;
};

} // namespace mingus
