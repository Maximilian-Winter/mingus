// ============================================================================
// TypeResolver.cpp — Pass 2: Resolve type annotations and infer types
//
// After Pass 1 builds the scope tree and creates all symbols,
// Pass 2 walks the AST and resolves:
//   - TypeNode annotations → TypeSymbol instances
//   - Variable types (declared and inferred)
//   - Function return types and parameter types
//   - Enum underlying types
//   - ReferenceType unwrapping (T& → base type + isReference flag)
// ============================================================================

#include "mingus/sema/TypeResolver.h"

namespace mingus {

TypeResolver::TypeResolver(SymbolTable& symbolTable, ErrorReporter& errors)
    : symbolTable_(symbolTable)
    , errors_(errors) {}

// ============================================================================
// Entry point
// ============================================================================

void TypeResolver::resolve(ProgramNode& program) {
    visit(program);
}

// ============================================================================
// Core type resolution
// ============================================================================

TypeSymbolPtr TypeResolver::resolveTypeNode(const std::shared_ptr<TypeNode>& typeNode) {
    if (!typeNode) return nullptr;
    return resolveTypeNode(typeNode.get());
}

TypeSymbolPtr TypeResolver::resolveTypeNode(TypeNode* typeNode) {
    if (!typeNode) return nullptr;

    // Already resolved?
    if (typeNode->resolvedType) return typeNode->resolvedType;

    // PrimitiveTypeNode → PrimitiveTypeSymbol
    if (auto* prim = typeNode->as<PrimitiveTypeNode>()) {
        switch (prim->kind) {
            case PrimitiveKind::Int:    prim->resolvedType = symbolTable_.getIntType();    break;
            case PrimitiveKind::Double: prim->resolvedType = symbolTable_.getDoubleType(); break;
            case PrimitiveKind::Float:  prim->resolvedType = symbolTable_.getFloatType();  break;
            case PrimitiveKind::Byte:   prim->resolvedType = symbolTable_.getByteType();   break;
            case PrimitiveKind::Char:   prim->resolvedType = symbolTable_.getCharType();   break;
            case PrimitiveKind::String: prim->resolvedType = symbolTable_.getStringType(); break;
            case PrimitiveKind::Bool:   prim->resolvedType = symbolTable_.getBoolType();   break;
            case PrimitiveKind::Void:   prim->resolvedType = symbolTable_.getVoidType();   break;
        }
        return prim->resolvedType;
    }

    // NamedTypeNode → scope lookup
    if (auto* named = typeNode->as<NamedTypeNode>()) {
        auto* scope = named->astScopeNode ? named->astScopeNode.get() : nullptr;
        named->resolvedType = resolveNamedType(named->qualifiedName, scope, named->debugInfo);
        return named->resolvedType;
    }

    // PointerTypeNode → PointerTypeSymbol, shared PointerTypeSymbol, or ReferenceTypeSymbol
    if (auto* ptr = typeNode->as<PointerTypeNode>()) {
        auto baseType = resolveTypeNode(ptr->baseType);
        if (!baseType) {
            ptr->resolvedType = symbolTable_.getErrorType();
        } else if (ptr->isReference) {
            ptr->resolvedType = symbolTable_.getReferenceType(baseType);
        } else if (ptr->isShared) {
            ptr->resolvedType = symbolTable_.getSharedPointerType(baseType);
        } else {
            ptr->resolvedType = symbolTable_.getPointerType(baseType);
        }
        return ptr->resolvedType;
    }

    // ArrayTypeNode → ArrayTypeSymbol
    if (auto* arr = typeNode->as<ArrayTypeNode>()) {
        auto elemType = resolveTypeNode(arr->elementType);
        if (!elemType) {
            arr->resolvedType = symbolTable_.getErrorType();
        } else {
            // Array size: for now use 0 for unsized arrays
            // Full constant evaluation would happen in a later pass
            int size = 0;
            if (arr->sizeExpr) {
                if (auto* intLit = arr->sizeExpr->as<IntegerLiteral>()) {
                    size = static_cast<int>(intLit->value);
                }
            }
            arr->resolvedType = symbolTable_.getArrayType(elemType, size);
        }
        return arr->resolvedType;
    }

    // TupleTypeNode → TupleTypeSymbol
    if (auto* tup = typeNode->as<TupleTypeNode>()) {
        std::vector<TypeSymbolPtr> elemTypes;
        for (auto& et : tup->elementTypes) {
            auto resolved = resolveTypeNode(et);
            elemTypes.push_back(resolved ? resolved : symbolTable_.getErrorType());
        }
        tup->resolvedType = symbolTable_.getTupleType(std::move(elemTypes));
        return tup->resolvedType;
    }

    // FunctionTypeNode → FunctionTypeSymbol
    if (auto* ft = typeNode->as<FunctionTypeNode>()) {
        std::vector<FunctionTypeSymbol::ParameterInfo> params;
        for (size_t i = 0; i < ft->parameterTypes.size(); i++) {
            auto paramType = resolveTypeNode(ft->parameterTypes[i]);
            FunctionTypeSymbol::ParameterInfo pi;
            pi.type = paramType ? paramType : symbolTable_.getErrorType();
            pi.name = "";  // Function type annotations don't carry param names
            pi.isReference = false;

            // If this parameter type is itself a reference, unwrap
            if (auto* refType = pi.type->as<ReferenceTypeSymbol>()) {
                pi.type = refType->baseType;
                pi.isReference = true;
            }
            params.push_back(pi);
        }
        auto retType = resolveTypeNode(ft->returnType);
        if (!retType) retType = symbolTable_.getVoidType();

        ft->resolvedType = symbolTable_.getFunctionType(std::move(params), retType);
        return ft->resolvedType;
    }

    // Fallback: error type
    errors_.error("unresolvable type annotation", typeNode->debugInfo);
    typeNode->resolvedType = symbolTable_.getErrorType();
    return typeNode->resolvedType;
}

TypeSymbolPtr TypeResolver::resolveNamedType(
    const std::vector<std::string>& qualifiedName,
    Scope* scope,
    const std::shared_ptr<DebugInfo>& loc)
{
    if (qualifiedName.empty()) {
        errors_.error("empty type name", loc);
        return symbolTable_.getErrorType();
    }

    // Single name: check type registry first, then scope chain
    if (qualifiedName.size() == 1) {
        const auto& name = qualifiedName[0];

        // Check SymbolTable type registry (primitives + registered user types)
        auto type = symbolTable_.resolveType(name);
        if (type) return type;

        // Fall back to scope resolution (for types not yet in registry)
        if (scope) {
            auto sym = scope->resolve(name);
            if (sym) {
                // Typedef: unwrap to the aliased type
                if (auto* aliasSym = sym->as<TypeAliasSymbol>()) {
                    if (aliasSym->aliasedType) return aliasSym->aliasedType;
                    errors_.error("typedef '" + name + "' has unresolved type", loc);
                    return symbolTable_.getErrorType();
                }
                if (auto* typeSym = sym->as<TypeSymbol>()) {
                    return std::dynamic_pointer_cast<TypeSymbol>(sym);
                }
                errors_.error("'" + name + "' is not a type", loc);
                return symbolTable_.getErrorType();
            }
        }

        errors_.error("unknown type '" + name + "'", loc);
        return symbolTable_.getErrorType();
    }

    // Qualified name: Module.TypeName
    // Resolve first part as module, then look up type within it
    if (scope) {
        auto moduleSym = scope->resolve(qualifiedName[0]);
        if (moduleSym) {
            auto* moduleScope = moduleSym->as<ModuleSymbol>();
            if (moduleScope) {
                auto typeSym = moduleScope->resolve(qualifiedName[1]);
                if (typeSym && typeSym->is<TypeSymbol>()) {
                    return std::dynamic_pointer_cast<TypeSymbol>(typeSym);
                }
                errors_.error("'" + qualifiedName[1] + "' not found in module '"
                    + qualifiedName[0] + "'", loc);
                return symbolTable_.getErrorType();
            }
        }
    }

    std::string fullName;
    for (size_t i = 0; i < qualifiedName.size(); i++) {
        if (i > 0) fullName += ".";
        fullName += qualifiedName[i];
    }
    errors_.error("unknown qualified type '" + fullName + "'", loc);
    return symbolTable_.getErrorType();
}

// ============================================================================
// Parameter resolution (the critical unwrapping logic)
// ============================================================================

void TypeResolver::resolveParameters(
    const std::vector<std::shared_ptr<ParameterNode>>& params,
    const std::shared_ptr<FunctionSymbol>& funcSym)
{
    for (size_t i = 0; i < params.size(); i++) {
        auto& param = params[i];
        if (!param || !param->resolvedSymbol) continue;

        auto* varSym = param->resolvedSymbol.get();

        if (param->type) {
            auto resolvedType = resolveTypeNode(param->type);
            if (resolvedType) {
                // CRITICAL: ReferenceType unwrapping
                // If the param type annotation is T& or T&&, store base type + isReference flag.
                // Without this, mapType(ReferenceType(int)) returns ptr instead of i32.
                if (auto* refType = resolvedType->as<ReferenceTypeSymbol>()) {
                    varSym->setType(refType->baseType);
                    varSym->isReference = true;
                    // Propagate rvalue reference flag from AST ParameterNode
                    if (param->isRvalueReference) {
                        varSym->isRvalueReference = true;
                    }
                } else {
                    varSym->setType(resolvedType);
                }
            } else {
                varSym->setType(symbolTable_.getErrorType());
            }
        }
        // If no type annotation (untyped param), leave type as nullptr
        // for inference by Pass 3 (TypeChecker).
    }
}

void TypeResolver::resolveReturnType(
    const std::shared_ptr<TypeNode>& returnTypeNode,
    const std::shared_ptr<FunctionSymbol>& funcSym)
{
    if (!funcSym) return;

    if (returnTypeNode) {
        auto resolved = resolveTypeNode(returnTypeNode);
        funcSym->returnType = resolved ? resolved : symbolTable_.getErrorType();
    } else {
        // No return type annotation → void
        funcSym->returnType = symbolTable_.getVoidType();
    }
}

// ============================================================================
// Helpers
// ============================================================================

void TypeResolver::visitStatements(
    std::vector<std::shared_ptr<StatementBaseNode>>& stmts)
{
    for (auto& stmt : stmts) {
        if (stmt) stmt->accept(*this);
    }
}

// ============================================================================
// Program & Module
// ============================================================================

void TypeResolver::visit(ProgramNode& node) {
    for (auto& module : node.modules) {
        if (module) module->accept(*this);
    }
}

void TypeResolver::visit(ModuleNode& node) {
    for (auto& decl : node.declarations) {
        if (decl) decl->accept(*this);
    }
}

void TypeResolver::visit(BlockStatementNode& node) {
    visitStatements(node.statements);
}

// ============================================================================
// Variable Declarations
// ============================================================================

void TypeResolver::visit(VariableDeclaration& node) {
    // Resolve type annotation if present
    if (node.type && node.resolvedVariable) {
        auto resolvedType = resolveTypeNode(node.type);
        if (resolvedType) {
            node.resolvedVariable->setType(resolvedType);
        }
    }

    // Walk initializer (may contain lambdas or nested var decls)
    if (node.initializer) {
        node.initializer->accept(*this);
    }

    // Type inference: if inferred and we don't have a type yet,
    // we leave it for Pass 3 (TypeChecker) to infer from the initializer.
    // Pass 2 only resolves explicit annotations.
}

void TypeResolver::visit(VariableDeclarationExpression& node) {
    if (node.type && node.resolvedVariable) {
        auto resolvedType = resolveTypeNode(node.type);
        if (resolvedType) {
            node.resolvedVariable->setType(resolvedType);
        }
    }

    if (node.initializer) {
        node.initializer->accept(*this);
    }
}

void TypeResolver::visit(TupleDestructuringDeclaration& node) {
    // Individual element types resolved when the tuple type is known
    // (during type checking, not type annotation resolution)
    if (node.initializer) {
        node.initializer->accept(*this);
    }
}

// ============================================================================
// Function Declarations
// ============================================================================

void TypeResolver::visit(FunctionDeclaration& node) {
    auto& funcSym = node.resolvedFunction;
    if (!funcSym) return;

    // Resolve parameter types
    resolveParameters(node.parameters, funcSym);

    // Resolve return type
    resolveReturnType(node.returnType, funcSym);

    // Walk body for nested declarations
    if (node.body) {
        node.body->accept(*this);
    }
}

void TypeResolver::visit(ConstructorDeclaration& node) {
    auto funcSym = std::dynamic_pointer_cast<FunctionSymbol>(node.resolvedConstructor);
    if (!funcSym) return;

    resolveParameters(node.parameters, funcSym);

    // Constructors return void
    funcSym->returnType = symbolTable_.getVoidType();

    if (node.body) {
        node.body->accept(*this);
    }
}

void TypeResolver::visit(DestructorDeclaration& node) {
    auto funcSym = std::dynamic_pointer_cast<FunctionSymbol>(node.resolvedDestructor);
    if (!funcSym) return;

    // Destructors return void, no parameters
    funcSym->returnType = symbolTable_.getVoidType();

    if (node.body) {
        node.body->accept(*this);
    }
}

void TypeResolver::visit(ExternFunctionDeclaration& node) {
    auto& funcSym = node.resolvedFunction;
    if (!funcSym) return;

    resolveParameters(node.parameters, funcSym);
    resolveReturnType(node.returnType, funcSym);
}

void TypeResolver::visit(OperatorDeclaration& node) {
    auto funcSym = std::dynamic_pointer_cast<FunctionSymbol>(node.resolvedOperator);
    if (!funcSym) return;

    resolveParameters(node.parameters, funcSym);
    resolveReturnType(node.returnType, funcSym);

    if (node.body) {
        node.body->accept(*this);
    }
}

// ============================================================================
// Type Declarations
// ============================================================================

void TypeResolver::visit(EnumDeclaration& node) {
    if (!node.resolvedEnum) return;

    // Resolve underlying type (default to int)
    if (node.underlyingType) {
        node.resolvedEnum->underlyingType = resolveTypeNode(node.underlyingType);
    } else {
        node.resolvedEnum->underlyingType = symbolTable_.getIntType();
    }
}

void TypeResolver::visit(StructDeclaration& node) {
    if (!node.resolvedStruct) return;

    // Resolve field types
    for (auto& field : node.fields) {
        if (field) field->accept(*this);
    }

    // Resolve method signatures
    for (auto& method : node.methods) {
        if (method) method->accept(*this);
    }

    // Resolve operator signatures
    for (auto& op : node.operators) {
        if (op) op->accept(*this);
    }
}

void TypeResolver::visit(ClassDeclaration& node) {
    if (!node.resolvedClass) return;

    // Resolve field types
    for (auto& field : node.fields) {
        if (field) field->accept(*this);
    }

    // Constructors
    for (auto& ctor : node.constructors) {
        if (ctor) ctor->accept(*this);
    }
    if (node.copyConstructor) {
        node.copyConstructor->accept(*this);
    }
    if (node.moveConstructor) {
        node.moveConstructor->accept(*this);
    }

    // Destructor
    if (node.destructor) {
        node.destructor->accept(*this);
    }

    // Methods
    for (auto& method : node.methods) {
        if (method) method->accept(*this);
    }

    // Operators
    for (auto& op : node.operators) {
        if (op) op->accept(*this);
    }
}

void TypeResolver::visit(InterfaceDeclaration& node) {
    if (!node.resolvedInterface) return;

    // Resolve method signatures (abstract — no bodies)
    for (auto& method : node.methods) {
        if (method) method->accept(*this);
    }
}

void TypeResolver::visit(ImportDeclaration& node) {
    // Nothing to resolve — imports handled in Pass 1
}

void TypeResolver::visit(ExternStructDeclaration& node) {
    if (!node.resolvedStruct) return;

    // Resolve field types from ParameterNode type annotations
    for (size_t i = 0; i < node.fields.size(); i++) {
        auto& param = node.fields[i];
        if (!param || !param->type) continue;

        auto resolvedType = resolveTypeNode(param->type);
        if (i < node.resolvedStruct->fields.size()) {
            node.resolvedStruct->fields[i]->setType(
                resolvedType ? resolvedType : symbolTable_.getErrorType());
        }
    }
}

void TypeResolver::visit(TypedefDeclaration& node) {
    if (!node.resolvedTypeAlias) return;

    // Resolve the underlying type
    if (node.underlyingType) {
        auto resolved = resolveTypeNode(node.underlyingType);
        node.resolvedTypeAlias->aliasedType = resolved
            ? resolved : symbolTable_.getErrorType();
    } else {
        errors_.error("typedef missing underlying type", node.debugInfo);
        node.resolvedTypeAlias->aliasedType = symbolTable_.getErrorType();
    }

    // Register the alias name in the type registry so resolveType(name) works
    if (node.resolvedTypeAlias->aliasedType &&
        !node.resolvedTypeAlias->aliasedType->is<ErrorTypeSymbol>()) {
        symbolTable_.registerType(node.aliasName,
                                   node.resolvedTypeAlias->aliasedType);
    }
}

// ============================================================================
// Statements — recurse for nested declarations
// ============================================================================

void TypeResolver::visit(ExpressionStatement& node) {
    if (node.expression) node.expression->accept(*this);
}

void TypeResolver::visit(ReturnStatement& node) {
    if (node.value) node.value->accept(*this);
}

void TypeResolver::visit(IfStatement& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.thenBody) node.thenBody->accept(*this);
    for (auto& elseIf : node.elseIfClauses) {
        if (elseIf.condition) elseIf.condition->accept(*this);
        if (elseIf.body) elseIf.body->accept(*this);
    }
    if (node.elseBody) node.elseBody->accept(*this);
}

