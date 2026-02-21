#pragma once

// ============================================================================
// AstNode.h — AST base classes for Mingus V2
//
// Every AST node carries:
//   - astScopeNode: pointer to the scope this node lives in (set by Pass 1)
//   - debugInfo: full source range for diagnostics and debug info
//
// This eliminates childIndexStack_ and positional scope navigation.
//
// Node hierarchy:
//   AstBaseNode
//   +-- ExpressionBaseNode (resolvedType, resolvedSymbol)
//   +-- StatementBaseNode
//   |   +-- DeclarationBaseNode
//   +-- BlockStatementNode
//   +-- ArgumentsNode (per-argument IsReference)
//   +-- ParameterNode (linked to VariableSymbol by Pass 1)
//   +-- TypeNode + subclasses
//   +-- PatternNode + subclasses (for match)
//   +-- ModifiersNode
//   +-- ModuleNode / ProgramNode
// ============================================================================

#include "Forward.h"
#include "DebugInfo.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mingus {

// ============================================================================
// ASTVisitor — Forward declaration for visitor pattern
// ============================================================================

class ASTVisitor;

// ============================================================================
// AstBaseNode — Root of all AST nodes
// ============================================================================

class AstBaseNode {
public:
    virtual ~AstBaseNode() = default;
    virtual void accept(ASTVisitor& visitor) = 0;

    // The scope this node lives in (set by Pass 1)
    ScopePtr astScopeNode;

    // Source location with full range
    std::shared_ptr<DebugInfo> debugInfo;

    // RTTI-lite
    template<typename T> T* as() { return dynamic_cast<T*>(this); }
    template<typename T> const T* as() const { return dynamic_cast<const T*>(this); }
    template<typename T> bool is() const { return dynamic_cast<const T*>(this) != nullptr; }
};

// ============================================================================
// ExpressionBaseNode — Base for all expression nodes
// ============================================================================

class ExpressionBaseNode : public AstBaseNode {
public:
    // Set by Pass 3 (TypeChecker)
    TypeSymbolPtr resolvedType;

    // Set by Pass 2/3 for identifiers, member access, etc.
    SymbolPtr resolvedSymbol;
};

// ============================================================================
// StatementBaseNode — Base for all statement nodes
// ============================================================================

class StatementBaseNode : public AstBaseNode {
public:
};

// ============================================================================
// DeclarationBaseNode — Base for all declaration nodes
// ============================================================================

class DeclarationBaseNode : public StatementBaseNode {
public:
};

// ============================================================================
// BlockStatementNode — A { } block containing statements
// ============================================================================

class BlockStatementNode : public StatementBaseNode {
public:
    void accept(ASTVisitor& visitor) override;
    std::vector<std::shared_ptr<StatementBaseNode>> statements;
};

// ============================================================================
// TypeNode hierarchy — Type annotations in source code
//
// These represent what the programmer wrote. Resolved to TypeSymbol by Pass 2.
// ============================================================================

class TypeNode : public AstBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    // Resolved by Pass 2
    TypeSymbolPtr resolvedType;
};

class PrimitiveTypeNode : public TypeNode {
public:
    void accept(ASTVisitor& visitor) override;
    PrimitiveKind kind;
};

class NamedTypeNode : public TypeNode {
public:
    void accept(ASTVisitor& visitor) override;
    std::vector<std::string> qualifiedName;  // ["ModName", "TypeName"] or just ["TypeName"]
};

class PointerTypeNode : public TypeNode {
public:
    void accept(ASTVisitor& visitor) override;
    std::shared_ptr<TypeNode> baseType;
    bool isReference = false;  // true for T&
};

class ArrayTypeNode : public TypeNode {
public:
    void accept(ASTVisitor& visitor) override;
    std::shared_ptr<TypeNode> elementType;
    std::shared_ptr<ExpressionBaseNode> sizeExpr;  // nullptr for unsized
};

class TupleTypeNode : public TypeNode {
public:
    void accept(ASTVisitor& visitor) override;
    std::vector<std::shared_ptr<TypeNode>> elementTypes;
};

class FunctionTypeNode : public TypeNode {
public:
    void accept(ASTVisitor& visitor) override;
    std::vector<std::shared_ptr<TypeNode>> parameterTypes;
    std::shared_ptr<TypeNode> returnType;
};

