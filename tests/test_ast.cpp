//================================================================================
// MINGUS v1 - AST Unit Tests
//================================================================================

#include "mingus/AST.h"
#include <gtest/gtest.h>

using namespace mingus::ast;

//================================================================================
// Type Tests
//================================================================================
TEST(TypeTest, PrimitiveTypeCreation) {
    auto intType = PrimitiveType::getInt();
    EXPECT_EQ(intType->getKind(), Type::Kind::Primitive);
    EXPECT_EQ(intType->kind, PrimitiveType::PrimitiveKind::Int);
    EXPECT_EQ(intType->toString(), "int");

    auto doubleType = PrimitiveType::getDouble();
    EXPECT_EQ(doubleType->kind, PrimitiveType::PrimitiveKind::Double);
    EXPECT_EQ(doubleType->toString(), "double");
}

TEST(TypeTest, PointerTypeCreation) {
    auto intType = PrimitiveType::getInt();
    auto intPtr = std::make_shared<PointerType>(intType);
    
    EXPECT_EQ(intPtr->getKind(), Type::Kind::Pointer);
    EXPECT_EQ(intPtr->toString(), "int*");
    EXPECT_TRUE(intPtr->baseType->equals(*intType));
}

TEST(TypeTest, ArrayTypeCreation) {
    auto intType = PrimitiveType::getInt();
    auto intArray = std::make_shared<ArrayType>(intType, 10);
    
    EXPECT_EQ(intArray->getKind(), Type::Kind::Array);
    EXPECT_EQ(intArray->size, 10);
    EXPECT_FALSE(intArray->isUnsized());
    EXPECT_EQ(intArray->toString(), "int[10]");

    auto unsizedArray = std::make_shared<ArrayType>(intType);
    EXPECT_TRUE(unsizedArray->isUnsized());
    EXPECT_EQ(unsizedArray->toString(), "int[]");
}

TEST(TypeTest, FunctionTypeCreation) {
    auto intType = PrimitiveType::getInt();
    auto doubleType = PrimitiveType::getDouble();
    
    TypeList<Type> params = { intType, doubleType };
    auto funcType = std::make_shared<FunctionType>(params, intType);
    
    EXPECT_EQ(funcType->getKind(), Type::Kind::Function);
    EXPECT_EQ(funcType->parameterTypes.size(), 2);
    EXPECT_TRUE(funcType->returnType->equals(*intType));
}

TEST(TypeTest, TypeEquality) {
    auto int1 = PrimitiveType::getInt();
    auto int2 = PrimitiveType::getInt();
    auto doubleType = PrimitiveType::getDouble();
    
    EXPECT_TRUE(int1->equals(*int2));
    EXPECT_FALSE(int1->equals(*doubleType));
    
    auto ptr1 = std::make_shared<PointerType>(int1);
    auto ptr2 = std::make_shared<PointerType>(int2);
    EXPECT_TRUE(ptr1->equals(*ptr2));
}

TEST(TypeTest, ClassTypeCreation) {
    auto classType = std::make_shared<ClassType>("MyClass", "MyModule");
    EXPECT_EQ(classType->name, "MyClass");
    EXPECT_EQ(classType->module, "MyModule");
    EXPECT_EQ(classType->toString(), "MyModule.MyClass");
    EXPECT_FALSE(classType->hasDestructor);
    
    auto noModuleClass = std::make_shared<ClassType>("SimpleClass");
    EXPECT_EQ(noModuleClass->toString(), "SimpleClass");
}

TEST(TypeTest, EnumTypeCreation) {
    auto intType = PrimitiveType::getInt();
    auto enumType = std::make_shared<EnumType>("Color", intType);
    
    EXPECT_EQ(enumType->name, "Color");
    EXPECT_TRUE(enumType->underlyingType->equals(*intType));
    
    enumType->members.push_back(EnumMember("Red", 0, true));
    enumType->members.push_back(EnumMember("Green", 1, true));
    enumType->members.push_back(EnumMember("Blue", 2, true));
    
    EXPECT_EQ(enumType->members.size(), 3);
    
    auto redValue = enumType->getMemberValue("Red");
    EXPECT_TRUE(redValue.has_value());
    EXPECT_EQ(redValue.value(), 0);
    
    auto missingValue = enumType->getMemberValue("Yellow");
    EXPECT_FALSE(missingValue.has_value());
}

