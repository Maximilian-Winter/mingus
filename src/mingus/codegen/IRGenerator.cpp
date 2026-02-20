//================================================================================
// MINGUS v1 - LLVM IR Generator Implementation
//================================================================================

#include "mingus/codegen/IRGenerator.h"
#include "mingus/sema/Scope.h"

#pragma warning(push, 0)
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/raw_ostream.h>
#pragma warning(pop)

#include <cassert>
#include <functional>
#include <stdexcept>

namespace mingus {
namespace codegen {

using namespace mingus::ast;
using namespace mingus::sema;

//================================================================================
// Local helpers
//================================================================================
static bool isIntegerKind(const Type* t) {
    if (auto* p = t->as<PrimitiveType>()) {
        return p->kind == PrimitiveType::PrimitiveKind::Int ||
               p->kind == PrimitiveType::PrimitiveKind::Byte ||
               p->kind == PrimitiveType::PrimitiveKind::Char;
    }
    if (t->is<UserType>() && t->as<UserType>()->underlyingKind == Type::Kind::Enum) {
        auto* enumSym = static_cast<EnumSymbol*>(t->as<UserType>()->symbol);
        if (enumSym && enumSym->underlyingType)
            return isIntegerKind(enumSym->underlyingType.get());
        return true;  // default (int) is integer
    }
    if (t->is<EnumType>()) return true;
    return false;
}

static bool isFloatingKind(const Type* t) {
    if (auto* p = t->as<PrimitiveType>()) {
        return p->kind == PrimitiveType::PrimitiveKind::Double ||
               p->kind == PrimitiveType::PrimitiveKind::Float;
    }
    return false;
}

static bool isBoolKind(const Type* t) {
    if (auto* p = t->as<PrimitiveType>()) {
        return p->kind == PrimitiveType::PrimitiveKind::Bool;
    }
    return false;
}

static bool isStringKind(const Type* t) {
    return t->is<PrimitiveType>() &&
           t->as<PrimitiveType>()->kind == PrimitiveType::PrimitiveKind::String;
}

static bool isPointerKind(const Type* t) {
    return t->is<PointerType>() ||
           (t->is<PrimitiveType>() &&
            t->as<PrimitiveType>()->kind == PrimitiveType::PrimitiveKind::String);
}

static bool isUserStructKind(const Type* t) {
    if (auto* u = t->as<UserType>()) {
        return u->underlyingKind == Type::Kind::Struct ||
               u->underlyingKind == Type::Kind::Class;
    }
    return false;
}

//================================================================================
// Constructor
//================================================================================
IRGenerator::IRGenerator(SymbolTable& symbolTable, TypeRegistry& registry)
    : builder_(context_)
    , symbolTable_(symbolTable)
    , registry_(registry)
    , currentFunction_(nullptr)
    , currentThisPtr_(nullptr)
    , currentType_(nullptr)
    , lastValue_(nullptr)
    , loopExitBlock_(nullptr)
    , loopIterBlock_(nullptr)
    , lambdaCounter_(0)
    , currentScope_(nullptr)
{}

//================================================================================
// Entry point
//================================================================================
std::unique_ptr<llvm::Module> IRGenerator::generate(ProgramNode& program) {
    module_ = std::make_unique<llvm::Module>("mingus_module", context_);

    currentScope_ = symbolTable_.getGlobalScope();

    // Phase A: Forward declarations
    declareStructTypes();
    declareExternFunctions();
    declareFunctions();
    declareVtables();
    declareItables();

    // Phase B: Function bodies (visitor-based)
    program.accept(*this);

    return std::move(module_);
}

//================================================================================
// Scope navigation (same pattern as sema passes)
//================================================================================
void IRGenerator::enterNamedScope(Scope* scope) {
    childIndexStack_.push_back(0);
    currentScope_ = scope;
}

void IRGenerator::leaveNamedScope() {
    currentScope_ = currentScope_->parent;
    childIndexStack_.pop_back();
}

void IRGenerator::enterNextChildScope() {
    size_t idx = childIndexStack_.back();
    childIndexStack_.back() = idx + 1;
    childIndexStack_.push_back(0);
    currentScope_ = currentScope_->children[idx].get();
}

void IRGenerator::leaveChildScope() {
    currentScope_ = currentScope_->parent;
    childIndexStack_.pop_back();
}

//================================================================================
// Type Mapping
//================================================================================
llvm::Type* IRGenerator::mapType(const TypePtr<Type>& type) {
    if (!type) return llvm::Type::getVoidTy(context_);
    return mapType(type.get());
}

llvm::StructType* IRGenerator::getFatPtrType() {
    auto* ptrTy = llvm::PointerType::getUnqual(context_);
    return llvm::StructType::get(context_, { ptrTy, ptrTy });
}

llvm::Type* IRGenerator::mapType(const Type* type) {
    if (!type) return llvm::Type::getVoidTy(context_);

    // Check cache
    auto it = typeCache_.find(type);
    if (it != typeCache_.end()) return it->second;

    llvm::Type* result = nullptr;

    if (auto* prim = type->as<PrimitiveType>()) {
        switch (prim->kind) {
            case PrimitiveType::PrimitiveKind::Int:
                result = llvm::Type::getInt32Ty(context_);
                break;
            case PrimitiveType::PrimitiveKind::Double:
                result = llvm::Type::getDoubleTy(context_);
                break;
            case PrimitiveType::PrimitiveKind::Float:
                result = llvm::Type::getFloatTy(context_);
                break;
            case PrimitiveType::PrimitiveKind::Byte:
                result = llvm::Type::getInt8Ty(context_);
                break;
            case PrimitiveType::PrimitiveKind::Char:
                result = llvm::Type::getInt8Ty(context_);
                break;
            case PrimitiveType::PrimitiveKind::Bool:
                result = llvm::Type::getInt1Ty(context_);
                break;
            case PrimitiveType::PrimitiveKind::Void:
                result = llvm::Type::getVoidTy(context_);
                break;
            case PrimitiveType::PrimitiveKind::String:
                result = llvm::PointerType::getUnqual(context_);
                break;
        }
    }
    else if (auto* ptrType = type->as<PointerType>()) {
        // Interface pointer → fat pointer { ptr, ptr }
        if (auto* inner = ptrType->baseType->as<UserType>()) {
            if (inner->underlyingKind == Type::Kind::Interface) {
                result = getFatPtrType();
            } else {
                result = llvm::PointerType::getUnqual(context_);
            }
        } else {
            result = llvm::PointerType::getUnqual(context_);
        }
    }
    else if (auto* arr = type->as<ArrayType>()) {
        llvm::Type* elemTy = mapType(arr->elementType);
        if (arr->size > 0) {
            result = llvm::ArrayType::get(elemTy, arr->size);
        } else {
            result = llvm::PointerType::getUnqual(context_);
        }
    }
    else if (auto* tup = type->as<TupleType>()) {
        std::vector<llvm::Type*> elements;
        for (const auto& et : tup->elementTypes) {
            elements.push_back(mapType(et));
        }
        result = llvm::StructType::get(context_, elements);
    }
    else if (auto* user = type->as<UserType>()) {
        if (user->underlyingKind == Type::Kind::Enum) {
            auto* enumSym = static_cast<EnumSymbol*>(user->symbol);
            if (enumSym && enumSym->underlyingType) {
                result = mapType(enumSym->underlyingType);
            } else {
                result = llvm::Type::getInt32Ty(context_);
            }
        } else {
            auto sit = structTypeCache_.find(type);
            if (sit != structTypeCache_.end()) {
                result = sit->second;
            } else {
                result = llvm::StructType::create(context_, user->name);
                structTypeCache_[type] = llvm::cast<llvm::StructType>(result);
            }
        }
    }
    else if (auto* func = type->as<FunctionType>()) {
        // Function-typed values are fat pointers: { fnPtr, envPtr }
        result = getFatPtrType();
    }
    else if (type->is<ErrorType>() || type->is<NullType>()) {
        result = llvm::PointerType::getUnqual(context_);
    }

    if (!result) {
        result = llvm::Type::getInt32Ty(context_);
    }

    typeCache_[type] = result;
    return result;
}

llvm::StructType* IRGenerator::getStructType(const Type* type) {
    auto it = structTypeCache_.find(type);
    if (it != structTypeCache_.end()) return it->second;

    if (auto* user = type->as<UserType>()) {
        for (auto& [t, st] : structTypeCache_) {
            if (auto* u = t->as<UserType>()) {
                if (u->name == user->name) return st;
            }
        }
    }

    return nullptr;
}

//================================================================================
// Name Mangling
//================================================================================
std::string IRGenerator::opToString(OverloadableOp op) {
    switch (op) {
        case OverloadableOp::Plus:         return "add";
        case OverloadableOp::Minus:        return "sub";
        case OverloadableOp::Star:         return "mul";
        case OverloadableOp::Slash:        return "div";
        case OverloadableOp::Modulo:       return "mod";
        case OverloadableOp::Index:        return "index";
        case OverloadableOp::Equals:       return "eq";
        case OverloadableOp::NotEquals:    return "ne";
        case OverloadableOp::Less:         return "lt";
        case OverloadableOp::Greater:      return "gt";
        case OverloadableOp::LessEqual:    return "le";
        case OverloadableOp::GreaterEqual: return "ge";
    }
    return "unknown";
}

std::string IRGenerator::mangleName(Symbol* sym) {
    if (!sym) return "__unknown__";

    if (auto* func = sym->as<FunctionSymbol>()) {
        if (func->isExtern) return func->name;
    }

    if (auto* opSym = sym->as<OperatorSymbol>()) {
        auto* ownerType = opSym->ownerType;
        if (ownerType) {
            return ownerType->name + "_operator_" + opToString(opSym->op);
        }
        return "operator_" + opToString(opSym->op);
    }

    if (sym->kind == SymbolKind::Constructor) {
        if (sym->owner) return sym->owner->name + "_constructor";
        return "constructor";
    }

    if (sym->kind == SymbolKind::Destructor) {
        if (sym->owner) return sym->owner->name + "_destructor";
        return "destructor";
    }

    if (auto* func = sym->as<FunctionSymbol>()) {
        if (func->isMethod && sym->owner) {
            return sym->owner->name + "_" + sym->name;
        }
        if (sym->owner && sym->owner->is<ModuleSymbol>()) {
            return sym->owner->name + "_" + sym->name;
        }
    }

    return sym->name;
}

//================================================================================
// Utility: Entry-block alloca
//================================================================================
llvm::AllocaInst* IRGenerator::createEntryBlockAlloca(
    llvm::Function* fn, llvm::Type* type, const std::string& name) {
    llvm::IRBuilder<> tmpBuilder(&fn->getEntryBlock(),
                                  fn->getEntryBlock().begin());
    return tmpBuilder.CreateAlloca(type, nullptr, name);
}

//================================================================================
// Forward Declarations — Phase A
//================================================================================

void IRGenerator::declareStructTypes() {
    auto* globalScope = symbolTable_.getGlobalScope();

    for (auto& [name, sym] : globalScope->getSymbolMap()) {
        auto* moduleSym = sym->as<ModuleSymbol>();
        if (!moduleSym || !moduleSym->moduleScope) continue;

        for (auto& [declName, declSym] : moduleSym->moduleScope->getSymbolMap()) {
            if (auto* structSym = declSym->as<StructSymbol>()) {
                declareStructTypeForSymbol(structSym);
            } else if (auto* classSym = declSym->as<ClassSymbol>()) {
                declareClassTypeForSymbol(classSym);
            }
        }
    }

    for (auto& [type, structTy] : structTypeCache_) {
        if (structTy->isOpaque()) {
            auto* user = type->as<UserType>();
            if (!user) continue;

            auto* typeSym = static_cast<TypeSymbol*>(user->symbol);
            std::vector<llvm::Type*> fieldTypes;

            if (auto* ss = typeSym->as<StructSymbol>()) {
                for (auto* field : ss->fields) {
                    fieldTypes.push_back(mapType(field->type));
                }
            } else if (auto* cs = typeSym->as<ClassSymbol>()) {
                // Prepend vtable pointer if class has virtual methods
                if (cs->vtableSize > 0) {
                    fieldTypes.push_back(llvm::PointerType::getUnqual(context_));
                }
                // Use allFields (inherited + own) for layout
                for (auto* field : cs->allFields) {
                    fieldTypes.push_back(mapType(field->type));
                }
            }

            if (!fieldTypes.empty()) {
                structTy->setBody(fieldTypes);
            } else {
                structTy->setBody(llvm::Type::getInt8Ty(context_));
            }
        }
    }
}

void IRGenerator::declareStructTypeForSymbol(StructSymbol* sym) {
    auto userType = registry_.getUserType(sym->name, Type::Kind::Struct, sym);
    auto it = structTypeCache_.find(userType.get());
    if (it != structTypeCache_.end()) return;

    auto* structTy = llvm::StructType::create(context_, sym->name);
    structTypeCache_[userType.get()] = structTy;
    typeCache_[userType.get()] = structTy;
}

void IRGenerator::declareClassTypeForSymbol(ClassSymbol* sym) {
    auto userType = registry_.getUserType(sym->name, Type::Kind::Class, sym);
    auto it = structTypeCache_.find(userType.get());
    if (it != structTypeCache_.end()) return;

    // Also ensure base class struct types are declared first
    if (sym->baseClass) {
        declareClassTypeForSymbol(sym->baseClass);
    }

    auto* structTy = llvm::StructType::create(context_, sym->name);
    structTypeCache_[userType.get()] = structTy;
    typeCache_[userType.get()] = structTy;
}

void IRGenerator::declareVtables() {
    auto* globalScope = symbolTable_.getGlobalScope();

    for (auto& [name, sym] : globalScope->getSymbolMap()) {
        auto* moduleSym = sym->as<ModuleSymbol>();
        if (!moduleSym || !moduleSym->moduleScope) continue;

        for (auto& [declName, declSym] : moduleSym->moduleScope->getSymbolMap()) {
            auto* classSym = declSym->as<ClassSymbol>();
            if (!classSym || classSym->vtableSize <= 0) continue;
            if (classSym->isAbstract) continue;  // Abstract classes don't need vtable globals

            // Build array of function pointers
            auto* ptrTy = llvm::PointerType::getUnqual(context_);
            auto* vtableArrayTy = llvm::ArrayType::get(ptrTy, classSym->vtableSize);

            std::vector<llvm::Constant*> vtableEntries;
            for (auto* methodSym : classSym->vtable) {
                if (methodSym && !methodSym->isAbstract) {
                    auto it = functionCache_.find(methodSym);
                    if (it != functionCache_.end()) {
                        vtableEntries.push_back(it->second);
                    } else {
                        vtableEntries.push_back(llvm::ConstantPointerNull::get(ptrTy));
                    }
                } else {
                    vtableEntries.push_back(llvm::ConstantPointerNull::get(ptrTy));
                }
            }

            auto* vtableInit = llvm::ConstantArray::get(vtableArrayTy, vtableEntries);
            auto* vtableGlobal = new llvm::GlobalVariable(
                *module_, vtableArrayTy, true,
                llvm::GlobalValue::InternalLinkage,
                vtableInit, classSym->name + "_vtable");
            vtableCache_[classSym] = vtableGlobal;
        }
    }
}

void IRGenerator::declareItables() {
    auto* globalScope = symbolTable_.getGlobalScope();
    auto* ptrTy = llvm::PointerType::getUnqual(context_);

    for (auto& [name, sym] : globalScope->getSymbolMap()) {
        auto* moduleSym = sym->as<ModuleSymbol>();
        if (!moduleSym || !moduleSym->moduleScope) continue;

        for (auto& [declName, declSym] : moduleSym->moduleScope->getSymbolMap()) {
            auto* classSym = declSym->as<ClassSymbol>();
            if (!classSym || classSym->isAbstract) continue;

            for (auto* ifaceSym : classSym->implementedInterfaces) {
                std::vector<llvm::Constant*> slots;
                for (auto* ifaceMethod : ifaceSym->methods) {
                    auto* implMethod = classSym->findMethod(ifaceMethod->name);
                    if (implMethod) {
                        auto it = functionCache_.find(implMethod);
                        if (it != functionCache_.end()) {
                            slots.push_back(it->second);
                        } else {
                            slots.push_back(llvm::ConstantPointerNull::get(ptrTy));
                        }
                    } else {
                        slots.push_back(llvm::ConstantPointerNull::get(ptrTy));
                    }
                }

                auto* arrTy = llvm::ArrayType::get(ptrTy, slots.size());
                auto* init = llvm::ConstantArray::get(arrTy, slots);
                auto* global = new llvm::GlobalVariable(*module_, arrTy, true,
                    llvm::GlobalValue::InternalLinkage,
                    init,
                    classSym->name + "." + ifaceSym->name + ".itable");
                itableCache_[{classSym, ifaceSym}] = global;
            }
        }
    }
}

llvm::Value* IRGenerator::emitWrapToInterfacePtr(llvm::Value* objPtr,
                                                   ClassSymbol* cls,
                                                   InterfaceSymbol* iface) {
    auto it = itableCache_.find({cls, iface});
    if (it == itableCache_.end()) return objPtr;  // fallback (shouldn't happen)

    auto* fatTy = getFatPtrType();
    llvm::Value* fat = llvm::UndefValue::get(fatTy);
    fat = builder_.CreateInsertValue(fat, objPtr, {0}, "fat.obj");
    fat = builder_.CreateInsertValue(fat, it->second, {1}, "fat.itable");
    return fat;
}

int IRGenerator::getFieldGEPIndex(ClassSymbol* cls, VariableSymbol* field) {
    int offset = cls->vtableSize > 0 ? 1 : 0;
    for (size_t i = 0; i < cls->allFields.size(); ++i) {
        if (cls->allFields[i] == field) {
            return offset + static_cast<int>(i);
        }
    }
    return -1;
}

void IRGenerator::storeVtablePtr(llvm::Value* objPtr, ClassSymbol* cls) {
    if (cls->vtableSize <= 0) return;
    auto it = vtableCache_.find(cls);
    if (it == vtableCache_.end()) return;

    auto userType = registry_.getUserType(cls->name, Type::Kind::Class, cls);
    auto* structTy = getStructType(userType.get());
    if (!structTy) return;

    auto* vtableSlot = builder_.CreateStructGEP(structTy, objPtr, 0, "vtable.slot");
    builder_.CreateStore(it->second, vtableSlot);
}

void IRGenerator::declareExternFunctions() {
    auto* globalScope = symbolTable_.getGlobalScope();

    for (auto& [name, sym] : globalScope->getSymbolMap()) {
        auto* moduleSym = sym->as<ModuleSymbol>();
        if (!moduleSym || !moduleSym->moduleScope) continue;

        for (auto& [declName, declSym] : moduleSym->moduleScope->getSymbolMap()) {
            auto* funcSym = declSym->as<FunctionSymbol>();
            if (!funcSym || !funcSym->isExtern) continue;
            if (functionCache_.count(funcSym)) continue;

            auto* fnTy = buildFunctionType(funcSym);
            bool isVarArg = (funcSym->name == "printf" || funcSym->name == "snprintf");
            if (isVarArg) {
                std::vector<llvm::Type*> paramTypes;
                for (auto* param : funcSym->parameters) {
                    paramTypes.push_back(mapType(param->type));
                }
                fnTy = llvm::FunctionType::get(mapType(funcSym->returnType),
                                                paramTypes, true);
            }

            auto* fn = llvm::Function::Create(fnTy,
                llvm::Function::ExternalLinkage, funcSym->name, module_.get());
            functionCache_[funcSym] = fn;
        }
    }
}

void IRGenerator::declareFunctions() {
    auto* globalScope = symbolTable_.getGlobalScope();

    for (auto& [name, sym] : globalScope->getSymbolMap()) {
        auto* moduleSym = sym->as<ModuleSymbol>();
        if (!moduleSym || !moduleSym->moduleScope) continue;

        for (auto& [declName, declSym] : moduleSym->moduleScope->getSymbolMap()) {
            if (auto* funcSym = declSym->as<FunctionSymbol>()) {
                if (!funcSym->isExtern) {
                    declareFunctionSymbol(funcSym);
                }
            }

            if (auto* typeSym = declSym->as<TypeSymbol>()) {
                if (!typeSym->memberScope) continue;

                for (auto& [mName, mSym] : typeSym->memberScope->getSymbolMap()) {
                    if (auto* methodSym = mSym->as<FunctionSymbol>()) {
                        // Skip extern and abstract (no body) methods
                        if (!methodSym->isExtern && !methodSym->isAbstract) {
                            declareFunctionSymbol(methodSym);
                        }
                    }
                }

                if (auto* classSym = typeSym->as<ClassSymbol>()) {
                    if (classSym->constructor) {
                        declareFunctionSymbol(classSym->constructor);
                    }
                    if (classSym->destructor) {
                        declareFunctionSymbol(classSym->destructor);
                    }
                }

                for (auto* opSym : typeSym->memberScope->getOperatorList()) {
                    declareOperatorSymbol(opSym);
                }
            }
        }
    }
}

void IRGenerator::declareFunctionSymbol(FunctionSymbol* sym) {
    if (functionCache_.count(sym)) return;

    auto* fnTy = buildFunctionType(sym);
    std::string mangledName = mangleName(sym);

    auto* fn = llvm::Function::Create(fnTy,
        llvm::Function::ExternalLinkage, mangledName, module_.get());

    unsigned idx = 0;
    if (sym->hasThisParam) {
        fn->getArg(idx)->setName("this");
        idx++;
    }
    for (auto* param : sym->parameters) {
        if (idx < fn->arg_size()) {
            fn->getArg(idx)->setName(param->name);
            idx++;
        }
    }

    functionCache_[sym] = fn;
}

void IRGenerator::declareOperatorSymbol(OperatorSymbol* sym) {
    if (functionCache_.count(sym)) return;

    auto* fnTy = buildOperatorType(sym);
    std::string mangledName = mangleName(sym);

    auto* fn = llvm::Function::Create(fnTy,
        llvm::Function::ExternalLinkage, mangledName, module_.get());

    unsigned idx = 0;
    fn->getArg(idx++)->setName("this");
    for (auto* param : sym->parameters) {
        if (idx < fn->arg_size()) {
            fn->getArg(idx)->setName(param->name);
            idx++;
        }
    }

    functionCache_[sym] = fn;
}

llvm::FunctionType* IRGenerator::buildFunctionType(FunctionSymbol* sym) {
    // Constructors and destructors always return void
    llvm::Type* retTy;
    if (sym->kind == SymbolKind::Constructor || sym->kind == SymbolKind::Destructor) {
        retTy = llvm::Type::getVoidTy(context_);
    } else {
        retTy = mapType(sym->returnType);
    }

    std::vector<llvm::Type*> paramTypes;

    if (sym->hasThisParam && sym->owner) {
        paramTypes.push_back(llvm::PointerType::getUnqual(context_));
    }

    for (auto* param : sym->parameters) {
        llvm::Type* paramTy = mapType(param->type);
        if (param->type && isUserStructKind(param->type.get())) {
            paramTy = llvm::PointerType::getUnqual(context_);
        }
        paramTypes.push_back(paramTy);
    }

    return llvm::FunctionType::get(retTy, paramTypes, false);
}

llvm::FunctionType* IRGenerator::buildOperatorType(OperatorSymbol* sym) {
    llvm::Type* retTy = mapType(sym->returnType);
    std::vector<llvm::Type*> paramTypes;

    paramTypes.push_back(llvm::PointerType::getUnqual(context_));

    for (auto* param : sym->parameters) {
        llvm::Type* paramTy = mapType(param->type);
        if (param->type && isUserStructKind(param->type.get())) {
            paramTy = llvm::PointerType::getUnqual(context_);
        }
        paramTypes.push_back(paramTy);
    }

    return llvm::FunctionType::get(retTy, paramTypes, false);
}

//================================================================================
// LValue emission — returns a pointer, does NOT load
//================================================================================
llvm::Value* IRGenerator::emitLValue(ExpressionNode& expr) {
    // IdentifierExpression → alloca from namedValues_
    if (auto* ident = expr.as<IdentifierExpression>()) {
        if (ident->resolvedSymbol) {
            auto it = namedValues_.find(ident->resolvedSymbol);
            if (it != namedValues_.end()) return it->second;
        }
        return nullptr;
    }

    // MemberAccessExpression → GEP to field
    if (auto* mem = expr.as<MemberAccessExpression>()) {
        // Get the object pointer
        llvm::Value* objPtr = nullptr;
        if (mem->isArrow) {
            // Arrow: load pointer, then GEP
            mem->object->accept(*this);
            objPtr = lastValue_;
        } else {
            // Dot: get lvalue of object (its alloca)
            objPtr = emitLValue(*mem->object);
            if (!objPtr) {
                mem->object->accept(*this);
                objPtr = lastValue_;
                // If result is a struct value, store in temp alloca for GEP
                if (objPtr && objPtr->getType()->isStructTy()) {
                    auto* tmp = createEntryBlockAlloca(currentFunction_,
                        objPtr->getType(), "member.tmp");
                    builder_.CreateStore(objPtr, tmp);
                    objPtr = tmp;
                }
            }
        }
        if (!objPtr) return nullptr;

        // Find the struct type and field index
        const Type* objType = mem->object->resolvedType.get();
        // Auto-dereference pointer types
        if (auto* ptrType = objType->as<PointerType>()) {
            objType = ptrType->baseType.get();
        }
        auto* structTy = getStructType(objType);
        if (!structTy) return nullptr;

        // Find field index
        if (auto* userType = objType->as<UserType>()) {
            auto* typeSym = static_cast<TypeSymbol*>(userType->symbol);
            auto* fieldSym = typeSym->findField(mem->memberName);
            if (fieldSym) {
                // For classes, account for vtable pointer and inherited fields
                if (auto* classSym = typeSym->as<ClassSymbol>()) {
                    int gepIdx = getFieldGEPIndex(classSym, fieldSym);
                    if (gepIdx >= 0) {
                        return builder_.CreateStructGEP(structTy, objPtr, gepIdx,
                                                        mem->memberName + "_ptr");
                    }
                } else {
                    // Structs: direct field index
                    return builder_.CreateStructGEP(structTy, objPtr, fieldSym->fieldIndex,
                                                    mem->memberName + "_ptr");
                }
            }
        }
        return nullptr;
    }

    // UnaryExpression(Dereference) → emit operand, return pointer value
    if (auto* unary = expr.as<UnaryExpression>()) {
        if (unary->op == UnaryOp::Dereference) {
            unary->operand->accept(*this);
            return lastValue_;
        }
    }

    // IndexExpression → GEP to element
    if (auto* idx = expr.as<IndexExpression>()) {
        if (!idx->isOperatorOverload) {
            llvm::Value* arrPtr = emitLValue(*idx->object);
            if (!arrPtr) {
                idx->object->accept(*this);
                arrPtr = lastValue_;
            }
            idx->index->accept(*this);
            llvm::Value* indexVal = lastValue_;

            if (!arrPtr || !indexVal) return nullptr;

            const Type* objType = idx->object->resolvedType.get();
            if (auto* arrType = objType->as<ArrayType>()) {
                llvm::Type* elemTy = mapType(arrType->elementType);
                if (arrType->size > 0) {
                    // Sized array: GEP with two indices [0, idx]
                    auto* llvmArrTy = llvm::ArrayType::get(elemTy, arrType->size);
                    return builder_.CreateGEP(llvmArrTy, arrPtr,
                        { llvm::ConstantInt::get(llvm::Type::getInt32Ty(context_), 0), indexVal },
                        "elem_ptr");
                } else {
                    // Unsized array (pointer): GEP with one index
                    return builder_.CreateGEP(elemTy, arrPtr, indexVal, "elem_ptr");
                }
            }
            if (isPointerKind(objType)) {
                // Pointer indexing
                const Type* baseType = nullptr;
                if (auto* ptrTy = objType->as<PointerType>()) {
                    baseType = ptrTy->baseType.get();
                }
                llvm::Type* elemTy = baseType ? mapType(baseType)
                                              : llvm::Type::getInt8Ty(context_);
                return builder_.CreateGEP(elemTy, arrPtr, indexVal, "elem_ptr");
            }
        }
    }

    return nullptr;
}

//================================================================================
// RAII helpers
//================================================================================
void IRGenerator::pushRAIIScope() {
    raiiScopeStack_.push_back({});
}

void IRGenerator::popRAIIScope() {
    if (!raiiScopeStack_.empty()) {
        raiiScopeStack_.pop_back();
    }
}

void IRGenerator::registerRAII(llvm::Value* ptr, llvm::Function* dtor) {
    if (!raiiScopeStack_.empty()) {
        raiiScopeStack_.back().destructibles.push_back({ptr, dtor});
    }
}

void IRGenerator::emitScopeDestructors() {
    if (raiiScopeStack_.empty()) return;
    auto& scope = raiiScopeStack_.back();
    // LIFO order — reverse iterate
    for (auto it = scope.destructibles.rbegin(); it != scope.destructibles.rend(); ++it) {
        // Skip variables that are being returned
        bool skip = false;
        for (auto& [sym, val] : namedValues_) {
            if (val == it->first && scope.returnedVars.count(sym)) {
                skip = true;
                break;
            }
        }
        if (!skip && it->second) {
            builder_.CreateCall(it->second, {it->first});
        }
    }
}

void IRGenerator::emitReturnDestructors() {
    // Emit destructors for ALL active scopes, innermost to outermost, LIFO
    for (auto scopeIt = raiiScopeStack_.rbegin();
         scopeIt != raiiScopeStack_.rend(); ++scopeIt) {
        for (auto it = scopeIt->destructibles.rbegin();
             it != scopeIt->destructibles.rend(); ++it) {
            bool skip = false;
            for (auto& [sym, val] : namedValues_) {
                if (val == it->first && scopeIt->returnedVars.count(sym)) {
                    skip = true;
                    break;
                }
            }
            if (!skip && it->second) {
                builder_.CreateCall(it->second, {it->first});
            }
        }
    }
}

void IRGenerator::emitBreakDestructors() {
    // Emit destructors for scopes created inside the loop body only.
    // Walk from innermost scope down to loopRAIIScopeDepth_ (exclusive),
    // which is the stack depth recorded when the loop was entered.
    for (size_t i = raiiScopeStack_.size(); i > loopRAIIScopeDepth_; --i) {
        auto& scope = raiiScopeStack_[i - 1];
        for (auto it = scope.destructibles.rbegin();
             it != scope.destructibles.rend(); ++it) {
            if (it->second) {
                builder_.CreateCall(it->second, {it->first});
            }
        }
    }
}

//================================================================================
// Closure reference counting helpers
//================================================================================

llvm::Function* IRGenerator::getOrCreateClosureRetainFn() {
    if (closureRetainFn_) return closureRetainFn_;

    auto* ptrTy = llvm::PointerType::getUnqual(context_);
    auto* voidTy = llvm::Type::getVoidTy(context_);
    auto* i64Ty = llvm::Type::getInt64Ty(context_);

    auto* fnTy = llvm::FunctionType::get(voidTy, {ptrTy}, false);
    closureRetainFn_ = llvm::Function::Create(
        fnTy, llvm::Function::InternalLinkage,
        "__mingus_closure_retain", module_.get());

    auto* entry = llvm::BasicBlock::Create(context_, "entry", closureRetainFn_);
    auto* doRetain = llvm::BasicBlock::Create(context_, "do_retain", closureRetainFn_);
    auto* done = llvm::BasicBlock::Create(context_, "done", closureRetainFn_);

    llvm::IRBuilder<> b(entry);
    auto* env = closureRetainFn_->getArg(0);
    auto* isNull = b.CreateICmpEQ(env,
        llvm::ConstantPointerNull::get(ptrTy), "is_null");
    b.CreateCondBr(isNull, done, doRetain);

    b.SetInsertPoint(doRetain);
    auto* headerTy = llvm::StructType::get(context_, {i64Ty, ptrTy});
    auto* rcPtr = b.CreateStructGEP(headerTy, env, 0, "rc_ptr");
    auto* rc = b.CreateLoad(i64Ty, rcPtr, "rc");
    auto* rcInc = b.CreateAdd(rc, llvm::ConstantInt::get(i64Ty, 1), "rc_inc");
    b.CreateStore(rcInc, rcPtr);
    b.CreateBr(done);

    b.SetInsertPoint(done);
    b.CreateRetVoid();

    return closureRetainFn_;
}

llvm::Function* IRGenerator::getOrCreateClosureReleaseFn() {
    if (closureReleaseFn_) return closureReleaseFn_;

    auto* ptrTy = llvm::PointerType::getUnqual(context_);
    auto* voidTy = llvm::Type::getVoidTy(context_);
    auto* i64Ty = llvm::Type::getInt64Ty(context_);

    auto* fnTy = llvm::FunctionType::get(voidTy, {ptrTy}, false);
    closureReleaseFn_ = llvm::Function::Create(
        fnTy, llvm::Function::InternalLinkage,
        "__mingus_closure_release", module_.get());

    auto* entry = llvm::BasicBlock::Create(context_, "entry", closureReleaseFn_);
    auto* doRelease = llvm::BasicBlock::Create(context_, "do_release", closureReleaseFn_);
    auto* cleanup = llvm::BasicBlock::Create(context_, "cleanup", closureReleaseFn_);
    auto* callCleanup = llvm::BasicBlock::Create(context_, "call_cleanup", closureReleaseFn_);
    auto* doFree = llvm::BasicBlock::Create(context_, "do_free", closureReleaseFn_);
    auto* done = llvm::BasicBlock::Create(context_, "done", closureReleaseFn_);

    llvm::IRBuilder<> b(entry);
    auto* env = closureReleaseFn_->getArg(0);
    auto* isNull = b.CreateICmpEQ(env,
        llvm::ConstantPointerNull::get(ptrTy), "is_null");
    b.CreateCondBr(isNull, done, doRelease);

    b.SetInsertPoint(doRelease);
    auto* headerTy = llvm::StructType::get(context_, {i64Ty, ptrTy});
    auto* rcPtr = b.CreateStructGEP(headerTy, env, 0, "rc_ptr");
    auto* rc = b.CreateLoad(i64Ty, rcPtr, "rc");
    auto* rcDec = b.CreateSub(rc, llvm::ConstantInt::get(i64Ty, 1), "rc_dec");
    b.CreateStore(rcDec, rcPtr);
    auto* isZero = b.CreateICmpEQ(rcDec,
        llvm::ConstantInt::get(i64Ty, 0), "is_zero");
    b.CreateCondBr(isZero, cleanup, done);

    b.SetInsertPoint(cleanup);
    auto* cleanupFnPtr = b.CreateStructGEP(headerTy, env, 1, "cleanup_fn_ptr");
    auto* cleanupFn = b.CreateLoad(ptrTy, cleanupFnPtr, "cleanup_fn");
    auto* hasCleanup = b.CreateICmpNE(cleanupFn,
        llvm::ConstantPointerNull::get(ptrTy), "has_cleanup");
    b.CreateCondBr(hasCleanup, callCleanup, doFree);

    b.SetInsertPoint(callCleanup);
    auto* cleanupCallTy = llvm::FunctionType::get(voidTy, {ptrTy}, false);
    b.CreateCall(cleanupCallTy, cleanupFn, {env});
    b.CreateBr(doFree);

    b.SetInsertPoint(doFree);
    auto freeCallee = module_->getOrInsertFunction("free",
        llvm::FunctionType::get(voidTy, {ptrTy}, false));
    b.CreateCall(freeCallee, {env});
    b.CreateBr(done);

    b.SetInsertPoint(done);
    b.CreateRetVoid();

    return closureReleaseFn_;
}

llvm::Function* IRGenerator::generateClosureCleanupFn(
    llvm::StructType* closureTy,
    const std::vector<sema::Symbol*>& capturedVars,
    int headerOffset)
{
    auto* ptrTy = llvm::PointerType::getUnqual(context_);
    auto* voidTy = llvm::Type::getVoidTy(context_);

    auto* fnTy = llvm::FunctionType::get(voidTy, {ptrTy}, false);
    std::string name = "__closure_cleanup_" + std::to_string(closureCleanupCounter_++);
    auto* fn = llvm::Function::Create(fnTy, llvm::Function::InternalLinkage,
                                       name, module_.get());

    auto* entry = llvm::BasicBlock::Create(context_, "entry", fn);
    llvm::IRBuilder<> b(entry);
    auto* env = fn->getArg(0);

    auto* fatPtrTy = getFatPtrType();

    for (size_t i = 0; i < capturedVars.size(); i++) {
        auto* capSym = capturedVars[i];
        auto* varSym = capSym->as<sema::VariableSymbol>();
        if (!varSym) continue;

        // Check if this captured variable is a closure (FunctionType)
        if (varSym->type && varSym->type->is<FunctionType>()) {
            unsigned fieldIdx = headerOffset + (unsigned)i;
            auto* fieldPtr = b.CreateStructGEP(closureTy, env, fieldIdx,
                                                capSym->name + ".cleanup.slot");
            // Load the captured fat pointer { fnPtr, envPtr }
            auto* fatVal = b.CreateLoad(fatPtrTy, fieldPtr, capSym->name + ".fat");
            // Extract envPtr (index 1)
            auto* envPtr = b.CreateExtractValue(fatVal, {1}, capSym->name + ".env");
            // Release the inner closure
            b.CreateCall(getOrCreateClosureReleaseFn(), {envPtr});
        }
    }

    b.CreateRetVoid();
    return fn;
}

llvm::Function* IRGenerator::getOrCreateClosureReleaseWrapper() {
    if (closureReleaseWrapperFn_) return closureReleaseWrapperFn_;

    auto* ptrTy = llvm::PointerType::getUnqual(context_);
    auto* voidTy = llvm::Type::getVoidTy(context_);

    // Takes ptr to the fat pointer alloca
    auto* fnTy = llvm::FunctionType::get(voidTy, {ptrTy}, false);
    closureReleaseWrapperFn_ = llvm::Function::Create(
        fnTy, llvm::Function::InternalLinkage,
        "__mingus_closure_release_wrapper", module_.get());

    auto* entry = llvm::BasicBlock::Create(context_, "entry", closureReleaseWrapperFn_);
    llvm::IRBuilder<> b(entry);
    auto* allocaPtr = closureReleaseWrapperFn_->getArg(0);

    // Load the fat pointer { fnPtr, envPtr } from the alloca
    auto* fatPtrTy = getFatPtrType();
    auto* fatVal = b.CreateLoad(fatPtrTy, allocaPtr, "fat");

    // Extract envPtr (index 1)
    auto* envPtr = b.CreateExtractValue(fatVal, {1}, "env.ptr");

    // Call __mingus_closure_release(envPtr)
    b.CreateCall(getOrCreateClosureReleaseFn(), {envPtr});

    b.CreateRetVoid();
    return closureReleaseWrapperFn_;
}

//================================================================================
// Struct cleanup for closure-typed fields
//================================================================================
llvm::Function* IRGenerator::getOrCreateStructCleanupFn(StructSymbol* structSym) {
    auto it = structCleanupCache_.find(structSym->name);
    if (it != structCleanupCache_.end()) return it->second;

    auto* ptrTy = llvm::PointerType::getUnqual(context_);
    auto* voidTy = llvm::Type::getVoidTy(context_);
    auto* fnTy = llvm::FunctionType::get(voidTy, {ptrTy}, false);

    std::string name = "__struct_cleanup_" + structSym->name;
    auto* fn = llvm::Function::Create(fnTy, llvm::Function::InternalLinkage,
                                       name, module_.get());

    auto* entry = llvm::BasicBlock::Create(context_, "entry", fn);
    llvm::IRBuilder<> b(entry);
    auto* structPtr = fn->getArg(0);

    auto userType = registry_.getUserType(structSym->name, Type::Kind::Struct, structSym);
    auto* structTy = getStructType(userType.get());
    auto* fatPtrTy = getFatPtrType();

    for (auto* field : structSym->fields) {
        if (field->type && field->type->is<FunctionType>()) {
            auto* fieldPtr = b.CreateStructGEP(structTy, structPtr,
                                                field->fieldIndex,
                                                field->name + ".cleanup");
            auto* fatVal = b.CreateLoad(fatPtrTy, fieldPtr, field->name + ".fat");
            auto* envPtr = b.CreateExtractValue(fatVal, {1}, field->name + ".env");
            b.CreateCall(getOrCreateClosureReleaseFn(), {envPtr});
        }
    }

    b.CreateRetVoid();
    structCleanupCache_[structSym->name] = fn;
    return fn;
}

//================================================================================
// String helpers
//================================================================================
llvm::Function* IRGenerator::getOrCreateStringFreeFn() {
    if (stringFreeFn_) return stringFreeFn_;
    auto* fnTy = llvm::FunctionType::get(
        llvm::Type::getVoidTy(context_),
        {llvm::PointerType::getUnqual(context_)}, false);
    stringFreeFn_ = llvm::Function::Create(
        fnTy, llvm::Function::InternalLinkage,
        "__mingus_string_free", module_.get());
    auto* entry = llvm::BasicBlock::Create(context_, "entry", stringFreeFn_);
    llvm::IRBuilder<> b(entry);
    auto freeCallee = module_->getOrInsertFunction("free",
        llvm::FunctionType::get(llvm::Type::getVoidTy(context_),
            {llvm::PointerType::getUnqual(context_)}, false));
    b.CreateCall(freeCallee, {stringFreeFn_->getArg(0)});
    b.CreateRetVoid();
    return stringFreeFn_;
}

llvm::Value* IRGenerator::emitStringConcat(llvm::Value* left, llvm::Value* right) {
    auto ptrTy = llvm::PointerType::getUnqual(context_);
    auto i64Ty = llvm::Type::getInt64Ty(context_);

    // Declare strlen
    auto strlenCallee = module_->getOrInsertFunction("strlen",
        llvm::FunctionType::get(i64Ty, {ptrTy}, false));
    // Declare strcpy
    auto strcpyCallee = module_->getOrInsertFunction("strcpy",
        llvm::FunctionType::get(ptrTy, {ptrTy, ptrTy}, false));
    // Declare strcat
    auto strcatCallee = module_->getOrInsertFunction("strcat",
        llvm::FunctionType::get(ptrTy, {ptrTy, ptrTy}, false));
    // Declare malloc
    auto mallocCallee = module_->getOrInsertFunction("malloc",
        llvm::FunctionType::get(ptrTy, {i64Ty}, false));

    // len1 = strlen(left)
    llvm::Value* len1 = builder_.CreateCall(strlenCallee, {left}, "len1");
    // len2 = strlen(right)
    llvm::Value* len2 = builder_.CreateCall(strlenCallee, {right}, "len2");
    // total = len1 + len2 + 1
    llvm::Value* sum = builder_.CreateAdd(len1, len2, "sum");
    llvm::Value* total = builder_.CreateAdd(sum,
        llvm::ConstantInt::get(i64Ty, 1), "total");
    // buf = malloc(total)
    llvm::Value* buf = builder_.CreateCall(mallocCallee, {total}, "str.buf");
    // strcpy(buf, left)
    builder_.CreateCall(strcpyCallee, {buf, left});
    // strcat(buf, right)
    builder_.CreateCall(strcatCallee, {buf, right});
    // Register for RAII cleanup
    registerRAII(buf, getOrCreateStringFreeFn());

    return buf;
}

//================================================================================
// Program structure visitors
//================================================================================
void IRGenerator::visit(ProgramNode& node) {
    for (auto& mod : node.modules) {
        mod->accept(*this);
    }
}

void IRGenerator::visit(ModuleNode& node) {
    currentModuleName_ = node.name;

    // Enter module scope
    auto* modSym = currentScope_->lookupLocal(node.name);
    auto* moduleSym = modSym ? modSym->as<ModuleSymbol>() : nullptr;
    if (moduleSym && moduleSym->moduleScope) {
        enterNamedScope(moduleSym->moduleScope);
    }

    for (auto& decl : node.declarations) {
        decl->accept(*this);
    }

    if (moduleSym && moduleSym->moduleScope) {
        leaveNamedScope();
    }

    currentModuleName_.clear();
}

void IRGenerator::visit(ImportNode& /*node*/) {}

//================================================================================
// Type node visitors (no-op in codegen)
//================================================================================
void IRGenerator::visit(TypeNode& /*node*/) {}
void IRGenerator::visit(PrimitiveTypeNode& /*node*/) {}
void IRGenerator::visit(NamedTypeNode& /*node*/) {}
void IRGenerator::visit(PointerTypeNode& /*node*/) {}
void IRGenerator::visit(ArrayTypeNode& /*node*/) {}
void IRGenerator::visit(TupleTypeNode& /*node*/) {}
void IRGenerator::visit(FunctionTypeNode& /*node*/) {}

//================================================================================
// Declaration visitors
//================================================================================

void IRGenerator::visit(FunctionDeclaration& node) {
    // Look up the FunctionSymbol in current scope
    auto* sym = currentScope_->lookupLocal(node.name);
    auto* funcSym = sym ? sym->as<FunctionSymbol>() : nullptr;
    if (!funcSym) return;

    auto it = functionCache_.find(funcSym);
    if (it == functionCache_.end()) return;

    llvm::Function* fn = it->second;
    if (!node.body) return;  // Forward declaration only

    // Create entry block
    auto* entryBB = llvm::BasicBlock::Create(context_, "entry", fn);
    builder_.SetInsertPoint(entryBB);

    auto* prevFunction = currentFunction_;
    auto* prevThisPtr = currentThisPtr_;
    currentFunction_ = fn;

    // Save and clear namedValues for this function scope
    auto savedNamedValues = namedValues_;
    namedValues_.clear();

    // Handle parameters
    unsigned argIdx = 0;

    // Implicit 'this' parameter for methods
    if (funcSym->hasThisParam) {
        currentThisPtr_ = fn->getArg(argIdx++);
    } else {
        currentThisPtr_ = nullptr;
    }

    // Named parameters
    for (auto* paramSym : funcSym->parameters) {
        llvm::Value* argVal = fn->getArg(argIdx++);
        // If param is a struct/class passed by pointer, copy the struct into a local alloca
        // so that GEP field access works correctly on the alloca
        if (paramSym->type && isUserStructKind(paramSym->type.get())) {
            llvm::Type* structTy = mapType(paramSym->type);
            auto* alloca = createEntryBlockAlloca(fn, structTy, paramSym->name);
            auto* val = builder_.CreateLoad(structTy, argVal, paramSym->name + ".val");
            builder_.CreateStore(val, alloca);
            namedValues_[paramSym] = alloca;
        } else {
            llvm::Type* paramTy = argVal->getType();
            auto* alloca = createEntryBlockAlloca(fn, paramTy, paramSym->name);
            builder_.CreateStore(argVal, alloca);
            namedValues_[paramSym] = alloca;
        }
    }

    // Enter function body scope
    if (funcSym->bodyScope) {
        enterNamedScope(funcSym->bodyScope);
    }

    // Push RAII scope for function body (ensures destructors fire for
    // variables declared directly in the function body, not just in nested blocks)
    pushRAIIScope();

    // Visit body statements
    for (auto& stmt : node.body->statements) {
        stmt->accept(*this);
        // Stop emitting if current block already has a terminator
        if (builder_.GetInsertBlock()->getTerminator()) break;
    }

    // Add default terminator if needed
    if (!builder_.GetInsertBlock()->getTerminator()) {
        emitScopeDestructors();
        if (fn->getReturnType()->isVoidTy()) {
            builder_.CreateRetVoid();
        } else {
            builder_.CreateRet(llvm::UndefValue::get(fn->getReturnType()));
        }
    }

    popRAIIScope();

    if (funcSym->bodyScope) {
        leaveNamedScope();
    }

    // Restore state
    namedValues_ = savedNamedValues;
    currentFunction_ = prevFunction;
    currentThisPtr_ = prevThisPtr;
}

void IRGenerator::visit(ConstructorDeclaration& node) {
    if (!currentType_) return;
    auto* classSym = currentType_->as<ClassSymbol>();
    if (!classSym || !classSym->constructor) return;

    auto* ctorSym = classSym->constructor;
    auto it = functionCache_.find(ctorSym);
    if (it == functionCache_.end()) return;

    llvm::Function* fn = it->second;
    if (!node.body) return;

    auto* entryBB = llvm::BasicBlock::Create(context_, "entry", fn);
    builder_.SetInsertPoint(entryBB);

    auto* prevFunction = currentFunction_;
    auto* prevThisPtr = currentThisPtr_;
    currentFunction_ = fn;

    auto savedNamedValues = namedValues_;
    namedValues_.clear();

    // First arg is 'this'
    unsigned argIdx = 0;
    currentThisPtr_ = fn->getArg(argIdx++);

    for (auto* paramSym : ctorSym->parameters) {
        llvm::Value* argVal = fn->getArg(argIdx++);
        if (paramSym->type && isUserStructKind(paramSym->type.get())) {
            llvm::Type* structTy = mapType(paramSym->type);
            auto* alloca = createEntryBlockAlloca(fn, structTy, paramSym->name);
            auto* val = builder_.CreateLoad(structTy, argVal, paramSym->name + ".val");
            builder_.CreateStore(val, alloca);
            namedValues_[paramSym] = alloca;
        } else {
            llvm::Type* paramTy = argVal->getType();
            auto* alloca = createEntryBlockAlloca(fn, paramTy, paramSym->name);
            builder_.CreateStore(argVal, alloca);
            namedValues_[paramSym] = alloca;
        }
    }

    // Super constructor call (before body, after parameter setup)
    if (node.hasSuperCall && classSym->baseClass && classSym->baseClass->constructor) {
        auto baseCtorIt = functionCache_.find(classSym->baseClass->constructor);
        if (baseCtorIt != functionCache_.end()) {
            std::vector<llvm::Value*> superArgs;
            superArgs.push_back(currentThisPtr_);  // pass this to base ctor
            for (auto& arg : node.superArgs) {
                arg->accept(*this);
                if (lastValue_) {
                    if (arg->resolvedType && isUserStructKind(arg->resolvedType.get())) {
                        llvm::Value* argPtr = emitLValue(*arg);
                        if (argPtr) { superArgs.push_back(argPtr); continue; }
                    }
                    superArgs.push_back(lastValue_);
                }
            }
            builder_.CreateCall(baseCtorIt->second, superArgs);
        }
    }

    // Store vtable pointer (overwrites whatever base ctor set)
    storeVtablePtr(currentThisPtr_, classSym);

    // Zero-initialize closure-typed fields to prevent releasing garbage
    // if destructor fires before all closure fields are assigned
    {
        auto userType = registry_.getUserType(classSym->name, Type::Kind::Class, classSym);
        auto* structTy = getStructType(userType.get());
        auto* fatPtrTy = getFatPtrType();
        for (auto* field : classSym->fields) {
            if (field->type && field->type->is<FunctionType>()) {
                int gepIdx = getFieldGEPIndex(classSym, field);
                if (gepIdx >= 0) {
                    auto* fieldPtr = builder_.CreateStructGEP(structTy, currentThisPtr_,
                                                              gepIdx, field->name + ".init");
                    builder_.CreateStore(llvm::ConstantAggregateZero::get(fatPtrTy), fieldPtr);
                }
            }
        }
    }

    if (ctorSym->bodyScope) {
        enterNamedScope(ctorSym->bodyScope);
    }

    for (auto& stmt : node.body->statements) {
        stmt->accept(*this);
        if (builder_.GetInsertBlock()->getTerminator()) break;
    }

    if (!builder_.GetInsertBlock()->getTerminator()) {
        builder_.CreateRetVoid();
    }

    if (ctorSym->bodyScope) {
        leaveNamedScope();
    }

    namedValues_ = savedNamedValues;
    currentFunction_ = prevFunction;
    currentThisPtr_ = prevThisPtr;
}

void IRGenerator::visit(DestructorDeclaration& node) {
    if (!currentType_) return;
    auto* classSym = currentType_->as<ClassSymbol>();
    if (!classSym || !classSym->destructor) return;

    auto* dtorSym = classSym->destructor;
    auto it = functionCache_.find(dtorSym);
    if (it == functionCache_.end()) return;

    llvm::Function* fn = it->second;
    if (!node.body) return;

    auto* entryBB = llvm::BasicBlock::Create(context_, "entry", fn);
    builder_.SetInsertPoint(entryBB);

    auto* prevFunction = currentFunction_;
    auto* prevThisPtr = currentThisPtr_;
    currentFunction_ = fn;

    auto savedNamedValues = namedValues_;
    namedValues_.clear();

    currentThisPtr_ = fn->getArg(0);

    if (dtorSym->bodyScope) {
        enterNamedScope(dtorSym->bodyScope);
    }

    for (auto& stmt : node.body->statements) {
        stmt->accept(*this);
        if (builder_.GetInsertBlock()->getTerminator()) break;
    }

    // Release closure-typed fields (own fields only; base dtor handles its own)
    if (!builder_.GetInsertBlock()->getTerminator()) {
        auto userType = registry_.getUserType(classSym->name, Type::Kind::Class, classSym);
        auto* structTy = getStructType(userType.get());
        auto* fatPtrTy = getFatPtrType();

        for (auto* field : classSym->fields) {
            if (field->type && field->type->is<FunctionType>()) {
                int gepIdx = getFieldGEPIndex(classSym, field);
                if (gepIdx >= 0) {
                    auto* fieldPtr = builder_.CreateStructGEP(structTy, currentThisPtr_,
                                                              gepIdx, field->name + ".dtor.ptr");
                    auto* fatVal = builder_.CreateLoad(fatPtrTy, fieldPtr, field->name + ".dtor.fat");
                    auto* envPtr = builder_.CreateExtractValue(fatVal, {1}, field->name + ".dtor.env");
                    builder_.CreateCall(getOrCreateClosureReleaseFn(), {envPtr});
                }
            }
        }
    }

    // Chain to base destructor after body (derived cleanup first, then base)
    if (!builder_.GetInsertBlock()->getTerminator()) {
        if (classSym->baseClass && classSym->baseClass->destructor) {
            auto baseDtorIt = functionCache_.find(classSym->baseClass->destructor);
            if (baseDtorIt != functionCache_.end()) {
                builder_.CreateCall(baseDtorIt->second, {currentThisPtr_});
            }
        }
        builder_.CreateRetVoid();
    }

    if (dtorSym->bodyScope) {
        leaveNamedScope();
    }

    namedValues_ = savedNamedValues;
    currentFunction_ = prevFunction;
    currentThisPtr_ = prevThisPtr;
}

void IRGenerator::visit(OperatorDeclaration& node) {
    if (!currentType_) return;

    // Find the operator symbol
    OverloadableOp op;
    switch (node.op) {
        case OperatorKind::Plus:         op = OverloadableOp::Plus; break;
        case OperatorKind::Minus:        op = OverloadableOp::Minus; break;
        case OperatorKind::Star:         op = OverloadableOp::Star; break;
        case OperatorKind::Divide:       op = OverloadableOp::Slash; break;
        case OperatorKind::Modulo:       op = OverloadableOp::Modulo; break;
        case OperatorKind::Index:        op = OverloadableOp::Index; break;
        case OperatorKind::Equal:        op = OverloadableOp::Equals; break;
        case OperatorKind::NotEqual:     op = OverloadableOp::NotEquals; break;
        case OperatorKind::Less:         op = OverloadableOp::Less; break;
        case OperatorKind::LessEqual:    op = OverloadableOp::LessEqual; break;
        case OperatorKind::Greater:      op = OverloadableOp::Greater; break;
        case OperatorKind::GreaterEqual: op = OverloadableOp::GreaterEqual; break;
        default: return;
    }

    auto* opSym = currentType_->findOperator(op);
    if (!opSym) return;

    auto it = functionCache_.find(opSym);
    if (it == functionCache_.end()) return;

    llvm::Function* fn = it->second;
    if (!node.body) return;

    auto* entryBB = llvm::BasicBlock::Create(context_, "entry", fn);
    builder_.SetInsertPoint(entryBB);

    auto* prevFunction = currentFunction_;
    auto* prevThisPtr = currentThisPtr_;
    currentFunction_ = fn;

    auto savedNamedValues = namedValues_;
    namedValues_.clear();

    unsigned argIdx = 0;
    currentThisPtr_ = fn->getArg(argIdx++);

    for (auto* paramSym : opSym->parameters) {
        llvm::Value* argVal = fn->getArg(argIdx++);
        if (paramSym->type && isUserStructKind(paramSym->type.get())) {
            llvm::Type* structTy = mapType(paramSym->type);
            auto* alloca = createEntryBlockAlloca(fn, structTy, paramSym->name);
            auto* val = builder_.CreateLoad(structTy, argVal, paramSym->name + ".val");
            builder_.CreateStore(val, alloca);
            namedValues_[paramSym] = alloca;
        } else {
            llvm::Type* paramTy = argVal->getType();
            auto* alloca = createEntryBlockAlloca(fn, paramTy, paramSym->name);
            builder_.CreateStore(argVal, alloca);
            namedValues_[paramSym] = alloca;
        }
    }

    if (opSym->bodyScope) {
        enterNamedScope(opSym->bodyScope);
    }

    for (auto& stmt : node.body->statements) {
        stmt->accept(*this);
        if (builder_.GetInsertBlock()->getTerminator()) break;
    }

    if (!builder_.GetInsertBlock()->getTerminator()) {
        if (fn->getReturnType()->isVoidTy()) {
            builder_.CreateRetVoid();
        } else {
            builder_.CreateRet(llvm::UndefValue::get(fn->getReturnType()));
        }
    }

    if (opSym->bodyScope) {
        leaveNamedScope();
    }

    namedValues_ = savedNamedValues;
    currentFunction_ = prevFunction;
    currentThisPtr_ = prevThisPtr;
}

void IRGenerator::visit(ExternFunctionDeclaration& /*node*/) {}
void IRGenerator::visit(EnumMemberNode& /*node*/) {}
void IRGenerator::visit(EnumDeclaration& /*node*/) {}

void IRGenerator::visit(StructDeclaration& node) {
    auto* sym = currentScope_->lookupLocal(node.name);
    auto* prevType = currentType_;
    currentType_ = sym ? sym->as<TypeSymbol>() : nullptr;

    if (currentType_ && currentType_->memberScope) {
        enterNamedScope(currentType_->memberScope);
    }

    for (auto& method : node.methods) {
        method->accept(*this);
    }
    for (auto& op : node.operators) {
        op->accept(*this);
    }

    if (currentType_ && currentType_->memberScope) {
        leaveNamedScope();
    }

    currentType_ = prevType;
}

void IRGenerator::visit(ClassDeclaration& node) {
    auto* sym = currentScope_->lookupLocal(node.name);
    auto* prevType = currentType_;
    currentType_ = sym ? sym->as<TypeSymbol>() : nullptr;

    if (currentType_ && currentType_->memberScope) {
        enterNamedScope(currentType_->memberScope);
    }

    if (node.hasConstructor()) {
        node.constructor->accept(*this);
    }
    if (node.hasDestructor()) {
        node.destructor->accept(*this);
    }
    for (auto& method : node.methods) {
        method->accept(*this);
    }
    for (auto& op : node.operators) {
        op->accept(*this);
    }

    if (currentType_ && currentType_->memberScope) {
        leaveNamedScope();
    }

    currentType_ = prevType;
}

void IRGenerator::visit(InterfaceDeclaration& /*node*/) {
    // Interfaces have no runtime representation — nothing to emit
}

void IRGenerator::visit(ParameterNode& /*node*/) {}

//================================================================================
// Statement visitors
//================================================================================
void IRGenerator::visit(BlockStatement& node) {
    enterNextChildScope();
    pushRAIIScope();

    for (auto& stmt : node.statements) {
        stmt->accept(*this);
        if (builder_.GetInsertBlock()->getTerminator()) break;
    }

    emitScopeDestructors();
    popRAIIScope();
    leaveChildScope();
}

void IRGenerator::visit(ExpressionStatement& node) {
    node.expression->accept(*this);
    // Discard result
}

void IRGenerator::visit(ReturnStatement& node) {
    if (node.hasValue()) {
        // Mark returned RAII variable for suppression
        if (auto* ident = node.value->as<IdentifierExpression>()) {
            if (ident->resolvedSymbol && !raiiScopeStack_.empty()) {
                for (auto& scope : raiiScopeStack_) {
                    for (auto& [ptr, dtor] : scope.destructibles) {
                        auto it = namedValues_.find(ident->resolvedSymbol);
                        if (it != namedValues_.end() && it->second == ptr) {
                            scope.returnedVars.insert(ident->resolvedSymbol);
                        }
                    }
                }
            }
        }

        node.value->accept(*this);

        emitReturnDestructors();

        if (lastValue_ && !currentFunction_->getReturnType()->isVoidTy()) {
            builder_.CreateRet(lastValue_);
        } else {
            builder_.CreateRetVoid();
        }
    } else {
        emitReturnDestructors();
        builder_.CreateRetVoid();
    }
}

void IRGenerator::visit(IfStatement& node) {
    // Emit condition
    node.condition->accept(*this);
    llvm::Value* condVal = lastValue_;
    if (!condVal) {
        condVal = llvm::ConstantInt::getFalse(context_);
    }

    // Ensure condition is i1
    if (!condVal->getType()->isIntegerTy(1)) {
        condVal = builder_.CreateICmpNE(condVal,
            llvm::ConstantInt::get(condVal->getType(), 0), "ifcond");
    }

    auto* thenBB = llvm::BasicBlock::Create(context_, "then", currentFunction_);
    auto* mergeBB = llvm::BasicBlock::Create(context_, "ifmerge", currentFunction_);

    if (node.hasElse() || node.hasElseIf()) {
        auto* elseBB = llvm::BasicBlock::Create(context_, "else", currentFunction_);
        builder_.CreateCondBr(condVal, thenBB, elseBB);

        // Then
        builder_.SetInsertPoint(thenBB);
        node.thenBody->accept(*this);
        if (!builder_.GetInsertBlock()->getTerminator())
            builder_.CreateBr(mergeBB);

        // Else-if chain + else
        builder_.SetInsertPoint(elseBB);
        if (node.hasElseIf()) {
            for (size_t i = 0; i < node.elseIfClauses.size(); i++) {
                auto& clause = node.elseIfClauses[i];
                clause.condition->accept(*this);
                llvm::Value* eifCond = lastValue_;
                if (!eifCond) eifCond = llvm::ConstantInt::getFalse(context_);
                if (!eifCond->getType()->isIntegerTy(1)) {
                    eifCond = builder_.CreateICmpNE(eifCond,
                        llvm::ConstantInt::get(eifCond->getType(), 0), "eifcond");
                }

                auto* eifThenBB = llvm::BasicBlock::Create(context_, "elif.then", currentFunction_);
                auto* eifNextBB = llvm::BasicBlock::Create(context_, "elif.next", currentFunction_);
                builder_.CreateCondBr(eifCond, eifThenBB, eifNextBB);

                builder_.SetInsertPoint(eifThenBB);
                clause.body->accept(*this);
                if (!builder_.GetInsertBlock()->getTerminator())
                    builder_.CreateBr(mergeBB);

                builder_.SetInsertPoint(eifNextBB);
            }
        }
        if (node.hasElse()) {
            node.elseBody->accept(*this);
        }
        if (!builder_.GetInsertBlock()->getTerminator())
            builder_.CreateBr(mergeBB);
    } else {
        builder_.CreateCondBr(condVal, thenBB, mergeBB);

        builder_.SetInsertPoint(thenBB);
        node.thenBody->accept(*this);
        if (!builder_.GetInsertBlock()->getTerminator())
            builder_.CreateBr(mergeBB);
    }

    builder_.SetInsertPoint(mergeBB);
}

void IRGenerator::visit(SwitchStatement& node) {
    // Emit subject
    node.subject->accept(*this);
    llvm::Value* subjectVal = lastValue_;
    if (!subjectVal) return;

    auto* mergeBB = llvm::BasicBlock::Create(context_, "switch.merge", currentFunction_);
    auto* defaultBB = node.hasDefault()
        ? llvm::BasicBlock::Create(context_, "switch.default", currentFunction_)
        : mergeBB;

    // Check if all cases are constant integers → use LLVM switch instruction
    bool allConstant = true;
    for (auto& sc : node.cases) {
        if (!sc.value) { allConstant = false; break; }
        // We'll try to evaluate and see if we get a ConstantInt
    }

    if (allConstant && subjectVal->getType()->isIntegerTy()) {
        auto* switchInst = builder_.CreateSwitch(subjectVal, defaultBB,
                                                  (unsigned)node.cases.size());

        for (auto& sc : node.cases) {
            auto* caseBB = llvm::BasicBlock::Create(context_, "switch.case", currentFunction_);

            // Emit case value at module level to get constant
            sc.value->accept(*this);
            if (auto* constInt = llvm::dyn_cast<llvm::ConstantInt>(lastValue_)) {
                switchInst->addCase(constInt, caseBB);
            }

            builder_.SetInsertPoint(caseBB);
            for (auto& stmt : sc.body) {
                stmt->accept(*this);
                if (builder_.GetInsertBlock()->getTerminator()) break;
            }
            if (!builder_.GetInsertBlock()->getTerminator())
                builder_.CreateBr(mergeBB);
        }
    } else {
        // Emit as chain of if/else
        for (size_t i = 0; i < node.cases.size(); i++) {
            auto& sc = node.cases[i];
            sc.value->accept(*this);
            llvm::Value* caseVal = lastValue_;

            llvm::Value* cond;
            if (subjectVal->getType()->isFloatingPointTy()) {
                cond = builder_.CreateFCmpOEQ(subjectVal, caseVal, "switch.cmp");
            } else {
                cond = builder_.CreateICmpEQ(subjectVal, caseVal, "switch.cmp");
            }

            auto* caseBB = llvm::BasicBlock::Create(context_, "switch.case", currentFunction_);
            auto* nextBB = (i + 1 < node.cases.size())
                ? llvm::BasicBlock::Create(context_, "switch.next", currentFunction_)
                : defaultBB;

            builder_.CreateCondBr(cond, caseBB, nextBB);

            builder_.SetInsertPoint(caseBB);
            for (auto& stmt : sc.body) {
                stmt->accept(*this);
                if (builder_.GetInsertBlock()->getTerminator()) break;
            }
            if (!builder_.GetInsertBlock()->getTerminator())
                builder_.CreateBr(mergeBB);

            builder_.SetInsertPoint(nextBB);
        }
    }

    // Default case
    if (node.hasDefault()) {
        builder_.SetInsertPoint(defaultBB);
        for (auto& stmt : node.defaultCase) {
            stmt->accept(*this);
            if (builder_.GetInsertBlock()->getTerminator()) break;
        }
        if (!builder_.GetInsertBlock()->getTerminator())
            builder_.CreateBr(mergeBB);
    }

    builder_.SetInsertPoint(mergeBB);
}

void IRGenerator::visit(ForStatement& node) {
    // For loop has its own scope (for the loop variable)
    enterNextChildScope();

    // Emit initializer
    if (node.hasInitDeclaration()) {
        node.initDeclaration->accept(*this);
    }
    if (node.hasInitExpressions()) {
        for (auto& expr : node.initExpressions) {
            expr->accept(*this);
        }
    }

    auto* condBB = llvm::BasicBlock::Create(context_, "for.cond", currentFunction_);
    auto* bodyBB = llvm::BasicBlock::Create(context_, "for.body", currentFunction_);
    auto* iterBB = llvm::BasicBlock::Create(context_, "for.iter", currentFunction_);
    auto* exitBB = llvm::BasicBlock::Create(context_, "for.exit", currentFunction_);

    builder_.CreateBr(condBB);

    // Condition
    builder_.SetInsertPoint(condBB);
    if (node.hasCondition()) {
        node.condition->accept(*this);
        llvm::Value* condVal = lastValue_;
        if (!condVal) condVal = llvm::ConstantInt::getFalse(context_);
        if (!condVal->getType()->isIntegerTy(1)) {
            condVal = builder_.CreateICmpNE(condVal,
                llvm::ConstantInt::get(condVal->getType(), 0), "forcond");
        }
        builder_.CreateCondBr(condVal, bodyBB, exitBB);
    } else {
        builder_.CreateBr(bodyBB);
    }

    // Body
    builder_.SetInsertPoint(bodyBB);
    auto* prevExitBlock = loopExitBlock_;
    auto* prevIterBlock = loopIterBlock_;
    auto prevLoopRAIIDepth = loopRAIIScopeDepth_;
    loopExitBlock_ = exitBB;
    loopIterBlock_ = iterBB;
    loopRAIIScopeDepth_ = raiiScopeStack_.size();

    node.body->accept(*this);

    loopExitBlock_ = prevExitBlock;
    loopIterBlock_ = prevIterBlock;
    loopRAIIScopeDepth_ = prevLoopRAIIDepth;

    if (!builder_.GetInsertBlock()->getTerminator())
        builder_.CreateBr(iterBB);

    // Iterator
    builder_.SetInsertPoint(iterBB);
    if (node.hasIterators()) {
        for (auto& iter : node.iterators) {
            iter->accept(*this);
        }
    }
    builder_.CreateBr(condBB);

    // Exit
    builder_.SetInsertPoint(exitBB);

    leaveChildScope();
}

void IRGenerator::visit(WhileStatement& node) {
    auto* condBB = llvm::BasicBlock::Create(context_, "while.cond", currentFunction_);
    auto* bodyBB = llvm::BasicBlock::Create(context_, "while.body", currentFunction_);
    auto* exitBB = llvm::BasicBlock::Create(context_, "while.exit", currentFunction_);

    builder_.CreateBr(condBB);

    builder_.SetInsertPoint(condBB);
    node.condition->accept(*this);
    llvm::Value* condVal = lastValue_;
    if (!condVal) condVal = llvm::ConstantInt::getFalse(context_);
    if (!condVal->getType()->isIntegerTy(1)) {
        condVal = builder_.CreateICmpNE(condVal,
            llvm::ConstantInt::get(condVal->getType(), 0), "whilecond");
    }
    builder_.CreateCondBr(condVal, bodyBB, exitBB);

    builder_.SetInsertPoint(bodyBB);
    auto* prevExitBlock = loopExitBlock_;
    auto* prevIterBlock = loopIterBlock_;
    auto prevLoopRAIIDepth = loopRAIIScopeDepth_;
    loopExitBlock_ = exitBB;
    loopIterBlock_ = condBB;
    loopRAIIScopeDepth_ = raiiScopeStack_.size();

    node.body->accept(*this);

    loopExitBlock_ = prevExitBlock;
    loopIterBlock_ = prevIterBlock;
    loopRAIIScopeDepth_ = prevLoopRAIIDepth;

    if (!builder_.GetInsertBlock()->getTerminator())
        builder_.CreateBr(condBB);

    builder_.SetInsertPoint(exitBB);
}

void IRGenerator::visit(BreakStatement& /*node*/) {
    emitBreakDestructors();
    if (loopExitBlock_) {
        builder_.CreateBr(loopExitBlock_);
    }
}

void IRGenerator::visit(ContinueStatement& /*node*/) {
    emitBreakDestructors();
    if (loopIterBlock_) {
        builder_.CreateBr(loopIterBlock_);
    }
}

void IRGenerator::visit(DeleteStatement& node) {
    node.target->accept(*this);
    llvm::Value* ptrVal = lastValue_;
    if (!ptrVal) return;

    const Type* targetType = node.target->resolvedType.get();
    if (auto* ptrTy = targetType->as<PointerType>()) {
        const Type* pointee = ptrTy->baseType.get();
        if (auto* userType = pointee->as<UserType>()) {
            if (userType->underlyingKind == Type::Kind::Interface) {
                // Interface fat pointer { objPtr, itable } — free the object pointer
                ptrVal = builder_.CreateExtractValue(ptrVal, {0}, "iface.del.obj");
            } else {
                // Class/struct: call destructor if present, then free
                auto* typeSym = static_cast<TypeSymbol*>(userType->symbol);
                auto* classSym = typeSym ? typeSym->as<ClassSymbol>() : nullptr;
                if (classSym && classSym->destructor) {
                    auto it = functionCache_.find(classSym->destructor);
                    if (it != functionCache_.end()) {
                        builder_.CreateCall(it->second, {ptrVal});
                    }
                }
            }
        }
    }

    // Call free on the (possibly extracted) pointer
    auto freeCallee = module_->getOrInsertFunction("free",
        llvm::FunctionType::get(llvm::Type::getVoidTy(context_),
            {llvm::PointerType::getUnqual(context_)}, false));
    builder_.CreateCall(freeCallee, {ptrVal});
}

void IRGenerator::visit(RawBlock& node) {
    // Raw blocks have their own scope (matching SymbolTableBuilder)
    if (node.body) {
        enterNextChildScope();
        for (auto& stmt : node.body->statements) {
            stmt->accept(*this);
            if (builder_.GetInsertBlock()->getTerminator()) break;
        }
        leaveChildScope();
    }
}

//================================================================================
// Variable declaration
//================================================================================
void IRGenerator::visit(VariableDeclaration& node) {
    // Look up the VariableSymbol
    auto* sym = currentScope_->lookupLocal(node.name);
    auto* varSym = sym ? sym->as<VariableSymbol>() : nullptr;
    if (!varSym) return;

    // Get the LLVM type
    llvm::Type* varTy = mapType(varSym->type);
    if (varTy->isVoidTy()) return;

    // Create alloca in entry block
    auto* alloca = createEntryBlockAlloca(currentFunction_, varTy, node.name);
    namedValues_[varSym] = alloca;

    // Zero-initialize closure allocas (prevents releasing garbage on reassignment)
    if (varSym->type && varSym->type->is<FunctionType>()) {
        builder_.CreateStore(llvm::ConstantAggregateZero::get(varTy), alloca);
    }

    // Zero-initialize struct allocas that have closure-typed fields
    // (prevents releasing garbage if cleanup runs before all fields assigned)
    if (varSym->type && !varSym->type->is<FunctionType>()) {
        if (auto* userType = varSym->type->as<UserType>()) {
            if (userType->underlyingKind == Type::Kind::Struct) {
                auto* structSym = static_cast<TypeSymbol*>(userType->symbol)->as<StructSymbol>();
                if (structSym) {
                    for (auto* field : structSym->fields) {
                        if (field->type && field->type->is<FunctionType>()) {
                            // Zero-init the entire struct
                            builder_.CreateStore(llvm::Constant::getNullValue(varTy), alloca);
                            break;
                        }
                    }
                }
            }
        }
    }

    // Emit initializer
    if (node.initializer) {
        node.initializer->accept(*this);
        if (lastValue_) {
            // Wrap class* → interface* if needed
            if (varSym->type && node.initializer->resolvedType) {
                const Type* dstTy = varSym->type.get();
                const Type* srcTy = node.initializer->resolvedType.get();
                if (auto* dstPtr = dstTy->as<PointerType>()) {
                    if (auto* dstUser = dstPtr->baseType->as<UserType>()) {
                        if (dstUser->underlyingKind == Type::Kind::Interface) {
                            if (auto* srcPtr = srcTy->as<PointerType>()) {
                                if (auto* srcUser = srcPtr->baseType->as<UserType>()) {
                                    if (srcUser->underlyingKind == Type::Kind::Class) {
                                        auto* cls = static_cast<ClassSymbol*>(srcUser->symbol);
                                        auto* iface = static_cast<InterfaceSymbol*>(dstUser->symbol);
                                        lastValue_ = emitWrapToInterfacePtr(lastValue_, cls, iface);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            // Convert null → zero fat pointer for FunctionType variables
            if (varSym->type && varSym->type->is<FunctionType>() &&
                llvm::isa<llvm::ConstantPointerNull>(lastValue_)) {
                lastValue_ = llvm::ConstantAggregateZero::get(getFatPtrType());
            }

            builder_.CreateStore(lastValue_, alloca);
        }
    }

    // Register RAII if the type has a destructor
    if (varSym->type && isUserStructKind(varSym->type.get())) {
        if (auto* userType = varSym->type->as<UserType>()) {
            auto* typeSym = static_cast<TypeSymbol*>(userType->symbol);
            auto* classSym = typeSym ? typeSym->as<ClassSymbol>() : nullptr;
            if (classSym && classSym->hasRAII()) {
                auto it = functionCache_.find(classSym->destructor);
                if (it != functionCache_.end()) {
                    registerRAII(alloca, it->second);
                }
            }
        }
    }

    // Register RAII for structs with closure-typed fields (synthetic cleanup)
    if (varSym->type && !varSym->type->is<FunctionType>()) {
        if (auto* userType = varSym->type->as<UserType>()) {
            if (userType->underlyingKind == Type::Kind::Struct) {
                auto* structSym = static_cast<TypeSymbol*>(userType->symbol)->as<StructSymbol>();
                if (structSym) {
                    bool hasClosureFields = false;
                    for (auto* field : structSym->fields) {
                        if (field->type && field->type->is<FunctionType>()) {
                            hasClosureFields = true;
                            break;
                        }
                    }
                    if (hasClosureFields) {
                        registerRAII(alloca, getOrCreateStructCleanupFn(structSym));
                    }
                }
            }
        }
    }

    // Register RAII for closure-typed variables (release envPtr at scope exit)
    // No initializer check — null-initialized closures also need RAII cleanup
    if (varSym->type && varSym->type->is<FunctionType>()) {
        registerRAII(alloca, getOrCreateClosureReleaseWrapper());
    }
}

void IRGenerator::visit(TupleDestructuringDeclaration& node) {
    // Emit initializer → produces a struct/tuple value
    node.initializer->accept(*this);
    llvm::Value* tupleVal = lastValue_;
    if (!tupleVal) return;

    // For each binding: extractvalue, alloca, store
    for (size_t i = 0; i < node.elements.size(); i++) {
        auto& elem = node.elements[i];
        llvm::Value* elemVal = builder_.CreateExtractValue(tupleVal, {(unsigned)i},
                                                           elem.name);

        // Look up the variable symbol
        auto* sym = currentScope_->lookupLocal(elem.name);
        if (sym) {
            auto* alloca = createEntryBlockAlloca(currentFunction_,
                                                   elemVal->getType(), elem.name);
            builder_.CreateStore(elemVal, alloca);
            namedValues_[sym] = alloca;
        }
    }
}

//================================================================================
// Expression visitors
//================================================================================
void IRGenerator::visit(IntegerLiteral& node) {
    lastValue_ = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context_), node.value);
}

void IRGenerator::visit(FloatLiteral& node) {
    lastValue_ = llvm::ConstantFP::get(llvm::Type::getDoubleTy(context_), node.value);
}

void IRGenerator::visit(BoolLiteral& node) {
    lastValue_ = llvm::ConstantInt::get(llvm::Type::getInt1Ty(context_), node.value ? 1 : 0);
}

void IRGenerator::visit(CharLiteral& node) {
    lastValue_ = llvm::ConstantInt::get(llvm::Type::getInt8Ty(context_), (uint8_t)node.value);
}

void IRGenerator::visit(StringLiteral& node) {
    // Deduplicate string constants
    auto it = stringConstants_.find(node.value);
    if (it != stringConstants_.end()) {
        // With opaque pointers, GlobalVariable* is already ptr type
        lastValue_ = it->second;
        return;
    }
    lastValue_ = builder_.CreateGlobalStringPtr(node.value, "str");
}

void IRGenerator::visit(InterpolatedString& node) {
    // Build format string and collect expression values
    std::string formatStr;
    std::vector<llvm::Value*> args;

    for (auto& part : node.parts) {
        if (part.kind == InterpolatedPartKind::Text) {
            formatStr += part.text;
        } else {
            // Expression part — emit and determine format specifier
            part.expression->accept(*this);
            llvm::Value* val = lastValue_;
            if (!val) continue;

            const Type* exprType = part.expression->resolvedType.get();
            if (isFloatingKind(exprType)) {
                // Promote float to double for printf
                if (val->getType()->isFloatTy()) {
                    val = builder_.CreateFPExt(val,
                        llvm::Type::getDoubleTy(context_), "fpext");
                }
                formatStr += "%f";
            } else if (isIntegerKind(exprType)) {
                formatStr += "%d";
            } else if (isBoolKind(exprType)) {
                formatStr += "%d";
            } else if (isPointerKind(exprType)) {
                formatStr += "%s";
            } else {
                formatStr += "%d";
            }
            args.push_back(val);
        }
    }

    // Heap-allocate buffer: two-pass snprintf (get length, then format)
    auto ptrTy = llvm::PointerType::getUnqual(context_);
    auto i32Ty = llvm::Type::getInt32Ty(context_);
    auto i64Ty = llvm::Type::getInt64Ty(context_);

    llvm::Value* fmtStr = builder_.CreateGlobalStringPtr(formatStr, "fmt");

    // Declare snprintf
    auto snprintfCallee = module_->getOrInsertFunction("snprintf",
        llvm::FunctionType::get(i32Ty, {ptrTy, i32Ty, ptrTy}, true));

    // Pass 1: snprintf(nullptr, 0, fmt, args...) to get required length
    std::vector<llvm::Value*> sizeArgs;
    sizeArgs.push_back(llvm::ConstantPointerNull::get(
        llvm::PointerType::getUnqual(context_)));
    sizeArgs.push_back(llvm::ConstantInt::get(i32Ty, 0));
    sizeArgs.push_back(fmtStr);
    for (auto* arg : args) {
        sizeArgs.push_back(arg);
    }
    llvm::Value* needed = builder_.CreateCall(snprintfCallee, sizeArgs, "snprintf.len");

    // malloc(needed + 1)
    llvm::Value* needed64 = builder_.CreateSExt(needed, i64Ty, "needed.i64");
    llvm::Value* allocSize = builder_.CreateAdd(needed64,
        llvm::ConstantInt::get(i64Ty, 1), "alloc.size");
    auto mallocCallee = module_->getOrInsertFunction("malloc",
        llvm::FunctionType::get(ptrTy, {i64Ty}, false));
    llvm::Value* buf = builder_.CreateCall(mallocCallee, {allocSize}, "interp.buf");

    // Pass 2: snprintf(buf, needed + 1, fmt, args...)
    llvm::Value* bufSize = builder_.CreateAdd(needed,
        llvm::ConstantInt::get(i32Ty, 1), "buf.size");
    std::vector<llvm::Value*> fmtArgs;
    fmtArgs.push_back(buf);
    fmtArgs.push_back(bufSize);
    fmtArgs.push_back(fmtStr);
    for (auto* arg : args) {
        fmtArgs.push_back(arg);
    }
    builder_.CreateCall(snprintfCallee, fmtArgs);

    // Register for RAII cleanup
    registerRAII(buf, getOrCreateStringFreeFn());
    lastValue_ = buf;
}

void IRGenerator::visit(NullLiteral& /*node*/) {
    lastValue_ = llvm::ConstantPointerNull::get(
        llvm::PointerType::getUnqual(context_));
}

void IRGenerator::visit(IdentifierExpression& node) {
    if (!node.resolvedSymbol) {
        lastValue_ = nullptr;
        return;
    }

    // Function symbol → return function pointer
    if (node.resolvedSymbol->is<FunctionSymbol>()) {
        auto it = functionCache_.find(node.resolvedSymbol);
        if (it != functionCache_.end()) {
            lastValue_ = it->second;
            return;
        }
    }

    // Variable symbol → load from alloca
    auto it = namedValues_.find(node.resolvedSymbol);
    if (it != namedValues_.end()) {
        llvm::Type* loadTy = mapType(node.resolvedType);
        if (loadTy->isVoidTy()) {
            lastValue_ = nullptr;
            return;
        }
        lastValue_ = builder_.CreateLoad(loadTy, it->second, node.name);
        return;
    }

    lastValue_ = nullptr;
}

void IRGenerator::visit(QualifiedNameExpression& node) {
    // Handle enum member access: TokenKind.Plus → constant int/string
    if (node.resolvedSymbol) {
        if (auto* enumSym = node.resolvedSymbol->as<EnumSymbol>()) {
            // Last part is the member name
            const auto& parts = node.qualifiedName.parts;
            if (parts.size() >= 2) {
                const auto& memberName = parts.back();
                for (const auto& member : enumSym->members) {
                    if (member.name == memberName) {
                        if (member.isString) {
                            lastValue_ = builder_.CreateGlobalStringPtr(
                                member.stringValue, "enum.str");
                        } else {
                            llvm::Type* enumTy = enumSym->underlyingType
                                ? mapType(enumSym->underlyingType)
                                : llvm::Type::getInt32Ty(context_);
                            lastValue_ = llvm::ConstantInt::get(enumTy, member.value);
                        }
                        return;
                    }
                }
            }
        }
    }

    lastValue_ = nullptr;
}

void IRGenerator::visit(ThisExpression& /*node*/) {
    lastValue_ = currentThisPtr_;
}

void IRGenerator::visit(MemberAccessExpression& node) {
    // Handle enum member access: Color.Red → constant int/string
    if (node.isEnumAccess) {
        if (node.isStringEnumAccess) {
            lastValue_ = builder_.CreateGlobalStringPtr(
                node.resolvedEnumStringValue, "enum.str");
        } else {
            llvm::Type* enumTy = llvm::Type::getInt32Ty(context_);
            if (node.resolvedType) {
                if (auto* user = node.resolvedType->as<UserType>()) {
                    if (user->underlyingKind == Type::Kind::Enum) {
                        auto* enumSym = static_cast<EnumSymbol*>(user->symbol);
                        if (enumSym && enumSym->underlyingType) {
                            enumTy = mapType(enumSym->underlyingType);
                        }
                    }
                }
            }
            lastValue_ = llvm::ConstantInt::get(enumTy, node.resolvedEnumValue);
        }
        return;
    }

    // Get lvalue (pointer to field)
    llvm::Value* fieldPtr = emitLValue(node);
    if (!fieldPtr) {
        lastValue_ = nullptr;
        return;
    }

    // Check if this is a method access vs a closure-typed field
    if (node.resolvedType && node.resolvedType->is<FunctionType>()) {
        // Distinguish closure fields (VariableSymbol) from methods (FunctionSymbol).
        // Closure fields must be loaded as fat pointer values; methods pass through as GEP.
        const Type* objType = node.object->resolvedType.get();
        if (auto* pt = objType->as<PointerType>()) objType = pt->baseType.get();
        if (auto* ut = objType->as<UserType>()) {
            auto* typeSym = static_cast<TypeSymbol*>(ut->symbol);
            auto* fieldSym = typeSym->findField(node.memberName);
            if (fieldSym && fieldSym->type && fieldSym->type->is<FunctionType>()) {
                // Closure-typed field — load the fat pointer value
                lastValue_ = builder_.CreateLoad(getFatPtrType(), fieldPtr, node.memberName);
                return;
            }
        }
        // Method reference — just pass through, CallExpression handles it
        lastValue_ = fieldPtr;
        return;
    }

    // Load the field value
    llvm::Type* fieldTy = mapType(node.resolvedType);
    if (fieldTy->isVoidTy()) {
        lastValue_ = nullptr;
        return;
    }
    lastValue_ = builder_.CreateLoad(fieldTy, fieldPtr, node.memberName);
}

void IRGenerator::visit(BinaryExpression& node) {
    // Check operator overload first
    if (node.isOperatorOverload && node.resolvedOperatorFunction) {
        auto it = functionCache_.find(node.resolvedOperatorFunction);
        if (it != functionCache_.end()) {
            llvm::Value* lhsPtr = emitLValue(*node.left);
            if (!lhsPtr) {
                node.left->accept(*this);
                lhsPtr = lastValue_;
                // If lhs is a struct value (not a pointer), store in temp alloca
                if (lhsPtr && lhsPtr->getType()->isStructTy()) {
                    auto* tmp = createEntryBlockAlloca(currentFunction_,
                        lhsPtr->getType(), "op.lhs.tmp");
                    builder_.CreateStore(lhsPtr, tmp);
                    lhsPtr = tmp;
                }
            }
            node.right->accept(*this);
            llvm::Value* rhsVal = lastValue_;

            // Pass rhs by pointer if it's a struct type
            llvm::Value* rhsArg = rhsVal;
            if (node.right->resolvedType && isUserStructKind(node.right->resolvedType.get())) {
                rhsArg = emitLValue(*node.right);
                if (!rhsArg) {
                    rhsArg = rhsVal;
                    // If rhs is a struct value, store in temp alloca
                    if (rhsArg && rhsArg->getType()->isStructTy()) {
                        auto* tmp = createEntryBlockAlloca(currentFunction_,
                            rhsArg->getType(), "op.rhs.tmp");
                        builder_.CreateStore(rhsArg, tmp);
                        rhsArg = tmp;
                    }
                }
            }

            lastValue_ = builder_.CreateCall(it->second, {lhsPtr, rhsArg});
            return;
        }
    }

    // Short-circuit logical AND/OR
    if (node.op == BinaryOp::LogicalAnd || node.op == BinaryOp::LogicalOr) {
        node.left->accept(*this);
        llvm::Value* leftVal = lastValue_;
        if (!leftVal) leftVal = llvm::ConstantInt::getFalse(context_);
        if (!leftVal->getType()->isIntegerTy(1)) {
            leftVal = builder_.CreateICmpNE(leftVal,
                llvm::ConstantInt::get(leftVal->getType(), 0));
        }

        auto* rhsBB = llvm::BasicBlock::Create(context_,
            node.op == BinaryOp::LogicalAnd ? "and.rhs" : "or.rhs", currentFunction_);
        auto* mergeBB = llvm::BasicBlock::Create(context_,
            node.op == BinaryOp::LogicalAnd ? "and.merge" : "or.merge", currentFunction_);

        auto* entryBB = builder_.GetInsertBlock();

        if (node.op == BinaryOp::LogicalAnd) {
            builder_.CreateCondBr(leftVal, rhsBB, mergeBB);
        } else {
            builder_.CreateCondBr(leftVal, mergeBB, rhsBB);
        }

        builder_.SetInsertPoint(rhsBB);
        node.right->accept(*this);
        llvm::Value* rightVal = lastValue_;
        if (!rightVal) rightVal = llvm::ConstantInt::getFalse(context_);
        if (!rightVal->getType()->isIntegerTy(1)) {
            rightVal = builder_.CreateICmpNE(rightVal,
                llvm::ConstantInt::get(rightVal->getType(), 0));
        }
        auto* rhsEndBB = builder_.GetInsertBlock();
        builder_.CreateBr(mergeBB);

        builder_.SetInsertPoint(mergeBB);
        auto* phi = builder_.CreatePHI(llvm::Type::getInt1Ty(context_), 2);
        if (node.op == BinaryOp::LogicalAnd) {
            phi->addIncoming(llvm::ConstantInt::getFalse(context_), entryBB);
        } else {
            phi->addIncoming(llvm::ConstantInt::getTrue(context_), entryBB);
        }
        phi->addIncoming(rightVal, rhsEndBB);
        lastValue_ = phi;
        return;
    }

    // Evaluate both sides
    node.left->accept(*this);
    llvm::Value* leftVal = lastValue_;
    node.right->accept(*this);
    llvm::Value* rightVal = lastValue_;

    if (!leftVal || !rightVal) {
        lastValue_ = nullptr;
        return;
    }

    const Type* leftType = node.left->resolvedType.get();
    const Type* rightType = node.right->resolvedType.get();

    // IMPORTANT: String checks must come BEFORE pointer checks because
    // isPointerKind() returns true for strings. Order matters!

    // String concatenation: s1 + s2
    if (isStringKind(leftType) && isStringKind(rightType) && node.op == BinaryOp::Add) {
        lastValue_ = emitStringConcat(leftVal, rightVal);
        return;
    }

    // Pointer arithmetic
    if (isPointerKind(leftType) && isIntegerKind(rightType) &&
        (node.op == BinaryOp::Add || node.op == BinaryOp::Sub)) {
        llvm::Type* elemTy = llvm::Type::getInt8Ty(context_);
        if (auto* ptrTy = leftType->as<PointerType>()) {
            elemTy = mapType(ptrTy->baseType);
        }
        if (node.op == BinaryOp::Add) {
            lastValue_ = builder_.CreateGEP(elemTy, leftVal, rightVal, "ptr.add");
        } else {
            auto* negIdx = builder_.CreateNeg(rightVal, "neg");
            lastValue_ = builder_.CreateGEP(elemTy, leftVal, negIdx, "ptr.sub");
        }
        return;
    }
    if (isIntegerKind(leftType) && isPointerKind(rightType) && node.op == BinaryOp::Add) {
        llvm::Type* elemTy = llvm::Type::getInt8Ty(context_);
        if (auto* ptrTy = rightType->as<PointerType>()) {
            elemTy = mapType(ptrTy->baseType);
        }
        lastValue_ = builder_.CreateGEP(elemTy, rightVal, leftVal, "ptr.add");
        return;
    }

    // String content comparison: s1 == s2, s1 != s2 (via strcmp)
    if (isStringKind(leftType) && isStringKind(rightType)) {
        if (node.op == BinaryOp::Equal || node.op == BinaryOp::NotEqual) {
            auto ptrTy = llvm::PointerType::getUnqual(context_);
            auto i32Ty = llvm::Type::getInt32Ty(context_);
            auto strcmpCallee = module_->getOrInsertFunction("strcmp",
                llvm::FunctionType::get(i32Ty, {ptrTy, ptrTy}, false));
            llvm::Value* cmp = builder_.CreateCall(strcmpCallee, {leftVal, rightVal}, "strcmp");
            if (node.op == BinaryOp::Equal) {
                lastValue_ = builder_.CreateICmpEQ(cmp,
                    llvm::ConstantInt::get(i32Ty, 0), "str.eq");
            } else {
                lastValue_ = builder_.CreateICmpNE(cmp,
                    llvm::ConstantInt::get(i32Ty, 0), "str.ne");
            }
            return;
        }
    }

    // Pointer comparison
    if (isPointerKind(leftType) || isPointerKind(rightType)) {
        switch (node.op) {
            case BinaryOp::Equal:
                lastValue_ = builder_.CreateICmpEQ(leftVal, rightVal, "ptr.eq");
                return;
            case BinaryOp::NotEqual:
                lastValue_ = builder_.CreateICmpNE(leftVal, rightVal, "ptr.ne");
                return;
            default: break;
        }
    }

    // Type widening: if one is int and other is double, widen the int
    bool leftIsFloat = isFloatingKind(leftType);
    bool rightIsFloat = isFloatingKind(rightType);
    bool leftIsInt = isIntegerKind(leftType);
    bool rightIsInt = isIntegerKind(rightType);

    if (leftIsInt && rightIsFloat) {
        leftVal = builder_.CreateSIToFP(leftVal, rightVal->getType(), "widen");
        leftIsFloat = true;
        leftIsInt = false;
    } else if (leftIsFloat && rightIsInt) {
        rightVal = builder_.CreateSIToFP(rightVal, leftVal->getType(), "widen");
        rightIsFloat = true;
        rightIsInt = false;
    }

    // Floating-point operations
    if (leftIsFloat && rightIsFloat) {
        switch (node.op) {
            case BinaryOp::Add: lastValue_ = builder_.CreateFAdd(leftVal, rightVal, "fadd"); return;
            case BinaryOp::Sub: lastValue_ = builder_.CreateFSub(leftVal, rightVal, "fsub"); return;
            case BinaryOp::Mul: lastValue_ = builder_.CreateFMul(leftVal, rightVal, "fmul"); return;
            case BinaryOp::Div: lastValue_ = builder_.CreateFDiv(leftVal, rightVal, "fdiv"); return;
            case BinaryOp::Mod: lastValue_ = builder_.CreateFRem(leftVal, rightVal, "frem"); return;
            case BinaryOp::Equal:        lastValue_ = builder_.CreateFCmpOEQ(leftVal, rightVal, "feq"); return;
            case BinaryOp::NotEqual:     lastValue_ = builder_.CreateFCmpONE(leftVal, rightVal, "fne"); return;
            case BinaryOp::Less:         lastValue_ = builder_.CreateFCmpOLT(leftVal, rightVal, "flt"); return;
            case BinaryOp::LessEqual:    lastValue_ = builder_.CreateFCmpOLE(leftVal, rightVal, "fle"); return;
            case BinaryOp::Greater:      lastValue_ = builder_.CreateFCmpOGT(leftVal, rightVal, "fgt"); return;
            case BinaryOp::GreaterEqual: lastValue_ = builder_.CreateFCmpOGE(leftVal, rightVal, "fge"); return;
            default: break;
        }
    }

    // Integer operations
    if (leftIsInt && rightIsInt) {
        // Ensure same width
        if (leftVal->getType() != rightVal->getType()) {
            if (leftVal->getType()->getIntegerBitWidth() < rightVal->getType()->getIntegerBitWidth()) {
                leftVal = builder_.CreateSExt(leftVal, rightVal->getType(), "sext");
            } else {
                rightVal = builder_.CreateSExt(rightVal, leftVal->getType(), "sext");
            }
        }

        switch (node.op) {
            case BinaryOp::Add: lastValue_ = builder_.CreateAdd(leftVal, rightVal, "add"); return;
            case BinaryOp::Sub: lastValue_ = builder_.CreateSub(leftVal, rightVal, "sub"); return;
            case BinaryOp::Mul: lastValue_ = builder_.CreateMul(leftVal, rightVal, "mul"); return;
            case BinaryOp::Div: lastValue_ = builder_.CreateSDiv(leftVal, rightVal, "sdiv"); return;
            case BinaryOp::Mod: lastValue_ = builder_.CreateSRem(leftVal, rightVal, "srem"); return;
            case BinaryOp::Equal:        lastValue_ = builder_.CreateICmpEQ(leftVal, rightVal, "eq"); return;
            case BinaryOp::NotEqual:     lastValue_ = builder_.CreateICmpNE(leftVal, rightVal, "ne"); return;
            case BinaryOp::Less:         lastValue_ = builder_.CreateICmpSLT(leftVal, rightVal, "slt"); return;
            case BinaryOp::LessEqual:    lastValue_ = builder_.CreateICmpSLE(leftVal, rightVal, "sle"); return;
            case BinaryOp::Greater:      lastValue_ = builder_.CreateICmpSGT(leftVal, rightVal, "sgt"); return;
            case BinaryOp::GreaterEqual: lastValue_ = builder_.CreateICmpSGE(leftVal, rightVal, "sge"); return;
            case BinaryOp::BitwiseAnd:   lastValue_ = builder_.CreateAnd(leftVal, rightVal, "and"); return;
            case BinaryOp::BitwiseOr:    lastValue_ = builder_.CreateOr(leftVal, rightVal, "or"); return;
            case BinaryOp::BitwiseXor:   lastValue_ = builder_.CreateXor(leftVal, rightVal, "xor"); return;
            case BinaryOp::ShiftLeft:    lastValue_ = builder_.CreateShl(leftVal, rightVal, "shl"); return;
            case BinaryOp::ShiftRight:   lastValue_ = builder_.CreateAShr(leftVal, rightVal, "ashr"); return;
            default: break;
        }
    }

    // Bool operations
    if (isBoolKind(leftType) && isBoolKind(rightType)) {
        switch (node.op) {
            case BinaryOp::Equal:    lastValue_ = builder_.CreateICmpEQ(leftVal, rightVal, "eq"); return;
            case BinaryOp::NotEqual: lastValue_ = builder_.CreateICmpNE(leftVal, rightVal, "ne"); return;
            default: break;
        }
    }

    lastValue_ = nullptr;
}

void IRGenerator::visit(UnaryExpression& node) {
    const Type* operandType = node.operand->resolvedType.get();

    // Address-of: return lvalue (pointer)
    if (node.op == UnaryOp::AddressOf) {
        lastValue_ = emitLValue(*node.operand);
        return;
    }

    // Pre/Post increment/decrement need lvalue
    if (node.op == UnaryOp::PreIncrement || node.op == UnaryOp::PreDecrement ||
        node.op == UnaryOp::PostIncrement || node.op == UnaryOp::PostDecrement) {
        llvm::Value* ptr = emitLValue(*node.operand);
        if (!ptr) { lastValue_ = nullptr; return; }

        llvm::Type* valTy = mapType(operandType);
        llvm::Value* oldVal = builder_.CreateLoad(valTy, ptr, "old");
        llvm::Value* one;

        if (isFloatingKind(operandType)) {
            one = llvm::ConstantFP::get(valTy, 1.0);
        } else {
            one = llvm::ConstantInt::get(valTy, 1);
        }

        llvm::Value* newVal;
        if (node.op == UnaryOp::PreIncrement || node.op == UnaryOp::PostIncrement) {
            newVal = isFloatingKind(operandType)
                ? builder_.CreateFAdd(oldVal, one, "inc")
                : builder_.CreateAdd(oldVal, one, "inc");
        } else {
            newVal = isFloatingKind(operandType)
                ? builder_.CreateFSub(oldVal, one, "dec")
                : builder_.CreateSub(oldVal, one, "dec");
        }

        builder_.CreateStore(newVal, ptr);
        lastValue_ = (node.op == UnaryOp::PreIncrement || node.op == UnaryOp::PreDecrement)
                     ? newVal : oldVal;
        return;
    }

    // Evaluate operand
    node.operand->accept(*this);
    llvm::Value* val = lastValue_;
    if (!val) { lastValue_ = nullptr; return; }

    switch (node.op) {
        case UnaryOp::Negate:
            if (isFloatingKind(operandType)) {
                lastValue_ = builder_.CreateFNeg(val, "fneg");
            } else {
                lastValue_ = builder_.CreateNeg(val, "neg");
            }
            break;

        case UnaryOp::LogicalNot:
            if (!val->getType()->isIntegerTy(1)) {
                val = builder_.CreateICmpNE(val,
                    llvm::ConstantInt::get(val->getType(), 0));
            }
            lastValue_ = builder_.CreateXor(val, llvm::ConstantInt::getTrue(context_), "not");
            break;

        case UnaryOp::BitwiseNot:
            lastValue_ = builder_.CreateNot(val, "bitnot");
            break;

        case UnaryOp::Dereference: {
            // Load from pointer
            llvm::Type* pointeeTy = llvm::Type::getInt8Ty(context_);
            if (auto* ptrTy = operandType->as<PointerType>()) {
                pointeeTy = mapType(ptrTy->baseType);
            }
            if (!pointeeTy->isVoidTy()) {
                lastValue_ = builder_.CreateLoad(pointeeTy, val, "deref");
            }
            break;
        }

        default:
            break;
    }
}

void IRGenerator::visit(AssignmentExpression& node) {
    // Get lvalue (pointer to target)
    llvm::Value* targetPtr = emitLValue(*node.target);
    if (!targetPtr) { lastValue_ = nullptr; return; }

    if (node.op == AssignOp::Assign) {
        // Release old closure envPtr before overwriting a closure variable
        if (node.target->resolvedType && node.target->resolvedType->is<FunctionType>()) {
            auto* oldFat = builder_.CreateLoad(getFatPtrType(), targetPtr, "old.fat");
            auto* oldEnv = builder_.CreateExtractValue(oldFat, {1}, "old.env");
            builder_.CreateCall(getOrCreateClosureReleaseFn(), {oldEnv});
        }

        // Simple assignment
        node.value->accept(*this);
        if (lastValue_) {
            // Convert null → zero fat pointer for FunctionType assignments
            if (node.target->resolvedType && node.target->resolvedType->is<FunctionType>() &&
                llvm::isa<llvm::ConstantPointerNull>(lastValue_)) {
                lastValue_ = llvm::ConstantAggregateZero::get(getFatPtrType());
            }

            builder_.CreateStore(lastValue_, targetPtr);

            // Retain new closure envPtr when storing into a field (struct/class).
            // Local variable reassignment doesn't need this — RAII handles it.
            // Field stores need retain because the source variable's RAII will
            // release its own reference at scope exit.
            if (node.target->resolvedType && node.target->resolvedType->is<FunctionType>() &&
                node.target->as<MemberAccessExpression>()) {
                auto* newEnv = builder_.CreateExtractValue(lastValue_, {1}, "new.env");
                builder_.CreateCall(getOrCreateClosureRetainFn(), {newEnv});
            }
        }
    } else {
        // Compound assignment: load, operate, store
        llvm::Type* valTy = mapType(node.target->resolvedType);
        llvm::Value* oldVal = builder_.CreateLoad(valTy, targetPtr, "old");

        node.value->accept(*this);
        llvm::Value* rhsVal = lastValue_;
        if (!rhsVal) return;

        const Type* targetType = node.target->resolvedType.get();
        bool isFloat = isFloatingKind(targetType);

        // Type widen rhs if needed
        if (isFloat && rhsVal->getType()->isIntegerTy()) {
            rhsVal = builder_.CreateSIToFP(rhsVal, oldVal->getType(), "widen");
        }

        llvm::Value* result = nullptr;
        switch (node.op) {
            case AssignOp::AddAssign:
                if (isStringKind(targetType)) {
                    result = emitStringConcat(oldVal, rhsVal);
                    break;
                }
                result = isFloat ? builder_.CreateFAdd(oldVal, rhsVal, "add")
                                 : builder_.CreateAdd(oldVal, rhsVal, "add"); break;
            case AssignOp::SubAssign:
                result = isFloat ? builder_.CreateFSub(oldVal, rhsVal, "sub")
                                 : builder_.CreateSub(oldVal, rhsVal, "sub"); break;
            case AssignOp::MulAssign:
                result = isFloat ? builder_.CreateFMul(oldVal, rhsVal, "mul")
                                 : builder_.CreateMul(oldVal, rhsVal, "mul"); break;
            case AssignOp::DivAssign:
                result = isFloat ? builder_.CreateFDiv(oldVal, rhsVal, "div")
                                 : builder_.CreateSDiv(oldVal, rhsVal, "div"); break;
            case AssignOp::ModAssign:
                result = isFloat ? builder_.CreateFRem(oldVal, rhsVal, "mod")
                                 : builder_.CreateSRem(oldVal, rhsVal, "mod"); break;
            case AssignOp::AndAssign:    result = builder_.CreateAnd(oldVal, rhsVal, "and"); break;
            case AssignOp::OrAssign:     result = builder_.CreateOr(oldVal, rhsVal, "or"); break;
            case AssignOp::XorAssign:    result = builder_.CreateXor(oldVal, rhsVal, "xor"); break;
            case AssignOp::ShiftLeftAssign:  result = builder_.CreateShl(oldVal, rhsVal, "shl"); break;
            case AssignOp::ShiftRightAssign: result = builder_.CreateAShr(oldVal, rhsVal, "ashr"); break;
            default: break;
        }

        if (result) {
            builder_.CreateStore(result, targetPtr);
            lastValue_ = result;
        }
    }
}

void IRGenerator::visit(TernaryExpression& node) {
    node.condition->accept(*this);
    llvm::Value* condVal = lastValue_;
    if (!condVal) condVal = llvm::ConstantInt::getFalse(context_);
    if (!condVal->getType()->isIntegerTy(1)) {
        condVal = builder_.CreateICmpNE(condVal,
            llvm::ConstantInt::get(condVal->getType(), 0), "terncond");
    }

    auto* thenBB = llvm::BasicBlock::Create(context_, "tern.then", currentFunction_);
    auto* elseBB = llvm::BasicBlock::Create(context_, "tern.else", currentFunction_);
    auto* mergeBB = llvm::BasicBlock::Create(context_, "tern.merge", currentFunction_);

    builder_.CreateCondBr(condVal, thenBB, elseBB);

    builder_.SetInsertPoint(thenBB);
    node.thenExpr->accept(*this);
    llvm::Value* thenVal = lastValue_;
    auto* thenEndBB = builder_.GetInsertBlock();
    builder_.CreateBr(mergeBB);

    builder_.SetInsertPoint(elseBB);
    node.elseExpr->accept(*this);
    llvm::Value* elseVal = lastValue_;
    auto* elseEndBB = builder_.GetInsertBlock();
    builder_.CreateBr(mergeBB);

    builder_.SetInsertPoint(mergeBB);
    if (thenVal && elseVal && thenVal->getType() == elseVal->getType()) {
        auto* phi = builder_.CreatePHI(thenVal->getType(), 2, "ternval");
        phi->addIncoming(thenVal, thenEndBB);
        phi->addIncoming(elseVal, elseEndBB);
        lastValue_ = phi;
    } else {
        lastValue_ = thenVal;
    }
}

void IRGenerator::visit(CallExpression& node) {
    llvm::Function* calleeFn = nullptr;
    llvm::Value* calleeVal = nullptr;
    llvm::Value* thisPtr = nullptr;
    bool isCtorCall = false;
    bool isVirtualCall = false;
    FunctionSymbol* virtualMethodSym = nullptr;
    ClassSymbol* virtualCallClass = nullptr;
    std::vector<llvm::Value*> args;

    // Method call: callee is MemberAccessExpression
    if (auto* memAccess = node.callee->as<MemberAccessExpression>()) {
        // String built-in methods: length(), charAt(i), substring(start, len)
        if (memAccess->isStringBuiltinMethod) {
            memAccess->object->accept(*this);
            llvm::Value* strVal = lastValue_;
            if (!strVal) { lastValue_ = nullptr; return; }

            auto ptrTy = llvm::PointerType::getUnqual(context_);
            auto i64Ty = llvm::Type::getInt64Ty(context_);
            auto i32Ty = llvm::Type::getInt32Ty(context_);
            auto i8Ty = llvm::Type::getInt8Ty(context_);

            if (memAccess->memberName == "length") {
                auto strlenCallee = module_->getOrInsertFunction("strlen",
                    llvm::FunctionType::get(i64Ty, {ptrTy}, false));
                llvm::Value* len = builder_.CreateCall(strlenCallee, {strVal}, "strlen");
                // Truncate i64 to i32 (Mingus int is i32)
                lastValue_ = builder_.CreateTrunc(len, i32Ty, "len.i32");
            } else if (memAccess->memberName == "charAt") {
                // Evaluate index argument
                if (!node.arguments.empty() && node.arguments[0]) {
                    node.arguments[0]->accept(*this);
                    llvm::Value* idx = lastValue_;
                    // Sign-extend i32 index to i64 for GEP
                    if (idx->getType()->isIntegerTy(32)) {
                        idx = builder_.CreateSExt(idx, i64Ty, "idx.i64");
                    }
                    llvm::Value* charPtr = builder_.CreateGEP(i8Ty, strVal, idx, "str.charAt");
                    lastValue_ = builder_.CreateLoad(i8Ty, charPtr, "char");
                } else {
                    lastValue_ = llvm::ConstantInt::get(i8Ty, 0);
                }
            } else if (memAccess->memberName == "substring") {
                // Evaluate args: start, len
                llvm::Value* start = nullptr;
                llvm::Value* len = nullptr;
                if (node.arguments.size() >= 2) {
                    node.arguments[0]->accept(*this);
                    start = lastValue_;
                    node.arguments[1]->accept(*this);
                    len = lastValue_;
                }
                if (!start || !len) { lastValue_ = nullptr; return; }

                // Sign-extend to i64
                if (start->getType()->isIntegerTy(32)) {
                    start = builder_.CreateSExt(start, i64Ty, "start.i64");
                }
                if (len->getType()->isIntegerTy(32)) {
                    len = builder_.CreateSExt(len, i64Ty, "len.i64");
                }

                // malloc(len + 1)
                auto mallocCallee = module_->getOrInsertFunction("malloc",
                    llvm::FunctionType::get(ptrTy, {i64Ty}, false));
                llvm::Value* allocSize = builder_.CreateAdd(len,
                    llvm::ConstantInt::get(i64Ty, 1), "alloc.size");
                llvm::Value* buf = builder_.CreateCall(mallocCallee, {allocSize}, "sub.buf");

                // memcpy(buf, str + start, len)
                auto memcpyCallee = module_->getOrInsertFunction("memcpy",
                    llvm::FunctionType::get(ptrTy, {ptrTy, ptrTy, i64Ty}, false));
                llvm::Value* srcPtr = builder_.CreateGEP(i8Ty, strVal, start, "sub.src");
                builder_.CreateCall(memcpyCallee, {buf, srcPtr, len});

                // Null-terminate: buf[len] = 0
                llvm::Value* endPtr = builder_.CreateGEP(i8Ty, buf, len, "sub.end");
                builder_.CreateStore(llvm::ConstantInt::get(i8Ty, 0), endPtr);

                // Register for RAII cleanup
                registerRAII(buf, getOrCreateStringFreeFn());
                lastValue_ = buf;
            }
            return;
        }

        // Interface dispatch: d->method() where d is an interface fat pointer
        if (memAccess->isArrow && memAccess->object && memAccess->object->resolvedType) {
            const Type* testObjType = memAccess->object->resolvedType.get();
            if (auto* testPtrTy = testObjType->as<PointerType>()) {
                if (auto* testUser = testPtrTy->baseType->as<UserType>()) {
                    if (testUser->underlyingKind == Type::Kind::Interface) {
                        // Load the fat pointer {objPtr, itable}
                        memAccess->object->accept(*this);
                        llvm::Value* fat = lastValue_;
                        llvm::Value* objPtr = builder_.CreateExtractValue(fat, {0}, "iface.obj");
                        llvm::Value* itable = builder_.CreateExtractValue(fat, {1}, "iface.itable");

                        auto* ifaceSym = static_cast<InterfaceSymbol*>(testUser->symbol);
                        auto* methodSym = ifaceSym->findMethod(memAccess->memberName);
                        if (!methodSym) { lastValue_ = nullptr; return; }

                        // GEP into itable to the method slot
                        auto* ptrTy2 = llvm::PointerType::getUnqual(context_);
                        llvm::Value* fnSlot = builder_.CreateGEP(
                            ptrTy2, itable,
                            builder_.getInt32(methodSym->vtableIndex),
                            "iface.slot");
                        llvm::Value* fn = builder_.CreateLoad(ptrTy2, fnSlot, "iface.fn");

                        // Build args: objPtr (this) + explicit call args
                        std::vector<llvm::Value*> ifaceArgs = {objPtr};
                        for (auto& arg : node.arguments) {
                            if (arg) {
                                arg->accept(*this);
                                if (lastValue_) {
                                    if (arg->resolvedType && isUserStructKind(arg->resolvedType.get())) {
                                        llvm::Value* argPtr = emitLValue(*arg);
                                        if (argPtr) { ifaceArgs.push_back(argPtr); continue; }
                                    }
                                    ifaceArgs.push_back(lastValue_);
                                }
                            }
                        }

                        auto* fnTy = buildFunctionType(methodSym);
                        if (fnTy->getReturnType()->isVoidTy()) {
                            builder_.CreateCall(fnTy, fn, ifaceArgs);
                            lastValue_ = nullptr;
                        } else {
                            lastValue_ = builder_.CreateCall(fnTy, fn, ifaceArgs, "iface.result");
                        }
                        return;
                    }
                }
            }
        }

        // Get the object pointer (this for the method)
        if (memAccess->isArrow) {
            // Arrow call: evaluate object expression to get the pointer value
            memAccess->object->accept(*this);
            thisPtr = lastValue_;
        } else {
            // Dot call: get lvalue (alloca) of the object — IS the this pointer
            thisPtr = emitLValue(*memAccess->object);
            if (!thisPtr) {
                memAccess->object->accept(*this);
                thisPtr = lastValue_;
                // If result is a struct value (not a pointer), store in temp alloca
                if (thisPtr && thisPtr->getType()->isStructTy()) {
                    auto* tmp = createEntryBlockAlloca(currentFunction_,
                        thisPtr->getType(), "method.this.tmp");
                    builder_.CreateStore(thisPtr, tmp);
                    thisPtr = tmp;
                }
            }
        }

        // Find the method function
        const Type* objType = memAccess->object->resolvedType.get();
        if (auto* ptrTy = objType->as<PointerType>()) {
            objType = ptrTy->baseType.get();
        }
        if (auto* userType = objType->as<UserType>()) {
            auto* typeSym = static_cast<TypeSymbol*>(userType->symbol);
            auto* methodSym = typeSym->findMethod(memAccess->memberName);
            if (methodSym) {
                auto* classSym = typeSym->as<ClassSymbol>();
                if (classSym && methodSym->vtableIndex >= 0 && classSym->vtableSize > 0) {
                    // Virtual dispatch
                    isVirtualCall = true;
                    virtualMethodSym = methodSym;
                    virtualCallClass = classSym;
                } else {
                    // Direct dispatch (structs, static methods)
                    auto it = functionCache_.find(methodSym);
                    if (it != functionCache_.end()) {
                        calleeFn = it->second;
                    }
                }
            } else {
                // Not a method — check for closure-typed field
                auto* fieldSym = typeSym->findField(memAccess->memberName);
                if (fieldSym && fieldSym->type && fieldSym->type->is<FunctionType>()) {
                    // Load the fat pointer from the field and use indirect call path
                    node.callee->accept(*this);
                    calleeVal = lastValue_;
                    thisPtr = nullptr;  // Not a method call — no this pointer
                }
            }
        }

        if ((calleeFn || isVirtualCall) && thisPtr) {
            args.push_back(thisPtr);
        }
    }
    // Constructor call via type name: callee is IdentifierExpression resolving to type
    else if (auto* ident = node.callee->as<IdentifierExpression>()) {
        if (ident->resolvedSymbol) {
            // Check if it resolves to a ClassSymbol (constructor call)
            if (auto* classSym = ident->resolvedSymbol->as<ClassSymbol>()) {
                if (classSym->constructor) {
                    auto it = functionCache_.find(classSym->constructor);
                    if (it != functionCache_.end()) {
                        calleeFn = it->second;
                        isCtorCall = true;
                        // Allocate storage for the object, pass as this
                        auto userType = registry_.getUserType(classSym->name,
                            Type::Kind::Class, classSym);
                        llvm::Type* objTy = mapType(userType);
                        thisPtr = createEntryBlockAlloca(currentFunction_, objTy, "ctor.tmp");
                        // Store vtable pointer before constructor call
                        storeVtablePtr(thisPtr, classSym);
                        args.push_back(thisPtr);
                    }
                }
                // For class without constructor, just create undef
                if (!calleeFn) {
                    auto userType = registry_.getUserType(classSym->name,
                        Type::Kind::Class, classSym);
                    llvm::Type* objTy = mapType(userType);
                    auto* tmp = createEntryBlockAlloca(currentFunction_, objTy, "class.tmp");
                    storeVtablePtr(tmp, classSym);
                    lastValue_ = builder_.CreateLoad(objTy, tmp, "class.val");
                    return;
                }
            }
            // Check if it resolves to a StructSymbol (struct construction)
            else if (auto* structSym = ident->resolvedSymbol->as<StructSymbol>()) {
                auto userType = registry_.getUserType(structSym->name,
                    Type::Kind::Struct, structSym);
                // Use zero-init for structs with closure fields (prevents releasing garbage);
                // use undef for plain structs (enables optimizer freedom)
                bool hasClosureFields = false;
                for (auto* field : structSym->fields) {
                    if (field->type && field->type->is<FunctionType>()) {
                        hasClosureFields = true;
                        break;
                    }
                }
                if (hasClosureFields) {
                    lastValue_ = llvm::Constant::getNullValue(mapType(userType));
                } else {
                    lastValue_ = llvm::UndefValue::get(mapType(userType));
                }
                return;
            }
            // Regular function call
            else if (auto* funcSym = ident->resolvedSymbol->as<FunctionSymbol>()) {
                auto it = functionCache_.find(funcSym);
                if (it != functionCache_.end()) {
                    calleeFn = it->second;
                }
            }
        }

        // Indirect call through function pointer
        if (!calleeFn) {
            node.callee->accept(*this);
            calleeVal = lastValue_;
        }
    }
    else {
        // Other callee expression
        node.callee->accept(*this);
        calleeVal = lastValue_;
    }

    // Emit arguments
    for (auto& arg : node.arguments) {
        arg->accept(*this);
        if (lastValue_) {
            // Pass user types by pointer
            if (arg->resolvedType && isUserStructKind(arg->resolvedType.get())) {
                llvm::Value* argPtr = emitLValue(*arg);
                if (argPtr) {
                    args.push_back(argPtr);
                    continue;
                }
                // If emitLValue failed but we have a struct value, store in temp
                if (lastValue_->getType()->isStructTy()) {
                    auto* tmp = createEntryBlockAlloca(currentFunction_,
                        lastValue_->getType(), "arg.tmp");
                    builder_.CreateStore(lastValue_, tmp);
                    args.push_back(tmp);
                    continue;
                }
            }
            args.push_back(lastValue_);
        }
    }

    // Implicit integer widening for arguments (e.g. byte enum → int param)
    llvm::FunctionType* callFnTy = nullptr;
    if (calleeFn) {
        callFnTy = calleeFn->getFunctionType();
    } else if (isVirtualCall && virtualMethodSym) {
        callFnTy = buildFunctionType(virtualMethodSym);
    }
    if (callFnTy) {
        for (size_t i = 0; i < args.size() && i < callFnTy->getNumParams(); ++i) {
            auto* expectedTy = callFnTy->getParamType(i);
            auto* actualTy = args[i]->getType();
            if (expectedTy->isIntegerTy() && actualTy->isIntegerTy() &&
                expectedTy->getIntegerBitWidth() > actualTy->getIntegerBitWidth()) {
                args[i] = builder_.CreateSExt(args[i], expectedTy, "arg.widen");
            }
        }
    }

    // Emit the call
    if (isVirtualCall && thisPtr && virtualMethodSym && virtualCallClass) {
        // Virtual dispatch: load vtable pointer, GEP to slot, indirect call
        auto vtUserType = registry_.getUserType(virtualCallClass->name,
            Type::Kind::Class, virtualCallClass);
        auto* structTy = getStructType(vtUserType.get());
        auto* vtablePtrPtr = builder_.CreateStructGEP(structTy, thisPtr, 0, "vtable.ptr");
        auto* ptrTy = llvm::PointerType::getUnqual(context_);
        auto* vtable = builder_.CreateLoad(ptrTy, vtablePtrPtr, "vtable");
        auto* methodSlot = builder_.CreateGEP(ptrTy, vtable,
            builder_.getInt32(virtualMethodSym->vtableIndex), "method.slot");
        auto* methodFn = builder_.CreateLoad(ptrTy, methodSlot, "method.fn");
        auto* fnTy = buildFunctionType(virtualMethodSym);
        lastValue_ = builder_.CreateCall(fnTy, methodFn, args);
    } else if (calleeFn) {
        llvm::Value* callResult = builder_.CreateCall(calleeFn, args);
        if (isCtorCall && thisPtr) {
            // Constructor call — return the constructed object value
            auto* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(thisPtr);
            if (allocaInst) {
                lastValue_ = builder_.CreateLoad(
                    allocaInst->getAllocatedType(), thisPtr, "ctor.val");
            } else {
                lastValue_ = thisPtr;
            }
        } else {
            lastValue_ = callResult;
        }
    } else if (calleeVal) {
        // Indirect call through function-typed value
        if (calleeVal->getType()->isStructTy()) {
            // Fat pointer { fnPtr, envPtr } — extract both components
            llvm::Value* fnPtr = builder_.CreateExtractValue(calleeVal, {0}, "fn.ptr");
            llvm::Value* envPtr = builder_.CreateExtractValue(calleeVal, {1}, "env.ptr");

            if (auto* funcType = node.callee->resolvedType->as<FunctionType>()) {
                llvm::Type* retTy = mapType(funcType->returnType);
                std::vector<llvm::Type*> paramTypes;
                for (auto& pt : funcType->parameterTypes) {
                    paramTypes.push_back(mapType(pt));
                }
                // Add env pointer as last parameter (fat pointer calling convention)
                paramTypes.push_back(llvm::PointerType::getUnqual(context_));
                auto* fnTy = llvm::FunctionType::get(retTy, paramTypes, false);

                // Append envPtr as last argument
                args.push_back(envPtr);
                lastValue_ = builder_.CreateCall(fnTy, fnPtr, args);
            }
        } else if (llvm::dyn_cast<llvm::PointerType>(calleeVal->getType())) {
            // Bare function pointer fallback (legacy path)
            if (auto* funcType = node.callee->resolvedType->as<FunctionType>()) {
                llvm::Type* retTy = mapType(funcType->returnType);
                std::vector<llvm::Type*> paramTypes;
                for (auto& pt : funcType->parameterTypes) {
                    paramTypes.push_back(mapType(pt));
                }
                auto* fnTy = llvm::FunctionType::get(retTy, paramTypes, false);
                lastValue_ = builder_.CreateCall(fnTy, calleeVal, args);
            }
        }
    } else {
        lastValue_ = nullptr;
    }
}

void IRGenerator::visit(NewExpression& node) {
    const Type* allocType = node.type->resolvedType.get();
    if (!allocType) {
        lastValue_ = llvm::ConstantPointerNull::get(
            llvm::PointerType::getUnqual(context_));
        return;
    }

    if (node.isArray) {
        // Array allocation: new Type[size]
        llvm::Type* elemTy = mapType(allocType);
        node.arraySize->accept(*this);
        llvm::Value* sizeVal = lastValue_;

        // Calculate total bytes: size * sizeof(element)
        auto& dl = module_->getDataLayout();
        uint64_t elemSize = dl.getTypeAllocSize(elemTy);
        llvm::Value* elemSizeVal = llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(context_), elemSize);
        llvm::Value* totalBytes = builder_.CreateMul(sizeVal, elemSizeVal, "arr.bytes");

        // Call malloc
        auto mallocCallee = module_->getOrInsertFunction("malloc",
            llvm::FunctionType::get(llvm::PointerType::getUnqual(context_),
                {llvm::Type::getInt32Ty(context_)}, false));
        lastValue_ = builder_.CreateCall(mallocCallee, {totalBytes}, "arr.ptr");
    } else {
        // Object allocation: new Type(args)
        llvm::Type* objTy = mapType(allocType);
        auto& dl = module_->getDataLayout();
        uint64_t objSize = dl.getTypeAllocSize(objTy);
        llvm::Value* sizeVal = llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(context_), objSize);

        // Call malloc
        auto mallocCallee = module_->getOrInsertFunction("malloc",
            llvm::FunctionType::get(llvm::PointerType::getUnqual(context_),
                {llvm::Type::getInt32Ty(context_)}, false));
        llvm::Value* rawPtr = builder_.CreateCall(mallocCallee, {sizeVal}, "new.ptr");

        // If the type has a constructor, call it; store vtable pointer
        if (auto* userType = allocType->as<UserType>()) {
            auto* typeSym = static_cast<TypeSymbol*>(userType->symbol);
            auto* classSym = typeSym ? typeSym->as<ClassSymbol>() : nullptr;
            if (classSym) {
                if (classSym->constructor) {
                    // Constructor handles vtable pointer storage internally
                    auto it = functionCache_.find(classSym->constructor);
                    if (it != functionCache_.end()) {
                        std::vector<llvm::Value*> ctorArgs;
                        ctorArgs.push_back(rawPtr);  // this
                        for (auto& arg : node.arguments) {
                            arg->accept(*this);
                            if (lastValue_) {
                                if (arg->resolvedType && isUserStructKind(arg->resolvedType.get())) {
                                    llvm::Value* argPtr = emitLValue(*arg);
                                    if (argPtr) { ctorArgs.push_back(argPtr); continue; }
                                }
                                ctorArgs.push_back(lastValue_);
                            }
                        }
                        builder_.CreateCall(it->second, ctorArgs);
                    }
                } else {
                    // No constructor — store vtable pointer manually
                    storeVtablePtr(rawPtr, classSym);

                    // Zero-init closure-typed fields to prevent releasing garbage
                    // when destructor fires (same safety as constructor prologue)
                    auto userType = registry_.getUserType(classSym->name,
                        Type::Kind::Class, classSym);
                    auto* structTy = getStructType(userType.get());
                    auto* fatPtrTy = getFatPtrType();
                    for (auto* field : classSym->fields) {
                        if (field->type && field->type->is<FunctionType>()) {
                            int gepIdx = getFieldGEPIndex(classSym, field);
                            if (gepIdx >= 0) {
                                auto* fieldPtr = builder_.CreateStructGEP(
                                    structTy, rawPtr, gepIdx,
                                    field->name + ".new.init");
                                builder_.CreateStore(
                                    llvm::ConstantAggregateZero::get(fatPtrTy),
                                    fieldPtr);
                            }
                        }
                    }
                }
            }
        }

        lastValue_ = rawPtr;
    }
}

void IRGenerator::visit(IndexExpression& node) {
    // Operator overload
    if (node.isOperatorOverload && node.resolvedOperatorFunction) {
        auto it = functionCache_.find(node.resolvedOperatorFunction);
        if (it != functionCache_.end()) {
            llvm::Value* objPtr = emitLValue(*node.object);
            if (!objPtr) {
                node.object->accept(*this);
                objPtr = lastValue_;
                // If result is a struct value, store in temp alloca
                if (objPtr && objPtr->getType()->isStructTy()) {
                    auto* tmp = createEntryBlockAlloca(currentFunction_,
                        objPtr->getType(), "idx.obj.tmp");
                    builder_.CreateStore(objPtr, tmp);
                    objPtr = tmp;
                }
            }
            node.index->accept(*this);
            llvm::Value* idxVal = lastValue_;

            lastValue_ = builder_.CreateCall(it->second, {objPtr, idxVal});
            return;
        }
    }

    // String indexing: s[i] -> GEP + load i8
    {
        const Type* objType = node.object->resolvedType.get();
        if (isStringKind(objType)) {
            node.object->accept(*this);
            llvm::Value* strPtr = lastValue_;
            node.index->accept(*this);
            llvm::Value* idx = lastValue_;
            if (strPtr && idx) {
                auto i64Ty = llvm::Type::getInt64Ty(context_);
                if (idx->getType()->isIntegerTy(32)) {
                    idx = builder_.CreateSExt(idx, i64Ty, "idx.i64");
                }
                llvm::Value* charPtr = builder_.CreateGEP(
                    llvm::Type::getInt8Ty(context_), strPtr, idx, "str.idx");
                lastValue_ = builder_.CreateLoad(
                    llvm::Type::getInt8Ty(context_), charPtr, "char");
            } else {
                lastValue_ = nullptr;
            }
            return;
        }
    }

    // Regular array/pointer indexing
    llvm::Value* elemPtr = emitLValue(node);
    if (elemPtr) {
        llvm::Type* elemTy = mapType(node.resolvedType);
        lastValue_ = builder_.CreateLoad(elemTy, elemPtr, "elem");
    } else {
        lastValue_ = nullptr;
    }
}

void IRGenerator::visit(CastExpression& node) {
    node.operand->accept(*this);
    llvm::Value* val = lastValue_;
    if (!val) return;

    llvm::Type* targetTy = mapType(node.targetType->resolvedType);
    const Type* fromType = node.operand->resolvedType.get();
    const Type* toType = node.targetType->resolvedType.get();

    if (isIntegerKind(fromType) && isFloatingKind(toType)) {
        lastValue_ = builder_.CreateSIToFP(val, targetTy, "sitofp");
    } else if (isFloatingKind(fromType) && isIntegerKind(toType)) {
        lastValue_ = builder_.CreateFPToSI(val, targetTy, "fptosi");
    } else if (isFloatingKind(fromType) && isFloatingKind(toType)) {
        if (val->getType()->getPrimitiveSizeInBits() < targetTy->getPrimitiveSizeInBits()) {
            lastValue_ = builder_.CreateFPExt(val, targetTy, "fpext");
        } else {
            lastValue_ = builder_.CreateFPTrunc(val, targetTy, "fptrunc");
        }
    } else if (isIntegerKind(fromType) && isIntegerKind(toType)) {
        unsigned fromBits = val->getType()->getIntegerBitWidth();
        unsigned toBits = targetTy->getIntegerBitWidth();
        if (fromBits < toBits) {
            lastValue_ = builder_.CreateSExt(val, targetTy, "sext");
        } else if (fromBits > toBits) {
            lastValue_ = builder_.CreateTrunc(val, targetTy, "trunc");
        }
    } else if (isPointerKind(fromType) && isIntegerKind(toType)) {
        lastValue_ = builder_.CreatePtrToInt(val, targetTy, "ptrtoint");
    } else if (isIntegerKind(fromType) && isPointerKind(toType)) {
        lastValue_ = builder_.CreateIntToPtr(val, targetTy, "inttoptr");
    }
    // Pointer-to-pointer casts are no-ops with opaque pointers
}

void IRGenerator::visit(SizeOfExpression& node) {
    llvm::Type* ty = mapType(node.targetType->resolvedType);
    auto& dl = module_->getDataLayout();
    uint64_t size = dl.getTypeAllocSize(ty);
    lastValue_ = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context_), size);
}

void IRGenerator::visit(AlignOfExpression& node) {
    llvm::Type* ty = mapType(node.targetType->resolvedType);
    auto& dl = module_->getDataLayout();
    uint64_t align = dl.getABITypeAlign(ty).value();
    lastValue_ = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context_), align);
}

void IRGenerator::visit(PipeExpression& node) {
    // Emit input
    node.input->accept(*this);
    llvm::Value* currentVal = lastValue_;
    if (!currentVal) { lastValue_ = nullptr; return; }

    // For each pipe stage: prepend current value to args, call function
    for (auto& stage : node.stages) {
        // Resolve the function
        llvm::Function* calleeFn = nullptr;
        llvm::Value* calleeVal = nullptr;

        if (stage.function && stage.function->resolvedSymbol) {
            auto* sym = stage.function->resolvedSymbol;
            if (auto* funcSym = sym->as<FunctionSymbol>()) {
                auto it = functionCache_.find(funcSym);
                if (it != functionCache_.end()) {
                    calleeFn = it->second;
                }
            }
            // Could also be a variable holding a function pointer
            if (!calleeFn) {
                auto it = namedValues_.find(sym);
                if (it != namedValues_.end()) {
                    llvm::Type* loadTy = mapType(stage.function->resolvedType);
                    calleeVal = builder_.CreateLoad(loadTy, it->second, "pipe.fn");
                }
            }
        }

        // Build args: [currentVal] + extraArguments
        std::vector<llvm::Value*> args;
        args.push_back(currentVal);
        for (auto& extra : stage.extraArguments) {
            extra->accept(*this);
            if (lastValue_) args.push_back(lastValue_);
        }

        if (calleeFn) {
            currentVal = builder_.CreateCall(calleeFn, args, "pipe.result");
        } else if (calleeVal) {
            // Indirect call through function-typed value
            if (stage.function && stage.function->resolvedType) {
                if (auto* fnType = stage.function->resolvedType->as<FunctionType>()) {
                    llvm::Type* retTy = mapType(fnType->returnType);
                    std::vector<llvm::Type*> paramTypes;
                    for (auto& pt : fnType->parameterTypes) {
                        paramTypes.push_back(mapType(pt));
                    }

                    if (calleeVal->getType()->isStructTy()) {
                        // Fat pointer { fnPtr, envPtr }
                        llvm::Value* fnPtr = builder_.CreateExtractValue(calleeVal, {0}, "pipe.fn.ptr");
                        llvm::Value* envPtr = builder_.CreateExtractValue(calleeVal, {1}, "pipe.env.ptr");
                        paramTypes.push_back(llvm::PointerType::getUnqual(context_));
                        auto* fnTy = llvm::FunctionType::get(retTy, paramTypes, false);
                        args.push_back(envPtr);
                        currentVal = builder_.CreateCall(fnTy, fnPtr, args, "pipe.result");
                    } else {
                        // Bare function pointer fallback
                        auto* fnTy = llvm::FunctionType::get(retTy, paramTypes, false);
                        currentVal = builder_.CreateCall(fnTy, calleeVal, args, "pipe.result");
                    }
                }
            }
        }
    }

    lastValue_ = currentVal;
}

void IRGenerator::visit(MatchExpression& node) {
    // Emit subject
    node.subject->accept(*this);
    llvm::Value* subjectVal = lastValue_;
    if (!subjectVal) { lastValue_ = nullptr; return; }

    const Type* subjectType = node.subject->resolvedType.get();

    // Check if we can use a switch instruction (all literal/enum patterns, no guards)
    bool canUseSwitch = !node.arms.empty() && (isIntegerKind(subjectType) || isBoolKind(subjectType));
    bool hasWildcard = false;
    if (canUseSwitch) {
        for (auto& arm : node.arms) {
            if (arm.pattern->is<LiteralPattern>() ||
                (arm.pattern->is<WildcardPattern>() && !hasWildcard)) {
                if (arm.pattern->is<WildcardPattern>()) hasWildcard = true;
            } else if (arm.pattern->is<GuardedPattern>() ||
                       arm.pattern->is<BindingPattern>() ||
                       arm.pattern->is<RangePattern>()) {
                canUseSwitch = false;
                break;
            }
        }
    }

    auto* mergeBB = llvm::BasicBlock::Create(context_, "match.merge", currentFunction_);
    llvm::Type* resultTy = mapType(node.resolvedType);
    bool hasResult = resultTy && !resultTy->isVoidTy();

    // Collect (value, block) pairs for phi
    std::vector<std::pair<llvm::Value*, llvm::BasicBlock*>> phiIncoming;

    if (canUseSwitch && isIntegerKind(subjectType)) {
        // Optimized switch-based match
        auto* defaultBB = llvm::BasicBlock::Create(context_, "match.default", currentFunction_);
        auto* switchInst = builder_.CreateSwitch(subjectVal, defaultBB, (unsigned)node.arms.size());

        for (auto& arm : node.arms) {
            enterNextChildScope();
            if (auto* wildcard = arm.pattern->as<WildcardPattern>()) {
                // Wildcard becomes the default case
                builder_.SetInsertPoint(defaultBB);
                arm.body->accept(*this);
                llvm::Value* bodyVal = lastValue_;
                auto* bodyEndBB = builder_.GetInsertBlock();
                if (!bodyEndBB->getTerminator()) builder_.CreateBr(mergeBB);
                if (hasResult && bodyVal) phiIncoming.push_back({bodyVal, bodyEndBB});
                defaultBB = nullptr;  // Already handled
            } else if (auto* litPat = arm.pattern->as<LiteralPattern>()) {
                auto* armBB = llvm::BasicBlock::Create(context_, "match.arm", currentFunction_);
                litPat->value->accept(*this);
                if (auto* constInt = llvm::dyn_cast<llvm::ConstantInt>(lastValue_)) {
                    switchInst->addCase(constInt, armBB);
                }
                builder_.SetInsertPoint(armBB);
                arm.body->accept(*this);
                llvm::Value* bodyVal = lastValue_;
                auto* bodyEndBB = builder_.GetInsertBlock();
                if (!bodyEndBB->getTerminator()) builder_.CreateBr(mergeBB);
                if (hasResult && bodyVal) phiIncoming.push_back({bodyVal, bodyEndBB});
            }
            leaveChildScope();
        }

        // If no wildcard was provided, default just jumps to merge
        if (defaultBB) {
            builder_.SetInsertPoint(defaultBB);
            builder_.CreateBr(mergeBB);
            if (hasResult) {
                phiIncoming.push_back({llvm::UndefValue::get(resultTy), defaultBB});
            }
        }
    } else {
        // Chain of conditional branches
        for (size_t i = 0; i < node.arms.size(); i++) {
            auto& arm = node.arms[i];
            enterNextChildScope();
            auto* armBodyBB = llvm::BasicBlock::Create(context_, "arm.body", currentFunction_);
            auto* nextTestBB = (i + 1 < node.arms.size())
                ? llvm::BasicBlock::Create(context_, "arm.test", currentFunction_)
                : mergeBB;

            auto* pattern = arm.pattern.get();

            // Pattern matching
            if (auto* litPat = pattern->as<LiteralPattern>()) {
                litPat->value->accept(*this);
                llvm::Value* patVal = lastValue_;
                llvm::Value* cond;
                if (isFloatingKind(subjectType)) {
                    cond = builder_.CreateFCmpOEQ(subjectVal, patVal, "match.cmp");
                } else {
                    // Ensure same type
                    if (subjectVal->getType() != patVal->getType() &&
                        subjectVal->getType()->isIntegerTy() && patVal->getType()->isIntegerTy()) {
                        patVal = builder_.CreateSExtOrTrunc(patVal, subjectVal->getType());
                    }
                    cond = builder_.CreateICmpEQ(subjectVal, patVal, "match.cmp");
                }
                builder_.CreateCondBr(cond, armBodyBB, nextTestBB);

            } else if (pattern->is<WildcardPattern>()) {
                builder_.CreateBr(armBodyBB);

            } else if (auto* bindPat = pattern->as<BindingPattern>()) {
                // Alloca for binding, store subject
                llvm::Type* bindTy = mapType(subjectType);
                auto* bindAlloca = createEntryBlockAlloca(currentFunction_, bindTy, bindPat->name);
                builder_.CreateStore(subjectVal, bindAlloca);

                // Register under scope symbol
                auto* bindSym = currentScope_->lookupLocal(bindPat->name);
                if (bindSym) namedValues_[bindSym] = bindAlloca;

                // Also scan arm body for identifier references to this binding
                std::function<void(ASTNode*)> registerBinding = [&](ASTNode* n) {
                    if (!n) return;
                    if (auto* id = n->as<IdentifierExpression>()) {
                        if (id->name == bindPat->name && id->resolvedSymbol)
                            namedValues_[id->resolvedSymbol] = bindAlloca;
                        return;
                    }
                    if (auto* bin = n->as<BinaryExpression>()) {
                        registerBinding(bin->left.get()); registerBinding(bin->right.get());
                    } else if (auto* un = n->as<UnaryExpression>()) {
                        registerBinding(un->operand.get());
                    } else if (auto* call = n->as<CallExpression>()) {
                        registerBinding(call->callee.get());
                        for (auto& a : call->arguments) registerBinding(a.get());
                    } else if (auto* mem = n->as<MemberAccessExpression>()) {
                        registerBinding(mem->object.get());
                    } else if (auto* ret = n->as<ReturnStatement>()) {
                        if (ret->hasValue()) registerBinding(ret->value.get());
                    } else if (auto* blk = n->as<BlockStatement>()) {
                        for (auto& s : blk->statements) registerBinding(s.get());
                    } else if (auto* es = n->as<ExpressionStatement>()) {
                        registerBinding(es->expression.get());
                    }
                };
                registerBinding(arm.body.get());

                builder_.CreateBr(armBodyBB);

            } else if (auto* guardPat = pattern->as<GuardedPattern>()) {
                // First handle inner pattern (usually BindingPattern)
                if (auto* innerBind = guardPat->innerPattern->as<BindingPattern>()) {
                    llvm::Type* bindTy = mapType(subjectType);
                    auto* bindAlloca = createEntryBlockAlloca(currentFunction_, bindTy, innerBind->name);
                    builder_.CreateStore(subjectVal, bindAlloca);

                    // Register binding under scope symbol AND under any
                    // resolvedSymbol found in the guard/body that references it
                    auto* bindSym = currentScope_->lookupLocal(innerBind->name);
                    if (bindSym) namedValues_[bindSym] = bindAlloca;

                    // Scan guard + body for IdentifierExpressions matching the binding name
                    std::function<void(ASTNode*)> registerBinding = [&](ASTNode* n) {
                        if (!n) return;
                        if (auto* id = n->as<IdentifierExpression>()) {
                            if (id->name == innerBind->name && id->resolvedSymbol) {
                                namedValues_[id->resolvedSymbol] = bindAlloca;
                            }
                            return;
                        }
                        if (auto* bin = n->as<BinaryExpression>()) {
                            registerBinding(bin->left.get());
                            registerBinding(bin->right.get());
                        } else if (auto* un = n->as<UnaryExpression>()) {
                            registerBinding(un->operand.get());
                        } else if (auto* call = n->as<CallExpression>()) {
                            registerBinding(call->callee.get());
                            for (auto& a : call->arguments) registerBinding(a.get());
                        } else if (auto* mem = n->as<MemberAccessExpression>()) {
                            registerBinding(mem->object.get());
                        } else if (auto* idx = n->as<IndexExpression>()) {
                            registerBinding(idx->object.get());
                            registerBinding(idx->index.get());
                        } else if (auto* ret = n->as<ReturnStatement>()) {
                            if (ret->hasValue()) registerBinding(ret->value.get());
                        } else if (auto* blk = n->as<BlockStatement>()) {
                            for (auto& s : blk->statements) registerBinding(s.get());
                        } else if (auto* es = n->as<ExpressionStatement>()) {
                            registerBinding(es->expression.get());
                        } else if (auto* asgn = n->as<AssignmentExpression>()) {
                            registerBinding(asgn->target.get());
                            registerBinding(asgn->value.get());
                        } else if (auto* tern = n->as<TernaryExpression>()) {
                            registerBinding(tern->condition.get());
                            registerBinding(tern->thenExpr.get());
                            registerBinding(tern->elseExpr.get());
                        }
                    };
                    registerBinding(guardPat->guard.get());
                    registerBinding(arm.body.get());
                }

                // Evaluate guard
                guardPat->guard->accept(*this);
                llvm::Value* guardVal = lastValue_;
                if (!guardVal) {
                    builder_.CreateBr(nextTestBB);
                } else {
                    if (!guardVal->getType()->isIntegerTy(1)) {
                        guardVal = builder_.CreateICmpNE(guardVal,
                            llvm::ConstantInt::get(guardVal->getType(), 0), "guard");
                    }
                    builder_.CreateCondBr(guardVal, armBodyBB, nextTestBB);
                }

            } else if (auto* rangePat = pattern->as<RangePattern>()) {
                llvm::Value* lowVal = llvm::ConstantInt::get(subjectVal->getType(), rangePat->low);
                llvm::Value* highVal = llvm::ConstantInt::get(subjectVal->getType(), rangePat->high);
                llvm::Value* ge = builder_.CreateICmpSGE(subjectVal, lowVal, "range.ge");
                llvm::Value* le = builder_.CreateICmpSLE(subjectVal, highVal, "range.le");
                llvm::Value* inRange = builder_.CreateAnd(ge, le, "range.in");
                builder_.CreateCondBr(inRange, armBodyBB, nextTestBB);

            } else {
                // Fallback: unconditional
                builder_.CreateBr(armBodyBB);
            }

            // Emit arm body
            builder_.SetInsertPoint(armBodyBB);
            arm.body->accept(*this);
            llvm::Value* bodyVal = lastValue_;
            auto* bodyEndBB = builder_.GetInsertBlock();
            if (!bodyEndBB->getTerminator()) builder_.CreateBr(mergeBB);
            if (hasResult && bodyVal) phiIncoming.push_back({bodyVal, bodyEndBB});

            leaveChildScope();

            // Move to next test block
            if (nextTestBB != mergeBB) {
                builder_.SetInsertPoint(nextTestBB);
            }
        }
    }

    builder_.SetInsertPoint(mergeBB);
    if (hasResult && !phiIncoming.empty()) {
        auto* phi = builder_.CreatePHI(resultTy, (unsigned)phiIncoming.size(), "match.result");
        for (auto& [val, bb] : phiIncoming) {
            // Ensure type matches
            if (val->getType() == resultTy) {
                phi->addIncoming(val, bb);
            } else {
                phi->addIncoming(llvm::UndefValue::get(resultTy), bb);
            }
        }
        lastValue_ = phi;
    } else {
        lastValue_ = nullptr;
    }
}

void IRGenerator::visit(TupleExpression& node) {
    // Build the tuple type from resolved type
    llvm::Type* tupleTy = mapType(node.resolvedType);
    llvm::Value* tupleVal = llvm::UndefValue::get(tupleTy);

    for (size_t i = 0; i < node.elements.size(); i++) {
        node.elements[i]->accept(*this);
        if (lastValue_) {
            tupleVal = builder_.CreateInsertValue(tupleVal, lastValue_, {(unsigned)i}, "tup");
        }
    }

    lastValue_ = tupleVal;
}

void IRGenerator::visit(LambdaExpression& node) {
    bool hasCaptures = !node.capturedVariables.empty();

    // Determine lambda return type and param types from resolvedType
    llvm::Type* retTy = llvm::Type::getVoidTy(context_);
    std::vector<llvm::Type*> paramTypes;

    if (auto* fnType = node.resolvedType->as<FunctionType>()) {
        retTy = mapType(fnType->returnType);
        for (auto& pt : fnType->parameterTypes) {
            paramTypes.push_back(mapType(pt));
        }
    }

    // ALL lambdas get ptr %env as last parameter (fat pointer calling convention)
    // Non-capturing lambdas ignore it; capturing lambdas use it to access captures
    paramTypes.push_back(llvm::PointerType::getUnqual(context_));

    auto* fnTy = llvm::FunctionType::get(retTy, paramTypes, false);
    std::string lambdaName = "__lambda_" + std::to_string(lambdaCounter_++);
    auto* lambdaFn = llvm::Function::Create(fnTy,
        llvm::Function::InternalLinkage, lambdaName, module_.get());

    // Save current state
    auto* prevFunction = currentFunction_;
    auto* prevThisPtr = currentThisPtr_;
    auto savedNamedValues = namedValues_;
    auto savedInsertPoint = builder_.GetInsertBlock();
    auto savedInsertPointIt = builder_.GetInsertPoint();
    auto savedRAIIStack = std::move(raiiScopeStack_);
    raiiScopeStack_.clear();

    // Enter the lambda's scope
    enterNextChildScope();

    currentFunction_ = lambdaFn;
    currentThisPtr_ = nullptr;
    namedValues_.clear();

    auto* entryBB = llvm::BasicBlock::Create(context_, "entry", lambdaFn);
    builder_.SetInsertPoint(entryBB);

    // Set up parameters — we need to find the VariableSymbol* for each param
    // by matching names against identifiers in the lambda body
    unsigned argIdx = 0;
    std::unordered_map<std::string, llvm::Value*> paramAllocaByName;
    for (auto& param : node.parameters) {
        llvm::Value* argVal = lambdaFn->getArg(argIdx++);
        argVal->setName(param->name);
        auto* alloca = createEntryBlockAlloca(lambdaFn, argVal->getType(), param->name);
        builder_.CreateStore(argVal, alloca);
        paramAllocaByName[param->name] = alloca;
    }

    // Pre-populate namedValues_ by scanning the body for identifier references
    // that match parameter names — this resolves the param symbol mapping
    std::function<void(ASTNode*)> scanForParamSymbols = [&](ASTNode* n) {
        if (!n) return;
        if (auto* ident = n->as<IdentifierExpression>()) {
            auto pit = paramAllocaByName.find(ident->name);
            if (pit != paramAllocaByName.end() && ident->resolvedSymbol) {
                namedValues_[ident->resolvedSymbol] = pit->second;
            }
        }
        // Recurse into child nodes (common expression types)
        if (auto* bin = n->as<BinaryExpression>()) {
            scanForParamSymbols(bin->left.get());
            scanForParamSymbols(bin->right.get());
        } else if (auto* unary = n->as<UnaryExpression>()) {
            scanForParamSymbols(unary->operand.get());
        } else if (auto* call = n->as<CallExpression>()) {
            scanForParamSymbols(call->callee.get());
            for (auto& arg : call->arguments) scanForParamSymbols(arg.get());
        } else if (auto* ret = n->as<ReturnStatement>()) {
            if (ret->hasValue()) scanForParamSymbols(ret->value.get());
        } else if (auto* block = n->as<BlockStatement>()) {
            for (auto& stmt : block->statements) scanForParamSymbols(stmt.get());
        } else if (auto* exprStmt = n->as<ExpressionStatement>()) {
            scanForParamSymbols(exprStmt->expression.get());
        } else if (auto* assign = n->as<AssignmentExpression>()) {
            scanForParamSymbols(assign->target.get());
            scanForParamSymbols(assign->value.get());
        } else if (auto* ternary = n->as<TernaryExpression>()) {
            scanForParamSymbols(ternary->condition.get());
            scanForParamSymbols(ternary->thenExpr.get());
            scanForParamSymbols(ternary->elseExpr.get());
        } else if (auto* mem = n->as<MemberAccessExpression>()) {
            scanForParamSymbols(mem->object.get());
        } else if (auto* idx = n->as<IndexExpression>()) {
            scanForParamSymbols(idx->object.get());
            scanForParamSymbols(idx->index.get());
        }
    };
    scanForParamSymbols(node.body.get());

    // Extract captures from env pointer
    if (hasCaptures) {
        llvm::Value* envPtr = lambdaFn->getArg(lambdaFn->arg_size() - 1);
        envPtr->setName("env");

        // Build closure struct type WITH RC header (must match allocation layout)
        auto* i64TyCap = llvm::Type::getInt64Ty(context_);
        auto* ptrTyCap = llvm::PointerType::getUnqual(context_);

        std::vector<llvm::Type*> closureFields;
        closureFields.push_back(i64TyCap);   // refcount  (header field 0)
        closureFields.push_back(ptrTyCap);   // cleanup_fn (header field 1)
        for (auto* capSym : node.capturedVariables) {
            auto* varSym = capSym->as<VariableSymbol>();
            closureFields.push_back(varSym ? mapType(varSym->type)
                                           : llvm::Type::getInt32Ty(context_));
        }
        auto* closureTy = llvm::StructType::get(context_, closureFields);

        // Extract each capture from env pointer (offset by 2 for RC header)
        const int headerOffset = 2;
        for (size_t i = 0; i < node.capturedVariables.size(); i++) {
            auto* capSym = node.capturedVariables[i];
            auto* varSym = capSym->as<VariableSymbol>();
            llvm::Type* capType = varSym ? mapType(varSym->type)
                                         : llvm::Type::getInt32Ty(context_);
            auto* capPtr = builder_.CreateStructGEP(closureTy, envPtr,
                                                     headerOffset + (unsigned)i,
                                                     capSym->name + ".cap");
            auto* capVal = builder_.CreateLoad(capType, capPtr, capSym->name);
            auto* capAlloca = createEntryBlockAlloca(lambdaFn, capType, capSym->name);
            builder_.CreateStore(capVal, capAlloca);
            namedValues_[capSym] = capAlloca;
        }
    }

    // Emit body
    if (node.hasExpressionBody()) {
        auto exprBody = node.getExpressionBody();
        exprBody->accept(*this);
        if (lastValue_ && !retTy->isVoidTy()) {
            builder_.CreateRet(lastValue_);
        } else {
            builder_.CreateRetVoid();
        }
    } else if (node.hasBlockBody()) {
        auto blockBody = node.getBlockBody();
        blockBody->accept(*this);
        if (!builder_.GetInsertBlock()->getTerminator()) {
            if (retTy->isVoidTy()) {
                builder_.CreateRetVoid();
            } else {
                builder_.CreateRet(llvm::UndefValue::get(retTy));
            }
        }
    }

    // Restore state
    leaveChildScope();
    currentFunction_ = prevFunction;
    currentThisPtr_ = prevThisPtr;
    namedValues_ = savedNamedValues;
    raiiScopeStack_ = std::move(savedRAIIStack);
    builder_.SetInsertPoint(savedInsertPoint, savedInsertPointIt);

    if (hasCaptures) {
        // Build closure struct with RC header: { i64 refcount, ptr cleanup_fn, captures... }
        auto* i64TyA = llvm::Type::getInt64Ty(context_);
        auto* ptrTyA = llvm::PointerType::getUnqual(context_);

        std::vector<llvm::Type*> closureFields;
        closureFields.push_back(i64TyA);   // refcount  (header field 0)
        closureFields.push_back(ptrTyA);   // cleanup_fn (header field 1)
        for (auto* capSym : node.capturedVariables) {
            auto* varSym = capSym->as<VariableSymbol>();
            closureFields.push_back(varSym ? mapType(varSym->type)
                                           : llvm::Type::getInt32Ty(context_));
        }
        auto* closureTy = llvm::StructType::get(context_, closureFields);

        auto& dl = module_->getDataLayout();
        uint64_t closureSize = dl.getTypeAllocSize(closureTy);
        llvm::Value* sizeVal = llvm::ConstantInt::get(i64TyA, closureSize);

        auto mallocCallee = module_->getOrInsertFunction("malloc",
            llvm::FunctionType::get(ptrTyA, {i64TyA}, false));
        llvm::Value* closurePtr = builder_.CreateCall(mallocCallee, {sizeVal}, "closure.ptr");

        // Initialize refcount = 1
        auto* rcSlot = builder_.CreateStructGEP(closureTy, closurePtr, 0, "rc.slot");
        builder_.CreateStore(llvm::ConstantInt::get(i64TyA, 1), rcSlot);

        // Check if any captured variable is a closure (needs cleanup function)
        bool capturesClosures = false;
        for (auto* capSym : node.capturedVariables) {
            auto* varSym = capSym->as<VariableSymbol>();
            if (varSym && varSym->type && varSym->type->is<FunctionType>()) {
                capturesClosures = true;
                break;
            }
        }

        // Store cleanup function pointer (or null if no inner closures)
        auto* cleanupSlot = builder_.CreateStructGEP(closureTy, closurePtr, 1, "cleanup.slot");
        if (capturesClosures) {
            auto* cleanupFn = generateClosureCleanupFn(closureTy, node.capturedVariables, 2);
            builder_.CreateStore(cleanupFn, cleanupSlot);
        } else {
            builder_.CreateStore(llvm::ConstantPointerNull::get(ptrTyA), cleanupSlot);
        }

        // Store captured values (offset by 2 for the RC header)
        const int headerOffset = 2;
        for (size_t i = 0; i < node.capturedVariables.size(); i++) {
            auto* capSym = node.capturedVariables[i];
            auto* varSym = capSym->as<VariableSymbol>();
            auto it = savedNamedValues.find(capSym);
            if (it != savedNamedValues.end()) {
                llvm::Type* capTy = varSym ? mapType(varSym->type)
                                           : llvm::Type::getInt32Ty(context_);
                llvm::Value* capVal = builder_.CreateLoad(capTy, it->second, capSym->name + ".val");
                auto* capSlot = builder_.CreateStructGEP(closureTy, closurePtr,
                                                          headerOffset + (unsigned)i,
                                                          capSym->name + ".slot");
                builder_.CreateStore(capVal, capSlot);

                // If capturing a closure, retain its envPtr
                if (varSym && varSym->type && varSym->type->is<FunctionType>()) {
                    auto* capturedEnv = builder_.CreateExtractValue(capVal, {1},
                                                                    capSym->name + ".cap.env");
                    builder_.CreateCall(getOrCreateClosureRetainFn(), {capturedEnv});
                }
            }
        }

        // Build fat pointer { fnPtr, envPtr }
        auto* fatTy = getFatPtrType();
        llvm::Value* fat = llvm::UndefValue::get(fatTy);
        fat = builder_.CreateInsertValue(fat, lambdaFn, {0}, "fat.fn");
        fat = builder_.CreateInsertValue(fat, closurePtr, {1}, "fat.env");
        lastValue_ = fat;
    } else {
        // No captures — fat pointer with null env
        auto* fatTy = getFatPtrType();
        auto* nullPtr = llvm::ConstantPointerNull::get(
            llvm::PointerType::getUnqual(context_));
        llvm::Value* fat = llvm::UndefValue::get(fatTy);
        fat = builder_.CreateInsertValue(fat, lambdaFn, {0}, "fat.fn");
        fat = builder_.CreateInsertValue(fat, nullPtr, {1}, "fat.env");
        lastValue_ = fat;
    }
}

//================================================================================
// Pattern visitors (handled inline by MatchExpression)
//================================================================================
void IRGenerator::visit(LiteralPattern& /*node*/) {}
void IRGenerator::visit(RangePattern& /*node*/) {}
void IRGenerator::visit(WildcardPattern& /*node*/) {}
void IRGenerator::visit(BindingPattern& /*node*/) {}
void IRGenerator::visit(TuplePattern& /*node*/) {}
void IRGenerator::visit(GuardedPattern& /*node*/) {}

} // namespace codegen
} // namespace mingus
