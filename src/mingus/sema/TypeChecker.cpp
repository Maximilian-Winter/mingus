//================================================================================
// MINGUS v1 - Type Checker Implementation (Pass 3)
// Walks every expression/statement. Sets resolvedType on all ExpressionNodes.
// Checks type compatibility. Resolves identifiers. Handles operator overloads.
//================================================================================

#include "mingus/sema/TypeChecker.h"

#include <algorithm>

namespace mingus {
namespace sema {

//================================================================================
// Constructor & Entry Point
//================================================================================
TypeChecker::TypeChecker(SymbolTable& table, TypeRegistry& registry, ErrorReporter& errors)
    : symbolTable_(table)
    , registry_(registry)
    , errors_(errors)
    , currentScope_(nullptr)
    , currentFunction_(nullptr)
    , currentReturnType_(nullptr)
    , currentType_(nullptr)
    , matchSubjectType_(nullptr)
{
}

void TypeChecker::check(ProgramNode& program) {
    currentScope_ = symbolTable_.getGlobalScope();
    visit(program);
}

//================================================================================
// Scope Navigation
//================================================================================
void TypeChecker::enterNamedScope(Scope* scope) {
    currentScope_ = scope;
    childIndexStack_.push_back(0);
}

void TypeChecker::leaveNamedScope() {
    if (!childIndexStack_.empty()) {
        childIndexStack_.pop_back();
    }
    if (currentScope_->parent) {
        currentScope_ = currentScope_->parent;
    }
}

void TypeChecker::enterNextChildScope() {
    if (childIndexStack_.empty()) {
        childIndexStack_.push_back(0);
    }
    size_t& idx = childIndexStack_.back();
    if (idx < currentScope_->children.size()) {
        currentScope_ = currentScope_->children[idx++].get();
        childIndexStack_.push_back(0);
    }
}

void TypeChecker::leaveChildScope() {
    if (!childIndexStack_.empty()) {
        childIndexStack_.pop_back();
    }
    if (currentScope_->parent) {
        currentScope_ = currentScope_->parent;
    }
}

//================================================================================
// Helpers
//================================================================================
void TypeChecker::visitStatements(NodeList<StatementNode>& stmts) {
    for (auto& stmt : stmts) {
        if (stmt) stmt->accept(*this);
    }
}

bool TypeChecker::isVoidType(const Type* t) const {
    if (!t) return false;
    auto* prim = t->as<PrimitiveType>();
    return prim && prim->kind == PrimitiveType::PrimitiveKind::Void;
}

void TypeChecker::checkAssignability(const Type* from, const Type* to,
                                      const SourceLocation& loc,
                                      const std::string& context) {
    if (!from || !to) return;
    if (from->is<ErrorType>() || to->is<ErrorType>()) return;
    if (!registry_.isCompatible(from, to)) {
        std::string msg = "cannot convert '" + from->toString() +
                          "' to '" + to->toString() + "'";
        if (!context.empty()) {
            msg = "in " + context + ": " + msg;
        }
        errors_.error(loc, msg);
    }
}

bool TypeChecker::isLValue(ExpressionNode* expr) {
    if (!expr) return false;
    if (dynamic_cast<IdentifierExpression*>(expr)) return true;
    if (dynamic_cast<MemberAccessExpression*>(expr)) return true;
    if (dynamic_cast<IndexExpression*>(expr)) return true;
    if (auto* unary = dynamic_cast<UnaryExpression*>(expr)) {
        return unary->op == UnaryOp::Dereference;
    }
    return false;
}

TypePtr<Type> TypeChecker::getSymbolType(Symbol* sym) {
    if (!sym) return registry_.getErrorType();

    if (auto* var = sym->as<VariableSymbol>()) {
        return var->type ? var->type : registry_.getErrorType();
    }

    if (auto* fn = sym->as<FunctionSymbol>()) {
        TypeList<Type> paramTypes;
        for (auto* param : fn->parameters) {
            paramTypes.push_back(param->type ? param->type : registry_.getErrorType());
        }
        auto retType = fn->returnType ? fn->returnType : registry_.getVoid();
        return registry_.getFunctionType(std::move(paramTypes), retType);
    }

    if (auto* typeSym = sym->as<TypeSymbol>()) {
        Type::Kind kind;
        switch (typeSym->kind) {
            case SymbolKind::Struct:     kind = Type::Kind::Struct;     break;
            case SymbolKind::Class:      kind = Type::Kind::Class;      break;
            case SymbolKind::Enum:       kind = Type::Kind::Enum;       break;
            case SymbolKind::Interface:  kind = Type::Kind::Interface;  break;
            default:                     kind = Type::Kind::Struct;     break;
        }
        return registry_.getUserType(typeSym->name, kind, typeSym);
    }

    return registry_.getErrorType();
}

std::optional<OverloadableOp> TypeChecker::binaryOpToOverloadableOp(BinaryOp op) {
    switch (op) {
        case BinaryOp::Add:          return OverloadableOp::Plus;
        case BinaryOp::Sub:          return OverloadableOp::Minus;
        case BinaryOp::Mul:          return OverloadableOp::Star;
        case BinaryOp::Div:          return OverloadableOp::Slash;
        case BinaryOp::Mod:          return OverloadableOp::Modulo;
        case BinaryOp::Equal:        return OverloadableOp::Equals;
        case BinaryOp::NotEqual:     return OverloadableOp::NotEquals;
        case BinaryOp::Less:         return OverloadableOp::Less;
        case BinaryOp::LessEqual:    return OverloadableOp::LessEqual;
        case BinaryOp::Greater:      return OverloadableOp::Greater;
        case BinaryOp::GreaterEqual: return OverloadableOp::GreaterEqual;
        default: return std::nullopt;
    }
}

//================================================================================
// resolveTypeNode — same logic as TypeResolver
//================================================================================
TypePtr<Type> TypeChecker::resolveTypeNode(TypeNode* node) {
    if (!node) return registry_.getErrorType();
    if (node->resolvedType) return node->resolvedType;

    if (auto* prim = dynamic_cast<PrimitiveTypeNode*>(node)) {
        node->resolvedType = registry_.getPrimitive(prim->kind);
        return node->resolvedType;
    }

    if (auto* named = dynamic_cast<NamedTypeNode*>(node)) {
        Symbol* sym = nullptr;
        if (named->qualifiedName.size() == 1) {
            sym = currentScope_->lookup(named->qualifiedName[0]);
        } else {
            sym = currentScope_->lookup(named->qualifiedName[0]);
            for (size_t i = 1; i < named->qualifiedName.size() && sym; ++i) {
                Scope* innerScope = nullptr;
                if (auto* mod = sym->as<ModuleSymbol>()) {
                    innerScope = mod->moduleScope;
                } else if (auto* ts = sym->as<TypeSymbol>()) {
                    innerScope = ts->memberScope;
                }
                if (!innerScope) {
                    node->resolvedType = registry_.getErrorType();
                    return node->resolvedType;
                }
                sym = innerScope->lookupLocal(named->qualifiedName[i]);
            }
        }

        if (!sym) {
            errors_.error(node->location, "unknown type '" + named->getName() + "'");
            node->resolvedType = registry_.getErrorType();
            return node->resolvedType;
        }

        auto* typeSym = sym->as<TypeSymbol>();
        if (!typeSym) {
            errors_.error(node->location, "'" + sym->name + "' is not a type");
            node->resolvedType = registry_.getErrorType();
            return node->resolvedType;
        }

        Type::Kind typeKind;
        switch (typeSym->kind) {
            case SymbolKind::Struct:     typeKind = Type::Kind::Struct;     break;
            case SymbolKind::Class:      typeKind = Type::Kind::Class;      break;
            case SymbolKind::Enum:       typeKind = Type::Kind::Enum;       break;
            case SymbolKind::Interface:  typeKind = Type::Kind::Interface;  break;
            default:                     typeKind = Type::Kind::Struct;     break;
        }
        node->resolvedType = registry_.getUserType(typeSym->name, typeKind, typeSym);
        return node->resolvedType;
    }

    if (auto* ptr = dynamic_cast<PointerTypeNode*>(node)) {
        auto base = resolveTypeNode(ptr->baseType.get());
        node->resolvedType = registry_.getPointerTo(base);
        return node->resolvedType;
    }

    if (auto* arr = dynamic_cast<ArrayTypeNode*>(node)) {
        auto elem = resolveTypeNode(arr->elementType.get());
        int size = -1;
        if (arr->size) {
            if (auto* intLit = dynamic_cast<IntegerLiteral*>(arr->size.get())) {
                size = static_cast<int>(intLit->value);
            }
        }
        node->resolvedType = registry_.getArrayOf(elem, size);
        return node->resolvedType;
    }

    if (auto* tup = dynamic_cast<TupleTypeNode*>(node)) {
        TypeList<Type> elemTypes;
        for (auto& elemNode : tup->elementTypes) {
            elemTypes.push_back(resolveTypeNode(elemNode.get()));
        }
        node->resolvedType = registry_.getTupleOf(std::move(elemTypes));
        return node->resolvedType;
    }

    if (auto* fn = dynamic_cast<FunctionTypeNode*>(node)) {
        TypeList<Type> paramTypes;
        for (auto& paramNode : fn->parameterTypes) {
            paramTypes.push_back(resolveTypeNode(paramNode.get()));
        }
        auto retType = fn->returnType
            ? resolveTypeNode(fn->returnType.get())
            : registry_.getVoid();
        node->resolvedType = registry_.getFunctionType(std::move(paramTypes), retType);
        return node->resolvedType;
    }

    node->resolvedType = registry_.getErrorType();
    return node->resolvedType;
}

//================================================================================
// Program Structure
//================================================================================
void TypeChecker::visit(ProgramNode& node) {
    for (auto& mod : node.modules) {
        if (mod) mod->accept(*this);
    }
}

void TypeChecker::visit(ModuleNode& node) {
    auto* sym = currentScope_->lookupLocal(node.name);
    if (!sym || !sym->is<ModuleSymbol>()) return;
    enterNamedScope(sym->as<ModuleSymbol>()->moduleScope);

    for (auto& decl : node.declarations) {
        if (decl) decl->accept(*this);
    }

    leaveNamedScope();
}

void TypeChecker::visit(ImportNode& /*node*/) {}

//================================================================================
// Declarations — entering bodies for type checking
//================================================================================
void TypeChecker::visit(InterfaceDeclaration& node) {
    auto* sym = currentScope_->lookupLocal(node.name);
    if (!sym || !sym->is<InterfaceSymbol>()) return;
    auto* ifaceSym = sym->as<InterfaceSymbol>();
    enterNamedScope(ifaceSym->memberScope);

    for (auto& method : node.methods) {
        if (method) method->accept(*this);
    }

    leaveNamedScope();
}

void TypeChecker::visit(StructDeclaration& node) {
    auto* sym = currentScope_->lookupLocal(node.name);
    if (!sym || !sym->is<StructSymbol>()) return;
    auto* prevType = currentType_;
    currentType_ = sym->as<TypeSymbol>();
    enterNamedScope(currentType_->memberScope);

    for (auto& method : node.methods) {
        if (method) method->accept(*this);
    }
    for (auto& op : node.operators) {
        if (op) op->accept(*this);
    }

    leaveNamedScope();
    currentType_ = prevType;
}

void TypeChecker::visit(ClassDeclaration& node) {
    auto* sym = currentScope_->lookupLocal(node.name);
    if (!sym || !sym->is<ClassSymbol>()) return;
    auto* prevType = currentType_;
    currentType_ = sym->as<TypeSymbol>();
    enterNamedScope(currentType_->memberScope);

    if (node.constructor) node.constructor->accept(*this);
    if (node.destructor) node.destructor->accept(*this);
    for (auto& method : node.methods) {
        if (method) method->accept(*this);
    }
    for (auto& op : node.operators) {
        if (op) op->accept(*this);
    }

    leaveNamedScope();
    currentType_ = prevType;
}

void TypeChecker::visit(FunctionDeclaration& node) {
    auto* sym = currentScope_->lookupLocal(node.name);
    if (!sym || !sym->is<FunctionSymbol>()) return;
    auto* fnSym = sym->as<FunctionSymbol>();

    if (!node.body) return;

    auto* prevFn = currentFunction_;
    auto prevRetType = currentReturnType_;
    currentFunction_ = fnSym;
    currentReturnType_ = fnSym->returnType;

    enterNamedScope(fnSym->bodyScope);
    visitStatements(node.body->statements);
    leaveNamedScope();

    currentFunction_ = prevFn;
    currentReturnType_ = prevRetType;
}

void TypeChecker::visit(ConstructorDeclaration& node) {
    auto* sym = currentScope_->lookupLocal("constructor");
    if (!sym || !sym->is<ConstructorSymbol>()) return;
    auto* ctorSym = sym->as<ConstructorSymbol>();

    if (!node.body) return;

    auto* prevFn = currentFunction_;
    auto prevRetType = currentReturnType_;
    currentFunction_ = ctorSym;
    currentReturnType_ = registry_.getVoid(); // Constructors don't return explicitly

    enterNamedScope(ctorSym->bodyScope);

    // Type-check super constructor arguments
    if (node.hasSuperCall) {
        for (auto& arg : node.superArgs) {
            if (arg) arg->accept(*this);
        }
        auto* classSym = currentType_ ? currentType_->as<ClassSymbol>() : nullptr;
        if (classSym && classSym->baseClass && classSym->baseClass->constructor) {
            auto* baseCtor = classSym->baseClass->constructor;
            if (node.superArgs.size() != baseCtor->parameters.size()) {
                errors_.error(node.location,
                    "super() expects " + std::to_string(baseCtor->parameters.size()) +
                    " arguments, got " + std::to_string(node.superArgs.size()));
            }
            size_t count = std::min(node.superArgs.size(), baseCtor->parameters.size());
            for (size_t i = 0; i < count; ++i) {
                if (node.superArgs[i] && node.superArgs[i]->resolvedType &&
                    baseCtor->parameters[i]->type) {
                    checkAssignability(
                        node.superArgs[i]->resolvedType.get(),
                        baseCtor->parameters[i]->type.get(),
                        node.superArgs[i]->location,
                        "super argument " + std::to_string(i + 1));
                }
            }
        }
    }

    visitStatements(node.body->statements);
    leaveNamedScope();

    currentFunction_ = prevFn;
    currentReturnType_ = prevRetType;
}

void TypeChecker::visit(DestructorDeclaration& node) {
    auto* sym = currentScope_->lookupLocal("destructor");
    if (!sym || !sym->is<DestructorSymbol>()) return;
    auto* dtorSym = sym->as<DestructorSymbol>();

    if (!node.body) return;

    auto* prevFn = currentFunction_;
    auto prevRetType = currentReturnType_;
    currentFunction_ = dtorSym;
    currentReturnType_ = registry_.getVoid();

    enterNamedScope(dtorSym->bodyScope);
    visitStatements(node.body->statements);
    leaveNamedScope();

    currentFunction_ = prevFn;
    currentReturnType_ = prevRetType;
}

void TypeChecker::visit(OperatorDeclaration& node) {
    auto overloadOp = operatorKindToOverloadableOp(node.op);
    auto* opSym = currentScope_->lookupOperator(overloadOp);
    if (!opSym || !node.body) return;

    auto* prevFn = currentFunction_;
    auto prevRetType = currentReturnType_;
    currentFunction_ = nullptr;
    currentReturnType_ = opSym->returnType;

    enterNamedScope(opSym->bodyScope);
    visitStatements(node.body->statements);
    leaveNamedScope();

    currentFunction_ = prevFn;
    currentReturnType_ = prevRetType;
}

void TypeChecker::visit(ExternFunctionDeclaration& /*node*/) {
    // Types already resolved in Pass 2 — nothing to check
}

void TypeChecker::visit(EnumDeclaration& /*node*/) {}
void TypeChecker::visit(EnumMemberNode& /*node*/) {}
void TypeChecker::visit(ParameterNode& /*node*/) {}

//================================================================================
// Variable Declarations (local + inference)
//================================================================================
void TypeChecker::visit(VariableDeclaration& node) {
    auto* sym = currentScope_->lookupLocal(node.name);
    if (!sym || !sym->is<VariableSymbol>()) return;
    auto* varSym = sym->as<VariableSymbol>();

    if (node.isInferred) {
        // Type inference: resolve initializer, set variable type from it
        if (node.initializer) {
            node.initializer->accept(*this);
            auto initType = node.initializer->resolvedType;
            if (initType && !initType->is<ErrorType>()) {
                if (initType->is<NullType>()) {
                    errors_.error(node.location,
                        "cannot infer type from 'null' literal");
                    varSym->type = registry_.getErrorType();
                } else if (isVoidType(initType.get())) {
                    errors_.error(node.location,
                        "cannot declare variable of type 'void'");
                    varSym->type = registry_.getErrorType();
                } else {
                    varSym->type = initType;
                }
            } else {
                varSym->type = registry_.getErrorType();
            }
        } else {
            errors_.error(node.location,
                "type-inferred variable must have an initializer");
            varSym->type = registry_.getErrorType();
        }
    } else {
        // Explicit type — resolve if not already done by Pass 2
        if (!varSym->type && node.type) {
            varSym->type = resolveTypeNode(node.type.get());
        }
        // Check initializer compatibility
        if (node.initializer) {
            node.initializer->accept(*this);
            if (node.initializer->resolvedType && varSym->type) {
                checkAssignability(node.initializer->resolvedType.get(),
                                   varSym->type.get(),
                                   node.location,
                                   "initializer for '" + node.name + "'");
            }
        }
    }
}

void TypeChecker::visit(TupleDestructuringDeclaration& node) {
    if (node.initializer) {
        node.initializer->accept(*this);
    }

    auto initType = node.initializer ? node.initializer->resolvedType : nullptr;
    auto* tupleType = initType ? initType->as<TupleType>() : nullptr;

    if (!tupleType && initType && !initType->is<ErrorType>()) {
        errors_.error(node.location,
            "tuple destructuring requires a tuple expression, got '" +
            initType->toString() + "'");
    }

    for (size_t i = 0; i < node.elements.size(); ++i) {
        auto* sym = currentScope_->lookupLocal(node.elements[i].name);
        if (!sym || !sym->is<VariableSymbol>()) continue;
        auto* varSym = sym->as<VariableSymbol>();

        if (tupleType && i < tupleType->elementTypes.size()) {
            if (node.elements[i].isInferred) {
                varSym->type = tupleType->elementTypes[i];
            } else if (node.elements[i].type) {
                varSym->type = resolveTypeNode(node.elements[i].type.get());
                checkAssignability(tupleType->elementTypes[i].get(),
                                   varSym->type.get(),
                                   node.location);
            }
        } else if (tupleType) {
            errors_.error(node.location, "too many variables in tuple destructuring");
        } else {
            varSym->type = registry_.getErrorType();
        }
    }

    if (tupleType && node.elements.size() < tupleType->elementTypes.size()) {
        errors_.error(node.location, "too few variables in tuple destructuring");
    }
}

//================================================================================
// Statements
//================================================================================
void TypeChecker::visit(BlockStatement& node) {
    enterNextChildScope();
    visitStatements(node.statements);
    leaveChildScope();
}

void TypeChecker::visit(ExpressionStatement& node) {
    if (node.expression) node.expression->accept(*this);
}

void TypeChecker::visit(ReturnStatement& node) {
    if (node.value) {
        node.value->accept(*this);
        if (!currentReturnType_ && node.value->resolvedType) {
            // Lambda return type inference — set from first return statement
            currentReturnType_ = node.value->resolvedType;
        } else if (currentReturnType_ && node.value->resolvedType) {
            if (isVoidType(currentReturnType_.get())) {
                errors_.error(node.location,
                    "void function should not return a value");
            } else {
                checkAssignability(node.value->resolvedType.get(),
                                   currentReturnType_.get(),
                                   node.location,
                                   "return value");
            }
        }
    } else {
        if (currentReturnType_ && !isVoidType(currentReturnType_.get())) {
            errors_.error(node.location,
                "non-void function must return a value");
        }
    }
}

void TypeChecker::visit(IfStatement& node) {
    if (node.condition) {
        node.condition->accept(*this);
        if (node.condition->resolvedType &&
            !registry_.isCompatible(node.condition->resolvedType.get(),
                                     registry_.getBool().get())) {
            errors_.error(node.location,
                "if condition must be 'bool', got '" +
                node.condition->resolvedType->toString() + "'");
        }
    }
    if (node.thenBody) node.thenBody->accept(*this);

    for (auto& elseIf : node.elseIfClauses) {
        if (elseIf.condition) {
            elseIf.condition->accept(*this);
            if (elseIf.condition->resolvedType &&
                !registry_.isCompatible(elseIf.condition->resolvedType.get(),
                                         registry_.getBool().get())) {
                errors_.error(elseIf.condition->location,
                    "else-if condition must be 'bool'");
            }
        }
        if (elseIf.body) elseIf.body->accept(*this);
    }

    if (node.elseBody) node.elseBody->accept(*this);
}

void TypeChecker::visit(SwitchStatement& node) {
    if (node.subject) node.subject->accept(*this);

    for (auto& switchCase : node.cases) {
        if (switchCase.value) {
            switchCase.value->accept(*this);
            if (node.subject && node.subject->resolvedType &&
                switchCase.value->resolvedType) {
                checkAssignability(switchCase.value->resolvedType.get(),
                                   node.subject->resolvedType.get(),
                                   switchCase.value->location,
                                   "switch case");
            }
        }
        visitStatements(switchCase.body);
    }
    visitStatements(node.defaultCase);
}

void TypeChecker::visit(ForStatement& node) {
    enterNextChildScope();

    if (node.initDeclaration) node.initDeclaration->accept(*this);
    for (auto& expr : node.initExpressions) {
        if (expr) expr->accept(*this);
    }
    if (node.condition) {
        node.condition->accept(*this);
        if (node.condition->resolvedType &&
            !registry_.isCompatible(node.condition->resolvedType.get(),
                                     registry_.getBool().get())) {
            errors_.error(node.location, "for condition must be 'bool'");
        }
    }
    for (auto& iter : node.iterators) {
        if (iter) iter->accept(*this);
    }
    if (node.body) node.body->accept(*this);

    leaveChildScope();
}

void TypeChecker::visit(WhileStatement& node) {
    if (node.condition) {
        node.condition->accept(*this);
        if (node.condition->resolvedType &&
            !registry_.isCompatible(node.condition->resolvedType.get(),
                                     registry_.getBool().get())) {
            errors_.error(node.location, "while condition must be 'bool'");
        }
    }
    if (node.body) node.body->accept(*this);
}

void TypeChecker::visit(BreakStatement& /*node*/) {}
void TypeChecker::visit(ContinueStatement& /*node*/) {}

void TypeChecker::visit(DeleteStatement& node) {
    if (node.target) {
        node.target->accept(*this);
        if (node.target->resolvedType &&
            !node.target->resolvedType->is<PointerType>() &&
            !node.target->resolvedType->is<ErrorType>()) {
            errors_.error(node.location,
                "delete requires a pointer type, got '" +
                node.target->resolvedType->toString() + "'");
        }
    }
}

void TypeChecker::visit(RawBlock& node) {
    enterNextChildScope();
    if (node.body) {
        visitStatements(node.body->statements);
    }
    leaveChildScope();
}

//================================================================================
// Expression Visitors — Literals
//================================================================================
void TypeChecker::visit(IntegerLiteral& node) {
    node.resolvedType = registry_.getInt();
}

void TypeChecker::visit(FloatLiteral& node) {
    node.resolvedType = registry_.getDouble();
}

void TypeChecker::visit(BoolLiteral& node) {
    node.resolvedType = registry_.getBool();
}

void TypeChecker::visit(CharLiteral& node) {
    node.resolvedType = registry_.getChar();
}

void TypeChecker::visit(StringLiteral& node) {
    node.resolvedType = registry_.getString();
}

void TypeChecker::visit(InterpolatedString& node) {
    for (auto& part : node.parts) {
        if (part.kind == InterpolatedPartKind::Expression && part.expression) {
            part.expression->accept(*this);
        }
    }
    node.resolvedType = registry_.getString();
}

void TypeChecker::visit(NullLiteral& node) {
    node.resolvedType = registry_.getNullType();
}

//================================================================================
// Expression Visitors — Identifier Resolution
//================================================================================
void TypeChecker::visit(IdentifierExpression& node) {
    auto* sym = currentScope_->lookup(node.name);
    if (!sym) {
        errors_.error(node.location,
            "undeclared identifier '" + node.name + "'");
        node.resolvedType = registry_.getErrorType();
        return;
    }
    node.resolvedSymbol = sym;
    node.resolvedType = getSymbolType(sym);
}

void TypeChecker::visit(QualifiedNameExpression& node) {
    Symbol* sym = nullptr;

    if (!node.qualifiedName.parts.empty()) {
        sym = currentScope_->lookup(node.qualifiedName.parts[0]);
        for (size_t i = 1; i < node.qualifiedName.parts.size() && sym; ++i) {
            Scope* innerScope = nullptr;
            if (auto* mod = sym->as<ModuleSymbol>()) {
                innerScope = mod->moduleScope;
            } else if (auto* ts = sym->as<TypeSymbol>()) {
                // Handle enum member access (enums don't have a memberScope)
                if (auto* enumSym = ts->as<EnumSymbol>()) {
                    const auto& memberName = node.qualifiedName.parts[i];
                    bool found = false;
                    for (const auto& member : enumSym->members) {
                        if (member.name == memberName) {
                            found = true;
                            break;
                        }
                    }
                    if (found) {
                        node.resolvedSymbol = enumSym;
                        node.resolvedType = registry_.getUserType(
                            enumSym->name, Type::Kind::Enum, enumSym);
                        return;
                    } else {
                        errors_.error(node.location,
                            "'" + enumSym->name + "' has no member '" + memberName + "'");
                        node.resolvedType = registry_.getErrorType();
                        return;
                    }
                }
                innerScope = ts->memberScope;
            }
            if (!innerScope) {
                errors_.error(node.location,
                    "'" + sym->name + "' does not have members");
                node.resolvedType = registry_.getErrorType();
                return;
            }
            sym = innerScope->lookupLocal(node.qualifiedName.parts[i]);
        }
    }

    if (!sym) {
        errors_.error(node.location,
            "undeclared name '" + node.getName() + "'");
        node.resolvedType = registry_.getErrorType();
        return;
    }
    node.resolvedSymbol = sym;
    node.resolvedType = getSymbolType(sym);
}

void TypeChecker::visit(ThisExpression& node) {
    if (!currentType_) {
        errors_.error(node.location,
            "'this' used outside of a struct or class");
        node.resolvedType = registry_.getErrorType();
        return;
    }
    Type::Kind kind = (currentType_->kind == SymbolKind::Class)
        ? Type::Kind::Class : Type::Kind::Struct;
    // 'this' resolves to the type itself (codegen treats it as a pointer under the hood)
    node.resolvedType = registry_.getUserType(currentType_->name, kind, currentType_);
}

//================================================================================
// Expression Visitors — Member Access
//================================================================================
void TypeChecker::visit(MemberAccessExpression& node) {
    if (node.object) node.object->accept(*this);
    auto objType = node.object ? node.object->resolvedType : nullptr;
    if (!objType || objType->is<ErrorType>()) {
        node.resolvedType = registry_.getErrorType();
        return;
    }

    // If arrow access, dereference the pointer first
    const Type* baseType = objType.get();
    if (node.isArrow) {
        if (auto* ptr = baseType->as<PointerType>()) {
            baseType = ptr->baseType.get();
        } else {
            errors_.error(node.location,
                "'->' requires a pointer type, got '" + objType->toString() + "'");
            node.resolvedType = registry_.getErrorType();
            return;
        }
    }

    // Auto-dereference pointers for dot access (e.g., this.x)
    if (!node.isArrow) {
        if (auto* ptr = baseType->as<PointerType>()) {
            baseType = ptr->baseType.get();
        }
    }

    // String built-in methods: length(), charAt(i), substring(start, len)
    if (auto* prim = baseType->as<PrimitiveType>()) {
        if (prim->kind == PrimitiveType::PrimitiveKind::String) {
            if (node.memberName == "length") {
                node.isStringBuiltinMethod = true;
                node.resolvedType = registry_.getPrimitive(PrimitiveType::PrimitiveKind::Int);
                return;
            } else if (node.memberName == "charAt") {
                node.isStringBuiltinMethod = true;
                node.resolvedType = registry_.getPrimitive(PrimitiveType::PrimitiveKind::Char);
                return;
            } else if (node.memberName == "substring") {
                node.isStringBuiltinMethod = true;
                node.resolvedType = registry_.getPrimitive(PrimitiveType::PrimitiveKind::String);
                return;
            } else {
                errors_.error(node.location,
                    "string has no method '" + node.memberName + "'");
                node.resolvedType = registry_.getErrorType();
                return;
            }
        }
    }

    auto* userType = baseType->as<UserType>();
    if (!userType || !userType->symbol) {
        errors_.error(node.location,
            "'" + baseType->toString() + "' has no members");
        node.resolvedType = registry_.getErrorType();
        return;
    }

    auto* typeSym = static_cast<TypeSymbol*>(userType->symbol);

    // Check for enum member access (e.g., Color.Red)
    if (auto* enumSym = typeSym->as<EnumSymbol>()) {
        for (const auto& member : enumSym->members) {
            if (member.name == node.memberName) {
                node.isEnumAccess = true;
                if (member.isString) {
                    node.isStringEnumAccess = true;
                    node.resolvedEnumStringValue = member.stringValue;
                } else {
                    node.resolvedEnumValue = member.value;
                }
                node.resolvedType = registry_.getUserType(
                    enumSym->name, Type::Kind::Enum, enumSym);
                return;
            }
        }
        errors_.error(node.location,
            "'" + enumSym->name + "' has no member '" + node.memberName + "'");
        node.resolvedType = registry_.getErrorType();
        return;
    }

    // Check for field
    if (auto* field = typeSym->findField(node.memberName)) {
        node.resolvedType = field->type ? field->type : registry_.getErrorType();
        return;
    }

    // Check for method
    if (auto* method = typeSym->findMethod(node.memberName)) {
        node.resolvedType = getSymbolType(method);
        return;
    }

    errors_.error(node.location,
        "'" + typeSym->name + "' has no member '" + node.memberName + "'");
    node.resolvedType = registry_.getErrorType();
}

//================================================================================
// Expression Visitors — Binary, Unary, Assignment
//================================================================================
void TypeChecker::visit(BinaryExpression& node) {
    if (node.left) node.left->accept(*this);
    if (node.right) node.right->accept(*this);

    auto leftType = node.left ? node.left->resolvedType : nullptr;
    auto rightType = node.right ? node.right->resolvedType : nullptr;

    if (!leftType || !rightType ||
        leftType->is<ErrorType>() || rightType->is<ErrorType>()) {
        node.resolvedType = registry_.getErrorType();
        return;
    }

    // Check for operator overload on user types
    if (leftType->is<UserType>()) {
        auto overloadOp = binaryOpToOverloadableOp(node.op);
        if (overloadOp.has_value()) {
            auto* ut = leftType->as<UserType>();
            auto* typeSym = static_cast<TypeSymbol*>(ut->symbol);
            if (auto* opSym = typeSym->findOperator(overloadOp.value())) {
                node.isOperatorOverload = true;
                node.resolvedOperatorFunction = opSym;
                node.resolvedType = opSym->returnType
                    ? opSym->returnType : registry_.getErrorType();
                return;
            }
        }
    }

    // Arithmetic operators
    if (binaryOpIsArithmetic(node.op)) {
        bool leftIsPointer = leftType->is<PointerType>();
        bool rightIsPointer = rightType->is<PointerType>();
        bool leftIsInteger = registry_.isIntegerType(leftType.get());
        bool rightIsInteger = registry_.isIntegerType(rightType.get());

        // String concatenation: string + string → string
        auto isStringType = [](const Type* t) {
            auto* p = t->as<PrimitiveType>();
            return p && p->kind == PrimitiveType::PrimitiveKind::String;
        };
        if (isStringType(leftType.get()) && isStringType(rightType.get()) &&
            node.op == BinaryOp::Add) {
            node.resolvedType = leftType;
            return;
        }

        // Pointer arithmetic: ptr + int, ptr - int → ptr
        if (leftIsPointer && rightIsInteger &&
            (node.op == BinaryOp::Add || node.op == BinaryOp::Sub)) {
            node.resolvedType = leftType;
            return;
        }
        // int + ptr → ptr
        if (rightIsPointer && leftIsInteger && node.op == BinaryOp::Add) {
            node.resolvedType = rightType;
            return;
        }
        // ptr - ptr → int (pointer difference)
        if (leftIsPointer && rightIsPointer && node.op == BinaryOp::Sub) {
            node.resolvedType = registry_.getInt();
            return;
        }

        if (!registry_.isNumericType(leftType.get()) ||
            !registry_.isNumericType(rightType.get())) {
            errors_.error(node.location,
                "arithmetic operator requires numeric types, got '" +
                leftType->toString() + "' and '" + rightType->toString() + "'");
            node.resolvedType = registry_.getErrorType();
        } else {
            auto wider = registry_.getWiderType(leftType.get(), rightType.get());
            node.resolvedType = wider ? wider : leftType;
        }
        return;
    }

    // Comparison operators
    if (binaryOpIsComparison(node.op)) {
        if (!registry_.isCompatible(leftType.get(), rightType.get()) &&
            !registry_.isCompatible(rightType.get(), leftType.get())) {
            errors_.error(node.location,
                "cannot compare '" + leftType->toString() +
                "' and '" + rightType->toString() + "'");
        }
        node.resolvedType = registry_.getBool();
        return;
    }

    // Logical operators (&&, ||)
    if (binaryOpIsLogical(node.op)) {
        if (!registry_.isCompatible(leftType.get(), registry_.getBool().get())) {
            errors_.error(node.location,
                "logical operator requires 'bool' operands");
        }
        if (!registry_.isCompatible(rightType.get(), registry_.getBool().get())) {
            errors_.error(node.location,
                "logical operator requires 'bool' operands");
        }
        node.resolvedType = registry_.getBool();
        return;
    }

    // Bitwise operators
    if (binaryOpIsBitwise(node.op)) {
        if (!registry_.isIntegerType(leftType.get()) ||
            !registry_.isIntegerType(rightType.get())) {
            errors_.error(node.location,
                "bitwise operator requires integer types");
        }
        auto wider = registry_.getWiderType(leftType.get(), rightType.get());
        node.resolvedType = wider ? wider : leftType;
        return;
    }

    node.resolvedType = registry_.getErrorType();
}

void TypeChecker::visit(UnaryExpression& node) {
    if (node.operand) node.operand->accept(*this);
    auto operandType = node.operand ? node.operand->resolvedType : nullptr;
    if (!operandType || operandType->is<ErrorType>()) {
        node.resolvedType = registry_.getErrorType();
        return;
    }

    switch (node.op) {
        case UnaryOp::Negate:
            if (!registry_.isNumericType(operandType.get())) {
                errors_.error(node.location,
                    "unary '-' requires a numeric type");
                node.resolvedType = registry_.getErrorType();
            } else {
                node.resolvedType = operandType;
            }
            break;

        case UnaryOp::LogicalNot:
            if (!registry_.isCompatible(operandType.get(), registry_.getBool().get())) {
                errors_.error(node.location, "'!' requires a 'bool' operand");
            }
            node.resolvedType = registry_.getBool();
            break;

        case UnaryOp::BitwiseNot:
            if (!registry_.isIntegerType(operandType.get())) {
                errors_.error(node.location, "'~' requires an integer type");
            }
            node.resolvedType = operandType;
            break;

        case UnaryOp::AddressOf:
            node.resolvedType = registry_.getPointerTo(operandType);
            break;

        case UnaryOp::Dereference:
            if (auto* ptr = operandType->as<PointerType>()) {
                node.resolvedType = ptr->baseType;
            } else {
                errors_.error(node.location,
                    "dereference requires a pointer type, got '" +
                    operandType->toString() + "'");
                node.resolvedType = registry_.getErrorType();
            }
            break;

        case UnaryOp::PreIncrement:
        case UnaryOp::PreDecrement:
        case UnaryOp::PostIncrement:
        case UnaryOp::PostDecrement:
            if (!registry_.isNumericType(operandType.get())) {
                errors_.error(node.location,
                    "increment/decrement requires a numeric type");
                node.resolvedType = registry_.getErrorType();
            } else {
                node.resolvedType = operandType;
            }
            break;
    }
}

void TypeChecker::visit(AssignmentExpression& node) {
    if (node.target) node.target->accept(*this);
    if (node.value) node.value->accept(*this);

    if (!isLValue(node.target.get())) {
        errors_.error(node.location,
            "left side of assignment is not an lvalue");
    }

    auto targetType = node.target ? node.target->resolvedType : nullptr;
    auto valueType = node.value ? node.value->resolvedType : nullptr;

    if (targetType && valueType &&
        !targetType->is<ErrorType>() && !valueType->is<ErrorType>()) {
        if (assignOpIsCompound(node.op)) {
            auto binaryOp = assignOpToBinaryOp(node.op);
            if (binaryOpIsArithmetic(binaryOp)) {
                // Allow string += string
                auto isStringType = [](const Type* t) {
                    auto* p = t->as<PrimitiveType>();
                    return p && p->kind == PrimitiveType::PrimitiveKind::String;
                };
                bool isStringConcat = (node.op == AssignOp::AddAssign) &&
                    isStringType(targetType.get()) && isStringType(valueType.get());
                if (!isStringConcat &&
                    (!registry_.isNumericType(targetType.get()) ||
                     !registry_.isNumericType(valueType.get()))) {
                    errors_.error(node.location,
                        "compound assignment requires numeric types");
                }
            }
        } else {
            checkAssignability(valueType.get(), targetType.get(),
                               node.location, "assignment");
        }
    }

    node.resolvedType = targetType ? targetType : registry_.getErrorType();
}

//================================================================================
// Expression Visitors — Ternary, Call, New, Index, Cast, SizeOf
//================================================================================
void TypeChecker::visit(TernaryExpression& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.thenExpr) node.thenExpr->accept(*this);
    if (node.elseExpr) node.elseExpr->accept(*this);