//================================================================================
// AST Node Tests
//================================================================================
TEST(ASTNodeTest, SourceLocation) {
    SourceLocation loc("test.mingus", 42, 10);
    EXPECT_EQ(loc.file, "test.mingus");
    EXPECT_EQ(loc.line, 42);
    EXPECT_EQ(loc.column, 10);
    EXPECT_EQ(loc.toString(), "test.mingus:42:10");
}

TEST(ASTNodeTest, QualifiedName) {
    QualifiedName simple("value");
    EXPECT_TRUE(simple.isSimple());
    EXPECT_EQ(simple.getSimpleName(), "value");
    
    QualifiedName qualified(std::vector<std::string>{"Module", "SubModule", "Class"});
    EXPECT_FALSE(qualified.isSimple());
    EXPECT_EQ(qualified.toString(), "Module.SubModule.Class");
    EXPECT_EQ(qualified.getSimpleName(), "Class");
}

//================================================================================
// Literal Tests
//================================================================================
TEST(LiteralTest, IntegerLiteral) {
    SourceLocation loc;
    auto lit = std::make_shared<IntegerLiteral>(42, loc);
    EXPECT_EQ(lit->value, 42);
}

TEST(LiteralTest, FloatLiteral) {
    SourceLocation loc;
    auto lit = std::make_shared<FloatLiteral>(3.14159, loc);
    EXPECT_DOUBLE_EQ(lit->value, 3.14159);
}

TEST(LiteralTest, BoolLiteral) {
    SourceLocation loc;
    auto trueLit = std::make_shared<BoolLiteral>(true, loc);
    auto falseLit = std::make_shared<BoolLiteral>(false, loc);
    EXPECT_TRUE(trueLit->value);
    EXPECT_FALSE(falseLit->value);
}

TEST(LiteralTest, StringLiteral) {
    SourceLocation loc;
    auto lit = std::make_shared<StringLiteral>("hello world", loc);
    EXPECT_EQ(lit->value, "hello world");
}

//================================================================================
// Expression Tests
//================================================================================
TEST(ExpressionTest, BinaryExpression) {
    SourceLocation loc;
    auto left = std::make_shared<IntegerLiteral>(10, loc);
    auto right = std::make_shared<IntegerLiteral>(20, loc);
    auto expr = std::make_shared<BinaryExpression>(left, BinaryOp::Add, right, loc);
    
    EXPECT_EQ(expr->op, BinaryOp::Add);
    EXPECT_FALSE(expr->isOperatorOverload);
    EXPECT_EQ(binaryOpToString(expr->op), "+");
    EXPECT_TRUE(binaryOpIsArithmetic(expr->op));
    EXPECT_FALSE(binaryOpIsComparison(expr->op));
}

TEST(ExpressionTest, BinaryExpressionComparison) {
    SourceLocation loc;
    auto left = std::make_shared<IntegerLiteral>(1, loc);
    auto right = std::make_shared<IntegerLiteral>(2, loc);
    auto expr = std::make_shared<BinaryExpression>(left, BinaryOp::Less, right, loc);
    
    EXPECT_TRUE(binaryOpIsComparison(expr->op));
    EXPECT_FALSE(binaryOpIsLogical(expr->op));
    EXPECT_EQ(binaryOpToString(expr->op), "<");
}

TEST(ExpressionTest, UnaryExpression) {
    SourceLocation loc;
    auto operand = std::make_shared<IdentifierExpression>("x", loc);
    auto expr = std::make_shared<UnaryExpression>(UnaryOp::Negate, operand, loc);
    
    EXPECT_EQ(expr->op, UnaryOp::Negate);
    EXPECT_TRUE(unaryOpIsPrefix(expr->op));
    EXPECT_FALSE(unaryOpIsPostfix(expr->op));
    EXPECT_EQ(unaryOpToString(expr->op), "-");
}

TEST(ExpressionTest, UnaryExpressionPostfix) {
    SourceLocation loc;
    auto operand = std::make_shared<IdentifierExpression>("i", loc);
    auto expr = std::make_shared<UnaryExpression>(UnaryOp::PostIncrement, operand, loc);
    
    EXPECT_TRUE(unaryOpIsPostfix(expr->op));
    EXPECT_FALSE(unaryOpIsPrefix(expr->op));
}

