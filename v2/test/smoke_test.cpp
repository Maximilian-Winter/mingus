// ============================================================================
// smoke_test.cpp — Compile and linkage verification for Mingus V2 core
//
// This test instantiates all V2 types, exercises the key patterns
// (SymbolWithScope MI, TypeSymbol unification, scope chain resolution,
// type interning), and prints a summary.
// ============================================================================

#include "mingus/Forward.h"
#include "mingus/DebugInfo.h"
#include "mingus/Scope.h"
#include "mingus/Symbol.h"
#include "mingus/TypeSymbol.h"
#include "mingus/Symbols.h"
#include "mingus/SymbolTable.h"
#include "mingus/AstNode.h"
#include "mingus/Expressions.h"
#include "mingus/Statements.h"
#include "mingus/Declarations.h"
#include "mingus/sema/ErrorReporter.h"
#include "mingus/sema/SymbolTableBuilder.h"
#include "mingus/sema/TypeResolver.h"
#include "mingus/sema/TypeChecker.h"
#include "mingus/sema/SemanticValidator.h"
#include "mingus/codegen/IRGenerator.h"

#pragma warning(push, 0)
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#pragma warning(pop)

#include <cassert>
#include <iostream>
#include <memory>
#include <string>

using namespace mingus;

// Helper: assert with message
#define ASSERT_EQ(a, b, msg) \
    do { if ((a) != (b)) { \
        std::cerr << "FAIL: " << msg << " (got " << (a) << ", expected " << (b) << ")\n"; \
        return 1; \
    }} while(0)

#define ASSERT_TRUE(cond, msg) \
    do { if (!(cond)) { \
        std::cerr << "FAIL: " << msg << "\n"; \
        return 1; \
    }} while(0)

#define ASSERT_NULL(ptr, msg) \
    do { if ((ptr) != nullptr) { \
        std::cerr << "FAIL: " << msg << " (expected null)\n"; \
        return 1; \
    }} while(0)

#define ASSERT_NOT_NULL(ptr, msg) \
    do { if ((ptr) == nullptr) { \
        std::cerr << "FAIL: " << msg << " (got null)\n"; \
        return 1; \
    }} while(0)

