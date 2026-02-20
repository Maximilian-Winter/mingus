//================================================================================
// MINGUS v1 - Declarations Implementation
//================================================================================

#include "mingus/ast/Declarations.h"
#include "mingus/ast/ASTVisitor.h"

namespace mingus {
namespace ast {

//================================================================================
// Utility functions
//================================================================================
std::string accessModifierToString(AccessModifier mod) {
    switch (mod) {
        case AccessModifier::Public: return "public";
        case AccessModifier::Private: return "private";
        case AccessModifier::Protected: return "protected";
        case AccessModifier::None: return "";
    }
    return "";
}

std::string operatorKindToString(OperatorKind op) {
    switch (op) {
        case OperatorKind::Plus: return "+";
        case OperatorKind::Minus: return "-";
        case OperatorKind::Star: return "*";
        case OperatorKind::Divide: return "/";
        case OperatorKind::Modulo: return "%";
        case OperatorKind::Equal: return "==";
        case OperatorKind::NotEqual: return "!=";
        case OperatorKind::Less: return "<";
        case OperatorKind::LessEqual: return "<=";
        case OperatorKind::Greater: return ">";
        case OperatorKind::GreaterEqual: return ">=";
        case OperatorKind::Index: return "[]";
    }
    return "unknown";
}

//================================================================================
// ParameterNode
//================================================================================
void ParameterNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ParameterNode::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// VariableDeclaration
//================================================================================
void VariableDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void VariableDeclaration::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// TupleDestructuringDeclaration
//================================================================================
void TupleDestructuringDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void TupleDestructuringDeclaration::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// FunctionDeclaration
//================================================================================
void FunctionDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void FunctionDeclaration::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// ConstructorDeclaration
//================================================================================
void ConstructorDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ConstructorDeclaration::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// DestructorDeclaration
//================================================================================
void DestructorDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void DestructorDeclaration::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// OperatorDeclaration
//================================================================================
void OperatorDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void OperatorDeclaration::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// ExternFunctionDeclaration
//================================================================================
void ExternFunctionDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ExternFunctionDeclaration::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// EnumMemberNode
//================================================================================
void EnumMemberNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void EnumMemberNode::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// EnumDeclaration
//================================================================================
void EnumDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void EnumDeclaration::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// StructDeclaration
//================================================================================
void StructDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void StructDeclaration::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// ClassDeclaration
//================================================================================
void ClassDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ClassDeclaration::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// InterfaceDeclaration
//================================================================================
void InterfaceDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void InterfaceDeclaration::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

} // namespace ast
} // namespace mingus
