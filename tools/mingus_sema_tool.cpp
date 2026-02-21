//================================================================================
// MINGUS v1 - Semantic Analysis Tool
// Parses a Mingus source file, runs all 4 semantic passes, and dumps
// comprehensive information about the resolved symbols, types, and validation.
//
// Usage: mingus_sema_tool <source_file.mingus>
//================================================================================

#include "mingus/parser/ASTGenerator.h"
#include "mingus/Sema.h"
#include "MingusLexer.h"
#include "MingusParser.h"

#include <antlr4-runtime.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <stdexcept>

using namespace mingus;
using namespace mingus::ast;
using namespace mingus::parser;
using namespace mingus::sema;

//================================================================================
// Error listener to capture parse errors
//================================================================================
class VerboseErrorListener : public antlr4::BaseErrorListener {
public:
    bool hasErrors = false;
    std::vector<std::string> errorMessages;

    void syntaxError(antlr4::Recognizer*, antlr4::Token*,
                     size_t line, size_t charPositionInLine,
                     const std::string& msg, std::exception_ptr) override {
        hasErrors = true;
        std::ostringstream oss;
        oss << "Line " << line << ":" << charPositionInLine << " " << msg;
        errorMessages.push_back(oss.str());
        std::cerr << "ERROR: " << oss.str() << "\n";
    }
};

//================================================================================
// Utility functions
//================================================================================
static std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static std::shared_ptr<ProgramNode> parseFile(const std::string& filename) {
    std::string source = readFile(filename);

    antlr4::ANTLRInputStream input(source);
    input.name = filename;

    AntlrMingusParser::MingusLexer lexer(&input);
    VerboseErrorListener lexerErrors;
    lexer.removeErrorListeners();
    lexer.addErrorListener(&lexerErrors);

    antlr4::CommonTokenStream tokens(&lexer);

    AntlrMingusParser::MingusParser parser(&tokens);
    VerboseErrorListener parserErrors;
    parser.removeErrorListeners();
    parser.addErrorListener(&parserErrors);

    auto* tree = parser.program();

    if (lexerErrors.hasErrors || parserErrors.hasErrors) {
        std::cerr << "\nParse failed.\n";
        return nullptr;
    }

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

static std::string ind(int depth) {
    return std::string(depth * 2, ' ');
}

static std::string typeStr(const Type* t) {
    if (!t) return "<unresolved>";
    return t->toString();
}

static std::string typeStr(const TypePtr<Type>& t) {
    return typeStr(t.get());
}

static std::string overloadableOpStr(OverloadableOp op) {
    switch (op) {
        case OverloadableOp::Plus:         return "+";
        case OverloadableOp::Minus:        return "-";
        case OverloadableOp::Star:         return "*";
        case OverloadableOp::Slash:        return "/";
        case OverloadableOp::Modulo:       return "%";
        case OverloadableOp::Index:        return "[]";
        case OverloadableOp::Equals:       return "==";
        case OverloadableOp::NotEquals:    return "!=";
        case OverloadableOp::Less:         return "<";
        case OverloadableOp::Greater:      return ">";
        case OverloadableOp::LessEqual:    return "<=";
        case OverloadableOp::GreaterEqual: return ">=";
    }
    return "?";
}

static std::string paramListStr(const std::vector<VariableSymbol*>& params) {
    std::string result;
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) result += ", ";
        result += params[i]->name + ": " + typeStr(params[i]->type);
    }
    return result;
}

//================================================================================
// Reachability analysis (standalone, for control flow dump)
//================================================================================
enum class DumpReachability { Always, Sometimes, Never };

