#pragma once

// ============================================================================
// Declarations.h — All concrete declaration AST nodes for Mingus V2
//
// Declaration nodes represent user declarations: variables, functions,
// classes, structs, enums, interfaces, imports, operators.
//
// ModuleNode is defined in AstNode.h (used broadly).
// ============================================================================

#include "AstNode.h"

#include <memory>
#include <string>
#include <vector>

namespace mingus {

// ============================================================================
// VariableDeclaration — var x: int = expr; or var x = expr; (inferred)
//
// This is the declaration-level node. VariableDeclarationExpression (in
// Expressions.h) is the expression-level variant used in ExpressionStatement.
// Both produce VariableSymbol during Pass 1.
// ============================================================================

class VariableDeclaration : public DeclarationBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::string name;
    AccessModifier accessModifier = AccessModifier::Public;
    bool isStatic = false;
    bool isConst = false;
    std::shared_ptr<TypeNode> type;  // nullptr if inferred
    bool isInferred = false;
    std::shared_ptr<ExpressionBaseNode> initializer;  // required if inferred

    // Resolved in Pass 1
    std::shared_ptr<VariableSymbol> resolvedVariable;
};

// ============================================================================
// TupleDestructuringDeclaration — var (a, b, c) = tupleExpr;
// ============================================================================

class TupleDestructuringDeclaration : public StatementBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::vector<DestructureElement> elements;
    std::shared_ptr<ExpressionBaseNode> initializer;

    // Resolved variable symbols for each element (set by Pass 1)
    std::vector<std::shared_ptr<VariableSymbol>> resolvedVariables;
};

// ============================================================================
// FunctionDeclaration — func name(params) => RetType { body }
// ============================================================================

class FunctionDeclaration : public DeclarationBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::string name;
    AccessModifier accessModifier = AccessModifier::Public;
    bool isStatic = false;
    bool isAbstract = false;
    bool isVirtual = false;
    bool isOverride = false;
    std::vector<std::shared_ptr<ParameterNode>> parameters;
    std::shared_ptr<TypeNode> returnType;
    std::shared_ptr<BlockStatementNode> body;  // nullptr if abstract

    // Resolved in Pass 1
    std::shared_ptr<FunctionSymbol> resolvedFunction;
};

// ============================================================================
// ConstructorDeclaration — constructor(params) { body }
// ============================================================================

class ConstructorDeclaration : public DeclarationBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    AccessModifier accessModifier = AccessModifier::Public;
    std::vector<std::shared_ptr<ParameterNode>> parameters;
    std::shared_ptr<BlockStatementNode> body;

    // Super constructor call arguments (if hasSuperCall)
    std::vector<std::shared_ptr<ExpressionBaseNode>> superArgs;
    bool hasSuperCall = false;

    // Resolved in Pass 1
    std::shared_ptr<ConstructorSymbol> resolvedConstructor;
};

// ============================================================================
// DestructorDeclaration — destructor() { body }
// ============================================================================

class DestructorDeclaration : public DeclarationBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::shared_ptr<BlockStatementNode> body;

    // Resolved in Pass 1
    std::shared_ptr<DestructorSymbol> resolvedDestructor;
};

// ============================================================================
// ExternFunctionDeclaration — extern func name(params) => RetType;
// ============================================================================

class ExternFunctionDeclaration : public DeclarationBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::string name;
    std::vector<std::shared_ptr<ParameterNode>> parameters;
    std::shared_ptr<TypeNode> returnType;
    bool isVariadic = false;

    // Resolved in Pass 1
    std::shared_ptr<FunctionSymbol> resolvedFunction;
};

// ============================================================================
// OperatorDeclaration — operator+(params) => RetType { body }
// ============================================================================

class OperatorDeclaration : public DeclarationBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    OverloadableOp op;
    std::vector<std::shared_ptr<ParameterNode>> parameters;
    std::shared_ptr<TypeNode> returnType;
    std::shared_ptr<BlockStatementNode> body;

    // Resolved in Pass 1
    std::shared_ptr<OperatorSymbol> resolvedOperator;
};

// ============================================================================
// EnumDeclaration — enum Color { Red, Green = 1, Blue }
// ============================================================================

class EnumMemberNode : public AstBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::string name;
    std::shared_ptr<ExpressionBaseNode> value;  // nullptr for auto-assigned
};

class EnumDeclaration : public DeclarationBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::string name;
    AccessModifier accessModifier = AccessModifier::Public;
    std::shared_ptr<TypeNode> underlyingType;  // nullptr defaults to int
    std::vector<std::shared_ptr<EnumMemberNode>> members;

    // Resolved in Pass 1
    std::shared_ptr<EnumSymbol> resolvedEnum;
};

// ============================================================================
// StructDeclaration — struct Vec2 { double x; double y; operator+ ... }
// ============================================================================

class StructDeclaration : public DeclarationBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::string name;
    AccessModifier accessModifier = AccessModifier::Public;
    std::vector<std::shared_ptr<VariableDeclaration>> fields;
    std::vector<std::shared_ptr<FunctionDeclaration>> methods;
    std::vector<std::shared_ptr<OperatorDeclaration>> operators;

    // Resolved in Pass 1
    std::shared_ptr<StructSymbol> resolvedStruct;
};

// ============================================================================
// ClassDeclaration — class Dog : Animal implements Drawable { ... }
// ============================================================================

class ClassDeclaration : public DeclarationBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::string name;
    AccessModifier accessModifier = AccessModifier::Public;
    bool isStatic = false;
    bool isAbstract = false;
    std::vector<std::string> baseClasses;  // unresolved names from parse

    std::vector<std::shared_ptr<VariableDeclaration>> fields;
    std::vector<std::shared_ptr<FunctionDeclaration>> methods;
    std::vector<std::shared_ptr<OperatorDeclaration>> operators;
    std::shared_ptr<ConstructorDeclaration> constructor;   // nullptr if auto-generated
    std::shared_ptr<DestructorDeclaration> destructor;     // nullptr if auto-generated

    // Resolved in Pass 1
    std::shared_ptr<ClassSymbol> resolvedClass;
};

// ============================================================================
// InterfaceDeclaration — interface Drawable { func draw() => void; }
// ============================================================================

class InterfaceDeclaration : public DeclarationBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::string name;
    AccessModifier accessModifier = AccessModifier::Public;
    std::vector<std::shared_ptr<FunctionDeclaration>> methods;  // abstract only

    // Resolved in Pass 1
    std::shared_ptr<InterfaceSymbol> resolvedInterface;
};

// ============================================================================
// ImportDeclaration — import name from Module; or import Module;
// ============================================================================

class ImportDeclaration : public DeclarationBaseNode {
public:
    void accept(ASTVisitor& visitor) override;

    std::vector<ImportTarget> targets;
    std::vector<std::string> sourcePath;  // ["ModuleName"] or ["Pkg", "Sub"]

    // Is this a whole-module import (import Module;)?
    bool isWholeModule = false;
};

} // namespace mingus
