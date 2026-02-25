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

    // Sub-pass A: pre-register all type names (enables forward references)
    for (auto& decl : node.declarations) {
        if (!decl) continue;
        if (auto* cls = decl->as<ClassDeclaration>())             preRegisterType(*cls);
        else if (auto* s = decl->as<StructDeclaration>())       preRegisterType(*s);
        else if (auto* i = decl->as<InterfaceDeclaration>())    preRegisterType(*i);
        else if (auto* e = decl->as<EnumDeclaration>())         preRegisterType(*e);
        else if (auto* o = decl->as<OpaqueTypeDeclaration>())   preRegisterType(*o);
        else if (auto* es = decl->as<ExternStructDeclaration>()) preRegisterType(*es);
        else if (auto* u = decl->as<UnionDeclaration>())        preRegisterType(*u);
        else if (auto* eu = decl->as<ExternUnionDeclaration>()) preRegisterType(*eu);
        else if (auto* tu = decl->as<TaggedUnionDeclaration>()) preRegisterType(*tu);
    }

    // Sub-pass B: full processing (existing traversal)
    for (auto& decl : node.declarations) {
        if (decl) decl->accept(*this);
    }

    // Sub-pass C: finalize inheritance (copy inherited fields for forward-ref bases)
    for (auto& decl : node.declarations) {
        if (!decl) continue;
        if (auto* cls = decl->as<ClassDeclaration>()) {
            auto* classSym = cls->resolvedClass.get();
            if (classSym && classSym->resolvedBaseClass) {
                // Re-build allFields from base (base is now fully populated)
                classSym->allFields = classSym->resolvedBaseClass->allFields;
                for (auto& field : classSym->fields) {
                    classSym->allFields.push_back(field);
                }
                // Re-build vtable (base vtable is now complete)
                classSym->vtable.clear();
                classSym->vtableSize = 0;
                buildVtable(classSym);
            }
        }
    }

    symbolTable_.popScope();
}

// ============================================================================
// Forward reference pre-registration (Sub-pass A)
// ============================================================================

void SymbolTableBuilder::preRegisterType(ClassDeclaration& node) {
    auto classSym = std::make_shared<ClassSymbol>(
        node.name, symbolTable_.getCurrentScope());
    classSym->accessLevel = node.accessModifier;
    classSym->isAbstract = node.isAbstract;
    classSym->baseClassNames = node.baseClasses;
    symbolTable_.defineSymbol(classSym);
    symbolTable_.registerType(node.name, classSym);
    node.resolvedClass = classSym;
}

void SymbolTableBuilder::preRegisterType(StructDeclaration& node) {
    auto structSym = std::make_shared<StructSymbol>(node.name);
    structSym->accessLevel = node.accessModifier;
    symbolTable_.defineSymbol(structSym);
    symbolTable_.registerType(node.name, structSym);
    node.resolvedStruct = structSym;
}

void SymbolTableBuilder::preRegisterType(InterfaceDeclaration& node) {
    auto ifaceSym = std::make_shared<InterfaceSymbol>(node.name);
    ifaceSym->accessLevel = node.accessModifier;
    symbolTable_.defineSymbol(ifaceSym);
    symbolTable_.registerType(node.name, ifaceSym);
    node.resolvedInterface = ifaceSym;
}

void SymbolTableBuilder::preRegisterType(EnumDeclaration& node) {
    auto enumSym = std::make_shared<EnumSymbol>(node.name);
    enumSym->accessLevel = node.accessModifier;
    symbolTable_.defineSymbol(enumSym);
    symbolTable_.registerType(node.name, enumSym);
    node.resolvedEnum = enumSym;
}

void SymbolTableBuilder::preRegisterType(OpaqueTypeDeclaration& node) {
    auto opaqueSym = std::make_shared<OpaqueTypeSymbol>(node.name);
    symbolTable_.defineSymbol(opaqueSym);
    symbolTable_.registerType(node.name, opaqueSym);
    node.resolvedType = opaqueSym;
}

void SymbolTableBuilder::preRegisterType(ExternStructDeclaration& node) {
    auto structSym = std::make_shared<StructSymbol>(node.name);
    structSym->isExtern = true;
    symbolTable_.defineSymbol(structSym);
    symbolTable_.registerType(node.name, structSym);
    node.resolvedStruct = structSym;
}