static DumpReachability classifyStmt(StatementNode* stmt) {
    if (!stmt) return DumpReachability::Never;

    if (dynamic_cast<ReturnStatement*>(stmt))
        return DumpReachability::Always;

    if (auto* block = dynamic_cast<BlockStatement*>(stmt)) {
        DumpReachability result = DumpReachability::Never;
        for (auto& s : block->statements) {
            if (!s) continue;
            auto r = classifyStmt(s.get());
            if (r == DumpReachability::Always) return DumpReachability::Always;
            if (r == DumpReachability::Sometimes) result = DumpReachability::Sometimes;
        }
        return result;
    }

    if (auto* ifStmt = dynamic_cast<IfStatement*>(stmt)) {
        if (!ifStmt->elseBody) {
            auto thenR = classifyStmt(ifStmt->thenBody.get());
            return (thenR != DumpReachability::Never)
                ? DumpReachability::Sometimes : DumpReachability::Never;
        }
        auto thenR = classifyStmt(ifStmt->thenBody.get());
        if (thenR != DumpReachability::Always) return DumpReachability::Sometimes;
        for (auto& elseIf : ifStmt->elseIfClauses) {
            auto r = classifyStmt(elseIf.body.get());
            if (r != DumpReachability::Always) return DumpReachability::Sometimes;
        }
        auto elseR = classifyStmt(ifStmt->elseBody.get());
        return (elseR == DumpReachability::Always)
            ? DumpReachability::Always : DumpReachability::Sometimes;
    }

    if (auto* sw = dynamic_cast<SwitchStatement*>(stmt)) {
        if (!sw->hasDefault()) return DumpReachability::Never;
        for (auto& c : sw->cases) {
            DumpReachability cr = DumpReachability::Never;
            for (auto& s : c.body) {
                auto r = classifyStmt(s.get());
                if (r == DumpReachability::Always) { cr = DumpReachability::Always; break; }
                if (r == DumpReachability::Sometimes) cr = DumpReachability::Sometimes;
            }
            if (cr != DumpReachability::Always) return DumpReachability::Sometimes;
        }
        DumpReachability dr = DumpReachability::Never;
        for (auto& s : sw->defaultCase) {
            auto r = classifyStmt(s.get());
            if (r == DumpReachability::Always) { dr = DumpReachability::Always; break; }
            if (r == DumpReachability::Sometimes) dr = DumpReachability::Sometimes;
        }
        return (dr == DumpReachability::Always) ? DumpReachability::Always : DumpReachability::Sometimes;
    }

    if (auto* raw = dynamic_cast<RawBlock*>(stmt)) {
        if (raw->body) return classifyStmt(raw->body.get());
    }

    return DumpReachability::Never;
}

static DumpReachability classifyBody(const NodeList<StatementNode>& stmts) {
    DumpReachability result = DumpReachability::Never;
    for (auto& s : stmts) {
        if (!s) continue;
        auto r = classifyStmt(s.get());
        if (r == DumpReachability::Always) return DumpReachability::Always;
        if (r == DumpReachability::Sometimes) result = DumpReachability::Sometimes;
    }
    return result;
}

//================================================================================
// Part 1: Symbol Table Dump — recursive scope walker
//================================================================================
static void dumpScope(Scope* scope, std::ostream& out, int depth) {
    if (!scope) return;

    // Print scope header
    std::string kindStr;
    std::string nameStr;
    switch (scope->kind) {
        case ScopeKind::Global:      kindStr = "Global"; break;
        case ScopeKind::Module:      kindStr = "Module"; break;
        case ScopeKind::TypeMembers: kindStr = "TypeMembers"; break;
        case ScopeKind::Function:    kindStr = "Function"; break;
        case ScopeKind::Block:       kindStr = "Block"; break;
        case ScopeKind::RawBlock:    kindStr = "RawBlock"; break;
    }
    if (scope->ownerSymbol) {
        nameStr = " " + scope->ownerSymbol->name;
    }

    out << ind(depth) << "[Scope: " << kindStr << nameStr << "]\n";

    // Print symbols in this scope
    for (auto& [name, sym] : scope->getSymbolMap()) {
        if (auto* var = sym->as<VariableSymbol>()) {
            std::string roleStr;
            switch (var->role) {
                case VariableRole::Field:     roleStr = "Field[" + std::to_string(var->fieldIndex) + "]"; break;
                case VariableRole::Parameter: roleStr = "Param"; break;
                case VariableRole::Local:     roleStr = "Local"; break;
            }
            out << ind(depth + 1) << roleStr << ": " << var->name
                << " : " << typeStr(var->type);
            if (var->isInferred) out << " [inferred]";
            if (!var->isMutable) out << " [const]";
            out << "\n";
        }
        else if (auto* fn = sym->as<FunctionSymbol>()) {
            // Skip constructors/destructors — printed under their type
            if (fn->kind == SymbolKind::Constructor || fn->kind == SymbolKind::Destructor)
                continue;

            out << ind(depth + 1) << "Function: " << fn->name;
            if (fn->isExtern) out << " (extern)";
            out << "(" << paramListStr(fn->parameters) << ")";
            out << " -> " << typeStr(fn->returnType) << "\n";

            if (fn->bodyScope) {
                dumpScope(fn->bodyScope, out, depth + 2);
            }
        }
        else if (auto* structSym = sym->as<StructSymbol>()) {
            out << ind(depth + 1) << "Struct: " << structSym->name
                << " (fields: " << structSym->fields.size() << ")\n";
            if (structSym->memberScope) {
                dumpScope(structSym->memberScope, out, depth + 2);
            }
        }
        else if (auto* classSym = sym->as<ClassSymbol>()) {
            out << ind(depth + 1) << "Class: " << classSym->name
                << " (fields: " << classSym->fields.size()
                << ", RAII: " << (classSym->hasRAII() ? "yes" : "no") << ")\n";

            if (classSym->constructor) {
                out << ind(depth + 2) << "Constructor("
                    << paramListStr(classSym->constructor->parameters) << ")\n";
            }
            if (classSym->destructor) {
                out << ind(depth + 2) << "Destructor (RAII cleanup)\n";
            }
            if (classSym->memberScope) {
                dumpScope(classSym->memberScope, out, depth + 2);
            }
        }
        else if (auto* enumSym = sym->as<EnumSymbol>()) {
            out << ind(depth + 1) << "Enum: " << enumSym->name
                << " (members: " << enumSym->members.size() << ")\n";
            for (auto& member : enumSym->members) {
                out << ind(depth + 2) << member.name << " = " << member.value << "\n";
            }
        }
        else if (auto* modSym = sym->as<ModuleSymbol>()) {
            out << ind(depth + 1) << "Module: " << modSym->name << "\n";
            if (modSym->moduleScope) {
                dumpScope(modSym->moduleScope, out, depth + 2);
            }
        }
    }

    // Print operators
    for (auto* op : scope->getOperatorList()) {
        out << ind(depth + 1) << "Operator: " << overloadableOpStr(op->op)
            << "(" << paramListStr(op->parameters) << ")"
            << " -> " << typeStr(op->returnType) << "\n";
    }
}

