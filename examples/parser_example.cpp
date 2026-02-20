//================================================================================
// MINGUS v1 - Parser Example
// Demonstrates parsing Mingus source code and generating an AST
//================================================================================

#include "mingus/parser/ASTGenerator.h"
#include "MingusLexer.h"
#include "MingusParser.h"

#include <antlr4-runtime.h>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace mingus;
using namespace mingus::ast;
using namespace mingus::parser;

//================================================================================
// Simple Print Visitor for demonstration
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
        for (const auto& i : node.imports) i->accept(*this);
        for (const auto& d : node.declarations) d->accept(*this);
        indent--;
    }

    void visit(const ImportNode& node) override {
        printIndent();
        std::cout << "Import: ";
        for (const auto& t : node.targets) {
            std::cout << t.name;
            if (t.alias) std::cout << " as " << *t.alias;
            std::cout << " ";
        }
        if (!node.source.empty()) {
            std::cout << "from " << node.source.toString();
        }
        std::cout << "\n";
    }

    void visit(const FunctionDeclaration& node) override {
        printIndent();
        std::cout << "Function: " << node.name;
        std::cout << " (";
        for (size_t i = 0; i < node.parameters.size(); ++i) {
            if (i > 0) std::cout << ", ";
            node.parameters[i]->accept(*this);
        }
        std::cout << ")";
        if (node.isAbstract) std::cout << " [abstract]";
        std::cout << "\n";
        indent++;
        if (node.body) node.body->accept(*this);
        indent--;
    }

    void visit(const ParameterNode& node) override {
        std::cout << node.name << ": ";
        if (node.type) {
            // Would print type here
        }
    }

    void visit(const VariableDeclaration& node) override {
        printIndent();
        std::cout << "Var: " << node.name;
        if (node.isInferred) std::cout << " (inferred)";
        std::cout << "\n";
        indent++;
        if (node.initializer) node.initializer->accept(*this);
        indent--;
    }

    void visit(const ClassDeclaration& node) override {
        printIndent();
        std::cout << "Class: " << node.name;
        if (node.isAbstract) std::cout << " [abstract]";
        if (node.isStatic) std::cout << " [static]";
        std::cout << "\n";
        indent++;
        for (const auto& f : node.fields) f->accept(*this);
        for (const auto& m : node.methods) m->accept(*this);
        indent--;
    }

    void visit(const StructDeclaration& node) override {
        printIndent();
        std::cout << "Struct: " << node.name << "\n";
        indent++;
        for (const auto& f : node.fields) f->accept(*this);
        for (const auto& m : node.methods) m->accept(*this);
        indent--;
    }

    void visit(const EnumDeclaration& node) override {
        printIndent();
        std::cout << "Enum: " << node.name << "\n";
        indent++;
        for (const auto& m : node.members) m->accept(*this);
        indent--;
    }

    void visit(const EnumMemberNode& node) override {
        printIndent();
        std::cout << "Member: " << node.name;
        if (node.value) {
            std::cout << " = ";
            // Would print value
        }
        std::cout << "\n";
    }

    void visit(const BlockStatement& node) override {
        printIndent();
        std::cout << "Block\n";
        indent++;
        for (const auto& s : node.statements) s->accept(*this);
        indent--;
    }

    void visit(const ExpressionStatement& node) override {
        printIndent();
        std::cout << "ExprStmt\n";
        indent++;
        if (node.expression) node.expression->accept(*this);
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

    void visit(const ForStatement& node) override {
        printIndent();
        std::cout << "For\n";
        indent++;
        if (node.body) node.body->accept(*this);
        indent--;
    }

    void visit(const WhileStatement& node) override {
        printIndent();
        std::cout << "While\n";
        indent++;
        if (node.body) node.body->accept(*this);
        indent--;
    }

    void visit(const BreakStatement& node) override {
        printIndent();
        std::cout << "Break\n";
    }

    void visit(const ContinueStatement& node) override {
        printIndent();
        std::cout << "Continue\n";
    }

    void visit(const DeleteStatement& node) override {
        printIndent();
        std::cout << "Delete\n";
    }

    void visit(const RawBlock& node) override {
        printIndent();
        std::cout << "Raw\n";
        indent++;
        if (node.body) node.body->accept(*this);
        indent--;
    }

    void visit(const IntegerLiteral& node) override {
        printIndent();
        std::cout << "Int: " << node.value << "\n";
    }

    void visit(const FloatLiteral& node) override {
        printIndent();
        std::cout << "Float: " << node.value << "\n";
    }

    void visit(const BoolLiteral& node) override {
        printIndent();
        std::cout << "Bool: " << (node.value ? "true" : "false") << "\n";
    }

    void visit(const StringLiteral& node) override {
        printIndent();
        std::cout << "String: \"" << node.value << "\"\n";
    }

    void visit(const NullLiteral& node) override {
        printIndent();
        std::cout << "Null\n";
    }

    void visit(const IdentifierExpression& node) override {
        printIndent();
        std::cout << "Id: " << node.name << "\n";
    }

    void visit(const BinaryExpression& node) override {
        printIndent();
        std::cout << "Binary: " << binaryOpToString(node.op) << "\n";
        indent++;
        node.left->accept(*this);
        node.right->accept(*this);
        indent--;
    }

    void visit(const UnaryExpression& node) override {
        printIndent();
        std::cout << "Unary: " << unaryOpToString(node.op) << "\n";
        indent++;
        node.operand->accept(*this);
        indent--;
    }

    void visit(const AssignmentExpression& node) override {
        printIndent();
        std::cout << "Assign: " << assignOpToString(node.op) << "\n";
        indent++;
        node.target->accept(*this);
        node.value->accept(*this);
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

    void visit(const MemberAccessExpression& node) override {
        printIndent();
        std::cout << "MemberAccess: " << (node.isArrow ? "->" : ".") << node.memberName << "\n";
        indent++;
        node.object->accept(*this);
        indent--;
    }

    void visit(const IndexExpression& node) override {
        printIndent();
        std::cout << "Index\n";
        indent++;
        node.object->accept(*this);
        node.index->accept(*this);
        indent--;
    }

    void visit(const NewExpression& node) override {
        printIndent();
        std::cout << "New" << (node.isArray ? "[]" : "()") << "\n";
    }

    void visit(const ThisExpression& node) override {
        printIndent();
        std::cout << "This\n";
    }

    void visit(const CastExpression& node) override {
        printIndent();
        std::cout << "Cast\n";
        indent++;
        node.operand->accept(*this);
        indent--;
    }

    void visit(const TernaryExpression& node) override {
        printIndent();
        std::cout << "Ternary\n";
        indent++;
        node.condition->accept(*this);
        node.thenExpr->accept(*this);
        node.elseExpr->accept(*this);
        indent--;
    }

    void visit(const MatchExpression& node) override {
        printIndent();
        std::cout << "Match\n";
        indent++;
        node.subject->accept(*this);
        for (const auto& arm : node.arms) {
            printIndent(); std::cout << "Arm:\n";
            indent++;
            arm.pattern->accept(*this);
            if (arm.body->is<ExpressionNode>()) {
                arm.body->as<ExpressionNode>()->accept(*this);
            } else if (arm.body->is<BlockStatement>()) {
                arm.body->as<BlockStatement>()->accept(*this);
            }
            indent--;
        }
        indent--;
    }

    void visit(const LambdaExpression& node) override {
        printIndent();
        std::cout << "Lambda\n";
        indent++;
        if (node.body->is<ExpressionNode>()) {
            node.body->as<ExpressionNode>()->accept(*this);
        } else if (node.body->is<BlockStatement>()) {
            node.body->as<BlockStatement>()->accept(*this);
        }
        indent--;
    }

    void visit(const TupleExpression& node) override {
        printIndent();
        std::cout << "Tuple\n";
        indent++;
        for (const auto& e : node.elements) {
            e->accept(*this);
        }
        indent--;
    }

    void visit(const PipeExpression& node) override {
        printIndent();
        std::cout << "Pipe\n";
        indent++;
        node.input->accept(*this);
        indent--;
    }

    void visit(const SizeOfExpression& node) override {
        printIndent();
        std::cout << "SizeOf\n";
    }

    void visit(const AlignOfExpression& node) override {
        printIndent();
        std::cout << "AlignOf\n";
    }

    // Patterns
    void visit(const LiteralPattern& node) override {
        printIndent();
        std::cout << "LiteralPattern\n";
        indent++;
        if (node.value) node.value->accept(*this);
        indent--;
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
        indent++;
        for (const auto& e : node.elements) {
            e->accept(*this);
        }
        indent--;
    }

    void visit(const GuardedPattern& node) override {
        printIndent();
        std::cout << "GuardedPattern\n";
        indent++;
        node.innerPattern->accept(*this);
        node.guard->accept(*this);
        indent--;
    }

    // Types - minimal implementations
    void visit(const TypeNode&) override {}
    void visit(const PrimitiveTypeNode&) override {}
    void visit(const NamedTypeNode&) override {}
    void visit(const PointerTypeNode&) override {}
    void visit(const ArrayTypeNode&) override {}
    void visit(const TupleTypeNode&) override {}
    void visit(const FunctionTypeNode&) override {}

    // Declarations - minimal implementations
    void visit(const TupleDestructuringDeclaration&) override {}
    void visit(const ConstructorDeclaration&) override {}
    void visit(const DestructorDeclaration&) override {}
    void visit(const OperatorDeclaration&) override {}
    void visit(const ExternFunctionDeclaration&) override {}

    // Statements - minimal implementations
    void visit(const SwitchStatement&) override {}

    // Expressions - minimal implementations
    void visit(const QualifiedNameExpression&) override {}
    void visit(const CharLiteral&) override {}
    void visit(const InterpolatedString&) override {}
};

//================================================================================
// Parse and generate AST
//================================================================================
// Error listener to capture parse errors
class VerboseErrorListener : public antlr4::BaseErrorListener {
public:
    bool hasErrors = false;
    
    void syntaxError(antlr4::Recognizer* recognizer, antlr4::Token* offendingSymbol,
                     size_t line, size_t charPositionInLine,
                     const std::string& msg, std::exception_ptr e) override {
        hasErrors = true;
        std::cerr << "Line " << line << ":" << charPositionInLine << " " << msg << "\n";
    }
};

std::shared_ptr<ProgramNode> parseSource(const std::string& source, const std::string& filename) {
    try {
        // Create input stream
        antlr4::ANTLRInputStream input(source);
        input.name = filename;
        
        // Create lexer with error listener
        AntlrMingusParser::MingusLexer lexer(&input);
        VerboseErrorListener lexerErrorListener;
        lexer.removeErrorListeners();
        lexer.addErrorListener(&lexerErrorListener);
        
        antlr4::CommonTokenStream tokens(&lexer);
        
        // Create parser with error listener
        AntlrMingusParser::MingusParser parser(&tokens);
        VerboseErrorListener parserErrorListener;
        parser.removeErrorListeners();
        parser.addErrorListener(&parserErrorListener);
        
        // Parse
        auto* tree = parser.program();
        
        if (lexerErrorListener.hasErrors || parserErrorListener.hasErrors) {
            std::cerr << "Parse errors detected.\n";
            return nullptr;
        }
        
        // Generate AST
        ASTGenerator generator;
        auto ast = generator.generate(tree);
        
        // Print status
        std::cout << "AST generation result: " << (ast ? "success" : "failure") << "\n";
        std::cout << "Error count: " << generator.errors.size() << "\n";
        
        // Print any errors
        if (generator.hasErrors()) {
            std::cerr << "AST Generation errors:\n";
            for (const auto& err : generator.errors) {
                std::cerr << "  Line " << err.location.line << ":" << err.location.column 
                          << " " << err.message << "\n";
            }
        }
        
        return ast;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing source: " << e.what() << "\n";
        return nullptr;
    }
}

//================================================================================
// Main
//================================================================================
int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << "Mingus Parser - AST Generator Example\n";
    std::cout << "========================================\n\n";

    // Simple test input
    std::string testSource = R"(
module TestModule {
    import io, utils from std;
    
    public func add(int a, int b) => int {
        return a + b;
    }
    
    public func factorial(int n) => int {
        if (n <= 1) {
            return 1;
        } else {
            return n * factorial(n - 1);
        }
    }
    
    public class Point {
        public int x;
        public int y;
        
        public constructor(int x, int y) {
            this.x = x;
            this.y = y;
        }
        
        public func distance(Point other) => double {
            var dx = this.x - other.x;
            var dy = this.y - other.y;
            return 0.0;  // Simplified
        }
    }
    
    public enum Color {
        Red, Green, Blue
    }
}
)";

    std::cout << "Parsing test source...\n\n";
    
    auto program = parseSource(testSource, "test.mingus");
    
    if (program) {
        std::cout << "AST generated successfully!\n\n";
        
        PrintVisitor printer;
        program->accept(printer);
        
        std::cout << "\n========================================\n";
        std::cout << "Parser example completed successfully!\n";
        std::cout << "========================================\n";
    } else {
        std::cerr << "Failed to generate AST\n";
        return 1;
    }
    
    return 0;
}
