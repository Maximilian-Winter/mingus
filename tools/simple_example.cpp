//================================================================================
// MINGUS v1 - Simple AST Construction Example
// Demonstrates how to construct and traverse a simple AST
//================================================================================

#include "mingus/AST.h"
#include <iostream>
#include <memory>

using namespace mingus::ast;

//================================================================================
// Simple Print Visitor - demonstrates AST traversal
//================================================================================
class PrintVisitor : public ConstASTVisitor {
public:
    int indent = 0;

    void printIndent() const {
        for (int i = 0; i < indent; ++i) {
            std::cout << "  ";
        }
    }

    // Program structure
    void visit(const ProgramNode& node) override {
        printIndent();
        std::cout << "Program:\n";
        indent++;
        for (const auto& module : node.modules) {
            module->accept(*this);
        }
        indent--;
    }

    void visit(const ModuleNode& node) override {
        printIndent();
        std::cout << "Module: " << node.name << "\n";
        indent++;
        for (const auto& decl : node.declarations) {
            decl->accept(*this);
        }
        indent--;
    }

    void visit(const ImportNode& node) override {
        printIndent();
        std::cout << "Import: " << node.source.toString() << "\n";
    }

    // Type nodes
    void visit(const TypeNode& node) override {
        printIndent();
        std::cout << "TypeNode\n";
    }

    void visit(const PrimitiveTypeNode& node) override {
        printIndent();
        std::cout << "PrimitiveType: " << static_cast<int>(node.kind) << "\n";
    }

    void visit(const NamedTypeNode& node) override {
        printIndent();
        std::cout << "NamedType: " << node.getName() << "\n";
    }

    void visit(const PointerTypeNode& node) override {
        printIndent();
        std::cout << "PointerType:\n";
        indent++;
        node.baseType->accept(*this);
        indent--;
    }

    void visit(const ArrayTypeNode& node) override {
        printIndent();
        std::cout << "ArrayType:\n";
        indent++;
        node.elementType->accept(*this);
        indent--;
    }

    void visit(const TupleTypeNode& node) override {
        printIndent();
        std::cout << "TupleType:\n";
        indent++;
        for (const auto& elem : node.elementTypes) {
            elem->accept(*this);
        }
        indent--;
    }

    void visit(const FunctionTypeNode& node) override {
        printIndent();
        std::cout << "FunctionType:\n";
        indent++;
        for (const auto& param : node.parameterTypes) {
            param->accept(*this);
        }
        node.returnType->accept(*this);
        indent--;
    }

    // Declarations
    void visit(const VariableDeclaration& node) override {
        printIndent();
        std::cout << "VariableDeclaration: " << node.name;
        if (node.isInferred) {
            std::cout << " (inferred)";
        }
        std::cout << "\n";
        indent++;
        if (node.type) {
            node.type->accept(*this);
        }
        if (node.initializer) {
            printIndent();
            std::cout << "Initializer:\n";
            indent++;
            node.initializer->accept(*this);
            indent--;
        }
        indent--;
    }

    void visit(const TupleDestructuringDeclaration& node) override {
        printIndent();
        std::cout << "TupleDestructuringDeclaration:\n";
    }

    void visit(const FunctionDeclaration& node) override {
        printIndent();
        std::cout << "FunctionDeclaration: " << node.name << "\n";
        indent++;
        for (const auto& param : node.parameters) {
            param->accept(*this);
        }
        if (node.returnType) {
            printIndent();
            std::cout << "Returns:\n";
            indent++;
            node.returnType->accept(*this);
            indent--;
        }
        if (node.body) {
            node.body->accept(*this);
        }
        indent--;
    }

    void visit(const ConstructorDeclaration& node) override {
        printIndent();
        std::cout << "ConstructorDeclaration\n";
    }

    void visit(const DestructorDeclaration& node) override {
        printIndent();
        std::cout << "DestructorDeclaration\n";
    }

    void visit(const OperatorDeclaration& node) override {
        printIndent();
        std::cout << "OperatorDeclaration: " << operatorKindToString(node.op) << "\n";
    }

    void visit(const ExternFunctionDeclaration& node) override {
        printIndent();
        std::cout << "ExternFunctionDeclaration: " << node.name << "\n";
    }

    void visit(const EnumMemberNode& node) override {
        printIndent();
        std::cout << "EnumMember: " << node.name << "\n";
    }

    void visit(const EnumDeclaration& node) override {
        printIndent();
        std::cout << "EnumDeclaration: " << node.name << "\n";
        indent++;
        for (const auto& member : node.members) {
            member->accept(*this);
        }
        indent--;
    }

    void visit(const StructDeclaration& node) override {
        printIndent();
        std::cout << "StructDeclaration: " << node.name << "\n";
    }