TEST(ExpressionTest, AssignmentExpression) {
    SourceLocation loc;
    auto target = std::make_shared<IdentifierExpression>("x", loc);
    auto value = std::make_shared<IntegerLiteral>(5, loc);
    auto expr = std::make_shared<AssignmentExpression>(target, AssignOp::Assign, value, loc);
    
    EXPECT_EQ(expr->op, AssignOp::Assign);
    EXPECT_FALSE(assignOpIsCompound(expr->op));
    EXPECT_EQ(assignOpToString(expr->op), "=");
}

TEST(ExpressionTest, CompoundAssignment) {
    SourceLocation loc;
    auto target = std::make_shared<IdentifierExpression>("x", loc);
    auto value = std::make_shared<IntegerLiteral>(1, loc);
    auto expr = std::make_shared<AssignmentExpression>(target, AssignOp::AddAssign, value, loc);
    
    EXPECT_TRUE(assignOpIsCompound(expr->op));
    EXPECT_EQ(assignOpToString(expr->op), "+=");
    EXPECT_EQ(assignOpToBinaryOp(expr->op), BinaryOp::Add);
}

TEST(ExpressionTest, CallExpression) {
    SourceLocation loc;
    auto callee = std::make_shared<IdentifierExpression>("print", loc);
    NodeList<ExpressionNode> args = {
        std::make_shared<StringLiteral>("hello", loc)
    };
    auto call = std::make_shared<CallExpression>(callee, args, loc);
    
    EXPECT_EQ(call->arguments.size(), 1);
}

TEST(ExpressionTest, MemberAccessExpression) {
    SourceLocation loc;
    auto object = std::make_shared<IdentifierExpression>("obj", loc);
    auto access = std::make_shared<MemberAccessExpression>(object, "field", false, loc);
    
    EXPECT_EQ(access->memberName, "field");
    EXPECT_FALSE(access->isArrow);
    
    auto arrowAccess = std::make_shared<MemberAccessExpression>(object, "data", true, loc);
    EXPECT_TRUE(arrowAccess->isArrow);
}

TEST(ExpressionTest, NewExpressionObject) {
    SourceLocation loc;
    auto type = std::make_shared<NamedTypeNode>(std::vector<std::string>{"MyClass"}, loc);
    NodeList<ExpressionNode> args;
    auto newExpr = std::make_shared<NewExpression>(type, args, loc);
    
    EXPECT_FALSE(newExpr->isArray);
    EXPECT_EQ(newExpr->arguments.size(), 0);
}

TEST(ExpressionTest, NewExpressionArray) {
    SourceLocation loc;
    auto type = std::make_shared<PrimitiveTypeNode>(PrimitiveType::PrimitiveKind::Int, loc);
    auto size = std::make_shared<IntegerLiteral>(10, loc);
    auto newExpr = std::make_shared<NewExpression>(type, size, loc);
    
    EXPECT_TRUE(newExpr->isArray);
    EXPECT_NE(newExpr->arraySize, nullptr);
}

//================================================================================
// Statement Tests
//================================================================================
TEST(StatementTest, BlockStatement) {
    SourceLocation loc;
    NodeList<StatementNode> stmts;
    auto block = std::make_shared<BlockStatement>(stmts, loc);
    
    EXPECT_TRUE(block->isEmpty());
    
    stmts.push_back(std::make_shared<BreakStatement>(loc));
    auto block2 = std::make_shared<BlockStatement>(stmts, loc);
    EXPECT_FALSE(block2->isEmpty());
}

TEST(StatementTest, ReturnStatement) {
    SourceLocation loc;
    auto voidReturn = std::make_shared<ReturnStatement>(nullptr, loc);
    EXPECT_FALSE(voidReturn->hasValue());
    
    auto value = std::make_shared<IntegerLiteral>(42, loc);
    auto valueReturn = std::make_shared<ReturnStatement>(value, loc);
    EXPECT_TRUE(valueReturn->hasValue());
}

