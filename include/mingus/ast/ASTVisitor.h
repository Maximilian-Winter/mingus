//================================================================================
// MINGUS v1 - AST Visitor Interface
// Visitor pattern for traversing and processing the AST
//================================================================================

#pragma once

namespace mingus {
namespace ast {

// Forward declarations for all AST node types
// Program structure
class ProgramNode;
class ModuleNode;
class ImportNode;

// Type nodes
class TypeNode;
class PrimitiveTypeNode;
class NamedTypeNode;
class PointerTypeNode;
class ArrayTypeNode;
class TupleTypeNode;
class FunctionTypeNode;

// Declarations
class DeclarationNode;
class VariableDeclaration;
class TupleDestructuringDeclaration;
class FunctionDeclaration;
class ConstructorDeclaration;
class DestructorDeclaration;
class OperatorDeclaration;
class ExternFunctionDeclaration;
class EnumMemberNode;
class EnumDeclaration;
class StructDeclaration;
class ClassDeclaration;
class InterfaceDeclaration;
class ParameterNode;

// Statements
class StatementNode;
class BlockStatement;
class ExpressionStatement;
class ReturnStatement;
class IfStatement;
class SwitchStatement;
class ForStatement;
class WhileStatement;
class BreakStatement;
class ContinueStatement;
class DeleteStatement;
class RawBlock;

// Expressions
class ExpressionNode;
class IntegerLiteral;
class FloatLiteral;
class BoolLiteral;
class CharLiteral;
class StringLiteral;
class InterpolatedString;
class NullLiteral;
class IdentifierExpression;
class QualifiedNameExpression;
class MemberAccessExpression;
class ThisExpression;
class BinaryExpression;
class UnaryExpression;
class AssignmentExpression;
class TernaryExpression;
class CallExpression;
class NewExpression;
class IndexExpression;
class CastExpression;
class SizeOfExpression;
class AlignOfExpression;
class PipeExpression;
class MatchExpression;
class TupleExpression;
class LambdaExpression;

// Patterns
class PatternNode;
class LiteralPattern;
class RangePattern;
class WildcardPattern;
class BindingPattern;
class TuplePattern;
class GuardedPattern;

//================================================================================
// ASTVisitor - visitor interface for AST traversal
//================================================================================
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    // Program structure
    virtual void visit(ProgramNode& node) = 0;
    virtual void visit(ModuleNode& node) = 0;
    virtual void visit(ImportNode& node) = 0;

    // Type nodes
    virtual void visit(TypeNode& node) = 0;
    virtual void visit(PrimitiveTypeNode& node) = 0;
    virtual void visit(NamedTypeNode& node) = 0;
    virtual void visit(PointerTypeNode& node) = 0;
    virtual void visit(ArrayTypeNode& node) = 0;
    virtual void visit(TupleTypeNode& node) = 0;
    virtual void visit(FunctionTypeNode& node) = 0;

    // Declarations
    virtual void visit(VariableDeclaration& node) = 0;
    virtual void visit(TupleDestructuringDeclaration& node) = 0;
    virtual void visit(FunctionDeclaration& node) = 0;
    virtual void visit(ConstructorDeclaration& node) = 0;
    virtual void visit(DestructorDeclaration& node) = 0;
    virtual void visit(OperatorDeclaration& node) = 0;
    virtual void visit(ExternFunctionDeclaration& node) = 0;
    virtual void visit(EnumMemberNode& node) = 0;
    virtual void visit(EnumDeclaration& node) = 0;
    virtual void visit(StructDeclaration& node) = 0;
    virtual void visit(ClassDeclaration& node) = 0;
    virtual void visit(InterfaceDeclaration& node) {}
    virtual void visit(ParameterNode& node) = 0;

    // Statements
    virtual void visit(BlockStatement& node) = 0;
    virtual void visit(ExpressionStatement& node) = 0;
    virtual void visit(ReturnStatement& node) = 0;
    virtual void visit(IfStatement& node) = 0;
    virtual void visit(SwitchStatement& node) = 0;
    virtual void visit(ForStatement& node) = 0;
    virtual void visit(WhileStatement& node) = 0;
    virtual void visit(BreakStatement& node) = 0;
    virtual void visit(ContinueStatement& node) = 0;
    virtual void visit(DeleteStatement& node) = 0;
    virtual void visit(RawBlock& node) = 0;

    // Expressions
    virtual void visit(IntegerLiteral& node) = 0;
    virtual void visit(FloatLiteral& node) = 0;
    virtual void visit(BoolLiteral& node) = 0;
    virtual void visit(CharLiteral& node) = 0;
    virtual void visit(StringLiteral& node) = 0;
    virtual void visit(InterpolatedString& node) = 0;
    virtual void visit(NullLiteral& node) = 0;
    virtual void visit(IdentifierExpression& node) = 0;
    virtual void visit(QualifiedNameExpression& node) = 0;
    virtual void visit(MemberAccessExpression& node) = 0;
    virtual void visit(ThisExpression& node) = 0;
    virtual void visit(BinaryExpression& node) = 0;
    virtual void visit(UnaryExpression& node) = 0;
    virtual void visit(AssignmentExpression& node) = 0;
    virtual void visit(TernaryExpression& node) = 0;
    virtual void visit(CallExpression& node) = 0;
    virtual void visit(NewExpression& node) = 0;
    virtual void visit(IndexExpression& node) = 0;
    virtual void visit(CastExpression& node) = 0;
    virtual void visit(SizeOfExpression& node) = 0;
    virtual void visit(AlignOfExpression& node) = 0;
    virtual void visit(PipeExpression& node) = 0;
    virtual void visit(MatchExpression& node) = 0;
    virtual void visit(TupleExpression& node) = 0;
    virtual void visit(LambdaExpression& node) = 0;

