// ============================================================================
// TypeChecker.cpp — Pass 3: Expression type inference and compatibility
//
// Bottom-up walk: visit children first, then infer this node's type.
// Every ExpressionBaseNode gets a resolvedType (ErrorType on failure).
// ============================================================================

#include "mingus/sema/TypeChecker.h"

namespace mingus {

TypeChecker::TypeChecker(SymbolTable& symbolTable, ErrorReporter& errors)
    : symbolTable_(symbolTable)
    , errors_(errors) {}

// ============================================================================
// Entry point
// ============================================================================

void TypeChecker::check(ProgramNode& program) {
    visit(program);
}

// ============================================================================
// Helpers
// ============================================================================

void TypeChecker::visitStatements(
    std::vector<std::shared_ptr<StatementBaseNode>>& stmts)
{
    for (auto& stmt : stmts) {
        if (stmt) stmt->accept(*this);
    }
}

void TypeChecker::visitFunctionBody(
    const std::shared_ptr<FunctionSymbol>& funcSym,
    BlockStatementNode& body)
{
    auto savedFunc = currentFunction_;
    auto savedReturn = currentReturnType_;
    currentFunction_ = funcSym;
    currentReturnType_ = funcSym ? funcSym->returnType : nullptr;

    visitStatements(body.statements);

    currentFunction_ = savedFunc;
    currentReturnType_ = savedReturn;
}

TypeSymbolPtr TypeChecker::getSymbolType(const SymbolPtr& sym) {
    if (!sym) return symbolTable_.getErrorType();

    // Variable → its declared/inferred type
    if (auto* var = sym->as<VariableSymbol>()) {
        return var->getType() ? var->getType() : symbolTable_.getErrorType();
    }

    // Function → build FunctionTypeSymbol
    if (auto* func = sym->as<FunctionSymbol>()) {
        auto funcType = func->buildFunctionType();
        if (funcType) return funcType;
        return symbolTable_.getErrorType();
    }

    // TypeSymbol → the type itself (struct, class, enum, interface)
    if (auto* type = sym->as<TypeSymbol>()) {
        return std::dynamic_pointer_cast<TypeSymbol>(sym);
    }

    return symbolTable_.getErrorType();
}

bool TypeChecker::checkAssignability(
    TypeSymbol* from, TypeSymbol* to,
    const std::shared_ptr<DebugInfo>& loc,
    const std::string& context)
{
    if (!from || !to) return true;  // null = not yet resolved, skip check
    if (symbolTable_.isCompatible(from, to)) return true;

    std::string msg = "type mismatch: cannot convert '"
        + from->getTypeDescription() + "' to '"
        + to->getTypeDescription() + "'";
    if (!context.empty()) msg += " (" + context + ")";
    errors_.error(msg, loc);
    return false;
}

TypeSymbolPtr TypeChecker::getWiderType(TypeSymbol* a, TypeSymbol* b) {
    if (!a || !b) return symbolTable_.getErrorType();
    if (a == b) return std::dynamic_pointer_cast<TypeSymbol>(
        symbolTable_.resolveType(a->getName()));

    auto* pa = a->as<PrimitiveTypeSymbol>();
    auto* pb = b->as<PrimitiveTypeSymbol>();
    if (!pa || !pb) return symbolTable_.getErrorType();

    // Widening rules: double > float > int > char > byte
    auto rank = [](PrimitiveKind k) -> int {
        switch (k) {
            case PrimitiveKind::Byte:   return 1;
            case PrimitiveKind::Char:   return 2;
            case PrimitiveKind::Int:    return 3;
            case PrimitiveKind::Float:  return 4;
            case PrimitiveKind::Double: return 5;
            default: return 0;
        }
    };

    int ra = rank(pa->primitiveKind);
    int rb = rank(pb->primitiveKind);
    if (ra == 0 || rb == 0) return symbolTable_.getErrorType();

    if (ra >= rb) {
        return symbolTable_.resolveType(a->getName());
    } else {
        return symbolTable_.resolveType(b->getName());
    }
}

std::shared_ptr<OperatorSymbol> TypeChecker::findOperatorOverload(
    TypeSymbol* type, OverloadableOp op)
{
    if (!type) return nullptr;

    // Check if the type is a SymbolWithScope (has operator definitions)
    auto* scope = dynamic_cast<Scope*>(type);
    if (!scope) return nullptr;

    return scope->resolveOperator(op);
}

OverloadableOp TypeChecker::binaryOpToOverloadable(BinaryOp op) {
    switch (op) {
        case BinaryOp::Add:          return OverloadableOp::Add;
        case BinaryOp::Sub:          return OverloadableOp::Sub;
        case BinaryOp::Mul:          return OverloadableOp::Mul;
        case BinaryOp::Div:          return OverloadableOp::Div;
        case BinaryOp::Mod:          return OverloadableOp::Mod;
        case BinaryOp::Equal:        return OverloadableOp::Equal;
        case BinaryOp::NotEqual:     return OverloadableOp::NotEqual;
        case BinaryOp::Less:         return OverloadableOp::Less;
        case BinaryOp::Greater:      return OverloadableOp::Greater;
        case BinaryOp::LessEqual:    return OverloadableOp::LessEq;
        case BinaryOp::GreaterEqual: return OverloadableOp::GreaterEq;
        default: return OverloadableOp::Add;  // fallback
    }
}

bool TypeChecker::isLValue(ExpressionBaseNode* expr) {
    if (!expr) return false;
    return expr->is<IdentifierExpression>()
        || expr->is<MemberAccessExpression>()
        || expr->is<IndexExpression>();
}

// ============================================================================
// Program & Module
// ============================================================================

void TypeChecker::visit(ProgramNode& node) {
    for (auto& module : node.modules) {
        if (module) module->accept(*this);
    }
}

void TypeChecker::visit(ModuleNode& node) {
    for (auto& decl : node.declarations) {
        if (decl) decl->accept(*this);
    }
}

void TypeChecker::visit(BlockStatementNode& node) {
    visitStatements(node.statements);
}

// ============================================================================
// Variable Declarations — type inference for var x = expr;
// ============================================================================

void TypeChecker::visit(VariableDeclaration& node) {
    if (node.initializer) {
        node.initializer->accept(*this);
    }

    auto& varSym = node.resolvedVariable;
    if (!varSym) return;

    if (node.isInferred && node.initializer) {
        // Type inference: set variable type from initializer
        auto initType = node.initializer->resolvedType;
        if (initType && !initType->is<ErrorTypeSymbol>()) {
            if (initType->is<NullTypeSymbol>()) {
                errors_.error("cannot infer type from null", node.debugInfo);
            } else if (initType->getName() == "void") {
                errors_.error("cannot declare variable with void type", node.debugInfo);
            } else {
                varSym->setType(initType);
            }
        }
    } else if (varSym->getType() && node.initializer) {
        // Explicit type + initializer: check compatibility
        auto initType = node.initializer->resolvedType;
        if (initType) {
            checkAssignability(initType.get(), varSym->getType().get(),
                node.debugInfo, "initializer");
        }
    }
}

void TypeChecker::visit(VariableDeclarationExpression& node) {
    if (node.initializer) {
        node.initializer->accept(*this);
    }

    auto& varSym = node.resolvedVariable;
    if (!varSym) return;

    if (node.isInferred && node.initializer) {
        auto initType = node.initializer->resolvedType;
        if (initType && !initType->is<ErrorTypeSymbol>()) {
            if (initType->is<NullTypeSymbol>()) {
                errors_.error("cannot infer type from null", node.debugInfo);
            } else if (initType->getName() == "void") {
                errors_.error("cannot declare variable with void type", node.debugInfo);
            } else {
                varSym->setType(initType);
            }
        }
    } else if (varSym->getType() && node.initializer) {
        auto initType = node.initializer->resolvedType;
        if (initType) {
            checkAssignability(initType.get(), varSym->getType().get(),
                node.debugInfo, "initializer");
        }
    }

    // The expression's type is the variable's type
    node.resolvedType = varSym->getType();
}

void TypeChecker::visit(TupleDestructuringDeclaration& node) {
    if (node.initializer) {
        node.initializer->accept(*this);
    }

    // If initializer is a tuple, assign element types to variables
    if (node.initializer && node.initializer->resolvedType) {
        auto* tupType = node.initializer->resolvedType->as<TupleTypeSymbol>();
        if (tupType && tupType->elementTypes.size() == node.resolvedVariables.size()) {
            for (size_t i = 0; i < node.resolvedVariables.size(); i++) {
                if (node.resolvedVariables[i]) {
                    node.resolvedVariables[i]->setType(tupType->elementTypes[i]);
                }
            }
        }
    }
}

// ============================================================================
// Function Declarations — type-check bodies
// ============================================================================

void TypeChecker::visit(FunctionDeclaration& node) {
    if (node.body && node.resolvedFunction) {
        auto savedClass = currentClass_;
        // If this is a method, currentClass_ is already set
        visitFunctionBody(node.resolvedFunction, *node.body);
        currentClass_ = savedClass;
    }
}

void TypeChecker::visit(ConstructorDeclaration& node) {
    if (node.body && node.resolvedConstructor) {
        auto funcSym = std::dynamic_pointer_cast<FunctionSymbol>(node.resolvedConstructor);
        visitFunctionBody(funcSym, *node.body);
    }
}

void TypeChecker::visit(DestructorDeclaration& node) {
    if (node.body && node.resolvedDestructor) {
        auto funcSym = std::dynamic_pointer_cast<FunctionSymbol>(node.resolvedDestructor);
        visitFunctionBody(funcSym, *node.body);
    }
}

void TypeChecker::visit(ExternFunctionDeclaration& node) {
    // No body to check
}

void TypeChecker::visit(OperatorDeclaration& node) {
    if (node.body && node.resolvedOperator) {
        auto funcSym = std::dynamic_pointer_cast<FunctionSymbol>(node.resolvedOperator);
        visitFunctionBody(funcSym, *node.body);
    }
}

// ============================================================================
// Type Declarations — check member bodies
// ============================================================================

void TypeChecker::visit(EnumDeclaration& node) {
    // Enum member values could be checked for constant evaluation here
}

void TypeChecker::visit(StructDeclaration& node) {
    for (auto& method : node.methods) {
        if (method) method->accept(*this);
    }
    for (auto& op : node.operators) {
        if (op) op->accept(*this);
    }
}

void TypeChecker::visit(ClassDeclaration& node) {
    auto savedClass = currentClass_;
    currentClass_ = node.resolvedClass.get();

    // Check field initializers (if any)
    for (auto& field : node.fields) {
        if (field) field->accept(*this);
    }

    if (node.constructor) node.constructor->accept(*this);
    if (node.destructor) node.destructor->accept(*this);

    for (auto& method : node.methods) {
        if (method) method->accept(*this);
    }
    for (auto& op : node.operators) {
        if (op) op->accept(*this);
    }

    currentClass_ = savedClass;
}

void TypeChecker::visit(InterfaceDeclaration& node) {
    // Abstract methods only — no bodies to check
}

void TypeChecker::visit(ImportDeclaration& node) {}

// ============================================================================
// Statements
// ============================================================================

void TypeChecker::visit(ExpressionStatement& node) {
    if (node.expression) node.expression->accept(*this);
}

void TypeChecker::visit(ReturnStatement& node) {
    if (node.value) {
        node.value->accept(*this);
    }

    if (currentReturnType_) {
        if (node.value && node.value->resolvedType) {
            if (currentReturnType_->getName() == "void") {
                errors_.error("returning a value from void function", node.debugInfo);
            } else {
                checkAssignability(node.value->resolvedType.get(),
                    currentReturnType_.get(), node.debugInfo, "return");
            }
        } else if (!node.value && currentReturnType_->getName() != "void") {
            errors_.error("non-void function must return a value", node.debugInfo);
        }
    } else {
        // Lambda return type inference: if currentReturnType_ is null,
        // the first return statement sets it (block-body lambdas)
        if (node.value && node.value->resolvedType) {
            currentReturnType_ = node.value->resolvedType;
        }
    }
}

void TypeChecker::visit(IfStatement& node) {
    if (node.condition) {
        node.condition->accept(*this);
        if (node.condition->resolvedType &&
            !symbolTable_.isCompatible(node.condition->resolvedType.get(),
                                        symbolTable_.getBoolType().get())) {
            errors_.error("if condition must be bool", node.debugInfo);
        }
    }
    if (node.thenBody) node.thenBody->accept(*this);
    for (auto& elseIf : node.elseIfClauses) {
        if (elseIf.condition) {
            elseIf.condition->accept(*this);
        }
        if (elseIf.body) elseIf.body->accept(*this);
    }
    if (node.elseBody) node.elseBody->accept(*this);
}

void TypeChecker::visit(ForStatement& node) {
    if (node.initDeclaration) node.initDeclaration->accept(*this);
    for (auto& initExpr : node.initExpressions) {
        if (initExpr) initExpr->accept(*this);
    }
    if (node.condition) {
        node.condition->accept(*this);
    }
    for (auto& iter : node.iterators) {
        if (iter) iter->accept(*this);
    }
    if (node.body) node.body->accept(*this);
}

void TypeChecker::visit(WhileStatement& node) {
    if (node.condition) {
        node.condition->accept(*this);
        if (node.condition->resolvedType &&
            !symbolTable_.isCompatible(node.condition->resolvedType.get(),
                                        symbolTable_.getBoolType().get())) {
            errors_.error("while condition must be bool", node.debugInfo);
        }
    }
    if (node.body) node.body->accept(*this);
}

void TypeChecker::visit(BreakStatement& node) {}
void TypeChecker::visit(ContinueStatement& node) {}

void TypeChecker::visit(DeleteStatement& node) {
    if (node.target) {
        node.target->accept(*this);
        // Target must be a pointer type
        if (node.target->resolvedType &&
            !node.target->resolvedType->is<PointerTypeSymbol>() &&
            !node.target->resolvedType->is<ClassSymbol>()) {
            errors_.error("delete requires pointer or class type", node.debugInfo);
        }
    }
}

void TypeChecker::visit(SwitchStatement& node) {
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
// Expressions — type inference (bottom-up)
// ============================================================================

void TypeChecker::visit(IntegerLiteral& node) {
    node.resolvedType = symbolTable_.getIntType();
}

void TypeChecker::visit(FloatLiteral& node) {
    node.resolvedType = symbolTable_.getDoubleType();
}

void TypeChecker::visit(BoolLiteral& node) {
    node.resolvedType = symbolTable_.getBoolType();
}

void TypeChecker::visit(CharLiteral& node) {
    node.resolvedType = symbolTable_.getCharType();
}

void TypeChecker::visit(StringLiteral& node) {
    node.resolvedType = symbolTable_.getStringType();
}

void TypeChecker::visit(NullLiteral& node) {
    node.resolvedType = symbolTable_.getNullType();
}

void TypeChecker::visit(InterpolatedStringExpression& node) {
    for (auto& part : node.parts) {
        if (part.kind == InterpolatedPartKind::Expression && part.expression) {
            part.expression->accept(*this);
        }
    }
    node.resolvedType = symbolTable_.getStringType();
}

void TypeChecker::visit(ThisExpression& node) {
    if (currentClass_) {
        // 'this' is a pointer to the current class
        auto classType = symbolTable_.resolveType(currentClass_->getName());
        if (classType) {
            node.resolvedType = symbolTable_.getPointerType(classType);
        } else {
            node.resolvedType = symbolTable_.getErrorType();
        }
    } else {
        errors_.error("'this' used outside of class context", node.debugInfo);
        node.resolvedType = symbolTable_.getErrorType();
    }
}

// ============================================================================
// Identifier Resolution
// ============================================================================

void TypeChecker::visit(IdentifierExpression& node) {
    auto* scope = node.astScopeNode ? node.astScopeNode.get() : nullptr;
    if (!scope) {
        node.resolvedType = symbolTable_.getErrorType();
        return;
    }

    auto sym = scope->resolve(node.name);
    if (!sym) {
        errors_.error("undefined identifier '" + node.name + "'", node.debugInfo);
        node.resolvedType = symbolTable_.getErrorType();
        return;
    }

    node.resolvedSymbol = sym;
    node.resolvedType = getSymbolType(sym);
}

void TypeChecker::visit(QualifiedNameExpression& node) {
    if (node.parts.empty()) {
        node.resolvedType = symbolTable_.getErrorType();
        return;
    }

    auto* scope = node.astScopeNode ? node.astScopeNode.get() : nullptr;
    if (!scope) {
        node.resolvedType = symbolTable_.getErrorType();
        return;
    }

    // Resolve first part
    auto sym = scope->resolve(node.parts[0]);
    if (!sym) {
        errors_.error("undefined '" + node.parts[0] + "'", node.debugInfo);
        node.resolvedType = symbolTable_.getErrorType();
        return;
    }

    // Walk qualified parts: Module.Enum.Member etc.
    for (size_t i = 1; i < node.parts.size(); i++) {
        auto* symScope = dynamic_cast<Scope*>(sym.get());
        if (!symScope) {
            errors_.error("'" + node.parts[i-1] + "' is not a scope", node.debugInfo);
            node.resolvedType = symbolTable_.getErrorType();
            return;
        }
        sym = symScope->resolve(node.parts[i]);
        if (!sym) {
            errors_.error("'" + node.parts[i] + "' not found in '"
                + node.parts[i-1] + "'", node.debugInfo);
            node.resolvedType = symbolTable_.getErrorType();
            return;
        }
    }

    node.resolvedSymbol = sym;
    node.resolvedType = getSymbolType(sym);
}

// ============================================================================
// Member Access
// ============================================================================

void TypeChecker::visit(MemberAccessExpression& node) {
    if (node.object) node.object->accept(*this);

    auto objType = node.object ? node.object->resolvedType : nullptr;
    if (!objType) {
        node.resolvedType = symbolTable_.getErrorType();
        return;
    }

    // If pointer type, dereference for member access
    if (node.isArrow) {
        if (auto* ptrType = objType->as<PointerTypeSymbol>()) {
            objType = ptrType->baseType;
        }
    }

    // Enum member access: Color.Red
    if (auto* enumSym = objType->as<EnumSymbol>()) {
        auto* member = enumSym->findMember(node.memberName);
        if (member) {
            node.isEnumAccess = true;
            node.resolvedEnumValue = member->intValue;
            node.resolvedEnumStringValue = member->stringValue;
            node.resolvedType = std::dynamic_pointer_cast<TypeSymbol>(
                symbolTable_.resolveType(enumSym->getName()));
            return;
        }
    }

    // String builtin methods
    if (objType.get() == symbolTable_.getStringType().get()) {
        if (node.memberName == "length" || node.memberName == "charAt" ||
            node.memberName == "substring" || node.memberName == "indexOf" ||
            node.memberName == "toInt" || node.memberName == "toDouble") {
            node.isStringBuiltinMethod = true;
            // Return type will be resolved when the CallExpression visits this
            node.resolvedType = symbolTable_.getIntType();  // placeholder
            return;
        }
    }

    // Struct/Class field or method
    auto* typeScope = dynamic_cast<Scope*>(objType.get());
    if (typeScope) {
        auto memberSym = typeScope->resolve(node.memberName);
        if (memberSym) {
            node.resolvedSymbol = memberSym;
            node.resolvedType = getSymbolType(memberSym);

            // Check static vs instance access
            if (auto* func = memberSym->as<FunctionSymbol>()) {
                if (func->isStatic) {
                    node.isStaticAccess = true;
                }
            }
            return;
        }
    }

    errors_.error("no member '" + node.memberName + "' in type '"
        + objType->getTypeDescription() + "'", node.debugInfo);
    node.resolvedType = symbolTable_.getErrorType();
}

// ============================================================================
// Binary Expression
// ============================================================================

void TypeChecker::visit(BinaryExpression& node) {
    if (node.left) node.left->accept(*this);
    if (node.right) node.right->accept(*this);

    auto leftType = node.left ? node.left->resolvedType : nullptr;
    auto rightType = node.right ? node.right->resolvedType : nullptr;

    if (!leftType || !rightType) {
        node.resolvedType = symbolTable_.getErrorType();
        return;
    }

    // Check for operator overload on left type
    auto overloadOp = binaryOpToOverloadable(node.op);
    auto opSym = findOperatorOverload(leftType.get(), overloadOp);
    if (opSym) {
        node.isOperatorOverload = true;
        node.resolvedOperatorFunction = opSym;
        node.resolvedType = opSym->returnType ? opSym->returnType : symbolTable_.getErrorType();
        return;
    }

    switch (node.op) {
        // Arithmetic
        case BinaryOp::Add: {
            // String concatenation
            if (leftType.get() == symbolTable_.getStringType().get() ||
                rightType.get() == symbolTable_.getStringType().get()) {
                node.resolvedType = symbolTable_.getStringType();
                return;
            }
            node.resolvedType = getWiderType(leftType.get(), rightType.get());
            return;
        }
        case BinaryOp::Sub:
        case BinaryOp::Mul:
        case BinaryOp::Div:
        case BinaryOp::Mod:
            node.resolvedType = getWiderType(leftType.get(), rightType.get());
            return;

        // Bitwise
        case BinaryOp::BitwiseAnd:
        case BinaryOp::BitwiseOr:
        case BinaryOp::BitwiseXor:
        case BinaryOp::ShiftLeft:
        case BinaryOp::ShiftRight:
            node.resolvedType = getWiderType(leftType.get(), rightType.get());
            return;

        // Comparison → bool
        case BinaryOp::Equal:
        case BinaryOp::NotEqual:
        case BinaryOp::Less:
        case BinaryOp::Greater:
        case BinaryOp::LessEqual:
        case BinaryOp::GreaterEqual:
            node.resolvedType = symbolTable_.getBoolType();
            return;

        // Logical → bool
        case BinaryOp::LogicalAnd:
        case BinaryOp::LogicalOr:
            node.resolvedType = symbolTable_.getBoolType();
            return;

        default:
            node.resolvedType = symbolTable_.getErrorType();
            return;
    }
}

// ============================================================================
// Unary Expression
// ============================================================================

void TypeChecker::visit(UnaryExpression& node) {
    if (node.operand) node.operand->accept(*this);

    auto opType = node.operand ? node.operand->resolvedType : nullptr;
    if (!opType) {
        node.resolvedType = symbolTable_.getErrorType();
        return;
    }

    switch (node.op) {
        case UnaryOp::Negate:
            node.resolvedType = opType;  // same numeric type
            return;
        case UnaryOp::LogicalNot:
            node.resolvedType = symbolTable_.getBoolType();
            return;
        case UnaryOp::BitwiseNot:
            node.resolvedType = opType;
            return;
        case UnaryOp::Dereference:
            if (auto* ptr = opType->as<PointerTypeSymbol>()) {
                node.resolvedType = ptr->baseType;
            } else {
                errors_.error("dereference of non-pointer type", node.debugInfo);
                node.resolvedType = symbolTable_.getErrorType();
            }
            return;
        case UnaryOp::AddressOf:
            node.resolvedType = symbolTable_.getPointerType(opType);
            return;
        default:
            node.resolvedType = opType;
            return;
    }
}

// ============================================================================
// Assignment Expression
// ============================================================================

void TypeChecker::visit(AssignmentExpression& node) {
    if (node.target) node.target->accept(*this);
    if (node.value) node.value->accept(*this);

    auto targetType = node.target ? node.target->resolvedType : nullptr;
    auto valueType = node.value ? node.value->resolvedType : nullptr;

    if (!targetType || !valueType) {
        node.resolvedType = symbolTable_.getErrorType();
        return;
    }

    // Validate lvalue
    if (!isLValue(node.target.get())) {
        errors_.error("assignment to non-lvalue", node.debugInfo);
    }

    // Check compatibility
    if (node.op == AssignOp::Assign) {
        checkAssignability(valueType.get(), targetType.get(),
            node.debugInfo, "assignment");
    }
    // Compound assignments (+=, -=, etc.) — the result type matches target
    node.resolvedType = targetType;
}

// ============================================================================
// Ternary Expression
// ============================================================================

void TypeChecker::visit(TernaryExpression& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.thenExpr) node.thenExpr->accept(*this);
    if (node.elseExpr) node.elseExpr->accept(*this);

    // Result type is the wider of the two branches
    auto thenType = node.thenExpr ? node.thenExpr->resolvedType : nullptr;
    auto elseType = node.elseExpr ? node.elseExpr->resolvedType : nullptr;

    if (thenType && elseType) {
        if (symbolTable_.isCompatible(thenType.get(), elseType.get())) {
            node.resolvedType = elseType;
        } else if (symbolTable_.isCompatible(elseType.get(), thenType.get())) {
            node.resolvedType = thenType;
        } else {
            node.resolvedType = thenType;  // use then-branch, report mismatch
            errors_.error("ternary branches have incompatible types", node.debugInfo);
        }
    } else {
        node.resolvedType = thenType ? thenType : elseType;
    }
}

// ============================================================================
// Index Expression — arr[i]
// ============================================================================

void TypeChecker::visit(IndexExpression& node) {
    if (node.object) node.object->accept(*this);
    if (node.index) node.index->accept(*this);

    auto objType = node.object ? node.object->resolvedType : nullptr;
    if (!objType) {
        node.resolvedType = symbolTable_.getErrorType();
        return;
    }

    // Array indexing
    if (auto* arrType = objType->as<ArrayTypeSymbol>()) {
        node.resolvedType = arrType->elementType;
        return;
    }

    // Pointer indexing (ptr[i] = *(ptr + i))
    if (auto* ptrType = objType->as<PointerTypeSymbol>()) {
        node.resolvedType = ptrType->baseType;
        return;
    }

    // Operator[] overload
    auto opSym = findOperatorOverload(objType.get(), OverloadableOp::Index);
    if (opSym) {
        node.isOperatorOverload = true;
        node.resolvedOperatorFunction = opSym;
        node.resolvedType = opSym->returnType ? opSym->returnType : symbolTable_.getErrorType();
        return;
    }

    errors_.error("subscript on non-indexable type '" + objType->getTypeDescription() + "'",
        node.debugInfo);
    node.resolvedType = symbolTable_.getErrorType();
}

// ============================================================================
// Call Expression — the most critical resolution
// ============================================================================

void TypeChecker::visit(CallExpression& node) {
    if (node.callee) node.callee->accept(*this);
    if (node.arguments) {
        for (auto& arg : node.arguments->expressions) {
            if (arg) arg->accept(*this);
        }
    }

    auto calleeType = node.callee ? node.callee->resolvedType : nullptr;
    if (!calleeType) {
        node.resolvedType = symbolTable_.getErrorType();
        return;
    }

    // Get FunctionTypeSymbol from callee
    FunctionTypeSymbol* funcType = nullptr;
    std::shared_ptr<FunctionSymbol> funcSym;
    // Keep the built FunctionTypeSymbol alive for the duration of this function
    std::shared_ptr<FunctionTypeSymbol> funcTypeHolder;

    // Direct function call (identifier resolved to FunctionSymbol)
    if (node.callee->resolvedSymbol) {
        auto* fSym = node.callee->resolvedSymbol->as<FunctionSymbol>();
        if (fSym) {
            funcSym = std::dynamic_pointer_cast<FunctionSymbol>(node.callee->resolvedSymbol);
            funcTypeHolder = fSym->buildFunctionType();
            if (funcTypeHolder) funcType = funcTypeHolder.get();
        }
    }

    // Method call via member access
    if (!funcSym && node.callee->is<MemberAccessExpression>()) {
        auto* memberAccess = node.callee->as<MemberAccessExpression>();
        if (memberAccess->resolvedSymbol) {
            auto* fSym = memberAccess->resolvedSymbol->as<FunctionSymbol>();
            if (fSym) {
                funcSym = std::dynamic_pointer_cast<FunctionSymbol>(
                    memberAccess->resolvedSymbol);
                funcTypeHolder = fSym->buildFunctionType();
                if (funcTypeHolder) funcType = funcTypeHolder.get();
            }
        }
    }

    // Closure/function variable call (callee type is FunctionTypeSymbol)
    if (!funcType) {
        funcType = calleeType->as<FunctionTypeSymbol>();
    }

    // Constructor call: callee is a type name
    if (!funcType && calleeType->is<ClassSymbol>()) {
        auto* classSym = calleeType->as<ClassSymbol>();
        if (classSym && classSym->constructor) {
            funcSym = classSym->constructor;
            funcTypeHolder = classSym->constructor->buildFunctionType();
            if (funcTypeHolder) funcType = funcTypeHolder.get();
        }
        // Result of constructor call is a pointer to the class
        node.resolvedType = symbolTable_.getPointerType(calleeType);
        node.resolvedCallee = funcSym;

        // Set isReference on arguments
        if (funcType && node.arguments) {
            node.arguments->isReference.clear();
            for (size_t i = 0; i < node.arguments->expressions.size(); i++) {
                bool isRef = (i < funcType->parameters.size())
                    ? funcType->parameters[i].isReference : false;
                node.arguments->isReference.push_back(isRef);
            }
        }
        return;
    }

    if (!funcType) {
        errors_.error("call to non-callable type '" + calleeType->getTypeDescription() + "'",
            node.debugInfo);
        node.resolvedType = symbolTable_.getErrorType();
        return;
    }

    // Set resolved callee for codegen
    node.resolvedCallee = funcSym;

    // Return type
    node.resolvedType = funcType->returnType ? funcType->returnType : symbolTable_.getVoidType();

    // Check argument count
    size_t argCount = node.arguments ? node.arguments->expressions.size() : 0;
    size_t paramCount = funcType->parameters.size();
    if (argCount != paramCount && !funcType->isVariadic) {
        errors_.error("expected " + std::to_string(paramCount) + " arguments, got "
            + std::to_string(argCount), node.debugInfo);
    }

    // Set isReference per argument + check types
    if (node.arguments) {
        node.arguments->isReference.clear();
        for (size_t i = 0; i < node.arguments->expressions.size(); i++) {
            bool isRef = (i < funcType->parameters.size())
                ? funcType->parameters[i].isReference : false;
            node.arguments->isReference.push_back(isRef);

            // Type check argument against parameter
            if (i < funcType->parameters.size() && node.arguments->expressions[i]) {
                auto argType = node.arguments->expressions[i]->resolvedType;
                auto paramType = funcType->parameters[i].type;
                if (argType && paramType) {
                    checkAssignability(argType.get(), paramType.get(),
                        node.arguments->expressions[i]->debugInfo, "argument");
                }
            }
        }
    }
}

// ============================================================================
// Cast, New, SizeOf
// ============================================================================

void TypeChecker::visit(CastExpression& node) {
    if (node.operand) node.operand->accept(*this);

    // Target type already resolved by Pass 2
    if (node.targetType && node.targetType->resolvedType) {
        node.resolvedType = node.targetType->resolvedType;
    } else {
        node.resolvedType = symbolTable_.getErrorType();
    }
}

void TypeChecker::visit(NewExpression& node) {
    if (node.arguments) {
        for (auto& arg : node.arguments->expressions) {
            if (arg) arg->accept(*this);
        }
    }
    if (node.arraySize) node.arraySize->accept(*this);

    if (node.type && node.type->resolvedType) {
        if (node.isArray) {
            node.resolvedType = symbolTable_.getPointerType(node.type->resolvedType);
        } else {
            node.resolvedType = symbolTable_.getPointerType(node.type->resolvedType);
        }
    } else {
        node.resolvedType = symbolTable_.getErrorType();
    }
}

void TypeChecker::visit(SizeOfExpression& node) {
    node.resolvedType = symbolTable_.getIntType();
}

// ============================================================================
// Tuple
// ============================================================================

void TypeChecker::visit(TupleExpression& node) {
    std::vector<TypeSymbolPtr> elemTypes;
    for (auto& elem : node.elements) {
        if (elem) {
            elem->accept(*this);
            elemTypes.push_back(elem->resolvedType
                ? elem->resolvedType : symbolTable_.getErrorType());
        }
    }
    node.resolvedType = symbolTable_.getTupleType(std::move(elemTypes));
}

// ============================================================================
// Match Expression
// ============================================================================

void TypeChecker::visit(MatchExpression& node) {
    if (node.subject) node.subject->accept(*this);

    TypeSymbolPtr resultType = nullptr;
    for (auto& arm : node.arms) {
        // Pattern bindings: set type from subject
        if (arm.pattern) {
            if (auto* idPat = arm.pattern->as<IdentifierPattern>()) {
                if (idPat->resolvedSymbol && node.subject && node.subject->resolvedType) {
                    idPat->resolvedSymbol->setType(node.subject->resolvedType);
                }
                if (idPat->guard) idPat->guard->accept(*this);
            }
        }

        if (arm.body) {
            arm.body->accept(*this);

            // Infer result type from arm body
            if (auto* exprBody = arm.body->as<ExpressionBaseNode>()) {
                if (!resultType && exprBody->resolvedType) {
                    resultType = exprBody->resolvedType;
                }
            }
        }
    }

    node.resolvedType = resultType ? resultType : symbolTable_.getVoidType();
}

// ============================================================================
// Pipe Expression — x |> f |> g
// ============================================================================

void TypeChecker::visit(PipeExpression& node) {
    if (node.input) node.input->accept(*this);

    TypeSymbolPtr currentType = node.input ? node.input->resolvedType : nullptr;

    for (auto& stage : node.stages) {
        if (stage.function) stage.function->accept(*this);
        for (auto& arg : stage.extraArguments) {
            if (arg) arg->accept(*this);
        }

        // The stage function's return type becomes the next input
        auto funcType = stage.function ? stage.function->resolvedType : nullptr;
        if (funcType) {
            if (auto* ft = funcType->as<FunctionTypeSymbol>()) {
                currentType = ft->returnType;
            }
        }
    }

    node.resolvedType = currentType ? currentType : symbolTable_.getErrorType();
}

// ============================================================================
// Lambda Expression
// ============================================================================

void TypeChecker::visit(LambdaExpression& node) {
    // Save context
    auto savedFunc = currentFunction_;
    auto savedReturn = currentReturnType_;
    currentReturnType_ = nullptr;  // will be inferred

    // Visit body
    if (node.body) {
        if (auto* block = node.body->as<BlockStatementNode>()) {
            visitStatements(block->statements);
        } else if (auto* expr = node.body->as<ExpressionBaseNode>()) {
            expr->accept(*this);
            // Expression body: return type is the expression's type
            currentReturnType_ = expr->resolvedType;
        }
    }

    // Build function type for the lambda
    std::vector<FunctionTypeSymbol::ParameterInfo> params;
    for (auto& param : node.parameters) {
        FunctionTypeSymbol::ParameterInfo pi;
        if (param && param->resolvedSymbol) {
            pi.type = param->resolvedSymbol->getType();
            pi.name = param->name;
            pi.isReference = param->resolvedSymbol->isReference;
        }
        if (!pi.type) pi.type = symbolTable_.getErrorType();
        params.push_back(pi);
    }

    auto retType = currentReturnType_ ? currentReturnType_ : symbolTable_.getVoidType();
    node.resolvedType = symbolTable_.getFunctionType(std::move(params), retType);

    // Restore context
    currentFunction_ = savedFunc;
    currentReturnType_ = savedReturn;
}

} // namespace mingus