//================================================================================
// Part 2 & 3: SemaDumper — walks AST for expression types + validation info
//================================================================================
class SemaDumper : public ASTVisitor {
public:
    enum class Phase { ExpressionTypes, Pass4 };

    SemaDumper(SymbolTable& table, TypeRegistry& registry,
               const SemanticValidator& validator, std::ostream& out)
        : symbolTable_(table)
        , registry_(registry)
        , validator_(validator)
        , out_(out)
        , currentScope_(nullptr)
        , depth_(0)
        , phase_(Phase::ExpressionTypes)
        , loopDepth_(0)
        , rawDepth_(0)
    {}

    void dumpExpressionTypes(ProgramNode& program) {
        phase_ = Phase::ExpressionTypes;
        currentScope_ = symbolTable_.getGlobalScope();
        childIndexStack_.clear();
        depth_ = 0;
        visit(program);
    }

    void dumpPass4(ProgramNode& program) {
        phase_ = Phase::Pass4;
        currentScope_ = symbolTable_.getGlobalScope();
        childIndexStack_.clear();
        depth_ = 0;
        loopDepth_ = 0;
        rawDepth_ = 0;
        visit(program);

        // Dump RAII info from SemanticValidator
        auto& raiiInfo = validator_.getRAIIInfo();
        if (!raiiInfo.empty()) {
            out_ << "\n  [RAII Injection Points]\n";
            for (auto& [scope, info] : raiiInfo) {
                if (info.destructibles.empty()) continue;
                std::string scopeName = "<anonymous>";
                if (scope->ownerSymbol) scopeName = scope->ownerSymbol->name;
                out_ << "    Scope '" << scopeName << "':\n";
                for (auto& [var, dtor] : info.destructibles) {
                    std::string ownerName = "<unknown>";
                    if (dtor->owner) ownerName = dtor->owner->name;
                    out_ << "      RAII variable: " << var->name
                         << " (" << typeStr(var->type) << ")"
                         << " -> " << ownerName << ".destructor\n";
                }
                // Show LIFO destruction order
                out_ << "      Scope exit: destroy [";
                for (int i = static_cast<int>(info.destructibles.size()) - 1; i >= 0; --i) {
                    if (i < static_cast<int>(info.destructibles.size()) - 1) out_ << ", ";
                    out_ << info.destructibles[i].first->name;
                }
                out_ << "] (LIFO)\n";
            }
        }
    }

    // === Program structure ===
    void visit(ProgramNode& node) override {
        for (auto& mod : node.modules) {
            if (mod) mod->accept(*this);
        }
    }

    void visit(ModuleNode& node) override {
        auto* sym = currentScope_->lookupLocal(node.name);
        if (!sym || !sym->is<ModuleSymbol>()) return;
        enterNamedScope(sym->as<ModuleSymbol>()->moduleScope);

        if (phase_ == Phase::ExpressionTypes) {
            out_ << "\n  [Module: " << node.name << "]\n";
        } else {
            out_ << "\n  [Module: " << node.name << "]\n";
        }

        currentModule_ = node.name;
        for (auto& decl : node.declarations) {
            if (decl) decl->accept(*this);
        }

        leaveNamedScope();
    }

