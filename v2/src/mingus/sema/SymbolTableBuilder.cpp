// ============================================================================
// SymbolTableBuilder.cpp — Pass 1: Build the scope tree and all symbols
// ============================================================================

#include "mingus/sema/SymbolTableBuilder.h"

namespace mingus {

SymbolTableBuilder::SymbolTableBuilder(SymbolTable& symbolTable, ErrorReporter& errors)
    : symbolTable_(symbolTable)
    , errors_(errors) {}

// ============================================================================
// Entry point
// ============================================================================

void SymbolTableBuilder::build(ProgramNode& program) {
    // Phase 1a: Build scope tree and symbols
    visit(program);

    // Phase 1b: Resolve imports (all module scopes now exist)
    resolveAllImports(program);
}

// ============================================================================
// Scope helpers
// ============================================================================

void SymbolTableBuilder::setScope(AstBaseNode& node) {
    node.astScopeNode = symbolTable_.getCurrentScope();
}

void SymbolTableBuilder::pushBlockScope(AstBaseNode& node) {
    auto blockScope = std::make_shared<BlockScope>(symbolTable_.getCurrentScope());
    symbolTable_.getCurrentScope()->nest(blockScope);
    symbolTable_.pushScope(blockScope);
    node.astScopeNode = blockScope;
}

void SymbolTableBuilder::visitStatements(
    std::vector<std::shared_ptr<StatementBaseNode>>& stmts)
{
    for (auto& stmt : stmts) {
        if (stmt) stmt->accept(*this);
    }
}

void SymbolTableBuilder::createParameterSymbols(
    std::vector<std::shared_ptr<ParameterNode>>& params,
    const std::shared_ptr<FunctionSymbol>& funcSym)
{
    for (auto& param : params) {
        auto paramSym = std::make_shared<VariableSymbol>(param->name, nullptr);
        paramSym->role = VariableRole::Parameter;
        paramSym->isReference = param->isReference;
        symbolTable_.defineSymbol(paramSym);
        funcSym->parameters.push_back(paramSym);
        param->resolvedSymbol = paramSym;  // V2: direct link, no scanForParamSymbols
        setScope(*param);
    }
}

// ============================================================================
// Program & Module
// ============================================================================

void SymbolTableBuilder::visit(ProgramNode& node) {
    setScope(node);
    for (auto& module : node.modules) {
        if (module) module->accept(*this);
    }
}

void SymbolTableBuilder::visit(ModuleNode& node) {
    auto moduleSym = std::make_shared<ModuleSymbol>(node.name);
    symbolTable_.defineSymbol(moduleSym);
    symbolTable_.pushScope(moduleSym);
    node.astScopeNode = moduleSym;
    node.resolvedModule = moduleSym;

    for (auto& decl : node.declarations) {
        if (decl) decl->accept(*this);
    }

    symbolTable_.popScope();
}

// ============================================================================
// Block Statement
// ============================================================================

void SymbolTableBuilder::visit(BlockStatementNode& node) {
    pushBlockScope(node);
    visitStatements(node.statements);
    symbolTable_.popScope();
}

// ============================================================================
// Variable Declarations
// ============================================================================

void SymbolTableBuilder::visit(VariableDeclaration& node) {
    setScope(node);

    auto varSym = std::make_shared<VariableSymbol>(node.name, nullptr);
    varSym->role = inTypeScope_ ? VariableRole::Field : VariableRole::Local;
    varSym->isInferred = node.isInferred;
    varSym->isMutable = true;
    varSym->accessLevel = node.accessModifier;
    symbolTable_.defineSymbol(varSym);
    node.resolvedVariable = varSym;

    // Walk initializer for nested lambdas/declarations
    if (node.initializer) {
        node.initializer->accept(*this);
    }
}

void SymbolTableBuilder::visit(VariableDeclarationExpression& node) {
    setScope(node);

    auto varSym = std::make_shared<VariableSymbol>(node.name, nullptr);
    varSym->role = inTypeScope_ ? VariableRole::Field : VariableRole::Local;
    varSym->isInferred = node.isInferred;
    varSym->isMutable = true;
    varSym->accessLevel = node.accessModifier;
    symbolTable_.defineSymbol(varSym);
    node.resolvedVariable = varSym;

    if (node.initializer) {
        node.initializer->accept(*this);
    }
}

void SymbolTableBuilder::visit(TupleDestructuringDeclaration& node) {
    setScope(node);

    for (auto& elem : node.elements) {
        auto varSym = std::make_shared<VariableSymbol>(elem.name, nullptr);
        varSym->role = VariableRole::Local;
        varSym->isInferred = elem.isInferred;
        symbolTable_.defineSymbol(varSym);
        node.resolvedVariables.push_back(varSym);
    }

    if (node.initializer) {
        node.initializer->accept(*this);
    }
}

// ============================================================================
// Function Declarations
// ============================================================================

void SymbolTableBuilder::visit(FunctionDeclaration& node) {
    setScope(node);

    std::shared_ptr<FunctionSymbol> funcSym;
    if (inTypeScope_) {
        auto methodSym = std::make_shared<MethodSymbol>(node.name);
        methodSym->classOfThisMethod = currentClass_;
        methodSym->isVirtual = node.isVirtual || node.isOverride;
        funcSym = methodSym;
    } else {
        funcSym = std::make_shared<FunctionSymbol>(node.name);
    }

    funcSym->isStatic = node.isStatic;
    funcSym->isAbstract = node.isAbstract;
    funcSym->isMethod = inTypeScope_;
    funcSym->hasThisParam = inTypeScope_ && !node.isStatic;
    funcSym->accessLevel = node.accessModifier;
    symbolTable_.defineSymbol(funcSym);
    node.resolvedFunction = funcSym;

    if (node.body) {
        // FunctionSymbol IS the scope — push it
        symbolTable_.pushScope(funcSym);
        node.body->astScopeNode = funcSym;  // body shares function scope

        createParameterSymbols(node.parameters, funcSym);
        visitStatements(node.body->statements);

        symbolTable_.popScope();
    } else {
        // Abstract/interface method — create params but no body scope
        // Still need to create param symbols for type resolution
        auto tempScope = std::make_shared<BlockScope>(symbolTable_.getCurrentScope());
        symbolTable_.pushScope(tempScope);
        createParameterSymbols(node.parameters, funcSym);
        symbolTable_.popScope();
    }
}

void SymbolTableBuilder::visit(ConstructorDeclaration& node) {
    setScope(node);

    auto ctorSym = std::make_shared<ConstructorSymbol>(
        currentClass_ ? currentClass_->getName() : "");
    ctorSym->accessLevel = node.accessModifier;
    symbolTable_.defineSymbol(ctorSym);
    node.resolvedConstructor = ctorSym;

    if (currentClass_) {
        currentClass_->constructor = ctorSym;
    }

    if (node.body) {
        symbolTable_.pushScope(ctorSym);
        node.body->astScopeNode = ctorSym;
        createParameterSymbols(node.parameters, ctorSym);
        visitStatements(node.body->statements);
        symbolTable_.popScope();
    }
}

void SymbolTableBuilder::visit(DestructorDeclaration& node) {
    setScope(node);

    auto dtorSym = std::make_shared<DestructorSymbol>(
        currentClass_ ? currentClass_->getName() : "");
    symbolTable_.defineSymbol(dtorSym);
    node.resolvedDestructor = dtorSym;

    if (currentClass_) {
        currentClass_->destructor = dtorSym;
    }

    if (node.body) {
        symbolTable_.pushScope(dtorSym);
        node.body->astScopeNode = dtorSym;
        // Destructors have no parameters (only implicit this)
        visitStatements(node.body->statements);
        symbolTable_.popScope();
    }
}

void SymbolTableBuilder::visit(ExternFunctionDeclaration& node) {
    setScope(node);

    auto funcSym = std::make_shared<FunctionSymbol>(node.name);
    funcSym->isExtern = true;
    symbolTable_.defineSymbol(funcSym);
    node.resolvedFunction = funcSym;

    // Create param symbols in a temporary scope (no body)
    auto tempScope = std::make_shared<BlockScope>(symbolTable_.getCurrentScope());
    symbolTable_.pushScope(tempScope);
    createParameterSymbols(node.parameters, funcSym);
    symbolTable_.popScope();
}

void SymbolTableBuilder::visit(OperatorDeclaration& node) {
    setScope(node);

    auto opSym = std::make_shared<OperatorSymbol>("operator", node.op);
    opSym->isMethod = inTypeScope_;
    opSym->hasThisParam = inTypeScope_;
    if (currentClass_) {
        opSym->ownerType = currentClass_;
    }
    symbolTable_.defineSymbol(opSym);
    symbolTable_.getCurrentScope()->defineOperator(opSym);
    node.resolvedOperator = opSym;

    if (node.body) {
        symbolTable_.pushScope(opSym);
        node.body->astScopeNode = opSym;
        createParameterSymbols(node.parameters, opSym);
        visitStatements(node.body->statements);
        symbolTable_.popScope();
    }
}

// ============================================================================
// Type Declarations
// ============================================================================

void SymbolTableBuilder::visit(EnumDeclaration& node) {
    setScope(node);

    auto enumSym = std::make_shared<EnumSymbol>(node.name);
    enumSym->accessLevel = node.accessModifier;
    symbolTable_.defineSymbol(enumSym);
    symbolTable_.registerType(node.name, enumSym);
    node.resolvedEnum = enumSym;

    // Populate members: explicit integer values or auto-increment
    int64_t nextValue = 0;
    for (auto& member : node.members) {
        if (member) {
            setScope(*member);
            EnumSymbol::MemberInfo info;
            info.name = member->name;
            info.hasExplicitValue = (member->value != nullptr);

            if (info.hasExplicitValue) {
                // Evaluate constant integer literal
                if (auto* intLit = member->value->as<IntegerLiteral>()) {
                    info.intValue = intLit->value;
                    nextValue = info.intValue + 1;
                } else {
                    info.intValue = nextValue++;
                }
            } else {
                info.intValue = nextValue++;
            }

            enumSym->members.push_back(info);
        }
    }
}

void SymbolTableBuilder::visit(StructDeclaration& node) {
    setScope(node);

    auto structSym = std::make_shared<StructSymbol>(node.name);
    structSym->accessLevel = node.accessModifier;
    symbolTable_.defineSymbol(structSym);
    symbolTable_.registerType(node.name, structSym);
    node.resolvedStruct = structSym;

    // StructSymbol IS the member scope — push it
    symbolTable_.pushScope(structSym);
    auto savedInType = inTypeScope_;
    auto savedClass = currentClass_;
    inTypeScope_ = true;
    currentClass_ = nullptr;

    // Fields
    int fieldIndex = 0;
    for (auto& field : node.fields) {
        if (field) {
            field->accept(*this);
            if (field->resolvedVariable) {
                field->resolvedVariable->fieldIndex = fieldIndex++;
                structSym->fields.push_back(field->resolvedVariable);
            }
        }
    }

    // Methods
    for (auto& method : node.methods) {
        if (method) method->accept(*this);
    }

    // Operators
    for (auto& op : node.operators) {
        if (op) op->accept(*this);
    }

    inTypeScope_ = savedInType;
    currentClass_ = savedClass;
    symbolTable_.popScope();
}

void SymbolTableBuilder::visit(ClassDeclaration& node) {
    setScope(node);

    auto classSym = std::make_shared<ClassSymbol>(
        node.name, symbolTable_.getCurrentScope());
    classSym->accessLevel = node.accessModifier;
    classSym->isAbstract = node.isAbstract;
    classSym->baseClassNames = node.baseClasses;
    symbolTable_.defineSymbol(classSym);
    symbolTable_.registerType(node.name, classSym);
    node.resolvedClass = classSym;

    // Resolve base class (single inheritance, first name)
    if (!node.baseClasses.empty()) {
        auto baseSym = symbolTable_.getCurrentScope()->resolve(node.baseClasses[0]);
        if (baseSym && baseSym->is<ClassSymbol>()) {
            classSym->resolvedBaseClass = baseSym->as<ClassSymbol>();
            // Copy inherited fields into allFields
            if (classSym->resolvedBaseClass) {
                classSym->allFields = classSym->resolvedBaseClass->allFields;
            }
        } else if (baseSym && baseSym->is<InterfaceSymbol>()) {
            classSym->implementedInterfaces.push_back(
                std::dynamic_pointer_cast<InterfaceSymbol>(baseSym));
        } else if (baseSym == nullptr) {
            errors_.error("base class '" + node.baseClasses[0] + "' not found",
                          node.debugInfo);
        }

        // Remaining names are interfaces
        for (size_t i = 1; i < node.baseClasses.size(); i++) {
            auto ifaceSym = symbolTable_.getCurrentScope()->resolve(node.baseClasses[i]);
            if (ifaceSym && ifaceSym->is<InterfaceSymbol>()) {
                classSym->implementedInterfaces.push_back(
                    std::dynamic_pointer_cast<InterfaceSymbol>(ifaceSym));
            } else if (ifaceSym && ifaceSym->is<ClassSymbol>()) {
                errors_.error("multiple inheritance not supported, '"
                    + node.baseClasses[i] + "' is a class", node.debugInfo);
            }
        }
    }

    // ClassSymbol IS the member scope — push it
    symbolTable_.pushScope(classSym);
    auto savedInType = inTypeScope_;
    auto savedClass = currentClass_;
    inTypeScope_ = true;
    currentClass_ = classSym;

    // Fields
    int fieldIndex = 0;
    for (auto& field : node.fields) {
        if (field) {
            field->accept(*this);
            if (field->resolvedVariable) {
                field->resolvedVariable->fieldIndex = fieldIndex++;
                classSym->fields.push_back(field->resolvedVariable);
                classSym->allFields.push_back(field->resolvedVariable);
            }
        }
    }

    // Constructor (explicit or auto-generated)
    if (node.constructor) {
        node.constructor->accept(*this);
    } else {
        autoGenerateConstructor(node, classSym);
    }

    // Destructor (explicit or auto-generated)
    if (node.destructor) {
        node.destructor->accept(*this);
    } else {
        autoGenerateDestructor(node, classSym);
    }

    // Methods
    for (auto& method : node.methods) {
        if (method) method->accept(*this);
    }

    // Operators
    for (auto& op : node.operators) {
        if (op) op->accept(*this);
    }

    // Build vtable after all members are registered
    buildVtable(classSym.get());

    inTypeScope_ = savedInType;
    currentClass_ = savedClass;
    symbolTable_.popScope();
}

void SymbolTableBuilder::visit(InterfaceDeclaration& node) {
    setScope(node);

    auto ifaceSym = std::make_shared<InterfaceSymbol>(node.name);
    ifaceSym->accessLevel = node.accessModifier;
    symbolTable_.defineSymbol(ifaceSym);
    symbolTable_.registerType(node.name, ifaceSym);
    node.resolvedInterface = ifaceSym;

    // InterfaceSymbol IS the scope for its method signatures
    symbolTable_.pushScope(ifaceSym);
    auto savedInType = inTypeScope_;
    inTypeScope_ = true;

    for (auto& method : node.methods) {
        if (method) {
            method->isAbstract = true;
            method->accept(*this);
            if (method->resolvedFunction) {
                ifaceSym->methods.push_back(method->resolvedFunction);
            }
        }
    }

    inTypeScope_ = savedInType;
    symbolTable_.popScope();
}

// ============================================================================
// Auto-generate constructor/destructor
// ============================================================================

void SymbolTableBuilder::autoGenerateConstructor(
    ClassDeclaration& node,
    const std::shared_ptr<ClassSymbol>& classSym)
{
    auto ctorSym = std::make_shared<ConstructorSymbol>(classSym->getName());
    symbolTable_.defineSymbol(ctorSym);
    classSym->constructor = ctorSym;

    // Create empty body node for AST consistency
    auto body = std::make_shared<BlockStatementNode>();
    body->astScopeNode = ctorSym;
    node.constructor = std::make_shared<ConstructorDeclaration>();
    node.constructor->body = body;
    node.constructor->resolvedConstructor = ctorSym;
    setScope(*node.constructor);
}

void SymbolTableBuilder::autoGenerateDestructor(
    ClassDeclaration& node,
    const std::shared_ptr<ClassSymbol>& classSym)
{
    auto dtorSym = std::make_shared<DestructorSymbol>(classSym->getName());
    symbolTable_.defineSymbol(dtorSym);
    classSym->destructor = dtorSym;

    auto body = std::make_shared<BlockStatementNode>();
    body->astScopeNode = dtorSym;
    node.destructor = std::make_shared<DestructorDeclaration>();
    node.destructor->body = body;
    node.destructor->resolvedDestructor = dtorSym;
    setScope(*node.destructor);
}

// ============================================================================
// Vtable building
// ============================================================================

void SymbolTableBuilder::buildVtable(ClassSymbol* classSym) {
    if (!classSym) return;

    // Start with base class vtable (if any)
    if (classSym->resolvedBaseClass) {
        classSym->vtable = classSym->resolvedBaseClass->vtable;
    }

    // Slot 0: destructor (always)
    if (classSym->vtable.empty()) {
        // Root class: add destructor at slot 0
        classSym->vtable.push_back(
            std::dynamic_pointer_cast<FunctionSymbol>(classSym->destructor));
    } else {
        // Derived class: override slot 0 with our destructor
        classSym->vtable[0] =
            std::dynamic_pointer_cast<FunctionSymbol>(classSym->destructor);
    }
    if (classSym->destructor) {
        classSym->destructor->vtableIndex = 0;
    }

    // Add/override instance methods (non-static, non-ctor/dtor)
    auto allSymbols = classSym->getAllSymbols();
    for (auto& sym : allSymbols) {
        auto* funcSym = sym->as<FunctionSymbol>();
        if (!funcSym) continue;
        if (funcSym->is<ConstructorSymbol>() || funcSym->is<DestructorSymbol>()) continue;
        if (funcSym->isStatic) continue;
        if (!funcSym->isMethod) continue;

        // Check if this overrides a base class method
        bool overridden = false;
        for (size_t i = 1; i < classSym->vtable.size(); i++) {
            if (classSym->vtable[i] &&
                classSym->vtable[i]->getName() == funcSym->getName()) {
                classSym->vtable[i] = std::dynamic_pointer_cast<FunctionSymbol>(sym);
                funcSym->vtableIndex = static_cast<int>(i);
                funcSym->isVirtual = true;
                overridden = true;
                break;
            }
        }

        // New virtual method: append to vtable
        if (!overridden && (funcSym->isVirtual || classSym->resolvedBaseClass)) {
            funcSym->vtableIndex = static_cast<int>(classSym->vtable.size());
            funcSym->isVirtual = true;
            classSym->vtable.push_back(std::dynamic_pointer_cast<FunctionSymbol>(sym));
        }
    }

    classSym->vtableSize = static_cast<int>(classSym->vtable.size());
}

// ============================================================================
// Import resolution (Phase 1b)
// ============================================================================

void SymbolTableBuilder::resolveAllImports(ProgramNode& program) {
    for (auto& module : program.modules) {
        if (!module || !module->resolvedModule) continue;

        for (auto& decl : module->declarations) {
            auto* importDecl = decl->as<ImportDeclaration>();
            if (!importDecl) continue;

            // Find source module
            if (importDecl->sourcePath.empty()) continue;
            auto sourceSym = symbolTable_.getRootScope()->resolve(
                importDecl->sourcePath[0]);
            auto* sourceModule = sourceSym ? sourceSym->as<ModuleSymbol>() : nullptr;

            if (!sourceModule) {
                errors_.error("module '" + importDecl->sourcePath[0] + "' not found",
                              importDecl->debugInfo);
                continue;
            }

            if (importDecl->isWholeModule) {
                // import Module; — import all public symbols
                auto symbols = sourceModule->getAllSymbols();
                for (auto& sym : symbols) {
                    if (sym->accessLevel == AccessModifier::Public) {
                        module->resolvedModule->define(sym);
                    }
                }
            } else {
                // import x, y from Module; — selective import
                for (auto& target : importDecl->targets) {
                    auto sym = sourceModule->resolve(target.name);
                    if (sym) {
                        if (target.alias.has_value()) {
                            // Aliased import: define under alias name
                            // We create a wrapper or just define with the alias
                            // For now, define the same symbol under the alias name
                            module->resolvedModule->define(sym);
                        } else {
                            module->resolvedModule->define(sym);
                        }
                    } else {
                        errors_.error("'" + target.name + "' not found in module '"
                            + importDecl->sourcePath[0] + "'", importDecl->debugInfo);
                    }
                }
            }
        }
    }
}

// ============================================================================
// Statements — set scope and recurse
// ============================================================================

void SymbolTableBuilder::visit(ExpressionStatement& node) {
    setScope(node);
    if (node.expression) node.expression->accept(*this);
}

void SymbolTableBuilder::visit(ReturnStatement& node) {
    setScope(node);
    if (node.value) node.value->accept(*this);
}

void SymbolTableBuilder::visit(IfStatement& node) {
    setScope(node);
    if (node.condition) node.condition->accept(*this);
    if (node.thenBody) node.thenBody->accept(*this);
    for (auto& elseIf : node.elseIfClauses) {
        if (elseIf.condition) elseIf.condition->accept(*this);
        if (elseIf.body) elseIf.body->accept(*this);
    }
    if (node.elseBody) node.elseBody->accept(*this);
}

void SymbolTableBuilder::visit(ForStatement& node) {
    // For loop gets its own block scope (init var visible in body)
    pushBlockScope(node);

    if (node.initDeclaration) node.initDeclaration->accept(*this);
    for (auto& initExpr : node.initExpressions) {
        if (initExpr) initExpr->accept(*this);
    }
    if (node.condition) node.condition->accept(*this);
    for (auto& iter : node.iterators) {
        if (iter) iter->accept(*this);
    }
    if (node.body) node.body->accept(*this);

    symbolTable_.popScope();
}

void SymbolTableBuilder::visit(WhileStatement& node) {
    setScope(node);
    if (node.condition) node.condition->accept(*this);
    if (node.body) node.body->accept(*this);
}

void SymbolTableBuilder::visit(BreakStatement& node) { setScope(node); }
void SymbolTableBuilder::visit(ContinueStatement& node) { setScope(node); }

void SymbolTableBuilder::visit(DeleteStatement& node) {
    setScope(node);
    if (node.target) node.target->accept(*this);
}

void SymbolTableBuilder::visit(SwitchStatement& node) {
    setScope(node);
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
// Expressions — set scope and recurse for nested structures
// ============================================================================

void SymbolTableBuilder::visit(IntegerLiteral& node) { setScope(node); }
void SymbolTableBuilder::visit(FloatLiteral& node) { setScope(node); }
void SymbolTableBuilder::visit(BoolLiteral& node) { setScope(node); }
void SymbolTableBuilder::visit(CharLiteral& node) { setScope(node); }
void SymbolTableBuilder::visit(StringLiteral& node) { setScope(node); }
void SymbolTableBuilder::visit(NullLiteral& node) { setScope(node); }
void SymbolTableBuilder::visit(ThisExpression& node) { setScope(node); }

void SymbolTableBuilder::visit(InterpolatedStringExpression& node) {
    setScope(node);
    for (auto& part : node.parts) {
        if (part.kind == InterpolatedPartKind::Expression && part.expression) {
            part.expression->accept(*this);
        }
    }
}

void SymbolTableBuilder::visit(IdentifierExpression& node) { setScope(node); }
void SymbolTableBuilder::visit(QualifiedNameExpression& node) { setScope(node); }

void SymbolTableBuilder::visit(MemberAccessExpression& node) {
    setScope(node);
    if (node.object) node.object->accept(*this);
}

void SymbolTableBuilder::visit(BinaryExpression& node) {
    setScope(node);
    if (node.left) node.left->accept(*this);
    if (node.right) node.right->accept(*this);
}

void SymbolTableBuilder::visit(UnaryExpression& node) {
    setScope(node);
    if (node.operand) node.operand->accept(*this);
}

void SymbolTableBuilder::visit(AssignmentExpression& node) {
    setScope(node);
    if (node.target) node.target->accept(*this);
    if (node.value) node.value->accept(*this);
}

void SymbolTableBuilder::visit(TernaryExpression& node) {
    setScope(node);
    if (node.condition) node.condition->accept(*this);
    if (node.thenExpr) node.thenExpr->accept(*this);
    if (node.elseExpr) node.elseExpr->accept(*this);
}

void SymbolTableBuilder::visit(IndexExpression& node) {
    setScope(node);
    if (node.object) node.object->accept(*this);
    if (node.index) node.index->accept(*this);
}

void SymbolTableBuilder::visit(CallExpression& node) {
    setScope(node);
    if (node.callee) node.callee->accept(*this);
    if (node.arguments) {
        setScope(*node.arguments);
        for (auto& arg : node.arguments->expressions) {
            if (arg) arg->accept(*this);
        }
    }
}

void SymbolTableBuilder::visit(CastExpression& node) {
    setScope(node);
    if (node.operand) node.operand->accept(*this);
}

void SymbolTableBuilder::visit(NewExpression& node) {
    setScope(node);
    if (node.arguments) {
        setScope(*node.arguments);
        for (auto& arg : node.arguments->expressions) {
            if (arg) arg->accept(*this);
        }
    }
    if (node.arraySize) node.arraySize->accept(*this);
}

void SymbolTableBuilder::visit(SizeOfExpression& node) { setScope(node); }

void SymbolTableBuilder::visit(TupleExpression& node) {
    setScope(node);
    for (auto& elem : node.elements) {
        if (elem) elem->accept(*this);
    }
}

void SymbolTableBuilder::visit(MatchExpression& node) {
    setScope(node);
    if (node.subject) node.subject->accept(*this);

    for (auto& arm : node.arms) {
        // Each arm gets its own block scope (for binding patterns)
        auto armScope = std::make_shared<BlockScope>(symbolTable_.getCurrentScope());
        symbolTable_.getCurrentScope()->nest(armScope);
        symbolTable_.pushScope(armScope);

        if (arm.pattern) {
            arm.pattern->astScopeNode = armScope;
            // Binding patterns create variables
            if (auto* idPat = arm.pattern->as<IdentifierPattern>()) {
                auto bindSym = std::make_shared<VariableSymbol>(idPat->name, nullptr);
                bindSym->role = VariableRole::Local;
                symbolTable_.defineSymbol(bindSym);
                idPat->resolvedSymbol = bindSym;
                if (idPat->guard) idPat->guard->accept(*this);
            }
        }

        if (arm.body) arm.body->accept(*this);

        symbolTable_.popScope();
    }
}

void SymbolTableBuilder::visit(PipeExpression& node) {
    setScope(node);
    if (node.input) node.input->accept(*this);
    for (auto& stage : node.stages) {
        if (stage.function) stage.function->accept(*this);
        for (auto& arg : stage.extraArguments) {
            if (arg) arg->accept(*this);
        }
    }
}

void SymbolTableBuilder::visit(LambdaExpression& node) {
    setScope(node);

    // Create anonymous function scope for the lambda
    auto lambdaScope = std::make_shared<BlockScope>(symbolTable_.getCurrentScope());
    lambdaScope->label = "__lambda";
    symbolTable_.getCurrentScope()->nest(lambdaScope);
    symbolTable_.pushScope(lambdaScope);

    // Create parameter symbols in the lambda scope
    for (auto& param : node.parameters) {
        auto paramSym = std::make_shared<VariableSymbol>(param->name, nullptr);
        paramSym->role = VariableRole::Parameter;
        paramSym->isReference = param->isReference;
        symbolTable_.defineSymbol(paramSym);
        param->resolvedSymbol = paramSym;
        setScope(*param);
    }

    // Visit lambda body
    if (node.body) {
        node.body->accept(*this);
    }

    symbolTable_.popScope();
}

// ============================================================================
// Import (just set scope — actual resolution in Phase 1b)
// ============================================================================

void SymbolTableBuilder::visit(ImportDeclaration& node) {
    setScope(node);
}

} // namespace mingus
