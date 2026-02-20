//================================================================================
// MINGUS v1 - Statements Implementation
//================================================================================

#include "mingus/ast/Statements.h"
#include "mingus/ast/ASTVisitor.h"

namespace mingus {
namespace ast {

//================================================================================
// BlockStatement
//================================================================================
void BlockStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void BlockStatement::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// ExpressionStatement
//================================================================================
void ExpressionStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ExpressionStatement::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// ReturnStatement
//================================================================================
void ReturnStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ReturnStatement::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// IfStatement
//================================================================================
void IfStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void IfStatement::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// SwitchStatement
//================================================================================
void SwitchStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void SwitchStatement::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// ForStatement
//================================================================================
void ForStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ForStatement::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// WhileStatement
//================================================================================
void WhileStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void WhileStatement::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// BreakStatement
//================================================================================
void BreakStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void BreakStatement::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// ContinueStatement
//================================================================================
void ContinueStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ContinueStatement::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// DeleteStatement
//================================================================================
void DeleteStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void DeleteStatement::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// RawBlock
//================================================================================
void RawBlock::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void RawBlock::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

} // namespace ast
} // namespace mingus
