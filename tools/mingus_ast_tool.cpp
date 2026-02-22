//================================================================================
// MINGUS v1 - AST Tool
// Command-line utility to parse Mingus source files and print the AST
//
// Usage: mingus_ast_tool <source_file.mingus>
//================================================================================

#include "mingus/parser/ASTGenerator.h"
#include "MingusLexer.h"
#include "MingusParser.h"

#include <antlr4-runtime.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace mingus;
using namespace mingus::ast;
using namespace mingus::parser;

//================================================================================
// Error listener to capture parse errors
//================================================================================
class VerboseErrorListener : public antlr4::BaseErrorListener {
public:
    bool hasErrors = false;
    std::vector<std::string> errorMessages;
    
    void syntaxError(antlr4::Recognizer* recognizer, antlr4::Token* offendingSymbol,
                     size_t line, size_t charPositionInLine,
                     const std::string& msg, std::exception_ptr e) override {
        hasErrors = true;
        std::ostringstream oss;
        oss << "Line " << line << ":" << charPositionInLine << " " << msg;
        errorMessages.push_back(oss.str());
        std::cerr << "ERROR: " << oss.str() << "\n";
    }
};

//================================================================================
// Pretty Print Visitor - Outputs formatted AST
//================================================================================
class PrettyPrintVisitor : public ConstASTVisitor {
    int indent = 0;
    std::ostream& out;
    
    void printIndent() const {
        for (int i = 0; i < indent; ++i) out << "  ";
    }
    
    void printLoc(const SourceLocation& loc) const {
        if (loc.line > 0) {
            out << " [L" << loc.line;
            if (loc.column >= 0) {
                out << ":" << loc.column;
            }
            out << "]";
        }
    }

public:
    explicit PrettyPrintVisitor(std::ostream& os = std::cout) : out(os) {}
    
    //========================================================================
    // Program Structure
    //========================================================================
    void visit(const ProgramNode& node) override {
        printIndent();
        out << "Program";
        printLoc(node.location);
        out << "\n";
        indent++;
        for (const auto& m : node.modules) m->accept(*this);
        indent--;
    }

    void visit(const ModuleNode& node) override {
        printIndent();
        out << "Module: " << node.name;
        printLoc(node.location);
        out << "\n";
        indent++;
        for (const auto& i : node.imports) i->accept(*this);
        for (const auto& d : node.declarations) d->accept(*this);
        indent--;
    }

    void visit(const ImportNode& node) override {
        printIndent();
        out << "Import";
        printLoc(node.location);
        out << ": ";
        
        if (node.isFromImport()) {
            out << "from " << node.source.toString() << " import ";
            for (size_t i = 0; i < node.targets.size(); ++i) {
                if (i > 0) out << ", ";
                out << node.targets[i].name;
                if (node.targets[i].alias) {
                    out << " as " << *node.targets[i].alias;
                }
            }
        } else if (node.isModuleImport()) {
            out << node.source.toString();
        } else {
            // Direct import of targets
            for (size_t i = 0; i < node.targets.size(); ++i) {
                if (i > 0) out << ", ";
                out << node.targets[i].name;
            }
        }
        out << "\n";
    }

    //========================================================================
    // Declarations
    //========================================================================
    void visit(const FunctionDeclaration& node) override {
        printIndent();
        out << "Function: " << node.name;
        if (node.isStatic) out << " [static]";
        if (node.isAbstract) out << " [abstract]";
        printLoc(node.location);
        out << "\n";
        indent++;
        
        if (!node.parameters.empty()) {
            printIndent();
            out << "Parameters:\n";
            indent++;
            for (const auto& p : node.parameters) p->accept(*this);
            indent--;
        }
        
        if (node.returnType) {
            printIndent();
            out << "Returns:\n";
            indent++;
            node.returnType->accept(*this);
            indent--;
        }
        
        if (node.body) {
            printIndent();
            out << "Body:\n";
            indent++;
            node.body->accept(*this);
            indent--;
        }
        indent--;
    }

    void visit(const ParameterNode& node) override {
        printIndent();
        out << "Param: " << node.name;
        if (node.type) {
            out << ":\n";
            indent++;
            node.type->accept(*this);
            indent--;
        } else {
            out << "\n";
        }
        if (node.defaultValue) {
            printIndent();
            out << "Default:\n";
            indent++;
            node.defaultValue->accept(*this);
            indent--;
        }
    }

