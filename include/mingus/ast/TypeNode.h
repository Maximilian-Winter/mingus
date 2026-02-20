//================================================================================
// MINGUS v1 - TypeNode Hierarchy
// Type references in the AST (resolved to Type objects during type checking)
//================================================================================

#pragma once

#include "mingus/ast/ASTNode.h"
#include "mingus/ast/Type.h"

#include <memory>
#include <string>
#include <vector>

namespace mingus {
namespace ast {

//================================================================================
// TypeNode - base class for all type references in the AST
//================================================================================
class TypeNode : public ASTNode {
public:
    TypePtr<Type> resolvedType;  // Filled during type checking

    explicit TypeNode(const SourceLocation& loc = SourceLocation()) 
        : ASTNode(loc) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// PrimitiveTypeNode - direct primitive type references (int, double, etc.)
//================================================================================
class PrimitiveTypeNode : public TypeNode {
public:
    PrimitiveType::PrimitiveKind kind;

    explicit PrimitiveTypeNode(PrimitiveType::PrimitiveKind k, 
                               const SourceLocation& loc = SourceLocation())
        : TypeNode(loc), kind(k) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// NamedTypeNode - references to user-defined types (classes, structs, enums)
//================================================================================
class NamedTypeNode : public TypeNode {
public:
    std::vector<std::string> qualifiedName;  // e.g., ["Module", "ClassName"]

    explicit NamedTypeNode(std::vector<std::string> name,
                           const SourceLocation& loc = SourceLocation())
        : TypeNode(loc), qualifiedName(std::move(name)) {}

    std::string getName() const {
        std::string result;
        for (size_t i = 0; i < qualifiedName.size(); ++i) {
            if (i > 0) result += ".";
            result += qualifiedName[i];
        }
        return result;
    }

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// PointerTypeNode - baseType*
//================================================================================
class PointerTypeNode : public TypeNode {
public:
    NodePtr<TypeNode> baseType;

    explicit PointerTypeNode(NodePtr<TypeNode> base,
                             const SourceLocation& loc = SourceLocation())
        : TypeNode(loc), baseType(base) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// ArrayTypeNode - elementType[size] or elementType[]
//================================================================================
class ArrayTypeNode : public TypeNode {
public:
    NodePtr<TypeNode> elementType;
    NodePtr<ExpressionNode> size;  // nullptr for unsized arrays

    ArrayTypeNode(NodePtr<TypeNode> elemType, 
                  NodePtr<ExpressionNode> sz = nullptr,
                  const SourceLocation& loc = SourceLocation())
        : TypeNode(loc), elementType(elemType), size(sz) {}

    bool isUnsized() const { return size == nullptr; }

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// TupleTypeNode - (type0, type1, ...)
//================================================================================
class TupleTypeNode : public TypeNode {
public:
    NodeList<TypeNode> elementTypes;

    explicit TupleTypeNode(NodeList<TypeNode> elements,
                           const SourceLocation& loc = SourceLocation())
        : TypeNode(loc), elementTypes(std::move(elements)) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// FunctionTypeNode - fn(params) -> returnType
//================================================================================
class FunctionTypeNode : public TypeNode {
public:
    NodeList<TypeNode> parameterTypes;
    NodePtr<TypeNode> returnType;

    FunctionTypeNode(NodeList<TypeNode> params, 
                     NodePtr<TypeNode> retType,
                     const SourceLocation& loc = SourceLocation())
        : TypeNode(loc), parameterTypes(std::move(params)), returnType(retType) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

} // namespace ast
} // namespace mingus