TEST(StatementTest, IfStatement) {
    SourceLocation loc;
    auto condition = std::make_shared<BoolLiteral>(true, loc);
    auto thenBody = std::make_shared<BlockStatement>(NodeList<StatementNode>{}, loc);
    
    auto ifStmt = std::make_shared<IfStatement>(condition, thenBody, 
                                                 std::vector<ElseIfClause>{}, nullptr, loc);
    
    EXPECT_FALSE(ifStmt->hasElse());
    EXPECT_FALSE(ifStmt->hasElseIf());
    
    auto elseBody = std::make_shared<BlockStatement>(NodeList<StatementNode>{}, loc);
    auto ifElseStmt = std::make_shared<IfStatement>(condition, thenBody, 
                                                     std::vector<ElseIfClause>{}, elseBody, loc);
    EXPECT_TRUE(ifElseStmt->hasElse());
}

TEST(StatementTest, ForStatement) {
    SourceLocation loc;
    auto condition = std::make_shared<BoolLiteral>(true, loc);
    auto body = std::make_shared<BlockStatement>(NodeList<StatementNode>{}, loc);
    
    // for (; true; ) {}
    auto forStmt = std::make_shared<ForStatement>(
        NodeList<ExpressionNode>{}, condition, NodeList<ExpressionNode>{}, body, loc
    );
    
    EXPECT_FALSE(forStmt->hasInitDeclaration());
    EXPECT_FALSE(forStmt->hasInitExpressions());
    EXPECT_TRUE(forStmt->hasCondition());
    EXPECT_FALSE(forStmt->hasIterators());
}

TEST(StatementTest, WhileStatement) {
    SourceLocation loc;
    auto condition = std::make_shared<BoolLiteral>(true, loc);
    auto body = std::make_shared<BlockStatement>(NodeList<StatementNode>{}, loc);
    
    auto whileStmt = std::make_shared<WhileStatement>(condition, body, loc);
    EXPECT_NE(whileStmt->condition, nullptr);
    EXPECT_NE(whileStmt->body, nullptr);
}

//================================================================================
// Declaration Tests
//================================================================================
TEST(DeclarationTest, VariableDeclaration) {
    SourceLocation loc;
    auto type = std::make_shared<PrimitiveTypeNode>(PrimitiveType::PrimitiveKind::Int, loc);
    auto init = std::make_shared<IntegerLiteral>(0, loc);
    
    auto varDecl = std::make_shared<VariableDeclaration>(
        "x",
        AccessModifier::Public,
        false,  // isStatic
        type,
        false,  // isInferred
        init,
        loc
    );
    
    EXPECT_EQ(varDecl->name, "x");
    EXPECT_EQ(varDecl->accessModifier, AccessModifier::Public);
    EXPECT_FALSE(varDecl->isInferred);
}

TEST(DeclarationTest, VariableDeclarationInferred) {
    SourceLocation loc;
    auto init = std::make_shared<IntegerLiteral>(42, loc);
    
    auto varDecl = std::make_shared<VariableDeclaration>(
        "y",
        AccessModifier::Private,
        false,
        nullptr,  // type is inferred
        true,     // isInferred
        init,
        loc
    );
    
    EXPECT_TRUE(varDecl->isInferred);
    EXPECT_EQ(varDecl->type, nullptr);
}

TEST(DeclarationTest, FunctionDeclaration) {
    SourceLocation loc;
    auto paramType = std::make_shared<PrimitiveTypeNode>(PrimitiveType::PrimitiveKind::Int, loc);
    auto param = std::make_shared<ParameterNode>("n", paramType, nullptr, loc);
    NodeList<ParameterNode> params = { param };
    
    auto returnType = std::make_shared<PrimitiveTypeNode>(PrimitiveType::PrimitiveKind::Int, loc);
    auto body = std::make_shared<BlockStatement>(NodeList<StatementNode>{}, loc);
    
    auto funcDecl = std::make_shared<FunctionDeclaration>(
        "factorial",
        AccessModifier::Public,
        false,  // isStatic
        false,  // isAbstract
        params,
        returnType,
        body,
        loc
    );
    
    EXPECT_EQ(funcDecl->name, "factorial");
    EXPECT_EQ(funcDecl->parameters.size(), 1);
    EXPECT_FALSE(funcDecl->isForwardDeclaration());
    EXPECT_FALSE(funcDecl->isAbstract);
}

