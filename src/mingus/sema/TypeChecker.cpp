// ============================================================================
// TypeChecker.cpp — Pass 3: Expression type inference and compatibility
//
// Bottom-up walk: visit children first, then infer this node's type.
// Every ExpressionBaseNode gets a resolvedType (ErrorType on failure).
// ============================================================================

#include "mingus/sema/TypeChecker.h"
#include "mingus/Scope.h"  // BaseScope::resolveFunctions

#include <climits>  // INT_MAX

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

    // Unwrap enum types to underlying type for arithmetic
    if (auto* ea = a->as<EnumSymbol>()) {
        a = ea->underlyingType ? ea->underlyingType.get()
            : symbolTable_.getIntType().get();
    }
    if (auto* eb = b->as<EnumSymbol>()) {
        b = eb->underlyingType ? eb->underlyingType.get()
            : symbolTable_.getIntType().get();
    }

    if (a == b) return std::dynamic_pointer_cast<TypeSymbol>(
        symbolTable_.resolveType(a->getName()));

    auto* pa = a->as<PrimitiveTypeSymbol>();
    auto* pb = b->as<PrimitiveTypeSymbol>();
    if (!pa || !pb) return symbolTable_.getErrorType();

    // Width-based ranking: wider type wins, float outranks all integers
    auto widthRank = [](PrimitiveKind k) -> int {
        switch (k) {
            case PrimitiveKind::Byte:   case PrimitiveKind::Char:   return 1; // 8-bit
            case PrimitiveKind::Short:  case PrimitiveKind::UShort: return 2; // 16-bit
            case PrimitiveKind::Int:    case PrimitiveKind::UInt:   return 3; // 32-bit
            case PrimitiveKind::Long:   case PrimitiveKind::ULong:  return 4; // 64-bit
            case PrimitiveKind::Float:  return 5;
            case PrimitiveKind::Double: return 6;
            default: return 0;
        }
    };

    int ra = widthRank(pa->primitiveKind);
    int rb = widthRank(pb->primitiveKind);
    if (ra == 0 || rb == 0) return symbolTable_.getErrorType();

    // Different width ranks: wider type wins
    if (ra > rb) return symbolTable_.resolveType(a->getName());
    if (rb > ra) return symbolTable_.resolveType(b->getName());

    // Same rank, different signs: unsigned wins (C convention)
    if (pa->isUnsigned() && !pb->isUnsigned())
        return symbolTable_.resolveType(a->getName());
    if (!pa->isUnsigned() && pb->isUnsigned())
        return symbolTable_.resolveType(b->getName());

    // Same rank, same sign (e.g. byte vs char at rank 1)
    return symbolTable_.resolveType(a->getName());
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
    if (expr->is<IdentifierExpression>()) return true;
    if (expr->is<MemberAccessExpression>()) return true;
    if (expr->is<IndexExpression>()) return true;
    // Dereference (*ptr) is an lvalue
    if (auto* unary = expr->as<UnaryExpression>()) {
        return unary->op == UnaryOp::Dereference;
    }
    return false;
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
    // Skip generic template bodies — checked only on monomorphized instances
    if (node.resolvedFunction && node.resolvedFunction->isGenericTemplate()) {
        return;
    }
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

        // Type-check super constructor arguments (they reference ctor params)
        for (auto& arg : node.superArgs) {
            if (arg) arg->accept(*this);
        }

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
    auto savedClass = currentClass_;
    currentClass_ = node.resolvedStruct.get();

    for (auto& method : node.methods) {
        if (method) method->accept(*this);
    }
    for (auto& op : node.operators) {
        if (op) op->accept(*this);
    }

    currentClass_ = savedClass;
}

void TypeChecker::visit(UnionDeclaration& node) {
    auto savedClass = currentClass_;
    currentClass_ = node.resolvedUnion.get();

    for (auto& method : node.methods) {
        if (method) method->accept(*this);
    }

    currentClass_ = savedClass;
}

void TypeChecker::visit(TaggedUnionDeclaration& node) {
    // Types already resolved by TypeResolver (Pass 2) — nothing else to check
}