    void visit(ImportNode&) override {}

    // === Declarations ===
    void visit(StructDeclaration& node) override {
        auto* sym = currentScope_->lookupLocal(node.name);
        if (!sym || !sym->is<StructSymbol>()) return;
        enterNamedScope(sym->as<TypeSymbol>()->memberScope);

        for (auto& method : node.methods) {
            if (method) method->accept(*this);
        }
        for (auto& op : node.operators) {
            if (op) op->accept(*this);
        }

        leaveNamedScope();
    }

    void visit(ClassDeclaration& node) override {
        auto* sym = currentScope_->lookupLocal(node.name);
        if (!sym || !sym->is<ClassSymbol>()) return;
        enterNamedScope(sym->as<TypeSymbol>()->memberScope);

        if (node.constructor) node.constructor->accept(*this);
        if (node.destructor) node.destructor->accept(*this);
        for (auto& method : node.methods) {
            if (method) method->accept(*this);
        }
        for (auto& op : node.operators) {
            if (op) op->accept(*this);
        }

        leaveNamedScope();
    }

    void visit(FunctionDeclaration& node) override {
        auto* sym = currentScope_->lookupLocal(node.name);
        if (!sym || !sym->is<FunctionSymbol>()) return;
        auto* fnSym = sym->as<FunctionSymbol>();
        if (!node.body) return;

        std::string qualName = currentModule_ + "::" + node.name;

        if (phase_ == Phase::ExpressionTypes) {
            out_ << "    [Function: " << node.name << "]\n";
            enterNamedScope(fnSym->bodyScope);
            ++depth_;
            visitStatements(node.body->statements);
            --depth_;
            leaveNamedScope();
        }
        else if (phase_ == Phase::Pass4) {
            // Control flow analysis
            bool isVoid = false;
            if (fnSym->returnType) {
                auto* prim = fnSym->returnType->as<PrimitiveType>();
                isVoid = prim && prim->kind == PrimitiveType::PrimitiveKind::Void;
            }

            if (isVoid) {
                out_ << "    " << qualName << ": void function, no return required\n";
            } else {
                auto reach = classifyBody(node.body->statements);
                if (reach == DumpReachability::Always) {
                    out_ << "    " << qualName << ": all paths return "
                         << typeStr(fnSym->returnType) << "\n";
                } else {
                    out_ << "    " << qualName << ": WARNING — not all paths return\n";
                }
            }

            // Also walk body for raw block / lambda / match info
            enterNamedScope(fnSym->bodyScope);
            visitStatements(node.body->statements);
            leaveNamedScope();
        }
    }

    void visit(ConstructorDeclaration& node) override {
        auto* sym = currentScope_->lookupLocal("constructor");
        if (!sym || !sym->is<ConstructorSymbol>()) return;
        auto* ctorSym = sym->as<ConstructorSymbol>();
        if (!node.body) return;

        enterNamedScope(ctorSym->bodyScope);
        visitStatements(node.body->statements);
        leaveNamedScope();
    }

    void visit(DestructorDeclaration& node) override {
        auto* sym = currentScope_->lookupLocal("destructor");
        if (!sym || !sym->is<DestructorSymbol>()) return;
        auto* dtorSym = sym->as<DestructorSymbol>();
        if (!node.body) return;

        enterNamedScope(dtorSym->bodyScope);
        visitStatements(node.body->statements);
        leaveNamedScope();
    }

    void visit(OperatorDeclaration& node) override {
        auto overloadOp = operatorKindToOverloadableOp(node.op);
        auto* opSym = currentScope_->lookupOperator(overloadOp);
        if (!opSym || !node.body) return;

        if (phase_ == Phase::Pass4) {
            bool isVoid = false;
            if (opSym->returnType) {
                auto* prim = opSym->returnType->as<PrimitiveType>();
                isVoid = prim && prim->kind == PrimitiveType::PrimitiveKind::Void;
            }
            if (!isVoid) {
                auto reach = classifyBody(node.body->statements);
                if (reach == DumpReachability::Always) {
                    out_ << "    operator" << overloadableOpStr(overloadOp)
                         << ": all paths return " << typeStr(opSym->returnType) << "\n";
                }
            }
        }

        enterNamedScope(opSym->bodyScope);
        visitStatements(node.body->statements);
        leaveNamedScope();
    }