    // Patterns
    virtual void visit(LiteralPattern& node) = 0;
    virtual void visit(RangePattern& node) = 0;
    virtual void visit(WildcardPattern& node) = 0;
    virtual void visit(BindingPattern& node) = 0;
    virtual void visit(TuplePattern& node) = 0;
    virtual void visit(GuardedPattern& node) = 0;
};

//================================================================================
// ConstASTVisitor - const visitor interface for read-only AST traversal
//================================================================================
class ConstASTVisitor {
public:
    virtual ~ConstASTVisitor() = default;

    // Program structure
    virtual void visit(const ProgramNode& node) = 0;
    virtual void visit(const ModuleNode& node) = 0;
    virtual void visit(const ImportNode& node) = 0;

    // Type nodes
    virtual void visit(const TypeNode& node) = 0;
    virtual void visit(const PrimitiveTypeNode& node) = 0;
    virtual void visit(const NamedTypeNode& node) = 0;
    virtual void visit(const PointerTypeNode& node) = 0;
    virtual void visit(const ArrayTypeNode& node) = 0;
    virtual void visit(const TupleTypeNode& node) = 0;
    virtual void visit(const FunctionTypeNode& node) = 0;

    // Declarations
    virtual void visit(const VariableDeclaration& node) = 0;
    virtual void visit(const TupleDestructuringDeclaration& node) = 0;
    virtual void visit(const FunctionDeclaration& node) = 0;
    virtual void visit(const ConstructorDeclaration& node) = 0;
    virtual void visit(const DestructorDeclaration& node) = 0;
    virtual void visit(const OperatorDeclaration& node) = 0;
    virtual void visit(const ExternFunctionDeclaration& node) = 0;
    virtual void visit(const EnumMemberNode& node) = 0;
    virtual void visit(const EnumDeclaration& node) = 0;
    virtual void visit(const StructDeclaration& node) = 0;
    virtual void visit(const ClassDeclaration& node) = 0;
    virtual void visit(const InterfaceDeclaration& node) {}
    virtual void visit(const ParameterNode& node) = 0;

    // Statements
    virtual void visit(const BlockStatement& node) = 0;
    virtual void visit(const ExpressionStatement& node) = 0;
    virtual void visit(const ReturnStatement& node) = 0;
    virtual void visit(const IfStatement& node) = 0;
    virtual void visit(const SwitchStatement& node) = 0;
    virtual void visit(const ForStatement& node) = 0;
    virtual void visit(const WhileStatement& node) = 0;
    virtual void visit(const BreakStatement& node) = 0;
    virtual void visit(const ContinueStatement& node) = 0;
    virtual void visit(const DeleteStatement& node) = 0;
    virtual void visit(const RawBlock& node) = 0;

    // Expressions
    virtual void visit(const IntegerLiteral& node) = 0;
    virtual void visit(const FloatLiteral& node) = 0;
    virtual void visit(const BoolLiteral& node) = 0;
    virtual void visit(const CharLiteral& node) = 0;
    virtual void visit(const StringLiteral& node) = 0;
    virtual void visit(const InterpolatedString& node) = 0;
    virtual void visit(const NullLiteral& node) = 0;
    virtual void visit(const IdentifierExpression& node) = 0;
    virtual void visit(const QualifiedNameExpression& node) = 0;
    virtual void visit(const MemberAccessExpression& node) = 0;
    virtual void visit(const ThisExpression& node) = 0;
    virtual void visit(const BinaryExpression& node) = 0;
    virtual void visit(const UnaryExpression& node) = 0;
    virtual void visit(const AssignmentExpression& node) = 0;
    virtual void visit(const TernaryExpression& node) = 0;
    virtual void visit(const CallExpression& node) = 0;
    virtual void visit(const NewExpression& node) = 0;
    virtual void visit(const IndexExpression& node) = 0;
    virtual void visit(const CastExpression& node) = 0;
    virtual void visit(const SizeOfExpression& node) = 0;
    virtual void visit(const AlignOfExpression& node) = 0;
    virtual void visit(const PipeExpression& node) = 0;
    virtual void visit(const MatchExpression& node) = 0;
    virtual void visit(const TupleExpression& node) = 0;
    virtual void visit(const LambdaExpression& node) = 0;

    // Patterns
    virtual void visit(const LiteralPattern& node) = 0;
    virtual void visit(const RangePattern& node) = 0;
    virtual void visit(const WildcardPattern& node) = 0;
    virtual void visit(const BindingPattern& node) = 0;
    virtual void visit(const TuplePattern& node) = 0;
    virtual void visit(const GuardedPattern& node) = 0;
};

} // namespace ast
} // namespace mingus