    if (node.condition && node.condition->resolvedType) {
        if (!registry_.isCompatible(node.condition->resolvedType.get(),
                                     registry_.getBool().get())) {
            errors_.error(node.location, "ternary condition must be 'bool'");
        }
    }

    auto thenType = node.thenExpr ? node.thenExpr->resolvedType : nullptr;
    auto elseType = node.elseExpr ? node.elseExpr->resolvedType : nullptr;

    if (thenType && elseType) {
        auto wider = registry_.getWiderType(thenType.get(), elseType.get());
        if (wider) {
            node.resolvedType = wider;
        } else if (registry_.isCompatible(thenType.get(), elseType.get())) {
            node.resolvedType = elseType;
        } else if (registry_.isCompatible(elseType.get(), thenType.get())) {
            node.resolvedType = thenType;
        } else {
            errors_.error(node.location,
                "ternary branches have incompatible types: '" +
                thenType->toString() + "' and '" + elseType->toString() + "'");
            node.resolvedType = registry_.getErrorType();
        }
    } else {
        node.resolvedType = thenType ? thenType : (elseType ? elseType : registry_.getErrorType());
    }
}

void TypeChecker::visit(CallExpression& node) {
    if (node.callee) node.callee->accept(*this);
    for (auto& arg : node.arguments) {
        if (arg) arg->accept(*this);
    }

    // String built-in method calls: s.length(), s.charAt(i), s.substring(start, len)
    if (auto* memAccess = node.callee->as<MemberAccessExpression>()) {
        if (memAccess->isStringBuiltinMethod) {
            // The resolvedType was already set correctly on the MemberAccessExpression
            node.resolvedType = memAccess->resolvedType;
            return;
        }
    }

    auto calleeType = node.callee ? node.callee->resolvedType : nullptr;
    if (!calleeType || calleeType->is<ErrorType>()) {
        node.resolvedType = registry_.getErrorType();
        return;
    }

    // Handle constructor calls via type name (e.g., File(path, "r"))
    if (auto* userType = calleeType->as<UserType>()) {
        auto* typeSym = static_cast<TypeSymbol*>(userType->symbol);
        if (auto* classSym = typeSym->as<ClassSymbol>()) {
            if (classSym->isAbstract) {
                errors_.error(node.location,
                    "cannot instantiate abstract class '" + classSym->name + "'");
                node.resolvedType = registry_.getErrorType();
                return;
            }
            if (classSym->constructor) {
                auto* ctor = classSym->constructor;
                if (node.arguments.size() != ctor->parameters.size()) {
                    errors_.error(node.location,
                        "constructor expects " +
                        std::to_string(ctor->parameters.size()) +
                        " arguments, got " +
                        std::to_string(node.arguments.size()));
                }
                size_t count = std::min(node.arguments.size(),
                                        ctor->parameters.size());
                for (size_t i = 0; i < count; ++i) {
                    if (node.arguments[i] && node.arguments[i]->resolvedType &&
                        ctor->parameters[i]->type) {
                        checkAssignability(
                            node.arguments[i]->resolvedType.get(),
                            ctor->parameters[i]->type.get(),
                            node.arguments[i]->location,
                            "constructor argument " + std::to_string(i + 1));
                    }
                }
            }
            node.resolvedType = calleeType;
            return;
        }
        // Struct aggregate initialization
        if (typeSym->is<StructSymbol>()) {
            node.resolvedType = calleeType;
            return;
        }
    }

    auto* fnType = calleeType->as<FunctionType>();
    if (!fnType) {
        errors_.error(node.location,
            "expression is not callable (type: '" +
            calleeType->toString() + "')");
        node.resolvedType = registry_.getErrorType();
        return;
    }

    if (node.arguments.size() != fnType->parameterTypes.size()) {
        errors_.error(node.location,
            "expected " + std::to_string(fnType->parameterTypes.size()) +
            " arguments, got " + std::to_string(node.arguments.size()));
    }

    size_t count = std::min(node.arguments.size(), fnType->parameterTypes.size());
    for (size_t i = 0; i < count; ++i) {
        if (node.arguments[i] && node.arguments[i]->resolvedType) {
            checkAssignability(node.arguments[i]->resolvedType.get(),
                               fnType->parameterTypes[i].get(),
                               node.arguments[i]->location,
                               "argument " + std::to_string(i + 1));
        }
    }

    node.resolvedType = fnType->returnType;
}

void TypeChecker::visit(NewExpression& node) {
    auto allocType = resolveTypeNode(node.type.get());
    if (!allocType || allocType->is<ErrorType>()) {
        node.resolvedType = registry_.getErrorType();
        return;
    }

    if (node.isArray) {
        if (node.arraySize) {
            node.arraySize->accept(*this);
            if (node.arraySize->resolvedType &&
                !registry_.isIntegerType(node.arraySize->resolvedType.get())) {
                errors_.error(node.location, "array size must be an integer");
            }
        }
        node.resolvedType = registry_.getPointerTo(allocType);
    } else {
        for (auto& arg : node.arguments) {
            if (arg) arg->accept(*this);
        }

        // Look up the type's constructor if it's a class
        if (auto* userType = allocType->as<UserType>()) {
            auto* typeSym = static_cast<TypeSymbol*>(userType->symbol);
            if (auto* classSym = typeSym->as<ClassSymbol>()) {
                if (classSym->isAbstract) {
                    errors_.error(node.location,
                        "cannot instantiate abstract class '" + classSym->name + "' with 'new'");
                    node.resolvedType = registry_.getErrorType();
                    return;
                }
                if (classSym->constructor) {
                    auto* ctor = classSym->constructor;
                    if (node.arguments.size() != ctor->parameters.size()) {
                        errors_.error(node.location,
                            "constructor expects " +
                            std::to_string(ctor->parameters.size()) +
                            " arguments, got " +
                            std::to_string(node.arguments.size()));
                    }
                    size_t count = std::min(node.arguments.size(),
                                            ctor->parameters.size());
                    for (size_t i = 0; i < count; ++i) {
                        if (node.arguments[i] && node.arguments[i]->resolvedType &&
                            ctor->parameters[i]->type) {
                            checkAssignability(
                                node.arguments[i]->resolvedType.get(),
                                ctor->parameters[i]->type.get(),
                                node.arguments[i]->location,
                                "constructor argument " + std::to_string(i + 1));
                        }
                    }
                }
            }
        }
        node.resolvedType = registry_.getPointerTo(allocType);
    }
}

void TypeChecker::visit(IndexExpression& node) {
    if (node.object) node.object->accept(*this);
    if (node.index) node.index->accept(*this);

    auto objType = node.object ? node.object->resolvedType : nullptr;
    if (!objType || objType->is<ErrorType>()) {
        node.resolvedType = registry_.getErrorType();
        return;
    }

    auto idxType = node.index ? node.index->resolvedType : nullptr;

    // Array indexing
    if (auto* arr = objType->as<ArrayType>()) {
        if (idxType && !registry_.isIntegerType(idxType.get())) {
            errors_.error(node.location, "array index must be an integer");
        }
        node.resolvedType = arr->elementType;
        return;
    }

    // Pointer indexing
    if (auto* ptr = objType->as<PointerType>()) {
        if (idxType && !registry_.isIntegerType(idxType.get())) {
            errors_.error(node.location, "pointer index must be an integer");
        }
        node.resolvedType = ptr->baseType;
        return;
    }

    // String indexing: s[i] returns char
    if (auto* prim = objType->as<PrimitiveType>()) {
        if (prim->kind == PrimitiveType::PrimitiveKind::String) {
            if (idxType && !registry_.isIntegerType(idxType.get())) {
                errors_.error(node.location, "string index must be an integer");
            }
            node.resolvedType = registry_.getPrimitive(PrimitiveType::PrimitiveKind::Char);
            return;
        }
    }

    // Operator overload for user types
    if (auto* userType = objType->as<UserType>()) {
        auto* typeSym = static_cast<TypeSymbol*>(userType->symbol);
        if (auto* opSym = typeSym->findOperator(OverloadableOp::Index)) {
            node.isOperatorOverload = true;
            node.resolvedOperatorFunction = opSym;
            node.resolvedType = opSym->returnType
                ? opSym->returnType : registry_.getErrorType();
            return;
        }
    }

    errors_.error(node.location,
        "type '" + objType->toString() + "' does not support indexing");
    node.resolvedType = registry_.getErrorType();
}

void TypeChecker::visit(CastExpression& node) {
    if (node.operand) node.operand->accept(*this);
    auto targetType = resolveTypeNode(node.targetType.get());
    node.resolvedType = targetType ? targetType : registry_.getErrorType();

    auto srcType = node.operand ? node.operand->resolvedType : nullptr;
    if (srcType && targetType &&
        !srcType->is<ErrorType>() && !targetType->is<ErrorType>()) {
        bool legal = false;
        legal = legal || (registry_.isNumericType(srcType.get()) && registry_.isNumericType(targetType.get()));
        legal = legal || (srcType->is<PointerType>() && targetType->is<PointerType>());
        legal = legal || registry_.isCompatible(srcType.get(), targetType.get());
        legal = legal || (registry_.isIntegerType(srcType.get()) && targetType->is<PointerType>());
        legal = legal || (srcType->is<PointerType>() && registry_.isIntegerType(targetType.get()));
        if (!legal) {
            errors_.error(node.location,
                "invalid cast from '" + srcType->toString() +
                "' to '" + targetType->toString() + "'");
        }
    }
}

void TypeChecker::visit(SizeOfExpression& node) {
    resolveTypeNode(node.targetType.get());
    node.resolvedType = registry_.getInt();
}

void TypeChecker::visit(AlignOfExpression& node) {
    resolveTypeNode(node.targetType.get());
    node.resolvedType = registry_.getInt();
}

//================================================================================
// Expression Visitors — Pipe, Match, Tuple, Lambda
//================================================================================
void TypeChecker::visit(PipeExpression& node) {
    if (node.input) node.input->accept(*this);
    auto currentType = node.input ? node.input->resolvedType : nullptr;

    for (auto& stage : node.stages) {
        if (stage.function) stage.function->accept(*this);

        for (auto& arg : stage.extraArguments) {
            if (arg) arg->accept(*this);
        }

        auto fnType = stage.function ? stage.function->resolvedType : nullptr;
        if (fnType && fnType->is<FunctionType>()) {
            auto* ft = fnType->as<FunctionType>();
            size_t expectedParams = 1 + stage.extraArguments.size();
            if (ft->parameterTypes.size() != expectedParams) {
                errors_.error(stage.function->location,
                    "pipe stage expects " +
                    std::to_string(ft->parameterTypes.size()) +
                    " parameters, got " + std::to_string(expectedParams) +
                    " arguments");
            }
            // Check piped input type
            if (!ft->parameterTypes.empty() && currentType) {
                checkAssignability(currentType.get(),
                                   ft->parameterTypes[0].get(),
                                   stage.function->location,
                                   "pipe input");
            }
            // Check extra args
            for (size_t i = 0; i < stage.extraArguments.size(); ++i) {
                size_t paramIdx = i + 1;
                if (paramIdx < ft->parameterTypes.size() &&
                    stage.extraArguments[i] &&
                    stage.extraArguments[i]->resolvedType) {
                    checkAssignability(
                        stage.extraArguments[i]->resolvedType.get(),
                        ft->parameterTypes[paramIdx].get(),
                        stage.extraArguments[i]->location,
                        "pipe argument " + std::to_string(i + 1));
                }
            }
            currentType = ft->returnType;
        } else {
            if (fnType && !fnType->is<ErrorType>()) {
                errors_.error(stage.function->location,
                    "pipe stage is not a function");
            }
            currentType = registry_.getErrorType();
        }
    }

    node.resolvedType = currentType ? currentType : registry_.getErrorType();
}

void TypeChecker::visit(MatchExpression& node) {
    if (node.subject) node.subject->accept(*this);

    auto prevMatchSubjectType = matchSubjectType_;
    matchSubjectType_ = node.subject ? node.subject->resolvedType : nullptr;

    TypePtr<Type> resultType = nullptr;

    for (auto& arm : node.arms) {
        enterNextChildScope();
        if (arm.pattern) arm.pattern->accept(*this);
        if (arm.body) arm.body->accept(*this);

        // Get the body's type if it's an expression
        TypePtr<Type> bodyType = nullptr;
        if (auto* expr = dynamic_cast<ExpressionNode*>(arm.body.get())) {
            bodyType = expr->resolvedType;
        }

        if (bodyType && !resultType) {
            resultType = bodyType;
        } else if (bodyType && resultType) {
            auto wider = registry_.getWiderType(resultType.get(), bodyType.get());
            if (wider) {
                resultType = wider;
            } else if (!registry_.isCompatible(bodyType.get(), resultType.get())) {
                errors_.error(arm.body->location,
                    "match arm type '" + bodyType->toString() +
                    "' is incompatible with '" + resultType->toString() + "'");
            }
        }
        leaveChildScope();
    }

    matchSubjectType_ = prevMatchSubjectType;
    node.resolvedType = resultType ? resultType : registry_.getVoid();
}

void TypeChecker::visit(TupleExpression& node) {
    TypeList<Type> elemTypes;
    for (auto& elem : node.elements) {
        if (elem) {
            elem->accept(*this);
            elemTypes.push_back(elem->resolvedType
                ? elem->resolvedType : registry_.getErrorType());
        }
    }
    node.resolvedType = registry_.getTupleOf(std::move(elemTypes));
}

void TypeChecker::visit(LambdaExpression& node) {
    enterNextChildScope();

    TypeList<Type> paramTypes;
    for (auto& param : node.parameters) {
        if (param && param->type) {
            auto pType = resolveTypeNode(param->type.get());
            paramTypes.push_back(pType ? pType : registry_.getErrorType());
            auto* paramSym = currentScope_->lookupLocal(param->name);
            if (paramSym && paramSym->is<VariableSymbol>()) {
                paramSym->as<VariableSymbol>()->type = pType;
            }
        } else {
            paramTypes.push_back(registry_.getErrorType());
        }
    }

    auto prevRetType = currentReturnType_;
    currentReturnType_ = nullptr;

    if (node.body) node.body->accept(*this);

    // Infer return type from body
    TypePtr<Type> returnType = registry_.getVoid();
    if (auto* expr = dynamic_cast<ExpressionNode*>(node.body.get())) {
        returnType = expr->resolvedType ? expr->resolvedType : registry_.getVoid();
    } else if (currentReturnType_) {
        // Block-bodied lambda — return type inferred from return statements
        returnType = currentReturnType_;
    }

    currentReturnType_ = prevRetType;
    leaveChildScope();

    node.resolvedType = registry_.getFunctionType(std::move(paramTypes), returnType);
}

//================================================================================
// Type nodes (no-op — resolved via resolveTypeNode helper)
//================================================================================
void TypeChecker::visit(TypeNode& /*node*/) {}
void TypeChecker::visit(PrimitiveTypeNode& /*node*/) {}
void TypeChecker::visit(NamedTypeNode& /*node*/) {}
void TypeChecker::visit(PointerTypeNode& /*node*/) {}
void TypeChecker::visit(ArrayTypeNode& /*node*/) {}
void TypeChecker::visit(TupleTypeNode& /*node*/) {}
void TypeChecker::visit(FunctionTypeNode& /*node*/) {}

//================================================================================
// Pattern Visitors
//================================================================================
void TypeChecker::visit(LiteralPattern& node) {
    if (node.value) node.value->accept(*this);
}

void TypeChecker::visit(RangePattern& /*node*/) {}
void TypeChecker::visit(WildcardPattern& /*node*/) {}

void TypeChecker::visit(BindingPattern& node) {
    // Infer binding variable type from the match subject
    if (matchSubjectType_) {
        auto* sym = currentScope_->lookupLocal(node.name);
        if (sym && sym->is<VariableSymbol>()) {
            sym->as<VariableSymbol>()->type = matchSubjectType_;
        }
    }
}

void TypeChecker::visit(TuplePattern& node) {
    for (auto& elem : node.elements) {
        if (elem) elem->accept(*this);
    }
}

void TypeChecker::visit(GuardedPattern& node) {
    if (node.innerPattern) node.innerPattern->accept(*this);
    if (node.guard) {
        node.guard->accept(*this);
        if (node.guard->resolvedType &&
            !registry_.isCompatible(node.guard->resolvedType.get(),
                                     registry_.getBool().get())) {
            errors_.error(node.guard->location,
                "guard condition must be 'bool'");
        }
    }
}

} // namespace sema
} // namespace mingus
