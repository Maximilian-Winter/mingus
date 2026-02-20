//================================================================================
// MINGUS v1 - Factorial Function AST Example
// Demonstrates building a more complex AST with control flow
//================================================================================

#include "mingus/AST.h"
#include <iostream>
#include <memory>

using namespace mingus::ast;

//================================================================================
// Build AST for recursive factorial function:
// 
// module Example;
// 
// public fn factorial(n: int) -> int {
//     if (n <= 1) {
//         return 1;
//     } else {
//         return n * factorial(n - 1);
//     }
// }
//================================================================================
NodePtr<ProgramNode> buildFactorialAST() {
    SourceLocation loc("factorial.mingus", 1, 1);

    // Parameter: n: int
    auto paramN = std::make_shared<ParameterNode>(
        "n",
        std::make_shared<PrimitiveTypeNode>(PrimitiveType::PrimitiveKind::Int, loc),
        nullptr,
        /*isRef=*/false,
        loc
    );
    NodeList<ParameterNode> params = { paramN };

    // Return type: int
    auto returnType = std::make_shared<PrimitiveTypeNode>(
        PrimitiveType::PrimitiveKind::Int, loc
    );

    // Build condition: n <= 1
    auto nIdent1 = std::make_shared<IdentifierExpression>("n", loc);
    auto oneLit = std::make_shared<IntegerLiteral>(1, loc);
    auto condition = std::make_shared<BinaryExpression>(
        nIdent1, BinaryOp::LessEqual, oneLit, loc
    );

    // Then branch: return 1
    auto returnOne = std::make_shared<ReturnStatement>(
        std::make_shared<IntegerLiteral>(1, loc), loc
    );
    NodeList<StatementNode> thenStmts = { returnOne };
    auto thenBody = std::make_shared<BlockStatement>(thenStmts, loc);

    // Else branch: return n * factorial(n - 1)
    // n - 1
    auto nIdent2 = std::make_shared<IdentifierExpression>("n", loc);
    auto oneLit2 = std::make_shared<IntegerLiteral>(1, loc);
    auto minusExpr = std::make_shared<BinaryExpression>(
        nIdent2, BinaryOp::Sub, oneLit2, loc
    );

    // factorial(n - 1)
    auto factorialIdent = std::make_shared<IdentifierExpression>("factorial", loc);
    NodeList<ExpressionNode> callArgs = { minusExpr };
    auto recursiveCall = std::make_shared<CallExpression>(
        factorialIdent, callArgs, loc
    );

    // n * factorial(n - 1)
    auto nIdent3 = std::make_shared<IdentifierExpression>("n", loc);
    auto multExpr = std::make_shared<BinaryExpression>(
        nIdent3, BinaryOp::Mul, recursiveCall, loc
    );

    // return n * factorial(n - 1)
    auto returnRecursive = std::make_shared<ReturnStatement>(multExpr, loc);
    NodeList<StatementNode> elseStmts = { returnRecursive };
    auto elseBody = std::make_shared<BlockStatement>(elseStmts, loc);

    // If statement
    auto ifStmt = std::make_shared<IfStatement>(
        condition, thenBody, std::vector<ElseIfClause>{}, elseBody, loc
    );

    // Function body
    NodeList<StatementNode> bodyStmts = { ifStmt };
    auto body = std::make_shared<BlockStatement>(bodyStmts, loc);

    // Function declaration: public fn factorial(n: int) -> int
    auto funcDecl = std::make_shared<FunctionDeclaration>(
        "factorial",
        AccessModifier::Public,
        false,  // isStatic
        false,  // isAbstract
        params,
        returnType,
        body,
        loc
    );

    // Module: Example
    NodeList<ImportNode> imports;
    NodeList<DeclarationNode> declarations = { funcDecl };
    auto module = std::make_shared<ModuleNode>(
        "Example", imports, declarations, loc
    );

    // Program
    NodeList<ModuleNode> modules = { module };
    return std::make_shared<ProgramNode>(modules, loc);
}

//================================================================================
// Simple Print Visitor (minimal version)
//================================================================================
class PrintVisitor : public ConstASTVisitor {
    int indent = 0;
    
    void printIndent() const {
        for (int i = 0; i < indent; ++i) std::cout << "  ";
    }

public:
    void visit(const ProgramNode& node) override {
        printIndent();
        std::cout << "Program\n";
        indent++;
        for (const auto& m : node.modules) m->accept(*this);
        indent--;
    }

    void visit(const ModuleNode& node) override {
        printIndent();
        std::cout << "Module: " << node.name << "\n";
        indent++;
        for (const auto& d : node.declarations) d->accept(*this);
        indent--;
    }

    void visit(const FunctionDeclaration& node) override {
        printIndent();
        std::cout << "Function: " << node.name << " (";
        for (size_t i = 0; i < node.parameters.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << node.parameters[i]->name;
        }
        std::cout << ")\n";
        indent++;
        if (node.body) node.body->accept(*this);
        indent--;
    }