TEST(DeclarationTest, AbstractFunctionDeclaration) {
    SourceLocation loc;
    NodeList<ParameterNode> params;
    auto returnType = std::make_shared<PrimitiveTypeNode>(PrimitiveType::PrimitiveKind::Void, loc);
    
    auto abstractFunc = std::make_shared<FunctionDeclaration>(
        "abstractMethod",
        AccessModifier::Public,
        false,
        true,  // isAbstract
        params,
        returnType,
        nullptr,  // no body
        loc
    );
    
    EXPECT_TRUE(abstractFunc->isAbstract);
    EXPECT_TRUE(abstractFunc->isForwardDeclaration());
}

TEST(DeclarationTest, ParameterWithDefault) {
    SourceLocation loc;
    auto type = std::make_shared<PrimitiveTypeNode>(PrimitiveType::PrimitiveKind::Int, loc);
    auto defaultVal = std::make_shared<IntegerLiteral>(10, loc);
    
    auto param = std::make_shared<ParameterNode>("count", type, defaultVal, loc);
    
    EXPECT_TRUE(param->hasDefaultValue());
    EXPECT_NE(param->defaultValue, nullptr);
}

TEST(DeclarationTest, ClassDeclaration) {
    SourceLocation loc;
    NodeList<VariableDeclaration> fields;
    NodeList<FunctionDeclaration> methods;
    NodeList<OperatorDeclaration> operators;
    
    auto classDecl = std::make_shared<ClassDeclaration>(
        "MyClass",
        AccessModifier::Public,
        false,  // isStatic
        false,  // isAbstract
        std::vector<QualifiedName>{},  // base classes
        nullptr,  // constructor
        nullptr,  // destructor
        operators,
        methods,
        fields,
        loc
    );
    
    EXPECT_EQ(classDecl->name, "MyClass");
    EXPECT_FALSE(classDecl->hasConstructor());
    EXPECT_FALSE(classDecl->hasDestructor());
    EXPECT_FALSE(classDecl->hasBaseClass());
}

TEST(DeclarationTest, EnumDeclaration) {
    SourceLocation loc;
    NodeList<EnumMemberNode> members;
    members.push_back(std::make_shared<EnumMemberNode>("Red", nullptr, loc));
    members.push_back(std::make_shared<EnumMemberNode>("Green", nullptr, loc));
    members.push_back(std::make_shared<EnumMemberNode>("Blue", nullptr, loc));
    
    auto underlyingType = std::make_shared<PrimitiveTypeNode>(
        PrimitiveType::PrimitiveKind::Int, loc
    );
    
    auto enumDecl = std::make_shared<EnumDeclaration>(
        "Color",
        AccessModifier::Public,
        underlyingType,
        members,
        loc
    );
    
    EXPECT_EQ(enumDecl->name, "Color");
    EXPECT_EQ(enumDecl->members.size(), 3);
}

//================================================================================
// Pattern Tests
//================================================================================
TEST(PatternTest, LiteralPattern) {
    SourceLocation loc;
    auto value = std::make_shared<IntegerLiteral>(42, loc);
    auto pattern = std::make_shared<LiteralPattern>(value, loc);
    
    EXPECT_NE(pattern->value, nullptr);
}

TEST(PatternTest, RangePattern) {
    SourceLocation loc;
    auto pattern = std::make_shared<RangePattern>(1, 10, loc);
    
    EXPECT_EQ(pattern->low, 1);
    EXPECT_EQ(pattern->high, 10);
}

TEST(PatternTest, WildcardPattern) {
    SourceLocation loc;
    auto pattern = std::make_shared<WildcardPattern>(loc);
    // Wildcard pattern always matches
}

TEST(PatternTest, BindingPattern) {
    SourceLocation loc;
    auto pattern = std::make_shared<BindingPattern>("x", loc);
    
    EXPECT_EQ(pattern->name, "x");
}

TEST(PatternTest, TuplePattern) {
    SourceLocation loc;
    PatternList<PatternNode> elements = {
        std::make_shared<BindingPattern>("a", loc),
        std::make_shared<BindingPattern>("b", loc)
    };
    auto pattern = std::make_shared<TuplePattern>(elements, loc);
    
    EXPECT_EQ(pattern->elements.size(), 2);
}

