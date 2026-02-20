//================================================================================
// MINGUS v1 - Declaration Nodes
// Class, Struct, Enum, Function, Variable declarations and related nodes
//================================================================================

#pragma once

#include "mingus/ast/ASTNode.h"
#include "mingus/ast/TypeNode.h"
#include "mingus/ast/Statements.h"
#include "mingus/ast/Program.h"

#include <memory>
#include <string>
#include <vector>

namespace mingus {
namespace ast {

// Forward declarations
class StatementNode;
class BlockStatement;
class ExpressionNode;

//================================================================================
// AccessModifier - shared across declarations
//================================================================================
enum class AccessModifier {
    Public,
    Private,
    Protected,
    None
};

std::string accessModifierToString(AccessModifier mod);

//================================================================================
// ParameterNode - function/constructor/operator parameter
//================================================================================
class ParameterNode : public ASTNode {
public:
    std::string name;
    NodePtr<TypeNode> type;
    NodePtr<ExpressionNode> defaultValue;  // nullptr if no default
    bool isReference = false;              // true for int& x reference parameters

    ParameterNode(const std::string& n,
                  NodePtr<TypeNode> t,
                  NodePtr<ExpressionNode> defVal = nullptr,
                  bool isRef = false,
                  const SourceLocation& loc = SourceLocation())
        : ASTNode(loc), name(n), type(t), defaultValue(defVal), isReference(isRef) {}

    bool hasDefaultValue() const { return defaultValue != nullptr; }

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// VariableDeclaration - variable declarations (local, field, global)
//================================================================================
class VariableDeclaration : public DeclarationNode {
public:
    std::string name;
    AccessModifier accessModifier;
    bool isStatic;
    NodePtr<TypeNode> type;  // nullptr if inferred
    bool isInferred;
    NodePtr<ExpressionNode> initializer;  // required if isInferred

    VariableDeclaration(const std::string& n,
                        AccessModifier access,
                        bool stat,
                        NodePtr<TypeNode> t,
                        bool inferred,
                        NodePtr<ExpressionNode> init,
                        const SourceLocation& loc = SourceLocation())
        : DeclarationNode(loc), name(n), accessModifier(access), isStatic(stat),
          type(t), isInferred(inferred), initializer(init) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// DestructureElement - single element in a tuple destructuring
//================================================================================
struct DestructureElement {
    std::string name;
    NodePtr<TypeNode> type;  // nullptr if var
    bool isInferred;

    DestructureElement(const std::string& n, 
                       NodePtr<TypeNode> t = nullptr,
                       bool inferred = false)
        : name(n), type(t), isInferred(inferred) {}
};

//================================================================================
// TupleDestructuringDeclaration - (a, b) = tupleExpr
//================================================================================
class TupleDestructuringDeclaration : public StatementNode {
public:
    std::vector<DestructureElement> elements;
    NodePtr<ExpressionNode> initializer;

    TupleDestructuringDeclaration(std::vector<DestructureElement> elems,
                                   NodePtr<ExpressionNode> init,
                                   const SourceLocation& loc = SourceLocation())
        : StatementNode(loc), elements(std::move(elems)), initializer(init) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// FunctionDeclaration - function/method declaration
//================================================================================
class FunctionDeclaration : public DeclarationNode {
public:
    std::string name;
    AccessModifier accessModifier;
    bool isStatic;
    bool isAbstract;
    NodeList<ParameterNode> parameters;
    NodePtr<TypeNode> returnType;  // can be TupleTypeNode
    NodePtr<BlockStatement> body;  // nullptr if abstract or forward decl

    FunctionDeclaration(const std::string& n,
                        AccessModifier access,
                        bool stat,
                        bool abstr,
                        NodeList<ParameterNode> params,
                        NodePtr<TypeNode> retType,
                        NodePtr<BlockStatement> bdy,
                        const SourceLocation& loc = SourceLocation())
        : DeclarationNode(loc), name(n), accessModifier(access), isStatic(stat),
          isAbstract(abstr), parameters(std::move(params)), returnType(retType), body(bdy) {}

    bool isForwardDeclaration() const { return body == nullptr; }

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// ConstructorDeclaration - class/struct constructor
//================================================================================
class ConstructorDeclaration : public DeclarationNode {
public:
    AccessModifier accessModifier;
    NodeList<ParameterNode> parameters;
    NodePtr<BlockStatement> body;
    NodeList<ExpressionNode> superArgs;  // Arguments to super() call
    bool hasSuperCall = false;           // True if constructor has : super(...)

    ConstructorDeclaration(AccessModifier access,
                           NodeList<ParameterNode> params,
                           NodePtr<BlockStatement> bdy,
                           NodeList<ExpressionNode> superArgsList = {},
                           bool hasSuperCallFlag = false,
                           const SourceLocation& loc = SourceLocation())
        : DeclarationNode(loc), accessModifier(access),
          parameters(std::move(params)), body(bdy),
          superArgs(std::move(superArgsList)), hasSuperCall(hasSuperCallFlag) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// DestructorDeclaration - class destructor
//================================================================================
class DestructorDeclaration : public DeclarationNode {
public:
    NodePtr<BlockStatement> body;

