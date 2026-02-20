//================================================================================
// MINGUS v1 - Pattern Nodes
// Patterns for match expressions - tested against the match subject
//================================================================================

#pragma once

#include "mingus/ast/ASTNode.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mingus {
namespace ast {

// Forward declarations
class ExpressionNode;
class PatternNode;

template<typename T>
using PatternPtr = std::shared_ptr<T>;

template<typename T>
using PatternList = std::vector<PatternPtr<T>>;

//================================================================================
// PatternNode - base class for all patterns
// Patterns are not expressions - they're tested against the match subject
//================================================================================
class PatternNode : public ASTNode {
public:
    explicit PatternNode(const SourceLocation& loc = SourceLocation()) 
        : ASTNode(loc) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// LiteralPattern - match against a literal value
// Supports: int, float, bool, char, string, null, enum member
//================================================================================
class LiteralPattern : public PatternNode {
public:
    // The literal value is stored as an expression node
    NodePtr<ExpressionNode> value;

    explicit LiteralPattern(NodePtr<ExpressionNode> val,
                            const SourceLocation& loc = SourceLocation())
        : PatternNode(loc), value(val) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// RangePattern - match if value is in range [low, high]
//================================================================================
class RangePattern : public PatternNode {
public:
    int64_t low;
    int64_t high;

    RangePattern(int64_t l, int64_t h,
                 const SourceLocation& loc = SourceLocation())
        : PatternNode(loc), low(l), high(h) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// WildcardPattern - _ matches anything
//================================================================================
class WildcardPattern : public PatternNode {
public:
    explicit WildcardPattern(const SourceLocation& loc = SourceLocation())
        : PatternNode(loc) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// BindingPattern - name binds the subject to a variable
//================================================================================
class BindingPattern : public PatternNode {
public:
    std::string name;

    explicit BindingPattern(const std::string& n,
                            const SourceLocation& loc = SourceLocation())
        : PatternNode(loc), name(n) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// TuplePattern - match tuple elements against sub-patterns
//================================================================================
class TuplePattern : public PatternNode {
public:
    PatternList<PatternNode> elements;

    explicit TuplePattern(PatternList<PatternNode> elems,
                          const SourceLocation& loc = SourceLocation())
        : PatternNode(loc), elements(std::move(elems)) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// GuardedPattern - pattern if guard_condition
//================================================================================
class GuardedPattern : public PatternNode {
public:
    PatternPtr<PatternNode> innerPattern;
    NodePtr<ExpressionNode> guard;

    GuardedPattern(PatternPtr<PatternNode> inner,
                   NodePtr<ExpressionNode> grd,
                   const SourceLocation& loc = SourceLocation())
        : PatternNode(loc), innerPattern(inner), guard(grd) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

} // namespace ast
} // namespace mingus
