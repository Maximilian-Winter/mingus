// ============================================================================
// AstNode.cpp — accept() implementations for all AST nodes
//
// All concrete nodes from AstNode.h, Expressions.h, Statements.h,
// Declarations.h dispatch through the ASTVisitor.
// ============================================================================

#include "mingus/AstNode.h"
#include "mingus/Expressions.h"
#include "mingus/Statements.h"
#include "mingus/Declarations.h"

namespace mingus {

// ---- AstNode.h base nodes ----
void BlockStatementNode::accept(ASTVisitor& v) { v.visit(*this); }
void TypeNode::accept(ASTVisitor& v) { v.visit(*this); }
void PrimitiveTypeNode::accept(ASTVisitor& v) { v.visit(*this); }
void NamedTypeNode::accept(ASTVisitor& v) { v.visit(*this); }
void PointerTypeNode::accept(ASTVisitor& v) { v.visit(*this); }
void ArrayTypeNode::accept(ASTVisitor& v) { v.visit(*this); }
void TupleTypeNode::accept(ASTVisitor& v) { v.visit(*this); }
void FunctionTypeNode::accept(ASTVisitor& v) { v.visit(*this); }
void LiteralPattern::accept(ASTVisitor& v) { v.visit(*this); }
void IdentifierPattern::accept(ASTVisitor& v) { v.visit(*this); }
void WildcardPattern::accept(ASTVisitor& v) { v.visit(*this); }
void RangePattern::accept(ASTVisitor& v) { v.visit(*this); }
void ParameterNode::accept(ASTVisitor& v) { v.visit(*this); }
void ArgumentsNode::accept(ASTVisitor& v) { v.visit(*this); }
void ModifiersNode::accept(ASTVisitor& v) { v.visit(*this); }
void ModuleNode::accept(ASTVisitor& v) { v.visit(*this); }
void ProgramNode::accept(ASTVisitor& v) { v.visit(*this); }

// ---- Expressions.h ----
void IntegerLiteral::accept(ASTVisitor& v) { v.visit(*this); }
void FloatLiteral::accept(ASTVisitor& v) { v.visit(*this); }
void BoolLiteral::accept(ASTVisitor& v) { v.visit(*this); }
void CharLiteral::accept(ASTVisitor& v) { v.visit(*this); }
void StringLiteral::accept(ASTVisitor& v) { v.visit(*this); }
void NullLiteral::accept(ASTVisitor& v) { v.visit(*this); }
void InterpolatedStringExpression::accept(ASTVisitor& v) { v.visit(*this); }
void IdentifierExpression::accept(ASTVisitor& v) { v.visit(*this); }
void QualifiedNameExpression::accept(ASTVisitor& v) { v.visit(*this); }
void ThisExpression::accept(ASTVisitor& v) { v.visit(*this); }
void MemberAccessExpression::accept(ASTVisitor& v) { v.visit(*this); }
void BinaryExpression::accept(ASTVisitor& v) { v.visit(*this); }
void UnaryExpression::accept(ASTVisitor& v) { v.visit(*this); }
void AssignmentExpression::accept(ASTVisitor& v) { v.visit(*this); }
void TernaryExpression::accept(ASTVisitor& v) { v.visit(*this); }
void IndexExpression::accept(ASTVisitor& v) { v.visit(*this); }
void CallExpression::accept(ASTVisitor& v) { v.visit(*this); }
void CastExpression::accept(ASTVisitor& v) { v.visit(*this); }
void NewExpression::accept(ASTVisitor& v) { v.visit(*this); }
void SizeOfExpression::accept(ASTVisitor& v) { v.visit(*this); }
void TupleExpression::accept(ASTVisitor& v) { v.visit(*this); }
void MatchExpression::accept(ASTVisitor& v) { v.visit(*this); }
void PipeExpression::accept(ASTVisitor& v) { v.visit(*this); }
void LambdaExpression::accept(ASTVisitor& v) { v.visit(*this); }
void VariableDeclarationExpression::accept(ASTVisitor& v) { v.visit(*this); }
void MoveExpression::accept(ASTVisitor& v) { v.visit(*this); }
void ArrayLiteralExpression::accept(ASTVisitor& v) { v.visit(*this); }

// ---- Statements.h ----
void ExpressionStatement::accept(ASTVisitor& v) { v.visit(*this); }
void ReturnStatement::accept(ASTVisitor& v) { v.visit(*this); }
void IfStatement::accept(ASTVisitor& v) { v.visit(*this); }
void ForStatement::accept(ASTVisitor& v) { v.visit(*this); }
void WhileStatement::accept(ASTVisitor& v) { v.visit(*this); }
void DoWhileStatement::accept(ASTVisitor& v) { v.visit(*this); }
void LabeledStatement::accept(ASTVisitor& v) { v.visit(*this); }
void BreakStatement::accept(ASTVisitor& v) { v.visit(*this); }
void ContinueStatement::accept(ASTVisitor& v) { v.visit(*this); }
void DeleteStatement::accept(ASTVisitor& v) { v.visit(*this); }
void SwitchStatement::accept(ASTVisitor& v) { v.visit(*this); }

// ---- Declarations.h ----
void VariableDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void TupleDestructuringDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void FunctionDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void ConstructorDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void DestructorDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void ExternFunctionDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void ExternVariableDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void OperatorDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void EnumMemberNode::accept(ASTVisitor& v) { v.visit(*this); }
void EnumDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void StructDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void ClassDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void InterfaceDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void ImportDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void TypedefDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void LinkDirective::accept(ASTVisitor& v) { v.visit(*this); }
void OpaqueTypeDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void ExternStructDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void UnionDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void ExternUnionDeclaration::accept(ASTVisitor& v) { v.visit(*this); }

} // namespace mingus
