//================================================================================
// MINGUS v1 - LLVM IR Generator
// Walks the semantically-annotated AST and emits LLVM IR.
// Relies on all 4 semantic passes having completed successfully.
//================================================================================

#pragma once

#include "mingus/ast/ASTVisitor.h"
#include "mingus/ast/ASTNode.h"
#include "mingus/ast/Program.h"
#include "mingus/ast/Declarations.h"
#include "mingus/ast/Statements.h"
#include "mingus/ast/Expressions.h"
#include "mingus/ast/Patterns.h"
#include "mingus/ast/TypeNode.h"
#include "mingus/ast/Type.h"
#include "mingus/sema/SymbolTable.h"
#include "mingus/sema/TypeRegistry.h"

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

#include <unordered_map>
#include <map>
#include <set>
#include <string>
#include <memory>

namespace mingus {
namespace codegen {

using namespace mingus::ast;

//================================================================================
// IRGenerator — ASTVisitor that emits LLVM IR
//================================================================================
class IRGenerator : public ASTVisitor {
public:
    IRGenerator(sema::SymbolTable& symbolTable, sema::TypeRegistry& registry);

    // Entry point — generates LLVM IR and returns the module
    std::unique_ptr<llvm::Module> generate(ProgramNode& program);

    // Access the LLVMContext (needed by the IR tool for verification)
    llvm::LLVMContext& getContext() { return context_; }

    //==========================================================================
    // ASTVisitor overrides (all 62)
    //==========================================================================

    // Program structure
    void visit(ProgramNode& node) override;
    void visit(ModuleNode& node) override;
    void visit(ImportNode& node) override;

    // Type nodes (no-op in codegen)
    void visit(TypeNode& node) override;
    void visit(PrimitiveTypeNode& node) override;
    void visit(NamedTypeNode& node) override;
    void visit(PointerTypeNode& node) override;
    void visit(ArrayTypeNode& node) override;
    void visit(TupleTypeNode& node) override;
    void visit(FunctionTypeNode& node) override;

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
    void visit(ParameterNode& node) override;

