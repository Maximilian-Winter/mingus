#pragma once

//================================================================================
// MINGUS V2 - LLVM IR Generator
//
// Walks the semantically-annotated AST and emits LLVM IR.
// Requires all 4 semantic passes to have completed:
//   Pass 1 (SymbolTableBuilder): scope tree, symbols, auto ctor/dtor, vtables
//   Pass 2 (TypeResolver): TypeNode -> TypeSymbol resolution
//   Pass 3 (TypeChecker): expression type inference, call resolution
//   Pass 4 (SemanticValidator): capture analysis, RAII tracking
//
// Key V2 improvements over V1:
//   - No scope navigation stack (childIndexStack_) — each AST node has astScopeNode
//   - No scanForParamSymbols — ParameterNode::resolvedSymbol links directly
//   - Unified mapType(TypeSymbol*) — single entry point for type mapping
//   - mapParamType() handles struct/class/interface/ref params uniformly
//   - ArgumentsNode::isReference[] for per-arg ref tracking
//   - CallExpression::resolvedCallee for pre-resolved function dispatch
//================================================================================

#include "mingus/AstNode.h"
#include "mingus/Expressions.h"
#include "mingus/Statements.h"
#include "mingus/Declarations.h"
#include "mingus/Symbols.h"
#include "mingus/SymbolTable.h"
#include "mingus/sema/SemanticValidator.h"

#pragma warning(push, 0)
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Verifier.h>
#pragma warning(pop)

#include <map>
#include <set>
#include <string>
#include <memory>
#include <unordered_map>

namespace mingus {
namespace codegen {

//================================================================================
// IRGenerator — ASTVisitor that emits LLVM IR
//================================================================================
class IRGenerator : public ASTVisitor {
public:
    IRGenerator(SymbolTable& symbolTable,
                const std::unordered_map<Scope*, ScopeRAIIInfo>& raiiInfo);

    // Entry point — generates LLVM IR and returns the module
    std::unique_ptr<llvm::Module> generate(ProgramNode& program);

    // Access the LLVMContext (needed for verification)
    llvm::LLVMContext& getContext() { return context_; }

    //==========================================================================
    // ASTVisitor overrides
    //==========================================================================

    // Program structure
    void visit(ProgramNode& node) override;
    void visit(ModuleNode& node) override;
    void visit(BlockStatementNode& node) override;

