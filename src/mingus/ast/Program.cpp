//================================================================================
// MINGUS v1 - Program Structure Implementation
//================================================================================

#include "mingus/ast/Program.h"
#include "mingus/ast/ASTVisitor.h"

namespace mingus {
namespace ast {

// Static member definition
const std::string QualifiedName::empty_;

// ImportNode accept implementations
void ImportNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ImportNode::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

// ModuleNode accept implementations
void ModuleNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ModuleNode::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

// ProgramNode accept implementations
void ProgramNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ProgramNode::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

} // namespace ast
} // namespace mingus