    void visit(const VariableDeclaration& node) override {
        printIndent();
        out << "Var: " << node.name;
        if (node.isInferred) out << " [inferred]";
        if (node.isStatic) out << " [static]";
        // Note: mutability is part of the type, not a separate flag
        printLoc(node.location);
        out << "\n";
        indent++;
        if (node.type) {
            printIndent();
            out << "Type:\n";
            indent++;
            node.type->accept(*this);
            indent--;
        }
        if (node.initializer) {
            printIndent();
            out << "Initializer:\n";
            indent++;
            node.initializer->accept(*this);
            indent--;
        }
        indent--;
    }

    void visit(const ClassDeclaration& node) override {
        printIndent();
        out << "Class: " << node.name;
        if (node.isAbstract) out << " [abstract]";
        if (node.isStatic) out << " [static]";
        if (!node.baseClasses.empty()) {
            out << " extends ";
            for (size_t i = 0; i < node.baseClasses.size(); ++i) {
                if (i > 0) out << ", ";
                out << node.baseClasses[i].toString();
            }
        }
        printLoc(node.location);
        out << "\n";
        indent++;
        for (const auto& f : node.fields) f->accept(*this);
        for (const auto& ctor : node.constructors) {
            if (ctor) ctor->accept(*this);
        }
        if (node.destructor) {
            node.destructor->accept(*this);
        }
        for (const auto& op : node.operators) op->accept(*this);
        for (const auto& m : node.methods) m->accept(*this);
        indent--;
    }

    void visit(const ConstructorDeclaration& node) override {
        printIndent();
        out << "Constructor";
        // Note: default constructor detection is semantic analysis
        printLoc(node.location);
        out << "\n";
        indent++;
        if (!node.parameters.empty()) {
            printIndent();
            out << "Parameters:\n";
            indent++;
            for (const auto& p : node.parameters) p->accept(*this);
            indent--;
        }
        if (node.body) {
            printIndent();
            out << "Body:\n";
            indent++;
            node.body->accept(*this);
            indent--;
        }
        indent--;
    }

    void visit(const DestructorDeclaration& node) override {
        printIndent();
        out << "Destructor";
        printLoc(node.location);
        out << "\n";
        indent++;
        if (node.body) {
            printIndent();
            out << "Body:\n";
            indent++;
            node.body->accept(*this);
            indent--;
        }
        indent--;
    }

    void visit(const OperatorDeclaration& node) override {
        printIndent();
        out << "Operator: " << operatorKindToString(node.op);
        printLoc(node.location);
        out << "\n";
        indent++;
        if (!node.parameters.empty()) {
            printIndent();
            out << "Parameters:\n";
            indent++;
            for (const auto& p : node.parameters) p->accept(*this);
            indent--;
        }
        if (node.returnType) {
            printIndent();
            out << "Returns:\n";
            indent++;
            node.returnType->accept(*this);
            indent--;
        }
        if (node.body) {
            printIndent();
            out << "Body:\n";
            indent++;
            node.body->accept(*this);
            indent--;
        }
        indent--;
    }

    void visit(const ExternFunctionDeclaration& node) override {
        printIndent();
        out << "Extern: " << node.name;
        printLoc(node.location);
        out << "\n";
        indent++;
        if (!node.parameters.empty()) {
            printIndent();
            out << "Parameters:\n";
            indent++;
            for (const auto& p : node.parameters) {
                if (p) p->accept(*this);
            }
            indent--;
        }
        if (node.returnType) {
            printIndent();
            out << "Returns:\n";
            indent++;
            node.returnType->accept(*this);
            indent--;
        }
        indent--;
    }

    void visit(const StructDeclaration& node) override {
        printIndent();
        out << "Struct: " << node.name;
        printLoc(node.location);
        out << "\n";
        indent++;
        for (const auto& f : node.fields) f->accept(*this);
        for (const auto& op : node.operators) op->accept(*this);
        for (const auto& m : node.methods) m->accept(*this);
        indent--;
    }

    void visit(const EnumDeclaration& node) override {
        printIndent();
        out << "Enum: " << node.name;
        printLoc(node.location);
        out << "\n";
        indent++;
        for (const auto& m : node.members) m->accept(*this);
        indent--;
    }

    void visit(const EnumMemberNode& node) override {
        printIndent();
        out << "Member: " << node.name;
        if (node.value) {
            out << " = ";
            indent++;
            node.value->accept(*this);
            indent--;
        } else {
            out << "\n";
        }
    }