void TypeResolver::visit(ForStatement& node) {
    for (auto& initDecl : node.initDeclarations) {
        if (initDecl) initDecl->accept(*this);
    }
    for (auto& initExpr : node.initExpressions) {
        if (initExpr) initExpr->accept(*this);
    }
    if (node.condition) node.condition->accept(*this);
    for (auto& iter : node.iterators) {
        if (iter) iter->accept(*this);
    }
    if (node.body) node.body->accept(*this);
}

void TypeResolver::visit(WhileStatement& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.body) node.body->accept(*this);
}

void TypeResolver::visit(DoWhileStatement& node) {
    if (node.body) node.body->accept(*this);
    if (node.condition) node.condition->accept(*this);
}

void TypeResolver::visit(LabeledStatement& node) {
    if (node.statement) node.statement->accept(*this);
}

void TypeResolver::visit(BreakStatement& node) {}
void TypeResolver::visit(ContinueStatement& node) {}

void TypeResolver::visit(DeleteStatement& node) {
    if (node.target) node.target->accept(*this);
}

void TypeResolver::visit(SwitchStatement& node) {
    if (node.subject) node.subject->accept(*this);
    for (auto& c : node.cases) {
        if (c.value) c.value->accept(*this);
        for (auto& stmt : c.body) {
            if (stmt) stmt->accept(*this);
        }
    }
    for (auto& stmt : node.defaultCase) {
        if (stmt) stmt->accept(*this);
    }
}

