//================================================================================
// MINGUS v1 - Type Resolver Implementation (Pass 2)
// Walks declaration-level TypeNodes and converts them to concrete Type objects.
//================================================================================

#include "mingus/sema/TypeResolver.h"

namespace mingus {
namespace sema {

//================================================================================
// Constructor & Entry Point
//================================================================================
TypeResolver::TypeResolver(SymbolTable& table, TypeRegistry& registry, ErrorReporter& errors)
    : symbolTable_(table)
    , registry_(registry)
    , errors_(errors)
    , currentScope_(nullptr)
{
}

void TypeResolver::resolve(ProgramNode& program) {
    currentScope_ = symbolTable_.getGlobalScope();
    visit(program);
}

//================================================================================
// Scope Navigation
//================================================================================
void TypeResolver::enterScope(Scope* scope) {
    currentScope_ = scope;
}

void TypeResolver::leaveScope() {
    if (currentScope_->parent) {
        currentScope_ = currentScope_->parent;
    }
}

//================================================================================
// resolveTypeNode — core helper
//================================================================================
TypePtr<Type> TypeResolver::resolveTypeNode(TypeNode* node) {
    if (!node) return registry_.getErrorType();

    // Already resolved?
    if (node->resolvedType) return node->resolvedType;

    // PrimitiveTypeNode
    if (auto* prim = dynamic_cast<PrimitiveTypeNode*>(node)) {
        node->resolvedType = registry_.getPrimitive(prim->kind);
        return node->resolvedType;
    }

    // NamedTypeNode — lookup in scope chain, must resolve to a TypeSymbol
    if (auto* named = dynamic_cast<NamedTypeNode*>(node)) {
        Symbol* sym = nullptr;

        if (named->qualifiedName.size() == 1) {
            sym = currentScope_->lookup(named->qualifiedName[0]);
        } else {
            // Multi-part qualified name: navigate through modules/types
            sym = currentScope_->lookup(named->qualifiedName[0]);
            for (size_t i = 1; i < named->qualifiedName.size() && sym; ++i) {
                Scope* innerScope = nullptr;
                if (auto* mod = sym->as<ModuleSymbol>()) {
                    innerScope = mod->moduleScope;
                } else if (auto* ts = sym->as<TypeSymbol>()) {
                    innerScope = ts->memberScope;
                }
                if (!innerScope) {
                    errors_.error(node->location,
                        "'" + sym->name + "' does not have members");
                    node->resolvedType = registry_.getErrorType();
                    return node->resolvedType;
                }
                sym = innerScope->lookupLocal(named->qualifiedName[i]);
            }
        }

        if (!sym) {
            errors_.error(node->location,
                "unknown type '" + named->getName() + "'");
            node->resolvedType = registry_.getErrorType();
            return node->resolvedType;
        }

        auto* typeSym = sym->as<TypeSymbol>();
        if (!typeSym) {
            errors_.error(node->location,
                "'" + sym->name + "' is not a type");
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

    // PointerTypeNode
    if (auto* ptr = dynamic_cast<PointerTypeNode*>(node)) {
        auto base = resolveTypeNode(ptr->baseType.get());
        node->resolvedType = registry_.getPointerTo(base);
        return node->resolvedType;
    }

    // ArrayTypeNode
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

    // TupleTypeNode
    if (auto* tup = dynamic_cast<TupleTypeNode*>(node)) {
        TypeList<Type> elemTypes;
        for (auto& elemNode : tup->elementTypes) {
            elemTypes.push_back(resolveTypeNode(elemNode.get()));
        }
        node->resolvedType = registry_.getTupleOf(std::move(elemTypes));
        return node->resolvedType;
    }

    // FunctionTypeNode
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

    // Fallback
    errors_.error(node->location, "cannot resolve type");
    node->resolvedType = registry_.getErrorType();
    return node->resolvedType;
}

//================================================================================
// Helper: resolve parameter types
//================================================================================
void TypeResolver::resolveParameters(NodeList<ParameterNode>& params,
                                     std::vector<VariableSymbol*>& symbols) {
    for (size_t i = 0; i < params.size() && i < symbols.size(); ++i) {
        if (params[i] && params[i]->type) {
            symbols[i]->type = resolveTypeNode(params[i]->type.get());
        }
    }
}

//================================================================================
// Program Structure
//================================================================================
void TypeResolver::visit(ProgramNode& node) {
    for (auto& mod : node.modules) {
        if (mod) mod->accept(*this);
    }
}

void TypeResolver::visit(ModuleNode& node) {
    auto* sym = currentScope_->lookupLocal(node.name);
    if (!sym || !sym->is<ModuleSymbol>()) return;
    enterScope(sym->as<ModuleSymbol>()->moduleScope);

    for (auto& decl : node.declarations) {
        if (decl) decl->accept(*this);
    }

    leaveScope();
}

void TypeResolver::visit(ImportNode& /*node*/) {}

//================================================================================
// Declarations
//================================================================================
void TypeResolver::visit(StructDeclaration& node) {
    auto* sym = currentScope_->lookupLocal(node.name);
    if (!sym || !sym->is<StructSymbol>()) return;
    auto* structSym = sym->as<StructSymbol>();
    enterScope(structSym->memberScope);

    for (auto& field : node.fields) {
        if (field) field->accept(*this);
    }
    for (auto& method : node.methods) {
        if (method) method->accept(*this);
    }
    for (auto& op : node.operators) {
        if (op) op->accept(*this);
    }

    leaveScope();
}

void TypeResolver::visit(ClassDeclaration& node) {
    auto* sym = currentScope_->lookupLocal(node.name);
    if (!sym || !sym->is<ClassSymbol>()) return;
    auto* classSym = sym->as<ClassSymbol>();
    enterScope(classSym->memberScope);

    for (auto& field : node.fields) {
        if (field) field->accept(*this);
    }
    if (node.constructor) {
        node.constructor->accept(*this);
    }
    if (node.destructor) {
        node.destructor->accept(*this);
    }
    for (auto& method : node.methods) {
        if (method) method->accept(*this);
    }
    for (auto& op : node.operators) {
        if (op) op->accept(*this);
    }

    leaveScope();
}

void TypeResolver::visit(InterfaceDeclaration& node) {
    auto* sym = currentScope_->lookupLocal(node.name);
    if (!sym || !sym->is<InterfaceSymbol>()) return;
    auto* ifaceSym = sym->as<InterfaceSymbol>();
    enterScope(ifaceSym->memberScope);

    for (auto& method : node.methods) {
        if (method) method->accept(*this);
    }

    leaveScope();
}

void TypeResolver::visit(EnumDeclaration& node) {
    auto* sym = currentScope_->lookupLocal(node.name);
    if (!sym || !sym->is<EnumSymbol>()) return;
    auto* enumSym = sym->as<EnumSymbol>();

    if (node.underlyingType) {
        enumSym->underlyingType = resolveTypeNode(node.underlyingType.get());
    } else {
        enumSym->underlyingType = registry_.getInt();
    }
}

void TypeResolver::visit(VariableDeclaration& node) {
    auto* sym = currentScope_->lookupLocal(node.name);
    if (!sym || !sym->is<VariableSymbol>()) return;
    auto* varSym = sym->as<VariableSymbol>();

    if (!node.isInferred && node.type) {
        varSym->type = resolveTypeNode(node.type.get());
    }
    // Inferred types are resolved in Pass 3
}

void TypeResolver::visit(FunctionDeclaration& node) {
    auto* sym = currentScope_->lookupLocal(node.name);
    if (!sym || !sym->is<FunctionSymbol>()) return;
    auto* fnSym = sym->as<FunctionSymbol>();

    // Resolve return type
    fnSym->returnType = node.returnType
        ? resolveTypeNode(node.returnType.get())
        : registry_.getVoid();

    // Resolve parameter types
    resolveParameters(node.parameters, fnSym->parameters);

    // Do NOT enter function body — that is Pass 3's job
}

void TypeResolver::visit(ConstructorDeclaration& node) {
    auto* sym = currentScope_->lookupLocal("constructor");
    if (!sym || !sym->is<ConstructorSymbol>()) return;
    auto* ctorSym = sym->as<ConstructorSymbol>();

    // Constructor return type is implicitly the owning class
    if (ctorSym->owner && ctorSym->owner->is<TypeSymbol>()) {
        auto* ownerType = ctorSym->owner->as<TypeSymbol>();
        Type::Kind typeKind = (ownerType->kind == SymbolKind::Class)
            ? Type::Kind::Class : Type::Kind::Struct;
        ctorSym->returnType = registry_.getUserType(ownerType->name, typeKind, ownerType);
    }

    resolveParameters(node.parameters, ctorSym->parameters);
}

void TypeResolver::visit(DestructorDeclaration& /*node*/) {
    // Destructors have no parameters and return void — nothing to resolve
}

void TypeResolver::visit(OperatorDeclaration& node) {
    auto overloadOp = operatorKindToOverloadableOp(node.op);
    auto* opSym = currentScope_->lookupOperator(overloadOp);
    if (!opSym) return;

    opSym->returnType = node.returnType
        ? resolveTypeNode(node.returnType.get())
        : registry_.getVoid();

    resolveParameters(node.parameters, opSym->parameters);
}

void TypeResolver::visit(ExternFunctionDeclaration& node) {
    auto* sym = currentScope_->lookupLocal(node.name);
    if (!sym || !sym->is<FunctionSymbol>()) return;
    auto* fnSym = sym->as<FunctionSymbol>();

    fnSym->returnType = node.returnType
        ? resolveTypeNode(node.returnType.get())
        : registry_.getVoid();

    resolveParameters(node.parameters, fnSym->parameters);
}

void TypeResolver::visit(TupleDestructuringDeclaration& /*node*/) {}
void TypeResolver::visit(EnumMemberNode& /*node*/) {}
void TypeResolver::visit(ParameterNode& /*node*/) {}

//================================================================================
// Type nodes (no-op — resolved via resolveTypeNode helper)
//================================================================================
void TypeResolver::visit(TypeNode& /*node*/) {}
void TypeResolver::visit(PrimitiveTypeNode& /*node*/) {}
void TypeResolver::visit(NamedTypeNode& /*node*/) {}
void TypeResolver::visit(PointerTypeNode& /*node*/) {}
void TypeResolver::visit(ArrayTypeNode& /*node*/) {}
void TypeResolver::visit(TupleTypeNode& /*node*/) {}
void TypeResolver::visit(FunctionTypeNode& /*node*/) {}

//================================================================================
// Statements (no-op — Pass 2 does not enter function bodies)
//================================================================================
void TypeResolver::visit(BlockStatement& /*node*/) {}
void TypeResolver::visit(ExpressionStatement& /*node*/) {}
void TypeResolver::visit(ReturnStatement& /*node*/) {}
void TypeResolver::visit(IfStatement& /*node*/) {}
void TypeResolver::visit(SwitchStatement& /*node*/) {}
void TypeResolver::visit(ForStatement& /*node*/) {}
void TypeResolver::visit(WhileStatement& /*node*/) {}
void TypeResolver::visit(BreakStatement& /*node*/) {}
void TypeResolver::visit(ContinueStatement& /*node*/) {}
void TypeResolver::visit(DeleteStatement& /*node*/) {}
void TypeResolver::visit(RawBlock& /*node*/) {}

//================================================================================
// Expressions (no-op — Pass 2 does not resolve expressions)
//================================================================================
void TypeResolver::visit(IntegerLiteral& /*node*/) {}
void TypeResolver::visit(FloatLiteral& /*node*/) {}
void TypeResolver::visit(BoolLiteral& /*node*/) {}
void TypeResolver::visit(CharLiteral& /*node*/) {}
void TypeResolver::visit(StringLiteral& /*node*/) {}
void TypeResolver::visit(InterpolatedString& /*node*/) {}
void TypeResolver::visit(NullLiteral& /*node*/) {}
void TypeResolver::visit(IdentifierExpression& /*node*/) {}
void TypeResolver::visit(QualifiedNameExpression& /*node*/) {}
void TypeResolver::visit(MemberAccessExpression& /*node*/) {}
void TypeResolver::visit(ThisExpression& /*node*/) {}
void TypeResolver::visit(BinaryExpression& /*node*/) {}
void TypeResolver::visit(UnaryExpression& /*node*/) {}
void TypeResolver::visit(AssignmentExpression& /*node*/) {}
void TypeResolver::visit(TernaryExpression& /*node*/) {}
void TypeResolver::visit(CallExpression& /*node*/) {}
void TypeResolver::visit(NewExpression& /*node*/) {}
void TypeResolver::visit(IndexExpression& /*node*/) {}
void TypeResolver::visit(CastExpression& /*node*/) {}
void TypeResolver::visit(SizeOfExpression& /*node*/) {}
void TypeResolver::visit(AlignOfExpression& /*node*/) {}
void TypeResolver::visit(PipeExpression& /*node*/) {}
void TypeResolver::visit(MatchExpression& /*node*/) {}
void TypeResolver::visit(TupleExpression& /*node*/) {}
void TypeResolver::visit(LambdaExpression& /*node*/) {}

//================================================================================
// Patterns (no-op)
//================================================================================
void TypeResolver::visit(LiteralPattern& /*node*/) {}
void TypeResolver::visit(RangePattern& /*node*/) {}
void TypeResolver::visit(WildcardPattern& /*node*/) {}
void TypeResolver::visit(BindingPattern& /*node*/) {}
void TypeResolver::visit(TuplePattern& /*node*/) {}
void TypeResolver::visit(GuardedPattern& /*node*/) {}

} // namespace sema
} // namespace mingus