//================================================================================
// Complex AST Construction Tests
//================================================================================
TEST(ComplexASTTest, BuildFactorialFunction) {
    SourceLocation loc;
    
    // Build: fn factorial(n: int) -> int { ... }
    auto paramN = std::make_shared<ParameterNode>(
        "n",
        std::make_shared<PrimitiveTypeNode>(PrimitiveType::PrimitiveKind::Int, loc),
        nullptr, loc
    );
    NodeList<ParameterNode> params = { paramN };
    
    auto returnType = std::make_shared<PrimitiveTypeNode>(
        PrimitiveType::PrimitiveKind::Int, loc
    );
    
    // Build if statement: if (n <= 1) { return 1; } else { ... }
    auto nIdent = std::make_shared<IdentifierExpression>("n", loc);
    auto oneLit = std::make_shared<IntegerLiteral>(1, loc);
    auto condition = std::make_shared<BinaryExpression>(
        nIdent, BinaryOp::LessEqual, oneLit, loc
    );
    
    auto returnOne = std::make_shared<ReturnStatement>(oneLit, loc);
    NodeList<StatementNode> thenStmts = { returnOne };
    auto thenBody = std::make_shared<BlockStatement>(thenStmts, loc);
    
    // Recursive case
    auto nIdent2 = std::make_shared<IdentifierExpression>("n", loc);
    auto factorialCall = std::make_shared<CallExpression>(
        std::make_shared<IdentifierExpression>("factorial", loc),
        NodeList<ExpressionNode>{std::make_shared<BinaryExpression>(
            std::make_shared<IdentifierExpression>("n", loc),
            BinaryOp::Sub,
            std::make_shared<IntegerLiteral>(1, loc),
            loc
        )},
        loc
    );
    
    auto returnRecursive = std::make_shared<ReturnStatement>(
        std::make_shared<BinaryExpression>(nIdent2, BinaryOp::Mul, factorialCall, loc),
        loc
    );
    NodeList<StatementNode> elseStmts = { returnRecursive };
    auto elseBody = std::make_shared<BlockStatement>(elseStmts, loc);
    
    auto ifStmt = std::make_shared<IfStatement>(
        condition, thenBody, std::vector<ElseIfClause>{}, elseBody, loc
    );
    
    NodeList<StatementNode> bodyStmts = { ifStmt };
    auto body = std::make_shared<BlockStatement>(bodyStmts, loc);
    
    auto funcDecl = std::make_shared<FunctionDeclaration>(
        "factorial",
        AccessModifier::Public,
        false, false,
        params, returnType, body, loc
    );
    
    EXPECT_EQ(funcDecl->name, "factorial");
    EXPECT_FALSE(funcDecl->body->isEmpty());
}

TEST(ComplexASTTest, LambdaExpression) {
    SourceLocation loc;
    
    // Build: (x: int) => x * 2
    auto paramX = std::make_shared<ParameterNode>(
        "x",
        std::make_shared<PrimitiveTypeNode>(PrimitiveType::PrimitiveKind::Int, loc),
        nullptr, loc
    );
    NodeList<ParameterNode> params = { paramX };
    
    auto body = std::make_shared<BinaryExpression>(
        std::make_shared<IdentifierExpression>("x", loc),
        BinaryOp::Mul,
        std::make_shared<IntegerLiteral>(2, loc),
        loc
    );
    
    auto lambda = std::make_shared<LambdaExpression>(params, body, loc);
    
    EXPECT_EQ(lambda->parameters.size(), 1);
    EXPECT_TRUE(lambda->hasExpressionBody());
    EXPECT_FALSE(lambda->hasBlockBody());
}

//================================================================================
// Node Type Tests
//================================================================================
TEST(NodeTypeTest, AsAndIsMethods) {
    SourceLocation loc;
    
    NodePtr<ExpressionNode> expr = std::make_shared<IntegerLiteral>(42, loc);
    
    EXPECT_TRUE(expr->is<IntegerLiteral>());
    EXPECT_FALSE(expr->is<FloatLiteral>());
    
    auto intLit = expr->as<IntegerLiteral>();
    EXPECT_NE(intLit, nullptr);
    EXPECT_EQ(intLit->value, 42);
    
    auto floatLit = expr->as<FloatLiteral>();
    EXPECT_EQ(floatLit, nullptr);
}

TEST(NodeTypeTest, DowncastStatement) {
    SourceLocation loc;
    
    NodePtr<StatementNode> stmt = std::make_shared<BreakStatement>(loc);
    
    EXPECT_TRUE(stmt->is<BreakStatement>());
    EXPECT_FALSE(stmt->is<ContinueStatement>());
}