// ============================================================================
// Expressions — recurse for nested structures (lambdas, var-decl-exprs)
// Most expressions don't need type resolution in Pass 2.
// Their resolvedType is set by Pass 3 (TypeChecker).
// We just recurse to find nested declarations/lambdas.
// ============================================================================

void TypeResolver::visit(IntegerLiteral& node) {}
void TypeResolver::visit(FloatLiteral& node) {}
void TypeResolver::visit(BoolLiteral& node) {}
void TypeResolver::visit(CharLiteral& node) {}
void TypeResolver::visit(StringLiteral& node) {}
void TypeResolver::visit(NullLiteral& node) {}
void TypeResolver::visit(ThisExpression& node) {}

void TypeResolver::visit(InterpolatedStringExpression& node) {
    for (auto& part : node.parts) {
        if (part.kind == InterpolatedPartKind::Expression && part.expression) {
            part.expression->accept(*this);
        }
    }
}

void TypeResolver::visit(IdentifierExpression& node) {}
void TypeResolver::visit(QualifiedNameExpression& node) {}

void TypeResolver::visit(MemberAccessExpression& node) {
    if (node.object) node.object->accept(*this);
}

void TypeResolver::visit(BinaryExpression& node) {
    if (node.left) node.left->accept(*this);
    if (node.right) node.right->accept(*this);
}

