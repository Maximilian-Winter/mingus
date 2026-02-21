//================================================================================
// MINGUS V2 - LLVM IR Generator Implementation
//
// Ported from V1 (~4400 lines) with key V2 improvements:
//   - Unified TypeSymbol* for type mapping (no separate Type hierarchy)
//   - Pre-resolved symbols on AST nodes (no scope navigation)
//   - ParameterNode::resolvedSymbol eliminates scanForParamSymbols
//   - mapParamType() handles struct/ref/interface params uniformly
//   - ArgumentsNode::isReference[] for per-arg ref param tracking
//================================================================================

#include "mingus/codegen/IRGenerator.h"

#pragma warning(push, 0)
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/TargetParser/Triple.h>
#include <llvm/Support/raw_ostream.h>
#pragma warning(pop)

#include <cassert>
#include <functional>
#include <stdexcept>

namespace mingus {
namespace codegen {

//================================================================================
// V2 type query helpers (unified TypeSymbol hierarchy)
//================================================================================
bool IRGenerator::isIntegerKind(TypeSymbol* t) {
    if (!t) return false;
    if (auto* p = t->as<PrimitiveTypeSymbol>()) {
        return p->primitiveKind == PrimitiveKind::Int ||
               p->primitiveKind == PrimitiveKind::Byte ||
               p->primitiveKind == PrimitiveKind::Char;
    }
    if (auto* e = t->as<EnumSymbol>()) {
        if (e->underlyingType) return isIntegerKind(e->underlyingType.get());
        return true;  // default int
    }
    return false;
}

bool IRGenerator::isFloatingKind(TypeSymbol* t) {
    if (!t) return false;
    if (auto* p = t->as<PrimitiveTypeSymbol>()) {
        return p->primitiveKind == PrimitiveKind::Double ||
               p->primitiveKind == PrimitiveKind::Float;
    }
    return false;
}

bool IRGenerator::isBoolKind(TypeSymbol* t) {
    if (!t) return false;
    if (auto* p = t->as<PrimitiveTypeSymbol>()) {
        return p->primitiveKind == PrimitiveKind::Bool;
    }
    return false;
}

bool IRGenerator::isStringKind(TypeSymbol* t) {
    if (!t) return false;
    if (auto* p = t->as<PrimitiveTypeSymbol>()) {
        return p->primitiveKind == PrimitiveKind::String;
    }
    return false;
}

bool IRGenerator::isPointerKind(TypeSymbol* t) {
    if (!t) return false;
    if (t->is<PointerTypeSymbol>()) return true;
    // String is a pointer (char*)
    if (auto* p = t->as<PrimitiveTypeSymbol>()) {
        return p->primitiveKind == PrimitiveKind::String;
    }
    return false;
}

bool IRGenerator::isUserStructKind(TypeSymbol* t) {
    if (!t) return false;
    return t->is<StructSymbol>() || t->is<ClassSymbol>();
}

bool IRGenerator::isEnumKind(TypeSymbol* t) {
    if (!t) return false;
    return t->is<EnumSymbol>();
}

bool IRGenerator::isFunctionKind(TypeSymbol* t) {
    if (!t) return false;
    return t->is<FunctionTypeSymbol>();
}

//================================================================================
// Constructor
//================================================================================
IRGenerator::IRGenerator(SymbolTable& symbolTable,
                         const std::unordered_map<Scope*, ScopeRAIIInfo>& raiiInfo)
    : builder_(context_)
    , symbolTable_(symbolTable)
    , raiiInfo_(raiiInfo)
{}

//================================================================================
// Entry point
//================================================================================
std::unique_ptr<llvm::Module> IRGenerator::generate(ProgramNode& program) {
    module_ = std::make_unique<llvm::Module>("mingus_module", context_);
    module_->setTargetTriple(llvm::Triple("x86_64-pc-windows-msvc"));

    // Phase A: Forward declarations
    declareStructTypes(program);
    declareExternFunctions(program);
    declareFunctions(program);
    declareVtables(program);
    declareItables(program);

    // Phase B: Function bodies (visitor-based)
    program.accept(*this);

    return std::move(module_);
}

//================================================================================
// Type Mapping — unified TypeSymbol* -> llvm::Type*
//================================================================================
llvm::Type* IRGenerator::mapType(const TypeSymbolPtr& type) {
    if (!type) return llvm::Type::getVoidTy(context_);
    return mapType(type.get());
}

llvm::StructType* IRGenerator::getFatPtrType() {
    auto* ptrTy = llvm::PointerType::getUnqual(context_);
    return llvm::StructType::get(context_, { ptrTy, ptrTy });
}

llvm::Type* IRGenerator::mapType(TypeSymbol* type) {
    if (!type) return llvm::Type::getVoidTy(context_);

    // Primitives
    if (auto* prim = type->as<PrimitiveTypeSymbol>()) {
        switch (prim->primitiveKind) {
            case PrimitiveKind::Int:    return llvm::Type::getInt32Ty(context_);
            case PrimitiveKind::Double: return llvm::Type::getDoubleTy(context_);
            case PrimitiveKind::Float:  return llvm::Type::getFloatTy(context_);
            case PrimitiveKind::Byte:   return llvm::Type::getInt8Ty(context_);
            case PrimitiveKind::Char:   return llvm::Type::getInt8Ty(context_);
            case PrimitiveKind::Bool:   return llvm::Type::getInt1Ty(context_);
            case PrimitiveKind::Void:   return llvm::Type::getVoidTy(context_);
            case PrimitiveKind::String: return llvm::PointerType::getUnqual(context_);
        }
    }

    // Pointer types
    if (auto* ptrType = type->as<PointerTypeSymbol>()) {
        // Interface pointer -> fat pointer { ptr, ptr }
        if (ptrType->baseType && ptrType->baseType->is<InterfaceSymbol>()) {
            return getFatPtrType();
        }
        return llvm::PointerType::getUnqual(context_);
    }

    // Array types
    if (auto* arr = type->as<ArrayTypeSymbol>()) {
        llvm::Type* elemTy = mapType(arr->elementType);
        if (arr->arraySize > 0) {
            return llvm::ArrayType::get(elemTy, arr->arraySize);
        } else {
            return llvm::PointerType::getUnqual(context_);
        }
    }

    // Tuple types
    if (auto* tup = type->as<TupleTypeSymbol>()) {
        std::vector<llvm::Type*> elements;
        for (const auto& et : tup->elementTypes) {
            elements.push_back(mapType(et));
        }
        return llvm::StructType::get(context_, elements);
    }

    // Enum types -> underlying type
    if (auto* enumSym = type->as<EnumSymbol>()) {
        if (enumSym->underlyingType) return mapType(enumSym->underlyingType);
        return llvm::Type::getInt32Ty(context_);
    }

    // Struct/Class types -> cached LLVM StructType
    if (type->is<StructSymbol>() || type->is<ClassSymbol>()) {
        auto sit = structTypeCache_.find(type);
        if (sit != structTypeCache_.end()) return sit->second;
        // Create opaque if not found (shouldn't happen after Phase A)
        auto* structTy = llvm::StructType::create(context_, type->getName());
        structTypeCache_[type] = structTy;
        return structTy;
    }

    // Function types -> fat pointer { fnPtr, envPtr }
    if (type->is<FunctionTypeSymbol>()) {
        return getFatPtrType();
    }

    // Reference types -> ptr
    if (type->is<ReferenceTypeSymbol>()) {
        return llvm::PointerType::getUnqual(context_);
    }

    // Error/Null types -> ptr
    if (type->is<ErrorTypeSymbol>() || type->is<NullTypeSymbol>()) {
        return llvm::PointerType::getUnqual(context_);
    }

    // Interface types -> fat pointer
    if (type->is<InterfaceSymbol>()) {
        return getFatPtrType();
    }

    // Fallback
    return llvm::Type::getInt32Ty(context_);
}

// V2 improvement: unified param type mapping (fixes 3 HIGH bugs)
llvm::Type* IRGenerator::mapParamType(TypeSymbol* type, bool isReference) {
    if (isReference) return llvm::PointerType::getUnqual(context_);
    if (!type) return llvm::Type::getInt32Ty(context_);
    if (type->is<ClassSymbol>() || type->is<StructSymbol>()) {
        return llvm::PointerType::getUnqual(context_);
    }
    if (type->is<InterfaceSymbol>()) return getFatPtrType();
    return mapType(type);
}

llvm::StructType* IRGenerator::getStructType(TypeSymbol* type) {
    auto it = structTypeCache_.find(type);
    if (it != structTypeCache_.end()) return it->second;
    return nullptr;
}

//================================================================================
// Name Mangling
//================================================================================
std::string IRGenerator::opToString(OverloadableOp op) {
    switch (op) {
        case OverloadableOp::Add:       return "add";
        case OverloadableOp::Sub:       return "sub";
        case OverloadableOp::Mul:       return "mul";
        case OverloadableOp::Div:       return "div";
        case OverloadableOp::Mod:       return "mod";
        case OverloadableOp::Index:     return "index";
        case OverloadableOp::Equal:     return "eq";
        case OverloadableOp::NotEqual:  return "ne";
        case OverloadableOp::Less:      return "lt";
        case OverloadableOp::Greater:   return "gt";
        case OverloadableOp::LessEq:    return "le";
        case OverloadableOp::GreaterEq: return "ge";
        case OverloadableOp::Negate:    return "neg";
    }
    return "unknown";
}

std::string IRGenerator::mangleName(Symbol* sym) {
    if (!sym) return "__unknown__";

    // Extern functions use their own name (no mangling)
    if (auto* func = sym->as<FunctionSymbol>()) {
        if (func->isExtern) return func->getName();
    }

    // Operator symbols: TypeName_operator_op
    if (auto* opSym = sym->as<OperatorSymbol>()) {
        if (opSym->ownerType) {
            return opSym->ownerType->getName() + "_operator_" + opToString(opSym->op);
        }
        return "operator_" + opToString(opSym->op);
    }

    // Constructor: ClassName_constructor
    if (sym->is<ConstructorSymbol>()) {
        auto scope = sym->getSymbolScope();
        if (scope) {
            // Walk up to find the class
            auto parentSym = scope->resolve(sym->getName());
            // Use the scope name
        }
        // Use qualified name from symbol
        if (auto* func = sym->as<FunctionSymbol>()) {
            return func->getQualifiedName();
        }
    }

    // Destructor: ClassName_destructor
    if (sym->is<DestructorSymbol>()) {
        if (auto* func = sym->as<FunctionSymbol>()) {
            return func->getQualifiedName();
        }
    }

    // Method: ClassName_methodName
    if (auto* func = sym->as<FunctionSymbol>()) {
        if (func->isMethod) {
            return func->getQualifiedName();
        }
        // Module-level function: ModuleName_funcName
        return func->getQualifiedName();
    }

    return sym->getName();
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

void IRGenerator::declareStructTypes(ProgramNode& program) {
    // First pass: create opaque struct types
    for (auto& mod : program.modules) {
        if (!mod->resolvedModule) continue;
        auto allSyms = mod->resolvedModule->getAllSymbols();
        for (auto& sym : allSyms) {
            if (auto* structSym = sym->as<StructSymbol>()) {
                declareStructTypeForSymbol(structSym);
            } else if (auto* classSym = sym->as<ClassSymbol>()) {
                declareClassTypeForSymbol(classSym);
            }
        }
    }

    // Second pass: set bodies
    for (auto& [typeSym, structTy] : structTypeCache_) {
        if (!structTy->isOpaque()) continue;

        std::vector<llvm::Type*> fieldTypes;

        if (auto* ss = typeSym->as<StructSymbol>()) {
            for (auto& field : ss->fields) {
                fieldTypes.push_back(mapType(field->getType()));
            }
        } else if (auto* cs = typeSym->as<ClassSymbol>()) {
            // Prepend vtable pointer if class has virtual methods
            if (cs->hasVtable()) {
                fieldTypes.push_back(llvm::PointerType::getUnqual(context_));
            }
            // Use allFields (inherited + own) for layout
            for (auto& field : cs->allFields) {
                fieldTypes.push_back(mapType(field->getType()));
            }
        }

        if (!fieldTypes.empty()) {
            structTy->setBody(fieldTypes);
        } else {
            structTy->setBody(llvm::Type::getInt8Ty(context_));
        }
    }
}

void IRGenerator::declareStructTypeForSymbol(StructSymbol* sym) {
    auto it = structTypeCache_.find(sym);
    if (it != structTypeCache_.end()) return;

    auto* structTy = llvm::StructType::create(context_, sym->getName());
    structTypeCache_[sym] = structTy;
}

void IRGenerator::declareClassTypeForSymbol(ClassSymbol* sym) {
    auto it = structTypeCache_.find(sym);
    if (it != structTypeCache_.end()) return;

    // Ensure base class is declared first
    if (sym->resolvedBaseClass) {
        declareClassTypeForSymbol(sym->resolvedBaseClass);
    }

    auto* structTy = llvm::StructType::create(context_, sym->getName());
    structTypeCache_[sym] = structTy;
}

void IRGenerator::declareVtables(ProgramNode& program) {
    for (auto& mod : program.modules) {
        if (!mod->resolvedModule) continue;
        auto allSyms = mod->resolvedModule->getAllSymbols();
        for (auto& sym : allSyms) {
            auto* classSym = sym->as<ClassSymbol>();
            if (!classSym || !classSym->hasVtable()) continue;
            if (classSym->isAbstract) continue;

            auto* ptrTy = llvm::PointerType::getUnqual(context_);
            auto* vtableArrayTy = llvm::ArrayType::get(ptrTy, classSym->vtableSize);

            std::vector<llvm::Constant*> vtableEntries;
            for (auto& methodSym : classSym->vtable) {
                if (methodSym && !methodSym->isAbstract) {
                    auto it = functionCache_.find(methodSym.get());
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
                vtableInit, classSym->getName() + "_vtable");
            vtableCache_[classSym] = vtableGlobal;
        }
    }
}

void IRGenerator::declareItables(ProgramNode& program) {
    auto* ptrTy = llvm::PointerType::getUnqual(context_);

    for (auto& mod : program.modules) {
        if (!mod->resolvedModule) continue;
        auto allSyms = mod->resolvedModule->getAllSymbols();
        for (auto& sym : allSyms) {
            auto* classSym = sym->as<ClassSymbol>();
            if (!classSym || classSym->isAbstract) continue;

            for (auto& ifaceSym : classSym->implementedInterfaces) {
                std::vector<llvm::Constant*> slots;
                for (auto& ifaceMethod : ifaceSym->methods) {
                    // Find the class's implementation of this interface method
                    auto implSym = classSym->resolve(ifaceMethod->getName());
                    auto* implFunc = implSym ? implSym->as<FunctionSymbol>() : nullptr;
                    if (implFunc) {
                        auto it = functionCache_.find(implFunc);
                        if (it != functionCache_.end()) {
                            slots.push_back(it->second);
                        } else {
                            slots.push_back(llvm::ConstantPointerNull::get(ptrTy));
                        }
                    } else {
                        slots.push_back(llvm::ConstantPointerNull::get(ptrTy));
                    }
                }

                if (slots.empty()) continue;

                auto* arrTy = llvm::ArrayType::get(ptrTy, slots.size());
                auto* init = llvm::ConstantArray::get(arrTy, slots);
                auto* global = new llvm::GlobalVariable(*module_, arrTy, true,
                    llvm::GlobalValue::InternalLinkage,
                    init,
                    classSym->getName() + "." + ifaceSym->getName() + ".itable");
                itableCache_[{classSym, ifaceSym.get()}] = global;
            }
        }
    }
}

llvm::Value* IRGenerator::emitWrapToInterfacePtr(llvm::Value* objPtr,
                                                   ClassSymbol* cls,
                                                   InterfaceSymbol* iface) {
    auto it = itableCache_.find({cls, iface});
    if (it == itableCache_.end()) return objPtr;

    auto* fatTy = getFatPtrType();
    llvm::Value* fat = llvm::UndefValue::get(fatTy);
    fat = builder_.CreateInsertValue(fat, objPtr, {0}, "fat.obj");
    fat = builder_.CreateInsertValue(fat, it->second, {1}, "fat.itable");
    return fat;
}

int IRGenerator::getFieldGEPIndex(ClassSymbol* cls, VariableSymbol* field) {
    int offset = cls->hasVtable() ? 1 : 0;
    for (size_t i = 0; i < cls->allFields.size(); ++i) {
        if (cls->allFields[i].get() == field) {
            return offset + static_cast<int>(i);
        }
    }
    return -1;
}

void IRGenerator::storeVtablePtr(llvm::Value* objPtr, ClassSymbol* cls) {
    if (!cls->hasVtable()) return;
    auto it = vtableCache_.find(cls);
    if (it == vtableCache_.end()) return;

    auto* structTy = getStructType(cls);
    if (!structTy) return;

    auto* vtableSlot = builder_.CreateStructGEP(structTy, objPtr, 0, "vtable.slot");
    builder_.CreateStore(it->second, vtableSlot);
}

void IRGenerator::declareExternFunctions(ProgramNode& program) {
    for (auto& mod : program.modules) {
        if (!mod->resolvedModule) continue;
        auto allSyms = mod->resolvedModule->getAllSymbols();
        for (auto& sym : allSyms) {
            auto* funcSym = sym->as<FunctionSymbol>();
            if (!funcSym || !funcSym->isExtern) continue;
            if (functionCache_.count(funcSym)) continue;

            auto* fnTy = buildFunctionType(funcSym);
            // Handle vararg functions (declared with ... in extern)
            bool isVarArg = funcSym->isVariadic;
            if (isVarArg) {
                std::vector<llvm::Type*> paramTypes;
                for (auto& param : funcSym->parameters) {
                    paramTypes.push_back(mapParamType(param->getType().get(), param->isReference));
                }
                fnTy = llvm::FunctionType::get(mapType(funcSym->returnType),
                                                paramTypes, true);
            }

            auto* fn = llvm::Function::Create(fnTy,
                llvm::Function::ExternalLinkage, funcSym->getName(), module_.get());
            functionCache_[funcSym] = fn;
        }
    }
}

void IRGenerator::declareFunctions(ProgramNode& program) {
    for (auto& mod : program.modules) {
        if (!mod->resolvedModule) continue;
        auto allSyms = mod->resolvedModule->getAllSymbols();
        for (auto& sym : allSyms) {
            // Module-level functions
            if (auto* funcSym = sym->as<FunctionSymbol>()) {
                if (!funcSym->isExtern && !funcSym->isAbstract) {
                    declareFunctionSymbol(funcSym);
                }
            }

            // Type members (methods, ctor, dtor, operators)
            if (auto* classSym = sym->as<ClassSymbol>()) {
                // Methods
                auto classSyms = classSym->getAllSymbols();
                for (auto& mSym : classSyms) {
                    if (auto* methodSym = mSym->as<FunctionSymbol>()) {
                        if (!methodSym->isExtern && !methodSym->isAbstract) {
                            declareFunctionSymbol(methodSym);
                        }
                    }
                }
                // Constructor
                if (classSym->constructor) {
                    declareFunctionSymbol(classSym->constructor.get());
                }
                // Destructor
                if (classSym->destructor) {
                    declareFunctionSymbol(classSym->destructor.get());
                }
                // Operators
                auto ops = classSym->getAllOperators();
                for (auto& opSym : ops) {
                    declareOperatorSymbol(opSym.get());
                }
            }

            if (auto* structSym = sym->as<StructSymbol>()) {
                auto structSyms = structSym->getAllSymbols();
                for (auto& mSym : structSyms) {
                    if (auto* methodSym = mSym->as<FunctionSymbol>()) {
                        if (!methodSym->isExtern && !methodSym->isAbstract) {
                            declareFunctionSymbol(methodSym);
                        }
                    }
                }
                auto ops = structSym->getAllOperators();
                for (auto& opSym : ops) {
                    declareOperatorSymbol(opSym.get());
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
    for (auto& param : sym->parameters) {
        if (idx < fn->arg_size()) {
            fn->getArg(idx)->setName(param->getName());
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
    for (auto& param : sym->parameters) {
        if (idx < fn->arg_size()) {
            fn->getArg(idx)->setName(param->getName());
            idx++;
        }
    }

    functionCache_[sym] = fn;
}

llvm::FunctionType* IRGenerator::buildFunctionType(FunctionSymbol* sym) {
    // Constructors and destructors always return void
    llvm::Type* retTy;
    if (sym->is<ConstructorSymbol>() || sym->is<DestructorSymbol>()) {
        retTy = llvm::Type::getVoidTy(context_);
    } else {
        retTy = mapType(sym->returnType);
    }

    std::vector<llvm::Type*> paramTypes;

    // Implicit 'this' parameter
    if (sym->hasThisParam) {
        paramTypes.push_back(llvm::PointerType::getUnqual(context_));
    }

    // Named parameters — V2 uses mapParamType for unified handling
    for (auto& param : sym->parameters) {
        paramTypes.push_back(mapParamType(param->getType().get(), param->isReference));
    }

    return llvm::FunctionType::get(retTy, paramTypes, false);
}

llvm::FunctionType* IRGenerator::buildOperatorType(OperatorSymbol* sym) {
    llvm::Type* retTy = mapType(sym->returnType);
    std::vector<llvm::Type*> paramTypes;

    // Always has 'this'
    paramTypes.push_back(llvm::PointerType::getUnqual(context_));

    for (auto& param : sym->parameters) {
        paramTypes.push_back(mapParamType(param->getType().get(), param->isReference));
    }

    return llvm::FunctionType::get(retTy, paramTypes, false);
}

//================================================================================
// LValue emission — returns a pointer, does NOT load
//================================================================================
llvm::Value* IRGenerator::emitLValue(ExpressionBaseNode& expr) {
    // IdentifierExpression -> alloca from namedValues_ (locals/params)
    if (auto* ident = expr.as<IdentifierExpression>()) {
        if (ident->resolvedSymbol) {
            auto it = namedValues_.find(ident->resolvedSymbol.get());
            if (it != namedValues_.end()) return it->second;

            // Field fallback: bare field name → GEP via this pointer
            if (auto* varSym = ident->resolvedSymbol->as<VariableSymbol>()) {
                llvm::Value* fieldPtr = emitFieldGEP(varSym);
                if (fieldPtr) return fieldPtr;
            }
        }
        return nullptr;
    }

    // MemberAccessExpression -> GEP to field
    if (auto* mem = expr.as<MemberAccessExpression>()) {
        llvm::Value* objPtr = nullptr;
        if (mem->isArrow) {
            mem->object->accept(*this);
            objPtr = lastValue_;
        } else {
            objPtr = emitLValue(*mem->object);
            if (!objPtr) {
                mem->object->accept(*this);
                objPtr = lastValue_;
                if (objPtr && objPtr->getType()->isStructTy()) {
                    auto* tmp = createEntryBlockAlloca(currentFunction_,
                        objPtr->getType(), "member.tmp");
                    builder_.CreateStore(objPtr, tmp);
                    objPtr = tmp;
                }
            }
        }
        if (!objPtr) return nullptr;

        // V2: resolve field from the resolved symbol
        TypeSymbol* objType = mem->object->resolvedType ?
            mem->object->resolvedType->as<TypeSymbol>() : nullptr;
        // Auto-dereference pointer types
        if (auto* ptrTS = objType ? objType->as<PointerTypeSymbol>() : nullptr) {
            objType = ptrTS->baseType ? ptrTS->baseType->as<TypeSymbol>() : nullptr;
        }
        if (!objType) return nullptr;

        auto* structTy = getStructType(objType);
        if (!structTy) return nullptr;

        // V2: use resolvedSymbol to find the field
        auto* fieldSym = mem->resolvedSymbol ? mem->resolvedSymbol->as<VariableSymbol>() : nullptr;
        if (fieldSym) {
            if (auto* classSym = objType->as<ClassSymbol>()) {
                int gepIdx = getFieldGEPIndex(classSym, fieldSym);
                if (gepIdx >= 0) {
                    return builder_.CreateStructGEP(structTy, objPtr, gepIdx,
                                                    mem->memberName + "_ptr");
                }
            } else {
                // Structs: direct field index
                if (fieldSym->fieldIndex >= 0) {
                    return builder_.CreateStructGEP(structTy, objPtr, fieldSym->fieldIndex,
                                                    mem->memberName + "_ptr");
                }
            }
        }
        return nullptr;
    }

    // UnaryExpression(Dereference) -> emit operand, return pointer value
    if (auto* unary = expr.as<UnaryExpression>()) {
        if (unary->op == UnaryOp::Dereference) {
            unary->operand->accept(*this);
            return lastValue_;
        }
    }

    // IndexExpression -> GEP to element
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

            TypeSymbol* objType = idx->object->resolvedType ?
                idx->object->resolvedType->as<TypeSymbol>() : nullptr;
            if (auto* arrType = objType ? objType->as<ArrayTypeSymbol>() : nullptr) {
                llvm::Type* elemTy = mapType(arrType->elementType);
                if (arrType->arraySize > 0) {
                    auto* llvmArrTy = llvm::ArrayType::get(elemTy, arrType->arraySize);
                    return builder_.CreateGEP(llvmArrTy, arrPtr,
                        { llvm::ConstantInt::get(llvm::Type::getInt32Ty(context_), 0), indexVal },
                        "elem_ptr");
                } else {
                    return builder_.CreateGEP(elemTy, arrPtr, indexVal, "elem_ptr");
                }
            }
            if (isPointerKind(objType)) {
                TypeSymbol* baseType = nullptr;
                if (auto* ptrTy = objType->as<PointerTypeSymbol>()) {
                    baseType = ptrTy->baseType ? ptrTy->baseType->as<TypeSymbol>() : nullptr;
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
// Field GEP helper — bare field access via this pointer
//================================================================================
llvm::Value* IRGenerator::emitFieldGEP(VariableSymbol* fieldSym) {
    if (!fieldSym || !currentThisPtr_) return nullptr;
    if (fieldSym->role != VariableRole::Field) return nullptr;

    // Class field: use getFieldGEPIndex (vtable + inheritance aware)
    if (currentClassSym_) {
        auto* structTy = getStructType(currentClassSym_);
        if (!structTy) return nullptr;
        int gepIdx = getFieldGEPIndex(currentClassSym_, fieldSym);
        if (gepIdx < 0) return nullptr;
        return builder_.CreateStructGEP(structTy, currentThisPtr_,
                                         gepIdx, fieldSym->getName() + "_ptr");
    }

    // Struct field: direct fieldIndex (no vtable)
    if (currentStructSym_) {
        auto* structTy = getStructType(currentStructSym_);
        if (!structTy) return nullptr;
        if (fieldSym->fieldIndex < 0) return nullptr;
        return builder_.CreateStructGEP(structTy, currentThisPtr_,
                                         fieldSym->fieldIndex, fieldSym->getName() + "_ptr");
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
    for (auto it = scope.destructibles.rbegin(); it != scope.destructibles.rend(); ++it) {
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
    auto* isNull = b.CreateICmpEQ(env, llvm::ConstantPointerNull::get(ptrTy), "is_null");
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
    auto* isNull = b.CreateICmpEQ(env, llvm::ConstantPointerNull::get(ptrTy), "is_null");
    b.CreateCondBr(isNull, done, doRelease);

    b.SetInsertPoint(doRelease);
    auto* headerTy = llvm::StructType::get(context_, {i64Ty, ptrTy});
    auto* rcPtr = b.CreateStructGEP(headerTy, env, 0, "rc_ptr");
    auto* rc = b.CreateLoad(i64Ty, rcPtr, "rc");
    auto* rcDec = b.CreateSub(rc, llvm::ConstantInt::get(i64Ty, 1), "rc_dec");
    b.CreateStore(rcDec, rcPtr);
    auto* isZero = b.CreateICmpEQ(rcDec, llvm::ConstantInt::get(i64Ty, 0), "is_zero");
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
    const std::vector<SymbolPtr>& capturedVars,
    int headerOffset,
    const std::vector<CaptureMode>* captureModes)
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
        if (captureModes && i < captureModes->size() &&
            (*captureModes)[i] == CaptureMode::ByReference) {
            continue;
        }

        auto* varSym = capturedVars[i]->as<VariableSymbol>();
        if (!varSym) continue;

        if (varSym->getType() && varSym->getType()->is<FunctionTypeSymbol>()) {
            unsigned fieldIdx = headerOffset + (unsigned)i;
            auto* fieldPtr = b.CreateStructGEP(closureTy, env, fieldIdx,
                                                varSym->getName() + ".cleanup.slot");
            auto* fatVal = b.CreateLoad(fatPtrTy, fieldPtr, varSym->getName() + ".fat");
            auto* envPtr = b.CreateExtractValue(fatVal, {1}, varSym->getName() + ".env");
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

    auto* fnTy = llvm::FunctionType::get(voidTy, {ptrTy}, false);
    closureReleaseWrapperFn_ = llvm::Function::Create(
        fnTy, llvm::Function::InternalLinkage,
        "__mingus_closure_release_wrapper", module_.get());

    auto* entry = llvm::BasicBlock::Create(context_, "entry", closureReleaseWrapperFn_);
    llvm::IRBuilder<> b(entry);
    auto* allocaPtr = closureReleaseWrapperFn_->getArg(0);

    auto* fatPtrTy = getFatPtrType();
    auto* fatVal = b.CreateLoad(fatPtrTy, allocaPtr, "fat");
    auto* envPtr = b.CreateExtractValue(fatVal, {1}, "env.ptr");
    b.CreateCall(getOrCreateClosureReleaseFn(), {envPtr});

    b.CreateRetVoid();
    return closureReleaseWrapperFn_;
}

llvm::Function* IRGenerator::getOrCreateStructCleanupFn(StructSymbol* structSym) {
    auto it = structCleanupCache_.find(structSym->getName());
    if (it != structCleanupCache_.end()) return it->second;

    auto* ptrTy = llvm::PointerType::getUnqual(context_);
    auto* voidTy = llvm::Type::getVoidTy(context_);
    auto* fnTy = llvm::FunctionType::get(voidTy, {ptrTy}, false);

    std::string name = "__struct_cleanup_" + structSym->getName();
    auto* fn = llvm::Function::Create(fnTy, llvm::Function::InternalLinkage,
                                       name, module_.get());

    auto* entry = llvm::BasicBlock::Create(context_, "entry", fn);
    llvm::IRBuilder<> b(entry);
    auto* structPtr = fn->getArg(0);

    auto* structTy = getStructType(structSym);
    auto* fatPtrTy = getFatPtrType();

    for (auto& field : structSym->fields) {
        if (field->getType() && field->getType()->is<FunctionTypeSymbol>()) {
            auto* fieldPtr = b.CreateStructGEP(structTy, structPtr,
                                                field->fieldIndex,
                                                field->getName() + ".cleanup");
            auto* fatVal = b.CreateLoad(fatPtrTy, fieldPtr, field->getName() + ".fat");
            auto* envPtr = b.CreateExtractValue(fatVal, {1}, field->getName() + ".env");
            b.CreateCall(getOrCreateClosureReleaseFn(), {envPtr});
        }
    }

    b.CreateRetVoid();
    structCleanupCache_[structSym->getName()] = fn;
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

    auto strlenCallee = module_->getOrInsertFunction("strlen",
        llvm::FunctionType::get(i64Ty, {ptrTy}, false));
    auto strcpyCallee = module_->getOrInsertFunction("strcpy",
        llvm::FunctionType::get(ptrTy, {ptrTy, ptrTy}, false));
    auto strcatCallee = module_->getOrInsertFunction("strcat",
        llvm::FunctionType::get(ptrTy, {ptrTy, ptrTy}, false));
    auto mallocCallee = module_->getOrInsertFunction("malloc",
        llvm::FunctionType::get(ptrTy, {i64Ty}, false));

    llvm::Value* len1 = builder_.CreateCall(strlenCallee, {left}, "len1");
    llvm::Value* len2 = builder_.CreateCall(strlenCallee, {right}, "len2");
    llvm::Value* sum = builder_.CreateAdd(len1, len2, "sum");
    llvm::Value* total = builder_.CreateAdd(sum, llvm::ConstantInt::get(i64Ty, 1), "total");
    llvm::Value* buf = builder_.CreateCall(mallocCallee, {total}, "str.buf");
    builder_.CreateCall(strcpyCallee, {buf, left});
    builder_.CreateCall(strcatCallee, {buf, right});
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
    for (auto& decl : node.declarations) {
        decl->accept(*this);
    }
    currentModuleName_.clear();
}

void IRGenerator::visit(ImportDeclaration& /*node*/) {}
void IRGenerator::visit(TypedefDeclaration& /*node*/) {}

//================================================================================
// Declaration visitors
//================================================================================

void IRGenerator::visit(FunctionDeclaration& node) {
    auto* funcSym = node.resolvedFunction.get();
    if (!funcSym) return;

    auto it = functionCache_.find(funcSym);
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
    if (funcSym->hasThisParam) {
        currentThisPtr_ = fn->getArg(argIdx++);
    } else {
        currentThisPtr_ = nullptr;
    }

    // V2: use ParameterNode::resolvedSymbol directly (no scanForParamSymbols!)
    for (size_t i = 0; i < node.parameters.size(); i++) {
        auto& paramNode = node.parameters[i];
        auto* paramSym = paramNode->resolvedSymbol.get();
        llvm::Value* argVal = fn->getArg(argIdx++);

        if (paramSym && paramSym->isReference) {
            argVal->setName(paramSym->getName() + ".ref");
            namedValues_[paramSym] = argVal;
        } else if (paramSym && isUserStructKind(paramSym->getType().get())) {
            llvm::Type* structTy = mapType(paramSym->getType());
            auto* alloca = createEntryBlockAlloca(fn, structTy, paramSym->getName());
            auto* val = builder_.CreateLoad(structTy, argVal, paramSym->getName() + ".val");
            builder_.CreateStore(val, alloca);
            namedValues_[paramSym] = alloca;
        } else {
            llvm::Type* paramTy = argVal->getType();
            std::string pName = paramSym ? paramSym->getName() : paramNode->name;
            auto* alloca = createEntryBlockAlloca(fn, paramTy, pName);
            builder_.CreateStore(argVal, alloca);
            if (paramSym) namedValues_[paramSym] = alloca;
        }
    }

    pushRAIIScope();

    for (auto& stmt : node.body->statements) {
        stmt->accept(*this);
        if (builder_.GetInsertBlock()->getTerminator()) break;
    }

    if (!builder_.GetInsertBlock()->getTerminator()) {
        emitScopeDestructors();
        if (fn->getReturnType()->isVoidTy()) {
            builder_.CreateRetVoid();
        } else {
            builder_.CreateRet(llvm::UndefValue::get(fn->getReturnType()));
        }
    }

    popRAIIScope();

    namedValues_ = savedNamedValues;
    currentFunction_ = prevFunction;
    currentThisPtr_ = prevThisPtr;
}

void IRGenerator::visit(ConstructorDeclaration& node) {
    if (!currentClassSym_ || !currentClassSym_->constructor) return;

    auto* ctorSym = currentClassSym_->constructor.get();
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

    unsigned argIdx = 0;
    currentThisPtr_ = fn->getArg(argIdx++);

    for (size_t i = 0; i < node.parameters.size(); i++) {
        auto& paramNode = node.parameters[i];
        auto* paramSym = paramNode->resolvedSymbol.get();
        llvm::Value* argVal = fn->getArg(argIdx++);

        if (paramSym && paramSym->isReference) {
            argVal->setName(paramSym->getName() + ".ref");
            namedValues_[paramSym] = argVal;
        } else if (paramSym && isUserStructKind(paramSym->getType().get())) {
            llvm::Type* structTy = mapType(paramSym->getType());
            auto* alloca = createEntryBlockAlloca(fn, structTy, paramSym->getName());
            auto* val = builder_.CreateLoad(structTy, argVal, paramSym->getName() + ".val");
            builder_.CreateStore(val, alloca);
            namedValues_[paramSym] = alloca;
        } else {
            llvm::Type* paramTy = argVal->getType();
            std::string pName = paramSym ? paramSym->getName() : ("p" + std::to_string(i));
            auto* alloca = createEntryBlockAlloca(fn, paramTy, pName);
            builder_.CreateStore(argVal, alloca);
            if (paramSym) namedValues_[paramSym] = alloca;
        }
    }

    // Super constructor call
    if (node.hasSuperCall && currentClassSym_->resolvedBaseClass &&
        currentClassSym_->resolvedBaseClass->constructor) {
        auto baseCtorIt = functionCache_.find(
            currentClassSym_->resolvedBaseClass->constructor.get());
        if (baseCtorIt != functionCache_.end()) {
            std::vector<llvm::Value*> superArgs;
            superArgs.push_back(currentThisPtr_);
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

    // Store vtable pointer
    storeVtablePtr(currentThisPtr_, currentClassSym_);

    // Zero-init closure-typed fields
    {
        auto* structTy = getStructType(currentClassSym_);
        auto* fatPtrTy = getFatPtrType();
        for (auto& field : currentClassSym_->fields) {
            if (field->getType() && field->getType()->is<FunctionTypeSymbol>()) {
                int gepIdx = getFieldGEPIndex(currentClassSym_, field.get());
                if (gepIdx >= 0) {
                    auto* fieldPtr = builder_.CreateStructGEP(structTy, currentThisPtr_,
                                                              gepIdx, field->getName() + ".init");
                    builder_.CreateStore(llvm::ConstantAggregateZero::get(fatPtrTy), fieldPtr);
                }
            }
        }
    }

    for (auto& stmt : node.body->statements) {
        stmt->accept(*this);
        if (builder_.GetInsertBlock()->getTerminator()) break;
    }

    if (!builder_.GetInsertBlock()->getTerminator()) {
        builder_.CreateRetVoid();
    }

    namedValues_ = savedNamedValues;
    currentFunction_ = prevFunction;
    currentThisPtr_ = prevThisPtr;
}

void IRGenerator::visit(DestructorDeclaration& node) {
    if (!currentClassSym_ || !currentClassSym_->destructor) return;

    auto* dtorSym = currentClassSym_->destructor.get();
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

    for (auto& stmt : node.body->statements) {
        stmt->accept(*this);
        if (builder_.GetInsertBlock()->getTerminator()) break;
    }

    // Release closure-typed fields
    if (!builder_.GetInsertBlock()->getTerminator()) {
        auto* structTy = getStructType(currentClassSym_);
        auto* fatPtrTy = getFatPtrType();

        for (auto& field : currentClassSym_->fields) {
            if (field->getType() && field->getType()->is<FunctionTypeSymbol>()) {
                int gepIdx = getFieldGEPIndex(currentClassSym_, field.get());
                if (gepIdx >= 0) {
                    auto* fieldPtr = builder_.CreateStructGEP(structTy, currentThisPtr_,
                                                              gepIdx, field->getName() + ".dtor.ptr");
                    auto* fatVal = builder_.CreateLoad(fatPtrTy, fieldPtr, field->getName() + ".dtor.fat");
                    auto* envPtr = builder_.CreateExtractValue(fatVal, {1}, field->getName() + ".dtor.env");
                    builder_.CreateCall(getOrCreateClosureReleaseFn(), {envPtr});
                }
            }
        }
    }

    // Chain to base destructor
    if (!builder_.GetInsertBlock()->getTerminator()) {
        if (currentClassSym_->resolvedBaseClass && currentClassSym_->resolvedBaseClass->destructor) {
            auto baseDtorIt = functionCache_.find(
                currentClassSym_->resolvedBaseClass->destructor.get());
            if (baseDtorIt != functionCache_.end()) {
                builder_.CreateCall(baseDtorIt->second, {currentThisPtr_});
            }
        }
        builder_.CreateRetVoid();
    }

    namedValues_ = savedNamedValues;
    currentFunction_ = prevFunction;
    currentThisPtr_ = prevThisPtr;
}

void IRGenerator::visit(OperatorDeclaration& node) {
    // V2: resolvedOperator gives us the symbol directly
    auto* opSym = node.resolvedOperator.get();
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

    for (size_t i = 0; i < node.parameters.size(); i++) {
        auto& paramNode = node.parameters[i];
        auto* paramSym = paramNode->resolvedSymbol.get();
        llvm::Value* argVal = fn->getArg(argIdx++);

        if (paramSym && paramSym->isReference) {
            argVal->setName(paramSym->getName() + ".ref");
            namedValues_[paramSym] = argVal;
        } else if (paramSym && isUserStructKind(paramSym->getType().get())) {
            llvm::Type* structTy = mapType(paramSym->getType());
            auto* alloca = createEntryBlockAlloca(fn, structTy, paramSym->getName());
            auto* val = builder_.CreateLoad(structTy, argVal, paramSym->getName() + ".val");
            builder_.CreateStore(val, alloca);
            namedValues_[paramSym] = alloca;
        } else {
            llvm::Type* paramTy = argVal->getType();
            std::string pName = paramSym ? paramSym->getName() : ("p" + std::to_string(i));
            auto* alloca = createEntryBlockAlloca(fn, paramTy, pName);
            builder_.CreateStore(argVal, alloca);
            if (paramSym) namedValues_[paramSym] = alloca;
        }
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

    namedValues_ = savedNamedValues;
    currentFunction_ = prevFunction;
    currentThisPtr_ = prevThisPtr;
}

void IRGenerator::visit(ExternFunctionDeclaration& /*node*/) {}
void IRGenerator::visit(EnumMemberNode& /*node*/) {}
void IRGenerator::visit(EnumDeclaration& /*node*/) {}

void IRGenerator::visit(StructDeclaration& node) {
    auto* prevStructSym = currentStructSym_;
    currentStructSym_ = node.resolvedStruct.get();

    for (auto& method : node.methods) {
        method->accept(*this);
    }
    for (auto& op : node.operators) {
        op->accept(*this);
    }

    currentStructSym_ = prevStructSym;
}

void IRGenerator::visit(ClassDeclaration& node) {
    auto* prevClassSym = currentClassSym_;
    currentClassSym_ = node.resolvedClass.get();

    if (node.constructor) {
        node.constructor->accept(*this);
    }
    if (node.destructor) {
        node.destructor->accept(*this);
    }
    for (auto& method : node.methods) {
        method->accept(*this);
    }
    for (auto& op : node.operators) {
        op->accept(*this);
    }

    currentClassSym_ = prevClassSym;
}

void IRGenerator::visit(InterfaceDeclaration& /*node*/) {}

//================================================================================
// Statement visitors
//================================================================================
void IRGenerator::visit(BlockStatementNode& node) {
    pushRAIIScope();

    for (auto& stmt : node.statements) {
        stmt->accept(*this);
        if (builder_.GetInsertBlock()->getTerminator()) break;
    }

    if (!builder_.GetInsertBlock()->getTerminator()) {
        emitScopeDestructors();
    }
    popRAIIScope();
}

void IRGenerator::visit(ExpressionStatement& node) {
    node.expression->accept(*this);
}

void IRGenerator::visit(ReturnStatement& node) {
    if (node.value) {
        // Mark returned RAII variable for suppression
        if (auto* ident = node.value->as<IdentifierExpression>()) {
            if (ident->resolvedSymbol && !raiiScopeStack_.empty()) {
                for (auto& scope : raiiScopeStack_) {
                    for (auto& [ptr, dtor] : scope.destructibles) {
                        auto it = namedValues_.find(ident->resolvedSymbol.get());
                        if (it != namedValues_.end() && it->second == ptr) {
                            scope.returnedVars.insert(ident->resolvedSymbol.get());
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
    node.condition->accept(*this);
    llvm::Value* condVal = lastValue_;
    if (!condVal) condVal = llvm::ConstantInt::getFalse(context_);
    if (!condVal->getType()->isIntegerTy(1)) {
        condVal = builder_.CreateICmpNE(condVal,
            llvm::ConstantInt::get(condVal->getType(), 0), "ifcond");
    }

    auto* thenBB = llvm::BasicBlock::Create(context_, "then", currentFunction_);
    auto* mergeBB = llvm::BasicBlock::Create(context_, "ifmerge", currentFunction_);

    if (node.elseBody || !node.elseIfClauses.empty()) {
        auto* elseBB = llvm::BasicBlock::Create(context_, "else", currentFunction_);
        builder_.CreateCondBr(condVal, thenBB, elseBB);

        builder_.SetInsertPoint(thenBB);
        node.thenBody->accept(*this);
        if (!builder_.GetInsertBlock()->getTerminator())
            builder_.CreateBr(mergeBB);

        builder_.SetInsertPoint(elseBB);
        if (!node.elseIfClauses.empty()) {
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
        if (node.elseBody) {
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
    node.subject->accept(*this);
    llvm::Value* subjectVal = lastValue_;
    if (!subjectVal) return;

    auto* mergeBB = llvm::BasicBlock::Create(context_, "switch.merge", currentFunction_);
    auto* defaultBB = !node.defaultCase.empty()
        ? llvm::BasicBlock::Create(context_, "switch.default", currentFunction_)
        : mergeBB;

    if (subjectVal->getType()->isIntegerTy()) {
        auto* switchInst = builder_.CreateSwitch(subjectVal, defaultBB,
                                                  (unsigned)node.cases.size());
        for (auto& sc : node.cases) {
            if (!sc.value) continue;
            auto* caseBB = llvm::BasicBlock::Create(context_, "switch.case", currentFunction_);
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
        for (size_t i = 0; i < node.cases.size(); i++) {
            auto& sc = node.cases[i];
            if (!sc.value) continue;
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

    if (!node.defaultCase.empty()) {
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
    for (auto& initDecl : node.initDeclarations) {
        if (initDecl) initDecl->accept(*this);
    }
    for (auto& expr : node.initExpressions) {
        expr->accept(*this);
    }

    auto* condBB = llvm::BasicBlock::Create(context_, "for.cond", currentFunction_);
    auto* bodyBB = llvm::BasicBlock::Create(context_, "for.body", currentFunction_);
    auto* iterBB = llvm::BasicBlock::Create(context_, "for.iter", currentFunction_);
    auto* exitBB = llvm::BasicBlock::Create(context_, "for.exit", currentFunction_);

    builder_.CreateBr(condBB);

    builder_.SetInsertPoint(condBB);
    if (node.condition) {
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

    builder_.SetInsertPoint(iterBB);
    for (auto& iter : node.iterators) {
        iter->accept(*this);
    }
    builder_.CreateBr(condBB);

    builder_.SetInsertPoint(exitBB);
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

void IRGenerator::visit(DoWhileStatement& node) {
    auto* bodyBB = llvm::BasicBlock::Create(context_, "dowhile.body", currentFunction_);
    auto* condBB = llvm::BasicBlock::Create(context_, "dowhile.cond", currentFunction_);
    auto* exitBB = llvm::BasicBlock::Create(context_, "dowhile.exit", currentFunction_);

    // Body executes first — the essence of do-while
    builder_.CreateBr(bodyBB);

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

    // Condition evaluated after body
    builder_.SetInsertPoint(condBB);
    node.condition->accept(*this);
    llvm::Value* condVal = lastValue_;
    if (!condVal) condVal = llvm::ConstantInt::getFalse(context_);
    if (!condVal->getType()->isIntegerTy(1)) {
        condVal = builder_.CreateICmpNE(condVal,
            llvm::ConstantInt::get(condVal->getType(), 0), "dowhilecond");
    }
    builder_.CreateCondBr(condVal, bodyBB, exitBB);

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

    TypeSymbol* targetType = node.target->resolvedType ?
        node.target->resolvedType->as<TypeSymbol>() : nullptr;

    if (auto* ptrTS = targetType ? targetType->as<PointerTypeSymbol>() : nullptr) {
        TypeSymbol* pointee = ptrTS->baseType ? ptrTS->baseType->as<TypeSymbol>() : nullptr;

        if (pointee && pointee->is<InterfaceSymbol>()) {
            // Interface fat pointer — free the object pointer
            ptrVal = builder_.CreateExtractValue(ptrVal, {0}, "iface.del.obj");
        } else if (auto* classSym = pointee ? pointee->as<ClassSymbol>() : nullptr) {
            if (classSym->destructor) {
                if (classSym->hasVtable() && classSym->destructor->vtableIndex >= 0) {
                    // Virtual destructor dispatch
                    auto* structTy = getStructType(classSym);
                    auto* ptrTy = llvm::PointerType::getUnqual(context_);
                    auto* vtablePtrPtr = builder_.CreateStructGEP(structTy, ptrVal, 0, "del.vtable.ptr");
                    auto* vtable = builder_.CreateLoad(ptrTy, vtablePtrPtr, "del.vtable");
                    auto* dtorSlot = builder_.CreateGEP(ptrTy, vtable,
                        builder_.getInt32(0), "del.dtor.slot");
                    auto* dtorFn = builder_.CreateLoad(ptrTy, dtorSlot, "del.dtor.fn");
                    auto* dtorFnTy = llvm::FunctionType::get(
                        llvm::Type::getVoidTy(context_), {ptrTy}, false);
                    builder_.CreateCall(dtorFnTy, dtorFn, {ptrVal});
                } else {
                    auto dtorIt = functionCache_.find(classSym->destructor.get());
                    if (dtorIt != functionCache_.end()) {
                        builder_.CreateCall(dtorIt->second, {ptrVal});
                    }
                }
            }
        }
    }

    auto freeCallee = module_->getOrInsertFunction("free",
        llvm::FunctionType::get(llvm::Type::getVoidTy(context_),
            {llvm::PointerType::getUnqual(context_)}, false));
    builder_.CreateCall(freeCallee, {ptrVal});
}

//================================================================================
// Variable declaration
//================================================================================
void IRGenerator::visit(VariableDeclaration& node) {
    auto* varSym = node.resolvedVariable.get();
    if (!varSym) return;

    llvm::Type* varTy = mapType(varSym->getType());
    if (varTy->isVoidTy()) return;

    auto* alloca = createEntryBlockAlloca(currentFunction_, varTy, node.name);
    namedValues_[varSym] = alloca;

    // Zero-init closure allocas
    if (varSym->getType() && varSym->getType()->is<FunctionTypeSymbol>()) {
        builder_.CreateStore(llvm::ConstantAggregateZero::get(varTy), alloca);
    }

    // Zero-init ALL structs (prevents undef propagation — universal zero-init invariant)
    if (varSym->getType() && (varSym->getType()->is<StructSymbol>() ||
                               varSym->getType()->is<ClassSymbol>())) {
        builder_.CreateStore(llvm::Constant::getNullValue(varTy), alloca);
    }

    // Emit initializer
    if (node.initializer) {
        node.initializer->accept(*this);
        if (lastValue_) {
            // Wrap class* -> interface* if needed
            if (varSym->getType() && node.initializer->resolvedType) {
                auto* dstType = varSym->getType().get();
                auto* srcType = node.initializer->resolvedType.get();
                if (auto* dstPtr = dstType->as<PointerTypeSymbol>()) {
                    if (auto* dstIface = dstPtr->baseType ?
                        dstPtr->baseType->as<InterfaceSymbol>() : nullptr) {
                        if (auto* srcPtr = srcType->as<PointerTypeSymbol>()) {
                            if (auto* srcClass = srcPtr->baseType ?
                                srcPtr->baseType->as<ClassSymbol>() : nullptr) {
                                lastValue_ = emitWrapToInterfacePtr(lastValue_, srcClass, dstIface);
                            }
                        }
                    }
                }
            }

            // Convert null -> zero fat pointer for FunctionType variables
            if (varSym->getType() && varSym->getType()->is<FunctionTypeSymbol>() &&
                llvm::isa<llvm::ConstantPointerNull>(lastValue_)) {
                lastValue_ = llvm::ConstantAggregateZero::get(getFatPtrType());
            }

            builder_.CreateStore(lastValue_, alloca);

            // Self-capturing closure: patch env struct
            if (auto* lambda = node.initializer->as<LambdaExpression>()) {
                if (lambda->selfCapture && !lambda->capturedVariables.empty()) {
                    int selfCaptureIdx = -1;
                    for (size_t i = 0; i < lambda->capturedVariables.size(); i++) {
                        if (lambda->capturedVariables[i].get() == varSym) {
                            selfCaptureIdx = static_cast<int>(i);
                            break;
                        }
                    }
                    if (selfCaptureIdx >= 0) {
                        auto* envPtr = builder_.CreateExtractValue(lastValue_, {1}, "self.env");
                        auto* i64Ty = llvm::Type::getInt64Ty(context_);
                        auto* ptrTy = llvm::PointerType::getUnqual(context_);
                        std::vector<llvm::Type*> closureFields;
                        closureFields.push_back(i64Ty);
                        closureFields.push_back(ptrTy);
                        for (auto& capSym : lambda->capturedVariables) {
                            auto* capVar = capSym->as<VariableSymbol>();
                            closureFields.push_back(capVar ? mapType(capVar->getType())
                                                           : llvm::Type::getInt32Ty(context_));
                        }
                        auto* closureTy = llvm::StructType::get(context_, closureFields);
                        const int headerOffset = 2;
                        auto* selfSlot = builder_.CreateStructGEP(closureTy, envPtr,
                            headerOffset + selfCaptureIdx, "self.capture.slot");
                        builder_.CreateStore(lastValue_, selfSlot);
                    }
                }
            }
        }
    }

    // Register RAII for class types with destructors
    if (varSym->getType() && varSym->getType()->is<ClassSymbol>()) {
        auto* classSym = varSym->getType()->as<ClassSymbol>();
        if (classSym && classSym->hasRAII()) {
            auto dtorIt = functionCache_.find(classSym->destructor.get());
            if (dtorIt != functionCache_.end()) {
                registerRAII(alloca, dtorIt->second);
            }
        }
    }

    // Register RAII for structs with closure-typed fields
    if (varSym->getType() && varSym->getType()->is<StructSymbol>()) {
        auto* structSym = varSym->getType()->as<StructSymbol>();
        if (structSym && structSym->needsCleanup()) {
            registerRAII(alloca, getOrCreateStructCleanupFn(structSym));
        }
    }

    // Register RAII for closure-typed variables
    if (varSym->getType() && varSym->getType()->is<FunctionTypeSymbol>()) {
        registerRAII(alloca, getOrCreateClosureReleaseWrapper());
    }
}

void IRGenerator::visit(TupleDestructuringDeclaration& node) {
    node.initializer->accept(*this);
    llvm::Value* tupleVal = lastValue_;
    if (!tupleVal) return;

    for (size_t i = 0; i < node.elements.size(); i++) {
        llvm::Value* elemVal = builder_.CreateExtractValue(tupleVal, {(unsigned)i},
                                                           node.elements[i].name);
        if (i < node.resolvedVariables.size() && node.resolvedVariables[i]) {
            auto* varSym = node.resolvedVariables[i].get();
            auto* alloca = createEntryBlockAlloca(currentFunction_,
                                                   elemVal->getType(), node.elements[i].name);
            builder_.CreateStore(elemVal, alloca);
            namedValues_[varSym] = alloca;
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
    auto it = stringConstants_.find(node.value);
    if (it != stringConstants_.end()) {
        lastValue_ = it->second;
        return;
    }
    lastValue_ = builder_.CreateGlobalStringPtr(node.value, "str");
}

void IRGenerator::visit(InterpolatedStringExpression& node) {
    std::string formatStr;
    std::vector<llvm::Value*> args;

    for (auto& part : node.parts) {
        if (part.kind == InterpolatedPartKind::Text) {
            formatStr += part.text;
        } else {
            part.expression->accept(*this);
            llvm::Value* val = lastValue_;
            if (!val) continue;

            TypeSymbol* exprType = part.expression->resolvedType ?
                part.expression->resolvedType->as<TypeSymbol>() : nullptr;
            if (isFloatingKind(exprType)) {
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

    auto ptrTy = llvm::PointerType::getUnqual(context_);
    auto i32Ty = llvm::Type::getInt32Ty(context_);
    auto i64Ty = llvm::Type::getInt64Ty(context_);

    llvm::Value* fmtStr = builder_.CreateGlobalStringPtr(formatStr, "fmt");

    auto snprintfCallee = module_->getOrInsertFunction("snprintf",
        llvm::FunctionType::get(i32Ty, {ptrTy, i32Ty, ptrTy}, true));

    std::vector<llvm::Value*> sizeArgs;
    sizeArgs.push_back(llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(context_)));
    sizeArgs.push_back(llvm::ConstantInt::get(i32Ty, 0));
    sizeArgs.push_back(fmtStr);
    for (auto* arg : args) sizeArgs.push_back(arg);
    llvm::Value* needed = builder_.CreateCall(snprintfCallee, sizeArgs, "snprintf.len");

    llvm::Value* needed64 = builder_.CreateSExt(needed, i64Ty, "needed.i64");
    llvm::Value* allocSize = builder_.CreateAdd(needed64,
        llvm::ConstantInt::get(i64Ty, 1), "alloc.size");
    auto mallocCallee = module_->getOrInsertFunction("malloc",
        llvm::FunctionType::get(ptrTy, {i64Ty}, false));
    llvm::Value* buf = builder_.CreateCall(mallocCallee, {allocSize}, "interp.buf");

    llvm::Value* bufSize = builder_.CreateAdd(needed,
        llvm::ConstantInt::get(i32Ty, 1), "buf.size");
    std::vector<llvm::Value*> fmtArgs;
    fmtArgs.push_back(buf);
    fmtArgs.push_back(bufSize);
    fmtArgs.push_back(fmtStr);
    for (auto* arg : args) fmtArgs.push_back(arg);
    builder_.CreateCall(snprintfCallee, fmtArgs);

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

    // Function symbol -> return function pointer
    if (node.resolvedSymbol->is<FunctionSymbol>()) {
        auto it = functionCache_.find(node.resolvedSymbol.get());
        if (it != functionCache_.end()) {
            lastValue_ = it->second;
            return;
        }
    }

    // Variable symbol -> load from alloca (locals and params)
    auto it = namedValues_.find(node.resolvedSymbol.get());
    if (it != namedValues_.end()) {
        llvm::Type* loadTy = mapType(node.resolvedType);
        if (loadTy->isVoidTy()) {
            lastValue_ = nullptr;
            return;
        }
        lastValue_ = builder_.CreateLoad(loadTy, it->second, node.name);
        return;
    }

    // Field fallback: bare field name resolved by sema → access via this pointer
    if (auto* varSym = node.resolvedSymbol->as<VariableSymbol>()) {
        llvm::Value* fieldPtr = emitFieldGEP(varSym);
        if (fieldPtr) {
            // Closure-typed field: load as fat pointer
            if (varSym->getType() && varSym->getType()->is<FunctionTypeSymbol>()) {
                lastValue_ = builder_.CreateLoad(getFatPtrType(), fieldPtr, node.name);
                return;
            }
            llvm::Type* loadTy = mapType(node.resolvedType);
            if (loadTy->isVoidTy()) { lastValue_ = nullptr; return; }
            lastValue_ = builder_.CreateLoad(loadTy, fieldPtr, node.name);
            return;
        }
    }

    lastValue_ = nullptr;
}

void IRGenerator::visit(QualifiedNameExpression& node) {
    // Enum member access (resolved by TypeChecker)
    if (node.isEnumAccess) {
        // String-backed enum → global string pointer
        if (node.isStringEnumAccess) {
            lastValue_ = builder_.CreateGlobalStringPtr(
                node.resolvedEnumStringValue, "enum.str");
            return;
        }
        // Integer-backed enum → constant int with underlying type
        llvm::Type* enumTy = llvm::Type::getInt32Ty(context_);
        if (node.resolvedSymbol) {
            if (auto* enumSym = node.resolvedSymbol->as<EnumSymbol>()) {
                if (enumSym->underlyingType) {
                    enumTy = mapType(enumSym->underlyingType);
                }
            }
        }
        lastValue_ = llvm::ConstantInt::get(enumTy, node.resolvedEnumValue);
        return;
    }

    // Fallback: try scope-based resolution
    if (node.resolvedSymbol) {
        if (auto* enumSym = node.resolvedSymbol->as<EnumSymbol>()) {
            if (node.parts.size() >= 2) {
                const auto& memberName = node.parts.back();
                auto* member = enumSym->findMember(memberName);
                if (member) {
                    llvm::Type* enumTy = enumSym->underlyingType
                        ? mapType(enumSym->underlyingType)
                        : llvm::Type::getInt32Ty(context_);
                    lastValue_ = llvm::ConstantInt::get(enumTy, member->intValue);
                    return;
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
    // Static access: no runtime value until called
    if (node.isStaticAccess) {
        lastValue_ = nullptr;
        return;
    }

    // Enum member access
    if (node.isEnumAccess) {
        if (node.isStringEnumAccess) {
            lastValue_ = builder_.CreateGlobalStringPtr(
                node.resolvedEnumStringValue, "enum.str");
        } else {
            llvm::Type* enumTy = llvm::Type::getInt32Ty(context_);
            if (node.resolvedType) {
                if (auto* enumSym = node.resolvedType->as<EnumSymbol>()) {
                    if (enumSym->underlyingType) {
                        enumTy = mapType(enumSym->underlyingType);
                    }
                }
            }
            lastValue_ = llvm::ConstantInt::get(enumTy, node.resolvedEnumValue);
        }
        return;
    }

    // Get lvalue
    llvm::Value* fieldPtr = emitLValue(node);
    if (!fieldPtr) {
        lastValue_ = nullptr;
        return;
    }

    // Check if this is a closure-typed field (must load fat pointer, not pass through)
    if (node.resolvedType && node.resolvedType->is<FunctionTypeSymbol>()) {
        auto* fieldSym = node.resolvedSymbol ? node.resolvedSymbol->as<VariableSymbol>() : nullptr;
        if (fieldSym && fieldSym->getType() && fieldSym->getType()->is<FunctionTypeSymbol>()) {
            lastValue_ = builder_.CreateLoad(getFatPtrType(), fieldPtr, node.memberName);
            return;
        }
        // Method reference — pass through
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
    // Operator overload
    if (node.isOperatorOverload && node.resolvedOperatorFunction) {
        auto it = functionCache_.find(node.resolvedOperatorFunction.get());
        if (it != functionCache_.end()) {
            llvm::Value* lhsPtr = emitLValue(*node.left);
            if (!lhsPtr) {
                node.left->accept(*this);
                lhsPtr = lastValue_;
                if (lhsPtr && lhsPtr->getType()->isStructTy()) {
                    auto* tmp = createEntryBlockAlloca(currentFunction_,
                        lhsPtr->getType(), "op.lhs.tmp");
                    builder_.CreateStore(lhsPtr, tmp);
                    lhsPtr = tmp;
                }
            }
            node.right->accept(*this);
            llvm::Value* rhsArg = lastValue_;

            if (node.right->resolvedType && isUserStructKind(node.right->resolvedType.get())) {
                llvm::Value* rhsPtr = emitLValue(*node.right);
                if (rhsPtr) {
                    rhsArg = rhsPtr;
                } else if (rhsArg && rhsArg->getType()->isStructTy()) {
                    auto* tmp = createEntryBlockAlloca(currentFunction_,
                        rhsArg->getType(), "op.rhs.tmp");
                    builder_.CreateStore(rhsArg, tmp);
                    rhsArg = tmp;
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

    TypeSymbol* leftType = node.left->resolvedType ?
        node.left->resolvedType->as<TypeSymbol>() : nullptr;
    TypeSymbol* rightType = node.right->resolvedType ?
        node.right->resolvedType->as<TypeSymbol>() : nullptr;

    // String concatenation
    if (isStringKind(leftType) && isStringKind(rightType) && node.op == BinaryOp::Add) {
        lastValue_ = emitStringConcat(leftVal, rightVal);
        return;
    }

    // Pointer arithmetic
    if (isPointerKind(leftType) && isIntegerKind(rightType) &&
        (node.op == BinaryOp::Add || node.op == BinaryOp::Sub)) {
        llvm::Type* elemTy = llvm::Type::getInt8Ty(context_);
        if (auto* ptrTy = leftType->as<PointerTypeSymbol>()) {
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

    // String comparison via strcmp
    if (isStringKind(leftType) && isStringKind(rightType)) {
        if (node.op == BinaryOp::Equal || node.op == BinaryOp::NotEqual) {
            auto ptrTy = llvm::PointerType::getUnqual(context_);
            auto i32Ty = llvm::Type::getInt32Ty(context_);
            auto strcmpCallee = module_->getOrInsertFunction("strcmp",
                llvm::FunctionType::get(i32Ty, {ptrTy, ptrTy}, false));
            llvm::Value* cmp = builder_.CreateCall(strcmpCallee, {leftVal, rightVal}, "strcmp");
            if (node.op == BinaryOp::Equal) {
                lastValue_ = builder_.CreateICmpEQ(cmp, llvm::ConstantInt::get(i32Ty, 0), "str.eq");
            } else {
                lastValue_ = builder_.CreateICmpNE(cmp, llvm::ConstantInt::get(i32Ty, 0), "str.ne");
            }
            return;
        }
    }

    // Fat pointer null comparison
    if ((isFunctionKind(leftType) || isFunctionKind(rightType)) &&
        (node.op == BinaryOp::Equal || node.op == BinaryOp::NotEqual)) {
        auto* ptrTy = llvm::PointerType::getUnqual(context_);
        auto* nullPtr = llvm::ConstantPointerNull::get(ptrTy);
        llvm::Value* lhsFnPtr = isFunctionKind(leftType)
            ? builder_.CreateExtractValue(leftVal, {0}, "lhs.fnptr") : nullPtr;
        llvm::Value* rhsFnPtr = isFunctionKind(rightType)
            ? builder_.CreateExtractValue(rightVal, {0}, "rhs.fnptr") : nullPtr;
        if (node.op == BinaryOp::Equal) {
            lastValue_ = builder_.CreateICmpEQ(lhsFnPtr, rhsFnPtr, "fatptr.eq");
        } else {
            lastValue_ = builder_.CreateICmpNE(lhsFnPtr, rhsFnPtr, "fatptr.ne");
        }
        return;
    }

    // Pointer comparison
    if (isPointerKind(leftType) || isPointerKind(rightType)) {
        if (node.op == BinaryOp::Equal) {
            lastValue_ = builder_.CreateICmpEQ(leftVal, rightVal, "ptr.eq"); return;
        }
        if (node.op == BinaryOp::NotEqual) {
            lastValue_ = builder_.CreateICmpNE(leftVal, rightVal, "ptr.ne"); return;
        }
    }

    // Type widening
    bool leftIsFloat = isFloatingKind(leftType);
    bool rightIsFloat = isFloatingKind(rightType);
    bool leftIsInt = isIntegerKind(leftType);
    bool rightIsInt = isIntegerKind(rightType);

    if (leftIsInt && rightIsFloat) {
        leftVal = builder_.CreateSIToFP(leftVal, rightVal->getType(), "widen");
        leftIsFloat = true; leftIsInt = false;
    } else if (leftIsFloat && rightIsInt) {
        rightVal = builder_.CreateSIToFP(rightVal, leftVal->getType(), "widen");
        rightIsFloat = true; rightIsInt = false;
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

    // Bool comparison
    if (isBoolKind(leftType) && isBoolKind(rightType)) {
        if (node.op == BinaryOp::Equal) { lastValue_ = builder_.CreateICmpEQ(leftVal, rightVal, "eq"); return; }
        if (node.op == BinaryOp::NotEqual) { lastValue_ = builder_.CreateICmpNE(leftVal, rightVal, "ne"); return; }
    }

    lastValue_ = nullptr;
}

void IRGenerator::visit(UnaryExpression& node) {
    TypeSymbol* operandType = node.operand->resolvedType ?
        node.operand->resolvedType->as<TypeSymbol>() : nullptr;

    if (node.op == UnaryOp::AddressOf) {
        lastValue_ = emitLValue(*node.operand);
        return;
    }

    if (node.op == UnaryOp::PreIncrement || node.op == UnaryOp::PreDecrement ||
        node.op == UnaryOp::PostIncrement || node.op == UnaryOp::PostDecrement) {
        llvm::Value* ptr = emitLValue(*node.operand);
        if (!ptr) { lastValue_ = nullptr; return; }

        llvm::Type* valTy = mapType(operandType);
        llvm::Value* oldVal = builder_.CreateLoad(valTy, ptr, "old");
        llvm::Value* one = isFloatingKind(operandType)
            ? (llvm::Value*)llvm::ConstantFP::get(valTy, 1.0)
            : (llvm::Value*)llvm::ConstantInt::get(valTy, 1);

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
                val = builder_.CreateICmpNE(val, llvm::ConstantInt::get(val->getType(), 0));
            }
            lastValue_ = builder_.CreateXor(val, llvm::ConstantInt::getTrue(context_), "not");
            break;
        case UnaryOp::BitwiseNot:
            lastValue_ = builder_.CreateNot(val, "bitnot");
            break;
        case UnaryOp::Dereference: {
            llvm::Type* pointeeTy = llvm::Type::getInt8Ty(context_);
            if (auto* ptrTy = operandType ? operandType->as<PointerTypeSymbol>() : nullptr) {
                pointeeTy = mapType(ptrTy->baseType);
            }
            if (!pointeeTy->isVoidTy()) {
                lastValue_ = builder_.CreateLoad(pointeeTy, val, "deref");
            }
            break;
        }
        default: break;
    }
}

void IRGenerator::visit(AssignmentExpression& node) {
    llvm::Value* targetPtr = emitLValue(*node.target);
    if (!targetPtr) { lastValue_ = nullptr; return; }

    TypeSymbol* targetType = node.target->resolvedType ?
        node.target->resolvedType->as<TypeSymbol>() : nullptr;

    if (node.op == AssignOp::Assign) {
        // Release old closure envPtr before overwriting
        if (isFunctionKind(targetType)) {
            auto* oldFat = builder_.CreateLoad(getFatPtrType(), targetPtr, "old.fat");
            auto* oldEnv = builder_.CreateExtractValue(oldFat, {1}, "old.env");
            builder_.CreateCall(getOrCreateClosureReleaseFn(), {oldEnv});
        }

        node.value->accept(*this);
        if (lastValue_) {
            // Convert null -> zero fat pointer
            if (isFunctionKind(targetType) && llvm::isa<llvm::ConstantPointerNull>(lastValue_)) {
                lastValue_ = llvm::ConstantAggregateZero::get(getFatPtrType());
            }

            builder_.CreateStore(lastValue_, targetPtr);

            // Retain new closure envPtr when storing into a field
            if (isFunctionKind(targetType)) {
                bool isFieldStore = (node.target->as<MemberAccessExpression>() != nullptr);
                if (!isFieldStore) {
                    if (auto* ident = node.target->as<IdentifierExpression>()) {
                        if (auto* vs = ident->resolvedSymbol ?
                                ident->resolvedSymbol->as<VariableSymbol>() : nullptr) {
                            isFieldStore = (vs->role == VariableRole::Field);
                        }
                    }
                }
                if (isFieldStore) {
                    auto* newEnv = builder_.CreateExtractValue(lastValue_, {1}, "new.env");
                    builder_.CreateCall(getOrCreateClosureRetainFn(), {newEnv});
                }
            }
        }
    } else {
        // Compound assignment
        llvm::Type* valTy = mapType(targetType);
        llvm::Value* oldVal = builder_.CreateLoad(valTy, targetPtr, "old");

        node.value->accept(*this);
        llvm::Value* rhsVal = lastValue_;
        if (!rhsVal) return;

        bool isFloat = isFloatingKind(targetType);
        if (isFloat && rhsVal->getType()->isIntegerTy()) {
            rhsVal = builder_.CreateSIToFP(rhsVal, oldVal->getType(), "widen");
        }

        llvm::Value* result = nullptr;
        switch (node.op) {
            case AssignOp::AddAssign:
                if (isStringKind(targetType)) { result = emitStringConcat(oldVal, rhsVal); break; }
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
    FunctionSymbol* calleeFuncSym = nullptr;
    std::vector<llvm::Value*> args;

    // V2: use resolvedCallee for direct calls
    if (node.resolvedCallee) {
        calleeFuncSym = node.resolvedCallee.get();
    }

    // Method call: callee is MemberAccessExpression
    if (auto* memAccess = node.callee->as<MemberAccessExpression>()) {
        // Static method call
        if (memAccess->isStaticAccess && calleeFuncSym) {
            auto it = functionCache_.find(calleeFuncSym);
            if (it != functionCache_.end()) {
                calleeFn = it->second;
            }
        }
        // String built-in methods
        else if (memAccess->isStringBuiltinMethod) {
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
                lastValue_ = builder_.CreateTrunc(len, i32Ty, "len.i32");
            } else if (memAccess->memberName == "charAt") {
                if (node.arguments && !node.arguments->expressions.empty()) {
                    node.arguments->expressions[0]->accept(*this);
                    llvm::Value* idx = lastValue_;
                    if (idx->getType()->isIntegerTy(32)) {
                        idx = builder_.CreateSExt(idx, i64Ty, "idx.i64");
                    }
                    llvm::Value* charPtr = builder_.CreateGEP(i8Ty, strVal, idx, "str.charAt");
                    lastValue_ = builder_.CreateLoad(i8Ty, charPtr, "char");
                } else {
                    lastValue_ = llvm::ConstantInt::get(i8Ty, 0);
                }
            } else if (memAccess->memberName == "substring") {
                llvm::Value* start = nullptr;
                llvm::Value* len = nullptr;
                if (node.arguments && node.arguments->expressions.size() >= 2) {
                    node.arguments->expressions[0]->accept(*this);
                    start = lastValue_;
                    node.arguments->expressions[1]->accept(*this);
                    len = lastValue_;
                }
                if (!start || !len) { lastValue_ = nullptr; return; }

                if (start->getType()->isIntegerTy(32))
                    start = builder_.CreateSExt(start, i64Ty, "start.i64");
                if (len->getType()->isIntegerTy(32))
                    len = builder_.CreateSExt(len, i64Ty, "len.i64");

                auto mallocCallee = module_->getOrInsertFunction("malloc",
                    llvm::FunctionType::get(ptrTy, {i64Ty}, false));
                llvm::Value* allocSize = builder_.CreateAdd(len,
                    llvm::ConstantInt::get(i64Ty, 1), "alloc.size");
                llvm::Value* buf = builder_.CreateCall(mallocCallee, {allocSize}, "sub.buf");

                auto memcpyCallee = module_->getOrInsertFunction("memcpy",
                    llvm::FunctionType::get(ptrTy, {ptrTy, ptrTy, i64Ty}, false));
                llvm::Value* srcPtr = builder_.CreateGEP(i8Ty, strVal, start, "sub.src");
                builder_.CreateCall(memcpyCallee, {buf, srcPtr, len});

                llvm::Value* endPtr = builder_.CreateGEP(i8Ty, buf, len, "sub.end");
                builder_.CreateStore(llvm::ConstantInt::get(i8Ty, 0), endPtr);

                registerRAII(buf, getOrCreateStringFreeFn());
                lastValue_ = buf;
            }
            return;
        }
        // Interface dispatch
        else if (memAccess->isArrow && memAccess->object && memAccess->object->resolvedType) {
            TypeSymbol* testObjType = memAccess->object->resolvedType->as<TypeSymbol>();
            if (auto* testPtrTy = testObjType ? testObjType->as<PointerTypeSymbol>() : nullptr) {
                if (auto* ifaceSym = testPtrTy->baseType ?
                    testPtrTy->baseType->as<InterfaceSymbol>() : nullptr) {
                    memAccess->object->accept(*this);
                    llvm::Value* fat = lastValue_;
                    llvm::Value* objPtr = builder_.CreateExtractValue(fat, {0}, "iface.obj");
                    llvm::Value* itable = builder_.CreateExtractValue(fat, {1}, "iface.itable");

                    auto methodSym = ifaceSym->findMethod(memAccess->memberName);
                    if (!methodSym) { lastValue_ = nullptr; return; }

                    auto* ptrTy2 = llvm::PointerType::getUnqual(context_);
                    llvm::Value* fnSlot = builder_.CreateGEP(
                        ptrTy2, itable,
                        builder_.getInt32(methodSym->vtableIndex),
                        "iface.slot");
                    llvm::Value* fn = builder_.CreateLoad(ptrTy2, fnSlot, "iface.fn");

                    std::vector<llvm::Value*> ifaceArgs = {objPtr};
                    if (node.arguments) {
                        for (size_t i = 0; i < node.arguments->expressions.size(); i++) {
                            auto& arg = node.arguments->expressions[i];
                            // V2: use ArgumentsNode::isReference
                            if (i < node.arguments->isReference.size() && node.arguments->isReference[i]) {
                                llvm::Value* lval = emitLValue(*arg);
                                if (lval) { ifaceArgs.push_back(lval); continue; }
                            }
                            arg->accept(*this);
                            if (lastValue_) {
                                if (arg->resolvedType && isUserStructKind(arg->resolvedType.get())) {
                                    llvm::Value* argPtr = emitLValue(*arg);
                                    if (argPtr) { ifaceArgs.push_back(argPtr); continue; }
                                }
                                // Interface parameter wrapping for interface dispatch args
                                if (arg->resolvedType && i < methodSym->parameters.size()) {
                                    auto* pType = methodSym->parameters[i]->getType().get();
                                    auto* pPtr = pType ? pType->as<PointerTypeSymbol>() : nullptr;
                                    if (pPtr && pPtr->baseType) {
                                        auto* iSym = pPtr->baseType->as<InterfaceSymbol>();
                                        if (iSym) {
                                            auto* aPtrType = arg->resolvedType->as<PointerTypeSymbol>();
                                            if (aPtrType && aPtrType->baseType) {
                                                auto* cSym = aPtrType->baseType->as<ClassSymbol>();
                                                if (cSym) {
                                                    lastValue_ = emitWrapToInterfacePtr(lastValue_, cSym, iSym);
                                                }
                                            }
                                        }
                                    }
                                }
                                ifaceArgs.push_back(lastValue_);
                            }
                        }
                    }

                    auto* fnTy = buildFunctionType(methodSym.get());
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

        // Regular method call
        if (!calleeFn && !memAccess->isStaticAccess && !memAccess->isStringBuiltinMethod) {
            // Get the object pointer (this for the method)
            if (memAccess->isArrow) {
                memAccess->object->accept(*this);
                thisPtr = lastValue_;
            } else {
                thisPtr = emitLValue(*memAccess->object);
                if (!thisPtr) {
                    memAccess->object->accept(*this);
                    thisPtr = lastValue_;
                    if (thisPtr && thisPtr->getType()->isStructTy()) {
                        auto* tmp = createEntryBlockAlloca(currentFunction_,
                            thisPtr->getType(), "method.this.tmp");
                        builder_.CreateStore(thisPtr, tmp);
                        thisPtr = tmp;
                    }
                }
            }

            // Find the method
            TypeSymbol* objType = memAccess->object->resolvedType ?
                memAccess->object->resolvedType->as<TypeSymbol>() : nullptr;
            if (auto* ptrTy = objType ? objType->as<PointerTypeSymbol>() : nullptr) {
                objType = ptrTy->baseType ? ptrTy->baseType->as<TypeSymbol>() : nullptr;
            }

            if (calleeFuncSym) {
                auto* classSym = objType ? objType->as<ClassSymbol>() : nullptr;
                if (classSym && calleeFuncSym->vtableIndex >= 0 && classSym->hasVtable()) {
                    isVirtualCall = true;
                    virtualMethodSym = calleeFuncSym;
                    virtualCallClass = classSym;
                } else {
                    auto it = functionCache_.find(calleeFuncSym);
                    if (it != functionCache_.end()) {
                        calleeFn = it->second;
                    }
                    if (calleeFuncSym->isStatic) thisPtr = nullptr;
                }
            } else {
                // Not a method — check for closure-typed field
                auto* fieldSym = memAccess->resolvedSymbol ?
                    memAccess->resolvedSymbol->as<VariableSymbol>() : nullptr;
                if (fieldSym && fieldSym->getType() && fieldSym->getType()->is<FunctionTypeSymbol>()) {
                    node.callee->accept(*this);
                    calleeVal = lastValue_;
                    thisPtr = nullptr;
                }
            }

            if ((calleeFn || isVirtualCall) && thisPtr) {
                args.push_back(thisPtr);
            }
        }
    }
    // Constructor call via type name
    else if (auto* ident = node.callee->as<IdentifierExpression>()) {
        if (ident->resolvedSymbol) {
            if (auto* classSym = ident->resolvedSymbol->as<ClassSymbol>()) {
                if (classSym->constructor) {
                    auto it = functionCache_.find(classSym->constructor.get());
                    if (it != functionCache_.end()) {
                        calleeFn = it->second;
                        calleeFuncSym = classSym->constructor.get();
                        isCtorCall = true;
                        llvm::Type* objTy = mapType(classSym);
                        thisPtr = createEntryBlockAlloca(currentFunction_, objTy, "ctor.tmp");
                        storeVtablePtr(thisPtr, classSym);
                        args.push_back(thisPtr);
                    }
                }
                if (!calleeFn) {
                    llvm::Type* objTy = mapType(classSym);
                    auto* tmp = createEntryBlockAlloca(currentFunction_, objTy, "class.tmp");
                    storeVtablePtr(tmp, classSym);
                    lastValue_ = builder_.CreateLoad(objTy, tmp, "class.val");
                    return;
                }
            }
            else if (auto* structSym = ident->resolvedSymbol->as<StructSymbol>()) {
                // Struct construction: zero-init
                lastValue_ = llvm::Constant::getNullValue(mapType(structSym));
                return;
            }
            else if (auto* funcSym = ident->resolvedSymbol->as<FunctionSymbol>()) {
                // Regular function call — extract FunctionSymbol directly
                calleeFuncSym = funcSym;
                auto it = functionCache_.find(funcSym);
                if (it != functionCache_.end()) {
                    calleeFn = it->second;
                }
            }
            else if (!calleeFn && calleeFuncSym) {
                auto it = functionCache_.find(calleeFuncSym);
                if (it != functionCache_.end()) {
                    calleeFn = it->second;
                }
            }
        }
        // Indirect call through variable
        if (!calleeFn && !calleeFuncSym) {
            node.callee->accept(*this);
            calleeVal = lastValue_;
        }
    }
    else {
        node.callee->accept(*this);
        calleeVal = lastValue_;
    }

    // Collect expected parameter types for interface wrapping
    // From direct callee (FunctionSymbol) or closure type (FunctionTypeSymbol)
    const std::vector<FunctionTypeSymbol::ParameterInfo>* expectedFuncParams = nullptr;
    std::vector<std::shared_ptr<VariableSymbol>>* expectedFuncSymParams = nullptr;
    if (calleeFuncSym) {
        expectedFuncSymParams = &calleeFuncSym->parameters;
    } else if (node.callee && node.callee->resolvedType) {
        if (auto* fts = node.callee->resolvedType->as<FunctionTypeSymbol>()) {
            expectedFuncParams = &fts->parameters;
        }
    }

    // Emit arguments — V2 uses ArgumentsNode::isReference
    if (node.arguments) {
        for (size_t argI = 0; argI < node.arguments->expressions.size(); argI++) {
            auto& arg = node.arguments->expressions[argI];

            // V2: check isReference from ArgumentsNode
            bool isRef = (argI < node.arguments->isReference.size() &&
                         node.arguments->isReference[argI]);

            if (isRef) {
                llvm::Value* lval = emitLValue(*arg);
                if (lval) { args.push_back(lval); continue; }
                arg->accept(*this);
                if (lastValue_) {
                    auto* tmp = createEntryBlockAlloca(currentFunction_,
                        lastValue_->getType(), "ref.tmp");
                    builder_.CreateStore(lastValue_, tmp);
                    args.push_back(tmp);
                }
                continue;
            }

            arg->accept(*this);
            if (lastValue_) {
                if (arg->resolvedType && isUserStructKind(arg->resolvedType.get())) {
                    llvm::Value* argPtr = emitLValue(*arg);
                    if (argPtr) { args.push_back(argPtr); continue; }
                    if (lastValue_->getType()->isStructTy()) {
                        auto* tmp = createEntryBlockAlloca(currentFunction_,
                            lastValue_->getType(), "arg.tmp");
                        builder_.CreateStore(lastValue_, tmp);
                        args.push_back(tmp);
                        continue;
                    }
                }

                // Interface parameter wrapping: class* → interface fat pointer
                // When expected param is Drawable* (interface) but arg is Dog* (class),
                // wrap using emitWrapToInterfacePtr to create {objPtr, itablePtr} fat ptr.
                TypeSymbol* expectedParamType = nullptr;
                if (expectedFuncSymParams && argI < expectedFuncSymParams->size()) {
                    expectedParamType = (*expectedFuncSymParams)[argI]->getType().get();
                } else if (expectedFuncParams && argI < expectedFuncParams->size()) {
                    expectedParamType = (*expectedFuncParams)[argI].type.get();
                }
                if (expectedParamType && arg->resolvedType) {
                    auto* ptrParam = expectedParamType->as<PointerTypeSymbol>();
                    if (ptrParam && ptrParam->baseType) {
                        auto* ifaceSym = ptrParam->baseType->as<InterfaceSymbol>();
                        if (ifaceSym) {
                            auto* argPtrType = arg->resolvedType->as<PointerTypeSymbol>();
                            if (argPtrType && argPtrType->baseType) {
                                auto* classSym = argPtrType->baseType->as<ClassSymbol>();
                                if (classSym) {
                                    lastValue_ = emitWrapToInterfacePtr(lastValue_, classSym, ifaceSym);
                                }
                            }
                        }
                    }
                }

                // Varargs promotion: small ints → i32, float → double
                // (C calling convention requires promotion for variadic args)
                bool isVariadicArg = false;
                if (calleeFuncSym && calleeFuncSym->isVariadic) {
                    size_t fixedCount = calleeFuncSym->parameters.size();
                    isVariadicArg = (argI >= fixedCount);
                }
                if (isVariadicArg && lastValue_) {
                    auto* ty = lastValue_->getType();
                    if (ty->isIntegerTy() && ty->getIntegerBitWidth() < 32) {
                        lastValue_ = builder_.CreateSExt(lastValue_,
                            llvm::Type::getInt32Ty(context_), "vararg.promote");
                    } else if (ty->isFloatTy()) {
                        lastValue_ = builder_.CreateFPExt(lastValue_,
                            llvm::Type::getDoubleTy(context_), "vararg.fpromote");
                    }
                }

                // RAII-wrap temporary closure arguments
                if (arg->resolvedType && arg->resolvedType->is<FunctionTypeSymbol>() &&
                    arg->is<LambdaExpression>()) {
                    auto* fatPtrTy = getFatPtrType();
                    auto* tmpAlloca = createEntryBlockAlloca(currentFunction_,
                        fatPtrTy, "tmp.closure.arg");
                    builder_.CreateStore(lastValue_, tmpAlloca);
                    registerRAII(tmpAlloca, getOrCreateClosureReleaseWrapper());
                }
                args.push_back(lastValue_);
            }
        }
    }

    // Implicit integer widening for arguments
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
        auto* structTy = getStructType(virtualCallClass);
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
            llvm::Value* fnPtr = builder_.CreateExtractValue(calleeVal, {0}, "fn.ptr");
            llvm::Value* envPtr = builder_.CreateExtractValue(calleeVal, {1}, "env.ptr");

            TypeSymbol* calleeType = node.callee->resolvedType ?
                node.callee->resolvedType->as<TypeSymbol>() : nullptr;
            if (auto* funcType = calleeType ? calleeType->as<FunctionTypeSymbol>() : nullptr) {
                llvm::Type* retTy = mapType(funcType->returnType);
                std::vector<llvm::Type*> paramTypes;
                // V2: use FunctionTypeSymbol::ParameterInfo for correct param types
                for (auto& pi : funcType->parameters) {
                    paramTypes.push_back(mapParamType(pi.type.get(), pi.isReference));
                }
                paramTypes.push_back(llvm::PointerType::getUnqual(context_));
                auto* fnTy = llvm::FunctionType::get(retTy, paramTypes, false);
                args.push_back(envPtr);
                lastValue_ = builder_.CreateCall(fnTy, fnPtr, args);
            }
        } else if (llvm::dyn_cast<llvm::PointerType>(calleeVal->getType())) {
            TypeSymbol* calleeType = node.callee->resolvedType ?
                node.callee->resolvedType->as<TypeSymbol>() : nullptr;
            if (auto* funcType = calleeType ? calleeType->as<FunctionTypeSymbol>() : nullptr) {
                llvm::Type* retTy = mapType(funcType->returnType);
                std::vector<llvm::Type*> paramTypes;
                for (auto& pi : funcType->parameters) {
                    paramTypes.push_back(mapParamType(pi.type.get(), pi.isReference));
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
    TypeSymbol* allocType = node.type && node.type->resolvedType ?
        node.type->resolvedType->as<TypeSymbol>() : nullptr;
    if (!allocType) {
        lastValue_ = llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(context_));
        return;
    }

    if (node.isArray) {
        llvm::Type* elemTy = mapType(allocType);
        node.arraySize->accept(*this);
        llvm::Value* sizeVal = lastValue_;

        auto& dl = module_->getDataLayout();
        uint64_t elemSize = dl.getTypeAllocSize(elemTy);
        llvm::Value* elemSizeVal = llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(context_), elemSize);
        llvm::Value* totalBytes = builder_.CreateMul(sizeVal, elemSizeVal, "arr.bytes");

        auto mallocCallee = module_->getOrInsertFunction("malloc",
            llvm::FunctionType::get(llvm::PointerType::getUnqual(context_),
                {llvm::Type::getInt32Ty(context_)}, false));
        lastValue_ = builder_.CreateCall(mallocCallee, {totalBytes}, "arr.ptr");
    } else {
        llvm::Type* objTy = mapType(allocType);
        auto& dl = module_->getDataLayout();
        uint64_t objSize = dl.getTypeAllocSize(objTy);
        llvm::Value* sizeVal = llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(context_), objSize);

        auto mallocCallee = module_->getOrInsertFunction("malloc",
            llvm::FunctionType::get(llvm::PointerType::getUnqual(context_),
                {llvm::Type::getInt32Ty(context_)}, false));
        llvm::Value* rawPtr = builder_.CreateCall(mallocCallee, {sizeVal}, "new.ptr");

        if (auto* classSym = allocType->as<ClassSymbol>()) {
            if (classSym->constructor) {
                auto it = functionCache_.find(classSym->constructor.get());
                if (it != functionCache_.end()) {
                    std::vector<llvm::Value*> ctorArgs;
                    ctorArgs.push_back(rawPtr);
                    if (node.arguments) {
                        for (size_t i = 0; i < node.arguments->expressions.size(); i++) {
                            auto& arg = node.arguments->expressions[i];
                            bool isRef = (i < node.arguments->isReference.size() &&
                                         node.arguments->isReference[i]);
                            if (isRef) {
                                llvm::Value* lval = emitLValue(*arg);
                                if (lval) { ctorArgs.push_back(lval); continue; }
                            }
                            arg->accept(*this);
                            if (lastValue_) {
                                if (arg->resolvedType && isUserStructKind(arg->resolvedType.get())) {
                                    llvm::Value* argPtr = emitLValue(*arg);
                                    if (argPtr) { ctorArgs.push_back(argPtr); continue; }
                                }
                                ctorArgs.push_back(lastValue_);
                            }
                        }
                    }
                    builder_.CreateCall(it->second, ctorArgs);
                }
            } else {
                storeVtablePtr(rawPtr, classSym);
            }
        }

        lastValue_ = rawPtr;
    }
}

void IRGenerator::visit(IndexExpression& node) {
    if (node.isOperatorOverload && node.resolvedOperatorFunction) {
        auto it = functionCache_.find(node.resolvedOperatorFunction.get());
        if (it != functionCache_.end()) {
            llvm::Value* objPtr = emitLValue(*node.object);
            if (!objPtr) {
                node.object->accept(*this);
                objPtr = lastValue_;
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

    // String indexing
    TypeSymbol* objType = node.object->resolvedType ?
        node.object->resolvedType->as<TypeSymbol>() : nullptr;
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

    llvm::Type* targetTy = mapType(node.targetType ? node.targetType->resolvedType : nullptr);
    TypeSymbol* fromType = node.operand->resolvedType ?
        node.operand->resolvedType->as<TypeSymbol>() : nullptr;
    TypeSymbol* toType = node.targetType && node.targetType->resolvedType ?
        node.targetType->resolvedType->as<TypeSymbol>() : nullptr;

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
        if (fromBits < toBits) lastValue_ = builder_.CreateSExt(val, targetTy, "sext");
        else if (fromBits > toBits) lastValue_ = builder_.CreateTrunc(val, targetTy, "trunc");
    } else if (isPointerKind(fromType) && isIntegerKind(toType)) {
        lastValue_ = builder_.CreatePtrToInt(val, targetTy, "ptrtoint");
    } else if (isIntegerKind(fromType) && isPointerKind(toType)) {
        lastValue_ = builder_.CreateIntToPtr(val, targetTy, "inttoptr");
    }
}

void IRGenerator::visit(SizeOfExpression& node) {
    llvm::Type* ty = mapType(node.targetType ? node.targetType->resolvedType : nullptr);
    auto& dl = module_->getDataLayout();
    uint64_t size = dl.getTypeAllocSize(ty);
    lastValue_ = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context_), size);
}

void IRGenerator::visit(PipeExpression& node) {
    node.input->accept(*this);
    llvm::Value* currentVal = lastValue_;
    if (!currentVal) { lastValue_ = nullptr; return; }

    for (auto& stage : node.stages) {
        llvm::Function* stageFn = nullptr;
        llvm::Value* stageVal = nullptr;
        bool isMethodPipe = false;

        // Check for member access pipe: x |> obj.method(args)
        auto* memAccess = stage.function ? stage.function->as<MemberAccessExpression>() : nullptr;
        if (memAccess) {
            isMethodPipe = true;
        } else if (stage.function && stage.function->resolvedSymbol) {
            if (auto* funcSym = stage.function->resolvedSymbol->as<FunctionSymbol>()) {
                auto it = functionCache_.find(funcSym);
                if (it != functionCache_.end()) stageFn = it->second;
            }
            if (!stageFn) {
                auto it = namedValues_.find(stage.function->resolvedSymbol.get());
                if (it != namedValues_.end()) {
                    llvm::Type* loadTy = mapType(stage.function->resolvedType);
                    stageVal = builder_.CreateLoad(loadTy, it->second, "pipe.fn");
                }
            }
        }

        if (isMethodPipe && memAccess) {
            // Method call pipe: x |> obj.method(args) → obj.method(x, args)
            memAccess->object->accept(*this);
            llvm::Value* objPtr = lastValue_;
            if (!objPtr) { lastValue_ = nullptr; return; }

            auto* methodSym = memAccess->resolvedSymbol ?
                memAccess->resolvedSymbol->as<FunctionSymbol>() : nullptr;
            if (!methodSym) { lastValue_ = nullptr; return; }

            // Get class symbol for virtual dispatch
            TypeSymbol* objType = memAccess->object->resolvedType ?
                memAccess->object->resolvedType->as<TypeSymbol>() : nullptr;
            if (auto* ptrTy = objType ? objType->as<PointerTypeSymbol>() : nullptr) {
                objType = ptrTy->baseType ? ptrTy->baseType->as<TypeSymbol>() : nullptr;
            }
            auto* classSym = objType ? objType->as<ClassSymbol>() : nullptr;

            // Build args: (this, pipedValue, extraArgs...)
            std::vector<llvm::Value*> methodArgs;
            methodArgs.push_back(objPtr);      // this
            methodArgs.push_back(currentVal);   // piped value as first real arg
            for (auto& extra : stage.extraArguments) {
                extra->accept(*this);
                if (lastValue_) methodArgs.push_back(lastValue_);
            }

            // Virtual dispatch or direct call
            if (classSym && methodSym->vtableIndex >= 0 && classSym->hasVtable()) {
                auto* structTy = getStructType(classSym);
                auto* ptrTy = llvm::PointerType::getUnqual(context_);
                auto* vtablePtrPtr = builder_.CreateStructGEP(structTy, objPtr, 0, "pipe.vtable.ptr");
                auto* vtable = builder_.CreateLoad(ptrTy, vtablePtrPtr, "pipe.vtable");
                auto* methodSlot = builder_.CreateGEP(ptrTy, vtable,
                    builder_.getInt32(methodSym->vtableIndex), "pipe.method.slot");
                auto* methodFn = builder_.CreateLoad(ptrTy, methodSlot, "pipe.method.fn");
                auto* fnTy = buildFunctionType(methodSym);
                currentVal = builder_.CreateCall(fnTy, methodFn, methodArgs, "pipe.result");
            } else {
                auto it = functionCache_.find(methodSym);
                if (it != functionCache_.end()) {
                    currentVal = builder_.CreateCall(it->second, methodArgs, "pipe.result");
                }
            }
        } else {
            // Regular function or closure pipe
            std::vector<llvm::Value*> pipeArgs;
            pipeArgs.push_back(currentVal);
            for (auto& extra : stage.extraArguments) {
                extra->accept(*this);
                if (lastValue_) pipeArgs.push_back(lastValue_);
            }

            if (stageFn) {
                currentVal = builder_.CreateCall(stageFn, pipeArgs, "pipe.result");
            } else if (stageVal) {
                TypeSymbol* stageType = stage.function && stage.function->resolvedType ?
                    stage.function->resolvedType->as<TypeSymbol>() : nullptr;
                if (auto* fnType = stageType ? stageType->as<FunctionTypeSymbol>() : nullptr) {
                    llvm::Type* retTy = mapType(fnType->returnType);
                    std::vector<llvm::Type*> paramTypes;
                    for (auto& pi : fnType->parameters) {
                        paramTypes.push_back(mapParamType(pi.type.get(), pi.isReference));
                    }

                    if (stageVal->getType()->isStructTy()) {
                        llvm::Value* fnPtr = builder_.CreateExtractValue(stageVal, {0}, "pipe.fn.ptr");
                        llvm::Value* envPtr = builder_.CreateExtractValue(stageVal, {1}, "pipe.env.ptr");
                        paramTypes.push_back(llvm::PointerType::getUnqual(context_));
                        auto* fnTy = llvm::FunctionType::get(retTy, paramTypes, false);
                        pipeArgs.push_back(envPtr);
                        currentVal = builder_.CreateCall(fnTy, fnPtr, pipeArgs, "pipe.result");
                    } else {
                        auto* fnTy = llvm::FunctionType::get(retTy, paramTypes, false);
                        currentVal = builder_.CreateCall(fnTy, stageVal, pipeArgs, "pipe.result");
                    }
                }
            }
        }
    }

    lastValue_ = currentVal;
}

void IRGenerator::visit(MatchExpression& node) {
    node.subject->accept(*this);
    llvm::Value* subjectVal = lastValue_;
    if (!subjectVal) { lastValue_ = nullptr; return; }

    TypeSymbol* subjectType = node.subject->resolvedType ?
        node.subject->resolvedType->as<TypeSymbol>() : nullptr;

    auto* mergeBB = llvm::BasicBlock::Create(context_, "match.merge", currentFunction_);
    llvm::Type* resultTy = mapType(node.resolvedType);
    bool hasResult = resultTy && !resultTy->isVoidTy();

    std::vector<std::pair<llvm::Value*, llvm::BasicBlock*>> phiIncoming;

    // Chain of conditional branches
    for (size_t i = 0; i < node.arms.size(); i++) {
        auto& arm = node.arms[i];
        auto* armBodyBB = llvm::BasicBlock::Create(context_, "arm.body", currentFunction_);
        auto* nextTestBB = (i + 1 < node.arms.size())
            ? llvm::BasicBlock::Create(context_, "arm.test", currentFunction_)
            : mergeBB;

        auto* pattern = arm.pattern.get();

        if (auto* litPat = pattern->as<LiteralPattern>()) {
            litPat->value->accept(*this);
            llvm::Value* patVal = lastValue_;
            if (!patVal) {
                builder_.CreateBr(nextTestBB);
                if (nextTestBB != mergeBB) builder_.SetInsertPoint(nextTestBB);
                continue;
            }
            llvm::Value* cond;
            if (isFloatingKind(subjectType)) {
                cond = builder_.CreateFCmpOEQ(subjectVal, patVal, "match.cmp");
            } else {
                if (subjectVal->getType() != patVal->getType() &&
                    subjectVal->getType()->isIntegerTy() && patVal->getType()->isIntegerTy()) {
                    patVal = builder_.CreateSExtOrTrunc(patVal, subjectVal->getType());
                }
                cond = builder_.CreateICmpEQ(subjectVal, patVal, "match.cmp");
            }
            builder_.CreateCondBr(cond, armBodyBB, nextTestBB);

        } else if (pattern->is<WildcardPattern>()) {
            builder_.CreateBr(armBodyBB);

        } else if (auto* identPat = pattern->as<IdentifierPattern>()) {
            // Alloca for binding
            llvm::Type* bindTy = subjectVal->getType();
            auto* bindAlloca = createEntryBlockAlloca(currentFunction_, bindTy, identPat->name);
            builder_.CreateStore(subjectVal, bindAlloca);
            if (identPat->resolvedSymbol) {
                namedValues_[identPat->resolvedSymbol.get()] = bindAlloca;
            }

            if (identPat->guard) {
                identPat->guard->accept(*this);
                llvm::Value* guardVal = lastValue_;
                if (guardVal) {
                    if (!guardVal->getType()->isIntegerTy(1)) {
                        guardVal = builder_.CreateICmpNE(guardVal,
                            llvm::ConstantInt::get(guardVal->getType(), 0), "guard");
                    }
                    builder_.CreateCondBr(guardVal, armBodyBB, nextTestBB);
                } else {
                    builder_.CreateBr(nextTestBB);
                }
            } else {
                builder_.CreateBr(armBodyBB);
            }

        } else if (auto* rangePat = pattern->as<RangePattern>()) {
            rangePat->low->accept(*this);
            llvm::Value* lowVal = lastValue_;
            rangePat->high->accept(*this);
            llvm::Value* highVal = lastValue_;
            llvm::Value* ge = builder_.CreateICmpSGE(subjectVal, lowVal, "range.ge");
            llvm::Value* le = builder_.CreateICmpSLE(subjectVal, highVal, "range.le");
            llvm::Value* inRange = builder_.CreateAnd(ge, le, "range.in");
            builder_.CreateCondBr(inRange, armBodyBB, nextTestBB);

        } else {
            builder_.CreateBr(armBodyBB);
        }

        builder_.SetInsertPoint(armBodyBB);
        arm.body->accept(*this);
        llvm::Value* bodyVal = lastValue_;
        auto* bodyEndBB = builder_.GetInsertBlock();
        if (!bodyEndBB->getTerminator()) builder_.CreateBr(mergeBB);
        if (hasResult && bodyVal) phiIncoming.push_back({bodyVal, bodyEndBB});

        if (nextTestBB != mergeBB) {
            builder_.SetInsertPoint(nextTestBB);
        }
    }

    builder_.SetInsertPoint(mergeBB);
    if (hasResult && !phiIncoming.empty()) {
        auto* phi = builder_.CreatePHI(resultTy, (unsigned)phiIncoming.size(), "match.result");
        for (auto& [val, bb] : phiIncoming) {
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

    if (auto* fnType = node.resolvedType ? node.resolvedType->as<FunctionTypeSymbol>() : nullptr) {
        retTy = mapType(fnType->returnType);
        // V2: use FunctionTypeSymbol::ParameterInfo for correct param types
        for (auto& pi : fnType->parameters) {
            paramTypes.push_back(mapParamType(pi.type.get(), pi.isReference));
        }
    }

    // ALL lambdas get ptr %env as last parameter (fat pointer calling convention)
    paramTypes.push_back(llvm::PointerType::getUnqual(context_));

    auto* fnTy = llvm::FunctionType::get(retTy, paramTypes, false);
    std::string lambdaName = "__lambda_" + std::to_string(lambdaCounter_++);
    auto* lambdaFn = llvm::Function::Create(fnTy,
        llvm::Function::InternalLinkage, lambdaName, module_.get());

    // CRITICAL: Save/restore state for lambda isolation
    auto* prevFunction = currentFunction_;
    auto* prevThisPtr = currentThisPtr_;
    auto savedNamedValues = namedValues_;
    auto savedInsertPoint = builder_.GetInsertBlock();
    auto savedInsertPointIt = builder_.GetInsertPoint();
    auto savedRAIIStack = std::move(raiiScopeStack_);
    raiiScopeStack_.clear();

    currentFunction_ = lambdaFn;
    currentThisPtr_ = nullptr;
    namedValues_.clear();

    auto* entryBB = llvm::BasicBlock::Create(context_, "entry", lambdaFn);
    builder_.SetInsertPoint(entryBB);

    // V2 IMPROVEMENT: use ParameterNode::resolvedSymbol directly (no scanForParamSymbols!)
    unsigned argIdx = 0;
    for (auto& param : node.parameters) {
        llvm::Value* argVal = lambdaFn->getArg(argIdx++);
        argVal->setName(param->name);

        auto* paramSym = param->resolvedSymbol.get();
        if (paramSym && paramSym->isReference) {
            namedValues_[paramSym] = argVal;
        } else if (paramSym && isUserStructKind(paramSym->getType().get())) {
            llvm::Type* structTy = mapType(paramSym->getType());
            auto* alloca = createEntryBlockAlloca(lambdaFn, structTy, param->name);
            auto* val = builder_.CreateLoad(structTy, argVal, param->name + ".val");
            builder_.CreateStore(val, alloca);
            namedValues_[paramSym] = alloca;
        } else {
            auto* alloca = createEntryBlockAlloca(lambdaFn, argVal->getType(), param->name);
            builder_.CreateStore(argVal, alloca);
            if (paramSym) namedValues_[paramSym] = alloca;
        }
    }

    // Extract captures from env pointer
    if (hasCaptures) {
        llvm::Value* envPtr = lambdaFn->getArg(lambdaFn->arg_size() - 1);
        envPtr->setName("env");

        auto* i64TyCap = llvm::Type::getInt64Ty(context_);
        auto* ptrTyCap = llvm::PointerType::getUnqual(context_);

        std::vector<llvm::Type*> closureFields;
        closureFields.push_back(i64TyCap);
        closureFields.push_back(ptrTyCap);
        for (size_t i = 0; i < node.capturedVariables.size(); i++) {
            bool isByRef = (i < node.captureModesResolved.size() &&
                           node.captureModesResolved[i] == CaptureMode::ByReference);
            if (isByRef) {
                closureFields.push_back(ptrTyCap);
            } else {
                auto* varSym = node.capturedVariables[i]->as<VariableSymbol>();
                closureFields.push_back(varSym ? mapType(varSym->getType())
                                               : llvm::Type::getInt32Ty(context_));
            }
        }
        auto* closureTy = llvm::StructType::get(context_, closureFields);

        const int headerOffset = 2;
        for (size_t i = 0; i < node.capturedVariables.size(); i++) {
            auto* capSym = node.capturedVariables[i].get();
            auto* varSym = capSym->as<VariableSymbol>();
            bool isByRef = (i < node.captureModesResolved.size() &&
                           node.captureModesResolved[i] == CaptureMode::ByReference);

            auto* capPtr = builder_.CreateStructGEP(closureTy, envPtr,
                                                     headerOffset + (unsigned)i,
                                                     capSym->getName() + ".cap");

            if (isByRef) {
                auto* origPtr = builder_.CreateLoad(ptrTyCap, capPtr, capSym->getName() + ".ref");
                namedValues_[capSym] = origPtr;
            } else {
                llvm::Type* capType = varSym ? mapType(varSym->getType())
                                             : llvm::Type::getInt32Ty(context_);
                auto* capVal = builder_.CreateLoad(capType, capPtr, capSym->getName());
                auto* capAlloca = createEntryBlockAlloca(lambdaFn, capType, capSym->getName());
                builder_.CreateStore(capVal, capAlloca);
                namedValues_[capSym] = capAlloca;
            }
        }
    }

    // Push RAII scope for lambda body
    pushRAIIScope();

    // Emit body
    if (node.hasExpressionBody()) {
        node.body->as<ExpressionBaseNode>()->accept(*this);
        if (lastValue_ && !retTy->isVoidTy()) {
            builder_.CreateRet(lastValue_);
        } else {
            builder_.CreateRetVoid();
        }
    } else if (node.hasBlockBody()) {
        node.body->as<BlockStatementNode>()->accept(*this);
        if (!builder_.GetInsertBlock()->getTerminator()) {
            if (retTy->isVoidTy()) {
                builder_.CreateRetVoid();
            } else {
                builder_.CreateRet(llvm::UndefValue::get(retTy));
            }
        }
    }

    popRAIIScope();

    // Restore state
    currentFunction_ = prevFunction;
    currentThisPtr_ = prevThisPtr;
    namedValues_ = savedNamedValues;
    raiiScopeStack_ = std::move(savedRAIIStack);
    builder_.SetInsertPoint(savedInsertPoint, savedInsertPointIt);

    if (hasCaptures) {
        auto* i64TyA = llvm::Type::getInt64Ty(context_);
        auto* ptrTyA = llvm::PointerType::getUnqual(context_);

        std::vector<llvm::Type*> closureFields;
        closureFields.push_back(i64TyA);
        closureFields.push_back(ptrTyA);
        for (size_t i = 0; i < node.capturedVariables.size(); i++) {
            bool isByRef = (i < node.captureModesResolved.size() &&
                           node.captureModesResolved[i] == CaptureMode::ByReference);
            if (isByRef) {
                closureFields.push_back(ptrTyA);
            } else {
                auto* varSym = node.capturedVariables[i]->as<VariableSymbol>();
                closureFields.push_back(varSym ? mapType(varSym->getType())
                                               : llvm::Type::getInt32Ty(context_));
            }
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

        // Check if any by-value captured variable is a closure
        bool capturesClosures = false;
        for (size_t i = 0; i < node.capturedVariables.size(); i++) {
            bool isByRef = (i < node.captureModesResolved.size() &&
                           node.captureModesResolved[i] == CaptureMode::ByReference);
            if (isByRef) continue;
            auto* varSym = node.capturedVariables[i]->as<VariableSymbol>();
            if (varSym && varSym->getType() && varSym->getType()->is<FunctionTypeSymbol>()) {
                capturesClosures = true;
                break;
            }
        }

        // Store cleanup function pointer
        auto* cleanupSlot = builder_.CreateStructGEP(closureTy, closurePtr, 1, "cleanup.slot");
        if (capturesClosures) {
            auto* cleanupFn = generateClosureCleanupFn(closureTy, node.capturedVariables,
                                                        2, &node.captureModesResolved);
            builder_.CreateStore(cleanupFn, cleanupSlot);
        } else {
            builder_.CreateStore(llvm::ConstantPointerNull::get(ptrTyA), cleanupSlot);
        }

        // Store captured values
        const int headerOffset = 2;
        for (size_t i = 0; i < node.capturedVariables.size(); i++) {
            auto* capSym = node.capturedVariables[i].get();
            auto* varSym = capSym->as<VariableSymbol>();
            bool isByRef = (i < node.captureModesResolved.size() &&
                           node.captureModesResolved[i] == CaptureMode::ByReference);

            auto it = savedNamedValues.find(capSym);
            if (it != savedNamedValues.end()) {
                auto* capSlot = builder_.CreateStructGEP(closureTy, closurePtr,
                                                          headerOffset + (unsigned)i,
                                                          capSym->getName() + ".slot");

                if (isByRef) {
                    builder_.CreateStore(it->second, capSlot);
                } else {
                    llvm::Type* capTy = varSym ? mapType(varSym->getType())
                                               : llvm::Type::getInt32Ty(context_);
                    llvm::Value* capVal = builder_.CreateLoad(capTy, it->second, capSym->getName() + ".val");
                    builder_.CreateStore(capVal, capSlot);

                    if (varSym && varSym->getType() && varSym->getType()->is<FunctionTypeSymbol>()) {
                        auto* capturedEnv = builder_.CreateExtractValue(capVal, {1},
                                                                        capSym->getName() + ".cap.env");
                        builder_.CreateCall(getOrCreateClosureRetainFn(), {capturedEnv});
                    }
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
        auto* nullPtr = llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(context_));
        llvm::Value* fat = llvm::UndefValue::get(fatTy);
        fat = builder_.CreateInsertValue(fat, lambdaFn, {0}, "fat.fn");
        fat = builder_.CreateInsertValue(fat, nullPtr, {1}, "fat.env");
        lastValue_ = fat;
    }
}

void IRGenerator::visit(VariableDeclarationExpression& node) {
    auto* varSym = node.resolvedVariable.get();
    if (!varSym) return;

    llvm::Type* varTy = mapType(varSym->getType());
    if (varTy->isVoidTy()) return;

    auto* alloca = createEntryBlockAlloca(currentFunction_, varTy, node.name);
    namedValues_[varSym] = alloca;

    if (node.initializer) {
        node.initializer->accept(*this);
        if (lastValue_) {
            builder_.CreateStore(lastValue_, alloca);
        }
    }

    lastValue_ = alloca;
}

} // namespace codegen
} // namespace mingus
