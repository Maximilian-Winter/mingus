//================================================================================
// MINGUS v1 - Semantic Validator Implementation (Pass 4)
// Performs control flow analysis, RAII tracking, raw block safety,
// pattern exhaustiveness, and lambda capture analysis.
//================================================================================

#include "mingus/sema/SemanticValidator.h"

namespace mingus {
namespace sema {

//================================================================================
// Constructor & Entry Point
//================================================================================
SemanticValidator::SemanticValidator(SymbolTable& table, TypeRegistry& registry,
                                     ErrorReporter& errors)
    : symbolTable_(table)
    , registry_(registry)
    , errors_(errors)
    , currentScope_(nullptr)
    , loopDepth_(0)
    , currentReturnType_(nullptr)
    , rawDepth_(0)
{
}

void SemanticValidator::validate(ProgramNode& program) {
    currentScope_ = symbolTable_.getGlobalScope();
    visit(program);
}

const std::unordered_map<Scope*, ScopeRAIIInfo>& SemanticValidator::getRAIIInfo() const {
    return raiiInfo_;
}

//================================================================================
// Scope Navigation (same pattern as TypeChecker)
//================================================================================
void SemanticValidator::enterNamedScope(Scope* scope) {
    currentScope_ = scope;
    childIndexStack_.push_back(0);
}

void SemanticValidator::leaveNamedScope() {
    if (!childIndexStack_.empty()) {
        childIndexStack_.pop_back();
    }
    if (currentScope_->parent) {
        currentScope_ = currentScope_->parent;
    }
}

void SemanticValidator::enterNextChildScope() {
    if (childIndexStack_.empty()) {
        childIndexStack_.push_back(0);
    }
    size_t& idx = childIndexStack_.back();
    if (idx < currentScope_->children.size()) {
        currentScope_ = currentScope_->children[idx++].get();
        childIndexStack_.push_back(0);
    }
}

void SemanticValidator::leaveChildScope() {
    if (!childIndexStack_.empty()) {
        childIndexStack_.pop_back();
    }
    if (currentScope_->parent) {
        currentScope_ = currentScope_->parent;
    }
}

void SemanticValidator::visitStatements(NodeList<StatementNode>& stmts) {
    for (auto& stmt : stmts) {
        if (stmt) stmt->accept(*this);
    }
}

//================================================================================
// 4a: Reachability Analysis (standalone, uses dynamic_cast)
//================================================================================
bool SemanticValidator::isVoidReturn() const {
    if (!currentReturnType_) return true;
    auto* prim = currentReturnType_->as<PrimitiveType>();
    return prim && prim->kind == PrimitiveType::PrimitiveKind::Void;
}

Reachability SemanticValidator::classifyStatement(StatementNode* stmt) {
    if (!stmt) return Reachability::NeverReturns;

    if (dynamic_cast<ReturnStatement*>(stmt)) {
        return Reachability::AlwaysReturns;
    }

    if (auto* block = dynamic_cast<BlockStatement*>(stmt)) {
        return classifyBlock(block->statements);
    }

    if (auto* ifStmt = dynamic_cast<IfStatement*>(stmt)) {
        // Without else → can't guarantee return
        if (!ifStmt->elseBody) {
            // Check if then or any else-if AlwaysReturns
            auto thenR = classifyStatement(ifStmt->thenBody.get());
            if (thenR == Reachability::AlwaysReturns) {
                return Reachability::SometimesReturns;
            }
            for (auto& elseIf : ifStmt->elseIfClauses) {
                auto r = classifyStatement(elseIf.body.get());
                if (r == Reachability::AlwaysReturns) {
                    return Reachability::SometimesReturns;
                }
            }
            return Reachability::NeverReturns;
        }

        // With else → AlwaysReturns only if ALL branches AlwaysReturn
        auto thenR = classifyStatement(ifStmt->thenBody.get());
        if (thenR != Reachability::AlwaysReturns) {
            return (thenR == Reachability::SometimesReturns)
                ? Reachability::SometimesReturns
                : Reachability::NeverReturns;
        }

        for (auto& elseIf : ifStmt->elseIfClauses) {
            auto r = classifyStatement(elseIf.body.get());
            if (r != Reachability::AlwaysReturns) {
                return Reachability::SometimesReturns;
            }
        }

        auto elseR = classifyStatement(ifStmt->elseBody.get());
        if (elseR != Reachability::AlwaysReturns) {
            return Reachability::SometimesReturns;
        }

        return Reachability::AlwaysReturns;
    }

    if (auto* switchStmt = dynamic_cast<SwitchStatement*>(stmt)) {
        // Must have default case to be exhaustive
        if (!switchStmt->hasDefault()) {
            return Reachability::NeverReturns;
        }
        // All cases + default must AlwaysReturn
        for (auto& switchCase : switchStmt->cases) {
            auto r = classifyBlock(switchCase.body);
            if (r != Reachability::AlwaysReturns) {
                return Reachability::SometimesReturns;
            }
        }
        auto defaultR = classifyBlock(switchStmt->defaultCase);
        if (defaultR != Reachability::AlwaysReturns) {
            return Reachability::SometimesReturns;
        }
        return Reachability::AlwaysReturns;
    }

    if (auto* rawBlock = dynamic_cast<RawBlock*>(stmt)) {
        if (rawBlock->body) {
            return classifyBlock(rawBlock->body->statements);
        }
        return Reachability::NeverReturns;
    }

    // For/While loops might not execute → NeverReturns
    // ExpressionStatement, DeleteStatement, BreakStatement, ContinueStatement → NeverReturns
    return Reachability::NeverReturns;
}

Reachability SemanticValidator::classifyBlock(const NodeList<StatementNode>& stmts) {
    Reachability result = Reachability::NeverReturns;
    bool seenAlwaysReturns = false;

    for (size_t i = 0; i < stmts.size(); ++i) {
        if (!stmts[i]) continue;

        if (seenAlwaysReturns) {
            errors_.warning(stmts[i]->location,
                "unreachable code after return statement");
            break;
        }

        auto r = classifyStatement(stmts[i].get());
        if (r == Reachability::AlwaysReturns) {
            result = Reachability::AlwaysReturns;
            seenAlwaysReturns = true;
        } else if (r == Reachability::SometimesReturns &&
                   result == Reachability::NeverReturns) {
            result = Reachability::SometimesReturns;
        }
    }

    return result;
}

//================================================================================
// 4b: RAII Tracking
//================================================================================
void SemanticValidator::trackRAIIVariable(VariableSymbol* var) {
    if (!var || !var->type) return;

    auto* userType = var->type->as<UserType>();
    if (!userType) return;

    auto* typeSym = static_cast<TypeSymbol*>(userType->symbol);
    if (!typeSym) return;

    auto* classSym = typeSym->as<ClassSymbol>();
    if (!classSym || !classSym->hasRAII()) return;

    raiiInfo_[currentScope_].destructibles.emplace_back(var, classSym->destructor);
}

//================================================================================
// 4d: Pattern Exhaustiveness
//================================================================================
void SemanticValidator::checkExhaustiveness(MatchExpression& node) {
    // Check if any arm has an unguarded wildcard or binding pattern
    for (auto& arm : node.arms) {
        if (!arm.pattern) continue;

        // Unguarded wildcard → exhaustive
        if (dynamic_cast<WildcardPattern*>(arm.pattern.get())) {
            return;
        }

        // Unguarded binding → exhaustive
        if (dynamic_cast<BindingPattern*>(arm.pattern.get())) {
            return;
        }

        // GuardedPattern with wildcard/binding does NOT count as exhaustive
        // (the guard might be false)
    }

    // No wildcard/binding found — check subject type
    auto subjectType = node.subject ? node.subject->resolvedType : nullptr;
    if (!subjectType || subjectType->is<ErrorType>()) return;

    auto* userType = subjectType->as<UserType>();
    if (userType && userType->symbol) {
        auto* typeSym = static_cast<TypeSymbol*>(userType->symbol);
        auto* enumSym = typeSym->as<EnumSymbol>();
        if (enumSym) {
            // Enum exhaustiveness: collect covered members
            std::set<std::string> coveredMembers;

            for (auto& arm : node.arms) {
                if (!arm.pattern) continue;

                auto* litPat = dynamic_cast<LiteralPattern*>(arm.pattern.get());
                if (!litPat || !litPat->value) continue;

                // Check if the literal is a qualified enum member reference
                auto* qualExpr = dynamic_cast<QualifiedNameExpression*>(litPat->value.get());
                if (qualExpr && !qualExpr->qualifiedName.parts.empty()) {
                    coveredMembers.insert(qualExpr->qualifiedName.parts.back());
                }

                // Also check plain identifier (might reference enum member)
                auto* identExpr = dynamic_cast<IdentifierExpression*>(litPat->value.get());
                if (identExpr) {
                    coveredMembers.insert(identExpr->name);
                }
            }

            // Check if all enum members are covered
            std::vector<std::string> missing;
            for (auto& member : enumSym->members) {
                if (coveredMembers.find(member.name) == coveredMembers.end()) {
                    missing.push_back(member.name);
                }
            }

            if (!missing.empty()) {
                std::string memberList;
                for (size_t i = 0; i < missing.size(); ++i) {
                    if (i > 0) memberList += ", ";
                    memberList += missing[i];
                }
                errors_.error(node.location,
                    "non-exhaustive match expression — missing members: " + memberList);
            }
            return;
        }
    }

    // Non-enum type without wildcard → error
    errors_.error(node.location,
        "non-exhaustive match expression — wildcard pattern required");
}

//================================================================================
// 4e: Lambda Capture Analysis
//================================================================================
void SemanticValidator::checkLambdaCapture(IdentifierExpression& node) {
    if (lambdaStack_.empty()) return;
    if (!node.resolvedSymbol) return;

    auto* varSym = node.resolvedSymbol->as<VariableSymbol>();
    if (!varSym) return;
    if (varSym->role != VariableRole::Local && varSym->role != VariableRole::Parameter) return;

    // Check if this symbol is local to the innermost lambda
    auto& ctx = lambdaStack_.back();
    if (ctx.localSymbols.count(node.resolvedSymbol) > 0) return;

    // Not local to the lambda → it's a capture
    // Avoid duplicates
    for (auto* existing : ctx.lambda->capturedVariables) {
        if (existing == node.resolvedSymbol) return;
    }
    ctx.lambda->capturedVariables.push_back(node.resolvedSymbol);
}

//================================================================================
// Program Structure
//================================================================================
void SemanticValidator::visit(ProgramNode& node) {
    for (auto& mod : node.modules) {
        if (mod) mod->accept(*this);
    }
}

void SemanticValidator::visit(ModuleNode& node) {
    auto* sym = currentScope_->lookupLocal(node.name);
    if (!sym || !sym->is<ModuleSymbol>()) return;
    enterNamedScope(sym->as<ModuleSymbol>()->moduleScope);

    for (auto& decl : node.declarations) {
        if (decl) decl->accept(*this);
    }

    leaveNamedScope();
}

void SemanticValidator::visit(ImportNode& /*node*/) {}

//================================================================================
// Declarations — Entering Bodies
//================================================================================
void SemanticValidator::visit(InterfaceDeclaration& node) {
    // Interface methods have no bodies — nothing to validate
    (void)node;
}

void SemanticValidator::visit(StructDeclaration& node) {
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

void SemanticValidator::visit(ClassDeclaration& node) {
    auto* sym = currentScope_->lookupLocal(node.name);
    if (!sym || !sym->is<ClassSymbol>()) return;
    auto* classSym = sym->as<ClassSymbol>();

    // Check that concrete classes override all abstract methods from base
    if (!classSym->isAbstract && classSym->baseClass) {
        for (auto* vtableEntry : classSym->vtable) {
            if (vtableEntry && vtableEntry->isAbstract) {
                errors_.error(node.location,
                    "concrete class '" + node.name +
                    "' does not override abstract method '" +
                    vtableEntry->name + "' from base class '" +
                    classSym->baseClass->name + "'");
            }
        }
    }

    // Check that concrete classes implement all interface methods
    if (!classSym->isAbstract) {
        for (auto* ifaceSym : classSym->implementedInterfaces) {
            for (auto* ifaceMethod : ifaceSym->methods) {
                auto* impl = classSym->findMethod(ifaceMethod->name);
                if (!impl || impl->isAbstract) {
                    errors_.error(node.location,
                        "class '" + node.name +
                        "' does not implement interface method '" +
                        ifaceMethod->name + "' from '" + ifaceSym->name + "'");
                }
            }
        }
    }

    enterNamedScope(classSym->memberScope);

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

void SemanticValidator::visit(FunctionDeclaration& node) {
    auto* sym = currentScope_->lookupLocal(node.name);
    if (!sym || !sym->is<FunctionSymbol>()) return;
    auto* fnSym = sym->as<FunctionSymbol>();

    if (!node.body) return;

    auto prevRetType = currentReturnType_;
    currentReturnType_ = fnSym->returnType;

    enterNamedScope(fnSym->bodyScope);
    visitStatements(node.body->statements);
    leaveNamedScope();

    // 4a: Return completeness check
    if (!isVoidReturn()) {
        auto reachability = classifyBlock(node.body->statements);
        if (reachability != Reachability::AlwaysReturns) {
            errors_.error(node.location,
                "not all code paths return a value in function '" + node.name + "'");
        }
    }

    currentReturnType_ = prevRetType;
}

void SemanticValidator::visit(ConstructorDeclaration& node) {
    auto* sym = currentScope_->lookupLocal("constructor");
    if (!sym || !sym->is<ConstructorSymbol>()) return;
    auto* ctorSym = sym->as<ConstructorSymbol>();

    if (!node.body) return;

    auto prevRetType = currentReturnType_;
    currentReturnType_ = registry_.getVoid();

    enterNamedScope(ctorSym->bodyScope);
    visitStatements(node.body->statements);
    leaveNamedScope();

    currentReturnType_ = prevRetType;
}

void SemanticValidator::visit(DestructorDeclaration& node) {
    auto* sym = currentScope_->lookupLocal("destructor");
    if (!sym || !sym->is<DestructorSymbol>()) return;
    auto* dtorSym = sym->as<DestructorSymbol>();

    if (!node.body) return;

    auto prevRetType = currentReturnType_;
    currentReturnType_ = registry_.getVoid();

    enterNamedScope(dtorSym->bodyScope);
    visitStatements(node.body->statements);
    leaveNamedScope();

    currentReturnType_ = prevRetType;
}

void SemanticValidator::visit(OperatorDeclaration& node) {
    auto overloadOp = operatorKindToOverloadableOp(node.op);
    auto* opSym = currentScope_->lookupOperator(overloadOp);
    if (!opSym || !node.body) return;

    auto prevRetType = currentReturnType_;
    currentReturnType_ = opSym->returnType;

    enterNamedScope(opSym->bodyScope);
    visitStatements(node.body->statements);
    leaveNamedScope();

    // 4a: Return completeness check for non-void operators
    if (!isVoidReturn()) {
        auto reachability = classifyBlock(node.body->statements);
        if (reachability != Reachability::AlwaysReturns) {
            errors_.error(node.location,
                "not all code paths return a value in operator");
        }
    }

    currentReturnType_ = prevRetType;
}

void SemanticValidator::visit(VariableDeclaration& node) {
    // 4b: Track RAII variables
    auto* sym = currentScope_->lookupLocal(node.name);
    if (sym && sym->is<VariableSymbol>()) {
        auto* varSym = sym->as<VariableSymbol>();
        if (varSym->role == VariableRole::Local) {
            trackRAIIVariable(varSym);
        }

        // Track for lambda capture analysis
        if (!lambdaStack_.empty()) {
            lambdaStack_.back().localSymbols.insert(sym);
        }
    }

    // Walk initializer for expression analysis (4c, 4e)
    if (node.initializer) {
        node.initializer->accept(*this);
    }
}

void SemanticValidator::visit(TupleDestructuringDeclaration& node) {
    // Track destructured variables for lambda capture
    if (!lambdaStack_.empty()) {
        for (auto& elem : node.elements) {
            auto* sym = currentScope_->lookupLocal(elem.name);
            if (sym) {
                lambdaStack_.back().localSymbols.insert(sym);
            }
        }
    }

    if (node.initializer) {
        node.initializer->accept(*this);
    }
}

void SemanticValidator::visit(ExternFunctionDeclaration& /*node*/) {}
void SemanticValidator::visit(EnumDeclaration& /*node*/) {}
void SemanticValidator::visit(EnumMemberNode& /*node*/) {}
void SemanticValidator::visit(ParameterNode& /*node*/) {}

//================================================================================
// Statements
//================================================================================
void SemanticValidator::visit(BlockStatement& node) {
    enterNextChildScope();
    visitStatements(node.statements);
    leaveChildScope();
}

void SemanticValidator::visit(ExpressionStatement& node) {
    if (node.expression) node.expression->accept(*this);
}

void SemanticValidator::visit(ReturnStatement& node) {
    if (node.value) node.value->accept(*this);
}

void SemanticValidator::visit(IfStatement& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.thenBody) node.thenBody->accept(*this);

    for (auto& elseIf : node.elseIfClauses) {
        if (elseIf.condition) elseIf.condition->accept(*this);
        if (elseIf.body) elseIf.body->accept(*this);
    }

    if (node.elseBody) node.elseBody->accept(*this);
}

void SemanticValidator::visit(SwitchStatement& node) {
    if (node.subject) node.subject->accept(*this);

    for (auto& switchCase : node.cases) {
        if (switchCase.value) switchCase.value->accept(*this);
        visitStatements(switchCase.body);
    }
    visitStatements(node.defaultCase);
}

void SemanticValidator::visit(ForStatement& node) {
    enterNextChildScope();
    ++loopDepth_;

    if (node.initDeclaration) node.initDeclaration->accept(*this);
    for (auto& expr : node.initExpressions) {
        if (expr) expr->accept(*this);
    }
    if (node.condition) node.condition->accept(*this);
    for (auto& iter : node.iterators) {
        if (iter) iter->accept(*this);
    }
    if (node.body) node.body->accept(*this);

    --loopDepth_;
    leaveChildScope();
}

void SemanticValidator::visit(WhileStatement& node) {
    ++loopDepth_;

    if (node.condition) node.condition->accept(*this);
    if (node.body) node.body->accept(*this);

    --loopDepth_;
}

void SemanticValidator::visit(BreakStatement& node) {
    // 4a: break must be inside a loop
    if (loopDepth_ == 0) {
        errors_.error(node.location, "'break' outside of loop");
    }
}

void SemanticValidator::visit(ContinueStatement& node) {
    // 4a: continue must be inside a loop
    if (loopDepth_ == 0) {
        errors_.error(node.location, "'continue' outside of loop");
    }
}

void SemanticValidator::visit(DeleteStatement& node) {
    if (node.target) node.target->accept(*this);
}

void SemanticValidator::visit(RawBlock& node) {
    // 4c: Entering raw block — permit unsafe pointer operations
    ++rawDepth_;
    enterNextChildScope();
    if (node.body) {
        visitStatements(node.body->statements);
    }
    leaveChildScope();
    --rawDepth_;
}

//================================================================================
// Expression Visitors
//================================================================================

// Literals — no children to walk
void SemanticValidator::visit(IntegerLiteral& /*node*/) {}
void SemanticValidator::visit(FloatLiteral& /*node*/) {}
void SemanticValidator::visit(BoolLiteral& /*node*/) {}
void SemanticValidator::visit(CharLiteral& /*node*/) {}
void SemanticValidator::visit(StringLiteral& /*node*/) {}
void SemanticValidator::visit(NullLiteral& /*node*/) {}

void SemanticValidator::visit(InterpolatedString& node) {
    for (auto& part : node.parts) {
        if (part.kind == InterpolatedPartKind::Expression && part.expression) {
            part.expression->accept(*this);
        }
    }
}

void SemanticValidator::visit(IdentifierExpression& node) {
    // 4e: Lambda capture analysis
    checkLambdaCapture(node);
}

void SemanticValidator::visit(QualifiedNameExpression& /*node*/) {}
void SemanticValidator::visit(ThisExpression& /*node*/) {}

void SemanticValidator::visit(BinaryExpression& node) {
    // 4c: Raw block safety — pointer arithmetic
    if (rawDepth_ == 0 &&
        (node.op == BinaryOp::Add || node.op == BinaryOp::Sub)) {
        auto* leftType = node.left ? node.left->resolvedType.get() : nullptr;
        auto* rightType = node.right ? node.right->resolvedType.get() : nullptr;
        if ((leftType && leftType->is<PointerType>()) ||
            (rightType && rightType->is<PointerType>())) {
            errors_.error(node.location, "pointer arithmetic requires 'raw' block");
        }
    }

    // Walk children
    if (node.left) node.left->accept(*this);
    if (node.right) node.right->accept(*this);
}

void SemanticValidator::visit(UnaryExpression& node) {
    if (node.operand) node.operand->accept(*this);
}

void SemanticValidator::visit(AssignmentExpression& node) {
    if (node.target) node.target->accept(*this);
    if (node.value) node.value->accept(*this);
}

void SemanticValidator::visit(TernaryExpression& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.thenExpr) node.thenExpr->accept(*this);
    if (node.elseExpr) node.elseExpr->accept(*this);
}

void SemanticValidator::visit(CallExpression& node) {
    if (node.callee) node.callee->accept(*this);
    for (auto& arg : node.arguments) {
        if (arg) arg->accept(*this);
    }
}

void SemanticValidator::visit(NewExpression& node) {
    for (auto& arg : node.arguments) {
        if (arg) arg->accept(*this);
    }
    if (node.arraySize) node.arraySize->accept(*this);
}

void SemanticValidator::visit(IndexExpression& node) {
    if (node.object) node.object->accept(*this);
    if (node.index) node.index->accept(*this);
}

void SemanticValidator::visit(CastExpression& node) {
    // 4c: Raw block safety — pointer casts
    if (rawDepth_ == 0 && node.operand && node.operand->resolvedType &&
        node.targetType && node.targetType->resolvedType) {
        auto* fromType = node.operand->resolvedType.get();
        auto* toType = node.targetType->resolvedType.get();

        bool fromIsPointer = fromType->is<PointerType>();
        bool toIsPointer = toType->is<PointerType>();
        bool fromIsInteger = registry_.isIntegerType(fromType);
        bool toIsInteger = registry_.isIntegerType(toType);

        // Pointer-to-pointer cast (different pointer types)
        if (fromIsPointer && toIsPointer && fromType != toType) {
            errors_.error(node.location, "pointer cast requires 'raw' block");
        }
        // Pointer-to-integer or integer-to-pointer cast
        else if ((fromIsPointer && toIsInteger) || (fromIsInteger && toIsPointer)) {
            errors_.error(node.location, "pointer cast requires 'raw' block");
        }
    }

    if (node.operand) node.operand->accept(*this);
}

void SemanticValidator::visit(MemberAccessExpression& node) {
    if (node.object) node.object->accept(*this);
}

void SemanticValidator::visit(SizeOfExpression& /*node*/) {}
void SemanticValidator::visit(AlignOfExpression& /*node*/) {}

void SemanticValidator::visit(PipeExpression& node) {
    if (node.input) node.input->accept(*this);
    for (auto& stage : node.stages) {
        if (stage.function) stage.function->accept(*this);
        for (auto& arg : stage.extraArguments) {
            if (arg) arg->accept(*this);
        }
    }
}

void SemanticValidator::visit(MatchExpression& node) {
    // 4d: Pattern exhaustiveness
    checkExhaustiveness(node);

    // Walk subject and arms (each arm has its own scope)
    if (node.subject) node.subject->accept(*this);
    for (auto& arm : node.arms) {
        enterNextChildScope();
        if (arm.pattern) arm.pattern->accept(*this);
        if (arm.body) arm.body->accept(*this);
        leaveChildScope();
    }
}

void SemanticValidator::visit(TupleExpression& node) {
    for (auto& elem : node.elements) {
        if (elem) elem->accept(*this);
    }
}

void SemanticValidator::visit(LambdaExpression& node) {
    // 4e: Push lambda context
    LambdaContext ctx;
    ctx.lambda = &node;

    // Add lambda parameters to local symbols
    // Look up parameter symbols in the lambda's scope (which is the next child scope)
    // We need to find the lambda's body scope to get its parameters
    // For now, we mark the parameters by name in the current scope
    enterNextChildScope();

    // Look up parameter symbols in the lambda scope
    for (auto& param : node.parameters) {
        if (param) {
            auto* sym = currentScope_->lookupLocal(param->name);
            if (sym) {
                ctx.localSymbols.insert(sym);
            }
        }
    }

    lambdaStack_.push_back(std::move(ctx));

    // Walk body
    if (node.body) {
        node.body->accept(*this);
    }

    // Pop context — captured variables are already stored on the lambda node
    lambdaStack_.pop_back();
    leaveChildScope();
}

//================================================================================
// Pattern Visitors — walk children for expression analysis
//================================================================================
void SemanticValidator::visit(LiteralPattern& node) {
    if (node.value) node.value->accept(*this);
}

void SemanticValidator::visit(RangePattern& /*node*/) {}
void SemanticValidator::visit(WildcardPattern& /*node*/) {}
void SemanticValidator::visit(BindingPattern& /*node*/) {}

void SemanticValidator::visit(TuplePattern& node) {
    for (auto& elem : node.elements) {
        if (elem) elem->accept(*this);
    }
}

void SemanticValidator::visit(GuardedPattern& node) {
    if (node.innerPattern) node.innerPattern->accept(*this);
    if (node.guard) node.guard->accept(*this);
}

//================================================================================
// Type Node Visitors (no-op — types are already resolved)
//================================================================================
void SemanticValidator::visit(TypeNode& /*node*/) {}
void SemanticValidator::visit(PrimitiveTypeNode& /*node*/) {}
void SemanticValidator::visit(NamedTypeNode& /*node*/) {}
void SemanticValidator::visit(PointerTypeNode& /*node*/) {}
void SemanticValidator::visit(ArrayTypeNode& /*node*/) {}
void SemanticValidator::visit(TupleTypeNode& /*node*/) {}
void SemanticValidator::visit(FunctionTypeNode& /*node*/) {}

} // namespace sema
} // namespace mingus
