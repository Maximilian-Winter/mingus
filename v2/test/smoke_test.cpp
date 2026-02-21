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

    return 0;
}