    void visit(VariableDeclaration& node) override {
        if (phase_ == Phase::ExpressionTypes && node.isInferred && node.initializer) {
            auto* sym = currentScope_->lookupLocal(node.name);
            auto* varSym = sym ? sym->as<VariableSymbol>() : nullptr;
            if (varSym && varSym->type) {
                out_ << "      L" << node.location.line << ": var " << node.name
                     << " = ... -> " << typeStr(varSym->type) << " [inferred]\n";
            }
        }
        if (node.initializer) node.initializer->accept(*this);
    }

    void visit(TupleDestructuringDeclaration& node) override {
        if (phase_ == Phase::ExpressionTypes) {
            out_ << "      L" << node.location.line << ": tuple destructuring (";
            for (size_t i = 0; i < node.elements.size(); ++i) {
                if (i > 0) out_ << ", ";
                auto* sym = currentScope_->lookupLocal(node.elements[i].name);
                auto* varSym = sym ? sym->as<VariableSymbol>() : nullptr;
                out_ << node.elements[i].name << ": "
                     << (varSym ? typeStr(varSym->type) : "<unresolved>");
            }
            out_ << ")\n";
        }
        if (node.initializer) node.initializer->accept(*this);
    }

    void visit(ExternFunctionDeclaration&) override {}
    void visit(EnumDeclaration&) override {}
    void visit(EnumMemberNode&) override {}
    void visit(ParameterNode&) override {}

    // === Statements ===
    void visit(BlockStatement& node) override {
        enterNextChildScope();
        visitStatements(node.statements);
        leaveChildScope();
    }

    void visit(ExpressionStatement& node) override {
        if (node.expression) node.expression->accept(*this);
    }

    void visit(ReturnStatement& node) override {
        if (node.value) node.value->accept(*this);
    }

    void visit(IfStatement& node) override {
        if (node.condition) node.condition->accept(*this);
        if (node.thenBody) node.thenBody->accept(*this);
        for (auto& elseIf : node.elseIfClauses) {
            if (elseIf.condition) elseIf.condition->accept(*this);
            if (elseIf.body) elseIf.body->accept(*this);
        }
        if (node.elseBody) node.elseBody->accept(*this);
    }

    void visit(SwitchStatement& node) override {
        if (node.subject) node.subject->accept(*this);
        for (auto& c : node.cases) {
            if (c.value) c.value->accept(*this);
            visitStatements(c.body);
        }
        visitStatements(node.defaultCase);
    }

    void visit(ForStatement& node) override {
        enterNextChildScope();
        if (node.initDeclaration) node.initDeclaration->accept(*this);
        for (auto& e : node.initExpressions) { if (e) e->accept(*this); }
        if (node.condition) node.condition->accept(*this);
        for (auto& e : node.iterators) { if (e) e->accept(*this); }
        if (node.body) node.body->accept(*this);
        leaveChildScope();
    }

    void visit(WhileStatement& node) override {
        if (node.condition) node.condition->accept(*this);
        if (node.body) node.body->accept(*this);
    }

    void visit(DoWhileStatement& node) override {
        if (node.body) node.body->accept(*this);
        if (node.condition) node.condition->accept(*this);
    }

    void visit(BreakStatement&) override {}
    void visit(ContinueStatement&) override {}

    void visit(DeleteStatement& node) override {
        if (node.target) node.target->accept(*this);
    }

    void visit(RawBlock& node) override {
        ++rawDepth_;
        enterNextChildScope();
        if (node.body) visitStatements(node.body->statements);
        leaveChildScope();
        --rawDepth_;
    }

    // === Expressions ===
    void visit(IntegerLiteral&) override {}
    void visit(FloatLiteral&) override {}
    void visit(BoolLiteral&) override {}
    void visit(CharLiteral&) override {}
    void visit(StringLiteral&) override {}
    void visit(NullLiteral&) override {}

    void visit(InterpolatedString& node) override {
        for (auto& part : node.parts) {
            if (part.kind == InterpolatedPartKind::Expression && part.expression)
                part.expression->accept(*this);
        }
    }

    void visit(IdentifierExpression&) override {}
    void visit(QualifiedNameExpression&) override {}
    void visit(ThisExpression&) override {}