    void visit(const BlockStatement& node) override {
        printIndent();
        std::cout << "Block\n";
        indent++;
        for (const auto& s : node.statements) s->accept(*this);
        indent--;
    }

    void visit(const IfStatement& node) override {
        printIndent();
        std::cout << "If\n";
        indent++;
        printIndent(); std::cout << "Condition:\n";
        indent++; node.condition->accept(*this); indent--;
        printIndent(); std::cout << "Then:\n";
        indent++; node.thenBody->accept(*this); indent--;
        if (node.elseBody) {
            printIndent(); std::cout << "Else:\n";
            indent++; node.elseBody->accept(*this); indent--;
        }
        indent--;
    }

    void visit(const ReturnStatement& node) override {
        printIndent();
        std::cout << "Return\n";
        if (node.value) {
            indent++;
            node.value->accept(*this);
            indent--;
        }
    }

    void visit(const BinaryExpression& node) override {
        printIndent();
        std::cout << "Binary: " << binaryOpToString(node.op) << "\n";
        indent++;
        node.left->accept(*this);
        node.right->accept(*this);
        indent--;
    }

    void visit(const CallExpression& node) override {
        printIndent();
        std::cout << "Call\n";
        indent++;
        node.callee->accept(*this);
        for (const auto& a : node.arguments) {
            a->accept(*this);
        }
        indent--;
    }

    void visit(const IdentifierExpression& node) override {
        printIndent();
        std::cout << "Identifier: " << node.name << "\n";
    }

    void visit(const IntegerLiteral& node) override {
        printIndent();
        std::cout << "Int: " << node.value << "\n";
    }

    void visit(const ParameterNode& node) override {}
    void visit(const PrimitiveTypeNode& node) override {}

    // Default empty implementations for other nodes
    void visit(const ImportNode&) override {}
    void visit(const TypeNode&) override {}
    void visit(const NamedTypeNode&) override {}
    void visit(const PointerTypeNode&) override {}
    void visit(const ArrayTypeNode&) override {}
    void visit(const TupleTypeNode&) override {}
    void visit(const FunctionTypeNode&) override {}
    void visit(const VariableDeclaration&) override {}
    void visit(const TupleDestructuringDeclaration&) override {}
    void visit(const ConstructorDeclaration&) override {}
    void visit(const DestructorDeclaration&) override {}
    void visit(const OperatorDeclaration&) override {}
    void visit(const ExternFunctionDeclaration&) override {}
    void visit(const EnumMemberNode&) override {}
    void visit(const EnumDeclaration&) override {}
    void visit(const StructDeclaration&) override {}
    void visit(const ClassDeclaration&) override {}
    void visit(const ExpressionStatement&) override {}
    void visit(const SwitchStatement&) override {}
    void visit(const ForStatement&) override {}
    void visit(const WhileStatement&) override {}
    void visit(const BreakStatement&) override {}
    void visit(const ContinueStatement&) override {}
    void visit(const DeleteStatement&) override {}
    void visit(const RawBlock&) override {}
    void visit(const FloatLiteral&) override {}
    void visit(const BoolLiteral&) override {}
    void visit(const CharLiteral&) override {}
    void visit(const StringLiteral&) override {}
    void visit(const InterpolatedString&) override {}
    void visit(const NullLiteral&) override {}
    void visit(const QualifiedNameExpression&) override {}
    void visit(const MemberAccessExpression&) override {}
    void visit(const ThisExpression&) override {}
    void visit(const UnaryExpression&) override {}
    void visit(const AssignmentExpression&) override {}
    void visit(const TernaryExpression&) override {}
    void visit(const NewExpression&) override {}
    void visit(const IndexExpression&) override {}
    void visit(const CastExpression&) override {}
    void visit(const SizeOfExpression&) override {}
    void visit(const AlignOfExpression&) override {}
    void visit(const PipeExpression&) override {}
    void visit(const MatchExpression&) override {}
    void visit(const TupleExpression&) override {}
    void visit(const LambdaExpression&) override {}
    void visit(const LiteralPattern&) override {}
    void visit(const RangePattern&) override {}
    void visit(const WildcardPattern&) override {}
    void visit(const BindingPattern&) override {}
    void visit(const TuplePattern&) override {}
    void visit(const GuardedPattern&) override {}
};

//================================================================================
// Main
//================================================================================
int main() {
    std::cout << "========================================\n";
    std::cout << "Mingus AST - Factorial Example\n";
    std::cout << "========================================\n\n";

    std::cout << "Building AST for recursive factorial function:\n";
    std::cout << "  public fn factorial(n: int) -> int {\n";
    std::cout << "      if (n <= 1) { return 1; }\n";
    std::cout << "      else { return n * factorial(n - 1); }\n";
    std::cout << "  }\n\n";

    auto program = buildFactorialAST();

    std::cout << "AST Structure:\n";
    std::cout << "-------------\n";

    PrintVisitor printer;
    program->accept(printer);

    std::cout << "\n========================================\n";
    std::cout << "Factorial AST construction successful!\n";
    std::cout << "========================================\n";

    return 0;
}