    // Statements
    void visit(BlockStatement& node) override;
    void visit(ExpressionStatement& node) override;
    void visit(ReturnStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(SwitchStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(BreakStatement& node) override;
    void visit(ContinueStatement& node) override;
    void visit(DeleteStatement& node) override;
    void visit(RawBlock& node) override;

    // Expressions
    void visit(IntegerLiteral& node) override;
    void visit(FloatLiteral& node) override;
    void visit(BoolLiteral& node) override;
    void visit(CharLiteral& node) override;
    void visit(StringLiteral& node) override;
    void visit(InterpolatedString& node) override;
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
    void visit(AlignOfExpression& node) override;
    void visit(PipeExpression& node) override;
    void visit(MatchExpression& node) override;
    void visit(TupleExpression& node) override;
    void visit(LambdaExpression& node) override;

    // Patterns
    void visit(LiteralPattern& node) override;
    void visit(RangePattern& node) override;
    void visit(WildcardPattern& node) override;
    void visit(BindingPattern& node) override;
    void visit(TuplePattern& node) override;
    void visit(GuardedPattern& node) override;

private:
    //==========================================================================
    // LLVM infrastructure
    //==========================================================================
    llvm::LLVMContext context_;
    std::unique_ptr<llvm::Module> module_;
    llvm::IRBuilder<> builder_;

    //==========================================================================
    // Semantic analysis data
    //==========================================================================
    sema::SymbolTable& symbolTable_;
    sema::TypeRegistry& registry_;

    //==========================================================================
    // Codegen state
    //==========================================================================
    llvm::Function* currentFunction_;
    llvm::Value* currentThisPtr_;
    sema::TypeSymbol* currentType_;
    llvm::Value* lastValue_;                // result of the last expression visit

    //==========================================================================
    // Caches
    //==========================================================================
    std::unordered_map<sema::Symbol*, llvm::Value*> namedValues_;
    std::unordered_map<const Type*, llvm::Type*> typeCache_;
    std::unordered_map<sema::Symbol*, llvm::Function*> functionCache_;
    std::unordered_map<std::string, llvm::GlobalVariable*> stringConstants_;

    // Maps user-type TypeInfo* → LLVM StructType* (for GEP with opaque pointers)
    std::unordered_map<const Type*, llvm::StructType*> structTypeCache_;

    // Maps ClassSymbol* → vtable global variable
    std::unordered_map<sema::ClassSymbol*, llvm::GlobalVariable*> vtableCache_;

    // Maps (ClassSymbol*, InterfaceSymbol*) → itable global variable
    std::map<std::pair<sema::ClassSymbol*, sema::InterfaceSymbol*>, llvm::GlobalVariable*> itableCache_;

    //==========================================================================
    // Loop context (for break/continue)
    //==========================================================================
    llvm::BasicBlock* loopExitBlock_;
    llvm::BasicBlock* loopIterBlock_;
    size_t loopRAIIScopeDepth_ = 0;  // RAII stack depth at loop entry

    //==========================================================================
    // RAII scope stack
    //==========================================================================
    struct RAIIScope {
        std::vector<std::pair<llvm::Value*, llvm::Function*>> destructibles;
        std::set<sema::Symbol*> returnedVars;
    };
    std::vector<RAIIScope> raiiScopeStack_;

    //==========================================================================
    // String RAII
    //==========================================================================
    llvm::Function* stringFreeFn_ = nullptr;
    llvm::Function* getOrCreateStringFreeFn();

    // Emit string concatenation: malloc + strcpy + strcat, returns new ptr
    llvm::Value* emitStringConcat(llvm::Value* left, llvm::Value* right);

    //==========================================================================
    // Closure reference counting
    //==========================================================================
    llvm::Function* closureRetainFn_ = nullptr;
    llvm::Function* closureReleaseFn_ = nullptr;
    llvm::Function* closureReleaseWrapperFn_ = nullptr;
    int closureCleanupCounter_ = 0;

    // Increment refcount on a closure environment pointer (null-safe)
    llvm::Function* getOrCreateClosureRetainFn();

    // Decrement refcount; free + call cleanup when it hits zero (null-safe)
    llvm::Function* getOrCreateClosureReleaseFn();

    // RAII wrapper: loads fat pointer from alloca, extracts envPtr, calls release
    llvm::Function* getOrCreateClosureReleaseWrapper();

    // Generate a per-closure cleanup function that releases captured closures
    llvm::Function* generateClosureCleanupFn(
        llvm::StructType* closureTy,
        const std::vector<sema::Symbol*>& capturedVars,
        int headerOffset);

    //==========================================================================
    // Lambda counter
    //==========================================================================
    int lambdaCounter_;

    //==========================================================================
    // Current module name (for name mangling)
    //==========================================================================
    std::string currentModuleName_;

    //==========================================================================
    // Scope navigation (same pattern as sema passes)
    //==========================================================================
    sema::Scope* currentScope_;
    std::vector<size_t> childIndexStack_;

    void enterNamedScope(sema::Scope* scope);
    void leaveNamedScope();
    void enterNextChildScope();
    void leaveChildScope();

    //==========================================================================
    // Core helpers
    //==========================================================================

    // Map a Mingus Type* to an LLVM Type*
    llvm::Type* mapType(const Type* type);
    llvm::Type* mapType(const TypePtr<Type>& type);

    // Get the canonical fat pointer type { ptr, ptr } for function-typed values
    llvm::StructType* getFatPtrType();

    // Get the LLVM struct type for a user type (for GEP operations)
    llvm::StructType* getStructType(const Type* type);

    // Generate a mangled name for a symbol
    std::string mangleName(sema::Symbol* sym);

    // Map OverloadableOp to string suffix
    static std::string opToString(sema::OverloadableOp op);

    // Create an alloca in the entry block of a function
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* fn,
                                              llvm::Type* type,
                                              const std::string& name);

    // Emit an expression as an lvalue (returns pointer, not loaded value)
    llvm::Value* emitLValue(ExpressionNode& expr);

    //==========================================================================
    // RAII helpers
    //==========================================================================
    void pushRAIIScope();
    void popRAIIScope();
    void registerRAII(llvm::Value* ptr, llvm::Function* dtor);
    void emitScopeDestructors();
    void emitReturnDestructors();
    void emitBreakDestructors();

    //==========================================================================
    // Forward declaration phase (Phase A)
    //==========================================================================
    void declareStructTypes();
    void declareExternFunctions();
    void declareFunctions();
    void declareVtables();

    // Helpers for forward declarations
    void declareStructTypeForSymbol(sema::StructSymbol* sym);
    void declareClassTypeForSymbol(sema::ClassSymbol* sym);
    void declareFunctionSymbol(sema::FunctionSymbol* sym);
    void declareOperatorSymbol(sema::OperatorSymbol* sym);
    void declareItables();

    // Wrap a class pointer into an interface fat pointer { objPtr, itablePtr }
    llvm::Value* emitWrapToInterfacePtr(llvm::Value* objPtr,
                                         sema::ClassSymbol* cls,
                                         sema::InterfaceSymbol* iface);

    // Build an LLVM function type for a function symbol
    llvm::FunctionType* buildFunctionType(sema::FunctionSymbol* sym);
    llvm::FunctionType* buildOperatorType(sema::OperatorSymbol* sym);

    // Inheritance helpers
    int getFieldGEPIndex(sema::ClassSymbol* cls, sema::VariableSymbol* field);
    void storeVtablePtr(llvm::Value* objPtr, sema::ClassSymbol* cls);
};

} // namespace codegen
} // namespace mingus