void SymbolTableBuilder::preRegisterType(UnionDeclaration& node) {
    auto sym = std::make_shared<StructSymbol>(node.name);
    sym->isUnion = true;
    sym->accessLevel = node.accessModifier;
    symbolTable_.defineSymbol(sym);
    symbolTable_.registerType(node.name, sym);
    node.resolvedUnion = sym;
}

void SymbolTableBuilder::preRegisterType(ExternUnionDeclaration& node) {
    auto sym = std::make_shared<StructSymbol>(node.name);
    sym->isUnion = true;
    sym->isExtern = true;
    symbolTable_.defineSymbol(sym);
    symbolTable_.registerType(node.name, sym);
    node.resolvedUnion = sym;
}

void SymbolTableBuilder::preRegisterType(TaggedUnionDeclaration& node) {
    auto tuSym = std::make_shared<TaggedUnionSymbol>(node.name);
    tuSym->accessLevel = node.accessModifier;
    symbolTable_.defineSymbol(tuSym);
    symbolTable_.registerType(node.name, tuSym);
    node.resolvedTaggedUnion = tuSym;
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

    // Determine variable role based on scope context
    if (inTypeScope_) {
        varSym->role = VariableRole::Field;
    } else if (dynamic_cast<ModuleSymbol*>(symbolTable_.getCurrentScope().get())) {
        varSym->role = VariableRole::Global;
    } else {
        varSym->role = VariableRole::Local;
    }

    varSym->isInferred = node.isInferred;
    varSym->isMutable = !node.isConst;
    varSym->accessLevel = node.accessModifier;
    symbolTable_.defineSymbol(varSym);
    node.resolvedVariable = varSym;

    // Walk initializer for nested lambdas/declarations
    if (node.initializer) {
        node.initializer->accept(*this);
    }
}