    void visit(BinaryExpression& node) override {
        if (phase_ == Phase::ExpressionTypes && node.isOperatorOverload) {
            std::string lhsType = node.left ? typeStr(node.left->resolvedType) : "?";
            std::string rhsType = node.right ? typeStr(node.right->resolvedType) : "?";
            std::string resultType = typeStr(node.resolvedType);
            std::string opName = binaryOpToString(node.op);

            out_ << "      L" << node.location.line << ": " << opName << ": "
                 << lhsType << " " << opName << " " << rhsType
                 << " -> " << resultType << " [OPERATOR OVERLOAD";
            if (node.resolvedOperatorFunction) {
                auto* opSym = node.resolvedOperatorFunction->as<OperatorSymbol>();
                if (opSym && opSym->ownerType) {
                    out_ << ": " << opSym->ownerType->name << ".operator" << opName;
                }
            }
            out_ << "]\n";
        }

        if (node.left) node.left->accept(*this);
        if (node.right) node.right->accept(*this);
    }

    void visit(UnaryExpression& node) override {
        if (node.operand) node.operand->accept(*this);
    }

    void visit(AssignmentExpression& node) override {
        if (phase_ == Phase::ExpressionTypes && assignOpIsCompound(node.op)) {
            std::string targetType = node.target ? typeStr(node.target->resolvedType) : "?";
            out_ << "      L" << node.location.line << ": " << assignOpToString(node.op)
                 << ": " << targetType << " " << assignOpToString(node.op) << " "
                 << (node.value ? typeStr(node.value->resolvedType) : "?")
                 << " -> " << targetType << "\n";
        }
        if (node.target) node.target->accept(*this);
        if (node.value) node.value->accept(*this);
    }

    void visit(TernaryExpression& node) override {
        if (node.condition) node.condition->accept(*this);
        if (node.thenExpr) node.thenExpr->accept(*this);
        if (node.elseExpr) node.elseExpr->accept(*this);
    }

    void visit(CallExpression& node) override {
        if (phase_ == Phase::ExpressionTypes) {
            // Print call info
            std::string calleeName;
            if (auto* ident = dynamic_cast<IdentifierExpression*>(node.callee.get())) {
                calleeName = ident->name;
            } else if (auto* member = dynamic_cast<MemberAccessExpression*>(node.callee.get())) {
                std::string objType = member->object ? typeStr(member->object->resolvedType) : "?";
                calleeName = objType + "." + member->memberName;
            }

            if (!calleeName.empty()) {
                out_ << "      L" << node.location.line << ": Call: " << calleeName << "(";
                for (size_t i = 0; i < node.arguments.size(); ++i) {
                    if (i > 0) out_ << ", ";
                    out_ << (node.arguments[i] ? typeStr(node.arguments[i]->resolvedType) : "?");
                }
                out_ << ") -> " << typeStr(node.resolvedType) << "\n";
            }
        }

        if (node.callee) node.callee->accept(*this);
        for (auto& arg : node.arguments) { if (arg) arg->accept(*this); }
    }

    void visit(NewExpression& node) override {
        if (phase_ == Phase::ExpressionTypes) {
            std::string typeName = node.type && node.type->resolvedType
                ? typeStr(node.type->resolvedType) : "?";
            if (node.isArray) {
                out_ << "      L" << node.location.line << ": new " << typeName << "[...] -> "
                     << typeStr(node.resolvedType) << "\n";
            } else {
                out_ << "      L" << node.location.line << ": new " << typeName << "(...) -> "
                     << typeStr(node.resolvedType) << "\n";
            }
        }
        for (auto& arg : node.arguments) { if (arg) arg->accept(*this); }
        if (node.arraySize) node.arraySize->accept(*this);
    }

    void visit(IndexExpression& node) override {
        if (phase_ == Phase::ExpressionTypes && node.isOperatorOverload) {
            std::string objType = node.object ? typeStr(node.object->resolvedType) : "?";
            out_ << "      L" << node.location.line << ": Index: " << objType
                 << "[" << (node.index ? typeStr(node.index->resolvedType) : "?") << "]"
                 << " -> " << typeStr(node.resolvedType) << " [OPERATOR OVERLOAD]\n";
        }
        if (node.object) node.object->accept(*this);
        if (node.index) node.index->accept(*this);
    }

    void visit(CastExpression& node) override {
        if (node.operand) node.operand->accept(*this);
    }

    void visit(MemberAccessExpression& node) override {
        if (node.object) node.object->accept(*this);
    }

    void visit(SizeOfExpression&) override {}
    void visit(AlignOfExpression&) override {}

    void visit(PipeExpression& node) override {
        if (phase_ == Phase::ExpressionTypes) {
            out_ << "      L" << node.location.line << ": Pipe: "
                 << (node.input ? typeStr(node.input->resolvedType) : "?");
            for (auto& stage : node.stages) {
                if (stage.function) {
                    out_ << " |> " << stage.function->getName();
                }
            }
            out_ << " -> " << typeStr(node.resolvedType) << "\n";
        }
        if (node.input) node.input->accept(*this);
        for (auto& stage : node.stages) {
            if (stage.function) stage.function->accept(*this);
            for (auto& arg : stage.extraArguments) { if (arg) arg->accept(*this); }
        }
    }