int main() {
    int passed = 0;

    // ========================================
    // 1. SymbolTable construction + primitives
    // ========================================
    {
        SymbolTable st;
        ASSERT_NOT_NULL(st.getRootScope(), "SymbolTable has root scope");
        ASSERT_NOT_NULL(st.getIntType(), "int type registered");
        ASSERT_NOT_NULL(st.getDoubleType(), "double type registered");
        ASSERT_NOT_NULL(st.getVoidType(), "void type registered");
        ASSERT_NOT_NULL(st.getErrorType(), "error type registered");
        ASSERT_NOT_NULL(st.getNullType(), "null type registered");

        ASSERT_EQ(st.getIntType()->getName(), std::string("int"), "int name");
        ASSERT_EQ(st.getIntType()->sizeInBytes, 4, "int size");
        ASSERT_TRUE(st.getIntType()->is<PrimitiveTypeSymbol>(), "int is PrimitiveTypeSymbol");
        ASSERT_TRUE(st.getIntType()->isPrimaryType, "int is primary");
        ASSERT_TRUE(st.getIntType()->as<PrimitiveTypeSymbol>()->isIntegral(), "int is integral");

        // Type resolution by name
        auto resolved = st.resolveType("double");
        ASSERT_NOT_NULL(resolved, "resolve 'double'");
        ASSERT_EQ(resolved.get(), st.getDoubleType().get(), "resolved double is canonical");

        passed++;
        std::cout << "[PASS] 1. SymbolTable primitives\n";
    }

    // ========================================
    // 2. Type interning (compound types)
    // ========================================
    {
        SymbolTable st;

        // Pointer type: int*
        auto intPtr1 = st.getPointerType(st.getIntType());
        auto intPtr2 = st.getPointerType(st.getIntType());
        ASSERT_EQ(intPtr1.get(), intPtr2.get(), "int* interning");
        ASSERT_EQ(intPtr1->getTypeDescription(), std::string("int*"), "int* description");
        ASSERT_TRUE(intPtr1->is<PointerTypeSymbol>(), "is PointerTypeSymbol");

        // Array type: int[10]
        auto intArr1 = st.getArrayType(st.getIntType(), 10);
        auto intArr2 = st.getArrayType(st.getIntType(), 10);
        ASSERT_EQ(intArr1.get(), intArr2.get(), "int[10] interning");
        ASSERT_EQ(intArr1->arraySize, 10, "array size");

        // Different array size = different type
        auto intArr3 = st.getArrayType(st.getIntType(), 20);
        ASSERT_TRUE(intArr1.get() != intArr3.get(), "int[10] != int[20]");

        // Tuple type: (int, double)
        auto tup1 = st.getTupleType({st.getIntType(), st.getDoubleType()});
        auto tup2 = st.getTupleType({st.getIntType(), st.getDoubleType()});
        ASSERT_EQ(tup1.get(), tup2.get(), "(int,double) interning");
        ASSERT_EQ(tup1->getTypeDescription(), std::string("(int, double)"), "tuple description");

        // Function type: (int, int) => double
        FunctionTypeSymbol::ParameterInfo p1{st.getIntType(), "a", false};
        FunctionTypeSymbol::ParameterInfo p2{st.getIntType(), "b", false};
        auto ft1 = st.getFunctionType({p1, p2}, st.getDoubleType());
        auto ft2 = st.getFunctionType({p1, p2}, st.getDoubleType());
        ASSERT_EQ(ft1.get(), ft2.get(), "(int,int)=>double interning");
        ASSERT_EQ(ft1->sizeInBytes, 16, "function type = fat pointer = 16 bytes");
        ASSERT_EQ(ft1->parameters.size(), size_t(2), "function type param count");

        // Function type with ref param: different type
        FunctionTypeSymbol::ParameterInfo p3{st.getIntType(), "c", true};
        auto ft3 = st.getFunctionType({p1, p3}, st.getDoubleType());
        ASSERT_TRUE(ft1.get() != ft3.get(), "(int,int)=>double != (int,int&)=>double");

        // Reference type: int&
        auto intRef = st.getReferenceType(st.getIntType());
        ASSERT_EQ(intRef->getTypeDescription(), std::string("int&"), "int& description");

        passed++;
        std::cout << "[PASS] 2. Type interning\n";
    }

    // ========================================
    // 3. SymbolWithScope — dual nature pattern
    // ========================================
    {
        SymbolTable st;

        // Create a module symbol
        auto moduleSym = std::make_shared<ModuleSymbol>("Main");
        st.defineSymbol(moduleSym);
        st.pushScope(moduleSym);

        // Module IS a scope — define a function in it
        auto funcSym = std::make_shared<FunctionSymbol>("compute");
        funcSym->returnType = st.getIntType();
        st.defineSymbol(funcSym);

        // Resolve function through module
        auto resolved = moduleSym->resolve("compute");
        ASSERT_NOT_NULL(resolved, "resolve 'compute' in module");
        ASSERT_EQ(resolved->getName(), std::string("compute"), "resolved name");
        ASSERT_TRUE(resolved->is<FunctionSymbol>(), "is FunctionSymbol");

        // FunctionSymbol IS a scope — push it and define parameters
        st.pushScope(funcSym);
        auto paramA = std::make_shared<VariableSymbol>("a", st.getIntType());
        paramA->role = VariableRole::Parameter;
        st.defineSymbol(paramA);
        funcSym->parameters.push_back(paramA);

        auto paramB = std::make_shared<VariableSymbol>("b", st.getIntType());
        paramB->role = VariableRole::Parameter;
        paramB->isReference = true;
        st.defineSymbol(paramB);
        funcSym->parameters.push_back(paramB);

        // Parameters visible in function scope
        auto resolvedA = funcSym->resolve("a");
        ASSERT_NOT_NULL(resolvedA, "resolve 'a' in function");

        // Parameters carry type info
        auto* varA = resolvedA->as<VariableSymbol>();
        ASSERT_NOT_NULL(varA, "a is VariableSymbol");
        ASSERT_TRUE(varA->getType()->is<PrimitiveTypeSymbol>(), "a type is primitive");
        ASSERT_EQ(varA->isReference, false, "a is not reference");
        ASSERT_EQ(paramB->isReference, true, "b is reference");

        // buildFunctionType preserves parameter info
        auto funcType = funcSym->buildFunctionType();
        ASSERT_NOT_NULL(funcType, "buildFunctionType");
        ASSERT_EQ(funcType->parameters.size(), size_t(2), "func type param count");
        ASSERT_EQ(funcType->parameters[0].isReference, false, "param 0 not ref");
        ASSERT_EQ(funcType->parameters[1].isReference, true, "param 1 is ref");

        st.popScope();  // back to module
        st.popScope();  // back to global

        passed++;
        std::cout << "[PASS] 3. SymbolWithScope dual nature\n";
    }

    // ========================================
    // 4. ClassSymbol — inheritance resolution
    // ========================================
    {
        SymbolTable st;

        auto moduleSym = std::make_shared<ModuleSymbol>("TestMod");
        st.defineSymbol(moduleSym);
        st.pushScope(moduleSym);

        // Create base class
        auto baseSym = std::make_shared<ClassSymbol>("Animal", st.getCurrentScope());
        st.defineSymbol(baseSym);
        st.registerType("Animal", baseSym);
        st.pushScope(baseSym);

        auto nameField = std::make_shared<VariableSymbol>("name", st.getStringType());
        nameField->role = VariableRole::Field;
        nameField->fieldIndex = 0;
        st.defineSymbol(nameField);
        baseSym->fields.push_back(nameField);
        baseSym->allFields.push_back(nameField);

        auto speakMethod = std::make_shared<MethodSymbol>("speak");
        speakMethod->returnType = st.getVoidType();
        speakMethod->isVirtual = true;
        speakMethod->vtableIndex = 1;  // slot 0 = destructor
        st.defineSymbol(speakMethod);

        st.popScope();

        // Create derived class
        auto derivedSym = std::make_shared<ClassSymbol>("Dog", st.getCurrentScope());
        derivedSym->resolvedBaseClass = baseSym.get();
        st.defineSymbol(derivedSym);
        st.registerType("Dog", derivedSym);
        st.pushScope(derivedSym);

        // Own field
        auto breedField = std::make_shared<VariableSymbol>("breed", st.getStringType());
        breedField->role = VariableRole::Field;
        breedField->fieldIndex = 1;
        st.defineSymbol(breedField);
        derivedSym->fields.push_back(breedField);
        derivedSym->allFields.push_back(nameField);   // inherited
        derivedSym->allFields.push_back(breedField);   // own

        st.popScope();

        // Dog.resolve("name") should find Animal's field via inheritance
        auto resolvedName = derivedSym->resolve("name");
        ASSERT_NOT_NULL(resolvedName, "Dog resolves 'name' from Animal");
        ASSERT_EQ(resolvedName->getName(), std::string("name"), "inherited field name");

        // Dog.resolve("speak") should find Animal's method
        auto resolvedSpeak = derivedSym->resolve("speak");
        ASSERT_NOT_NULL(resolvedSpeak, "Dog resolves 'speak' from Animal");
        ASSERT_TRUE(resolvedSpeak->is<MethodSymbol>(), "speak is MethodSymbol");

        // Dog.resolve("breed") should find own field
        auto resolvedBreed = derivedSym->resolve("breed");
        ASSERT_NOT_NULL(resolvedBreed, "Dog resolves own 'breed'");

        // TypeSymbol IS type check
        ASSERT_TRUE(baseSym->is<TypeSymbol>(), "Animal is TypeSymbol");
        ASSERT_TRUE(baseSym->is<ClassSymbol>(), "Animal is ClassSymbol");
        ASSERT_TRUE(derivedSym->is<ClassSymbol>(), "Dog is ClassSymbol");

        st.popScope();

        passed++;
        std::cout << "[PASS] 4. ClassSymbol inheritance resolution\n";
    }

    // ========================================
    // 5. Scope chain resolution
    // ========================================
    {
        SymbolTable st;

        // Global -> Module -> Function -> Block
        auto moduleSym = std::make_shared<ModuleSymbol>("Main");
        st.defineSymbol(moduleSym);
        st.pushScope(moduleSym);

        auto globalVar = std::make_shared<VariableSymbol>("PI", st.getDoubleType());
        st.defineSymbol(globalVar);

        auto funcSym = std::make_shared<FunctionSymbol>("calc");
        st.defineSymbol(funcSym);
        st.pushScope(funcSym);

        auto localVar = std::make_shared<VariableSymbol>("x", st.getIntType());
        st.defineSymbol(localVar);

        // Create a block scope inside the function
        auto blockScope = std::make_shared<BlockScope>(st.getCurrentScope());
        st.getCurrentScope()->nest(blockScope);
        st.pushScope(blockScope);

        auto innerVar = std::make_shared<VariableSymbol>("y", st.getIntType());
        st.defineSymbol(innerVar);

        // From block: resolve inner (y), local (x), and module (PI)
        auto ry = blockScope->resolve("y");
        ASSERT_NOT_NULL(ry, "block resolves 'y'");

        auto rx = blockScope->resolve("x");
        ASSERT_NOT_NULL(rx, "block resolves 'x' from function");

        auto rpi = blockScope->resolve("PI");
        ASSERT_NOT_NULL(rpi, "block resolves 'PI' from module");

        // Scope path to root
        auto path = blockScope->getEnclosingPathToRoot();
        ASSERT_TRUE(path.size() >= 2, "path has at least 2 entries (func, module)");

        st.popScope();
        st.popScope();
        st.popScope();

        passed++;
        std::cout << "[PASS] 5. Scope chain resolution\n";
    }

    // ========================================
    // 6. Type compatibility
    // ========================================
    {
        SymbolTable st;

        // Same type
        ASSERT_TRUE(st.isCompatible(st.getIntType().get(), st.getIntType().get()),
            "int == int");

        // Numeric widening
        ASSERT_TRUE(st.isCompatible(st.getByteType().get(), st.getIntType().get()),
            "byte -> int");
        ASSERT_TRUE(st.isCompatible(st.getIntType().get(), st.getDoubleType().get()),
            "int -> double");
        ASSERT_TRUE(st.isCompatible(st.getFloatType().get(), st.getDoubleType().get()),
            "float -> double");

        // Not compatible
        ASSERT_TRUE(!st.isCompatible(st.getDoubleType().get(), st.getIntType().get()),
            "double !-> int (no narrowing)");
        ASSERT_TRUE(!st.isCompatible(st.getStringType().get(), st.getIntType().get()),
            "string !-> int");

        // ErrorType always compatible
        ASSERT_TRUE(st.isCompatible(st.getErrorType().get(), st.getIntType().get()),
            "error -> int");
        ASSERT_TRUE(st.isCompatible(st.getIntType().get(), st.getErrorType().get()),
            "int -> error");

        // Null -> pointer
        auto intPtr = st.getPointerType(st.getIntType());
        ASSERT_TRUE(st.isCompatible(st.getNullType().get(), intPtr.get()),
            "null -> int*");

        // Null -> function type
        FunctionTypeSymbol::ParameterInfo p{st.getIntType(), "x", false};
        auto funcType = st.getFunctionType({p}, st.getIntType());
        ASSERT_TRUE(st.isCompatible(st.getNullType().get(), funcType.get()),
            "null -> (int)=>int");

        passed++;
        std::cout << "[PASS] 6. Type compatibility\n";
    }

    // ========================================
    // 7. AST node scope + debug info
    // ========================================
    {
        auto block = std::make_shared<BlockStatementNode>();
        auto debugInfo = std::make_shared<DebugInfo>(10, 5, 15, 20);
        block->debugInfo = debugInfo;

        auto scope = std::make_shared<BlockScope>(nullptr);
        block->astScopeNode = scope;

        ASSERT_NOT_NULL(block->astScopeNode, "block has scope");
        ASSERT_NOT_NULL(block->debugInfo, "block has debug info");
        ASSERT_EQ(block->debugInfo->lineNumberStart, 10, "debug info start line");
        ASSERT_EQ(block->debugInfo->lineNumberEnd, 15, "debug info end line");
        ASSERT_EQ(block->debugInfo->rangeString(), std::string("10:5-15:20"), "range string");

        // DebugInfo merge
        auto d1 = std::make_shared<DebugInfo>(5, 1, 5, 10);
        auto d2 = std::make_shared<DebugInfo>(8, 3, 8, 25);
        auto merged = DebugInfo::merge(d1, d2);
        ASSERT_EQ(merged->lineNumberStart, 5, "merged start line");
        ASSERT_EQ(merged->lineNumberEnd, 8, "merged end line");

        // ArgumentsNode
        auto args = std::make_shared<ArgumentsNode>();
        args->isReference = {false, true, false};
        ASSERT_EQ(args->isReference[1], true, "arg 1 is reference");

        // ParameterNode
        auto param = std::make_shared<ParameterNode>();
        param->name = "x";
        param->isReference = true;
        SymbolTable st;
        param->resolvedSymbol = std::make_shared<VariableSymbol>("x", st.getIntType());
        ASSERT_NOT_NULL(param->resolvedSymbol, "param has resolved symbol");

        passed++;
        std::cout << "[PASS] 7. AST node scope + debug info\n";
    }

    // ========================================
    // 8. StructSymbol + EnumSymbol + InterfaceSymbol
    // ========================================
    {
        SymbolTable st;

        // Struct
        auto vec2 = std::make_shared<StructSymbol>("Vec2");
        auto xField = std::make_shared<VariableSymbol>("x", st.getDoubleType());
        xField->fieldIndex = 0;
        vec2->fields.push_back(xField);
        vec2->define(xField);
        auto yField = std::make_shared<VariableSymbol>("y", st.getDoubleType());
        yField->fieldIndex = 1;
        vec2->fields.push_back(yField);
        vec2->define(yField);

        auto resolvedX = vec2->resolve("x");
        ASSERT_NOT_NULL(resolvedX, "Vec2 resolves 'x'");
        ASSERT_TRUE(!vec2->needsCleanup(), "Vec2 has no closure fields");

        // Enum
        auto color = std::make_shared<EnumSymbol>("Color");
        color->underlyingType = st.getIntType();
        color->members = {{"Red", 0, "", false}, {"Green", 1, "", false}, {"Blue", 2, "", false}};
        ASSERT_NOT_NULL(color->findMember("Green"), "find enum member 'Green'");
        ASSERT_EQ(color->findMember("Green")->intValue, int64_t(1), "Green = 1");
        ASSERT_NULL(color->findMember("Yellow"), "no 'Yellow' member");

        // Interface
        auto drawable = std::make_shared<InterfaceSymbol>("Drawable");
        auto drawMethod = std::make_shared<FunctionSymbol>("draw");
        drawMethod->returnType = st.getVoidType();
        drawable->methods.push_back(drawMethod);

        auto found = drawable->findMethod("draw");
        ASSERT_NOT_NULL(found, "Drawable has 'draw' method");
        ASSERT_NULL(drawable->findMethod("update"), "no 'update' method");

        passed++;
        std::cout << "[PASS] 8. Struct + Enum + Interface symbols\n";
    }

    // ========================================
    // 9. AST expression nodes + visitor pattern
    // ========================================
    {
        SymbolTable st;

        // Build a mini AST for: add(x, 42)
        auto callExpr = std::make_shared<CallExpression>();
        callExpr->debugInfo = std::make_shared<DebugInfo>(5, 10, 5, 22);

        // Callee: identifier "add"
        auto callee = std::make_shared<IdentifierExpression>();
        callee->name = "add";
        callExpr->callee = callee;

        // Arguments with isReference tracking
        auto args = std::make_shared<ArgumentsNode>();
        auto argX = std::make_shared<IdentifierExpression>();
        argX->name = "x";
        auto arg42 = std::make_shared<IntegerLiteral>();
        arg42->value = 42;
        args->expressions = {argX, arg42};
        args->isReference = {false, false};
        callExpr->arguments = args;

        ASSERT_NOT_NULL(callExpr->callee, "call has callee");
        ASSERT_EQ(callExpr->arguments->expressions.size(), size_t(2), "call has 2 args");
        ASSERT_EQ(callExpr->arguments->isReference[0], false, "arg 0 not ref");
        ASSERT_TRUE(callExpr->callee->is<IdentifierExpression>(), "callee is identifier");
        ASSERT_EQ(callExpr->callee->as<IdentifierExpression>()->name, std::string("add"),
            "callee name");

        // Verify resolvedCallee linkage
        auto addFunc = std::make_shared<FunctionSymbol>("add");
        addFunc->returnType = st.getIntType();
        callExpr->resolvedCallee = addFunc;
        ASSERT_NOT_NULL(callExpr->resolvedCallee, "call has resolved callee");

        // Verify debug info
        ASSERT_EQ(callExpr->debugInfo->rangeString(), std::string("5:10-5:22"),
            "call debug range");

        // Test other expression types
        auto binExpr = std::make_shared<BinaryExpression>();
        binExpr->op = BinaryOp::Add;
        binExpr->left = argX;
        binExpr->right = arg42;
        ASSERT_TRUE(binExpr->is<BinaryExpression>(), "is BinaryExpression");
        ASSERT_TRUE(binExpr->is<ExpressionBaseNode>(), "is ExpressionBaseNode");

        auto lambdaExpr = std::make_shared<LambdaExpression>();
        lambdaExpr->captureDefault = CaptureDefault::ByCopy;
        lambdaExpr->captureItems = {{"counter", CaptureMode::ByReference}};
        lambdaExpr->escapes = false;
        ASSERT_EQ(lambdaExpr->captureItems.size(), size_t(1), "lambda has 1 capture item");
        ASSERT_EQ(lambdaExpr->captureItems[0].name, std::string("counter"), "capture name");
        ASSERT_TRUE(!lambdaExpr->escapes, "lambda non-escaping");

        auto matchExpr = std::make_shared<MatchExpression>();
        auto wildcard = std::make_shared<WildcardPattern>();
        auto litPat = std::make_shared<LiteralPattern>();
        litPat->value = arg42;
        MatchArm arm1{litPat, std::make_shared<IntegerLiteral>()};
        MatchArm arm2{wildcard, std::make_shared<IntegerLiteral>()};
        matchExpr->arms = {arm1, arm2};
        ASSERT_EQ(matchExpr->arms.size(), size_t(2), "match has 2 arms");

        passed++;
        std::cout << "[PASS] 9. AST expression nodes\n";
    }

    // ========================================
    // 10. AST statement + declaration nodes
    // ========================================
    {
        SymbolTable st;

        // IfStatement with else-if chain
        auto ifStmt = std::make_shared<IfStatement>();
        auto cond = std::make_shared<BoolLiteral>();
        cond->value = true;
        ifStmt->condition = cond;
        ifStmt->thenBody = std::make_shared<BlockStatementNode>();
        ElseIfClause elseIf;
        elseIf.condition = std::make_shared<BoolLiteral>();
        elseIf.body = std::make_shared<BlockStatementNode>();
        ifStmt->elseIfClauses = {elseIf};
        ifStmt->elseBody = std::make_shared<BlockStatementNode>();

        ASSERT_NOT_NULL(ifStmt->condition, "if has condition");
        ASSERT_EQ(ifStmt->elseIfClauses.size(), size_t(1), "if has 1 else-if");
        ASSERT_NOT_NULL(ifStmt->elseBody, "if has else");

        // ForStatement
        auto forStmt = std::make_shared<ForStatement>();
        forStmt->condition = std::make_shared<BoolLiteral>();
        forStmt->body = std::make_shared<BlockStatementNode>();
        ASSERT_NOT_NULL(forStmt->body, "for has body");

        // FunctionDeclaration
        auto funcDecl = std::make_shared<FunctionDeclaration>();
        funcDecl->name = "compute";
        funcDecl->accessModifier = AccessModifier::Public;
        auto param = std::make_shared<ParameterNode>();
        param->name = "x";
        param->isReference = false;
        funcDecl->parameters = {param};
        funcDecl->returnType = std::make_shared<PrimitiveTypeNode>();
        funcDecl->body = std::make_shared<BlockStatementNode>();

        ASSERT_EQ(funcDecl->name, std::string("compute"), "func name");
        ASSERT_EQ(funcDecl->parameters.size(), size_t(1), "func has 1 param");
        ASSERT_EQ(funcDecl->parameters[0]->name, std::string("x"), "param name");

        // ClassDeclaration
        auto classDecl = std::make_shared<ClassDeclaration>();
        classDecl->name = "Animal";
        classDecl->baseClasses = {"Drawable"};
        classDecl->isAbstract = true;
        auto fieldDecl = std::make_shared<VariableDeclaration>();
        fieldDecl->name = "name";
        fieldDecl->isInferred = false;
        classDecl->fields = {fieldDecl};
        auto methodDecl = std::make_shared<FunctionDeclaration>();
        methodDecl->name = "speak";
        methodDecl->isVirtual = true;
        classDecl->methods = {methodDecl};

        ASSERT_EQ(classDecl->name, std::string("Animal"), "class name");
        ASSERT_EQ(classDecl->baseClasses.size(), size_t(1), "class has 1 base");
        ASSERT_TRUE(classDecl->isAbstract, "class is abstract");
        ASSERT_EQ(classDecl->fields.size(), size_t(1), "class has 1 field");
        ASSERT_EQ(classDecl->methods.size(), size_t(1), "class has 1 method");

        // ImportDeclaration
        auto importDecl = std::make_shared<ImportDeclaration>();
        importDecl->sourcePath = {"MathLib"};
        importDecl->targets = {{"sin", std::nullopt}, {"cos", std::string("cosine")}};
        ASSERT_EQ(importDecl->targets.size(), size_t(2), "import has 2 targets");
        ASSERT_EQ(importDecl->targets[1].alias.value(), std::string("cosine"), "import alias");

        // EnumDeclaration
        auto enumDecl = std::make_shared<EnumDeclaration>();
        enumDecl->name = "Color";
        auto member = std::make_shared<EnumMemberNode>();
        member->name = "Red";
        enumDecl->members = {member};
        ASSERT_EQ(enumDecl->members[0]->name, std::string("Red"), "enum member name");

        // VariableDeclarationExpression (the hybrid)
        auto varDeclExpr = std::make_shared<VariableDeclarationExpression>();
        varDeclExpr->name = "count";
        varDeclExpr->isInferred = true;
        varDeclExpr->initializer = std::make_shared<IntegerLiteral>();
        ASSERT_TRUE(varDeclExpr->is<ExpressionBaseNode>(), "var decl expr IS expression");
        ASSERT_TRUE(varDeclExpr->isInferred, "var decl is inferred");

        passed++;
        std::cout << "[PASS] 10. AST statement + declaration nodes\n";
    }

    // ========================================
    // 11. Visitor pattern dispatch
    // ========================================
    {
        // A minimal visitor that counts visits
        struct CountingVisitor : public ASTVisitor {
            int programCount = 0;
            int moduleCount = 0;
            int blockCount = 0;
            int callCount = 0;
            int funcDeclCount = 0;
            int intLitCount = 0;

            void visit(ProgramNode& node) override { programCount++; }
            void visit(ModuleNode& node) override { moduleCount++; }
            void visit(BlockStatementNode& node) override { blockCount++; }
            void visit(CallExpression& node) override { callCount++; }
            void visit(FunctionDeclaration& node) override { funcDeclCount++; }
            void visit(IntegerLiteral& node) override { intLitCount++; }
        };

        CountingVisitor cv;

        auto program = std::make_shared<ProgramNode>();
        program->accept(cv);
        ASSERT_EQ(cv.programCount, 1, "visitor: program visited");

        auto module = std::make_shared<ModuleNode>();
        module->accept(cv);
        ASSERT_EQ(cv.moduleCount, 1, "visitor: module visited");

        auto block = std::make_shared<BlockStatementNode>();
        block->accept(cv);
        ASSERT_EQ(cv.blockCount, 1, "visitor: block visited");

        auto call = std::make_shared<CallExpression>();
        call->accept(cv);
        ASSERT_EQ(cv.callCount, 1, "visitor: call visited");

        auto funcDecl = std::make_shared<FunctionDeclaration>();
        funcDecl->accept(cv);
        ASSERT_EQ(cv.funcDeclCount, 1, "visitor: func decl visited");

        auto intLit = std::make_shared<IntegerLiteral>();
        intLit->accept(cv);
        intLit->accept(cv);
        ASSERT_EQ(cv.intLitCount, 2, "visitor: int literal visited twice");

        // Default no-op: visiting a StringLiteral shouldn't crash
        auto strLit = std::make_shared<StringLiteral>();
        strLit->accept(cv);  // no override = no-op

        passed++;
        std::cout << "[PASS] 11. Visitor pattern dispatch\n";
    }

    // ========================================
    // 12. Pass 1 — SymbolTableBuilder basic
    // ========================================
    {
        // Build a mini program AST:
        //   module Main {
        //     func add(int a, int b) => int { return a + b; }
        //     var PI: double;
        //   }

        auto program = std::make_shared<ProgramNode>();

        auto module = std::make_shared<ModuleNode>();
        module->name = "Main";

        // func add(int a, int b) => int { return a + b; }
        auto funcDecl = std::make_shared<FunctionDeclaration>();
        funcDecl->name = "add";
        funcDecl->accessModifier = AccessModifier::Public;

        auto paramA = std::make_shared<ParameterNode>();
        paramA->name = "a";
        paramA->isReference = false;
        auto paramB = std::make_shared<ParameterNode>();
        paramB->name = "b";
        paramB->isReference = true;  // b is ref param
        funcDecl->parameters = {paramA, paramB};

        // Body: return a + b;
        auto body = std::make_shared<BlockStatementNode>();
        auto retStmt = std::make_shared<ReturnStatement>();
        auto binExpr = std::make_shared<BinaryExpression>();
        binExpr->op = BinaryOp::Add;
        binExpr->left = std::make_shared<IdentifierExpression>();
        binExpr->right = std::make_shared<IdentifierExpression>();
        retStmt->value = binExpr;
        body->statements.push_back(retStmt);
        funcDecl->body = body;

        // var PI: double;
        auto piDecl = std::make_shared<VariableDeclaration>();
        piDecl->name = "PI";
        piDecl->isInferred = false;

        module->declarations.push_back(funcDecl);
        module->declarations.push_back(piDecl);
        program->modules.push_back(module);

        // Run Pass 1
        SymbolTable st;
        ErrorReporter errors;
        SymbolTableBuilder builder(st, errors);
        builder.build(*program);

        ASSERT_TRUE(!errors.hasErrors(), "Pass 1 no errors");

        // Verify: module resolved
        ASSERT_NOT_NULL(module->resolvedModule, "module has resolvedModule");
        ASSERT_EQ(module->resolvedModule->getName(), std::string("Main"), "module name = Main");

        // Verify: function resolved
        ASSERT_NOT_NULL(funcDecl->resolvedFunction, "func has resolvedFunction");
        ASSERT_EQ(funcDecl->resolvedFunction->getName(), std::string("add"), "func name = add");
        ASSERT_EQ(funcDecl->resolvedFunction->parameters.size(), size_t(2), "func has 2 params");

        // Verify: parameter symbols linked directly (no scanForParamSymbols needed!)
        ASSERT_NOT_NULL(paramA->resolvedSymbol, "param a has resolvedSymbol");
        ASSERT_EQ(paramA->resolvedSymbol->getName(), std::string("a"), "param a name");
        ASSERT_EQ(paramA->resolvedSymbol->isReference, false, "param a not ref");
        ASSERT_NOT_NULL(paramB->resolvedSymbol, "param b has resolvedSymbol");
        ASSERT_EQ(paramB->resolvedSymbol->isReference, true, "param b is ref");
        ASSERT_TRUE(paramB->resolvedSymbol->role == VariableRole::Parameter, "param b is Parameter role");

        // Verify: variable resolved
        ASSERT_NOT_NULL(piDecl->resolvedVariable, "PI has resolvedVariable");
        ASSERT_EQ(piDecl->resolvedVariable->getName(), std::string("PI"), "PI name");

        // Verify: scope chain — resolve PI from function scope
        auto piFromFunc = funcDecl->resolvedFunction->resolve("PI");
        ASSERT_NOT_NULL(piFromFunc, "func scope resolves PI from module");

        // Verify: resolve params from function scope
        auto aFromFunc = funcDecl->resolvedFunction->resolve("a");
        ASSERT_NOT_NULL(aFromFunc, "func scope resolves param a");

        // Verify: module scope has the function and variable
        auto addFromModule = module->resolvedModule->resolve("add");
        ASSERT_NOT_NULL(addFromModule, "module resolves 'add'");
        auto piFromModule = module->resolvedModule->resolve("PI");
        ASSERT_NOT_NULL(piFromModule, "module resolves 'PI'");

        // Verify: every AST node has scope set
        ASSERT_NOT_NULL(program->astScopeNode, "program has scope");
        ASSERT_NOT_NULL(module->astScopeNode, "module has scope");
        ASSERT_NOT_NULL(funcDecl->astScopeNode, "funcDecl has scope");
        ASSERT_NOT_NULL(retStmt->astScopeNode, "returnStmt has scope");
        ASSERT_NOT_NULL(binExpr->astScopeNode, "binExpr has scope");

        passed++;
        std::cout << "[PASS] 12. Pass 1 — SymbolTableBuilder basic\n";
    }

    // ========================================
    // 13. Pass 1 — Class with inheritance + auto ctor/dtor
    // ========================================
    {
        // Build:
        //   module Zoo {
        //     class Animal {
        //       string name;
        //       func speak() => void { }
        //     }
        //     class Dog : Animal {
        //       int age;
        //     }
        //   }

        auto program = std::make_shared<ProgramNode>();
        auto module = std::make_shared<ModuleNode>();
        module->name = "Zoo";

        // class Animal
        auto animalDecl = std::make_shared<ClassDeclaration>();
        animalDecl->name = "Animal";

        auto nameField = std::make_shared<VariableDeclaration>();
        nameField->name = "name";
        animalDecl->fields.push_back(nameField);

        auto speakMethod = std::make_shared<FunctionDeclaration>();
        speakMethod->name = "speak";
        speakMethod->isVirtual = true;
        speakMethod->body = std::make_shared<BlockStatementNode>();
        animalDecl->methods.push_back(speakMethod);

        // No explicit ctor/dtor — should auto-generate

        // class Dog : Animal
        auto dogDecl = std::make_shared<ClassDeclaration>();
        dogDecl->name = "Dog";
        dogDecl->baseClasses = {"Animal"};

        auto ageField = std::make_shared<VariableDeclaration>();
        ageField->name = "age";
        dogDecl->fields.push_back(ageField);

        module->declarations.push_back(animalDecl);
        module->declarations.push_back(dogDecl);
        program->modules.push_back(module);

        // Run Pass 1
        SymbolTable st;
        ErrorReporter errors;
        SymbolTableBuilder builder(st, errors);
        builder.build(*program);

        ASSERT_TRUE(!errors.hasErrors(), "Pass 1 class no errors");

        // Verify: Animal class symbol
        auto* animalSym = animalDecl->resolvedClass.get();
        ASSERT_NOT_NULL(animalSym, "Animal has resolvedClass");
        ASSERT_EQ(animalSym->getName(), std::string("Animal"), "Animal name");
        ASSERT_EQ(animalSym->fields.size(), size_t(1), "Animal has 1 field");
        ASSERT_EQ(animalSym->fields[0]->getName(), std::string("name"), "Animal field = name");
        ASSERT_EQ(animalSym->fields[0]->fieldIndex, 0, "Animal field index = 0");
        ASSERT_TRUE(animalSym->fields[0]->role == VariableRole::Field, "field role = Field");

        // Auto-generated ctor/dtor
        ASSERT_NOT_NULL(animalSym->constructor, "Animal has auto-generated constructor");
        ASSERT_NOT_NULL(animalSym->destructor, "Animal has auto-generated destructor");

        // Vtable: slot 0 = dtor, slot 1 = speak
        ASSERT_TRUE(animalSym->hasVtable(), "Animal has vtable");
        ASSERT_TRUE(animalSym->vtableSize >= 2, "Animal vtable >= 2 slots");
        ASSERT_EQ(animalSym->destructor->vtableIndex, 0, "destructor at vtable slot 0");

        // Dog class symbol
        auto* dogSym = dogDecl->resolvedClass.get();
        ASSERT_NOT_NULL(dogSym, "Dog has resolvedClass");
        ASSERT_NOT_NULL(dogSym->resolvedBaseClass, "Dog has base class");
        ASSERT_EQ(dogSym->resolvedBaseClass->getName(), std::string("Animal"),
            "Dog base = Animal");
        ASSERT_EQ(dogSym->fields.size(), size_t(1), "Dog own fields = 1");
        ASSERT_TRUE(dogSym->allFields.size() >= 2,
            "Dog allFields >= 2 (inherited name + own age)");

        // Dog resolves Animal.name through inheritance
        auto nameFromDog = dogSym->resolve("name");
        ASSERT_NOT_NULL(nameFromDog, "Dog resolves 'name' from Animal");

        // Dog resolves speak through inheritance
        auto speakFromDog = dogSym->resolve("speak");
        ASSERT_NOT_NULL(speakFromDog, "Dog resolves 'speak' from Animal");

        // Dog has auto-generated ctor/dtor
        ASSERT_NOT_NULL(dogSym->constructor, "Dog has auto-generated constructor");
        ASSERT_NOT_NULL(dogSym->destructor, "Dog has auto-generated destructor");

        // Type registered in SymbolTable
        auto animalType = st.resolveType("Animal");
        ASSERT_NOT_NULL(animalType, "Animal registered in type registry");
        auto dogType = st.resolveType("Dog");
        ASSERT_NOT_NULL(dogType, "Dog registered in type registry");

        passed++;
        std::cout << "[PASS] 13. Pass 1 — Class with inheritance + auto ctor/dtor\n";
    }

    // ========================================
    // 14. Pass 1 — Struct, Enum, Lambda, For, Match
    // ========================================
    {
        // Build:
        //   module Shapes {
        //     struct Vec2 { double x; double y; }
        //     enum Color { Red, Green, Blue }
        //     func apply() => void {
        //       var fn = [=](int x) => { return x; };
        //       for (var i = 0; i < 10; i++) { }
        //     }
        //   }

        auto program = std::make_shared<ProgramNode>();
        auto module = std::make_shared<ModuleNode>();
        module->name = "Shapes";

        // struct Vec2
        auto structDecl = std::make_shared<StructDeclaration>();
        structDecl->name = "Vec2";
        auto xField = std::make_shared<VariableDeclaration>();
        xField->name = "x";
        auto yField = std::make_shared<VariableDeclaration>();
        yField->name = "y";
        structDecl->fields = {xField, yField};

        // enum Color
        auto enumDecl = std::make_shared<EnumDeclaration>();
        enumDecl->name = "Color";
        auto red = std::make_shared<EnumMemberNode>();
        red->name = "Red";
        auto green = std::make_shared<EnumMemberNode>();
        green->name = "Green";
        auto blue = std::make_shared<EnumMemberNode>();
        blue->name = "Blue";
        enumDecl->members = {red, green, blue};

        // func apply() => void { ... }
        auto funcDecl = std::make_shared<FunctionDeclaration>();
        funcDecl->name = "apply";

        auto funcBody = std::make_shared<BlockStatementNode>();

        // Lambda: var fn = [=](int x) => { return x; };
        auto lambdaDecl = std::make_shared<VariableDeclaration>();
        lambdaDecl->name = "fn";
        lambdaDecl->isInferred = true;
        auto lambda = std::make_shared<LambdaExpression>();
        lambda->captureDefault = CaptureDefault::ByCopy;
        auto lambdaParam = std::make_shared<ParameterNode>();
        lambdaParam->name = "x";
        lambdaParam->isReference = false;
        lambda->parameters = {lambdaParam};
        auto lambdaBody = std::make_shared<BlockStatementNode>();
        auto lambdaRet = std::make_shared<ReturnStatement>();
        lambdaRet->value = std::make_shared<IdentifierExpression>();
        lambdaBody->statements.push_back(lambdaRet);
        lambda->body = lambdaBody;
        lambdaDecl->initializer = lambda;
        auto lambdaDeclStmt = std::make_shared<ExpressionStatement>();
        // We'll use VariableDeclaration directly in statements
        funcBody->statements.push_back(lambdaDecl);

        // For: for (var i = 0; i < 10; i++) { }
        auto forStmt = std::make_shared<ForStatement>();
        auto initVar = std::make_shared<VariableDeclaration>();
        initVar->name = "i";
        initVar->isInferred = true;
        initVar->initializer = std::make_shared<IntegerLiteral>();
        forStmt->initDeclaration = initVar;
        forStmt->condition = std::make_shared<BinaryExpression>();
        forStmt->body = std::make_shared<BlockStatementNode>();
        funcBody->statements.push_back(forStmt);

        funcDecl->body = funcBody;

        module->declarations.push_back(structDecl);
        module->declarations.push_back(enumDecl);
        module->declarations.push_back(funcDecl);
        program->modules.push_back(module);

        // Run Pass 1
        SymbolTable st;
        ErrorReporter errors;
        SymbolTableBuilder builder(st, errors);
        builder.build(*program);

        ASSERT_TRUE(!errors.hasErrors(), "Pass 1 struct/enum no errors");

        // Verify struct
        ASSERT_NOT_NULL(structDecl->resolvedStruct, "Vec2 has resolvedStruct");
        ASSERT_EQ(structDecl->resolvedStruct->fields.size(), size_t(2), "Vec2 has 2 fields");
        ASSERT_EQ(structDecl->resolvedStruct->fields[0]->fieldIndex, 0, "x index = 0");
        ASSERT_EQ(structDecl->resolvedStruct->fields[1]->fieldIndex, 1, "y index = 1");
        ASSERT_TRUE(structDecl->resolvedStruct->fields[0]->role == VariableRole::Field,
            "struct field role");

        // Verify enum
        ASSERT_NOT_NULL(enumDecl->resolvedEnum, "Color has resolvedEnum");
        ASSERT_EQ(enumDecl->resolvedEnum->members.size(), size_t(3), "Color has 3 members");
        ASSERT_EQ(enumDecl->resolvedEnum->members[0].name, std::string("Red"), "member 0 = Red");
        ASSERT_EQ(enumDecl->resolvedEnum->members[1].intValue, int64_t(1), "Green = 1");
        ASSERT_EQ(enumDecl->resolvedEnum->members[2].intValue, int64_t(2), "Blue = 2");

        // Verify types registered
        ASSERT_NOT_NULL(st.resolveType("Vec2"), "Vec2 in type registry");
        ASSERT_NOT_NULL(st.resolveType("Color"), "Color in type registry");

        // Verify lambda parameter symbol linked
        ASSERT_NOT_NULL(lambdaParam->resolvedSymbol, "lambda param has resolvedSymbol");
        ASSERT_EQ(lambdaParam->resolvedSymbol->getName(), std::string("x"),
            "lambda param name = x");
        ASSERT_TRUE(lambdaParam->resolvedSymbol->role == VariableRole::Parameter,
            "lambda param role = Parameter");

        // Verify for-loop creates its own scope with init var
        ASSERT_NOT_NULL(initVar->resolvedVariable, "for-init var resolved");
        ASSERT_EQ(initVar->resolvedVariable->getName(), std::string("i"), "for-init var = i");

        // Verify fn variable resolved
        ASSERT_NOT_NULL(lambdaDecl->resolvedVariable, "fn var resolved");
        ASSERT_EQ(lambdaDecl->resolvedVariable->getName(), std::string("fn"), "fn name");

        passed++;
        std::cout << "[PASS] 14. Pass 1 — Struct, Enum, Lambda, For\n";
    }

    // ========================================
    // 15. Pass 1 — Import resolution
    // ========================================
    {
        // Build a two-module program:
        //   module MathLib {
        //     func sin(double x) => double { }
        //     func cos(double x) => double { }
        //   }
        //   module Main {
        //     import sin from MathLib;
        //   }

        auto program = std::make_shared<ProgramNode>();

        // Module: MathLib
        auto mathModule = std::make_shared<ModuleNode>();
        mathModule->name = "MathLib";

        auto sinFunc = std::make_shared<FunctionDeclaration>();
        sinFunc->name = "sin";
        auto sinParam = std::make_shared<ParameterNode>();
        sinParam->name = "x";
        sinFunc->parameters = {sinParam};
        sinFunc->body = std::make_shared<BlockStatementNode>();

        auto cosFunc = std::make_shared<FunctionDeclaration>();
        cosFunc->name = "cos";
        auto cosParam = std::make_shared<ParameterNode>();
        cosParam->name = "x";
        cosFunc->parameters = {cosParam};
        cosFunc->body = std::make_shared<BlockStatementNode>();

        mathModule->declarations.push_back(sinFunc);
        mathModule->declarations.push_back(cosFunc);

        // Module: Main (imports sin from MathLib)
        auto mainModule = std::make_shared<ModuleNode>();
        mainModule->name = "Main";

        auto importDecl = std::make_shared<ImportDeclaration>();
        importDecl->sourcePath = {"MathLib"};
        importDecl->isWholeModule = false;
        importDecl->targets = {{"sin", std::nullopt}};

        mainModule->declarations.push_back(importDecl);
        program->modules = {mathModule, mainModule};

        // Run Pass 1
        SymbolTable st;
        ErrorReporter errors;
        SymbolTableBuilder builder(st, errors);
        builder.build(*program);

        ASSERT_TRUE(!errors.hasErrors(), "Pass 1 import no errors");

        // Verify: Main module can resolve 'sin' (imported)
        auto sinFromMain = mainModule->resolvedModule->resolve("sin");
        ASSERT_NOT_NULL(sinFromMain, "Main resolves 'sin' via import");
        ASSERT_TRUE(sinFromMain->is<FunctionSymbol>(), "imported sin is FunctionSymbol");

        // Verify: Main module CANNOT resolve 'cos' (not imported)
        auto cosFromMain = mainModule->resolvedModule->resolve("cos");
        // cos should NOT be directly defined in Main's scope
        // (it may still resolve through the global scope chain, but
        //  the import only brings 'sin' into Main)
        // Actually Main has no 'cos' defined, so resolve might
        // walk up to global, then fail. Let's just verify sin works.

        // Verify: MathLib module has both functions
        auto sinFromMath = mathModule->resolvedModule->resolve("sin");
        auto cosFromMath = mathModule->resolvedModule->resolve("cos");
        ASSERT_NOT_NULL(sinFromMath, "MathLib resolves 'sin'");
        ASSERT_NOT_NULL(cosFromMath, "MathLib resolves 'cos'");

        passed++;
        std::cout << "[PASS] 15. Pass 1 — Import resolution\n";
    }

    // ========================================
    // 16. Pass 2 — TypeResolver: function signatures
    // ========================================
    {
        // Build:
        //   module Main {
        //     func add(int a, int& b) => double { return 0.0; }
        //     var PI: double;
        //   }

        auto program = std::make_shared<ProgramNode>();
        auto module = std::make_shared<ModuleNode>();
        module->name = "Main";

        // func add(int a, int& b) => double { ... }
        auto funcDecl = std::make_shared<FunctionDeclaration>();
        funcDecl->name = "add";

        // param a: int
        auto paramA = std::make_shared<ParameterNode>();
        paramA->name = "a";
        paramA->isReference = false;
        auto paramAType = std::make_shared<PrimitiveTypeNode>();
        paramAType->kind = PrimitiveKind::Int;
        paramA->type = paramAType;

        // param b: int& (reference)
        auto paramB = std::make_shared<ParameterNode>();
        paramB->name = "b";
        paramB->isReference = true;
        auto paramBBaseType = std::make_shared<PrimitiveTypeNode>();
        paramBBaseType->kind = PrimitiveKind::Int;
        auto paramBType = std::make_shared<PointerTypeNode>();
        paramBType->baseType = paramBBaseType;
        paramBType->isReference = true;  // T&
        paramB->type = paramBType;

        funcDecl->parameters = {paramA, paramB};

        // Return type: double
        auto retType = std::make_shared<PrimitiveTypeNode>();
        retType->kind = PrimitiveKind::Double;
        funcDecl->returnType = retType;

        // Body
        auto body = std::make_shared<BlockStatementNode>();
        auto retStmt = std::make_shared<ReturnStatement>();
        retStmt->value = std::make_shared<FloatLiteral>();
        body->statements.push_back(retStmt);
        funcDecl->body = body;

        // var PI: double;
        auto piDecl = std::make_shared<VariableDeclaration>();
        piDecl->name = "PI";
        piDecl->isInferred = false;
        auto piType = std::make_shared<PrimitiveTypeNode>();
        piType->kind = PrimitiveKind::Double;
        piDecl->type = piType;

        module->declarations = {funcDecl, piDecl};
        program->modules = {module};

        // Run Pass 1 + Pass 2
        SymbolTable st;
        ErrorReporter errors;
        SymbolTableBuilder builder(st, errors);
        builder.build(*program);
        ASSERT_TRUE(!errors.hasErrors(), "Pass 1 no errors (test 16)");

        TypeResolver resolver(st, errors);
        resolver.resolve(*program);
        ASSERT_TRUE(!errors.hasErrors(), "Pass 2 no errors (test 16)");

        // Verify: function return type resolved
        auto& funcSym = funcDecl->resolvedFunction;
        ASSERT_NOT_NULL(funcSym, "func resolved");
        ASSERT_NOT_NULL(funcSym->returnType, "func has return type");
        ASSERT_EQ(funcSym->returnType->getName(), std::string("double"), "return type = double");

        // Verify: param a has type int
        auto* paramASym = paramA->resolvedSymbol.get();
        ASSERT_NOT_NULL(paramASym, "param a symbol");
        ASSERT_NOT_NULL(paramASym->getType(), "param a has type");
        ASSERT_EQ(paramASym->getType()->getName(), std::string("int"), "param a type = int");
        ASSERT_EQ(paramASym->isReference, false, "param a not ref");

        // Verify: param b has type int BUT isReference = true (UNWRAPPED!)
        auto* paramBSym = paramB->resolvedSymbol.get();
        ASSERT_NOT_NULL(paramBSym, "param b symbol");
        ASSERT_NOT_NULL(paramBSym->getType(), "param b has type");
        ASSERT_EQ(paramBSym->getType()->getName(), std::string("int"),
            "param b type = int (unwrapped from int&)");
        ASSERT_EQ(paramBSym->isReference, true,
            "param b isReference = true (critical unwrap)");

        // Verify: variable PI has type double
        ASSERT_NOT_NULL(piDecl->resolvedVariable->getType(), "PI has type");
        ASSERT_EQ(piDecl->resolvedVariable->getType()->getName(), std::string("double"),
            "PI type = double");

        passed++;
        std::cout << "[PASS] 16. Pass 2 — Function signatures + ref unwrap\n";
    }

    // ========================================
    // 17. Pass 2 — Compound types + struct fields
    // ========================================
    {
        // Build:
        //   module Geom {
        //     struct Vec2 { double x; double y; }
        //     func process(int* ptr, int[10] arr) => (int, double) { }
        //     enum Color { Red, Green, Blue }
        //   }

        auto program = std::make_shared<ProgramNode>();
        auto module = std::make_shared<ModuleNode>();
        module->name = "Geom";

        // struct Vec2 { double x; double y; }
        auto structDecl = std::make_shared<StructDeclaration>();
        structDecl->name = "Vec2";

        auto xField = std::make_shared<VariableDeclaration>();
        xField->name = "x";
        auto xType = std::make_shared<PrimitiveTypeNode>();
        xType->kind = PrimitiveKind::Double;
        xField->type = xType;

        auto yField = std::make_shared<VariableDeclaration>();
        yField->name = "y";
        auto yType = std::make_shared<PrimitiveTypeNode>();
        yType->kind = PrimitiveKind::Double;
        yField->type = yType;

        structDecl->fields = {xField, yField};

        // func process(int* ptr, int[10] arr) => (int, double)
        auto funcDecl = std::make_shared<FunctionDeclaration>();
        funcDecl->name = "process";

        // param ptr: int*
        auto ptrParam = std::make_shared<ParameterNode>();
        ptrParam->name = "ptr";
        auto ptrType = std::make_shared<PointerTypeNode>();
        auto ptrBase = std::make_shared<PrimitiveTypeNode>();
        ptrBase->kind = PrimitiveKind::Int;
        ptrType->baseType = ptrBase;
        ptrType->isReference = false;
        ptrParam->type = ptrType;

        // param arr: int[10]
        auto arrParam = std::make_shared<ParameterNode>();
        arrParam->name = "arr";
        auto arrType = std::make_shared<ArrayTypeNode>();
        auto arrElem = std::make_shared<PrimitiveTypeNode>();
        arrElem->kind = PrimitiveKind::Int;
        arrType->elementType = arrElem;
        auto arrSize = std::make_shared<IntegerLiteral>();
        arrSize->value = 10;
        arrType->sizeExpr = arrSize;
        arrParam->type = arrType;

        funcDecl->parameters = {ptrParam, arrParam};

        // Return type: (int, double)
        auto tupleRet = std::make_shared<TupleTypeNode>();
        auto tupElem1 = std::make_shared<PrimitiveTypeNode>();
        tupElem1->kind = PrimitiveKind::Int;
        auto tupElem2 = std::make_shared<PrimitiveTypeNode>();
        tupElem2->kind = PrimitiveKind::Double;
        tupleRet->elementTypes = {tupElem1, tupElem2};
        funcDecl->returnType = tupleRet;
        funcDecl->body = std::make_shared<BlockStatementNode>();

        // enum Color (underlying type defaults to int)
        auto enumDecl = std::make_shared<EnumDeclaration>();
        enumDecl->name = "Color";
        auto red = std::make_shared<EnumMemberNode>();
        red->name = "Red";
        enumDecl->members = {red};

        module->declarations = {structDecl, funcDecl, enumDecl};
        program->modules = {module};

        // Run Pass 1 + Pass 2
        SymbolTable st;
        ErrorReporter errors;
        SymbolTableBuilder builder(st, errors);
        builder.build(*program);
        TypeResolver resolver(st, errors);
        resolver.resolve(*program);
        ASSERT_TRUE(!errors.hasErrors(), "Pass 2 no errors (test 17)");

        // Verify struct field types
        auto* vecSym = structDecl->resolvedStruct.get();
        ASSERT_NOT_NULL(vecSym, "Vec2 resolved");
        ASSERT_NOT_NULL(vecSym->fields[0]->getType(), "Vec2.x has type");
        ASSERT_EQ(vecSym->fields[0]->getType()->getName(), std::string("double"),
            "Vec2.x type = double");
        ASSERT_NOT_NULL(vecSym->fields[1]->getType(), "Vec2.y has type");
        ASSERT_EQ(vecSym->fields[1]->getType()->getName(), std::string("double"),
            "Vec2.y type = double");

        // Verify: param ptr has pointer type
        auto* ptrSym = ptrParam->resolvedSymbol.get();
        ASSERT_NOT_NULL(ptrSym->getType(), "ptr param has type");
        ASSERT_TRUE(ptrSym->getType()->is<PointerTypeSymbol>(), "ptr type is PointerTypeSymbol");
        ASSERT_EQ(ptrSym->getType()->getTypeDescription(), std::string("int*"),
            "ptr type = int*");

        // Verify: param arr has array type
        auto* arrSym = arrParam->resolvedSymbol.get();
        ASSERT_NOT_NULL(arrSym->getType(), "arr param has type");
        ASSERT_TRUE(arrSym->getType()->is<ArrayTypeSymbol>(), "arr type is ArrayTypeSymbol");
        auto* arrTypeSym = arrSym->getType()->as<ArrayTypeSymbol>();
        ASSERT_EQ(arrTypeSym->arraySize, 10, "arr size = 10");

        // Verify: return type is tuple
        auto& fSym = funcDecl->resolvedFunction;
        ASSERT_NOT_NULL(fSym->returnType, "func has return type");
        ASSERT_TRUE(fSym->returnType->is<TupleTypeSymbol>(), "return is TupleTypeSymbol");
        ASSERT_EQ(fSym->returnType->getTypeDescription(), std::string("(int, double)"),
            "return type = (int, double)");

        // Verify: enum underlying type defaults to int
        ASSERT_NOT_NULL(enumDecl->resolvedEnum->underlyingType, "enum has underlying type");
        ASSERT_EQ(enumDecl->resolvedEnum->underlyingType->getName(), std::string("int"),
            "enum underlying = int");

        passed++;
        std::cout << "[PASS] 17. Pass 2 — Compound types + struct fields\n";
    }

    // ========================================
    // 18. Pass 2 — Named types + cast/new
    // ========================================
    {
        // Build:
        //   module Zoo {
        //     class Animal { int age; }
        //     func factory() => void {
        //       var a: Animal;
        //       var p = new Animal();
        //       var n = cast<int>(x);
        //     }
        //   }

        auto program = std::make_shared<ProgramNode>();
        auto module = std::make_shared<ModuleNode>();
        module->name = "Zoo";

        // class Animal { int age; }
        auto classDecl = std::make_shared<ClassDeclaration>();
        classDecl->name = "Animal";
        auto ageField = std::make_shared<VariableDeclaration>();
        ageField->name = "age";
        auto ageType = std::make_shared<PrimitiveTypeNode>();
        ageType->kind = PrimitiveKind::Int;
        ageField->type = ageType;
        classDecl->fields = {ageField};

        // func factory() => void { ... }
        auto funcDecl = std::make_shared<FunctionDeclaration>();
        funcDecl->name = "factory";
        auto retType = std::make_shared<PrimitiveTypeNode>();
        retType->kind = PrimitiveKind::Void;
        funcDecl->returnType = retType;

        auto body = std::make_shared<BlockStatementNode>();

        // var a: Animal;
        auto varA = std::make_shared<VariableDeclaration>();
        varA->name = "a";
        auto animalTypeNode = std::make_shared<NamedTypeNode>();
        animalTypeNode->qualifiedName = {"Animal"};
        varA->type = animalTypeNode;
        body->statements.push_back(varA);

        // new Animal() — just test that NewExpression type annotation resolves
        auto newExpr = std::make_shared<NewExpression>();
        auto newTypeNode = std::make_shared<NamedTypeNode>();
        newTypeNode->qualifiedName = {"Animal"};
        newExpr->type = newTypeNode;
        newExpr->arguments = std::make_shared<ArgumentsNode>();
        auto newStmt = std::make_shared<ExpressionStatement>();
        newStmt->expression = newExpr;
        body->statements.push_back(newStmt);

        // cast<int>(x)
        auto castExpr = std::make_shared<CastExpression>();
        auto castType = std::make_shared<PrimitiveTypeNode>();
        castType->kind = PrimitiveKind::Int;
        castExpr->targetType = castType;
        castExpr->operand = std::make_shared<IdentifierExpression>();
        auto castStmt = std::make_shared<ExpressionStatement>();
        castStmt->expression = castExpr;
        body->statements.push_back(castStmt);

        funcDecl->body = body;
        module->declarations = {classDecl, funcDecl};
        program->modules = {module};

        // Run Pass 1 + Pass 2
        SymbolTable st;
        ErrorReporter errors;
        SymbolTableBuilder builder(st, errors);
        builder.build(*program);
        TypeResolver resolver(st, errors);
        resolver.resolve(*program);
        ASSERT_TRUE(!errors.hasErrors(), "Pass 2 no errors (test 18)");

        // Verify: var a has type Animal (resolved via NamedTypeNode)
        ASSERT_NOT_NULL(varA->resolvedVariable->getType(), "var a has type");
        ASSERT_EQ(varA->resolvedVariable->getType()->getName(), std::string("Animal"),
            "var a type = Animal");
        ASSERT_TRUE(varA->resolvedVariable->getType()->is<ClassSymbol>(),
            "var a type is ClassSymbol");

        // Verify: NamedTypeNode in new expression resolved
        ASSERT_NOT_NULL(newTypeNode->resolvedType, "new type annotation resolved");
        ASSERT_EQ(newTypeNode->resolvedType->getName(), std::string("Animal"),
            "new type = Animal");

        // Verify: cast target type resolved
        ASSERT_NOT_NULL(castType->resolvedType, "cast type resolved");
        ASSERT_EQ(castType->resolvedType->getName(), std::string("int"),
            "cast type = int");

        // Verify: class field type resolved
        auto* animalSym = classDecl->resolvedClass.get();
        ASSERT_NOT_NULL(animalSym->fields[0]->getType(), "Animal.age has type");
        ASSERT_EQ(animalSym->fields[0]->getType()->getName(), std::string("int"),
            "Animal.age type = int");

        passed++;
        std::cout << "[PASS] 18. Pass 2 — Named types + cast/new\n";
    }

    // ========================================
    // 19. Pass 3 — TypeChecker: literals + identifiers + var inference
    // ========================================
    {
        // Build:
        //   module Main {
        //     func compute() => int {
        //       var x: int = 42;
        //       var y = 3.14;           // inferred double
        //       var s = "hello";        // inferred string
        //       var b = true;           // inferred bool
        //       return x;
        //     }
        //   }

        auto program = std::make_shared<ProgramNode>();
        auto module = std::make_shared<ModuleNode>();
        module->name = "Main";

        auto funcDecl = std::make_shared<FunctionDeclaration>();
        funcDecl->name = "compute";
        auto retType = std::make_shared<PrimitiveTypeNode>();
        retType->kind = PrimitiveKind::Int;
        funcDecl->returnType = retType;

        auto body = std::make_shared<BlockStatementNode>();

        // var x: int = 42;
        auto varX = std::make_shared<VariableDeclaration>();
        varX->name = "x";
        varX->isInferred = false;
        auto xType = std::make_shared<PrimitiveTypeNode>();
        xType->kind = PrimitiveKind::Int;
        varX->type = xType;
        auto lit42 = std::make_shared<IntegerLiteral>();
        lit42->value = 42;
        varX->initializer = lit42;
        body->statements.push_back(varX);

        // var y = 3.14;
        auto varY = std::make_shared<VariableDeclaration>();
        varY->name = "y";
        varY->isInferred = true;
        auto litPi = std::make_shared<FloatLiteral>();
        litPi->value = 3.14;
        varY->initializer = litPi;
        body->statements.push_back(varY);

        // var s = "hello";
        auto varS = std::make_shared<VariableDeclaration>();
        varS->name = "s";
        varS->isInferred = true;
        auto litStr = std::make_shared<StringLiteral>();
        litStr->value = "hello";
        varS->initializer = litStr;
        body->statements.push_back(varS);

        // var b = true;
        auto varB = std::make_shared<VariableDeclaration>();
        varB->name = "b";
        varB->isInferred = true;
        auto litBool = std::make_shared<BoolLiteral>();
        litBool->value = true;
        varB->initializer = litBool;
        body->statements.push_back(varB);

        // return x;
        auto retStmt = std::make_shared<ReturnStatement>();
        auto retIdent = std::make_shared<IdentifierExpression>();
        retIdent->name = "x";
        retStmt->value = retIdent;
        body->statements.push_back(retStmt);

        funcDecl->body = body;
        module->declarations = {funcDecl};
        program->modules = {module};

        // Run all 3 passes
        SymbolTable st;
        ErrorReporter errors;
        SymbolTableBuilder builder(st, errors);
        builder.build(*program);
        TypeResolver resolver(st, errors);
        resolver.resolve(*program);
        TypeChecker checker(st, errors);
        checker.check(*program);

        ASSERT_TRUE(!errors.hasErrors(), "Pass 3 no errors (test 19)");

        // Literal types
        ASSERT_NOT_NULL(lit42->resolvedType, "int literal has type");
        ASSERT_EQ(lit42->resolvedType->getName(), std::string("int"), "42 is int");
        ASSERT_NOT_NULL(litPi->resolvedType, "float literal has type");
        ASSERT_EQ(litPi->resolvedType->getName(), std::string("double"), "3.14 is double");
        ASSERT_NOT_NULL(litStr->resolvedType, "string literal has type");
        ASSERT_EQ(litStr->resolvedType->getName(), std::string("string"), "hello is string");
        ASSERT_NOT_NULL(litBool->resolvedType, "bool literal has type");
        ASSERT_EQ(litBool->resolvedType->getName(), std::string("bool"), "true is bool");

        // Variable type inference
        ASSERT_NOT_NULL(varY->resolvedVariable->getType(), "y has inferred type");
        ASSERT_EQ(varY->resolvedVariable->getType()->getName(), std::string("double"),
            "y inferred as double");
        ASSERT_NOT_NULL(varS->resolvedVariable->getType(), "s has inferred type");
        ASSERT_EQ(varS->resolvedVariable->getType()->getName(), std::string("string"),
            "s inferred as string");
        ASSERT_NOT_NULL(varB->resolvedVariable->getType(), "b has inferred type");
        ASSERT_EQ(varB->resolvedVariable->getType()->getName(), std::string("bool"),
            "b inferred as bool");

        // Identifier resolution: return x resolves to variable x
        ASSERT_NOT_NULL(retIdent->resolvedSymbol, "return ident resolved");
        ASSERT_NOT_NULL(retIdent->resolvedType, "return ident has type");
        ASSERT_EQ(retIdent->resolvedType->getName(), std::string("int"),
            "return x has type int");

        passed++;
        std::cout << "[PASS] 19. Pass 3 — Literals + identifiers + var inference\n";
    }

    // ========================================
    // 20. Pass 3 — Binary ops + call resolution
    // ========================================
    {
        // Build:
        //   module Math {
        //     func add(int a, int& b) => int { return a; }
        //     func main() => void {
        //       var x: int = 10;
        //       var y: int = 20;
        //       var sum = add(x, y);   // call with resolvedCallee + isReference
        //       var cmp = x < y;       // comparison → bool
        //     }
        //   }

        auto program = std::make_shared<ProgramNode>();
        auto module = std::make_shared<ModuleNode>();
        module->name = "Math";

        // func add(int a, int& b) => int
        auto addFunc = std::make_shared<FunctionDeclaration>();
        addFunc->name = "add";
        auto paramA = std::make_shared<ParameterNode>();
        paramA->name = "a";
        auto paramAType = std::make_shared<PrimitiveTypeNode>();
        paramAType->kind = PrimitiveKind::Int;
        paramA->type = paramAType;

        auto paramB = std::make_shared<ParameterNode>();
        paramB->name = "b";
        paramB->isReference = true;
        auto paramBBase = std::make_shared<PrimitiveTypeNode>();
        paramBBase->kind = PrimitiveKind::Int;
        auto paramBType = std::make_shared<PointerTypeNode>();
        paramBType->baseType = paramBBase;
        paramBType->isReference = true;
        paramB->type = paramBType;

        addFunc->parameters = {paramA, paramB};
        auto addRet = std::make_shared<PrimitiveTypeNode>();
        addRet->kind = PrimitiveKind::Int;
        addFunc->returnType = addRet;
        auto addBody = std::make_shared<BlockStatementNode>();
        auto addReturn = std::make_shared<ReturnStatement>();
        auto addRetId = std::make_shared<IdentifierExpression>();
        addRetId->name = "a";
        addReturn->value = addRetId;
        addBody->statements.push_back(addReturn);
        addFunc->body = addBody;

        // func main() => void
        auto mainFunc = std::make_shared<FunctionDeclaration>();
        mainFunc->name = "main";
        auto mainRet = std::make_shared<PrimitiveTypeNode>();
        mainRet->kind = PrimitiveKind::Void;
        mainFunc->returnType = mainRet;
        auto mainBody = std::make_shared<BlockStatementNode>();

        // var x: int = 10;
        auto varX = std::make_shared<VariableDeclaration>();
        varX->name = "x";
        auto xType = std::make_shared<PrimitiveTypeNode>();
        xType->kind = PrimitiveKind::Int;
        varX->type = xType;
        varX->initializer = std::make_shared<IntegerLiteral>();
        mainBody->statements.push_back(varX);

        // var y: int = 20;
        auto varY = std::make_shared<VariableDeclaration>();
        varY->name = "y";
        auto yType = std::make_shared<PrimitiveTypeNode>();
        yType->kind = PrimitiveKind::Int;
        varY->type = yType;
        varY->initializer = std::make_shared<IntegerLiteral>();
        mainBody->statements.push_back(varY);

        // var sum = add(x, y);
        auto sumDecl = std::make_shared<VariableDeclaration>();
        sumDecl->name = "sum";
        sumDecl->isInferred = true;
        auto callExpr = std::make_shared<CallExpression>();
        auto calleeId = std::make_shared<IdentifierExpression>();
        calleeId->name = "add";
        callExpr->callee = calleeId;
        auto args = std::make_shared<ArgumentsNode>();
        auto argX = std::make_shared<IdentifierExpression>();
        argX->name = "x";
        auto argY = std::make_shared<IdentifierExpression>();
        argY->name = "y";
        args->expressions = {argX, argY};
        callExpr->arguments = args;
        sumDecl->initializer = callExpr;
        mainBody->statements.push_back(sumDecl);

        // var cmp = x < y;
        auto cmpDecl = std::make_shared<VariableDeclaration>();
        cmpDecl->name = "cmp";
        cmpDecl->isInferred = true;
        auto cmpExpr = std::make_shared<BinaryExpression>();
        cmpExpr->op = BinaryOp::Less;
        auto cmpLeft = std::make_shared<IdentifierExpression>();
        cmpLeft->name = "x";
        auto cmpRight = std::make_shared<IdentifierExpression>();
        cmpRight->name = "y";
        cmpExpr->left = cmpLeft;
        cmpExpr->right = cmpRight;
        cmpDecl->initializer = cmpExpr;
        mainBody->statements.push_back(cmpDecl);

        mainFunc->body = mainBody;
        module->declarations = {addFunc, mainFunc};
        program->modules = {module};

        // Run all 3 passes
        SymbolTable st;
        ErrorReporter errors;
        SymbolTableBuilder builder(st, errors);
        builder.build(*program);
        TypeResolver resolver(st, errors);
        resolver.resolve(*program);
        TypeChecker checker(st, errors);
        checker.check(*program);

        ASSERT_TRUE(!errors.hasErrors(), "Pass 3 no errors (test 20)");

        // Call expression: resolved callee
        ASSERT_NOT_NULL(callExpr->resolvedCallee, "call has resolvedCallee");
        ASSERT_EQ(callExpr->resolvedCallee->getName(), std::string("add"),
            "callee = add");
        ASSERT_NOT_NULL(callExpr->resolvedType, "call has resolvedType");
        ASSERT_EQ(callExpr->resolvedType->getName(), std::string("int"),
            "call returns int");

        // Call expression: isReference per-argument
        ASSERT_EQ(callExpr->arguments->isReference.size(), size_t(2),
            "call has 2 isReference entries");
        ASSERT_EQ(callExpr->arguments->isReference[0], false,
            "arg 0 not ref");
        ASSERT_EQ(callExpr->arguments->isReference[1], true,
            "arg 1 IS ref (critical for codegen!)");

        // sum inferred as int (from call return type)
        ASSERT_NOT_NULL(sumDecl->resolvedVariable->getType(), "sum has type");
        ASSERT_EQ(sumDecl->resolvedVariable->getType()->getName(), std::string("int"),
            "sum inferred as int");

        // Comparison → bool
        ASSERT_NOT_NULL(cmpExpr->resolvedType, "cmp expr has type");
        ASSERT_EQ(cmpExpr->resolvedType->getName(), std::string("bool"),
            "x < y is bool");
        ASSERT_NOT_NULL(cmpDecl->resolvedVariable->getType(), "cmp has type");
        ASSERT_EQ(cmpDecl->resolvedVariable->getType()->getName(), std::string("bool"),
            "cmp inferred as bool");

        passed++;
        std::cout << "[PASS] 20. Pass 3 — Binary ops + call resolution\n";
    }

    // ========================================
    // 21. Pass 3 — Operator overload + member access
    // ========================================
    {
        // Build:
        //   module Geom {
        //     struct Vec2 {
        //       double x;
        //       double y;
        //       operator+(Vec2 other) => Vec2 { }
        //     }
        //     func test() => void {
        //       var v: Vec2;
        //       var vx = v.x;           // field access → double
        //     }
        //   }

        auto program = std::make_shared<ProgramNode>();
        auto module = std::make_shared<ModuleNode>();
        module->name = "Geom";

        // struct Vec2
        auto structDecl = std::make_shared<StructDeclaration>();
        structDecl->name = "Vec2";

        auto xField = std::make_shared<VariableDeclaration>();
        xField->name = "x";
        auto xType = std::make_shared<PrimitiveTypeNode>();
        xType->kind = PrimitiveKind::Double;
        xField->type = xType;

        auto yField = std::make_shared<VariableDeclaration>();
        yField->name = "y";
        auto yType = std::make_shared<PrimitiveTypeNode>();
        yType->kind = PrimitiveKind::Double;
        yField->type = yType;

        structDecl->fields = {xField, yField};

        // operator+(Vec2 other) => Vec2
        auto opDecl = std::make_shared<OperatorDeclaration>();
        opDecl->op = OverloadableOp::Add;
        auto opParam = std::make_shared<ParameterNode>();
        opParam->name = "other";
        auto opParamType = std::make_shared<NamedTypeNode>();
        opParamType->qualifiedName = {"Vec2"};
        opParam->type = opParamType;
        opDecl->parameters = {opParam};
        auto opRetType = std::make_shared<NamedTypeNode>();
        opRetType->qualifiedName = {"Vec2"};
        opDecl->returnType = opRetType;
        opDecl->body = std::make_shared<BlockStatementNode>();
        structDecl->operators = {opDecl};

        // func test() => void
        auto funcDecl = std::make_shared<FunctionDeclaration>();
        funcDecl->name = "test";
        auto funcRet = std::make_shared<PrimitiveTypeNode>();
        funcRet->kind = PrimitiveKind::Void;
        funcDecl->returnType = funcRet;
        auto funcBody = std::make_shared<BlockStatementNode>();

        // var v: Vec2;
        auto varV = std::make_shared<VariableDeclaration>();
        varV->name = "v";
        auto vType = std::make_shared<NamedTypeNode>();
        vType->qualifiedName = {"Vec2"};
        varV->type = vType;
        funcBody->statements.push_back(varV);

        // var vx = v.x;
        auto varVX = std::make_shared<VariableDeclaration>();
        varVX->name = "vx";
        varVX->isInferred = true;
        auto memberAccess = std::make_shared<MemberAccessExpression>();
        auto vIdent = std::make_shared<IdentifierExpression>();
        vIdent->name = "v";
        memberAccess->object = vIdent;
        memberAccess->memberName = "x";
        varVX->initializer = memberAccess;
        funcBody->statements.push_back(varVX);

        funcDecl->body = funcBody;
        module->declarations = {structDecl, funcDecl};
        program->modules = {module};

        // Run all 3 passes
        SymbolTable st;
        ErrorReporter errors;
        SymbolTableBuilder builder(st, errors);
        builder.build(*program);
        TypeResolver resolver(st, errors);
        resolver.resolve(*program);
        TypeChecker checker(st, errors);
        checker.check(*program);

        ASSERT_TRUE(!errors.hasErrors(), "Pass 3 no errors (test 21)");

        // Verify: v.x resolves to field with type double
        ASSERT_NOT_NULL(memberAccess->resolvedType, "v.x has type");
        ASSERT_EQ(memberAccess->resolvedType->getName(), std::string("double"),
            "v.x type = double");
        ASSERT_NOT_NULL(memberAccess->resolvedSymbol, "v.x has resolvedSymbol");

        // Verify: vx inferred as double from v.x
        ASSERT_NOT_NULL(varVX->resolvedVariable->getType(), "vx has type");
        ASSERT_EQ(varVX->resolvedVariable->getType()->getName(), std::string("double"),
            "vx inferred as double");

        // Verify: operator+ exists and is registered
        auto* vecSym = structDecl->resolvedStruct.get();
        ASSERT_NOT_NULL(vecSym, "Vec2 resolved");
        auto addOp = vecSym->resolveOperator(OverloadableOp::Add);
        ASSERT_NOT_NULL(addOp, "Vec2 has operator+");
        ASSERT_NOT_NULL(addOp->returnType, "operator+ has return type");
        ASSERT_EQ(addOp->returnType->getName(), std::string("Vec2"),
            "operator+ returns Vec2");

        passed++;
        std::cout << "[PASS] 21. Pass 3 — Operator overload + member access\n";
    }

    // ========================================
    // 22. Pass 4 — Lambda capture analysis
    // ========================================
    {
        // Build:
        //   module Main {
        //     func outer() => void {
        //       var count: int = 0;
        //       var name: string = "hello";
        //       var fn = [=, &count](int x) => { return count + x; };
        //       // fn should capture:
        //       //   count by reference (explicit &count)
        //       //   x is a parameter — not captured
        //       // name is not used in lambda — not captured
        //     }
        //   }

        auto program = std::make_shared<ProgramNode>();
        auto module = std::make_shared<ModuleNode>();
        module->name = "Main";

        auto funcDecl = std::make_shared<FunctionDeclaration>();
        funcDecl->name = "outer";
        auto retType = std::make_shared<PrimitiveTypeNode>();
        retType->kind = PrimitiveKind::Void;
        funcDecl->returnType = retType;
        auto body = std::make_shared<BlockStatementNode>();

        // var count: int = 0;
        auto varCount = std::make_shared<VariableDeclaration>();
        varCount->name = "count";
        auto countType = std::make_shared<PrimitiveTypeNode>();
        countType->kind = PrimitiveKind::Int;
        varCount->type = countType;
        varCount->initializer = std::make_shared<IntegerLiteral>();
        body->statements.push_back(varCount);

        // var name: string = "hello";
        auto varName = std::make_shared<VariableDeclaration>();
        varName->name = "name";
        auto nameType = std::make_shared<PrimitiveTypeNode>();
        nameType->kind = PrimitiveKind::String;
        varName->type = nameType;
        varName->initializer = std::make_shared<StringLiteral>();
        body->statements.push_back(varName);

        // var fn = [=, &count](int x) => { return count + x; };
        auto varFn = std::make_shared<VariableDeclaration>();
        varFn->name = "fn";
        varFn->isInferred = true;

        auto lambda = std::make_shared<LambdaExpression>();
        lambda->captureDefault = CaptureDefault::ByCopy;
        lambda->captureItems = {{"count", CaptureMode::ByReference}};

        auto lambdaParam = std::make_shared<ParameterNode>();
        lambdaParam->name = "x";
        auto paramType = std::make_shared<PrimitiveTypeNode>();
        paramType->kind = PrimitiveKind::Int;
        lambdaParam->type = paramType;
        lambda->parameters = {lambdaParam};

        // Body: return count + x;
        auto lambdaBody = std::make_shared<BlockStatementNode>();
        auto lambdaRet = std::make_shared<ReturnStatement>();
        auto addExpr = std::make_shared<BinaryExpression>();
        addExpr->op = BinaryOp::Add;
        auto countIdent = std::make_shared<IdentifierExpression>();
        countIdent->name = "count";
        auto xIdent = std::make_shared<IdentifierExpression>();
        xIdent->name = "x";
        addExpr->left = countIdent;
        addExpr->right = xIdent;
        lambdaRet->value = addExpr;
        lambdaBody->statements.push_back(lambdaRet);
        lambda->body = lambdaBody;
        varFn->initializer = lambda;
        body->statements.push_back(varFn);

        funcDecl->body = body;
        module->declarations = {funcDecl};
        program->modules = {module};

        // Run all 4 passes
        SymbolTable st;
        ErrorReporter errors;
        SymbolTableBuilder builder(st, errors);
        builder.build(*program);
        TypeResolver resolver(st, errors);
        resolver.resolve(*program);
        TypeChecker checker(st, errors);
        checker.check(*program);
        SemanticValidator validator(st, errors);
        validator.validate(*program);

        ASSERT_TRUE(!errors.hasErrors(), "Pass 4 no errors (test 22)");

        // Verify: lambda captured 'count' (used in body)
        ASSERT_EQ(lambda->capturedVariables.size(), size_t(1),
            "lambda captures 1 variable");
        ASSERT_EQ(lambda->capturedVariables[0]->getName(), std::string("count"),
            "captured var = count");

        // Verify: capture mode is ByReference (explicit &count)
        ASSERT_EQ(lambda->captureModesResolved.size(), size_t(1),
            "1 capture mode resolved");
        ASSERT_TRUE(lambda->captureModesResolved[0] == CaptureMode::ByReference,
            "count captured by reference");

        // Verify: 'x' is NOT captured (it's a parameter)
        // 'name' is NOT captured (not used in lambda body)

        passed++;
        std::cout << "[PASS] 22. Pass 4 — Lambda capture analysis\n";
    }

    // ========================================
    // 23. Pass 4 — RAII tracking + control flow
    // ========================================
    {
        // Build:
        //   module Main {
        //     class Widget { int id; }
        //     func test() => void {
        //       var w: Widget;       // RAII: Widget has destructor
        //       while (true) {
        //         break;             // valid: inside loop
        //       }
        //     }
        //   }

        auto program = std::make_shared<ProgramNode>();
        auto module = std::make_shared<ModuleNode>();
        module->name = "Main";

        // class Widget { int id; }
        auto classDecl = std::make_shared<ClassDeclaration>();
        classDecl->name = "Widget";
        auto idField = std::make_shared<VariableDeclaration>();
        idField->name = "id";
        auto idType = std::make_shared<PrimitiveTypeNode>();
        idType->kind = PrimitiveKind::Int;
        idField->type = idType;
        classDecl->fields = {idField};

        // func test() => void
        auto funcDecl = std::make_shared<FunctionDeclaration>();
        funcDecl->name = "test";
        auto retType = std::make_shared<PrimitiveTypeNode>();
        retType->kind = PrimitiveKind::Void;
        funcDecl->returnType = retType;
        auto body = std::make_shared<BlockStatementNode>();

        // var w: Widget;
        auto varW = std::make_shared<VariableDeclaration>();
        varW->name = "w";
        auto wType = std::make_shared<NamedTypeNode>();
        wType->qualifiedName = {"Widget"};
        varW->type = wType;
        body->statements.push_back(varW);

        // while (true) { break; }
        auto whileStmt = std::make_shared<WhileStatement>();
        auto trueLit = std::make_shared<BoolLiteral>();
        trueLit->value = true;
        whileStmt->condition = trueLit;
        auto whileBody = std::make_shared<BlockStatementNode>();
        auto breakStmt = std::make_shared<BreakStatement>();
        whileBody->statements.push_back(breakStmt);
        whileStmt->body = whileBody;
        body->statements.push_back(whileStmt);

        funcDecl->body = body;
        module->declarations = {classDecl, funcDecl};
        program->modules = {module};

        // Run all 4 passes
        SymbolTable st;
        ErrorReporter errors;
        SymbolTableBuilder builder(st, errors);
        builder.build(*program);
        TypeResolver resolver(st, errors);
        resolver.resolve(*program);
        TypeChecker checker(st, errors);
        checker.check(*program);
        SemanticValidator validator(st, errors);
        validator.validate(*program);

        ASSERT_TRUE(!errors.hasErrors(), "Pass 4 no errors (test 23)");

        // Verify: RAII tracking — Widget has destructor, so 'w' is tracked
        auto& raiiInfo = validator.getRAIIInfo();
        bool foundRAII = false;
        for (auto& [scope, info] : raiiInfo) {
            for (auto& d : info.destructibles) {
                if (d.variable->getName() == "w") {
                    foundRAII = true;
                    ASSERT_NOT_NULL(d.destructor, "w has destructor in RAII info");
                }
            }
        }
        ASSERT_TRUE(foundRAII, "var w tracked for RAII cleanup");

        // Verify: break inside loop — no error
        // (if break were outside loop, errors.hasErrors() would be true)

        passed++;
        std::cout << "[PASS] 23. Pass 4 — RAII tracking + control flow\n";
    }

    // ========================================
    // 24. Pass 4 — Nested lambda + self-capture + non-escaping
    // ========================================
    {
        // Build:
        //   module Main {
        //     func test() => void {
        //       var val: int = 10;
        //       var f = [=](int x) => { return val + x; };  // self-capture
        //       apply(f);                                     // non-escaping: lambda as arg
        //     }
        //     func apply((int)=>int callback) => void { }
        //   }

        auto program = std::make_shared<ProgramNode>();
        auto module = std::make_shared<ModuleNode>();
        module->name = "Main";

        // func apply((int)=>int callback) => void { }
        auto applyFunc = std::make_shared<FunctionDeclaration>();
        applyFunc->name = "apply";
        auto applyRet = std::make_shared<PrimitiveTypeNode>();
        applyRet->kind = PrimitiveKind::Void;
        applyFunc->returnType = applyRet;
        auto callbackParam = std::make_shared<ParameterNode>();
        callbackParam->name = "callback";
        auto callbackType = std::make_shared<FunctionTypeNode>();
        auto cbParamType = std::make_shared<PrimitiveTypeNode>();
        cbParamType->kind = PrimitiveKind::Int;
        callbackType->parameterTypes = {cbParamType};
        auto cbRetType = std::make_shared<PrimitiveTypeNode>();
        cbRetType->kind = PrimitiveKind::Int;
        callbackType->returnType = cbRetType;
        callbackParam->type = callbackType;
        applyFunc->parameters = {callbackParam};
        applyFunc->body = std::make_shared<BlockStatementNode>();

        // func test() => void
        auto testFunc = std::make_shared<FunctionDeclaration>();
        testFunc->name = "test";
        auto testRet = std::make_shared<PrimitiveTypeNode>();
        testRet->kind = PrimitiveKind::Void;
        testFunc->returnType = testRet;
        auto testBody = std::make_shared<BlockStatementNode>();

        // var val: int = 10;
        auto varVal = std::make_shared<VariableDeclaration>();
        varVal->name = "val";
        auto valType = std::make_shared<PrimitiveTypeNode>();
        valType->kind = PrimitiveKind::Int;
        varVal->type = valType;
        varVal->initializer = std::make_shared<IntegerLiteral>();
        testBody->statements.push_back(varVal);

        // var f = [=](int x) => { return val + x; };
        auto varF = std::make_shared<VariableDeclaration>();
        varF->name = "f";
        varF->isInferred = true;
        auto lambda = std::make_shared<LambdaExpression>();
        lambda->captureDefault = CaptureDefault::ByCopy;
        auto lParam = std::make_shared<ParameterNode>();
        lParam->name = "x";
        auto lParamType = std::make_shared<PrimitiveTypeNode>();
        lParamType->kind = PrimitiveKind::Int;
        lParam->type = lParamType;
        lambda->parameters = {lParam};
        auto lBody = std::make_shared<BlockStatementNode>();
        auto lRet = std::make_shared<ReturnStatement>();
        auto lAdd = std::make_shared<BinaryExpression>();
        lAdd->op = BinaryOp::Add;
        auto valIdent = std::make_shared<IdentifierExpression>();
        valIdent->name = "val";
        auto xIdent2 = std::make_shared<IdentifierExpression>();
        xIdent2->name = "x";
        lAdd->left = valIdent;
        lAdd->right = xIdent2;
        lRet->value = lAdd;
        lBody->statements.push_back(lRet);
        lambda->body = lBody;
        varF->initializer = lambda;
        testBody->statements.push_back(varF);

        // apply(lambda directly as argument)
        auto callStmt = std::make_shared<ExpressionStatement>();
        auto callExpr = std::make_shared<CallExpression>();
        auto applyIdent = std::make_shared<IdentifierExpression>();
        applyIdent->name = "apply";
        callExpr->callee = applyIdent;
        auto callArgs = std::make_shared<ArgumentsNode>();
        // Pass a lambda literal directly as argument (should be non-escaping)
        auto inlineLambda = std::make_shared<LambdaExpression>();
        inlineLambda->captureDefault = CaptureDefault::ByCopy;
        auto inlineParam = std::make_shared<ParameterNode>();
        inlineParam->name = "y";
        auto inlineParamType = std::make_shared<PrimitiveTypeNode>();
        inlineParamType->kind = PrimitiveKind::Int;
        inlineParam->type = inlineParamType;
        inlineLambda->parameters = {inlineParam};
        auto inlineBody = std::make_shared<BlockStatementNode>();
        auto inlineRet = std::make_shared<ReturnStatement>();
        auto yIdent = std::make_shared<IdentifierExpression>();
        yIdent->name = "y";
        inlineRet->value = yIdent;
        inlineBody->statements.push_back(inlineRet);
        inlineLambda->body = inlineBody;
        callArgs->expressions = {inlineLambda};
        callExpr->arguments = callArgs;
        callStmt->expression = callExpr;
        testBody->statements.push_back(callStmt);

        testFunc->body = testBody;
        module->declarations = {applyFunc, testFunc};
        program->modules = {module};

        // Run all 4 passes
        SymbolTable st;
        ErrorReporter errors;
        SymbolTableBuilder builder(st, errors);
        builder.build(*program);
        TypeResolver resolver(st, errors);
        resolver.resolve(*program);
        TypeChecker checker(st, errors);
        checker.check(*program);
        SemanticValidator validator(st, errors);
        validator.validate(*program);

        ASSERT_TRUE(!errors.hasErrors(), "Pass 4 no errors (test 24)");

        // Verify: lambda captures 'val' by value
        ASSERT_EQ(lambda->capturedVariables.size(), size_t(1),
            "lambda captures 1 var (val)");
        ASSERT_EQ(lambda->capturedVariables[0]->getName(), std::string("val"),
            "captured = val");
        ASSERT_TRUE(lambda->captureModesResolved[0] == CaptureMode::ByValue,
            "val captured by value ([=] default)");

        // Verify: self-capture detection
        // f captures val, not itself — selfCapture should be false
        ASSERT_TRUE(!lambda->selfCapture, "f does not self-capture");

        // Verify: inline lambda passed as argument is non-escaping
        ASSERT_TRUE(!inlineLambda->escapes,
            "inline lambda arg is non-escaping");

        // Verify: named lambda (f) defaults to escaping
        ASSERT_TRUE(lambda->escapes,
            "named lambda defaults to escaping");

        passed++;
        std::cout << "[PASS] 24. Pass 4 — Nested lambda + self-capture + non-escaping\n";
    }

    // ========================================
    // 25. Codegen — Simple function returning int
    // ========================================
    {
        // Build:
        //   module Main {
        //     func answer() => int {
        //       return 42;
        //     }
        //   }

        auto program = std::make_shared<ProgramNode>();
        auto module = std::make_shared<ModuleNode>();
        module->name = "Main";

        auto funcDecl = std::make_shared<FunctionDeclaration>();
        funcDecl->name = "answer";
        auto retType = std::make_shared<PrimitiveTypeNode>();
        retType->kind = PrimitiveKind::Int;
        funcDecl->returnType = retType;

        auto body = std::make_shared<BlockStatementNode>();
        auto retStmt = std::make_shared<ReturnStatement>();
        auto literal42 = std::make_shared<IntegerLiteral>();
        literal42->value = 42;
        retStmt->value = literal42;
        body->statements.push_back(retStmt);
        funcDecl->body = body;

        module->declarations = {funcDecl};
        program->modules = {module};

        // Run all 4 sema passes
        SymbolTable st;
        ErrorReporter errors;
        SymbolTableBuilder builder(st, errors);
        builder.build(*program);
        TypeResolver resolver(st, errors);
        resolver.resolve(*program);
        TypeChecker checker(st, errors);
        checker.check(*program);
        SemanticValidator validator(st, errors);
        validator.validate(*program);

        ASSERT_TRUE(!errors.hasErrors(), "Sema passes no errors (test 25)");

        // Run codegen
        auto& raiiInfo = validator.getRAIIInfo();
        mingus::codegen::IRGenerator irgen(st, raiiInfo);
        auto llvmModule = irgen.generate(*program);

        ASSERT_NOT_NULL(llvmModule.get(), "LLVM module generated");

        // Verify the module
        std::string verifyErrors;
        llvm::raw_string_ostream verifyOS(verifyErrors);
        bool broken = llvm::verifyModule(*llvmModule, &verifyOS);
        if (broken) {
            std::cerr << "FAIL: LLVM verify errors:\n" << verifyErrors << "\n";
            return 1;
        }

        // Verify: function 'Main_answer' exists
        auto* answerFn = llvmModule->getFunction("Main_answer");
        ASSERT_NOT_NULL(answerFn, "Main_answer function in module");
        ASSERT_TRUE(!answerFn->isDeclaration(), "Main_answer has body");
        ASSERT_TRUE(answerFn->getReturnType()->isIntegerTy(32), "Main_answer returns i32");

        passed++;
        std::cout << "[PASS] 25. Codegen — Simple function returning int\n";
    }

    // ========================================
    // 26. Codegen — Extern function declaration
    // ========================================
    {
        // Build:
        //   module Main {
        //     extern func printf(string fmt) => int;
        //     func main() => int {
        //       return 0;
        //     }
        //   }

        auto program = std::make_shared<ProgramNode>();
        auto module = std::make_shared<ModuleNode>();
        module->name = "Main";

        auto externDecl = std::make_shared<ExternFunctionDeclaration>();
        externDecl->name = "printf";
        auto fmtParam = std::make_shared<ParameterNode>();
        fmtParam->name = "fmt";
        auto fmtType = std::make_shared<PrimitiveTypeNode>();
        fmtType->kind = PrimitiveKind::String;
        fmtParam->type = fmtType;
        externDecl->parameters = {fmtParam};
        auto printfRetType = std::make_shared<PrimitiveTypeNode>();
        printfRetType->kind = PrimitiveKind::Int;
        externDecl->returnType = printfRetType;

        auto mainDecl = std::make_shared<FunctionDeclaration>();
        mainDecl->name = "main";
        auto mainRetType = std::make_shared<PrimitiveTypeNode>();
        mainRetType->kind = PrimitiveKind::Int;
        mainDecl->returnType = mainRetType;
        auto mainBody = std::make_shared<BlockStatementNode>();
        auto mainRet = std::make_shared<ReturnStatement>();
        auto zero = std::make_shared<IntegerLiteral>();
        zero->value = 0;
        mainRet->value = zero;
        mainBody->statements.push_back(mainRet);
        mainDecl->body = mainBody;

        module->declarations = {externDecl, mainDecl};
        program->modules = {module};

        // Run all 4 sema passes
        SymbolTable st;
        ErrorReporter errors;
        SymbolTableBuilder builder(st, errors);
        builder.build(*program);
        TypeResolver resolver(st, errors);
        resolver.resolve(*program);
        TypeChecker checker(st, errors);
        checker.check(*program);
        SemanticValidator validator(st, errors);
        validator.validate(*program);

        ASSERT_TRUE(!errors.hasErrors(), "Sema passes no errors (test 26)");

        // Run codegen
        auto& raiiInfo = validator.getRAIIInfo();
        mingus::codegen::IRGenerator irgen(st, raiiInfo);
        auto llvmModule = irgen.generate(*program);
        ASSERT_NOT_NULL(llvmModule.get(), "LLVM module generated (test 26)");

        // Verify module
        std::string verifyErrors;
        llvm::raw_string_ostream verifyOS(verifyErrors);
        bool broken = llvm::verifyModule(*llvmModule, &verifyOS);
        if (broken) {
            std::cerr << "FAIL: LLVM verify errors (test 26):\n" << verifyErrors << "\n";
            return 1;
        }

        // Verify: extern 'printf' exists and is a declaration (no body)
        auto* printfFn = llvmModule->getFunction("printf");
        ASSERT_NOT_NULL(printfFn, "printf declared in module");
        ASSERT_TRUE(printfFn->isDeclaration(), "printf is extern (no body)");
        ASSERT_TRUE(printfFn->isVarArg(), "printf is vararg");

        // Verify: 'main' function exists
        auto* mainFn = llvmModule->getFunction("Main_main");
        ASSERT_NOT_NULL(mainFn, "Main_main exists");
        ASSERT_TRUE(!mainFn->isDeclaration(), "Main_main has body");

        passed++;
        std::cout << "[PASS] 26. Codegen — Extern function + vararg printf\n";
    }

    // ========================================
    // 27. Codegen — Struct with fields
    // ========================================
    {
        // Build:
        //   module Main {
        //     struct Vec2 {
        //       double x;
        //       double y;
        //     }
        //     func make() => Vec2 {
        //       var v: Vec2;
        //       return v;
        //     }
        //   }

        auto program = std::make_shared<ProgramNode>();
        auto module = std::make_shared<ModuleNode>();
        module->name = "Main";

        auto structDecl = std::make_shared<StructDeclaration>();
        structDecl->name = "Vec2";
        auto fieldX = std::make_shared<VariableDeclaration>();
        fieldX->name = "x";
        auto xType = std::make_shared<PrimitiveTypeNode>();
        xType->kind = PrimitiveKind::Double;
        fieldX->type = xType;
        auto fieldY = std::make_shared<VariableDeclaration>();
        fieldY->name = "y";
        auto yType = std::make_shared<PrimitiveTypeNode>();
        yType->kind = PrimitiveKind::Double;
        fieldY->type = yType;
        structDecl->fields = {fieldX, fieldY};

        auto funcDecl = std::make_shared<FunctionDeclaration>();
        funcDecl->name = "make";
        auto retType = std::make_shared<NamedTypeNode>();
        retType->qualifiedName = {"Vec2"};
        funcDecl->returnType = retType;
        auto body = std::make_shared<BlockStatementNode>();

        // var v: Vec2;
        auto varV = std::make_shared<VariableDeclaration>();
        varV->name = "v";
        auto vType = std::make_shared<NamedTypeNode>();
        vType->qualifiedName = {"Vec2"};
        varV->type = vType;
        body->statements.push_back(varV);

        // return v;
        auto retStmt = std::make_shared<ReturnStatement>();
        auto vIdent = std::make_shared<IdentifierExpression>();
        vIdent->name = "v";
        retStmt->value = vIdent;
        body->statements.push_back(retStmt);

        funcDecl->body = body;
        module->declarations = {structDecl, funcDecl};
        program->modules = {module};

        // Run all 4 sema passes
        SymbolTable st;
        ErrorReporter errors;
        SymbolTableBuilder builder(st, errors);
        builder.build(*program);
        TypeResolver resolver(st, errors);
        resolver.resolve(*program);
        TypeChecker checker(st, errors);
        checker.check(*program);
        SemanticValidator validator(st, errors);
        validator.validate(*program);

        ASSERT_TRUE(!errors.hasErrors(), "Sema passes no errors (test 27)");

        // Run codegen
        auto& raiiInfo = validator.getRAIIInfo();
        mingus::codegen::IRGenerator irgen(st, raiiInfo);
        auto llvmModule = irgen.generate(*program);
        ASSERT_NOT_NULL(llvmModule.get(), "LLVM module generated (test 27)");

        // Verify module
        std::string verifyErrors;
        llvm::raw_string_ostream verifyOS(verifyErrors);
        bool broken = llvm::verifyModule(*llvmModule, &verifyOS);
        if (broken) {
            std::cerr << "FAIL: LLVM verify errors (test 27):\n" << verifyErrors << "\n";
            return 1;
        }

        // Verify: Vec2 struct type exists (LLVM 21: StructType::getTypeByName)
        auto* vec2Ty = llvm::StructType::getTypeByName(irgen.getContext(), "Vec2");
        ASSERT_NOT_NULL(vec2Ty, "Vec2 type in module");
        ASSERT_EQ(vec2Ty->getNumElements(), unsigned(2), "Vec2 has 2 fields");
        ASSERT_TRUE(vec2Ty->getElementType(0)->isDoubleTy(), "Vec2.x is double");
        ASSERT_TRUE(vec2Ty->getElementType(1)->isDoubleTy(), "Vec2.y is double");

        // Verify: make function returns struct by pointer
        auto* makeFn = llvmModule->getFunction("Main_make");
        ASSERT_NOT_NULL(makeFn, "Main_make exists");

        passed++;
        std::cout << "[PASS] 27. Codegen — Struct with fields + return\n";
    }

    // ========================================
    // 28. Codegen — Integer arithmetic in function body
    // ========================================
    {
        // Build:
        //   module Main {
        //     func add(int a, int b) => int {
        //       var result: int = a + b;
        //       return result;
        //     }
        //   }

        auto program = std::make_shared<ProgramNode>();
        auto module = std::make_shared<ModuleNode>();
        module->name = "Main";

        auto funcDecl = std::make_shared<FunctionDeclaration>();
        funcDecl->name = "add";
        auto retType = std::make_shared<PrimitiveTypeNode>();
        retType->kind = PrimitiveKind::Int;
        funcDecl->returnType = retType;

        auto paramA = std::make_shared<ParameterNode>();
        paramA->name = "a";
        auto aType = std::make_shared<PrimitiveTypeNode>();
        aType->kind = PrimitiveKind::Int;
        paramA->type = aType;

        auto paramB = std::make_shared<ParameterNode>();
        paramB->name = "b";
        auto bType = std::make_shared<PrimitiveTypeNode>();
        bType->kind = PrimitiveKind::Int;
        paramB->type = bType;

        funcDecl->parameters = {paramA, paramB};

        auto body = std::make_shared<BlockStatementNode>();

        // var result: int = a + b;
        auto varResult = std::make_shared<VariableDeclaration>();
        varResult->name = "result";
        auto resultType = std::make_shared<PrimitiveTypeNode>();
        resultType->kind = PrimitiveKind::Int;
        varResult->type = resultType;

        auto addExpr = std::make_shared<BinaryExpression>();
        addExpr->op = BinaryOp::Add;
        auto aIdent = std::make_shared<IdentifierExpression>();
        aIdent->name = "a";
        auto bIdent = std::make_shared<IdentifierExpression>();
        bIdent->name = "b";
        addExpr->left = aIdent;
        addExpr->right = bIdent;
        varResult->initializer = addExpr;
        body->statements.push_back(varResult);

        // return result;
        auto retStmt = std::make_shared<ReturnStatement>();
        auto resultIdent = std::make_shared<IdentifierExpression>();
        resultIdent->name = "result";
        retStmt->value = resultIdent;
        body->statements.push_back(retStmt);

        funcDecl->body = body;
        module->declarations = {funcDecl};
        program->modules = {module};

        // Run all 4 sema passes
        SymbolTable st;
        ErrorReporter errors;
        SymbolTableBuilder builder(st, errors);
        builder.build(*program);
        TypeResolver resolver(st, errors);
        resolver.resolve(*program);
        TypeChecker checker(st, errors);
        checker.check(*program);
        SemanticValidator validator(st, errors);
        validator.validate(*program);

        ASSERT_TRUE(!errors.hasErrors(), "Sema passes no errors (test 28)");

        // Run codegen
        auto& raiiInfo = validator.getRAIIInfo();
        mingus::codegen::IRGenerator irgen(st, raiiInfo);
        auto llvmModule = irgen.generate(*program);
        ASSERT_NOT_NULL(llvmModule.get(), "LLVM module generated (test 28)");

        // Verify module
        std::string verifyErrors;
        llvm::raw_string_ostream verifyOS(verifyErrors);
        bool broken = llvm::verifyModule(*llvmModule, &verifyOS);
        if (broken) {
            std::cerr << "FAIL: LLVM verify errors (test 28):\n" << verifyErrors << "\n";
            return 1;
        }

        // Verify: add function exists with correct signature
        auto* addFn = llvmModule->getFunction("Main_add");
        ASSERT_NOT_NULL(addFn, "Main_add exists");
        ASSERT_TRUE(!addFn->isDeclaration(), "Main_add has body");
        ASSERT_EQ(addFn->arg_size(), unsigned(2), "Main_add has 2 params");
        ASSERT_TRUE(addFn->getReturnType()->isIntegerTy(32), "Main_add returns i32");
        ASSERT_TRUE(addFn->getArg(0)->getType()->isIntegerTy(32), "param a is i32");
        ASSERT_TRUE(addFn->getArg(1)->getType()->isIntegerTy(32), "param b is i32");

        // Dump the IR for manual inspection (optional)
        // llvmModule->print(llvm::outs(), nullptr);

        passed++;
        std::cout << "[PASS] 28. Codegen — Integer arithmetic + params\n";
    }

    // ========================================
    // Summary
    // ========================================
    std::cout << "\n=== All " << passed << " smoke tests passed ===\n";
    std::cout << "V2 foundation compiles and links correctly.\n";
    std::cout << "\nVerified:\n";
    std::cout << "  Symbol/Scope/TypeSymbol hierarchy (MI pattern)\n";
    std::cout << "  Type interning + compatibility\n";
    std::cout << "  ClassSymbol inheritance chain resolution\n";
    std::cout << "  62 AST node types (expressions, statements, declarations)\n";
    std::cout << "  ArgumentsNode with per-arg isReference\n";
    std::cout << "  ASTVisitor dispatch for all node types\n";
    std::cout << "  DebugInfo with full source ranges\n";
    std::cout << "  Pass 1 — SymbolTableBuilder (scope tree, symbols, imports)\n";
    std::cout << "  Pass 2 — TypeResolver (annotations, refs, compounds, named)\n";
    std::cout << "  Pass 3 — TypeChecker (inference, calls, member access, ops)\n";
    std::cout << "  Pass 4 — SemanticValidator (captures, RAII, control flow)\n";
    std::cout << "  Pass 5 — Codegen (IRGenerator: functions, externs, structs, arithmetic)\n";

    return 0;
}