    void visit(const TupleDestructuringDeclaration& node) override {
        printIndent();
        out << "Tuple Destructuring";
        printLoc(node.location);
        out << " [" << node.elements.size() << " elements]\n";
        indent++;
        for (const auto& elem : node.elements) {
            printIndent();
            out << "Element: " << elem.name;
            if (elem.isInferred) out << " [inferred]";
            out << "\n";
            if (elem.type) {
                printIndent();
                out << "Type:\n";
                indent++;
                elem.type->accept(*this);
                indent--;
            }
        }
        if (node.initializer) {
            printIndent();
            out << "Initializer:\n";
            indent++;
            node.initializer->accept(*this);
            indent--;
        }
        indent--;
    }

    //========================================================================
    // Statements
    //========================================================================
    void visit(const BlockStatement& node) override {
        printIndent();
        out << "Block";
        printLoc(node.location);
        out << "\n";
        indent++;
        for (const auto& s : node.statements) s->accept(*this);
        indent--;
    }

    void visit(const ExpressionStatement& node) override {
        printIndent();
        out << "ExprStmt";
        printLoc(node.location);
        out << "\n";
        indent++;
        if (node.expression) node.expression->accept(*this);
        indent--;
    }

    void visit(const ReturnStatement& node) override {
        printIndent();
        out << "Return";
        printLoc(node.location);
        out << "\n";
        if (node.value) {
            indent++;
            node.value->accept(*this);
            indent--;
        }
    }

    void visit(const IfStatement& node) override {
        printIndent();
        out << "If";
        printLoc(node.location);
        out << "\n";
        indent++;
        printIndent();
        out << "Condition:\n";
        indent++;
        node.condition->accept(*this);
        indent--;
        printIndent();
        out << "Then:\n";
        indent++;
        node.thenBody->accept(*this);
        indent--;
        if (node.elseBody) {
            printIndent();
            out << "Else:\n";
            indent++;
            node.elseBody->accept(*this);
            indent--;
        }
        indent--;
    }

    void visit(const ForStatement& node) override {
        printIndent();
        out << "For";
        printLoc(node.location);
        out << "\n";
        indent++;
        
        // Print initializer
        if (node.initDeclaration) {
            printIndent();
            out << "Init:\n";
            indent++;
            node.initDeclaration->accept(*this);
            indent--;
        } else if (!node.initExpressions.empty()) {
            printIndent();
            out << "Init:\n";
            indent++;
            for (const auto& e : node.initExpressions) {
                if (e) e->accept(*this);
            }
            indent--;
        }
        
        // Print condition
        if (node.condition) {
            printIndent();
            out << "Condition:\n";
            indent++;
            node.condition->accept(*this);
            indent--;
        }
        
        // Print iterator
        if (!node.iterators.empty()) {
            printIndent();
            out << "Iterator:\n";
            indent++;
            for (const auto& e : node.iterators) {
                if (e) e->accept(*this);
            }
            indent--;
        }
        
        // Print body
        if (node.body) {
            printIndent();
            out << "Body:\n";
            indent++;
            node.body->accept(*this);
            indent--;
        }
        indent--;
    }

    void visit(const WhileStatement& node) override {
        printIndent();
        out << "While";
        printLoc(node.location);
        out << "\n";
        indent++;
        if (node.body) node.body->accept(*this);
        indent--;
    }

    void visit(const BreakStatement& node) override {
        printIndent();
        out << "Break";
        printLoc(node.location);
        out << "\n";
    }

    void visit(const ContinueStatement& node) override {
        printIndent();
        out << "Continue";
        printLoc(node.location);
        out << "\n";
    }

    void visit(const DeleteStatement& node) override {
        printIndent();
        out << "Delete";
        printLoc(node.location);
        out << "\n";
        indent++;
        if (node.target) node.target->accept(*this);
        indent--;
    }

    void visit(const SwitchStatement& node) override {
        printIndent();
        out << "Switch";
        printLoc(node.location);
        out << "\n";
    }

    void visit(const RawBlock& node) override {
        printIndent();
        out << "Raw";
        printLoc(node.location);
        out << "\n";
        indent++;
        if (node.body) node.body->accept(*this);
        indent--;
    }

    //========================================================================
    // Expressions
    //========================================================================
    void visit(const IntegerLiteral& node) override {
        printIndent();
        out << "Int: " << node.value;
        printLoc(node.location);
        out << "\n";
    }

    void visit(const FloatLiteral& node) override {
        printIndent();
        out << "Float: " << node.value;
        printLoc(node.location);
        out << "\n";
    }