    void visit(const ClassDeclaration& node) override {
        printIndent();
        std::cout << "ClassDeclaration: " << node.name << "\n";
    }

    void visit(const ParameterNode& node) override {
        printIndent();
        std::cout << "Parameter: " << node.name << "\n";
        indent++;
        node.type->accept(*this);
        indent--;
    }

    // Statements
    void visit(const BlockStatement& node) override {
        printIndent();
        std::cout << "Block:\n";
        indent++;
        for (const auto& stmt : node.statements) {
            stmt->accept(*this);
        }
        indent--;
    }

    void visit(const ExpressionStatement& node) override {
        printIndent();
        std::cout << "ExpressionStatement:\n";
        indent++;
        node.expression->accept(*this);
        indent--;
    }

    void visit(const ReturnStatement& node) override {
        printIndent();
        std::cout << "ReturnStatement:\n";
        if (node.value) {
            indent++;
            node.value->accept(*this);
            indent--;
        }
    }

    void visit(const IfStatement& node) override {
        printIndent();
        std::cout << "IfStatement:\n";
        indent++;
        printIndent();
        std::cout << "Condition:\n";
        indent++;
        node.condition->accept(*this);
        indent--;
        printIndent();
        std::cout << "Then:\n";
        indent++;
        node.thenBody->accept(*this);
        indent--;
        if (node.elseBody) {
            printIndent();
            std::cout << "Else:\n";
            indent++;
            node.elseBody->accept(*this);
            indent--;
        }
        indent--;
    }

    void visit(const SwitchStatement& node) override {
        printIndent();
        std::cout << "SwitchStatement\n";
    }

    void visit(const ForStatement& node) override {
        printIndent();
        std::cout << "ForStatement\n";
    }

    void visit(const WhileStatement& node) override {
        printIndent();
        std::cout << "WhileStatement\n";
    }

    void visit(const BreakStatement& node) override {
        printIndent();
        std::cout << "BreakStatement\n";
    }

    void visit(const ContinueStatement& node) override {
        printIndent();
        std::cout << "ContinueStatement\n";
    }

    void visit(const DeleteStatement& node) override {
        printIndent();
        std::cout << "DeleteStatement\n";
    }

    void visit(const RawBlock& node) override {
        printIndent();
        std::cout << "RawBlock\n";
    }

    // Expressions
    void visit(const IntegerLiteral& node) override {
        printIndent();
        std::cout << "IntegerLiteral: " << node.value << "\n";
    }

    void visit(const FloatLiteral& node) override {
        printIndent();
        std::cout << "FloatLiteral: " << node.value << "\n";
    }

    void visit(const BoolLiteral& node) override {
        printIndent();
        std::cout << "BoolLiteral: " << (node.value ? "true" : "false") << "\n";
    }

    void visit(const CharLiteral& node) override {
        printIndent();
        std::cout << "CharLiteral: '" << node.value << "'\n";
    }

    void visit(const StringLiteral& node) override {
        printIndent();
        std::cout << "StringLiteral: \"" << node.value << "\"\n";
    }

    void visit(const InterpolatedString& node) override {
        printIndent();
        std::cout << "InterpolatedString\n";
    }

    void visit(const NullLiteral& node) override {
        printIndent();
        std::cout << "NullLiteral\n";
    }

    void visit(const IdentifierExpression& node) override {
        printIndent();
        std::cout << "Identifier: " << node.name << "\n";
    }

    void visit(const QualifiedNameExpression& node) override {
        printIndent();
        std::cout << "QualifiedName: " << node.getName() << "\n";
    }

    void visit(const MemberAccessExpression& node) override {
        printIndent();
        std::cout << "MemberAccess: ." << node.memberName << "\n";
        indent++;
        node.object->accept(*this);
        indent--;
    }

    void visit(const ThisExpression& node) override {
        printIndent();
        std::cout << "ThisExpression\n";
    }

    void visit(const BinaryExpression& node) override {
        printIndent();
        std::cout << "BinaryExpression: " << binaryOpToString(node.op) << "\n";
        indent++;
        node.left->accept(*this);
        node.right->accept(*this);
        indent--;
    }

    void visit(const UnaryExpression& node) override {
        printIndent();
        std::cout << "UnaryExpression: " << unaryOpToString(node.op) << "\n";
        indent++;
        node.operand->accept(*this);
        indent--;
    }

    void visit(const AssignmentExpression& node) override {
        printIndent();
        std::cout << "AssignmentExpression: " << assignOpToString(node.op) << "\n";
        indent++;
        node.target->accept(*this);
        node.value->accept(*this);
        indent--;
    }

    void visit(const TernaryExpression& node) override {
        printIndent();
        std::cout << "TernaryExpression\n";
    }