// ============================================================================
// PatternNode hierarchy — For match expressions
// ============================================================================

class PatternNode : public AstBaseNode {
public:
};

class LiteralPattern : public PatternNode {
public:
    void accept(ASTVisitor& visitor) override;
    std::shared_ptr<ExpressionBaseNode> value;  // IntegerLiteral, StringLiteral, etc.
};

class IdentifierPattern : public PatternNode {
public:
    void accept(ASTVisitor& visitor) override;
    std::string name;
    // Guard expression (optional): "var x if x > 0"
    std::shared_ptr<ExpressionBaseNode> guard;
    // Resolved variable symbol (set by Pass 1)
    std::shared_ptr<VariableSymbol> resolvedSymbol;
};

class WildcardPattern : public PatternNode {
public:
    void accept(ASTVisitor& visitor) override;
};

class RangePattern : public PatternNode {
public:
    void accept(ASTVisitor& visitor) override;
    std::shared_ptr<ExpressionBaseNode> low;
    std::shared_ptr<ExpressionBaseNode> high;
};

// ============================================================================
// ParameterNode — Function/lambda parameter declaration
//
// resolvedSymbol set by Pass 1: eliminates scanForParamSymbols.
// ============================================================================

class ParameterNode : public AstBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::string name;
    std::shared_ptr<TypeNode> type;
    bool isReference = false;
    std::shared_ptr<ExpressionBaseNode> defaultValue;  // nullptr if no default

    // Set by Pass 1: direct link to the parameter's VariableSymbol
    std::shared_ptr<VariableSymbol> resolvedSymbol;
};

// ============================================================================
// ArgumentsNode — Call arguments with per-argument reference tracking
//
// IsReference[i] set by Pass 3, read by codegen. No need to re-resolve callee.
// ============================================================================

class ArgumentsNode : public AstBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::vector<std::shared_ptr<ExpressionBaseNode>> expressions;
    std::vector<bool> isReference;  // per-argument, set by TypeChecker
};

// ============================================================================
// ModifiersNode — Access modifiers, static, abstract, extern, virtual
// ============================================================================

class ModifiersNode : public AstBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::vector<ModifierType> modifiers;

    bool hasModifier(ModifierType mod) const {
        for (auto m : modifiers) if (m == mod) return true;
        return false;
    }
};

// ============================================================================
// Supporting structs (used by expression/statement/declaration nodes)
// ============================================================================

struct CaptureItem {
    std::string name;
    CaptureMode mode = CaptureMode::ByValue;
};

struct InterpolatedPart {
    InterpolatedPartKind kind;
    std::string text;                             // for Text parts
    std::shared_ptr<ExpressionBaseNode> expression;  // for Expression parts
};

struct MatchArm {
    std::shared_ptr<PatternNode> pattern;
    std::shared_ptr<AstBaseNode> body;  // ExpressionBaseNode or BlockStatementNode
};

struct PipeStage {
    std::shared_ptr<ExpressionBaseNode> function;  // identifier or qualified name
    std::vector<std::shared_ptr<ExpressionBaseNode>> extraArguments;
};

struct ElseIfClause {
    std::shared_ptr<ExpressionBaseNode> condition;
    std::shared_ptr<StatementBaseNode> body;
};

struct SwitchCase {
    std::shared_ptr<ExpressionBaseNode> value;  // nullptr for default case
    std::vector<std::shared_ptr<StatementBaseNode>> body;
};

struct DestructureElement {
    std::string name;
    std::shared_ptr<TypeNode> type;   // nullptr if inferred
    bool isInferred = false;
};

struct ImportTarget {
    std::string name;
    std::optional<std::string> alias;
};

// ============================================================================
// ModuleNode — A module declaration (top-level container)
// ============================================================================

class ModuleNode : public DeclarationBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::string name;
    std::vector<std::shared_ptr<DeclarationBaseNode>> declarations;

    // Resolved in Pass 1
    std::shared_ptr<ModuleSymbol> resolvedModule;
};

// ============================================================================
// ProgramNode — Root AST node (owns all modules)
// ============================================================================