    void visit(const BoolLiteral& node) override {
        printIndent();
        out << "Bool: " << (node.value ? "true" : "false");
        printLoc(node.location);
        out << "\n";
    }

    void visit(const CharLiteral& node) override {
        printIndent();
        out << "Char: '" << node.value << "'";
        printLoc(node.location);
        out << "\n";
    }

    void visit(const StringLiteral& node) override {
        printIndent();
        out << "String: \"" << node.value << "\"";
        printLoc(node.location);
        out << "\n";
    }

    void visit(const NullLiteral& node) override {
        printIndent();
        out << "Null";
        printLoc(node.location);
        out << "\n";
    }

    void visit(const IdentifierExpression& node) override {
        printIndent();
        out << "Id: " << node.name;
        printLoc(node.location);
        out << "\n";
    }

    void visit(const QualifiedNameExpression& node) override {
        printIndent();
        out << "QualifiedName: " << node.getName();
        printLoc(node.location);
        out << "\n";
    }

    void visit(const BinaryExpression& node) override {
        printIndent();
        out << "Binary: " << binaryOpToString(node.op);
        printLoc(node.location);
        out << "\n";
        indent++;
        node.left->accept(*this);
        node.right->accept(*this);
        indent--;
    }

    void visit(const UnaryExpression& node) override {
        printIndent();
        out << "Unary: " << unaryOpToString(node.op);
        printLoc(node.location);
        out << "\n";
        indent++;
        node.operand->accept(*this);
        indent--;
    }

    void visit(const AssignmentExpression& node) override {
        printIndent();
        out << "Assign: " << assignOpToString(node.op);
        printLoc(node.location);
        out << "\n";
        indent++;
        node.target->accept(*this);
        node.value->accept(*this);
        indent--;
    }

    void visit(const CallExpression& node) override {
        printIndent();
        out << "Call";
        printLoc(node.location);
        out << "\n";
        indent++;
        node.callee->accept(*this);
        if (!node.arguments.empty()) {
            printIndent();
            out << "Arguments:\n";
            indent++;
            for (const auto& a : node.arguments) {
                a->accept(*this);
            }
            indent--;
        }
        indent--;
    }

    void visit(const MemberAccessExpression& node) override {
        printIndent();
        out << "MemberAccess: " << (node.isArrow ? "->" : ".") << node.memberName;
        printLoc(node.location);
        out << "\n";
        indent++;
        node.object->accept(*this);
        indent--;
    }

    void visit(const IndexExpression& node) override {
        printIndent();
        out << "Index";
        printLoc(node.location);
        out << "\n";
        indent++;
        node.object->accept(*this);
        node.index->accept(*this);
        indent--;
    }

    void visit(const NewExpression& node) override {
        printIndent();
        out << "New" << (node.isArray ? "[]" : "()");
        printLoc(node.location);
        out << "\n";
        indent++;
        if (node.type) {
            printIndent();
            out << "Type:\n";
            indent++;
            node.type->accept(*this);
            indent--;
        }
        if (node.arraySize) {
            printIndent();
            out << "Size:\n";
            indent++;
            node.arraySize->accept(*this);
            indent--;
        }
        if (!node.arguments.empty()) {
            printIndent();
            out << "Args:\n";
            indent++;
            for (const auto& a : node.arguments) {
                a->accept(*this);
            }
            indent--;
        }
        indent--;
    }

    void visit(const ThisExpression& node) override {
        printIndent();
        out << "This";
        printLoc(node.location);
        out << "\n";
    }

    void visit(const CastExpression& node) override {
        printIndent();
        out << "Cast";
        printLoc(node.location);
        out << "\n";
        indent++;
        if (node.targetType) {
            printIndent();
            out << "To:\n";
            indent++;
            node.targetType->accept(*this);
            indent--;
        }
        printIndent();
        out << "From:\n";
        indent++;
        node.operand->accept(*this);
        indent--;
        indent--;
    }

    void visit(const TernaryExpression& node) override {
        printIndent();
        out << "Ternary";
        printLoc(node.location);
        out << "\n";
        indent++;
        printIndent();
        out << "Condition:\n";
        indent++;
        node.condition->accept(*this);
        indent--;
        printIndent();
        out << "Then:\n";
        indent++;
        node.thenExpr->accept(*this);
        indent--;
        printIndent();
        out << "Else:\n";
        indent++;
        node.elseExpr->accept(*this);
        indent--;
        indent--;
    }

