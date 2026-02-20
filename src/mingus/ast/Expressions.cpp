//================================================================================
// MINGUS v1 - Expressions Implementation
//================================================================================

#include "mingus/ast/Expressions.h"
#include "mingus/ast/Statements.h"
#include "mingus/ast/ASTVisitor.h"
#include "mingus/ast/ASTVisitor.h"

namespace mingus {
namespace ast {

//================================================================================
// BinaryOp utilities
//================================================================================
std::string binaryOpToString(BinaryOp op) {
    switch (op) {
        case BinaryOp::Add: return "+";
        case BinaryOp::Sub: return "-";
        case BinaryOp::Mul: return "*";
        case BinaryOp::Div: return "/";
        case BinaryOp::Mod: return "%";
        case BinaryOp::Equal: return "==";
        case BinaryOp::NotEqual: return "!=";
        case BinaryOp::Less: return "<";
        case BinaryOp::LessEqual: return "<=";
        case BinaryOp::Greater: return ">";
        case BinaryOp::GreaterEqual: return ">=";
        case BinaryOp::LogicalAnd: return "&&";
        case BinaryOp::LogicalOr: return "||";
        case BinaryOp::BitwiseAnd: return "&";
        case BinaryOp::BitwiseOr: return "|";
        case BinaryOp::BitwiseXor: return "^";
        case BinaryOp::ShiftLeft: return "<<";
        case BinaryOp::ShiftRight: return ">>";
    }
    return "unknown";
}

bool binaryOpIsComparison(BinaryOp op) {
    return op == BinaryOp::Equal || op == BinaryOp::NotEqual ||
           op == BinaryOp::Less || op == BinaryOp::LessEqual ||
           op == BinaryOp::Greater || op == BinaryOp::GreaterEqual;
}

bool binaryOpIsLogical(BinaryOp op) {
    return op == BinaryOp::LogicalAnd || op == BinaryOp::LogicalOr;
}

bool binaryOpIsBitwise(BinaryOp op) {
    return op == BinaryOp::BitwiseAnd || op == BinaryOp::BitwiseOr ||
           op == BinaryOp::BitwiseXor || op == BinaryOp::ShiftLeft ||
           op == BinaryOp::ShiftRight;
}

bool binaryOpIsArithmetic(BinaryOp op) {
    return op == BinaryOp::Add || op == BinaryOp::Sub ||
           op == BinaryOp::Mul || op == BinaryOp::Div || op == BinaryOp::Mod;
}

//================================================================================
// UnaryOp utilities
//================================================================================
std::string unaryOpToString(UnaryOp op) {
    switch (op) {
        case UnaryOp::Negate: return "-";
        case UnaryOp::LogicalNot: return "!";
        case UnaryOp::BitwiseNot: return "~";
        case UnaryOp::AddressOf: return "&";
        case UnaryOp::Dereference: return "*";
        case UnaryOp::PreIncrement: return "++";
        case UnaryOp::PreDecrement: return "--";
        case UnaryOp::PostIncrement: return "++";
        case UnaryOp::PostDecrement: return "--";
    }
    return "unknown";
}

bool unaryOpIsPrefix(UnaryOp op) {
    return op == UnaryOp::Negate || op == UnaryOp::LogicalNot ||
           op == UnaryOp::BitwiseNot || op == UnaryOp::AddressOf ||
           op == UnaryOp::Dereference || op == UnaryOp::PreIncrement ||
           op == UnaryOp::PreDecrement;
}

bool unaryOpIsPostfix(UnaryOp op) {
    return op == UnaryOp::PostIncrement || op == UnaryOp::PostDecrement;
}

//================================================================================
// AssignOp utilities
//================================================================================
std::string assignOpToString(AssignOp op) {
    switch (op) {
        case AssignOp::Assign: return "=";
        case AssignOp::AddAssign: return "+=";
        case AssignOp::SubAssign: return "-=";
        case AssignOp::MulAssign: return "*=";
        case AssignOp::DivAssign: return "/=";
        case AssignOp::ModAssign: return "%=";
        case AssignOp::AndAssign: return "&=";
        case AssignOp::OrAssign: return "|=";
        case AssignOp::XorAssign: return "^=";
        case AssignOp::ShiftLeftAssign: return "<<=";
        case AssignOp::ShiftRightAssign: return ">>=";
    }
    return "unknown";
}

bool assignOpIsCompound(AssignOp op) {
    return op != AssignOp::Assign;
}

BinaryOp assignOpToBinaryOp(AssignOp op) {
    switch (op) {
        case AssignOp::AddAssign: return BinaryOp::Add;
        case AssignOp::SubAssign: return BinaryOp::Sub;
        case AssignOp::MulAssign: return BinaryOp::Mul;
        case AssignOp::DivAssign: return BinaryOp::Div;
        case AssignOp::ModAssign: return BinaryOp::Mod;
        case AssignOp::AndAssign: return BinaryOp::BitwiseAnd;
        case AssignOp::OrAssign: return BinaryOp::BitwiseOr;
        case AssignOp::XorAssign: return BinaryOp::BitwiseXor;
        case AssignOp::ShiftLeftAssign: return BinaryOp::ShiftLeft;
        case AssignOp::ShiftRightAssign: return BinaryOp::ShiftRight;
        default: return BinaryOp::Add;  // Undefined for simple assign
    }
}

//================================================================================
// IntegerLiteral
//================================================================================
void IntegerLiteral::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void IntegerLiteral::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// FloatLiteral
//================================================================================
void FloatLiteral::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void FloatLiteral::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// BoolLiteral
//================================================================================
void BoolLiteral::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void BoolLiteral::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// CharLiteral
//================================================================================
void CharLiteral::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void CharLiteral::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// StringLiteral
//================================================================================
void StringLiteral::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void StringLiteral::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// InterpolatedString
//================================================================================
void InterpolatedString::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void InterpolatedString::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// NullLiteral
//================================================================================
void NullLiteral::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void NullLiteral::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// IdentifierExpression
//================================================================================
void IdentifierExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void IdentifierExpression::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// QualifiedNameExpression
//================================================================================
void QualifiedNameExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void QualifiedNameExpression::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// MemberAccessExpression
//================================================================================
void MemberAccessExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void MemberAccessExpression::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// ThisExpression
//================================================================================
void ThisExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ThisExpression::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// BinaryExpression
//================================================================================
void BinaryExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void BinaryExpression::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// UnaryExpression
//================================================================================
void UnaryExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void UnaryExpression::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// AssignmentExpression
//================================================================================
void AssignmentExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void AssignmentExpression::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// TernaryExpression
//================================================================================
void TernaryExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void TernaryExpression::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// CallExpression
//================================================================================
void CallExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void CallExpression::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// NewExpression
//================================================================================
void NewExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void NewExpression::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// IndexExpression
//================================================================================
void IndexExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void IndexExpression::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// CastExpression
//================================================================================
void CastExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void CastExpression::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// SizeOfExpression
//================================================================================
void SizeOfExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void SizeOfExpression::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// AlignOfExpression
//================================================================================
void AlignOfExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void AlignOfExpression::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// PipeExpression
//================================================================================
void PipeExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void PipeExpression::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// MatchExpression
//================================================================================
void MatchExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void MatchExpression::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// TupleExpression
//================================================================================
void TupleExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void TupleExpression::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

//================================================================================
// LambdaExpression
//================================================================================
bool LambdaExpression::hasExpressionBody() const {
    return body && body->is<ExpressionNode>();
}

bool LambdaExpression::hasBlockBody() const {
    return body && body->is<BlockStatement>();
}

NodePtr<ExpressionNode> LambdaExpression::getExpressionBody() const {
    if (hasExpressionBody()) {
        return std::static_pointer_cast<ExpressionNode>(
            std::shared_ptr<ASTNode>(body));
    }
    return nullptr;
}

NodePtr<BlockStatement> LambdaExpression::getBlockBody() const {
    if (hasBlockBody()) {
        return std::static_pointer_cast<BlockStatement>(
            std::shared_ptr<ASTNode>(body));
    }
    return nullptr;
}

void LambdaExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void LambdaExpression::accept(ConstASTVisitor& visitor) const {
    visitor.visit(*this);
}

} // namespace ast
} // namespace mingus
