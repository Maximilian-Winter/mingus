//================================================================================
// MINGUS v1 - TypeNode Implementation
//================================================================================

#include "mingus/ast/TypeNode.h"
#include "mingus/ast/ASTVisitor.h"

namespace mingus {
namespace ast {

// TypeNode accept implementations
void TypeNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void TypeNode::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

// PrimitiveTypeNode accept implementations
void PrimitiveTypeNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void PrimitiveTypeNode::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

// NamedTypeNode accept implementations
void NamedTypeNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void NamedTypeNode::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

// PointerTypeNode accept implementations
void PointerTypeNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void PointerTypeNode::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

// ArrayTypeNode accept implementations
void ArrayTypeNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ArrayTypeNode::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

// TupleTypeNode accept implementations
void TupleTypeNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void TupleTypeNode::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

// FunctionTypeNode accept implementations
void FunctionTypeNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void FunctionTypeNode::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

} // namespace ast
} // namespace mingus