    // Declarations
    void visit(VariableDeclaration& node) override;
    void visit(TupleDestructuringDeclaration& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(ConstructorDeclaration& node) override;
    void visit(DestructorDeclaration& node) override;
    void visit(OperatorDeclaration& node) override;
    void visit(ExternFunctionDeclaration& node) override;
    void visit(EnumMemberNode& node) override;
    void visit(EnumDeclaration& node) override;
    void visit(StructDeclaration& node) override;
    void visit(ClassDeclaration& node) override;
    void visit(InterfaceDeclaration& node) override;
    void visit(ImportDeclaration& node) override;
    void visit(TypedefDeclaration& node) override;

    // Statements
    void visit(ExpressionStatement& node) override;
    void visit(ReturnStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(SwitchStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(DoWhileStatement& node) override;
    void visit(LabeledStatement& node) override;
    void visit(BreakStatement& node) override;
    void visit(ContinueStatement& node) override;
    void visit(DeleteStatement& node) override;

    // Expressions
    void visit(IntegerLiteral& node) override;
    void visit(FloatLiteral& node) override;
    void visit(BoolLiteral& node) override;
    void visit(CharLiteral& node) override;
    void visit(StringLiteral& node) override;
    void visit(InterpolatedStringExpression& node) override;
    void visit(NullLiteral& node) override;
    void visit(IdentifierExpression& node) override;
    void visit(QualifiedNameExpression& node) override;
    void visit(MemberAccessExpression& node) override;
    void visit(ThisExpression& node) override;
    void visit(BinaryExpression& node) override;
    void visit(UnaryExpression& node) override;
    void visit(AssignmentExpression& node) override;
    void visit(TernaryExpression& node) override;
    void visit(CallExpression& node) override;
    void visit(NewExpression& node) override;
    void visit(IndexExpression& node) override;
    void visit(CastExpression& node) override;
    void visit(SizeOfExpression& node) override;
    void visit(PipeExpression& node) override;
    void visit(MatchExpression& node) override;
    void visit(TupleExpression& node) override;
    void visit(LambdaExpression& node) override;
    void visit(VariableDeclarationExpression& node) override;

private:
    //==========================================================================
    // LLVM infrastructure
    //==========================================================================
    llvm::LLVMContext context_;
    std::unique_ptr<llvm::Module> module_;
    llvm::IRBuilder<> builder_;

    //==========================================================================
    // Semantic analysis data (read-only)
    //==========================================================================
    SymbolTable& symbolTable_;
    const std::unordered_map<Scope*, ScopeRAIIInfo>& raiiInfo_;

    //==========================================================================
    // Codegen state
    //==========================================================================
    llvm::Function* currentFunction_ = nullptr;
    llvm::Value* currentThisPtr_ = nullptr;
    llvm::Value* lastValue_ = nullptr;

    // Current module name (for name mangling)
    std::string currentModuleName_;

    // Current class/struct being visited (for ctor/dtor/method/operator)
    ClassSymbol* currentClassSym_ = nullptr;
    StructSymbol* currentStructSym_ = nullptr;

    //==========================================================================
    // Caches
    //==========================================================================
    std::unordered_map<Symbol*, llvm::Value*> namedValues_;
    std::unordered_map<Symbol*, llvm::Function*> functionCache_;
    std::unordered_map<std::string, llvm::GlobalVariable*> stringConstants_;

    // TypeSymbol* -> LLVM StructType* (for GEP with opaque pointers)
    std::unordered_map<TypeSymbol*, llvm::StructType*> structTypeCache_;

    // ClassSymbol* -> vtable global variable
    std::unordered_map<ClassSymbol*, llvm::GlobalVariable*> vtableCache_;

    // (ClassSymbol*, InterfaceSymbol*) -> itable global variable
    std::map<std::pair<ClassSymbol*, InterfaceSymbol*>, llvm::GlobalVariable*> itableCache_;

    // Struct cleanup functions for structs with closure-typed fields
    std::unordered_map<std::string, llvm::Function*> structCleanupCache_;

    //==========================================================================
    // Loop context (for break/continue, supports labeled loops)
    //==========================================================================
    struct LoopInfo {
        std::string label;              // empty for unlabeled loops
        llvm::BasicBlock* exitBlock;
        llvm::BasicBlock* iterBlock;
        size_t raiiScopeDepth;
    };
    std::vector<LoopInfo> loopStack_;
    std::string pendingLabel_;          // set by LabeledStatement, consumed by next loop

    //==========================================================================
    // RAII scope stack
    //==========================================================================
    struct RAIIScope {
        std::vector<std::pair<llvm::Value*, llvm::Function*>> destructibles;
        std::set<Symbol*> returnedVars;
    };
    std::vector<RAIIScope> raiiScopeStack_;

    //==========================================================================
    // String RAII
    //==========================================================================
    llvm::Function* stringFreeFn_ = nullptr;
    llvm::Function* getOrCreateStringFreeFn();
    llvm::Value* emitStringConcat(llvm::Value* left, llvm::Value* right);

    //==========================================================================
    // Closure reference counting
    //==========================================================================
    llvm::Function* closureRetainFn_ = nullptr;
    llvm::Function* closureReleaseFn_ = nullptr;
    llvm::Function* closureReleaseWrapperFn_ = nullptr;
    int closureCleanupCounter_ = 0;
    int lambdaCounter_ = 0;

    llvm::Function* getOrCreateClosureRetainFn();
    llvm::Function* getOrCreateClosureReleaseFn();
    llvm::Function* getOrCreateClosureReleaseWrapper();
    llvm::Function* generateClosureCleanupFn(
        llvm::StructType* closureTy,
        const std::vector<SymbolPtr>& capturedVars,
        int headerOffset,
        const std::vector<CaptureMode>* captureModes = nullptr);
    llvm::Function* getOrCreateStructCleanupFn(StructSymbol* structSym);

    //==========================================================================
    // Core helpers
    //==========================================================================

    // Map a V2 TypeSymbol to an LLVM Type
    llvm::Type* mapType(TypeSymbol* type);
    llvm::Type* mapType(const TypeSymbolPtr& type);

    // Map a parameter type (handles struct-by-ptr, ref-by-ptr, interface-by-fatptr)
    llvm::Type* mapParamType(TypeSymbol* type, bool isReference);

    // Get the canonical fat pointer type { ptr, ptr }
    llvm::StructType* getFatPtrType();

    // Get the LLVM struct type for a user type
    llvm::StructType* getStructType(TypeSymbol* type);

    // Generate a mangled name for a symbol
    std::string mangleName(Symbol* sym);

    // Map OverloadableOp to string suffix
    static std::string opToString(OverloadableOp op);

    // Create an alloca in the entry block of a function
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* fn,
                                              llvm::Type* type,
                                              const std::string& name);

    // Emit an expression as an lvalue (returns pointer, not loaded value)
    llvm::Value* emitLValue(ExpressionBaseNode& expr);

    // Emit a GEP to a class/struct field via currentThisPtr_
    // Returns nullptr if not in a method context or symbol is not a field
    llvm::Value* emitFieldGEP(VariableSymbol* fieldSym);

    //==========================================================================
    // RAII helpers
    //==========================================================================
    void pushRAIIScope();
    void popRAIIScope();
    void registerRAII(llvm::Value* ptr, llvm::Function* dtor);
    void emitScopeDestructors();
    void emitReturnDestructors();
    void emitBreakDestructors(size_t targetDepth);

    //==========================================================================
    // Forward declaration phase (Phase A)
    //==========================================================================
    void declareStructTypes(ProgramNode& program);
    void declareExternFunctions(ProgramNode& program);
    void declareFunctions(ProgramNode& program);
    void declareVtables(ProgramNode& program);
    void declareItables(ProgramNode& program);

    // Helpers for forward declarations
    void declareStructTypeForSymbol(StructSymbol* sym);
    void declareClassTypeForSymbol(ClassSymbol* sym);
    void declareFunctionSymbol(FunctionSymbol* sym);
    void declareOperatorSymbol(OperatorSymbol* sym);

    // Build an LLVM function type for a function symbol
    llvm::FunctionType* buildFunctionType(FunctionSymbol* sym);
    llvm::FunctionType* buildOperatorType(OperatorSymbol* sym);

    // Inheritance helpers
    int getFieldGEPIndex(ClassSymbol* cls, VariableSymbol* field);
    void storeVtablePtr(llvm::Value* objPtr, ClassSymbol* cls);

    // Wrap a class pointer into an interface fat pointer { objPtr, itablePtr }
    llvm::Value* emitWrapToInterfacePtr(llvm::Value* objPtr,
                                         ClassSymbol* cls,
                                         InterfaceSymbol* iface);

    //==========================================================================
    // V2 type query helpers (replacing V1's Type*-based helpers)
    //==========================================================================
    static bool isIntegerKind(TypeSymbol* t);
    static bool isFloatingKind(TypeSymbol* t);
    static bool isBoolKind(TypeSymbol* t);
    static bool isStringKind(TypeSymbol* t);
    static bool isPointerKind(TypeSymbol* t);
    static bool isUserStructKind(TypeSymbol* t);
    static bool isEnumKind(TypeSymbol* t);
    static bool isFunctionKind(TypeSymbol* t);
};

} // namespace codegen
} // namespace mingus