void TypeResolver::visit(UnaryExpression& node) {
    if (node.operand) node.operand->accept(*this);
}

void TypeResolver::visit(AssignmentExpression& node) {
    if (node.target) node.target->accept(*this);
    if (node.value) node.value->accept(*this);
}

void TypeResolver::visit(TernaryExpression& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.thenExpr) node.thenExpr->accept(*this);
    if (node.elseExpr) node.elseExpr->accept(*this);
}

void TypeResolver::visit(IndexExpression& node) {
    if (node.object) node.object->accept(*this);
    if (node.index) node.index->accept(*this);
}

void TypeResolver::visit(CallExpression& node) {
    if (node.callee) node.callee->accept(*this);
    if (node.arguments) {
        for (auto& arg : node.arguments->expressions) {
            if (arg) arg->accept(*this);
        }
    }
}

void TypeResolver::visit(CastExpression& node) {
    // Resolve the target type annotation
    if (node.targetType) {
        resolveTypeNode(node.targetType);
    }
    if (node.operand) node.operand->accept(*this);
}

void TypeResolver::visit(NewExpression& node) {
    // Resolve the allocated type
    if (node.type) {
        resolveTypeNode(node.type);
    }
    if (node.arguments) {
        for (auto& arg : node.arguments->expressions) {
            if (arg) arg->accept(*this);
        }
    }
    if (node.arraySize) node.arraySize->accept(*this);
}

