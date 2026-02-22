// ============================================================================
// SemanticValidator.cpp — Pass 4: Semantic validation and capture analysis
//
// Five responsibilities:
//   4a: Lambda capture resolution (the most critical invariant)
//   4b: RAII variable tracking (destructor calls at scope exit)
//   4c: Return completeness checking
//   4d: Control flow validation (break/continue in loops)
//   4e: Class/interface implementation checking + match exhaustiveness
// ============================================================================

#include "mingus/sema/SemanticValidator.h"

namespace mingus {

SemanticValidator::SemanticValidator(SymbolTable& symbolTable, ErrorReporter& errors)
    : symbolTable_(symbolTable)
    , errors_(errors) {}

// ============================================================================
// Entry point
// ============================================================================

void SemanticValidator::validate(ProgramNode& program) {
    visit(program);
}

const std::unordered_map<Scope*, ScopeRAIIInfo>&
SemanticValidator::getRAIIInfo() const {
    return raiiInfo_;
}

// ============================================================================
// Helpers
// ============================================================================

void SemanticValidator::visitStatements(
    std::vector<std::shared_ptr<StatementBaseNode>>& stmts)
{
    for (auto& stmt : stmts) {
        if (stmt) stmt->accept(*this);
    }
}

// ============================================================================
// RAII Tracking — 4b
//
// If a variable's type is a ClassSymbol with a destructor, track it for
// scope-exit cleanup. Codegen reverses for LIFO destruction order.
// ============================================================================

void SemanticValidator::trackRAIIVariable(VariableSymbol* var) {
    if (!var || !var->getType() || !currentScope_) return;

    auto* cls = var->getType()->as<ClassSymbol>();
    if (!cls || !cls->hasRAII()) return;

    auto& info = raiiInfo_[currentScope_];
    info.destructibles.push_back({var, cls->destructor.get()});
}

// ============================================================================
// Lambda Capture Analysis — 4a
//
// CRITICAL INVARIANT: Walk the ENTIRE lambdaStack_ from innermost to
// outermost. Each lambda that doesn't own the variable locally must
// capture it. Without full-stack propagation, nested lambdas crash
// (outer lambda gets null env → segfault at runtime).
// ============================================================================

void SemanticValidator::checkLambdaCapture(IdentifierExpression& node) {
    if (lambdaStack_.empty()) return;
    if (!node.resolvedSymbol) return;

    // Only capture variables and parameters (not functions, types, etc.)
    auto* varSym = node.resolvedSymbol->as<VariableSymbol>();
    if (!varSym) return;

    // Only capture locals and parameters (not fields)
    if (varSym->role == VariableRole::Field) return;

    // Walk from innermost lambda outward
    for (int i = static_cast<int>(lambdaStack_.size()) - 1; i >= 0; --i) {
        auto& ctx = lambdaStack_[i];
        auto* lambda = ctx.lambda;

        // If this lambda owns the variable locally (param or inner decl), stop
        if (ctx.localSymbols.count(node.resolvedSymbol.get()) > 0) {
            break;
        }

        // Determine capture mode from lambda's capture specification
        CaptureMode mode = CaptureMode::ByValue;
        bool allowed = false;

        // Check explicit capture items: [x], [&x]
        for (auto& item : lambda->captureItems) {
            if (item.name == varSym->getName()) {
                mode = item.mode;
                allowed = true;
                break;
            }
        }

        // Fall back to capture default: [=] or [&]
        if (!allowed) {
            if (lambda->captureDefault == CaptureDefault::ByCopy) {
                mode = CaptureMode::ByValue;
                allowed = true;
            } else if (lambda->captureDefault == CaptureDefault::ByRef) {
                mode = CaptureMode::ByReference;
                allowed = true;
            }
        }

        // If capture not allowed at this level, stop propagation
        if (!allowed) {
            // Empty capture list [] — variable is not capturable
            if (lambda->captureDefault == CaptureDefault::None
                && lambda->captureItems.empty()) {
                // Empty [] means no captures — this is fine if variable is
                // defined above all lambdas. But if we got here, it means
                // the variable is from an outer scope and not captured.
                // Report only for the innermost lambda that needs it.
                if (i == static_cast<int>(lambdaStack_.size()) - 1) {
                    errors_.error("variable '" + varSym->getName()
                        + "' not captured in lambda with empty capture list",
                        node.debugInfo);
                }
            }
            break;
        }

        // Check if already captured (avoid duplicates)
        bool alreadyCaptured = false;
        for (auto& captured : lambda->capturedVariables) {
            if (captured.get() == node.resolvedSymbol.get()) {
                alreadyCaptured = true;
                break;
            }
        }

        // Validate weak captures: only closure-typed variables can be weak-captured
        if (mode == CaptureMode::Weak) {
            if (!varSym->getType() || !varSym->getType()->is<FunctionTypeSymbol>()) {
                errors_.error("'weak' capture can only be used with closure-typed variables, "
                    "but '" + varSym->getName() + "' is not a closure",
                    node.debugInfo);
                break;
            }
        }

        if (!alreadyCaptured) {
            lambda->capturedVariables.push_back(node.resolvedSymbol);
            lambda->captureModesResolved.push_back(mode);
            if (mode == CaptureMode::ByReference) {
                lambda->hasRefCaptures = true;
            }
        }
    }
}

// ============================================================================
// Self-capture detection — letrec pattern
//
// var f = [=](int x) => { return f(x - 1); };
// If the lambda captures the variable it's being assigned to, mark selfCapture.
// ============================================================================

void SemanticValidator::checkSelfCapture(VariableDeclaration& node) {
    if (!node.initializer || !node.resolvedVariable) return;

    auto* lambda = node.initializer->as<LambdaExpression>();
    if (!lambda) return;

    for (auto& captured : lambda->capturedVariables) {
        if (captured.get() == node.resolvedVariable.get()) {
            lambda->selfCapture = true;
            break;
        }
    }
}

// ============================================================================
// Return Completeness — 4c
//
// Three-level reachability: AlwaysReturns, SometimesReturns, NeverReturns.
// Non-void functions must have AlwaysReturns from their body.
// ============================================================================

Reachability SemanticValidator::classifyStatement(StatementBaseNode* stmt) {
    if (!stmt) return Reachability::NeverReturns;

    if (stmt->is<ReturnStatement>()) {
        return Reachability::AlwaysReturns;
    }

    if (auto* block = stmt->as<BlockStatementNode>()) {
        return classifyBlock(block);
    }

    if (auto* ifStmt = stmt->as<IfStatement>()) {
        // Must have else clause for any chance of AlwaysReturns
        if (!ifStmt->elseBody) return Reachability::NeverReturns;

        auto thenReach = classifyStatement(ifStmt->thenBody.get());
        auto elseReach = classifyStatement(ifStmt->elseBody.get());

        // Check else-if clauses
        bool allReturn = (thenReach == Reachability::AlwaysReturns);
        bool someReturn = (thenReach != Reachability::NeverReturns);

        for (auto& elseIf : ifStmt->elseIfClauses) {
            auto reach = classifyStatement(elseIf.body.get());
            if (reach != Reachability::AlwaysReturns) allReturn = false;
            if (reach != Reachability::NeverReturns) someReturn = true;
        }

        if (elseReach != Reachability::AlwaysReturns) allReturn = false;
        if (elseReach != Reachability::NeverReturns) someReturn = true;

        if (allReturn) return Reachability::AlwaysReturns;
        if (someReturn) return Reachability::SometimesReturns;
        return Reachability::NeverReturns;
    }

    if (auto* switchStmt = stmt->as<SwitchStatement>()) {
        // Needs default case + all cases returning
        if (switchStmt->defaultCase.empty()) return Reachability::NeverReturns;

        bool allReturn = true;
        bool someReturn = false;

        for (auto& c : switchStmt->cases) {
            // Classify the case body statements
            bool caseReturns = false;
            for (auto& s : c.body) {
                if (classifyStatement(s.get()) == Reachability::AlwaysReturns) {
                    caseReturns = true;
                    break;
                }
            }
            if (!caseReturns) allReturn = false;
            else someReturn = true;
        }

        // Classify default case
        bool defaultReturns = false;
        for (auto& s : switchStmt->defaultCase) {
            if (classifyStatement(s.get()) == Reachability::AlwaysReturns) {
                defaultReturns = true;
                break;
            }
        }
        if (!defaultReturns) allReturn = false;
        else someReturn = true;

        if (allReturn) return Reachability::AlwaysReturns;
        if (someReturn) return Reachability::SometimesReturns;
        return Reachability::NeverReturns;
    }

    // For/While loops never guarantee return (body might not execute)
    return Reachability::NeverReturns;
}

Reachability SemanticValidator::classifyBlock(BlockStatementNode* block) {
    if (!block) return Reachability::NeverReturns;

    for (auto& stmt : block->statements) {
        auto reach = classifyStatement(stmt.get());
        if (reach == Reachability::AlwaysReturns) {
            return Reachability::AlwaysReturns;
        }
        if (reach == Reachability::SometimesReturns) {
            return Reachability::SometimesReturns;
        }
    }
    return Reachability::NeverReturns;
}

void SemanticValidator::checkReturnCompleteness(
    const std::string& funcName,
    TypeSymbolPtr returnType,
    BlockStatementNode* body,
    const std::shared_ptr<DebugInfo>& loc)
{
    if (!returnType || returnType->getName() == "void") return;
    if (!body) return;

    auto reach = classifyBlock(body);
    if (reach != Reachability::AlwaysReturns) {
        errors_.warning("not all code paths in '" + funcName + "' return a value", loc);
    }
}

// ============================================================================
// Class/Interface checking — 4e
// ============================================================================

void SemanticValidator::checkAbstractImplementation(
    ClassSymbol* cls,
    const std::shared_ptr<DebugInfo>& loc)
{
    if (!cls || cls->isAbstract) return;
    if (!cls->resolvedBaseClass) return;

    // Walk up the inheritance chain collecting abstract methods
    std::vector<std::string> unimplemented;
    auto* base = cls->resolvedBaseClass;
    while (base) {
        // Check all symbols in the base scope for abstract functions
        for (auto& sym : base->getAllSymbols()) {
            auto* func = sym->as<FunctionSymbol>();
            if (func && func->isAbstract) {
                // Check if cls implements this method
                auto impl = cls->resolve(func->getName());
                auto* implFunc = impl ? impl->as<FunctionSymbol>() : nullptr;
                if (!implFunc || implFunc->isAbstract) {
                    unimplemented.push_back(func->getName());
                }
            }
        }
        base = base->resolvedBaseClass;
    }

    for (auto& name : unimplemented) {
        errors_.error("concrete class '" + cls->getName()
            + "' does not implement abstract method '" + name + "'", loc);
    }
}

void SemanticValidator::checkInterfaceImplementation(
    ClassSymbol* cls,
    InterfaceSymbol* iface,
    const std::shared_ptr<DebugInfo>& loc)
{
    if (!cls || !iface) return;

    for (auto& ifaceMethod : iface->methods) {
        auto impl = cls->resolve(ifaceMethod->getName());
        if (!impl || !impl->as<FunctionSymbol>()) {
            errors_.error("class '" + cls->getName()
                + "' does not implement interface method '"
                + ifaceMethod->getName() + "' from '"
                + iface->getName() + "'", loc);
        }
    }
}

// ============================================================================
// Override return type covariance — 4e
// ============================================================================

void SemanticValidator::checkOverrideCovariance(
    ClassSymbol* cls,
    const std::shared_ptr<DebugInfo>& loc)
{
    if (!cls || !cls->resolvedBaseClass) return;

    // Compare each vtable slot against the base class vtable
    auto* base = cls->resolvedBaseClass;
    size_t baseVtableSize = base->vtable.size();

    for (size_t i = 1; i < std::min(cls->vtable.size(), baseVtableSize); i++) {
        auto& childFunc = cls->vtable[i];
        auto& parentFunc = base->vtable[i];
        if (!childFunc || !parentFunc) continue;

        // Skip if same function (not overridden)
        if (childFunc.get() == parentFunc.get()) continue;

        auto* childRet = childFunc->returnType.get();
        auto* parentRet = parentFunc->returnType.get();
        if (!childRet || !parentRet) continue;

        // Same type → OK
        if (childRet == parentRet) continue;

        // Covariant: child return type is compatible with parent return type
        if (symbolTable_.isCompatible(childRet, parentRet)) continue;

        // Incompatible return type
        errors_.error("override '" + childFunc->getName()
            + "' returns '" + childRet->getName()
            + "' but base method returns '" + parentRet->getName()
            + "' (not covariant)", loc);
    }
}

// ============================================================================
// Match exhaustiveness — 4f
// ============================================================================

void SemanticValidator::checkExhaustiveness(MatchExpression& node) {
    bool hasWildcard = false;

    for (auto& arm : node.arms) {
        if (!arm.pattern) continue;

        // Wildcard or unguarded identifier pattern = exhaustive
        if (arm.pattern->is<WildcardPattern>()) {
            hasWildcard = true;
            break;
        }
        if (auto* idPat = arm.pattern->as<IdentifierPattern>()) {
            if (!idPat->guard) {
                hasWildcard = true;  // Unguarded binding = matches anything
                break;
            }
        }
    }

    if (hasWildcard) return;

    // Check enum coverage
    if (node.subject && node.subject->resolvedType) {
        auto* enumSym = node.subject->resolvedType->as<EnumSymbol>();
        if (enumSym) {
            std::set<std::string> covered;
            for (auto& arm : node.arms) {
                if (!arm.pattern) continue;
                if (auto* litPat = arm.pattern->as<LiteralPattern>()) {
                    if (litPat->value) {
                        if (auto* ident = litPat->value->as<IdentifierExpression>()) {
                            covered.insert(ident->name);
                        }
                    }
                }
            }

            std::vector<std::string> missing;
            for (auto& member : enumSym->members) {
                if (covered.find(member.name) == covered.end()) {
                    missing.push_back(member.name);
                }
            }

            if (!missing.empty()) {
                std::string msg = "non-exhaustive match on enum '"
                    + enumSym->getName() + "': missing ";
                for (size_t i = 0; i < missing.size(); i++) {
                    if (i > 0) msg += ", ";
                    msg += "'" + missing[i] + "'";
                }
                errors_.warning(msg, node.debugInfo);
            }
            return;
        }
    }

    // Non-enum without wildcard
    errors_.warning("match expression may not be exhaustive", node.debugInfo);
}

// ============================================================================
// Program & Module
// ============================================================================

void SemanticValidator::visit(ProgramNode& node) {
    for (auto& module : node.modules) {
        if (module) module->accept(*this);
    }
}

void SemanticValidator::visit(ModuleNode& node) {
    auto savedScope = currentScope_;
    if (node.resolvedModule) {
        currentScope_ = node.resolvedModule.get();
    }

    for (auto& decl : node.declarations) {
        if (decl) decl->accept(*this);
    }

    currentScope_ = savedScope;
}

void SemanticValidator::visit(BlockStatementNode& node) {
    auto savedScope = currentScope_;
    if (node.astScopeNode) {
        currentScope_ = node.astScopeNode.get();
    }

    visitStatements(node.statements);

    currentScope_ = savedScope;
}

// ============================================================================
// Variable Declarations — RAII tracking + lambda capture context
// ============================================================================

void SemanticValidator::visit(VariableDeclaration& node) {
    // Visit initializer first (may contain lambdas, identifiers)
    if (node.initializer) {
        node.initializer->accept(*this);
    }

    // Track as local symbol in current lambda context
    if (!lambdaStack_.empty() && node.resolvedVariable) {
        lambdaStack_.back().localSymbols.insert(node.resolvedVariable.get());
    }

    // RAII tracking for local class-type variables with destructors
    if (node.resolvedVariable
        && node.resolvedVariable->role == VariableRole::Local) {
        trackRAIIVariable(node.resolvedVariable.get());
    }

    // Definite assignment: mark as assigned if has initializer
    if (node.resolvedVariable && node.initializer) {
        definitelyAssigned_.insert(node.resolvedVariable.get());
    }

    // Self-capture detection
    checkSelfCapture(node);

    // Track variables that hold closures with by-reference captures
    if (node.resolvedVariable && node.initializer) {
        if (auto* lambda = node.initializer->as<LambdaExpression>()) {
            if (lambda->hasRefCaptures) {
                refCaptureVars_.insert(node.resolvedVariable.get());
            }
        }
    }
}

void SemanticValidator::visit(VariableDeclarationExpression& node) {
    if (node.initializer) {
        node.initializer->accept(*this);
    }

    if (!lambdaStack_.empty() && node.resolvedVariable) {
        lambdaStack_.back().localSymbols.insert(node.resolvedVariable.get());
    }

    if (node.resolvedVariable
        && node.resolvedVariable->role == VariableRole::Local) {
        trackRAIIVariable(node.resolvedVariable.get());
    }

    // Definite assignment: mark as assigned if has initializer
    if (node.resolvedVariable && node.initializer) {
        definitelyAssigned_.insert(node.resolvedVariable.get());
    }
}

void SemanticValidator::visit(TupleDestructuringDeclaration& node) {
    if (node.initializer) {
        node.initializer->accept(*this);
    }

    // Track each destructured variable in lambda context
    if (!lambdaStack_.empty()) {
        for (auto& var : node.resolvedVariables) {
            if (var) lambdaStack_.back().localSymbols.insert(var.get());
        }
    }

    // Definite assignment: all destructured elements are assigned
    if (node.initializer) {
        for (auto& var : node.resolvedVariables) {
            if (var) definitelyAssigned_.insert(var.get());
        }
    }
}

// ============================================================================
// Function Declarations — enter body, check return completeness
// ============================================================================

void SemanticValidator::visit(FunctionDeclaration& node) {
    if (!node.body) return;

    auto savedScope = currentScope_;
    auto savedReturnType = currentReturnType_;
    auto savedAssigned = definitelyAssigned_;

    if (node.resolvedFunction) {
        currentScope_ = node.resolvedFunction.get();
        currentReturnType_ = node.resolvedFunction->returnType;

        // Mark all parameters as definitely assigned
        for (auto& param : node.resolvedFunction->parameters) {
            if (param) definitelyAssigned_.insert(param.get());
        }
    }

    visitStatements(node.body->statements);

    // Check return completeness
    if (node.resolvedFunction) {
        checkReturnCompleteness(node.name, node.resolvedFunction->returnType,
            node.body.get(), node.debugInfo);
    }

    currentScope_ = savedScope;
    currentReturnType_ = savedReturnType;
    definitelyAssigned_ = savedAssigned;
}

void SemanticValidator::visit(ConstructorDeclaration& node) {
    if (!node.body) return;

    auto savedScope = currentScope_;
    auto savedReturnType = currentReturnType_;
    auto savedAssigned = definitelyAssigned_;

    if (node.resolvedConstructor) {
        currentScope_ = node.resolvedConstructor.get();

        // Mark all parameters as definitely assigned
        for (auto& param : node.resolvedConstructor->parameters) {
            if (param) definitelyAssigned_.insert(param.get());
        }
    }
    currentReturnType_ = symbolTable_.getVoidType();  // constructors return void

    visitStatements(node.body->statements);

    currentScope_ = savedScope;
    currentReturnType_ = savedReturnType;
    definitelyAssigned_ = savedAssigned;
}

void SemanticValidator::visit(DestructorDeclaration& node) {
    if (!node.body) return;

    auto savedScope = currentScope_;
    auto savedReturnType = currentReturnType_;

    if (node.resolvedDestructor) {
        currentScope_ = node.resolvedDestructor.get();
    }
    currentReturnType_ = symbolTable_.getVoidType();

    visitStatements(node.body->statements);

    currentScope_ = savedScope;
    currentReturnType_ = savedReturnType;
}

void SemanticValidator::visit(ExternFunctionDeclaration& node) {
    // No body to validate
}

void SemanticValidator::visit(OperatorDeclaration& node) {
    if (!node.body) return;

    auto savedScope = currentScope_;
    auto savedReturnType = currentReturnType_;

    if (node.resolvedOperator) {
        currentScope_ = node.resolvedOperator.get();
        currentReturnType_ = node.resolvedOperator->returnType;
    }

    visitStatements(node.body->statements);

    if (node.resolvedOperator) {
        checkReturnCompleteness("operator", node.resolvedOperator->returnType,
            node.body.get(), node.debugInfo);
    }

    currentScope_ = savedScope;
    currentReturnType_ = savedReturnType;
}

// ============================================================================
// Type Declarations — check implementations
// ============================================================================

void SemanticValidator::visit(EnumDeclaration& node) {
    // Enum values are constants — nothing to validate here
}

void SemanticValidator::visit(StructDeclaration& node) {
    auto savedScope = currentScope_;
    if (node.resolvedStruct) {
        currentScope_ = node.resolvedStruct.get();
    }

    for (auto& method : node.methods) {
        if (method) method->accept(*this);
    }
    for (auto& op : node.operators) {
        if (op) op->accept(*this);
    }

    currentScope_ = savedScope;
}

void SemanticValidator::visit(ClassDeclaration& node) {
    auto savedScope = currentScope_;
    if (node.resolvedClass) {
        currentScope_ = node.resolvedClass.get();

        // Check abstract method implementation
        checkAbstractImplementation(node.resolvedClass.get(), node.debugInfo);

        // Check interface implementation
        for (auto& iface : node.resolvedClass->implementedInterfaces) {
            checkInterfaceImplementation(node.resolvedClass.get(),
                iface.get(), node.debugInfo);
        }

        // Check override return type covariance
        checkOverrideCovariance(node.resolvedClass.get(), node.debugInfo);
    }

    // Visit field initializers
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

    currentScope_ = savedScope;
}

void SemanticValidator::visit(InterfaceDeclaration& node) {
    // Abstract methods only — no bodies
}

void SemanticValidator::visit(ImportDeclaration& node) {}
void SemanticValidator::visit(TypedefDeclaration& node) {}

// ============================================================================
// Statements
// ============================================================================

void SemanticValidator::visit(ExpressionStatement& node) {
    if (node.expression) node.expression->accept(*this);
}

void SemanticValidator::visit(ReturnStatement& node) {
    if (node.value) node.value->accept(*this);

    // Escape analysis: reject returning closures with by-reference captures
    if (node.value) {
        // Direct lambda return: return [&x](...) => { ... };
        if (auto* lambda = node.value->as<LambdaExpression>()) {
            if (lambda->hasRefCaptures) {
                errors_.error("cannot return closure with by-reference captures "
                    "— captured references would dangle after the function returns",
                    node.debugInfo);
            }
        }
        // Named variable return: var f = [&x](...) => {...}; return f;
        else if (auto* ident = node.value->as<IdentifierExpression>()) {
            if (ident->resolvedSymbol &&
                refCaptureVars_.count(ident->resolvedSymbol.get())) {
                errors_.error("cannot return closure with by-reference captures "
                    "— captured references would dangle after the function returns",
                    node.debugInfo);
            }
        }
    }
}

void SemanticValidator::visit(IfStatement& node) {
    if (node.condition) node.condition->accept(*this);

    // Save state before branches for definite assignment analysis
    auto preIfAssigned = definitelyAssigned_;

    // Then branch
    if (node.thenBody) node.thenBody->accept(*this);
    auto thenAssigned = definitelyAssigned_;

    if (node.elseBody || !node.elseIfClauses.empty()) {
        // Has else: intersect then/else assignments
        // Process else-if clauses
        for (auto& elseIf : node.elseIfClauses) {
            definitelyAssigned_ = preIfAssigned;  // restore for each else-if
            if (elseIf.condition) elseIf.condition->accept(*this);
            if (elseIf.body) elseIf.body->accept(*this);
            // Intersect with thenAssigned
            std::set<VariableSymbol*> intersection;
            for (auto* v : thenAssigned) {
                if (definitelyAssigned_.count(v)) intersection.insert(v);
            }
            thenAssigned = intersection;
        }

        if (node.elseBody) {
            definitelyAssigned_ = preIfAssigned;  // restore for else
            node.elseBody->accept(*this);
            // Final intersection: thenAssigned ∩ elseAssigned
            std::set<VariableSymbol*> intersection;
            for (auto* v : thenAssigned) {
                if (definitelyAssigned_.count(v)) intersection.insert(v);
            }
            definitelyAssigned_ = intersection;
        } else {
            // else-if but no else: conservative, restore pre-if
            definitelyAssigned_ = preIfAssigned;
            // Merge in only what was assigned on ALL branches including "no branch"
        }
    } else {
        // No else: conservative — restore pre-if state (then-only assignments not guaranteed)
        definitelyAssigned_ = preIfAssigned;
    }
}

void SemanticValidator::visit(ForStatement& node) {
    std::string label = pendingLabel_;
    pendingLabel_.clear();
    loopLabelStack_.push_back(label);

    auto savedScope = currentScope_;
    if (node.body && node.body->as<BlockStatementNode>()
        && node.body->as<BlockStatementNode>()->astScopeNode) {
        // For loop may create its own scope
    }

    // Init declarations may introduce assigned variables
    for (auto& initDecl : node.initDeclarations) {
        if (initDecl) initDecl->accept(*this);
    }
    for (auto& initExpr : node.initExpressions) {
        if (initExpr) initExpr->accept(*this);
    }

    // Conservative: save pre-loop state, body assignments are not guaranteed
    auto preLoopAssigned = definitelyAssigned_;
    if (node.condition) node.condition->accept(*this);
    for (auto& iter : node.iterators) {
        if (iter) iter->accept(*this);
    }
    if (node.body) node.body->accept(*this);
    definitelyAssigned_ = preLoopAssigned;

    currentScope_ = savedScope;
    loopLabelStack_.pop_back();
}

void SemanticValidator::visit(WhileStatement& node) {
    std::string label = pendingLabel_;
    pendingLabel_.clear();
    loopLabelStack_.push_back(label);

    // Conservative: loop body may not execute
    auto preLoopAssigned = definitelyAssigned_;
    if (node.condition) node.condition->accept(*this);
    if (node.body) node.body->accept(*this);
    definitelyAssigned_ = preLoopAssigned;

    loopLabelStack_.pop_back();
}

void SemanticValidator::visit(DoWhileStatement& node) {
    std::string label = pendingLabel_;
    pendingLabel_.clear();
    loopLabelStack_.push_back(label);

    // do-while body runs at least once — keep body assignments
    if (node.body) node.body->accept(*this);
    if (node.condition) node.condition->accept(*this);

    loopLabelStack_.pop_back();
}

void SemanticValidator::visit(LabeledStatement& node) {
    // Set the pending label — the inner loop will consume it
    pendingLabel_ = node.label;
    if (node.statement) node.statement->accept(*this);
}

void SemanticValidator::visit(BreakStatement& node) {
    if (loopLabelStack_.empty()) {
        errors_.error("'break' statement outside of loop", node.debugInfo);
        return;
    }
    if (!node.label.empty()) {
        // Labeled break: search stack for matching label
        bool found = false;
        for (auto& lbl : loopLabelStack_) {
            if (lbl == node.label) { found = true; break; }
        }
        if (!found) {
            errors_.error("'break' label '" + node.label
                + "' does not match any enclosing loop", node.debugInfo);
        }
    }
}

void SemanticValidator::visit(ContinueStatement& node) {
    if (loopLabelStack_.empty()) {
        errors_.error("'continue' statement outside of loop", node.debugInfo);
        return;
    }
    if (!node.label.empty()) {
        // Labeled continue: search stack for matching label
        bool found = false;
        for (auto& lbl : loopLabelStack_) {
            if (lbl == node.label) { found = true; break; }
        }
        if (!found) {
            errors_.error("'continue' label '" + node.label
                + "' does not match any enclosing loop", node.debugInfo);
        }
    }
}

void SemanticValidator::visit(DeleteStatement& node) {
    if (node.target) node.target->accept(*this);
}

void SemanticValidator::visit(SwitchStatement& node) {
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
// Expressions — walk sub-expressions + lambda capture analysis
// ============================================================================

void SemanticValidator::visit(IntegerLiteral& node) {}
void SemanticValidator::visit(FloatLiteral& node) {}
void SemanticValidator::visit(BoolLiteral& node) {}
void SemanticValidator::visit(CharLiteral& node) {}
void SemanticValidator::visit(StringLiteral& node) {}
void SemanticValidator::visit(NullLiteral& node) {}

void SemanticValidator::visit(InterpolatedStringExpression& node) {
    for (auto& part : node.parts) {
        if (part.kind == InterpolatedPartKind::Expression && part.expression) {
            part.expression->accept(*this);
        }
    }
}

void SemanticValidator::visit(IdentifierExpression& node) {
    // 4a: Lambda capture analysis — this is the trigger point
    checkLambdaCapture(node);

    // 4f: Definite assignment check — warn on reading uninitialized locals
    if (auto* varSym = node.resolvedSymbol
            ? node.resolvedSymbol->as<VariableSymbol>() : nullptr) {
        if (varSym->role == VariableRole::Local
            && definitelyAssigned_.find(varSym) == definitelyAssigned_.end()) {
            errors_.warning("variable '" + varSym->getName()
                + "' may be used before being assigned", node.debugInfo);
        }
    }
}

void SemanticValidator::visit(QualifiedNameExpression& node) {
    // Qualified names (Module.Type) don't need capture analysis
}

void SemanticValidator::visit(ThisExpression& node) {
    // 'this' doesn't need capture analysis (always available in class methods)
}

void SemanticValidator::visit(MemberAccessExpression& node) {
    if (node.object) node.object->accept(*this);
}

void SemanticValidator::visit(BinaryExpression& node) {
    if (node.left) node.left->accept(*this);
    if (node.right) node.right->accept(*this);
}

void SemanticValidator::visit(UnaryExpression& node) {
    if (node.operand) node.operand->accept(*this);
}

void SemanticValidator::visit(MoveExpression& node) {
    if (node.operand) node.operand->accept(*this);
}

void SemanticValidator::visit(ArrayLiteralExpression& node) {
    for (auto& elem : node.elements) {
        if (elem) elem->accept(*this);
    }
}

void SemanticValidator::visit(AssignmentExpression& node) {
    // Visit value first (RHS), then mark target as assigned, then visit target
    // This order prevents false warnings like "x may be uninitialized" for "x = 42"
    if (node.value) node.value->accept(*this);

    // Definite assignment: mark target as assigned BEFORE visiting target
    if (auto* ident = node.target ? node.target->as<IdentifierExpression>() : nullptr) {
        if (auto* varSym = ident->resolvedSymbol
                ? ident->resolvedSymbol->as<VariableSymbol>() : nullptr) {
            if (varSym->role == VariableRole::Local) {
                definitelyAssigned_.insert(varSym);
            }
        }
    }

    if (node.target) node.target->accept(*this);

    // Escape analysis: reject storing ref-capture closures in fields
    bool isFieldStore = false;
    if (node.target) {
        if (node.target->as<MemberAccessExpression>()) {
            isFieldStore = true;
        } else if (auto* ident = node.target->as<IdentifierExpression>()) {
            if (auto* vs = ident->resolvedSymbol ?
                    ident->resolvedSymbol->as<VariableSymbol>() : nullptr) {
                if (vs->role == VariableRole::Field) isFieldStore = true;
            }
        }
    }
    if (isFieldStore && node.value) {
        if (auto* lambda = node.value->as<LambdaExpression>()) {
            if (lambda->hasRefCaptures) {
                errors_.error("cannot store closure with by-reference captures in a field "
                    "— captured references may outlive their scope",
                    node.debugInfo);
            }
        } else if (auto* ident = node.value->as<IdentifierExpression>()) {
            if (ident->resolvedSymbol &&
                refCaptureVars_.count(ident->resolvedSymbol.get())) {
                errors_.error("cannot store closure with by-reference captures in a field "
                    "— captured references may outlive their scope",
                    node.debugInfo);
            }
        }
    }
}

void SemanticValidator::visit(TernaryExpression& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.thenExpr) node.thenExpr->accept(*this);
    if (node.elseExpr) node.elseExpr->accept(*this);
}

void SemanticValidator::visit(IndexExpression& node) {
    if (node.object) node.object->accept(*this);
    if (node.index) node.index->accept(*this);
}

void SemanticValidator::visit(CallExpression& node) {
    if (node.callee) node.callee->accept(*this);
    if (node.arguments) {
        for (size_t i = 0; i < node.arguments->expressions.size(); i++) {
            auto& arg = node.arguments->expressions[i];
            if (arg) {
                arg->accept(*this);

                // Non-escaping lambda detection: lambdas passed directly
                // as arguments don't escape (can be stack-allocated)
                if (auto* lambda = arg->as<LambdaExpression>()) {
                    lambda->escapes = false;
                }
            }
        }
    }
}

void SemanticValidator::visit(CastExpression& node) {
    if (node.operand) node.operand->accept(*this);
}

void SemanticValidator::visit(NewExpression& node) {
    if (node.arguments) {
        for (auto& arg : node.arguments->expressions) {
            if (arg) arg->accept(*this);
        }
    }
    if (node.arraySize) node.arraySize->accept(*this);
}

void SemanticValidator::visit(SizeOfExpression& node) {}

void SemanticValidator::visit(TupleExpression& node) {
    for (auto& elem : node.elements) {
        if (elem) elem->accept(*this);
    }
}

void SemanticValidator::visit(MatchExpression& node) {
    // Exhaustiveness check
    checkExhaustiveness(node);

    if (node.subject) node.subject->accept(*this);
    for (auto& arm : node.arms) {
        if (arm.pattern) arm.pattern->accept(*this);
        if (arm.body) arm.body->accept(*this);
    }
}

void SemanticValidator::visit(PipeExpression& node) {
    if (node.input) node.input->accept(*this);
    for (auto& stage : node.stages) {
        if (stage.function) stage.function->accept(*this);
        for (auto& arg : stage.extraArguments) {
            if (arg) arg->accept(*this);
        }
    }
}

// ============================================================================
// Lambda Expression — Push capture context, visit body, pop
//
// This is where the lambda's captured variables get populated.
// Every IdentifierExpression inside the body triggers checkLambdaCapture().
// ============================================================================

void SemanticValidator::visit(LambdaExpression& node) {
    // Push a new capture context
    LambdaContext ctx;
    ctx.lambda = &node;

    // Register parameters as local symbols (they don't need capture)
    for (auto& param : node.parameters) {
        if (param && param->resolvedSymbol) {
            ctx.localSymbols.insert(param->resolvedSymbol.get());
        }
    }

    lambdaStack_.push_back(std::move(ctx));

    // Save/restore definite assignment set (lambda is a separate scope)
    auto savedAssigned = definitelyAssigned_;
    // Mark lambda parameters as assigned
    for (auto& param : node.parameters) {
        if (param && param->resolvedSymbol) {
            definitelyAssigned_.insert(
                param->resolvedSymbol->as<VariableSymbol>());
        }
    }

    // Visit body — this triggers checkLambdaCapture for all identifiers
    if (node.body) {
        if (auto* block = node.body->as<BlockStatementNode>()) {
            visitStatements(block->statements);
        } else if (auto* expr = node.body->as<ExpressionBaseNode>()) {
            expr->accept(*this);
        }
    }

    definitelyAssigned_ = savedAssigned;
    lambdaStack_.pop_back();
}

} // namespace mingus