    void visit(const MatchExpression& node) override {
        printIndent();
        out << "Match";
        printLoc(node.location);
        out << "\n";
        indent++;
        printIndent();
        out << "Subject:\n";
        indent++;
        node.subject->accept(*this);
        indent--;
        for (const auto& arm : node.arms) {
            printIndent();
            out << "Arm:\n";
            indent++;
            arm.pattern->accept(*this);
            if (arm.body) {
                if (arm.body->is<ExpressionNode>()) {
                    arm.body->as<ExpressionNode>()->accept(*this);
                } else if (arm.body->is<BlockStatement>()) {
                    arm.body->as<BlockStatement>()->accept(*this);
                }
            }
            indent--;
        }
        indent--;
    }

    void visit(const LambdaExpression& node) override {
        printIndent();
        out << "Lambda";
        printLoc(node.location);
        out << "\n";
        indent++;
        if (!node.parameters.empty()) {
            printIndent();
            out << "Parameters:\n";
            indent++;
            for (const auto& p : node.parameters) p->accept(*this);
            indent--;
        }
        if (node.body) {
            printIndent();
            out << "Body:\n";
            indent++;
            if (node.body->is<ExpressionNode>()) {
                node.body->as<ExpressionNode>()->accept(*this);
            } else if (node.body->is<BlockStatement>()) {
                node.body->as<BlockStatement>()->accept(*this);
            }
            indent--;
        }
        indent--;
    }

    void visit(const TupleExpression& node) override {
        printIndent();
        out << "Tuple";
        printLoc(node.location);
        out << " [" << node.elements.size() << " elements]\n";
        indent++;
        for (const auto& e : node.elements) {
            e->accept(*this);
        }
        indent--;
    }

    void visit(const PipeExpression& node) override {
        printIndent();
        out << "Pipe";
        printLoc(node.location);
        out << " [" << node.stages.size() << " stages]\n";
        indent++;
        printIndent();
        out << "Input:\n";
        indent++;
        node.input->accept(*this);
        indent--;
        for (size_t i = 0; i < node.stages.size(); ++i) {
            printIndent();
            out << "Stage " << (i + 1) << ":\n";
            indent++;
            if (node.stages[i].function) {
                node.stages[i].function->accept(*this);
            } else {
                printIndent();
                out << "(null function)\n";
            }
            if (!node.stages[i].extraArguments.empty()) {
                printIndent();
                out << "Extra Args:\n";
                indent++;
                for (const auto& a : node.stages[i].extraArguments) {
                    if (a) a->accept(*this);
                }
                indent--;
            }
            indent--;
        }
        indent--;
    }

    void visit(const SizeOfExpression& node) override {
        printIndent();
        out << "SizeOf";
        printLoc(node.location);
        out << "\n";
        indent++;
        if (node.targetType) node.targetType->accept(*this);
        indent--;
    }

    void visit(const AlignOfExpression& node) override {
        printIndent();
        out << "AlignOf";
        printLoc(node.location);
        out << "\n";
        indent++;
        if (node.targetType) node.targetType->accept(*this);
        indent--;
    }

    void visit(const InterpolatedString& node) override {
        printIndent();
        out << "InterpolatedString";
        printLoc(node.location);
        out << " [" << node.parts.size() << " parts]\n";
        indent++;
        for (const auto& part : node.parts) {
            printIndent();
            if (part.kind == InterpolatedPartKind::Text) {
                out << "Text: \"" << part.text << "\"\n";
            } else {
                out << "Expr:\n";
                indent++;
                if (part.expression) part.expression->accept(*this);
                indent--;
            }
        }
        indent--;
    }

    //========================================================================
    // Patterns
    //========================================================================
    void visit(const LiteralPattern& node) override {
        printIndent();
        out << "LiteralPattern";
        printLoc(node.location);
        out << "\n";
        indent++;
        if (node.value) node.value->accept(*this);
        indent--;
    }

    void visit(const RangePattern& node) override {
        printIndent();
        out << "RangePattern: " << node.low << ".." << node.high;
        printLoc(node.location);
        out << "\n";
    }

    void visit(const WildcardPattern& node) override {
        printIndent();
        out << "WildcardPattern";
        printLoc(node.location);
        out << "\n";
    }

    void visit(const BindingPattern& node) override {
        printIndent();
        out << "BindingPattern: " << node.name;
        printLoc(node.location);
        out << "\n";
    }