    explicit DestructorDeclaration(NodePtr<BlockStatement> bdy,
                                   const SourceLocation& loc = SourceLocation())
        : DeclarationNode(loc), body(bdy) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// OperatorKind - overloaded operators
//================================================================================
enum class OperatorKind {
    Plus,
    Minus,
    Star,
    Divide,
    Modulo,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Index
};

std::string operatorKindToString(OperatorKind op);

//================================================================================
// OperatorDeclaration - operator overloading
//================================================================================
class OperatorDeclaration : public DeclarationNode {
public:
    OperatorKind op;
    NodeList<ParameterNode> parameters;
    NodePtr<TypeNode> returnType;
    NodePtr<BlockStatement> body;

    OperatorDeclaration(OperatorKind o,
                        NodeList<ParameterNode> params,
                        NodePtr<TypeNode> retType,
                        NodePtr<BlockStatement> bdy,
                        const SourceLocation& loc = SourceLocation())
        : DeclarationNode(loc), op(o), parameters(std::move(params)), 
          returnType(retType), body(bdy) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// ExternFunctionDeclaration - external function declaration
//================================================================================
class ExternFunctionDeclaration : public DeclarationNode {
public:
    std::string name;
    NodeList<ParameterNode> parameters;
    NodePtr<TypeNode> returnType;

    ExternFunctionDeclaration(const std::string& n,
                              NodeList<ParameterNode> params,
                              NodePtr<TypeNode> retType,
                              const SourceLocation& loc = SourceLocation())
        : DeclarationNode(loc), name(n), parameters(std::move(params)), 
          returnType(retType) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// EnumMemberNode - single member in an enum declaration
//================================================================================
class EnumMemberNode : public ASTNode {
public:
    std::string name;
    NodePtr<ExpressionNode> value;  // nullptr for auto-assigned values

    EnumMemberNode(const std::string& n,
                   NodePtr<ExpressionNode> val = nullptr,
                   const SourceLocation& loc = SourceLocation())
        : ASTNode(loc), name(n), value(val) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// EnumDeclaration - enum type declaration
//================================================================================
class EnumDeclaration : public DeclarationNode {
public:
    std::string name;
    AccessModifier accessModifier;
    NodePtr<TypeNode> underlyingType;  // nullptr defaults to int
    NodeList<EnumMemberNode> members;

    EnumDeclaration(const std::string& n,
                    AccessModifier access,
                    NodePtr<TypeNode> underlying,
                    NodeList<EnumMemberNode> mems,
                    const SourceLocation& loc = SourceLocation())
        : DeclarationNode(loc), name(n), accessModifier(access), 
          underlyingType(underlying), members(std::move(mems)) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// StructDeclaration - struct type declaration
//================================================================================
class StructDeclaration : public DeclarationNode {
public:
    std::string name;
    AccessModifier accessModifier;
    NodeList<OperatorDeclaration> operators;
    NodeList<FunctionDeclaration> methods;
    NodeList<VariableDeclaration> fields;

    StructDeclaration(const std::string& n,
                      AccessModifier access,
                      NodeList<OperatorDeclaration> ops,
                      NodeList<FunctionDeclaration> meths,
                      NodeList<VariableDeclaration> flds,
                      const SourceLocation& loc = SourceLocation())
        : DeclarationNode(loc), name(n), accessModifier(access), 
          operators(std::move(ops)), methods(std::move(meths)), fields(std::move(flds)) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// ClassDeclaration - class type declaration
//================================================================================
class ClassDeclaration : public DeclarationNode {
public:
    std::string name;
    AccessModifier accessModifier;
    bool isStatic;
    bool isAbstract;
    std::vector<QualifiedName> baseClasses;
    NodePtr<ConstructorDeclaration> constructor;  // nullptr if no explicit constructor
    NodePtr<DestructorDeclaration> destructor;    // nullptr if no explicit destructor
    NodeList<OperatorDeclaration> operators;
    NodeList<FunctionDeclaration> methods;
    NodeList<VariableDeclaration> fields;

    ClassDeclaration(const std::string& n,
                     AccessModifier access,
                     bool stat,
                     bool abstr,
                     std::vector<QualifiedName> bases,
                     NodePtr<ConstructorDeclaration> ctor,
                     NodePtr<DestructorDeclaration> dtor,
                     NodeList<OperatorDeclaration> ops,
                     NodeList<FunctionDeclaration> meths,
                     NodeList<VariableDeclaration> flds,
                     const SourceLocation& loc = SourceLocation())
        : DeclarationNode(loc), name(n), accessModifier(access), isStatic(stat),
          isAbstract(abstr), baseClasses(std::move(bases)), constructor(ctor),
          destructor(dtor), operators(std::move(ops)), methods(std::move(meths)),
          fields(std::move(flds)) {}

    bool hasConstructor() const { return constructor != nullptr; }
    bool hasDestructor() const { return destructor != nullptr; }
    bool hasBaseClass() const { return !baseClasses.empty(); }

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// InterfaceDeclaration - interface type declaration (Go-style fat pointer dispatch)
//================================================================================
class InterfaceDeclaration : public DeclarationNode {
public:
    std::string name;
    AccessModifier accessModifier;
    NodeList<FunctionDeclaration> methods;  // Abstract method signatures (no bodies)

    InterfaceDeclaration(const std::string& n,
                         AccessModifier access,
                         NodeList<FunctionDeclaration> meths,
                         const SourceLocation& loc = SourceLocation())
        : DeclarationNode(loc), name(n), accessModifier(access),
          methods(std::move(meths)) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

} // namespace ast
} // namespace mingus