void TypeResolver::visit(SizeOfExpression& node) {
    if (node.targetType) {
        resolveTypeNode(node.targetType);
    }
}

void TypeResolver::visit(TupleExpression& node) {
    for (auto& elem : node.elements) {
        if (elem) elem->accept(*this);
    }
}

void TypeResolver::visit(MatchExpression& node) {
    if (node.subject) node.subject->accept(*this);
    for (auto& arm : node.arms) {
        if (arm.body) arm.body->accept(*this);
    }
}

void TypeResolver::visit(PipeExpression& node) {
    if (node.input) node.input->accept(*this);
    for (auto& stage : node.stages) {
        if (stage.function) stage.function->accept(*this);
        for (auto& arg : stage.extraArguments) {
            if (arg) arg->accept(*this);
        }
    }
}

void TypeResolver::visit(LambdaExpression& node) {
    // Resolve lambda parameter types
    for (auto& param : node.parameters) {
        if (param && param->resolvedSymbol && param->type) {
            auto resolvedType = resolveTypeNode(param->type);
            if (resolvedType) {
                auto* varSym = param->resolvedSymbol.get();
                // ReferenceType unwrapping for lambda params too
                if (auto* refType = resolvedType->as<ReferenceTypeSymbol>()) {
                    varSym->setType(refType->baseType);
                    varSym->isReference = true;
                } else {
                    varSym->setType(resolvedType);
                }
            }
        }
    }

    // Walk lambda body for nested declarations
    if (node.body) {
        node.body->accept(*this);
    }
}

} // namespace mingus