class ProgramNode : public AstBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::vector<std::shared_ptr<ModuleNode>> modules;
    std::string entryPoint;
};

// ============================================================================
// ASTVisitor — Complete visitor interface
//
// Default implementations are no-ops so visitors only override what they need.
// Concrete visitors: SymbolTableBuilder, TypeResolver, TypeChecker,
// SemanticValidator, IRGenerator.
// ============================================================================

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    // ---- Base / structural nodes ----
    virtual void visit(ProgramNode& node) = 0;
    virtual void visit(ModuleNode& node) = 0;
    virtual void visit(BlockStatementNode& node) = 0;

    // ---- Type annotation nodes ----
    virtual void visit(TypeNode& node) {}
    virtual void visit(PrimitiveTypeNode& node) {}
    virtual void visit(NamedTypeNode& node) {}
    virtual void visit(PointerTypeNode& node) {}
    virtual void visit(ArrayTypeNode& node) {}
    virtual void visit(TupleTypeNode& node) {}
    virtual void visit(FunctionTypeNode& node) {}

    // ---- Pattern nodes ----
    virtual void visit(LiteralPattern& node) {}
    virtual void visit(IdentifierPattern& node) {}
    virtual void visit(WildcardPattern& node) {}
    virtual void visit(RangePattern& node) {}

    // ---- Parameter / Arguments / Modifiers ----
    virtual void visit(ParameterNode& node) {}
    virtual void visit(ArgumentsNode& node) {}
    virtual void visit(ModifiersNode& node) {}

    // ---- Expression nodes ----
    virtual void visit(IntegerLiteral& node) {}
    virtual void visit(FloatLiteral& node) {}
    virtual void visit(BoolLiteral& node) {}
    virtual void visit(CharLiteral& node) {}
    virtual void visit(StringLiteral& node) {}
    virtual void visit(NullLiteral& node) {}
    virtual void visit(InterpolatedStringExpression& node) {}
    virtual void visit(IdentifierExpression& node) {}
    virtual void visit(QualifiedNameExpression& node) {}
    virtual void visit(ThisExpression& node) {}
    virtual void visit(MemberAccessExpression& node) {}
    virtual void visit(BinaryExpression& node) {}
    virtual void visit(UnaryExpression& node) {}
    virtual void visit(AssignmentExpression& node) {}
    virtual void visit(TernaryExpression& node) {}
    virtual void visit(IndexExpression& node) {}
    virtual void visit(CallExpression& node) {}
    virtual void visit(CastExpression& node) {}
    virtual void visit(NewExpression& node) {}
    virtual void visit(SizeOfExpression& node) {}
    virtual void visit(TupleExpression& node) {}
    virtual void visit(MatchExpression& node) {}
    virtual void visit(PipeExpression& node) {}
    virtual void visit(LambdaExpression& node) {}
    virtual void visit(VariableDeclarationExpression& node) {}

    // ---- Statement nodes ----
    virtual void visit(ExpressionStatement& node) {}
    virtual void visit(ReturnStatement& node) {}
    virtual void visit(IfStatement& node) {}
    virtual void visit(ForStatement& node) {}
    virtual void visit(WhileStatement& node) {}
    virtual void visit(DoWhileStatement& node) {}
    virtual void visit(BreakStatement& node) {}
    virtual void visit(ContinueStatement& node) {}
    virtual void visit(DeleteStatement& node) {}
    virtual void visit(SwitchStatement& node) {}

    // ---- Declaration nodes ----
    virtual void visit(VariableDeclaration& node) {}
    virtual void visit(TupleDestructuringDeclaration& node) {}
    virtual void visit(FunctionDeclaration& node) {}
    virtual void visit(ConstructorDeclaration& node) {}
    virtual void visit(DestructorDeclaration& node) {}
    virtual void visit(ExternFunctionDeclaration& node) {}
    virtual void visit(OperatorDeclaration& node) {}
    virtual void visit(EnumMemberNode& node) {}
    virtual void visit(EnumDeclaration& node) {}
    virtual void visit(StructDeclaration& node) {}
    virtual void visit(ClassDeclaration& node) {}
    virtual void visit(InterfaceDeclaration& node) {}
    virtual void visit(ImportDeclaration& node) {}
};

} // namespace mingus