void TypeChecker::visit(ClassDeclaration& node) {
    auto savedClass = currentClass_;
    currentClass_ = node.resolvedClass.get();

    // Check field initializers (if any)
    for (auto& field : node.fields) {
        if (field) field->accept(*this);
    }

    for (auto& ctor : node.constructors) {
        if (ctor) ctor->accept(*this);
    }
    if (node.copyConstructor) node.copyConstructor->accept(*this);
    if (node.moveConstructor) node.moveConstructor->accept(*this);
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
void TypeChecker::visit(TypedefDeclaration& node) {}

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
    for (auto& initDecl : node.initDeclarations) {
        if (initDecl) initDecl->accept(*this);
    }
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

void TypeChecker::visit(DoWhileStatement& node) {
    if (node.body) node.body->accept(*this);
    if (node.condition) {
        node.condition->accept(*this);
        if (node.condition->resolvedType &&
            !symbolTable_.isCompatible(node.condition->resolvedType.get(),
                                        symbolTable_.getBoolType().get())) {
            errors_.error("do-while condition must be bool", node.debugInfo);
        }
    }
}

void TypeChecker::visit(LabeledStatement& node) {
    if (node.statement) node.statement->accept(*this);
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
    // Auto-promote based on value (C++ convention: int → long for large values)
    if (node.value >= -2147483648LL && node.value <= 2147483647LL) {
        node.resolvedType = symbolTable_.getIntType();      // fits in i32
    } else {
        node.resolvedType = symbolTable_.getLongType();      // needs i64
    }
}

void TypeChecker::visit(FloatLiteral& node) {
    node.resolvedType = node.isFloat
        ? symbolTable_.getFloatType() : symbolTable_.getDoubleType();
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
        // 'this' resolves to the struct/class type in sema
        // (codegen handles the LLVM-level pointer internally)
        auto type = symbolTable_.resolveType(currentClass_->getName());
        node.resolvedType = type ? type : symbolTable_.getErrorType();
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
        // Special case: enum member access (members stored in data vector, not scope)
        if (auto* enumSym = sym->as<EnumSymbol>()) {
            auto* member = enumSym->findMember(node.parts[i]);
            if (member) {
                // Store the EnumSymbol as resolved (codegen uses it + member name)
                node.resolvedSymbol = std::dynamic_pointer_cast<Symbol>(
                    symbolTable_.resolveType(enumSym->getName()));
                node.isEnumAccess = true;

                // Detect string-backed enum
                bool isStringEnum = false;
                if (enumSym->underlyingType) {
                    if (auto* prim = enumSym->underlyingType->as<PrimitiveTypeSymbol>()) {
                        isStringEnum = (prim->primitiveKind == PrimitiveKind::String);
                    }
                }

                if (isStringEnum) {
                    node.isStringEnumAccess = true;
                    node.resolvedEnumStringValue = member->stringValue;
                    node.resolvedType = symbolTable_.getStringType();
                } else {
                    node.resolvedEnumValue = member->intValue;
                    node.resolvedType = node.resolvedSymbol
                        ? std::dynamic_pointer_cast<TypeSymbol>(node.resolvedSymbol)
                        : symbolTable_.getIntType();
                }
                return;
            }
            errors_.error("'" + node.parts[i] + "' not found in enum '"
                + enumSym->getName() + "'", node.debugInfo);
            node.resolvedType = symbolTable_.getErrorType();
            return;
        }

        // Tagged union variant access: Result.Ok, Option.None
        if (auto* tuSym = sym->as<TaggedUnionSymbol>()) {
            auto* variant = tuSym->findVariant(node.parts[i]);
            if (variant) {
                node.isTaggedUnionVariant = true;
                node.resolvedVariantTag = variant->tagValue;
                node.resolvedTaggedUnion = std::dynamic_pointer_cast<TaggedUnionSymbol>(
                    symbolTable_.resolveType(tuSym->getName()));

                // Type is the tagged union itself
                node.resolvedType = node.resolvedTaggedUnion;

                // If variant has fields, this is a constructor call (e.g. Result.Ok(42))
                // The CallExpression visitor will handle argument checking.
                // If no fields, this IS the constructed value (e.g. Option.None)
                return;
            }
            errors_.error("'" + node.parts[i] + "' not found in tagged union '"
                + tuSym->getName() + "'", node.debugInfo);
            node.resolvedType = symbolTable_.getErrorType();
            return;
        }

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

    // Auto-dereference pointer types for member access
    // Arrow (->) always dereferences; dot (.) also dereferences for `this.x` pattern
    if (auto* ptrType = objType->as<PointerTypeSymbol>()) {
        objType = ptrType->baseType;
    }

    // Enum member access: Color.Red
    if (auto* enumSym = objType->as<EnumSymbol>()) {
        auto* member = enumSym->findMember(node.memberName);
        if (member) {
            node.isEnumAccess = true;
            node.resolvedEnumValue = member->intValue;
            node.resolvedEnumStringValue = member->stringValue;

            // Detect string-backed enum
            bool isStringEnum = false;
            if (enumSym->underlyingType) {
                if (auto* prim = enumSym->underlyingType->as<PrimitiveTypeSymbol>()) {
                    isStringEnum = (prim->primitiveKind == PrimitiveKind::String);
                }
            }
            if (isStringEnum) {
                node.isStringEnumAccess = true;
                node.resolvedType = symbolTable_.getStringType();
            } else {
                node.resolvedType = std::dynamic_pointer_cast<TypeSymbol>(
                    symbolTable_.resolveType(enumSym->getName()));
            }
            return;
        }
    }

    // Tagged union variant access: Option.Some, Option.None
    if (auto* tuSym = objType->as<TaggedUnionSymbol>()) {
        auto* variant = tuSym->findVariant(node.memberName);
        if (variant) {
            node.isTaggedUnionVariant = true;
            node.resolvedVariantTag = variant->tagValue;
            node.resolvedTaggedUnion = std::dynamic_pointer_cast<TaggedUnionSymbol>(
                symbolTable_.resolveType(tuSym->getName()));
            node.resolvedType = node.resolvedTaggedUnion;
            return;
        }
        errors_.error("'" + node.memberName + "' not found in tagged union '"
            + tuSym->getName() + "'", node.debugInfo);
        node.resolvedType = symbolTable_.getErrorType();
        return;
    }

    // String builtin methods (for `string` / char*)
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

    // String object methods (for `String` value type)
    if (objType.get() == symbolTable_.getStringObjectType().get()) {
        if (node.memberName == "length" || node.memberName == "capacity" ||
            node.memberName == "charAt" || node.memberName == "slice" ||
            node.memberName == "append" || node.memberName == "indexOf" ||
            node.memberName == "contains" || node.memberName == "cstr" ||
            node.memberName == "toInt" || node.memberName == "toDouble") {
            node.isStringObjectMethod = true;
            node.resolvedType = symbolTable_.getIntType();  // placeholder, resolved in CallExpression
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
            // String object concatenation (String + String, String + string, string + String)
            {
                auto* soType = symbolTable_.getStringObjectType().get();
                auto* sType  = symbolTable_.getStringType().get();
                bool leftIsSO  = (leftType.get() == soType);
                bool rightIsSO = (rightType.get() == soType);
                bool leftIsStr = (leftType.get() == sType);
                bool rightIsStr = (rightType.get() == sType);
                if ((leftIsSO || rightIsSO) && (leftIsSO || leftIsStr) && (rightIsSO || rightIsStr)) {
                    node.resolvedType = symbolTable_.getStringObjectType();
                    return;
                }
            }
            // String (char*) concatenation
            if (leftType.get() == symbolTable_.getStringType().get() ||
                rightType.get() == symbolTable_.getStringType().get()) {
                node.resolvedType = symbolTable_.getStringType();
                return;
            }
            // Pointer arithmetic: ptr + int → ptr, int + ptr → ptr
            if (leftType->is<PointerTypeSymbol>()) {
                node.resolvedType = leftType;
                return;
            }
            if (rightType->is<PointerTypeSymbol>()) {
                node.resolvedType = rightType;
                return;
            }
            node.resolvedType = getWiderType(leftType.get(), rightType.get());
            return;
        }
        case BinaryOp::Sub: {
            // Pointer arithmetic: ptr - int → ptr
            if (leftType->is<PointerTypeSymbol>()) {
                node.resolvedType = leftType;
                return;
            }
            node.resolvedType = getWiderType(leftType.get(), rightType.get());
            return;
        }
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
// Move Expression — move(x) returns same type as operand
// ============================================================================

void TypeChecker::visit(MoveExpression& node) {
    if (node.operand) node.operand->accept(*this);

    auto opType = node.operand ? node.operand->resolvedType : nullptr;
    if (!opType) {
        node.resolvedType = symbolTable_.getErrorType();
        return;
    }

    // move(x) has the same type as x — the distinction is purely semantic
    node.resolvedType = opType;
}

void TypeChecker::visit(ArrayLiteralExpression& node) {
    if (node.elements.empty()) {
        errors_.error("empty array literal", node.debugInfo);
        node.resolvedType = symbolTable_.getErrorType();
        return;
    }
    TypeSymbolPtr elemType = nullptr;
    for (auto& elem : node.elements) {
        elem->accept(*this);
        if (!elemType) {
            elemType = elem->resolvedType;
        } else if (elem->resolvedType &&
                   !symbolTable_.isCompatible(elem->resolvedType.get(), elemType.get())) {
            errors_.error("array literal elements must all have the same type",
                          node.debugInfo);
            node.resolvedType = symbolTable_.getErrorType();
            return;
        }
    }
    node.resolvedType = symbolTable_.getArrayType(elemType, (int)node.elements.size());
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

    // Validate mutability (const enforcement)
    if (node.target && node.target->resolvedSymbol) {
        if (auto* varSym = node.target->resolvedSymbol->as<VariableSymbol>()) {
            if (!varSym->isMutable) {
                errors_.error("assignment to const variable '" + varSym->getName() + "'",
                    node.debugInfo);
            }
        }
    }

    // Validate const pointer (cannot write through const T*)
    if (auto* memAccess = node.target->as<MemberAccessExpression>()) {
        if (memAccess->object && memAccess->object->resolvedType) {
            if (auto* ptrType = memAccess->object->resolvedType->as<PointerTypeSymbol>()) {
                if (ptrType->isConst) {
                    errors_.error("cannot modify object through const pointer",
                        node.debugInfo);
                }
            }
        }
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

    // String indexing: str[i] → char
    if (auto* primType = objType->as<PrimitiveTypeSymbol>()) {
        if (primType->primitiveKind == PrimitiveKind::String) {
            node.resolvedType = symbolTable_.getCharType();
            return;
        }
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

    // Direct function call (identifier or member access resolved to FunctionSymbol)
    if (node.callee->resolvedSymbol) {
        auto* fSym = node.callee->resolvedSymbol->as<FunctionSymbol>();
        if (fSym) {
            // Overload resolution: if this function has overloads, find best match
            if (fSym->hasOverloads) {
                std::vector<std::shared_ptr<FunctionSymbol>> overloads;

                // Case 1: Free function call via identifier
                if (auto* identCallee = node.callee->as<IdentifierExpression>()) {
                    auto* scope = node.callee->astScopeNode.get();
                    auto* baseScope = scope ? dynamic_cast<BaseScope*>(scope) : nullptr;
                    if (baseScope) {
                        overloads = baseScope->resolveFunctions(identCallee->name);
                    }
                }
                // Case 2: Method call via member access
                else if (auto* memberAccess = node.callee->as<MemberAccessExpression>()) {
                    if (memberAccess->object) {
                        auto objType = memberAccess->object->resolvedType;
                        if (objType) {
                            if (auto* ptrT = objType->as<PointerTypeSymbol>())
                                objType = ptrT->baseType;
                            auto* typeScope = dynamic_cast<BaseScope*>(objType.get());
                            if (typeScope) {
                                overloads = typeScope->resolveFunctions(
                                    memberAccess->memberName);
                            }
                        }
                    }
                }

                if (overloads.size() > 1) {
                    auto bestMatch = resolveOverload(overloads, node.arguments);
                    if (bestMatch) {
                        node.callee->resolvedSymbol = bestMatch;
                        fSym = bestMatch.get();
                        funcSym = bestMatch;
                        funcTypeHolder = bestMatch->buildFunctionType();
                        if (funcTypeHolder) funcType = funcTypeHolder.get();
                    }
                }
            }
            if (!funcSym) {
                funcSym = std::dynamic_pointer_cast<FunctionSymbol>(node.callee->resolvedSymbol);
                funcTypeHolder = fSym->buildFunctionType();
                if (funcTypeHolder) funcType = funcTypeHolder.get();
            }

            // Generic function monomorphization
            if (fSym->isGenericTemplate()) {
                if (node.typeArguments.empty()) {
                    errors_.error("generic function '" + fSym->getName()
                                   + "' requires explicit type arguments (use ::<Type>)",
                                   node.debugInfo);
                    node.resolvedType = symbolTable_.getErrorType();
                    return;
                }
                if (node.typeArguments.size() != fSym->typeParameterNames.size()) {
                    errors_.error("generic function '" + fSym->getName()
                                   + "' expects " + std::to_string(fSym->typeParameterNames.size())
                                   + " type argument(s), got "
                                   + std::to_string(node.typeArguments.size()),
                                   node.debugInfo);
                    node.resolvedType = symbolTable_.getErrorType();
                    return;
                }

                // Collect resolved type arguments
                std::vector<TypeSymbolPtr> typeArgs;
                for (auto& ta : node.typeArguments) {
                    if (!ta || !ta->resolvedType) {
                        node.resolvedType = symbolTable_.getErrorType();
                        return;
                    }
                    typeArgs.push_back(ta->resolvedType);
                }

                // Get or create monomorphized function
                auto monoFunc = symbolTable_.getOrCreateMonomorphization(fSym, typeArgs);
                node.resolvedCallee = monoFunc;
                funcSym = monoFunc;
                fSym = monoFunc.get();
                funcTypeHolder = monoFunc->buildFunctionType();
                if (funcTypeHolder) funcType = funcTypeHolder.get();
            }
        }
    }

    // Method call via member access (fallback for unresolved symbols)
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

    // String builtin method call
    if (!funcType && node.callee->is<MemberAccessExpression>()) {
        auto* memberAccess = node.callee->as<MemberAccessExpression>();
        if (memberAccess->isStringBuiltinMethod) {
            // Resolve return type based on method name
            if (memberAccess->memberName == "length" ||
                memberAccess->memberName == "charAt" ||
                memberAccess->memberName == "indexOf" ||
                memberAccess->memberName == "toInt") {
                node.resolvedType = symbolTable_.getIntType();
            } else if (memberAccess->memberName == "toDouble") {
                node.resolvedType = symbolTable_.getDoubleType();
            } else if (memberAccess->memberName == "substring") {
                node.resolvedType = symbolTable_.getStringType();
            } else {
                node.resolvedType = symbolTable_.getIntType();
            }
            return;
        }

        // String object method call
        if (memberAccess->isStringObjectMethod) {
            const auto& name = memberAccess->memberName;
            if (name == "length" || name == "capacity" || name == "charAt" ||
                name == "indexOf" || name == "toInt") {
                node.resolvedType = symbolTable_.getIntType();
            } else if (name == "toDouble") {
                node.resolvedType = symbolTable_.getDoubleType();
            } else if (name == "contains") {
                node.resolvedType = symbolTable_.getBoolType();
            } else if (name == "slice") {
                node.resolvedType = symbolTable_.getStringObjectType();
            } else if (name == "cstr") {
                node.resolvedType = symbolTable_.getStringType();
            } else {
                node.resolvedType = symbolTable_.getIntType();
            }
            return;
        }
    }

    // Closure/function variable call (callee type is FunctionTypeSymbol)
    if (!funcType) {
        funcType = calleeType->as<FunctionTypeSymbol>();
    }

    // Struct construction: StructName() → zero-initialized struct value
    if (!funcType && calleeType->is<StructSymbol>()) {
        node.resolvedType = std::dynamic_pointer_cast<TypeSymbol>(
            symbolTable_.resolveType(calleeType->getName()));
        return;
    }

    // Constructor call: callee is a type name
    if (!funcType && calleeType->is<ClassSymbol>()) {
        auto* classSym = calleeType->as<ClassSymbol>();
        if (classSym && !classSym->constructors.empty()) {
            if (classSym->constructors.size() > 1) {
                // Overload resolution among constructors
                std::vector<std::shared_ptr<FunctionSymbol>> candidates(
                    classSym->constructors.begin(), classSym->constructors.end());
                auto best = resolveOverload(candidates, node.arguments);
                if (best) {
                    funcSym = best;
                    funcTypeHolder = best->buildFunctionType();
                    if (funcTypeHolder) funcType = funcTypeHolder.get();
                }
            } else {
                funcSym = classSym->constructors[0];
                funcTypeHolder = classSym->constructors[0]->buildFunctionType();
                if (funcTypeHolder) funcType = funcTypeHolder.get();
            }
        }
        // Result of constructor call is the class type (stack-allocated value)
        // Only 'new' expressions produce pointer types
        node.resolvedType = std::dynamic_pointer_cast<TypeSymbol>(
            symbolTable_.resolveType(classSym->getName()));
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

    // Tagged union variant constructor: Result.Ok(42)
    if (!funcType) {
        // Via QualifiedNameExpression path (patterns)
        auto* qnameCallee = node.callee->as<QualifiedNameExpression>();
        // Via MemberAccessExpression path (expressions)
        auto* memCallee = node.callee->as<MemberAccessExpression>();

        std::shared_ptr<TaggedUnionSymbol> tuSym;
        std::string variantName;
        if (qnameCallee && qnameCallee->isTaggedUnionVariant && qnameCallee->resolvedTaggedUnion) {
            tuSym = qnameCallee->resolvedTaggedUnion;
            variantName = qnameCallee->parts.back();
        } else if (memCallee && memCallee->isTaggedUnionVariant && memCallee->resolvedTaggedUnion) {
            tuSym = memCallee->resolvedTaggedUnion;
            variantName = memCallee->memberName;
        }

        if (tuSym) {
            auto* variant = tuSym->findVariant(variantName);
            if (variant) {
                // Check argument count matches variant field count
                size_t argCount = node.arguments ? node.arguments->expressions.size() : 0;
                size_t fieldCount = variant->fields.size();
                if (argCount != fieldCount) {
                    errors_.error("variant '" + variant->name + "' expects "
                        + std::to_string(fieldCount) + " arguments, got "
                        + std::to_string(argCount), node.debugInfo);
                }
                // Type-check each argument against variant field type
                if (node.arguments) {
                    node.arguments->isReference.clear();
                    for (size_t i = 0; i < node.arguments->expressions.size(); i++) {
                        node.arguments->isReference.push_back(false);  // no ref params for variants
                        if (i < variant->fields.size() && node.arguments->expressions[i]) {
                            auto argType = node.arguments->expressions[i]->resolvedType;
                            auto fieldType = variant->fields[i].type;
                            if (argType && fieldType) {
                                checkAssignability(argType.get(), fieldType.get(),
                                    node.arguments->expressions[i]->debugInfo, "variant argument");
                            }
                        }
                    }
                }
                node.resolvedType = tuSym;
                return;
            }
        }
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
    if (funcType->isVariadic) {
        if (argCount < paramCount) {
            errors_.error("expected at least " + std::to_string(paramCount) + " arguments, got "
                + std::to_string(argCount), node.debugInfo);
        }
    } else if (argCount != paramCount) {
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
        auto* allocType = node.type->resolvedType.get();

        // Validate 'new shared' — classes only, no arrays
        if (node.isShared) {
            if (node.isArray) {
                errors_.error("shared arrays are not supported", node.debugInfo);
                node.resolvedType = symbolTable_.getErrorType();
                return;
            }
            if (!allocType->is<ClassSymbol>()) {
                errors_.error("'new shared' can only be used with class types", node.debugInfo);
                node.resolvedType = symbolTable_.getErrorType();
                return;
            }
            node.resolvedType = symbolTable_.getSharedPointerType(node.type->resolvedType);
        } else if (node.isArray) {
            node.resolvedType = symbolTable_.getPointerType(node.type->resolvedType);
        } else {
            node.resolvedType = symbolTable_.getPointerType(node.type->resolvedType);
        }

        // Resolve constructor overload for class types
        if (auto* classSym = allocType->as<ClassSymbol>()) {
            if (classSym->constructors.size() > 1) {
                std::vector<std::shared_ptr<FunctionSymbol>> candidates(
                    classSym->constructors.begin(), classSym->constructors.end());
                auto best = resolveOverload(candidates, node.arguments);
                if (best) {
                    node.resolvedConstructor = best;
                }
            } else if (!classSym->constructors.empty()) {
                node.resolvedConstructor = classSym->constructors[0];
            }

            // Set isReference on arguments based on resolved constructor
            if (node.resolvedConstructor && node.arguments) {
                auto ctorFuncType = node.resolvedConstructor->buildFunctionType();
                if (ctorFuncType) {
                    node.arguments->isReference.clear();
                    for (size_t i = 0; i < node.arguments->expressions.size(); i++) {
                        bool isRef = (i < ctorFuncType->parameters.size())
                            ? ctorFuncType->parameters[i].isReference : false;
                        node.arguments->isReference.push_back(isRef);
                    }
                }
            }
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
        // Resolve pattern values and bindings
        if (arm.pattern) {
            // Literal patterns: type-check the value expression (e.g., Enum.Member)
            if (auto* litPat = arm.pattern->as<LiteralPattern>()) {
                if (litPat->value) litPat->value->accept(*this);
            }

            if (auto* idPat = arm.pattern->as<IdentifierPattern>()) {
                if (idPat->resolvedSymbol && node.subject && node.subject->resolvedType) {
                    idPat->resolvedSymbol->setType(node.subject->resolvedType);
                }
                if (idPat->guard) idPat->guard->accept(*this);
            }

            // Variant patterns: resolve tagged union + set binding types
            if (auto* varPat = arm.pattern->as<VariantPattern>()) {
                if (varPat->variantPath.size() >= 2) {
                    // Resolve the tagged union type from the path
                    auto* scope = arm.pattern->astScopeNode
                        ? arm.pattern->astScopeNode.get() : nullptr;
                    if (scope) {
                        auto tuSym = scope->resolve(varPat->variantPath[0]);
                        auto* tuType = tuSym ? tuSym->as<TaggedUnionSymbol>() : nullptr;
                        if (tuType) {
                            varPat->resolvedUnion = std::dynamic_pointer_cast<TaggedUnionSymbol>(
                                symbolTable_.resolveType(tuType->getName()));
                            auto* variant = tuType->findVariant(varPat->variantPath[1]);
                            if (variant) {
                                varPat->resolvedVariantIndex = variant->tagValue;

                                // Set binding variable types from variant fields
                                for (size_t fi = 0; fi < varPat->fieldPatterns.size(); fi++) {
                                    auto& fp = varPat->fieldPatterns[fi];
                                    if (!fp) continue;
                                    if (auto* idFP = fp->as<IdentifierPattern>()) {
                                        if (idFP->resolvedSymbol && fi < variant->fields.size()) {
                                            idFP->resolvedSymbol->setType(variant->fields[fi].type);
                                        }
                                    }
                                }
                            } else {
                                errors_.error("variant '" + varPat->variantPath[1]
                                    + "' not found in tagged union '"
                                    + tuType->getName() + "'", arm.pattern->debugInfo);
                            }
                        } else {
                            errors_.error("'" + varPat->variantPath[0]
                                + "' is not a tagged union", arm.pattern->debugInfo);
                        }
                    }
                }
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

// ============================================================================
// Overload resolution
// ============================================================================

int TypeChecker::scoreOverloadMatch(FunctionSymbol* func,
                                     const std::shared_ptr<ArgumentsNode>& args)
{
    size_t argCount = args ? args->expressions.size() : 0;
    size_t paramCount = func->parameters.size();

    // Skip 'this' param for method calls — 'this' is implicit
    // (methods called via identifier have hasThisParam=true but the this arg
    //  is added by codegen, not by the caller in the argument list)

    // Variadics: at least paramCount args
    if (func->isVariadic) {
        if (argCount < paramCount) return -1;  // too few args
    } else {
        if (argCount != paramCount) return -1;  // wrong count
    }

    int score = 0;  // 0 = perfect, higher = worse
    for (size_t i = 0; i < paramCount && i < argCount; i++) {
        auto argType = args->expressions[i]->resolvedType;
        auto paramType = func->parameters[i]->getType();
        if (!argType || !paramType) return -1;

        TypeSymbol* argT = argType->as<TypeSymbol>();
        TypeSymbol* paramT = paramType->as<TypeSymbol>();
        if (!argT || !paramT) return -1;

        // Exact match: score 0
        if (argT == paramT) continue;

        // Compatible match (widening): score penalty
        if (symbolTable_.isCompatible(argT, paramT)) {
            score += 10;  // compatible but not exact
            continue;
        }

        // No match at all
        return -1;
    }

    return score;
}

std::shared_ptr<FunctionSymbol> TypeChecker::resolveOverload(
    const std::vector<std::shared_ptr<FunctionSymbol>>& candidates,
    const std::shared_ptr<ArgumentsNode>& args)
{
    std::shared_ptr<FunctionSymbol> bestMatch;
    int bestScore = INT_MAX;
    bool ambiguous = false;

    for (auto& candidate : candidates) {
        int score = scoreOverloadMatch(candidate.get(), args);
        if (score < 0) continue;  // not a match

        if (score < bestScore) {
            bestScore = score;
            bestMatch = candidate;
            ambiguous = false;
        } else if (score == bestScore) {
            ambiguous = true;
        }
    }

    if (ambiguous) {
        // Return first match anyway — error reported by caller if needed
        return bestMatch;
    }

    return bestMatch;
}

} // namespace mingus