void SymbolTableBuilder::visit(ExternVariableDeclaration& node) {
    setScope(node);

    auto varSym = std::make_shared<VariableSymbol>(node.name, nullptr);
    varSym->role = VariableRole::Global;
    varSym->isExtern = true;
    varSym->isMutable = true;
    symbolTable_.defineSymbol(varSym);
    node.resolvedVariable = varSym;
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

    // Generic type parameters
    for (auto& tp : node.typeParameters) {
        funcSym->typeParameterNames.push_back(tp);
    }
    if (funcSym->isGenericTemplate()) {
        funcSym->genericASTNode = &node;
    }

    symbolTable_.defineSymbol(funcSym);
    node.resolvedFunction = funcSym;

    if (node.body) {
        // FunctionSymbol IS the scope — push it
        symbolTable_.pushScope(funcSym);
        node.body->astScopeNode = funcSym;  // body shares function scope

        // Register type parameter symbols in function scope
        if (funcSym->isGenericTemplate()) {
            for (auto& tpName : funcSym->typeParameterNames) {
                auto tpSym = std::make_shared<TypeParameterSymbol>(tpName);
                symbolTable_.defineSymbol(tpSym);
            }
            // Set astScopeNode on parameter types AND return type so TypeResolver
            // can find TypeParameterSymbol("T") during type resolution
            for (auto& param : node.parameters) {
                if (param && param->type) {
                    param->type->astScopeNode = funcSym;
                }
            }
            if (node.returnType) {
                node.returnType->astScopeNode = funcSym;
            }
        }

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
    if (node.isCopyConstructor) {
        ctorSym->isCopyConstructor = true;
    } else if (node.isMoveConstructor) {
        ctorSym->isMoveConstructor = true;
    }
    symbolTable_.defineSymbol(ctorSym);
    node.resolvedConstructor = ctorSym;

    if (currentClass_) {
        if (node.isMoveConstructor) {
            currentClass_->moveConstructor = ctorSym;
        } else if (node.isCopyConstructor) {
            currentClass_->copyConstructor = ctorSym;
        } else {
            currentClass_->constructors.push_back(ctorSym);
        }
    }

    if (node.body) {
        symbolTable_.pushScope(ctorSym);
        node.body->astScopeNode = ctorSym;
        createParameterSymbols(node.parameters, ctorSym);

        // Set scope for super constructor arguments (they reference ctor params)
        for (auto& arg : node.superArgs) {
            if (arg) arg->astScopeNode = ctorSym;
        }

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
    funcSym->isVariadic = node.isVariadic;

    // Map calling convention attribute
    if (node.callingConvention == "stdcall") {
        funcSym->callingConvention = CallingConvention::StdCall;
    } else if (node.callingConvention == "fastcall") {
        funcSym->callingConvention = CallingConvention::FastCall;
    }
    // CDecl is default

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

    std::shared_ptr<EnumSymbol> enumSym;
    if (node.resolvedEnum) {
        enumSym = node.resolvedEnum;  // already pre-registered
    } else {
        enumSym = std::make_shared<EnumSymbol>(node.name);
        enumSym->accessLevel = node.accessModifier;
        symbolTable_.defineSymbol(enumSym);
        symbolTable_.registerType(node.name, enumSym);
        node.resolvedEnum = enumSym;
    }
    enumSym->isExtern = node.isExtern;

    // Populate members: explicit integer values or auto-increment
    int64_t nextValue = 0;
    for (auto& member : node.members) {
        if (member) {
            setScope(*member);
            EnumSymbol::MemberInfo info;
            info.name = member->name;
            info.hasExplicitValue = (member->value != nullptr);

            if (info.hasExplicitValue) {
                // Evaluate constant literal value
                if (auto* intLit = member->value->as<IntegerLiteral>()) {
                    info.intValue = intLit->value;
                    nextValue = info.intValue + 1;
                } else if (auto* strLit = member->value->as<StringLiteral>()) {
                    info.stringValue = strLit->value;
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

    std::shared_ptr<StructSymbol> structSym;
    if (node.resolvedStruct) {
        structSym = node.resolvedStruct;  // already pre-registered
    } else {
        structSym = std::make_shared<StructSymbol>(node.name);
        structSym->accessLevel = node.accessModifier;
        symbolTable_.defineSymbol(structSym);
        symbolTable_.registerType(node.name, structSym);
        node.resolvedStruct = structSym;
    }

    // Layout attributes
    structSym->isPacked = node.isPacked;
    structSym->alignment = node.alignment;

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

    std::shared_ptr<ClassSymbol> classSym;
    if (node.resolvedClass) {
        classSym = node.resolvedClass;  // already pre-registered
    } else {
        classSym = std::make_shared<ClassSymbol>(
            node.name, symbolTable_.getCurrentScope());
        classSym->accessLevel = node.accessModifier;
        classSym->isAbstract = node.isAbstract;
        classSym->baseClassNames = node.baseClasses;
        symbolTable_.defineSymbol(classSym);
        symbolTable_.registerType(node.name, classSym);
        node.resolvedClass = classSym;
    }

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

    // Constructors (explicit or auto-generated)
    if (!node.constructors.empty()) {
        for (auto& ctor : node.constructors) {
            ctor->accept(*this);
        }
    } else {
        // No explicit regular constructor — auto-generate default
        autoGenerateConstructor(node, classSym);
    }

    // Copy constructor (if defined)
    if (node.copyConstructor) {
        node.copyConstructor->accept(*this);
    }

    // Move constructor (if defined)
    if (node.moveConstructor) {
        node.moveConstructor->accept(*this);
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

    // Mark constructor overloads for name mangling
    if (classSym->constructors.size() > 1) {
        for (auto& ctor : classSym->constructors) {
            ctor->hasOverloads = true;
        }
    }

    inTypeScope_ = savedInType;
    currentClass_ = savedClass;
    symbolTable_.popScope();
}

void SymbolTableBuilder::visit(InterfaceDeclaration& node) {
    setScope(node);

    std::shared_ptr<InterfaceSymbol> ifaceSym;
    if (node.resolvedInterface) {
        ifaceSym = node.resolvedInterface;  // already pre-registered
    } else {
        ifaceSym = std::make_shared<InterfaceSymbol>(node.name);
        ifaceSym->accessLevel = node.accessModifier;
        symbolTable_.defineSymbol(ifaceSym);
        symbolTable_.registerType(node.name, ifaceSym);
        node.resolvedInterface = ifaceSym;
    }

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

    // Assign vtableIndex to each interface method (used for itable dispatch)
    for (size_t i = 0; i < ifaceSym->methods.size(); i++) {
        if (ifaceSym->methods[i]) {
            ifaceSym->methods[i]->vtableIndex = static_cast<int>(i);
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
    classSym->constructors.push_back(ctorSym);

    // Create empty body node for AST consistency
    auto body = std::make_shared<BlockStatementNode>();
    body->astScopeNode = ctorSym;
    auto ctorDecl = std::make_shared<ConstructorDeclaration>();
    ctorDecl->body = body;
    ctorDecl->resolvedConstructor = ctorSym;
    node.constructors.push_back(ctorDecl);
    setScope(*node.constructors.back());
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
        // Match by name AND parameter count (overloaded methods are distinct vtable slots)
        bool overridden = false;
        for (size_t i = 1; i < classSym->vtable.size(); i++) {
            if (classSym->vtable[i] &&
                classSym->vtable[i]->getName() == funcSym->getName() &&
                classSym->vtable[i]->parameters.size() == funcSym->parameters.size()) {
                classSym->vtable[i] = std::dynamic_pointer_cast<FunctionSymbol>(sym);
                funcSym->vtableIndex = static_cast<int>(i);
                funcSym->isVirtual = true;
                overridden = true;
                break;
            }
        }

        // New method: append to vtable (all instance methods get vtable slots
        // so derived classes can override them via virtual dispatch)
        if (!overridden) {
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

    symbolTable_.popScope();
}

void SymbolTableBuilder::visit(WhileStatement& node) {
    setScope(node);
    if (node.condition) node.condition->accept(*this);
    if (node.body) node.body->accept(*this);
}

void SymbolTableBuilder::visit(DoWhileStatement& node) {
    setScope(node);
    if (node.body) node.body->accept(*this);
    if (node.condition) node.condition->accept(*this);
}

void SymbolTableBuilder::visit(LabeledStatement& node) {
    setScope(node);
    if (node.statement) node.statement->accept(*this);
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

void SymbolTableBuilder::visit(MoveExpression& node) {
    setScope(node);
    if (node.operand) node.operand->accept(*this);
}

void SymbolTableBuilder::visit(ArrayLiteralExpression& node) {
    setScope(node);
    for (auto& elem : node.elements) {
        if (elem) elem->accept(*this);
    }
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

            // Literal patterns: set scope on value expression (e.g., Enum.Member)
            if (auto* litPat = arm.pattern->as<LiteralPattern>()) {
                if (litPat->value) {
                    litPat->value->astScopeNode = armScope;
                }
            }

            // Binding patterns create variables
            if (auto* idPat = arm.pattern->as<IdentifierPattern>()) {
                auto bindSym = std::make_shared<VariableSymbol>(idPat->name, nullptr);
                bindSym->role = VariableRole::Local;
                symbolTable_.defineSymbol(bindSym);
                idPat->resolvedSymbol = bindSym;
                if (idPat->guard) idPat->guard->accept(*this);
            }

            // Variant patterns: create binding variables for each field pattern
            if (auto* varPat = arm.pattern->as<VariantPattern>()) {
                for (auto& fieldPat : varPat->fieldPatterns) {
                    if (!fieldPat) continue;
                    fieldPat->astScopeNode = armScope;
                    if (auto* idFP = fieldPat->as<IdentifierPattern>()) {
                        auto bindSym = std::make_shared<VariableSymbol>(idFP->name, nullptr);
                        bindSym->role = VariableRole::Local;
                        symbolTable_.defineSymbol(bindSym);
                        idFP->resolvedSymbol = bindSym;
                    }
                }
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

void SymbolTableBuilder::visit(TypedefDeclaration& node) {
    setScope(node);

    auto aliasSym = std::make_shared<TypeAliasSymbol>(node.aliasName);
    symbolTable_.defineSymbol(aliasSym);
    node.resolvedTypeAlias = aliasSym;
}

void SymbolTableBuilder::visit(OpaqueTypeDeclaration& node) {
    setScope(node);
    // Already pre-registered in Sub-pass A — nothing else to do
}

void SymbolTableBuilder::visit(ExternStructDeclaration& node) {
    setScope(node);

    auto structSym = node.resolvedStruct;
    if (!structSym) return;  // pre-registration failed

    // Layout attributes
    structSym->isPacked = node.isPacked;
    structSym->alignment = node.alignment;

    // Create VariableSymbol for each field (type resolved later by Pass 2)
    symbolTable_.pushScope(structSym);
    for (size_t i = 0; i < node.fields.size(); i++) {
        auto& param = node.fields[i];
        if (!param) continue;

        auto fieldSym = std::make_shared<VariableSymbol>(
            param->name, nullptr);
        fieldSym->role = VariableRole::Field;
        fieldSym->fieldIndex = static_cast<int>(i);
        symbolTable_.defineSymbol(fieldSym);
        structSym->fields.push_back(fieldSym);
    }
    symbolTable_.popScope();
}

// ============================================================================
// Union Declarations
// ============================================================================

void SymbolTableBuilder::visit(UnionDeclaration& node) {
    setScope(node);

    std::shared_ptr<StructSymbol> unionSym;
    if (node.resolvedUnion) {
        unionSym = node.resolvedUnion;  // already pre-registered
    } else {
        unionSym = std::make_shared<StructSymbol>(node.name);
        unionSym->isUnion = true;
        unionSym->accessLevel = node.accessModifier;
        symbolTable_.defineSymbol(unionSym);
        symbolTable_.registerType(node.name, unionSym);
        node.resolvedUnion = unionSym;
    }

    // Layout attributes
    unionSym->isPacked = node.isPacked;
    unionSym->alignment = node.alignment;

    // StructSymbol IS the member scope — push it
    symbolTable_.pushScope(unionSym);
    auto savedInType = inTypeScope_;
    auto savedClass = currentClass_;
    inTypeScope_ = true;
    currentClass_ = nullptr;

    // Fields — ALL at fieldIndex 0 (overlapping layout)
    for (auto& field : node.fields) {
        if (field) {
            field->accept(*this);
            if (field->resolvedVariable) {
                field->resolvedVariable->fieldIndex = 0;  // Union: all fields at offset 0
                unionSym->fields.push_back(field->resolvedVariable);
            }
        }
    }

    // Methods
    for (auto& method : node.methods) {
        if (method) method->accept(*this);
    }

    inTypeScope_ = savedInType;
    currentClass_ = savedClass;
    symbolTable_.popScope();
}

void SymbolTableBuilder::visit(ExternUnionDeclaration& node) {
    setScope(node);

    auto unionSym = node.resolvedUnion;
    if (!unionSym) return;  // pre-registration failed

    // Layout attributes
    unionSym->isPacked = node.isPacked;
    unionSym->alignment = node.alignment;

    // Create VariableSymbol for each field (type resolved later by Pass 2)
    symbolTable_.pushScope(unionSym);
    for (size_t i = 0; i < node.fields.size(); i++) {
        auto& param = node.fields[i];
        if (!param) continue;

        auto fieldSym = std::make_shared<VariableSymbol>(
            param->name, nullptr);
        fieldSym->role = VariableRole::Field;
        fieldSym->fieldIndex = 0;  // Union: all fields at offset 0
        symbolTable_.defineSymbol(fieldSym);
        unionSym->fields.push_back(fieldSym);
    }
    symbolTable_.popScope();
}

// ============================================================================
// Tagged Union Declarations
// ============================================================================

void SymbolTableBuilder::visit(TaggedUnionDeclaration& node) {
    setScope(node);

    auto tuSym = node.resolvedTaggedUnion;
    if (!tuSym) {
        // Fallback: create symbol if preRegisterType was skipped
        tuSym = std::make_shared<TaggedUnionSymbol>(node.name);
        tuSym->accessLevel = node.accessModifier;
        symbolTable_.defineSymbol(tuSym);
        symbolTable_.registerType(node.name, tuSym);
        node.resolvedTaggedUnion = tuSym;
    }

    // Populate variant info: names + auto-assigned tags
    // Field types are resolved later by TypeResolver (Pass 2)
    int tagValue = 0;
    for (auto& variant : node.variants) {
        if (!variant) continue;
        setScope(*variant);

        TaggedUnionSymbol::VariantInfo info;
        info.name = variant->name;
        info.tagValue = tagValue++;

        // Add field stubs (type = nullptr, resolved in Pass 2)
        for (auto& field : variant->fields) {
            TaggedUnionSymbol::VariantField vf;
            vf.name = field.name;
            vf.type = nullptr;  // resolved by TypeResolver
            info.fields.push_back(std::move(vf));
        }

        tuSym->variants.push_back(std::move(info));
    }
}

} // namespace mingus
