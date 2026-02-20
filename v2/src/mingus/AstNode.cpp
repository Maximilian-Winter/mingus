// ============================================================================
// AstNode.cpp — AST base node accept() implementations
// ============================================================================

#include "mingus/AstNode.h"

namespace mingus {

void BlockStatementNode::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void TypeNode::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ParameterNode::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ArgumentsNode::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ModifiersNode::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ModuleNode::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ProgramNode::accept(ASTVisitor& visitor) { visitor.visit(*this); }

} // namespace mingus