    void visit(const CallExpression& node) override {
        printIndent();
        std::cout << "CallExpression\n";
        indent++;
        printIndent();
        std::cout << "Callee:\n";
        indent++;
        node.callee->accept(*this);
        indent--;
        if (!node.arguments.empty()) {
            printIndent();
            std::cout << "Arguments:\n";
            indent++;
            for (const auto& arg : node.arguments) {
                arg->accept(*this);
            }
            indent--;
        }
        indent--;
    }

    void visit(const NewExpression& node) override {
        printIndent();
        std::cout << "NewExpression\n";
    }

    void visit(const IndexExpression& node) override {
        printIndent();
        std::cout << "IndexExpression\n";
    }

    void visit(const CastExpression& node) override {
        printIndent();
        std::cout << "CastExpression\n";
    }

    void visit(const SizeOfExpression& node) override {
        printIndent();
        std::cout << "SizeOfExpression\n";
    }

    void visit(const AlignOfExpression& node) override {
        printIndent();
        std::cout << "AlignOfExpression\n";
    }

    void visit(const PipeExpression& node) override {
        printIndent();
        std::cout << "PipeExpression\n";
    }

    void visit(const MatchExpression& node) override {
        printIndent();
        std::cout << "MatchExpression\n";
    }

    void visit(const TupleExpression& node) override {
        printIndent();
        std::cout << "TupleExpression\n";
    }

    void visit(const LambdaExpression& node) override {
        printIndent();
        std::cout << "LambdaExpression\n";
    }

    // Patterns
    void visit(const LiteralPattern& node) override {
        printIndent();
        std::cout << "LiteralPattern\n";
    }

    void visit(const RangePattern& node) override {
        printIndent();
        std::cout << "RangePattern: " << node.low << ".." << node.high << "\n";
    }

    void visit(const WildcardPattern& node) override {
        printIndent();
        std::cout << "WildcardPattern\n";
    }

    void visit(const BindingPattern& node) override {
        printIndent();
        std::cout << "BindingPattern: " << node.name << "\n";
    }

    void visit(const TuplePattern& node) override {
        printIndent();
        std::cout << "TuplePattern\n";
    }

    void visit(const GuardedPattern& node) override {
        printIndent();
        std::cout << "GuardedPattern\n";
    }
};

//================================================================================
// Build a simple AST: module with a function
//================================================================================
NodePtr<ProgramNode> buildSimpleAST() {
    SourceLocation loc("example.mingus", 1, 1);

    // Create a simple function: int add(int a, int b) { return a + b; }
    
    // Parameters
    auto paramA = std::make_shared<ParameterNode>(
        "a",
        std::make_shared<PrimitiveTypeNode>(PrimitiveType::PrimitiveKind::Int, loc),
        nullptr,
        /*isRef=*/false,
        loc
    );
    auto paramB = std::make_shared<ParameterNode>(
        "b",
        std::make_shared<PrimitiveTypeNode>(PrimitiveType::PrimitiveKind::Int, loc),
        nullptr,
        /*isRef=*/false,
        loc
    );
    NodeList<ParameterNode> params = { paramA, paramB };

    // Return type
    auto returnType = std::make_shared<PrimitiveTypeNode>(
        PrimitiveType::PrimitiveKind::Int, loc
    );

    // Body: return a + b
    auto left = std::make_shared<IdentifierExpression>("a", loc);
    auto right = std::make_shared<IdentifierExpression>("b", loc);
    auto addExpr = std::make_shared<BinaryExpression>(
        left, BinaryOp::Add, right, loc
    );
    auto returnStmt = std::make_shared<ReturnStatement>(addExpr, loc);
    
    NodeList<StatementNode> bodyStmts = { returnStmt };
    auto body = std::make_shared<BlockStatement>(bodyStmts, loc);

    // Function declaration
    auto funcDecl = std::make_shared<FunctionDeclaration>(
        "add",
        AccessModifier::Public,
        false,  // isStatic
        false,  // isAbstract
        params,
        returnType,
        body,
        loc
    );

    // Module
    NodeList<ImportNode> imports;
    NodeList<DeclarationNode> declarations = { funcDecl };
    auto module = std::make_shared<ModuleNode>("ExampleModule", imports, declarations, loc);

    // Program
    NodeList<ModuleNode> modules = { module };
    return std::make_shared<ProgramNode>(modules, loc);
}

//================================================================================
// Main
//================================================================================
int main() {
    std::cout << "========================================\n";
    std::cout << "Mingus AST - Simple Example\n";
    std::cout << "========================================\n\n";

    auto program = buildSimpleAST();

    PrintVisitor printer;
    program->accept(printer);

    std::cout << "\n========================================\n";
    std::cout << "AST construction successful!\n";
    std::cout << "========================================\n";

    return 0;
}