    void visit(MatchExpression& node) override {
        if (phase_ == Phase::ExpressionTypes || phase_ == Phase::Pass4) {
            std::string subjectType = node.subject ? typeStr(node.subject->resolvedType) : "?";

            // Count pattern types
            int literals = 0, wildcards = 0, bindings = 0, guarded = 0;
            for (auto& arm : node.arms) {
                if (!arm.pattern) continue;
                if (dynamic_cast<WildcardPattern*>(arm.pattern.get())) ++wildcards;
                else if (dynamic_cast<BindingPattern*>(arm.pattern.get())) ++bindings;
                else if (auto* g = dynamic_cast<GuardedPattern*>(arm.pattern.get())) {
                    ++guarded;
                    // Check if inner is binding/wildcard
                    if (dynamic_cast<WildcardPattern*>(g->innerPattern.get())) ++wildcards;
                    else if (dynamic_cast<BindingPattern*>(g->innerPattern.get())) ++bindings;
                }
                else if (dynamic_cast<LiteralPattern*>(arm.pattern.get())) ++literals;
            }

            bool exhaustive = (wildcards > 0) || (bindings > 0 && guarded == 0);
            // Check enum exhaustiveness
            if (!exhaustive && node.subject && node.subject->resolvedType) {
                auto* ut = node.subject->resolvedType->as<UserType>();
                if (ut && ut->symbol) {
                    auto* ts = static_cast<TypeSymbol*>(ut->symbol);
                    auto* enumSym = ts->as<EnumSymbol>();
                    if (enumSym) {
                        exhaustive = (literals >= static_cast<int>(enumSym->members.size()));
                    }
                }
            }

            if (phase_ == Phase::ExpressionTypes) {
                out_ << "      L" << node.location.line << ": Match on " << subjectType
                     << " (" << node.arms.size() << " arms: "
                     << literals << " literal, " << guarded << " guarded, "
                     << wildcards << " wildcard)\n";
            }
            if (phase_ == Phase::Pass4) {
                out_ << "    L" << node.location.line << ": match on " << subjectType
                     << " -> " << (exhaustive ? "EXHAUSTIVE" : "NOT EXHAUSTIVE") << "\n";
            }
        }

        if (node.subject) node.subject->accept(*this);
        for (auto& arm : node.arms) {
            enterNextChildScope();
            if (arm.pattern) arm.pattern->accept(*this);
            if (arm.body) arm.body->accept(*this);
            leaveChildScope();
        }
    }

    void visit(TupleExpression& node) override {
        for (auto& e : node.elements) { if (e) e->accept(*this); }
    }

    void visit(LambdaExpression& node) override {
        std::string lambdaType = typeStr(node.resolvedType);

        if (phase_ == Phase::ExpressionTypes) {
            out_ << "      L" << node.location.line << ": Lambda: " << lambdaType;
            if (!node.capturedVariables.empty()) {
                out_ << ", captures: [";
                for (size_t i = 0; i < node.capturedVariables.size(); ++i) {
                    if (i > 0) out_ << ", ";
                    out_ << node.capturedVariables[i]->name;
                    if (auto* var = node.capturedVariables[i]->as<VariableSymbol>()) {
                        out_ << ": " << typeStr(var->type);
                    }
                }
                out_ << "] -> CLOSURE REQUIRED";
            } else {
                out_ << ", captures: [] -> simple function pointer";
            }
            out_ << "\n";
        }

        if (phase_ == Phase::Pass4 && !node.capturedVariables.empty()) {
            out_ << "    L" << node.location.line << ": lambda captures [";
            for (size_t i = 0; i < node.capturedVariables.size(); ++i) {
                if (i > 0) out_ << ", ";
                out_ << node.capturedVariables[i]->name;
            }
            out_ << "] -> closure struct required\n";
        }

        enterNextChildScope();
        if (node.body) node.body->accept(*this);
        leaveChildScope();
    }

    // === Patterns ===
    void visit(LiteralPattern& node) override { if (node.value) node.value->accept(*this); }
    void visit(RangePattern&) override {}
    void visit(WildcardPattern&) override {}
    void visit(BindingPattern&) override {}
    void visit(TuplePattern& node) override {
        for (auto& e : node.elements) { if (e) e->accept(*this); }
    }
    void visit(GuardedPattern& node) override {
        if (node.innerPattern) node.innerPattern->accept(*this);
        if (node.guard) node.guard->accept(*this);
    }

