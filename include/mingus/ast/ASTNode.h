//================================================================================
// MINGUS v1 - AST Node Base Classes
//================================================================================

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace mingus {
namespace ast {

// Forward declarations
class ASTNode;
class ExpressionNode;
class StatementNode;
class DeclarationNode;
class TypeNode;
class PatternNode;
class Type;

// Type aliases for smart pointers
template<typename T>
using NodePtr = std::shared_ptr<T>;

template<typename T>
using NodeList = std::vector<NodePtr<T>>;

//================================================================================
// SourceLocation - tracks file, line, and column for error reporting
//================================================================================
struct SourceLocation {
    std::string file;
    int line;
    int column;

    SourceLocation() : file(""), line(0), column(0) {}
    SourceLocation(const std::string& f, int l, int c) 
        : file(f), line(l), column(c) {}

    std::string toString() const {
        return file + ":" + std::to_string(line) + ":" + std::to_string(column);
    }
};

//================================================================================
// ASTNode - base class for all AST nodes
//================================================================================
class ASTNode {
public:
    SourceLocation location;

    explicit ASTNode(const SourceLocation& loc = SourceLocation()) 
        : location(loc) {}
    virtual ~ASTNode() = default;

    // Visitor pattern support
    virtual void accept(class ASTVisitor& visitor) = 0;
    virtual void accept(class ConstASTVisitor& visitor) const = 0;

    // Utility for downcasting
    template<typename T>
    T* as() {
        return dynamic_cast<T*>(this);
    }

    template<typename T>
    const T* as() const {
        return dynamic_cast<const T*>(this);
    }

    template<typename T>
    bool is() const {
        return dynamic_cast<const T*>(this) != nullptr;
    }
};

//================================================================================
// ExpressionNode - base class for all expressions
// Every expression carries a resolved type filled during type checking
//================================================================================
class ExpressionNode : public ASTNode {
public:
    NodePtr<Type> resolvedType;  // Filled during type checking, nullptr until then

    explicit ExpressionNode(const SourceLocation& loc = SourceLocation()) 
        : ASTNode(loc) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// StatementNode - base class for all statements
//================================================================================
class StatementNode : public ASTNode {
public:
    explicit StatementNode(const SourceLocation& loc = SourceLocation()) 
        : ASTNode(loc) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// DeclarationNode - base class for all declarations
//================================================================================
class DeclarationNode : public ASTNode {
public:
    explicit DeclarationNode(const SourceLocation& loc = SourceLocation()) 
        : ASTNode(loc) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

} // namespace ast
} // namespace mingus
