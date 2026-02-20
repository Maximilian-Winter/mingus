//================================================================================
// MINGUS v1 - Patterns Implementation
//================================================================================

#include "mingus/ast/Patterns.h"
#include "mingus/ast/ASTVisitor.h"

namespace mingus {
namespace ast {

//================================================================================
// PatternNode base accept
//================================================================================
void PatternNode::accept(ASTVisitor& visitor) {
    // Base PatternNode should not be visited directly
}

void PatternNode::accept(ConstASTVisitor& visitor) const {
    // Base PatternNode should not be visited directly
}

//================================================================================
// LiteralPattern
//================================================================================
void LiteralPattern::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void LiteralPattern::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// RangePattern
//================================================================================
void RangePattern::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void RangePattern::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// WildcardPattern
//================================================================================
void WildcardPattern::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void WildcardPattern::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// BindingPattern
//================================================================================
void BindingPattern::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void BindingPattern::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// TuplePattern
//================================================================================
void TuplePattern::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void TuplePattern::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// GuardedPattern
//================================================================================
void GuardedPattern::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void GuardedPattern::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

} // namespace ast
} // namespace mingus
