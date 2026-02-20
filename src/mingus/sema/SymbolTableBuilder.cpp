//================================================================================
// MINGUS v1 - Symbol Table Builder Implementation (Pass 1)
// Walks the AST top-down, creates symbols, builds scope tree.
//================================================================================

#include "mingus/sema/SymbolTableBuilder.h"

namespace mingus {
namespace sema {

//================================================================================
// Utility: convert OperatorKind (AST) to OverloadableOp (sema)
//================================================================================
OverloadableOp operatorKindToOverloadableOp(OperatorKind kind) {
    switch (kind) {
        case OperatorKind::Plus:         return OverloadableOp::Plus;
        case OperatorKind::Minus:        return OverloadableOp::Minus;
        case OperatorKind::Star:         return OverloadableOp::Star;
        case OperatorKind::Divide:       return OverloadableOp::Slash;
        case OperatorKind::Modulo:       return OverloadableOp::Modulo;
        case OperatorKind::Equal:        return OverloadableOp::Equals;
        case OperatorKind::NotEqual:     return OverloadableOp::NotEquals;
        case OperatorKind::Less:         return OverloadableOp::Less;
        case OperatorKind::LessEqual:    return OverloadableOp::LessEqual;
        case OperatorKind::Greater:      return OverloadableOp::Greater;
        case OperatorKind::GreaterEqual: return OverloadableOp::GreaterEqual;
        case OperatorKind::Index:        return OverloadableOp::Index;
    }
    return OverloadableOp::Plus; // unreachable
}

//================================================================================
// TypeSymbol lookup helpers
//================================================================================
VariableSymbol* TypeSymbol::findField(const std::string& fieldName) const {
    if (!memberScope) return nullptr;
    auto* sym = memberScope->lookupLocal(fieldName);
    if (sym && sym->is<VariableSymbol>()) {
        return sym->as<VariableSymbol>();
    }
    // Walk inheritance chain for classes
    if (kind == SymbolKind::Class) {
        auto* classSym = static_cast<const ClassSymbol*>(this);
        if (classSym->baseClass) {
            return classSym->baseClass->findField(fieldName);
        }
    }
    return nullptr;
}

FunctionSymbol* TypeSymbol::findMethod(const std::string& methodName) const {
    if (!memberScope) return nullptr;
    auto* sym = memberScope->lookupLocal(methodName);
    if (sym && sym->is<FunctionSymbol>()) {
        return sym->as<FunctionSymbol>();
    }
    // Walk inheritance chain for classes
    if (kind == SymbolKind::Class) {
        auto* classSym = static_cast<const ClassSymbol*>(this);
        if (classSym->baseClass) {
            return classSym->baseClass->findMethod(methodName);
        }
    }
    return nullptr;
}

OperatorSymbol* TypeSymbol::findOperator(OverloadableOp op) const {
    if (!memberScope) return nullptr;
    auto* found = memberScope->lookupOperator(op);
    if (found) return found;
    // Walk inheritance chain for classes
    if (kind == SymbolKind::Class) {
        auto* classSym = static_cast<const ClassSymbol*>(this);
        if (classSym->baseClass) {
            return classSym->baseClass->findOperator(op);
        }
    }
    return nullptr;
}

//================================================================================
// Constructor
//================================================================================
SymbolTableBuilder::SymbolTableBuilder(SymbolTable& table, ErrorReporter& errors)
    : symbolTable_(table)
    , errors_(errors)
    , currentScope_(nullptr)
{
}

//================================================================================
// Entry point
//================================================================================
void SymbolTableBuilder::build(ProgramNode& program) {
    currentScope_ = symbolTable_.createGlobalScope();
    visit(program);
}

//================================================================================
// Helpers
//================================================================================
bool SymbolTableBuilder::defineSymbol(Symbol* symbol) {
    if (!currentScope_->define(symbol)) {
        errors_.error(symbol->location,
                      "'" + symbol->name + "' is already defined in this scope");
        return false;
    }
    return true;
}

Scope* SymbolTableBuilder::pushScope(ScopeKind kind, Symbol* owner) {
    currentScope_ = currentScope_->createChild(kind, owner);
    return currentScope_;
}

void SymbolTableBuilder::popScope() {
    if (currentScope_->parent) {
        currentScope_ = currentScope_->parent;
    }
}

void SymbolTableBuilder::visitStatements(NodeList<StatementNode>& stmts) {
    for (auto& stmt : stmts) {
        if (stmt) stmt->accept(*this);
    }
}

bool SymbolTableBuilder::isInTypeScope() const {
    return currentScope_ && currentScope_->kind == ScopeKind::TypeMembers;
}

//================================================================================
// Program Structure
//================================================================================
void SymbolTableBuilder::visit(ProgramNode& node) {
    // Pass 1a: Visit all modules (builds scopes, symbols, declarations).
    // ImportNode::accept() is a no-op during this sub-pass because imported
    // modules haven't been processed yet.
    for (auto& mod : node.modules) {
        if (mod) mod->accept(*this);
    }

    // Pass 1b: Resolve imports. All module scopes now exist in the global scope,
    // so we can look up symbols from the source module and define them (or aliases)
    // in the importing module's scope.
    resolveAllImports(node);
}

void SymbolTableBuilder::visit(ModuleNode& node) {
    auto* sym = symbolTable_.createSymbol<ModuleSymbol>(node.name, node.location);
    defineSymbol(sym);

    auto* moduleScope = pushScope(ScopeKind::Module, sym);
    sym->moduleScope = moduleScope;

    // Visit imports (no-op during Pass 1a — resolved later in Pass 1b)
    for (auto& imp : node.imports) {
        if (imp) imp->accept(*this);
    }

    // Visit declarations
    for (auto& decl : node.declarations) {
        if (decl) decl->accept(*this);
    }

    popScope();
}

void SymbolTableBuilder::visit(ImportNode& /*node*/) {
    // No-op during Pass 1a. Imports are resolved in resolveAllImports (Pass 1b).
}

//================================================================================
// Import Resolution (Pass 1b)
//================================================================================
void SymbolTableBuilder::resolveAllImports(ProgramNode& node) {
    Scope* globalScope = symbolTable_.getGlobalScope();

    for (auto& mod : node.modules) {
        if (!mod || mod->imports.empty()) continue;

        // Find this module's symbol and scope
        auto* modSym = globalScope->lookupLocal(mod->name);
        if (!modSym || !modSym->is<ModuleSymbol>()) continue;
        auto* moduleScope = modSym->as<ModuleSymbol>()->moduleScope;
        if (!moduleScope) continue;

        for (auto& imp : mod->imports) {
            if (!imp) continue;

            if (imp->isFromImport()) {
                // "import add, mul from Math;" — selective import
                std::string srcModuleName = imp->source.getSimpleName();
                auto* srcModSym = globalScope->lookupLocal(srcModuleName);
                if (!srcModSym || !srcModSym->is<ModuleSymbol>()) {
                    errors_.error(imp->location,
                        "import: module '" + srcModuleName + "' not found");
                    continue;
                }
                auto* srcScope = srcModSym->as<ModuleSymbol>()->moduleScope;
                if (!srcScope) continue;

                for (auto& target : imp->targets) {
                    auto* sym = srcScope->lookupLocal(target.name);
                    if (!sym) {
                        errors_.error(imp->location,
                            "import: '" + target.name + "' not found in module '"
                            + srcModuleName + "'");
                        continue;
                    }

                    std::string effectiveName = target.getEffectiveName();
                    if (target.alias.has_value()) {
                        // "import add as myAdd from Math;"
                        if (!moduleScope->defineAs(effectiveName, sym)) {
                            errors_.error(imp->location,
                                "import: '" + effectiveName
                                + "' already defined in this scope");
                        }
                    } else {
                        // "import add from Math;" — define under original name
                        if (!moduleScope->defineAs(target.name, sym)) {
                            errors_.error(imp->location,
                                "import: '" + target.name
                                + "' already defined in this scope");
                        }
                    }
                }
            } else {
                // "import Math;" — whole module import (each target is a module name)
                for (auto& target : imp->targets) {
                    std::string srcModuleName = target.name;
                    auto* srcModSym = globalScope->lookupLocal(srcModuleName);
                    if (!srcModSym || !srcModSym->is<ModuleSymbol>()) {
                        errors_.error(imp->location,
                            "import: module '" + srcModuleName + "' not found");
                        continue;
                    }
                    auto* srcScope = srcModSym->as<ModuleSymbol>()->moduleScope;
                    if (!srcScope) continue;

                    // Import all public symbols from the source module
                    for (auto& [name, sym] : srcScope->getSymbolMap()) {
                        if (!sym->isPublic()) continue;
                        if (moduleScope->lookupLocal(name)) continue;  // Skip if already defined
                        moduleScope->defineAs(name, sym);
                    }

                    // Import operators too
                    // (not needed for now — operators are resolved via type, not name)
                }
            }
        }
    }
}

//================================================================================
// Declarations
//================================================================================
void SymbolTableBuilder::visit(StructDeclaration& node) {
    auto* sym = symbolTable_.createSymbol<StructSymbol>(
        node.name, node.location, currentScope_->ownerSymbol);
    sym->accessLevel = node.accessModifier;
    defineSymbol(sym);

    auto* memberScope = pushScope(ScopeKind::TypeMembers, sym);
    sym->memberScope = memberScope;

    // Fields
    int fieldIdx = 0;
    for (auto& field : node.fields) {
        if (field) {
            field->accept(*this);
            // Set field index on the last defined variable
            auto* fieldSym = currentScope_->lookupLocal(field->name);
            if (fieldSym && fieldSym->is<VariableSymbol>()) {
                auto* varSym = fieldSym->as<VariableSymbol>();
                varSym->role = VariableRole::Field;
                varSym->fieldIndex = fieldIdx++;
                sym->fields.push_back(varSym);
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

    popScope();
}

void SymbolTableBuilder::visit(ClassDeclaration& node) {
    auto* sym = symbolTable_.createSymbol<ClassSymbol>(
        node.name, node.location, currentScope_->ownerSymbol);
    sym->accessLevel = node.accessModifier;
    sym->isAbstract = node.isAbstract;
    defineSymbol(sym);

    auto* memberScope = pushScope(ScopeKind::TypeMembers, sym);
    sym->memberScope = memberScope;

    // Resolve base class and interfaces
    // Syntax: class Dog : Animal, Drawable, Resizable { ... }
    // First base class name → class (single inheritance); others → interfaces
    for (auto& baseName : node.baseClasses) {
        const auto& nameStr = baseName.getSimpleName();
        // Look up in module scope (parent of current TypeMembers scope)
        Symbol* baseSym = currentScope_->parent->lookup(nameStr);
        if (!baseSym) {
            errors_.error(node.location, "'" + nameStr + "' not found");
        } else if (auto* ifaceSym = baseSym->as<InterfaceSymbol>()) {
            sym->implementedInterfaces.push_back(ifaceSym);
        } else if (auto* classSym2 = baseSym->as<ClassSymbol>()) {
            if (sym->baseClass) {
                errors_.error(node.location,
                    "multiple class inheritance is not supported");
            } else {
                sym->baseClass = classSym2;
            }
        } else {
            errors_.error(node.location,
                "'" + nameStr + "' is not a class or interface");
        }
    }

    // Fields
    int fieldIdx = 0;
    for (auto& field : node.fields) {
        if (field) {
            field->accept(*this);
            auto* fieldSym = currentScope_->lookupLocal(field->name);
            if (fieldSym && fieldSym->is<VariableSymbol>()) {
                auto* varSym = fieldSym->as<VariableSymbol>();
                varSym->role = VariableRole::Field;
                varSym->fieldIndex = fieldIdx++;
                sym->fields.push_back(varSym);
            }
        }
    }

    // Constructor
    if (node.constructor) {
        node.constructor->accept(*this);
        auto* ctorSym = currentScope_->lookupLocal("constructor");
        if (ctorSym && ctorSym->is<ConstructorSymbol>()) {
            sym->constructor = ctorSym->as<ConstructorSymbol>();
        }
    }

    // Auto-generate default constructor if none declared
    if (!sym->constructor) {
        auto emptyBody = std::make_shared<BlockStatement>(
            NodeList<StatementNode>{});
        node.constructor = std::make_shared<ConstructorDeclaration>(
            AccessModifier::Public,
            NodeList<ParameterNode>{},
            emptyBody,
            NodeList<ExpressionNode>{},  // no super args
            sym->baseClass != nullptr,   // hasSuperCall if base exists
            node.location);
        auto* ctorSym = symbolTable_.createSymbol<ConstructorSymbol>(
            node.location, currentScope_->ownerSymbol);
        ctorSym->accessLevel = AccessModifier::Public;
        defineSymbol(ctorSym);
        auto* fnScope = pushScope(ScopeKind::Function, ctorSym);
        ctorSym->bodyScope = fnScope;
        popScope();
        sym->constructor = ctorSym;
    }

    // Destructor
    if (node.destructor) {
        node.destructor->accept(*this);
        auto* dtorSym = currentScope_->lookupLocal("destructor");
        if (dtorSym && dtorSym->is<DestructorSymbol>()) {
            sym->destructor = dtorSym->as<DestructorSymbol>();
        }
    }

    // Auto-generate default destructor if none declared
    if (!sym->destructor) {
        auto emptyBody = std::make_shared<BlockStatement>(
            NodeList<StatementNode>{});
        node.destructor = std::make_shared<DestructorDeclaration>(
            emptyBody, node.location);
        auto* dtorSym = symbolTable_.createSymbol<DestructorSymbol>(
            node.location, currentScope_->ownerSymbol);
        defineSymbol(dtorSym);
        auto* fnScope = pushScope(ScopeKind::Function, dtorSym);
        dtorSym->bodyScope = fnScope;
        popScope();
        sym->destructor = dtorSym;
    }

    // Methods
    for (auto& method : node.methods) {
        if (method) method->accept(*this);
    }

    // Operators
    for (auto& op : node.operators) {
        if (op) op->accept(*this);
    }

    // Build vtable and allFields after all members are processed
    buildVtable(sym);

    popScope();
}

void SymbolTableBuilder::visit(InterfaceDeclaration& node) {
    auto* sym = symbolTable_.createSymbol<InterfaceSymbol>(
        node.name, node.location, currentScope_->ownerSymbol);
    sym->accessLevel = node.accessModifier;
    defineSymbol(sym);

    auto* memberScope = pushScope(ScopeKind::TypeMembers, sym);
    sym->memberScope = memberScope;

    // Process methods: all abstract, vtableIndex = position in interface
    for (auto& method : node.methods) {
        if (method) {
            method->accept(*this);  // Creates FunctionSymbol in memberScope
            auto* fnSym = memberScope->lookupLocal(method->name);
            if (fnSym && fnSym->is<FunctionSymbol>()) {
                auto* methodSym = fnSym->as<FunctionSymbol>();
                methodSym->isAbstract = true;
                methodSym->vtableIndex = static_cast<int>(sym->methods.size());
                sym->methods.push_back(methodSym);
            }
        }
    }

    popScope();
}

void SymbolTableBuilder::buildVtable(ClassSymbol* sym) {
    if (!sym) return;

    // Step 1: Inherit from base class
    if (sym->baseClass) {
        sym->vtable = sym->baseClass->vtable;       // Copy base vtable slots
        sym->allFields = sym->baseClass->allFields;  // Copy inherited fields
    }

    // Add own fields to allFields
    for (auto* field : sym->fields) {
        sym->allFields.push_back(field);
    }

    // Step 2: Reserve vtable slot 0 for the destructor (virtual destructor support)
    if (sym->vtable.empty()) {
        // Root class: insert destructor at slot 0
        if (sym->destructor) {
            sym->destructor->vtableIndex = 0;
            sym->vtable.push_back(sym->destructor);
        }
    } else {
        // Derived class: slot 0 is already the base destructor — override it
        if (sym->destructor) {
            sym->destructor->vtableIndex = 0;
            sym->vtable[0] = sym->destructor;
        }
    }

    // Step 3: Build vtable entries from this class's methods
    if (sym->memberScope) {
        for (auto& [name, msym] : sym->memberScope->getSymbolMap()) {
            auto* methodSym = msym->as<FunctionSymbol>();
            if (!methodSym) continue;
            if (!methodSym->isMethod || methodSym->isStatic) continue;
            if (methodSym->kind == SymbolKind::Constructor) continue;
            if (methodSym->kind == SymbolKind::Destructor) continue;

            // Check if this overrides a base class method
            bool overridden = false;
            for (size_t i = 0; i < sym->vtable.size(); ++i) {
                if (sym->vtable[i]->name == methodSym->name) {
                    // Override: replace the slot
                    sym->vtable[i] = methodSym;
                    methodSym->vtableIndex = static_cast<int>(i);
                    overridden = true;
                    break;
                }
            }
            if (!overridden) {
                // New method: append to vtable
                methodSym->vtableIndex = static_cast<int>(sym->vtable.size());
                sym->vtable.push_back(methodSym);
            }
        }
    }

    sym->vtableSize = static_cast<int>(sym->vtable.size());
}

void SymbolTableBuilder::visit(EnumDeclaration& node) {
    auto* sym = symbolTable_.createSymbol<EnumSymbol>(
        node.name, node.location, currentScope_->ownerSymbol);
    sym->accessLevel = node.accessModifier;

    // Evaluate enum member values
    int64_t nextValue = 0;
    for (auto& member : node.members) {
        if (member) {
            if (member->value) {
                if (auto* strLit = member->value->as<StringLiteral>()) {
                    sym->members.emplace_back(member->name, strLit->value);
                    continue;
                }
                if (auto* intLit = member->value->as<IntegerLiteral>()) {
                    nextValue = intLit->value;
                }
            }
            sym->members.emplace_back(member->name, nextValue);
            nextValue = nextValue + 1;
        }
    }

    defineSymbol(sym);
}

void SymbolTableBuilder::visit(FunctionDeclaration& node) {
    auto* sym = symbolTable_.createSymbol<FunctionSymbol>(
        node.name, node.location, currentScope_->ownerSymbol);
    sym->accessLevel = node.accessModifier;
    sym->isMethod = isInTypeScope();
    sym->isStatic = node.isStatic;
    sym->hasThisParam = sym->isMethod && !sym->isStatic;
    sym->isAbstract = node.isAbstract;
    defineSymbol(sym);

    if (node.body) {
        auto* fnScope = pushScope(ScopeKind::Function, sym);
        sym->bodyScope = fnScope;

        // Create parameter symbols
        for (auto& param : node.parameters) {
            if (param) {
                auto* paramSym = symbolTable_.createSymbol<VariableSymbol>(
                    param->name, VariableRole::Parameter, param->location, sym);
                defineSymbol(paramSym);
                sym->parameters.push_back(paramSym);
            }
        }

        // Visit function body
        visitStatements(node.body->statements);

        popScope();
    } else {
        // Abstract/interface method: create parameter symbols without a bodyScope
        // (same pattern as extern functions) so buildFunctionType works correctly
        for (auto& param : node.parameters) {
            if (param) {
                auto* paramSym = symbolTable_.createSymbol<VariableSymbol>(
                    param->name, VariableRole::Parameter, param->location, sym);
                sym->parameters.push_back(paramSym);
            }
        }
    }
}

void SymbolTableBuilder::visit(ConstructorDeclaration& node) {
    auto* sym = symbolTable_.createSymbol<ConstructorSymbol>(
        node.location, currentScope_->ownerSymbol);
    sym->accessLevel = node.accessModifier;
    defineSymbol(sym);

    if (node.body) {
        auto* fnScope = pushScope(ScopeKind::Function, sym);
        sym->bodyScope = fnScope;

        // Create parameter symbols
        for (auto& param : node.parameters) {
            if (param) {
                auto* paramSym = symbolTable_.createSymbol<VariableSymbol>(
                    param->name, VariableRole::Parameter, param->location, sym);
                defineSymbol(paramSym);
                sym->parameters.push_back(paramSym);
            }
        }

        // Visit constructor body
        visitStatements(node.body->statements);

        popScope();
    }
}

void SymbolTableBuilder::visit(DestructorDeclaration& node) {
    auto* sym = symbolTable_.createSymbol<DestructorSymbol>(
        node.location, currentScope_->ownerSymbol);
    defineSymbol(sym);

    if (node.body) {
        auto* fnScope = pushScope(ScopeKind::Function, sym);
        sym->bodyScope = fnScope;

        // Visit destructor body
        visitStatements(node.body->statements);

        popScope();
    }
}

void SymbolTableBuilder::visit(OperatorDeclaration& node) {
    auto overloadOp = operatorKindToOverloadableOp(node.op);
    auto* sym = symbolTable_.createSymbol<OperatorSymbol>(
        overloadOp, node.location, currentScope_->ownerSymbol);
    defineSymbol(sym);  // Goes into operators_ list, not symbols_ map

    if (node.body) {
        auto* fnScope = pushScope(ScopeKind::Function, sym);
        sym->bodyScope = fnScope;

        // Create parameter symbols
        for (auto& param : node.parameters) {
            if (param) {
                auto* paramSym = symbolTable_.createSymbol<VariableSymbol>(
                    param->name, VariableRole::Parameter, param->location, sym);
                defineSymbol(paramSym);
                sym->parameters.push_back(paramSym);
            }
        }

        // Visit operator body
        visitStatements(node.body->statements);

        popScope();
    }
}

void SymbolTableBuilder::visit(ExternFunctionDeclaration& node) {
    auto* sym = symbolTable_.createSymbol<FunctionSymbol>(
        node.name, node.location, currentScope_->ownerSymbol);
    sym->isExtern = true;

    // Create parameter symbols (owned by symbol table, but no scope)
    for (auto& param : node.parameters) {
        if (param) {
            auto* paramSym = symbolTable_.createSymbol<VariableSymbol>(
                param->name, VariableRole::Parameter, param->location, sym);
            sym->parameters.push_back(paramSym);
        }
    }

    defineSymbol(sym);
}

void SymbolTableBuilder::visit(VariableDeclaration& node) {
    VariableRole role = isInTypeScope() ? VariableRole::Field : VariableRole::Local;

    auto* sym = symbolTable_.createSymbol<VariableSymbol>(
        node.name, role, node.location, currentScope_->ownerSymbol);
    sym->accessLevel = node.accessModifier;
    sym->isInferred = node.isInferred;
    sym->isInitialized = (node.initializer != nullptr);
    defineSymbol(sym);

    // Walk the initializer expression to find any nested declarations (lambdas, etc.)
    if (node.initializer) {
        node.initializer->accept(*this);
    }
}

void SymbolTableBuilder::visit(TupleDestructuringDeclaration& node) {
    for (auto& elem : node.elements) {
        auto* sym = symbolTable_.createSymbol<VariableSymbol>(
            elem.name, VariableRole::Local, node.location, currentScope_->ownerSymbol);
        sym->isInferred = elem.isInferred;
        sym->isInitialized = true;
        defineSymbol(sym);
    }

    // Walk the initializer
    if (node.initializer) {
        node.initializer->accept(*this);
    }
}

void SymbolTableBuilder::visit(EnumMemberNode& /*node*/) {
    // Handled in visit(EnumDeclaration)
}

void SymbolTableBuilder::visit(ParameterNode& /*node*/) {
    // Handled in function/constructor/operator visits
}

//================================================================================
// Statements
//================================================================================
void SymbolTableBuilder::visit(BlockStatement& node) {
    pushScope(ScopeKind::Block);
    visitStatements(node.statements);
    popScope();
}

void SymbolTableBuilder::visit(ExpressionStatement& node) {
    if (node.expression) {
        node.expression->accept(*this);
    }
}

void SymbolTableBuilder::visit(ReturnStatement& node) {
    if (node.value) {
        node.value->accept(*this);
    }
}

void SymbolTableBuilder::visit(IfStatement& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.thenBody) node.thenBody->accept(*this);

    for (auto& elseIf : node.elseIfClauses) {
        if (elseIf.condition) elseIf.condition->accept(*this);
        if (elseIf.body) elseIf.body->accept(*this);
    }

    if (node.elseBody) node.elseBody->accept(*this);
}

void SymbolTableBuilder::visit(SwitchStatement& node) {
    if (node.subject) node.subject->accept(*this);

    for (auto& switchCase : node.cases) {
        if (switchCase.value) switchCase.value->accept(*this);
        visitStatements(switchCase.body);
    }

    visitStatements(node.defaultCase);
}

void SymbolTableBuilder::visit(ForStatement& node) {
    // For loop creates a scope for the loop variable
    pushScope(ScopeKind::Block);

    if (node.initDeclaration) {
        node.initDeclaration->accept(*this);
    }
    for (auto& expr : node.initExpressions) {
        if (expr) expr->accept(*this);
    }

    if (node.condition) node.condition->accept(*this);

    for (auto& iter : node.iterators) {
        if (iter) iter->accept(*this);
    }

    if (node.body) node.body->accept(*this);

    popScope();
}

void SymbolTableBuilder::visit(WhileStatement& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.body) node.body->accept(*this);
}

void SymbolTableBuilder::visit(BreakStatement& /*node*/) {}
void SymbolTableBuilder::visit(ContinueStatement& /*node*/) {}

void SymbolTableBuilder::visit(DeleteStatement& node) {
    if (node.target) node.target->accept(*this);
}

void SymbolTableBuilder::visit(RawBlock& node) {
    pushScope(ScopeKind::RawBlock);
    if (node.body) {
        visitStatements(node.body->statements);
    }
    popScope();
}

//================================================================================
// Type nodes (no-op in Pass 1)
//================================================================================
void SymbolTableBuilder::visit(TypeNode& /*node*/) {}
void SymbolTableBuilder::visit(PrimitiveTypeNode& /*node*/) {}
void SymbolTableBuilder::visit(NamedTypeNode& /*node*/) {}
void SymbolTableBuilder::visit(PointerTypeNode& /*node*/) {}
void SymbolTableBuilder::visit(ArrayTypeNode& /*node*/) {}
void SymbolTableBuilder::visit(TupleTypeNode& /*node*/) {}
void SymbolTableBuilder::visit(FunctionTypeNode& /*node*/) {}

//================================================================================
// Expressions — walk children for nested declarations (lambdas)
//================================================================================
void SymbolTableBuilder::visit(IntegerLiteral& /*node*/) {}
void SymbolTableBuilder::visit(FloatLiteral& /*node*/) {}
void SymbolTableBuilder::visit(BoolLiteral& /*node*/) {}
void SymbolTableBuilder::visit(CharLiteral& /*node*/) {}
void SymbolTableBuilder::visit(StringLiteral& /*node*/) {}

void SymbolTableBuilder::visit(InterpolatedString& node) {
    for (auto& part : node.parts) {
        if (part.kind == InterpolatedPartKind::Expression && part.expression) {
            part.expression->accept(*this);
        }
    }
}

void SymbolTableBuilder::visit(NullLiteral& /*node*/) {}
void SymbolTableBuilder::visit(IdentifierExpression& /*node*/) {}
void SymbolTableBuilder::visit(QualifiedNameExpression& /*node*/) {}

void SymbolTableBuilder::visit(MemberAccessExpression& node) {
    if (node.object) node.object->accept(*this);
}

void SymbolTableBuilder::visit(ThisExpression& /*node*/) {}

void SymbolTableBuilder::visit(BinaryExpression& node) {
    if (node.left) node.left->accept(*this);
    if (node.right) node.right->accept(*this);
}

void SymbolTableBuilder::visit(UnaryExpression& node) {
    if (node.operand) node.operand->accept(*this);
}

void SymbolTableBuilder::visit(AssignmentExpression& node) {
    if (node.target) node.target->accept(*this);
    if (node.value) node.value->accept(*this);
}

void SymbolTableBuilder::visit(TernaryExpression& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.thenExpr) node.thenExpr->accept(*this);
    if (node.elseExpr) node.elseExpr->accept(*this);
}

void SymbolTableBuilder::visit(CallExpression& node) {
    if (node.callee) node.callee->accept(*this);
    for (auto& arg : node.arguments) {
        if (arg) arg->accept(*this);
    }
}

void SymbolTableBuilder::visit(NewExpression& node) {
    for (auto& arg : node.arguments) {
        if (arg) arg->accept(*this);
    }
    if (node.arraySize) node.arraySize->accept(*this);
}

void SymbolTableBuilder::visit(IndexExpression& node) {
    if (node.object) node.object->accept(*this);
    if (node.index) node.index->accept(*this);
}

void SymbolTableBuilder::visit(CastExpression& node) {
    if (node.operand) node.operand->accept(*this);
}

void SymbolTableBuilder::visit(SizeOfExpression& /*node*/) {}
void SymbolTableBuilder::visit(AlignOfExpression& /*node*/) {}

void SymbolTableBuilder::visit(PipeExpression& node) {
    if (node.input) node.input->accept(*this);
    for (auto& stage : node.stages) {
        for (auto& arg : stage.extraArguments) {
            if (arg) arg->accept(*this);
        }
    }
}

void SymbolTableBuilder::visit(MatchExpression& node) {
    if (node.subject) node.subject->accept(*this);
    for (auto& arm : node.arms) {
        pushScope(ScopeKind::Block);
        if (arm.pattern) arm.pattern->accept(*this);
        if (arm.body) arm.body->accept(*this);
        popScope();
    }
}

void SymbolTableBuilder::visit(TupleExpression& node) {
    for (auto& elem : node.elements) {
        if (elem) elem->accept(*this);
    }
}

void SymbolTableBuilder::visit(LambdaExpression& node) {
    // Lambdas create a function scope
    pushScope(ScopeKind::Function);

    for (auto& param : node.parameters) {
        if (param) {
            auto* paramSym = symbolTable_.createSymbol<VariableSymbol>(
                param->name, VariableRole::Parameter, param->location);
            defineSymbol(paramSym);
        }
    }

    if (node.body) node.body->accept(*this);

    popScope();
}

//================================================================================
// Patterns
//================================================================================
void SymbolTableBuilder::visit(LiteralPattern& /*node*/) {}
void SymbolTableBuilder::visit(RangePattern& /*node*/) {}
void SymbolTableBuilder::visit(WildcardPattern& /*node*/) {}

void SymbolTableBuilder::visit(BindingPattern& node) {
    // Binding patterns create a variable in the current scope
    auto* sym = symbolTable_.createSymbol<VariableSymbol>(
        node.name, VariableRole::Local, node.location);
    sym->isInferred = true;
    defineSymbol(sym);
}

void SymbolTableBuilder::visit(TuplePattern& node) {
    for (auto& elem : node.elements) {
        if (elem) elem->accept(*this);
    }
}

void SymbolTableBuilder::visit(GuardedPattern& node) {
    if (node.innerPattern) node.innerPattern->accept(*this);
    if (node.guard) node.guard->accept(*this);
}

} // namespace sema
} // namespace mingus