    void visit(const TuplePattern& node) override {
        printIndent();
        out << "TuplePattern";
        printLoc(node.location);
        out << " [" << node.elements.size() << " elements]\n";
        indent++;
        for (const auto& e : node.elements) {
            e->accept(*this);
        }
        indent--;
    }

    void visit(const GuardedPattern& node) override {
        printIndent();
        out << "GuardedPattern";
        printLoc(node.location);
        out << "\n";
        indent++;
        node.innerPattern->accept(*this);
        if (node.guard) {
            printIndent();
            out << "Guard:\n";
            indent++;
            node.guard->accept(*this);
            indent--;
        }
        indent--;
    }

    //========================================================================
    // Types
    //========================================================================
    void visit(const TypeNode& node) override {
        printIndent();
        out << "Type";
        printLoc(node.location);
        out << "\n";
    }

    void visit(const PrimitiveTypeNode& node) override {
        printIndent();
        out << "Primitive: " << PrimitiveType(node.kind).toString();
        printLoc(node.location);
        out << "\n";
    }

    void visit(const NamedTypeNode& node) override {
        printIndent();
        out << "Named: " << node.getName();
        printLoc(node.location);
        out << "\n";
    }

    void visit(const PointerTypeNode& node) override {
        printIndent();
        out << "Pointer";
        printLoc(node.location);
        out << "\n";
        indent++;
        if (node.baseType) node.baseType->accept(*this);
        indent--;
    }

    void visit(const ArrayTypeNode& node) override {
        printIndent();
        out << "Array";
        if (node.isUnsized()) out << " []";
        else out << " [sized]";
        printLoc(node.location);
        out << "\n";
        indent++;
        if (node.elementType) node.elementType->accept(*this);
        if (node.size) {
            printIndent();
            out << "Size:\n";
            indent++;
            node.size->accept(*this);
            indent--;
        }
        indent--;
    }

    void visit(const TupleTypeNode& node) override {
        printIndent();
        out << "TupleType";
        out << " [" << node.elementTypes.size() << " element(s)]";
        printLoc(node.location);
        out << "\n";
        indent++;
        for (const auto& t : node.elementTypes) {
            if (t) t->accept(*this);
        }
        indent--;
    }

    void visit(const FunctionTypeNode& node) override {
        printIndent();
        out << "FunctionType";
        out << " [" << node.parameterTypes.size() << " param(s)]";
        printLoc(node.location);
        out << "\n";
        indent++;
        if (!node.parameterTypes.empty()) {
            printIndent();
            out << "Params:\n";
            indent++;
            for (const auto& t : node.parameterTypes) {
                if (t) t->accept(*this);
            }
            indent--;
        }
        if (node.returnType) {
            printIndent();
            out << "Returns:\n";
            indent++;
            node.returnType->accept(*this);
            indent--;
        }
        indent--;
    }
};

//================================================================================
// File reading utility
//================================================================================
std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

//================================================================================
// Parse file and generate AST
//================================================================================
std::shared_ptr<ProgramNode> parseFile(const std::string& filename) {
    // Read file
    std::string source = readFile(filename);
    
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
        std::cerr << "\nParse failed with " << parserErrorListener.errorMessages.size() << " error(s).\n";
        return nullptr;
    }
    
    // Generate AST
    ASTGenerator generator;
    auto ast = generator.generate(tree);
    
    if (generator.hasErrors()) {
        std::cerr << "\nAST Generation errors:\n";
        for (const auto& err : generator.errors) {
            std::cerr << "  Line " << err.location.line << ":" << err.location.column 
                      << " " << err.message << "\n";
        }
    }
    
    return ast;
}

//================================================================================
// Main
//================================================================================
int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << "Mingus AST Tool\n";
    std::cout << "========================================\n\n";
    
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file.mingus>\n";
        std::cerr << "\nExample:\n";
        std::cerr << "  " << argv[0] << " test.mingus\n";
        return 1;
    }
    
    std::string filename = argv[1];
    std::cout << "Parsing: " << filename << "\n\n";
    
    try {
        auto program = parseFile(filename);
        
        if (program) {
            std::cout << "----------------------------------------\n";
            std::cout << "AST Output:\n";
            std::cout << "----------------------------------------\n\n";
            
            PrettyPrintVisitor printer;
            program->accept(printer);
            
            std::cout << "\n----------------------------------------\n";
            std::cout << "Parse completed successfully!\n";
            std::cout << "----------------------------------------\n";
            return 0;
        } else {
            std::cerr << "\nFailed to parse file.\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