    // === Type nodes (no-op) ===
    void visit(TypeNode&) override {}
    void visit(PrimitiveTypeNode&) override {}
    void visit(NamedTypeNode&) override {}
    void visit(PointerTypeNode&) override {}
    void visit(ArrayTypeNode&) override {}
    void visit(TupleTypeNode&) override {}
    void visit(FunctionTypeNode&) override {}

private:
    SymbolTable& symbolTable_;
    TypeRegistry& registry_;
    const SemanticValidator& validator_;
    std::ostream& out_;
    Scope* currentScope_;
    int depth_;
    Phase phase_;
    std::string currentModule_;

    // For Pass 4 tracking
    int loopDepth_;
    int rawDepth_;

    std::vector<size_t> childIndexStack_;

    void enterNamedScope(Scope* scope) {
        currentScope_ = scope;
        childIndexStack_.push_back(0);
    }

    void leaveNamedScope() {
        if (!childIndexStack_.empty()) childIndexStack_.pop_back();
        if (currentScope_->parent) currentScope_ = currentScope_->parent;
    }

    void enterNextChildScope() {
        if (childIndexStack_.empty()) childIndexStack_.push_back(0);
        size_t& idx = childIndexStack_.back();
        if (idx < currentScope_->children.size()) {
            currentScope_ = currentScope_->children[idx++].get();
            childIndexStack_.push_back(0);
        }
    }

    void leaveChildScope() {
        if (!childIndexStack_.empty()) childIndexStack_.pop_back();
        if (currentScope_->parent) currentScope_ = currentScope_->parent;
    }

    void visitStatements(NodeList<StatementNode>& stmts) {
        for (auto& s : stmts) { if (s) s->accept(*this); }
    }
};

//================================================================================
// Main
//================================================================================
int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << "Mingus Semantic Analysis Dump\n";
    std::cout << "========================================\n\n";

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file.mingus>\n";
        return 1;
    }

    std::string filename = argv[1];
    std::cout << "Analyzing: " << filename << "\n";

    try {
        // Parse
        auto program = parseFile(filename);
        if (!program) {
            std::cerr << "Failed to parse file.\n";
            return 1;
        }

        // Run semantic analysis
        SymbolTable symbolTable;
        TypeRegistry registry;
        ErrorReporter errors;

        // Pass 1: Symbol Table Building
        SymbolTableBuilder builder(symbolTable, errors);
        builder.build(*program);

        // Pass 2: Type Resolution
        TypeResolver resolver(symbolTable, registry, errors);
        resolver.resolve(*program);

        // Pass 3: Type Checking
        TypeChecker checker(symbolTable, registry, errors);
        checker.check(*program);

        // Pass 4: Semantic Validation
        SemanticValidator validator(symbolTable, registry, errors);
        validator.validate(*program);

        // === Dump Pass 1: Symbol Table ===
        std::cout << "\n----------------------------------------\n";
        std::cout << "Pass 1: Symbol Table\n";
        std::cout << "----------------------------------------\n\n";

        dumpScope(symbolTable.getGlobalScope(), std::cout, 0);

        // === Dump Pass 3: Expression Types ===
        std::cout << "\n----------------------------------------\n";
        std::cout << "Pass 3: Expression Types\n";
        std::cout << "----------------------------------------\n";

        SemaDumper dumper(symbolTable, registry, validator, std::cout);
        dumper.dumpExpressionTypes(*program);

        // === Dump Pass 4: Semantic Validation ===
        std::cout << "\n----------------------------------------\n";
        std::cout << "Pass 4: Semantic Validation\n";
        std::cout << "----------------------------------------\n";

        std::cout << "\n  [Control Flow]\n";
        dumper.dumpPass4(*program);

        std::cout << "\n  [Pattern Exhaustiveness]\n";
        std::cout << "  (see match expression entries above)\n";

        std::cout << "\n  [Lambda Captures]\n";
        std::cout << "  (see lambda entries above)\n";

        // === Diagnostics Summary ===
        std::cout << "\n----------------------------------------\n";
        std::cout << "Semantic analysis completed: "
                  << errors.getErrorCount() << " errors, "
                  << errors.getWarningCount() << " warnings\n";
        std::cout << "----------------------------------------\n";

        if (errors.hasErrors() || errors.getWarningCount() > 0) {
            std::cout << "\nDiagnostics:\n";
            for (auto& diag : errors.getDiagnostics()) {
                std::cout << "  " << diag.toString() << "\n";
            }
        }

        return errors.hasErrors() ? 1 : 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
