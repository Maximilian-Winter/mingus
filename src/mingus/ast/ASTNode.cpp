//================================================================================
// MINGUS v1 - ASTNode Base Implementation
//================================================================================

#include "mingus/ast/ASTNode.h"
#include "mingus/ast/ASTVisitor.h"

namespace mingus {
namespace ast {

// ExpressionNode accept implementations
void ExpressionNode::accept(ASTVisitor& visitor) {
    // Base ExpressionNode should not be visited directly
    // Derived classes override this
}

void ExpressionNode::accept(ConstASTVisitor& visitor) const {
    // Base ExpressionNode should not be visited directly
}

// StatementNode accept implementations
void StatementNode::accept(ASTVisitor& visitor) {
    // Base StatementNode should not be visited directly
}

void StatementNode::accept(ConstASTVisitor& visitor) const {
    // Base StatementNode should not be visited directly
}

// DeclarationNode accept implementations
void DeclarationNode::accept(ASTVisitor& visitor) {
    // Base DeclarationNode should not be visited directly
}

void DeclarationNode::accept(ConstASTVisitor& visitor) const {
    // Base DeclarationNode should not be visited directly
}

} // namespace ast
} // namespace mingus
