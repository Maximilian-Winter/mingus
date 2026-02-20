//================================================================================
// MINGUS v1 - AST Generator Implementation
//================================================================================

#include "mingus/parser/ASTGenerator.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <stdexcept>

namespace mingus {
namespace parser {

using namespace ast;

//================================================================================
// Utility Functions
//================================================================================

// Parse an integer literal token that may use decimal, hex (0x/0X),
// binary (0b/0B), or octal (0o/0O) notation.
// std::stoll with base 0 handles 0x and leading-zero octal (C-style),
// but not the 0b / 0o prefixes — those need explicit base selection.
static int64_t parseIntegerLiteral(const std::string& text) {
    if (text.size() >= 2 && text[0] == '0') {
        char p = text[1];
        if (p == 'b' || p == 'B') return std::stoll(text.substr(2), nullptr, 2);
        if (p == 'o' || p == 'O') return std::stoll(text.substr(2), nullptr, 8);
    }
    return std::stoll(text, nullptr, 0);   // decimal or 0x... hex
}

SourceLocation getSourceLocation(antlr4::ParserRuleContext* ctx) {
    if (!ctx) return SourceLocation();
    
    auto* token = ctx->getStart();
    if (!token) return SourceLocation();
    
    return SourceLocation(
        "<input>",
        token->getLine(),
        token->getCharPositionInLine()
    );
}

void ASTGenerator::reportError(antlr4::ParserRuleContext* ctx, const std::string& message) {
    errors.push_back({getSourceLocation(ctx), message});
}

//================================================================================
// Entry Point
//================================================================================
std::shared_ptr<ProgramNode> ASTGenerator::generate(MingusParser::ProgramContext* ctx) {
    errors.clear();
    if (!ctx) {
        reportError(nullptr, "Null program context");
        return nullptr;
    }
    
    try {
        // Use visitProgram directly instead of visit() to ensure we're calling the right method
        auto result = visitProgram(ctx);
        if (!result.has_value()) {
            reportError(ctx, "visitProgram returned empty result");
            return nullptr;
        }
        
        auto node = anyToNode<ProgramNode>(result);
        if (!node) {
            reportError(ctx, "anyToNode<ProgramNode> returned nullptr - type mismatch");
            return nullptr;
        }
        return node;
    } catch (const std::exception& e) {
        reportError(ctx, std::string("Exception during AST generation: ") + e.what());
        return nullptr;
    }
}

//================================================================================
// Program Structure
//================================================================================
std::any ASTGenerator::visitProgram(MingusParser::ProgramContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    NodeList<ModuleNode> modules;
    for (auto* moduleCtx : ctx->module()) {
        auto moduleNode = anyToNode<ModuleNode>(visitModule(moduleCtx));
        if (moduleNode) {
            modules.push_back(moduleNode);
        }
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<ProgramNode>(modules, loc)
    );
}

std::any ASTGenerator::visitModule(MingusParser::ModuleContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    std::string name = ctx->Identifier()->getText();
    
    NodeList<ImportNode> imports;
    NodeList<DeclarationNode> declarations;
    
    if (ctx->moduleBlock()) {
        for (auto* declCtx : ctx->moduleBlock()->moduleDeclaration()) {
            // Determine if it's an import or a declaration
            if (declCtx->importDefinition()) {
                auto importNode = anyToNode<ImportNode>(
                    visitImportDefinition(declCtx->importDefinition())
                );
                if (importNode) imports.push_back(importNode);
            } else if (declCtx->classDeclaration()) {
                auto node = anyToNode<ClassDeclaration>(
                    visitClassDeclaration(declCtx->classDeclaration())
                );
                if (node) declarations.push_back(node);
            } else if (declCtx->interfaceDeclaration()) {
                auto node = anyToNode<InterfaceDeclaration>(
                    visitInterfaceDeclaration(declCtx->interfaceDeclaration())
                );
                if (node) declarations.push_back(node);
            } else if (declCtx->structDeclaration()) {
                auto node = anyToNode<StructDeclaration>(
                    visitStructDeclaration(declCtx->structDeclaration())
                );
                if (node) declarations.push_back(node);
            } else if (declCtx->enumDeclaration()) {
                auto node = anyToNode<EnumDeclaration>(
                    visitEnumDeclaration(declCtx->enumDeclaration())
                );
                if (node) declarations.push_back(node);
            } else if (declCtx->functionDeclaration()) {
                auto node = anyToNode<FunctionDeclaration>(
                    visitFunctionDeclaration(declCtx->functionDeclaration())
                );
                if (node) declarations.push_back(node);
            } else if (declCtx->externDeclaration()) {
                // Handle extern declarations
                auto* externCtx = declCtx->externDeclaration();
                if (externCtx->externBody()) {
                    auto* externBody = externCtx->externBody();
                    // externBody can have multiple externFunctionDeclarations
                    for (auto* funcCtx : externBody->externFunctionDeclaration()) {
                        auto node = anyToNode<ExternFunctionDeclaration>(
                            visitExternFunctionDeclaration(funcCtx)
                        );
                        if (node) declarations.push_back(node);
                    }
                }
            } else if (declCtx->variableDeclaration()) {
                auto node = anyToNode<VariableDeclaration>(
                    visitVariableDeclaration(declCtx->variableDeclaration())
                );
                if (node) declarations.push_back(node);
            }
        }
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<ModuleNode>(name, imports, declarations, loc)
    );
}

std::any ASTGenerator::visitImportDefinition(MingusParser::ImportDefinitionContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    std::vector<ImportTarget> targets;
    for (auto* targetCtx : ctx->importTarget()) {
        std::string name = targetCtx->Identifier(0)->getText();
        std::optional<std::string> alias;
        if (targetCtx->AsKeyword()) {
            alias = targetCtx->Identifier(1)->getText();
        }
        targets.emplace_back(name, alias);
    }
    
    QualifiedName source;
    if (ctx->FromDirective() && ctx->qualifiedName()) {
        source = parseQualifiedName(ctx->qualifiedName());
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<ImportNode>(source, targets, loc)
    );
}

std::any ASTGenerator::visitImportTarget(MingusParser::ImportTargetContext* ctx) {
    // Handled inline in visitImportDefinition
    return visitChildren(ctx);
}

std::any ASTGenerator::visitModuleBlock(MingusParser::ModuleBlockContext* ctx) {
    return visitChildren(ctx);
}

//================================================================================
// Helper Methods - Modifiers and Names
//================================================================================
ast::QualifiedName ASTGenerator::parseQualifiedName(MingusParser::QualifiedNameContext* ctx) {
    if (!ctx) return QualifiedName();
    
    std::vector<std::string> parts;
    for (auto* id : ctx->Identifier()) {
        parts.push_back(id->getText());
    }
    return QualifiedName(parts);
}

ast::AccessModifier ASTGenerator::parseAccessModifier(MingusParser::AccessModifierContext* ctx) {
    if (!ctx) return AccessModifier::None;
    
    if (ctx->DeclarePublic()) return AccessModifier::Public;
    if (ctx->DeclarePrivate()) return AccessModifier::Private;
    if (ctx->DeclareProtected()) return AccessModifier::Protected;
    
    return AccessModifier::None;
}

ast::AssignOp ASTGenerator::parseAssignmentOperator(MingusParser::AssignmentOperatorContext* ctx) {
    if (!ctx) return AssignOp::Assign;
    
    if (ctx->AssignOperator()) return AssignOp::Assign;
    if (ctx->PlusAssignOperator()) return AssignOp::AddAssign;
    if (ctx->MinusAssignOperator()) return AssignOp::SubAssign;
    if (ctx->MultiplyAssignOperator()) return AssignOp::MulAssign;
    if (ctx->DivideAssignOperator()) return AssignOp::DivAssign;
    if (ctx->ModuloAssignOperator()) return AssignOp::ModAssign;
    if (ctx->BitwiseAndAssignOperator()) return AssignOp::AndAssign;
    if (ctx->BitwiseOrAssignOperator()) return AssignOp::OrAssign;
    if (ctx->BitwiseXorAssignOperator()) return AssignOp::XorAssign;
    if (ctx->BitwiseLeftShiftAssignOperator()) return AssignOp::ShiftLeftAssign;
    if (ctx->BitwiseRightShiftAssignOperator()) return AssignOp::ShiftRightAssign;
    
    return AssignOp::Assign;
}

ast::BinaryOp ASTGenerator::parseBinaryOperator(const std::string& op) {
    if (op == "+") return BinaryOp::Add;
    if (op == "-") return BinaryOp::Sub;
    if (op == "*") return BinaryOp::Mul;
    if (op == "/") return BinaryOp::Div;
    if (op == "%") return BinaryOp::Mod;
    if (op == "==") return BinaryOp::Equal;
    if (op == "!=") return BinaryOp::NotEqual;
    if (op == "<") return BinaryOp::Less;
    if (op == "<=") return BinaryOp::LessEqual;
    if (op == ">") return BinaryOp::Greater;
    if (op == ">=") return BinaryOp::GreaterEqual;
    if (op == "&&") return BinaryOp::LogicalAnd;
    if (op == "||") return BinaryOp::LogicalOr;
    if (op == "&") return BinaryOp::BitwiseAnd;
    if (op == "|") return BinaryOp::BitwiseOr;
    if (op == "^") return BinaryOp::BitwiseXor;
    if (op == "<<") return BinaryOp::ShiftLeft;
    if (op == ">>") return BinaryOp::ShiftRight;
    
    return BinaryOp::Add;
}

ast::UnaryOp ASTGenerator::parseUnaryOperator(const std::string& op) {
    if (op == "-") return UnaryOp::Negate;
    if (op == "!") return UnaryOp::LogicalNot;
    if (op == "~") return UnaryOp::BitwiseNot;
    if (op == "&") return UnaryOp::AddressOf;
    if (op == "*") return UnaryOp::Dereference;
    if (op == "++") return UnaryOp::PreIncrement;
    if (op == "--") return UnaryOp::PreDecrement;
    
    return UnaryOp::Negate;
}

ast::OperatorKind ASTGenerator::parseOperatorKind(MingusParser::OverloadableOperatorContext* ctx) {
    if (!ctx) return OperatorKind::Plus;
    
    if (ctx->PlusOperator()) return OperatorKind::Plus;
    if (ctx->MinusOperator()) return OperatorKind::Minus;
    if (ctx->StarOperator()) return OperatorKind::Star;
    if (ctx->DivideOperator()) return OperatorKind::Divide;
    if (ctx->ModuloOperator()) return OperatorKind::Modulo;
    if (ctx->EqualOperator()) return OperatorKind::Equal;
    if (ctx->UnequalOperator()) return OperatorKind::NotEqual;
    if (ctx->SmallerOperator()) return OperatorKind::Less;
    if (ctx->SmallerEqualOperator()) return OperatorKind::LessEqual;
    if (ctx->GreaterOperator()) return OperatorKind::Greater;
    if (ctx->GreaterEqualOperator()) return OperatorKind::GreaterEqual;
    if (ctx->SquareBracketLeft() && ctx->SquareBracketRight()) return OperatorKind::Index;
    
    return OperatorKind::Plus;
}


//================================================================================
// Type Parsing
//================================================================================
std::any ASTGenerator::visitTypeIdentifier(MingusParser::TypeIdentifierContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    NodePtr<TypeNode> baseType;
    
    if (ctx->primitiveType()) {
        baseType = anyToNode<TypeNode>(visitPrimitiveType(ctx->primitiveType()));
    } else if (ctx->qualifiedName()) {
        auto qname = parseQualifiedName(ctx->qualifiedName());
        baseType = std::make_shared<NamedTypeNode>(qname.parts, loc);
    } else if (ctx->tupleType()) {
        baseType = anyToNode<TypeNode>(visitTupleType(ctx->tupleType()));
    } else if (ctx->functionType()) {
        baseType = anyToNode<TypeNode>(visitFunctionType(ctx->functionType()));
    }
    
    // Apply type modifiers (arrays and pointers)
    for (auto* modifier : ctx->typeModifier()) {
        if (modifier->arrayDimension()) {
            auto* arrayCtx = modifier->arrayDimension();
            NodePtr<ExpressionNode> size = nullptr;
            if (arrayCtx->IntegerLiteral()) {
                size = std::make_shared<IntegerLiteral>(
                    parseIntegerLiteral(arrayCtx->IntegerLiteral()->getText()), loc
                );
            }
            baseType = std::make_shared<ArrayTypeNode>(baseType, size, loc);
        } else if (modifier->pointerLevel()) {
            baseType = std::make_shared<PointerTypeNode>(baseType, loc);
        } else if (modifier->referenceLevel()) {
            baseType = std::make_shared<PointerTypeNode>(baseType, loc, /*isRef=*/true);
        }
    }
    
    return std::static_pointer_cast<ASTNode>(baseType);
}

std::any ASTGenerator::visitPrimitiveType(MingusParser::PrimitiveTypeContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    PrimitiveType::PrimitiveKind kind;
    if (ctx->IntegerType()) kind = PrimitiveType::PrimitiveKind::Int;
    else if (ctx->DoubleType()) kind = PrimitiveType::PrimitiveKind::Double;
    else if (ctx->FloatType()) kind = PrimitiveType::PrimitiveKind::Float;
    else if (ctx->ByteType()) kind = PrimitiveType::PrimitiveKind::Byte;
    else if (ctx->StringType()) kind = PrimitiveType::PrimitiveKind::String;
    else if (ctx->CharType()) kind = PrimitiveType::PrimitiveKind::Char;
    else if (ctx->BoolType()) kind = PrimitiveType::PrimitiveKind::Bool;
    else if (ctx->VoidType()) kind = PrimitiveType::PrimitiveKind::Void;
    else kind = PrimitiveType::PrimitiveKind::Int;
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<PrimitiveTypeNode>(kind, loc)
    );
}

std::any ASTGenerator::visitTupleType(MingusParser::TupleTypeContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    NodeList<TypeNode> elementTypes;
    for (auto* typeId : ctx->typeIdentifier()) {
        auto type = anyToNode<TypeNode>(visitTypeIdentifier(typeId));
        if (type) elementTypes.push_back(type);
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<TupleTypeNode>(elementTypes, loc)
    );
}

std::any ASTGenerator::visitFunctionType(MingusParser::FunctionTypeContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    NodeList<TypeNode> paramTypes;
    if (ctx->typeList()) {
        for (auto* typeId : ctx->typeList()->typeIdentifier()) {
            auto type = anyToNode<TypeNode>(visitTypeIdentifier(typeId));
            if (type) paramTypes.push_back(type);
        }
    }
    
    auto returnType = anyToNode<TypeNode>(visitReturnType(ctx->returnType()));
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<FunctionTypeNode>(paramTypes, returnType, loc)
    );
}

std::any ASTGenerator::visitArrayDimension(MingusParser::ArrayDimensionContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTGenerator::visitPointerLevel(MingusParser::PointerLevelContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTGenerator::visitReturnType(MingusParser::ReturnTypeContext* ctx) {
    if (ctx->typeIdentifier()) {
        return visitTypeIdentifier(ctx->typeIdentifier());
    } else if (ctx->tupleType()) {
        return visitTupleType(ctx->tupleType());
    }
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<PrimitiveTypeNode>(PrimitiveType::PrimitiveKind::Void, getSourceLocation(ctx))
    );
}

//================================================================================
// Utility Visitors
//================================================================================
std::any ASTGenerator::visitQualifiedName(MingusParser::QualifiedNameContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTGenerator::visitAccessModifier(MingusParser::AccessModifierContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTGenerator::visitStaticModifier(MingusParser::StaticModifierContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTGenerator::visitAbstractModifier(MingusParser::AbstractModifierContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTGenerator::visitOverloadableOperator(MingusParser::OverloadableOperatorContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTGenerator::visitString(MingusParser::StringContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    std::vector<InterpolatedPart> parts;
    std::string currentText;
    
    for (auto* partCtx : ctx->stringPart()) {
        if (partCtx->TEXT()) {
            currentText += partCtx->TEXT()->getText();
        } else if (partCtx->ESCAPE_SEQUENCE()) {
            std::string esc = partCtx->ESCAPE_SEQUENCE()->getText();
            if (esc == "\\n") currentText += '\n';
            else if (esc == "\\t") currentText += '\t';
            else if (esc == "\\r") currentText += '\r';
            else if (esc.length() >= 2) currentText += esc[1];
        } else if (partCtx->BACKSLASH_PAREN()) {
            if (!currentText.empty()) {
                parts.push_back(InterpolatedPart::makeText(currentText));
                currentText.clear();
            }
            auto expr = anyToNode<ExpressionNode>(visitExpression(partCtx->expression()));
            if (expr) {
                parts.push_back(InterpolatedPart::makeExpression(expr));
            }
        }
    }
    
    if (!currentText.empty()) {
        parts.push_back(InterpolatedPart::makeText(currentText));
    }
    
    if (parts.size() == 1 && parts[0].kind == InterpolatedPartKind::Text) {
        return std::static_pointer_cast<ASTNode>(
            std::make_shared<StringLiteral>(parts[0].text, loc)
        );
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<InterpolatedString>(parts, loc)
    );
}


//================================================================================
// Declarations
//================================================================================

std::any ASTGenerator::visitClassDeclaration(MingusParser::ClassDeclarationContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    std::string name = ctx->Identifier()->getText();
    auto access = parseAccessModifier(ctx->accessModifier());
    bool isStatic = ctx->staticModifier() != nullptr;
    bool isAbstract = ctx->abstractModifier() != nullptr;
    
    std::vector<QualifiedName> baseClasses;
    if (ctx->inheritance()) {
        for (auto* qnameCtx : ctx->inheritance()->qualifiedName()) {
            baseClasses.push_back(parseQualifiedName(qnameCtx));
        }
    }
    
    NodePtr<ConstructorDeclaration> constructor;
    NodePtr<DestructorDeclaration> destructor;
    NodeList<OperatorDeclaration> operators;
    NodeList<FunctionDeclaration> methods;
    NodeList<VariableDeclaration> fields;
    
    if (ctx->classBlock()) {
        for (auto* memberCtx : ctx->classBlock()->classMember()) {
            if (memberCtx->constructorDeclaration()) {
                constructor = anyToNode<ConstructorDeclaration>(
                    visitConstructorDeclaration(memberCtx->constructorDeclaration())
                );
            } else if (memberCtx->destructorDeclaration()) {
                destructor = anyToNode<DestructorDeclaration>(
                    visitDestructorDeclaration(memberCtx->destructorDeclaration())
                );
            } else if (memberCtx->operatorDeclaration()) {
                auto op = anyToNode<OperatorDeclaration>(
                    visitOperatorDeclaration(memberCtx->operatorDeclaration())
                );
                if (op) operators.push_back(op);
            } else if (memberCtx->functionDeclaration()) {
                auto method = anyToNode<FunctionDeclaration>(
                    visitFunctionDeclaration(memberCtx->functionDeclaration())
                );
                if (method) methods.push_back(method);
            } else if (memberCtx->variableDeclaration()) {
                auto field = anyToNode<VariableDeclaration>(
                    visitVariableDeclaration(memberCtx->variableDeclaration())
                );
                if (field) fields.push_back(field);
            }
        }
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<ClassDeclaration>(
            name, access, isStatic, isAbstract,
            baseClasses, constructor, destructor,
            operators, methods, fields, loc
        )
    );
}

std::any ASTGenerator::visitInterfaceDeclaration(MingusParser::InterfaceDeclarationContext* ctx) {
    auto loc = getSourceLocation(ctx);

    std::string name = ctx->Identifier()->getText();
    auto access = parseAccessModifier(ctx->accessModifier());

    NodeList<FunctionDeclaration> methods;
    if (ctx->interfaceBlock()) {
        for (auto* memberCtx : ctx->interfaceBlock()->interfaceMember()) {
            if (memberCtx->functionDeclaration()) {
                auto method = anyToNode<FunctionDeclaration>(
                    visitFunctionDeclaration(memberCtx->functionDeclaration())
                );
                if (method) methods.push_back(method);
            }
        }
    }

    return std::static_pointer_cast<ASTNode>(
        std::make_shared<InterfaceDeclaration>(name, access, std::move(methods), loc)
    );
}

std::any ASTGenerator::visitStructDeclaration(MingusParser::StructDeclarationContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    std::string name = ctx->Identifier()->getText();
    auto access = parseAccessModifier(ctx->accessModifier());
    
    NodeList<OperatorDeclaration> operators;
    NodeList<FunctionDeclaration> methods;
    NodeList<VariableDeclaration> fields;
    
    if (ctx->structBlock()) {
        for (auto* memberCtx : ctx->structBlock()->structMember()) {
            if (memberCtx->operatorDeclaration()) {
                auto op = anyToNode<OperatorDeclaration>(
                    visitOperatorDeclaration(memberCtx->operatorDeclaration())
                );
                if (op) operators.push_back(op);
            } else if (memberCtx->functionDeclaration()) {
                auto method = anyToNode<FunctionDeclaration>(
                    visitFunctionDeclaration(memberCtx->functionDeclaration())
                );
                if (method) methods.push_back(method);
            } else if (memberCtx->variableDeclaration()) {
                auto field = anyToNode<VariableDeclaration>(
                    visitVariableDeclaration(memberCtx->variableDeclaration())
                );
                if (field) fields.push_back(field);
            }
        }
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<StructDeclaration>(
            name, access, operators, methods, fields, loc
        )
    );
}

std::any ASTGenerator::visitEnumDeclaration(MingusParser::EnumDeclarationContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    std::string name = ctx->Identifier()->getText();
    auto access = parseAccessModifier(ctx->accessModifier());
    
    NodePtr<TypeNode> underlyingType;
    if (ctx->typeIdentifier()) {
        underlyingType = anyToNode<TypeNode>(visitTypeIdentifier(ctx->typeIdentifier()));
    }
    
    NodeList<EnumMemberNode> members;
    for (auto* memberCtx : ctx->enumMember()) {
        auto member = anyToNode<EnumMemberNode>(visitEnumMember(memberCtx));
        if (member) members.push_back(member);
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<EnumDeclaration>(
            name, access, underlyingType, members, loc
        )
    );
}

std::any ASTGenerator::visitEnumMember(MingusParser::EnumMemberContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    std::string name = ctx->Identifier()->getText();
    NodePtr<ExpressionNode> value;
    
    if (ctx->AssignOperator() && ctx->expression()) {
        value = anyToNode<ExpressionNode>(visitExpression(ctx->expression()));
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<EnumMemberNode>(name, value, loc)
    );
}

std::any ASTGenerator::visitFunctionDeclaration(MingusParser::FunctionDeclarationContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    std::string name = ctx->Identifier()->getText();
    auto access = parseAccessModifier(ctx->accessModifier());
    bool isStatic = ctx->staticModifier() != nullptr;
    bool isAbstract = ctx->abstractModifier() != nullptr;
    
    NodeList<ParameterNode> parameters;
    if (ctx->definitionParameters() && ctx->definitionParameters()->parameterList()) {
        for (auto* paramCtx : ctx->definitionParameters()->parameterList()->parameter()) {
            auto param = anyToNode<ParameterNode>(visitParameter(paramCtx));
            if (param) parameters.push_back(param);
        }
    }
    
    auto returnType = anyToNode<TypeNode>(visitReturnType(ctx->returnType()));
    
    // Handle block body, expression body, or abstract/no body
    NodePtr<BlockStatement> body;
    if (ctx->block()) {
        body = anyToNode<BlockStatement>(visitBlock(ctx->block()));
    } else if (ctx->expression()) {
        // Expression-bodied function: wrap in a block with return statement
        auto expr = anyToNode<ExpressionNode>(visitExpression(ctx->expression()));
        if (expr) {
            auto returnStmt = std::make_shared<ReturnStatement>(expr, expr->location);
            NodeList<StatementNode> stmts;
            stmts.push_back(returnStmt);
            body = std::make_shared<BlockStatement>(stmts, expr->location);
        }
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<FunctionDeclaration>(
            name, access, isStatic, isAbstract,
            parameters, returnType, body, loc
        )
    );
}

std::any ASTGenerator::visitConstructorDeclaration(MingusParser::ConstructorDeclarationContext* ctx) {
    auto loc = getSourceLocation(ctx);

    auto access = parseAccessModifier(ctx->accessModifier());

    NodeList<ParameterNode> parameters;
    if (ctx->definitionParameters() && ctx->definitionParameters()->parameterList()) {
        for (auto* paramCtx : ctx->definitionParameters()->parameterList()->parameter()) {
            auto param = anyToNode<ParameterNode>(visitParameter(paramCtx));
            if (param) parameters.push_back(param);
        }
    }

    // Parse optional super() call: constructor(params) : super(args) { body }
    NodeList<ExpressionNode> superArgs;
    bool hasSuperCall = false;
    if (ctx->SuperKeyword()) {
        hasSuperCall = true;
        if (ctx->callArguments() && ctx->callArguments()->argumentList()) {
            for (auto* argCtx : ctx->callArguments()->argumentList()->expression()) {
                auto arg = anyToNode<ExpressionNode>(visitExpression(argCtx));
                if (arg) superArgs.push_back(arg);
            }
        }
    }

    auto body = anyToNode<BlockStatement>(visitBlock(ctx->block()));

    return std::static_pointer_cast<ASTNode>(
        std::make_shared<ConstructorDeclaration>(
            access, parameters, body, superArgs, hasSuperCall, loc)
    );
}

std::any ASTGenerator::visitDestructorDeclaration(MingusParser::DestructorDeclarationContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    auto body = anyToNode<BlockStatement>(visitBlock(ctx->block()));
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<DestructorDeclaration>(body, loc)
    );
}

std::any ASTGenerator::visitOperatorDeclaration(MingusParser::OperatorDeclarationContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    auto opKind = parseOperatorKind(ctx->overloadableOperator());
    
    NodeList<ParameterNode> parameters;
    if (ctx->definitionParameters() && ctx->definitionParameters()->parameterList()) {
        for (auto* paramCtx : ctx->definitionParameters()->parameterList()->parameter()) {
            auto param = anyToNode<ParameterNode>(visitParameter(paramCtx));
            if (param) parameters.push_back(param);
        }
    }
    
    auto returnType = anyToNode<TypeNode>(visitReturnType(ctx->returnType()));
    
    // Handle both block body and expression body
    NodePtr<BlockStatement> body;
    if (ctx->block()) {
        body = anyToNode<BlockStatement>(visitBlock(ctx->block()));
    } else if (ctx->expression()) {
        // Expression-bodied operator: wrap in a block with return statement
        auto expr = anyToNode<ExpressionNode>(visitExpression(ctx->expression()));
        if (expr) {
            auto returnStmt = std::make_shared<ReturnStatement>(expr, expr->location);
            NodeList<StatementNode> stmts;
            stmts.push_back(returnStmt);
            body = std::make_shared<BlockStatement>(stmts, expr->location);
        }
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<OperatorDeclaration>(opKind, parameters, returnType, body, loc)
    );
}

std::any ASTGenerator::visitExternFunctionDeclaration(MingusParser::ExternFunctionDeclarationContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    std::string name = ctx->Identifier()->getText();
    
    NodeList<ParameterNode> parameters;
    if (ctx->definitionParameters() && ctx->definitionParameters()->parameterList()) {
        for (auto* paramCtx : ctx->definitionParameters()->parameterList()->parameter()) {
            auto param = anyToNode<ParameterNode>(visitParameter(paramCtx));
            if (param) parameters.push_back(param);
        }
    }
    
    auto returnType = anyToNode<TypeNode>(visitReturnType(ctx->returnType()));
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<ExternFunctionDeclaration>(name, parameters, returnType, loc)
    );
}

std::any ASTGenerator::visitParameter(MingusParser::ParameterContext* ctx) {
    auto loc = getSourceLocation(ctx);

    // Check for reference modifier (&) in the type modifiers
    bool isRef = false;
    for (auto* modifier : ctx->typeIdentifier()->typeModifier()) {
        if (modifier->referenceLevel()) {
            isRef = true;
            break;
        }
    }

    auto type = anyToNode<TypeNode>(visitTypeIdentifier(ctx->typeIdentifier()));
    std::string name = ctx->Identifier()->getText();

    NodePtr<ExpressionNode> defaultValue;
    if (ctx->AssignOperator() && ctx->expression()) {
        defaultValue = anyToNode<ExpressionNode>(visitExpression(ctx->expression()));
    }

    return std::static_pointer_cast<ASTNode>(
        std::make_shared<ParameterNode>(name, type, defaultValue, isRef, loc)
    );
}

std::any ASTGenerator::visitVariableDeclaration(MingusParser::VariableDeclarationContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    if (ctx->typedVariableDeclaration()) {
        return visitTypedVariableDeclaration(ctx->typedVariableDeclaration());
    } else if (ctx->inferredVariableDeclaration()) {
        return visitInferredVariableDeclaration(ctx->inferredVariableDeclaration());
    } else if (ctx->tupleDestructuring()) {
        return visitTupleDestructuring(ctx->tupleDestructuring());
    }
    
    return std::shared_ptr<ASTNode>(nullptr);
}

std::any ASTGenerator::visitTypedVariableDeclaration(MingusParser::TypedVariableDeclarationContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    auto type = anyToNode<TypeNode>(visitTypeIdentifier(ctx->typeIdentifier()));
    std::string name = ctx->Identifier()->getText();
    
    auto* parent = dynamic_cast<MingusParser::VariableDeclarationContext*>(ctx->parent);
    auto access = parseAccessModifier(parent ? parent->accessModifier() : nullptr);
    bool isStatic = parent && parent->staticModifier() != nullptr;
    
    NodePtr<ExpressionNode> initializer;
    if (ctx->AssignOperator() && ctx->exprStatement()) {
        auto exprStmt = anyToNode<ExpressionStatement>(visitExprStatement(ctx->exprStatement()));
        if (exprStmt) initializer = exprStmt->expression;
    } else if (ctx->callArguments()) {
        NodeList<ExpressionNode> args;
        if (ctx->callArguments()->argumentList()) {
            for (auto* argCtx : ctx->callArguments()->argumentList()->expression()) {
                auto arg = anyToNode<ExpressionNode>(visitExpression(argCtx));
                if (arg) args.push_back(arg);
            }
        }
        auto typeExpr = std::make_shared<IdentifierExpression>(type->as<NamedTypeNode>() ? 
            type->as<NamedTypeNode>()->getName() : "", loc);
        initializer = std::make_shared<CallExpression>(typeExpr, args, loc);
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<VariableDeclaration>(
            name, access, isStatic, type, false, initializer, loc
        )
    );
}

std::any ASTGenerator::visitInferredVariableDeclaration(MingusParser::InferredVariableDeclarationContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    std::string name = ctx->Identifier()->getText();
    
    auto* parent = dynamic_cast<MingusParser::VariableDeclarationContext*>(ctx->parent);
    auto access = parseAccessModifier(parent ? parent->accessModifier() : nullptr);
    bool isStatic = parent && parent->staticModifier() != nullptr;
    
    NodePtr<ExpressionNode> initializer;
    if (ctx->exprStatement()) {
        auto exprStmt = anyToNode<ExpressionStatement>(visitExprStatement(ctx->exprStatement()));
        if (exprStmt) initializer = exprStmt->expression;
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<VariableDeclaration>(
            name, access, isStatic, nullptr, true, initializer, loc
        )
    );
}

std::any ASTGenerator::visitTupleDestructuring(MingusParser::TupleDestructuringContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    std::vector<DestructureElement> elements;
    for (auto* elemCtx : ctx->tupleDestructureElement()) {
        std::string name = elemCtx->Identifier()->getText();
        
        if (elemCtx->DeclareVariable()) {
            elements.emplace_back(name, nullptr, true);
        } else {
            auto type = anyToNode<TypeNode>(visitTypeIdentifier(elemCtx->typeIdentifier()));
            elements.emplace_back(name, type, false);
        }
    }
    
    NodePtr<ExpressionNode> initializer;
    if (ctx->exprStatement()) {
        auto exprStmt = anyToNode<ExpressionStatement>(visitExprStatement(ctx->exprStatement()));
        if (exprStmt) initializer = exprStmt->expression;
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<TupleDestructuringDeclaration>(elements, initializer, loc)
    );
}

std::any ASTGenerator::visitTupleDestructureElement(MingusParser::TupleDestructureElementContext* ctx) {
    return visitChildren(ctx);
}


//================================================================================
// Statements
//================================================================================

std::any ASTGenerator::visitBlock(MingusParser::BlockContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    NodeList<StatementNode> statements;
    for (auto* stmtCtx : ctx->statement()) {
        auto stmt = anyToNode<StatementNode>(visitStatement(stmtCtx));
        if (stmt) statements.push_back(stmt);
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<BlockStatement>(statements, loc)
    );
}

std::any ASTGenerator::visitStatement(MingusParser::StatementContext* ctx) {
    if (ctx->exprStatement()) {
        return visitExprStatement(ctx->exprStatement());
    } else if (ctx->variableDeclaration()) {
        auto result = visitVariableDeclaration(ctx->variableDeclaration());
        // Handle both VariableDeclaration and TupleDestructuringDeclaration
        if (auto varDecl = anyToNode<VariableDeclaration>(result)) {
            return std::static_pointer_cast<ASTNode>(
                std::make_shared<ExpressionStatement>(
                    std::static_pointer_cast<ExpressionNode>(std::static_pointer_cast<ASTNode>(varDecl)),
                    getSourceLocation(ctx)
                )
            );
        } else if (auto tupleDecl = anyToNode<TupleDestructuringDeclaration>(result)) {
            // Tuple destructuring is a declaration statement
            return std::static_pointer_cast<ASTNode>(tupleDecl);
        }
        return std::shared_ptr<ASTNode>(nullptr);
    } else if (ctx->forStatement()) {
        return visitForStatement(ctx->forStatement());
    } else if (ctx->whileStatement()) {
        return visitWhileStatement(ctx->whileStatement());
    } else if (ctx->ifStatement()) {
        return visitIfStatement(ctx->ifStatement());
    } else if (ctx->switchStatement()) {
        return visitSwitchStatement(ctx->switchStatement());
    } else if (ctx->matchStatement()) {
        auto matchExpr = anyToNode<MatchExpression>(visitMatchExpression(ctx->matchStatement()->matchExpression()));
        return std::static_pointer_cast<ASTNode>(
            std::make_shared<ExpressionStatement>(
                std::static_pointer_cast<ExpressionNode>(std::static_pointer_cast<ASTNode>(matchExpr)),
                getSourceLocation(ctx)
            )
        );
    } else if (ctx->returnStatement()) {
        return visitReturnStatement(ctx->returnStatement());
    } else if (ctx->breakStatement()) {
        return visitBreakStatement(ctx->breakStatement());
    } else if (ctx->continueStatement()) {
        return visitContinueStatement(ctx->continueStatement());
    } else if (ctx->deleteStatement()) {
        return visitDeleteStatement(ctx->deleteStatement());
    } else if (ctx->rawBlock()) {
        return visitRawBlock(ctx->rawBlock());
    } else if (ctx->block()) {
        return visitBlock(ctx->block());
    }
    
    return std::shared_ptr<ASTNode>(nullptr);
}

std::any ASTGenerator::visitExprStatement(MingusParser::ExprStatementContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    auto expr = anyToNode<ExpressionNode>(visitExpression(ctx->expression()));
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<ExpressionStatement>(expr, loc)
    );
}

std::any ASTGenerator::visitReturnStatement(MingusParser::ReturnStatementContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    NodePtr<ExpressionNode> value;
    if (ctx->expression()) {
        value = anyToNode<ExpressionNode>(visitExpression(ctx->expression()));
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<ReturnStatement>(value, loc)
    );
}

std::any ASTGenerator::visitIfStatement(MingusParser::IfStatementContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    auto condition = anyToNode<ExpressionNode>(visitExpression(ctx->expression()));
    auto thenBody = anyToNode<StatementNode>(visitStatement(ctx->trueBody));
    
    std::vector<ElseIfClause> elseIfClauses;
    for (auto* elseIfCtx : ctx->elseIfClause()) {
        auto elseIfCond = anyToNode<ExpressionNode>(visitExpression(elseIfCtx->expression()));
        auto elseIfBody = anyToNode<StatementNode>(visitStatement(elseIfCtx->statement()));
        elseIfClauses.emplace_back(elseIfCond, elseIfBody);
    }
    
    NodePtr<StatementNode> elseBody;
    if (ctx->elseClause()) {
        elseBody = anyToNode<StatementNode>(visitElseClause(ctx->elseClause()));
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<IfStatement>(condition, thenBody, elseIfClauses, elseBody, loc)
    );
}

std::any ASTGenerator::visitElseIfClause(MingusParser::ElseIfClauseContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTGenerator::visitElseClause(MingusParser::ElseClauseContext* ctx) {
    return visitStatement(ctx->statement());
}

std::any ASTGenerator::visitSwitchStatement(MingusParser::SwitchStatementContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    auto subject = anyToNode<ExpressionNode>(visitExpression(ctx->expression()));
    
    std::vector<SwitchCase> cases;
    for (auto* caseCtx : ctx->switchCase()) {
        auto value = anyToNode<ExpressionNode>(visitExpression(caseCtx->expression()));
        
        NodeList<StatementNode> caseBody;
        for (auto* stmtCtx : caseCtx->statement()) {
            auto stmt = anyToNode<StatementNode>(visitStatement(stmtCtx));
            if (stmt) caseBody.push_back(stmt);
        }
        
        cases.emplace_back(value, caseBody);
    }
    
    NodeList<StatementNode> defaultCase;
    if (ctx->switchDefault()) {
        for (auto* stmtCtx : ctx->switchDefault()->statement()) {
            auto stmt = anyToNode<StatementNode>(visitStatement(stmtCtx));
            if (stmt) defaultCase.push_back(stmt);
        }
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<SwitchStatement>(subject, cases, defaultCase, loc)
    );
}

std::any ASTGenerator::visitSwitchCase(MingusParser::SwitchCaseContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTGenerator::visitSwitchDefault(MingusParser::SwitchDefaultContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTGenerator::visitForStatement(MingusParser::ForStatementContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    NodePtr<DeclarationNode> initDeclaration;
    NodeList<ExpressionNode> initExpressions;
    
    if (ctx->forInitializer()) {
        auto* initCtx = ctx->forInitializer();
        if (initCtx->localVarInitializer()) {
            auto* localInit = initCtx->localVarInitializer();
            for (auto* localVarCtx : localInit->localVarDeclaration()) {
                std::string name = localVarCtx->Identifier()->getText();
                NodePtr<TypeNode> type;
                NodePtr<ExpressionNode> initExpr;
                bool isInferred = false;
                
                if (localVarCtx->DeclareVariable()) {
                    isInferred = true;
                    if (localVarCtx->expression()) {
                        initExpr = anyToNode<ExpressionNode>(visitExpression(localVarCtx->expression()));
                    }
                } else {
                    type = anyToNode<TypeNode>(visitTypeIdentifier(localVarCtx->typeIdentifier()));
                    if (localVarCtx->expression()) {
                        initExpr = anyToNode<ExpressionNode>(visitExpression(localVarCtx->expression()));
                    }
                }
                
                auto varDecl = std::make_shared<VariableDeclaration>(
                    name, AccessModifier::None, false, type, isInferred, initExpr, 
                    getSourceLocation(localVarCtx)
                );
                initDeclaration = varDecl;
                break;
            }
        } else if (!initCtx->expression().empty()) {
            for (auto* exprCtx : initCtx->expression()) {
                auto expr = anyToNode<ExpressionNode>(visitExpression(exprCtx));
                if (expr) initExpressions.push_back(expr);
            }
        }
    }
    
    NodePtr<ExpressionNode> condition;
    if (ctx->expression()) {
        condition = anyToNode<ExpressionNode>(visitExpression(ctx->expression()));
    }
    
    NodeList<ExpressionNode> iterators;
    if (ctx->forIterator()) {
        for (auto* exprCtx : ctx->forIterator()->expression()) {
            auto expr = anyToNode<ExpressionNode>(visitExpression(exprCtx));
            if (expr) iterators.push_back(expr);
        }
    }
    
    auto body = anyToNode<StatementNode>(visitStatement(ctx->statement()));
    
    if (initDeclaration) {
        return std::static_pointer_cast<ASTNode>(
            std::make_shared<ForStatement>(initDeclaration, condition, iterators, body, loc)
        );
    } else {
        return std::static_pointer_cast<ASTNode>(
            std::make_shared<ForStatement>(initExpressions, condition, iterators, body, loc)
        );
    }
}

std::any ASTGenerator::visitWhileStatement(MingusParser::WhileStatementContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    auto condition = anyToNode<ExpressionNode>(visitExpression(ctx->expression()));
    
    NodePtr<StatementNode> body;
    if (ctx->block()) {
        body = anyToNode<StatementNode>(visitBlock(ctx->block()));
    } else if (ctx->statement()) {
        body = anyToNode<StatementNode>(visitStatement(ctx->statement()));
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<WhileStatement>(condition, body, loc)
    );
}

std::any ASTGenerator::visitBreakStatement(MingusParser::BreakStatementContext* ctx) {
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<BreakStatement>(getSourceLocation(ctx))
    );
}

std::any ASTGenerator::visitContinueStatement(MingusParser::ContinueStatementContext* ctx) {
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<ContinueStatement>(getSourceLocation(ctx))
    );
}

std::any ASTGenerator::visitDeleteStatement(MingusParser::DeleteStatementContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    auto target = anyToNode<ExpressionNode>(visitExpression(ctx->expression()));
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<DeleteStatement>(target, loc)
    );
}

std::any ASTGenerator::visitRawBlock(MingusParser::RawBlockContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    auto body = anyToNode<BlockStatement>(visitBlock(ctx->block()));
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<RawBlock>(body, loc)
    );
}


//================================================================================
// Expressions
//================================================================================

std::any ASTGenerator::visitExpression(MingusParser::ExpressionContext* ctx) {
    if (ctx->assignment()) {
        return visitAssignment(ctx->assignment());
    } else if (ctx->lambdaExpression()) {
        return visitLambdaExpression(ctx->lambdaExpression());
    }
    return std::shared_ptr<ASTNode>(nullptr);
}

std::any ASTGenerator::visitAssignment(MingusParser::AssignmentContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    if (ctx->unaryExpression() && ctx->assignmentOperator()) {
        auto target = anyToNode<ExpressionNode>(visitUnaryExpression(ctx->unaryExpression()));

        // RHS can be either a recursive assignment or a lambda expression
        NodePtr<ExpressionNode> value;
        if (ctx->lambdaExpression()) {
            value = anyToNode<ExpressionNode>(visitLambdaExpression(ctx->lambdaExpression()));
        } else if (ctx->assignment()) {
            value = anyToNode<ExpressionNode>(visitAssignment(ctx->assignment()));
        }

        // Determine the assignment operator
        AssignOp op = parseAssignmentOperator(ctx->assignmentOperator());

        return std::static_pointer_cast<ASTNode>(
            std::make_shared<AssignmentExpression>(target, op, value, loc)
        );
    } else if (ctx->pipe()) {
        return visitPipe(ctx->pipe());
    }
    
    return std::shared_ptr<ASTNode>(nullptr);
}

std::any ASTGenerator::visitLambdaExpression(MingusParser::LambdaExpressionContext* ctx) {
    auto loc = getSourceLocation(ctx);

    // ── Parse capture list ──────────────────────────────────────────
    CaptureDefault capDefault = CaptureDefault::None;
    std::vector<CaptureItem> capItems;

    if (ctx->captureList()) {
        auto* capCtx = ctx->captureList();

        // Check for default capture mode: [=] or [&]
        if (capCtx->captureDefault()) {
            auto* defCtx = capCtx->captureDefault();
            if (defCtx->AssignOperator()) {
                capDefault = CaptureDefault::AllByValue;
            } else if (defCtx->SingleAndOperator()) {
                capDefault = CaptureDefault::AllByReference;
            }
        }

        // Check for explicit capture items: [x, &y, z]
        for (auto* itemCtx : capCtx->captureItem()) {
            std::string name = itemCtx->Identifier()->getText();
            CaptureMode mode = itemCtx->SingleAndOperator()
                ? CaptureMode::ByReference
                : CaptureMode::ByValue;
            capItems.emplace_back(name, mode);
        }
    }

    // ── Parse parameters ────────────────────────────────────────────
    NodeList<ParameterNode> parameters;
    if (ctx->lambdaParameterList()) {
        for (auto* paramCtx : ctx->lambdaParameterList()->lambdaParameter()) {
            std::string name = paramCtx->Identifier()->getText();
            NodePtr<TypeNode> type;

            if (paramCtx->typeIdentifier()) {
                type = anyToNode<TypeNode>(visitTypeIdentifier(paramCtx->typeIdentifier()));
            }

            parameters.push_back(std::make_shared<ParameterNode>(name, type, nullptr, false, getSourceLocation(paramCtx)));
        }
    }

    // ── Parse body ──────────────────────────────────────────────────
    NodePtr<ASTNode> body;
    if (ctx->block()) {
        body = anyToNode<BlockStatement>(visitBlock(ctx->block()));
    } else if (ctx->expression()) {
        body = anyToNode<ExpressionNode>(visitExpression(ctx->expression()));
    }

    // ── Build LambdaExpression node ─────────────────────────────────
    auto lambda = std::make_shared<LambdaExpression>(parameters, body, loc);
    lambda->captureDefault = capDefault;
    lambda->captureItems = std::move(capItems);

    return std::static_pointer_cast<ASTNode>(lambda);
}

std::any ASTGenerator::visitPipe(MingusParser::PipeContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    auto left = anyToNode<ExpressionNode>(visitTernary(ctx->ternary()));
    
    if (ctx->pipeTarget().empty()) {
        return std::static_pointer_cast<ASTNode>(left);
    }
    
    std::vector<PipeStage> stages;
    for (auto* targetCtx : ctx->pipeTarget()) {
        // Create QualifiedNameExpression from the qualified name
        auto qname = parseQualifiedName(targetCtx->qualifiedName());
        auto func = std::make_shared<QualifiedNameExpression>(qname, getSourceLocation(targetCtx->qualifiedName()));
        
        NodeList<ExpressionNode> extraArgs;
        if (targetCtx->callArguments() && targetCtx->callArguments()->argumentList()) {
            for (auto* argCtx : targetCtx->callArguments()->argumentList()->expression()) {
                auto arg = anyToNode<ExpressionNode>(visitExpression(argCtx));
                if (arg) extraArgs.push_back(arg);
            }
        }
        
        stages.emplace_back(func, extraArgs);
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<PipeExpression>(left, stages, loc)
    );
}

std::any ASTGenerator::visitTernary(MingusParser::TernaryContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    auto condition = anyToNode<ExpressionNode>(visitLogicOr(ctx->logicOr()));
    
    if (ctx->QuestionMarkOperator()) {
        auto thenExpr = anyToNode<ExpressionNode>(visitExpression(ctx->expression(0)));
        auto elseExpr = anyToNode<ExpressionNode>(visitExpression(ctx->expression(1)));
        
        return std::static_pointer_cast<ASTNode>(
            std::make_shared<TernaryExpression>(condition, thenExpr, elseExpr, loc)
        );
    }
    
    return std::static_pointer_cast<ASTNode>(condition);
}

// Helper for building left-associative binary expressions from flattened lists
template<typename Context, typename ChildContext>
std::shared_ptr<ExpressionNode> buildLeftAssociativeBinary(
    Context* ctx,
    const std::vector<ChildContext*>& children,
    const std::vector<antlr4::tree::TerminalNode*>& operators,
    std::function<std::any(ASTGenerator*, ChildContext*)> visitChild,
    std::function<BinaryOp(const std::string&)> getOp,
    ASTGenerator* generator
) {
    if (children.empty()) return nullptr;
    if (children.size() == 1) {
        return generator->anyToNode<ExpressionNode>(visitChild(generator, children[0]));
    }
    
    auto loc = getSourceLocation(ctx);
    auto left = generator->anyToNode<ExpressionNode>(visitChild(generator, children[0]));
    
    for (size_t i = 1; i < children.size(); ++i) {
        auto right = generator->anyToNode<ExpressionNode>(visitChild(generator, children[i]));
        std::string opText = operators[i-1] ? operators[i-1]->getText() : "";
        BinaryOp op = getOp(opText);
        left = std::make_shared<BinaryExpression>(left, op, right, loc);
    }
    
    return left;
}

std::any ASTGenerator::visitLogicOr(MingusParser::LogicOrContext* ctx) {
    auto loc = getSourceLocation(ctx);
    auto children = ctx->logicAnd();
    auto operators = ctx->LogicalOrOperator();
    
    if (children.size() == 1) {
        return visitLogicAnd(children[0]);
    }
    
    auto left = anyToNode<ExpressionNode>(visitLogicAnd(children[0]));
    for (size_t i = 1; i < children.size(); ++i) {
        auto right = anyToNode<ExpressionNode>(visitLogicAnd(children[i]));
        left = std::make_shared<BinaryExpression>(left, BinaryOp::LogicalOr, right, loc);
    }
    return std::static_pointer_cast<ASTNode>(left);
}

std::any ASTGenerator::visitLogicAnd(MingusParser::LogicAndContext* ctx) {
    auto loc = getSourceLocation(ctx);
    auto children = ctx->bitwiseOr();
    
    if (children.size() == 1) {
        return visitBitwiseOr(children[0]);
    }
    
    auto left = anyToNode<ExpressionNode>(visitBitwiseOr(children[0]));
    for (size_t i = 1; i < children.size(); ++i) {
        auto right = anyToNode<ExpressionNode>(visitBitwiseOr(children[i]));
        left = std::make_shared<BinaryExpression>(left, BinaryOp::LogicalAnd, right, loc);
    }
    return std::static_pointer_cast<ASTNode>(left);
}

std::any ASTGenerator::visitBitwiseOr(MingusParser::BitwiseOrContext* ctx) {
    auto loc = getSourceLocation(ctx);
    auto children = ctx->bitwiseXor();
    
    if (children.size() == 1) {
        return visitBitwiseXor(children[0]);
    }
    
    auto left = anyToNode<ExpressionNode>(visitBitwiseXor(children[0]));
    for (size_t i = 1; i < children.size(); ++i) {
        auto right = anyToNode<ExpressionNode>(visitBitwiseXor(children[i]));
        left = std::make_shared<BinaryExpression>(left, BinaryOp::BitwiseOr, right, loc);
    }
    return std::static_pointer_cast<ASTNode>(left);
}

std::any ASTGenerator::visitBitwiseXor(MingusParser::BitwiseXorContext* ctx) {
    auto loc = getSourceLocation(ctx);
    auto children = ctx->bitwiseAnd();
    
    if (children.size() == 1) {
        return visitBitwiseAnd(children[0]);
    }
    
    auto left = anyToNode<ExpressionNode>(visitBitwiseAnd(children[0]));
    for (size_t i = 1; i < children.size(); ++i) {
        auto right = anyToNode<ExpressionNode>(visitBitwiseAnd(children[i]));
        left = std::make_shared<BinaryExpression>(left, BinaryOp::BitwiseXor, right, loc);
    }
    return std::static_pointer_cast<ASTNode>(left);
}

std::any ASTGenerator::visitBitwiseAnd(MingusParser::BitwiseAndContext* ctx) {
    auto loc = getSourceLocation(ctx);
    auto children = ctx->equality();
    
    if (children.size() == 1) {
        return visitEquality(children[0]);
    }
    
    auto left = anyToNode<ExpressionNode>(visitEquality(children[0]));
    for (size_t i = 1; i < children.size(); ++i) {
        auto right = anyToNode<ExpressionNode>(visitEquality(children[i]));
        left = std::make_shared<BinaryExpression>(left, BinaryOp::BitwiseAnd, right, loc);
    }
    return std::static_pointer_cast<ASTNode>(left);
}

std::any ASTGenerator::visitEquality(MingusParser::EqualityContext* ctx) {
    auto loc = getSourceLocation(ctx);
    auto children = ctx->relational();
    
    if (children.size() == 1) {
        return visitRelational(children[0]);
    }
    
    auto left = anyToNode<ExpressionNode>(visitRelational(children[0]));

    for (size_t i = 1; i < children.size(); ++i) {
        auto right = anyToNode<ExpressionNode>(visitRelational(children[i]));
        // Use positional index into raw children to get the correct operator
        auto* opNode = dynamic_cast<antlr4::tree::TerminalNode*>(ctx->children[2 * (i - 1) + 1]);
        BinaryOp op = (opNode && opNode->getText() == "==") ? BinaryOp::Equal : BinaryOp::NotEqual;
        left = std::make_shared<BinaryExpression>(left, op, right, loc);
    }
    return std::static_pointer_cast<ASTNode>(left);
}

std::any ASTGenerator::visitRelational(MingusParser::RelationalContext* ctx) {
    auto loc = getSourceLocation(ctx);
    auto children = ctx->shift();
    
    if (children.size() == 1) {
        return visitShift(children[0]);
    }
    
    auto left = anyToNode<ExpressionNode>(visitShift(children[0]));

    for (size_t i = 1; i < children.size(); ++i) {
        auto right = anyToNode<ExpressionNode>(visitShift(children[i]));
        // Use positional index into raw children to get the correct operator
        auto* opNode = dynamic_cast<antlr4::tree::TerminalNode*>(ctx->children[2 * (i - 1) + 1]);
        BinaryOp op = BinaryOp::Less;
        if (opNode) {
            std::string opText = opNode->getText();
            if (opText == "<=") op = BinaryOp::LessEqual;
            else if (opText == ">") op = BinaryOp::Greater;
            else if (opText == ">=") op = BinaryOp::GreaterEqual;
        }
        left = std::make_shared<BinaryExpression>(left, op, right, loc);
    }
    return std::static_pointer_cast<ASTNode>(left);
}

std::any ASTGenerator::visitShift(MingusParser::ShiftContext* ctx) {
    auto loc = getSourceLocation(ctx);
    auto children = ctx->additive();
    
    if (children.size() == 1) {
        return visitAdditive(children[0]);
    }
    
    auto left = anyToNode<ExpressionNode>(visitAdditive(children[0]));

    for (size_t i = 1; i < children.size(); ++i) {
        auto right = anyToNode<ExpressionNode>(visitAdditive(children[i]));
        // Use positional index into raw children to get the correct operator
        auto* opNode = dynamic_cast<antlr4::tree::TerminalNode*>(ctx->children[2 * (i - 1) + 1]);
        BinaryOp op = (opNode && opNode->getText() == "<<") ? BinaryOp::ShiftLeft : BinaryOp::ShiftRight;
        left = std::make_shared<BinaryExpression>(left, op, right, loc);
    }
    return std::static_pointer_cast<ASTNode>(left);
}

std::any ASTGenerator::visitAdditive(MingusParser::AdditiveContext* ctx) {
    auto loc = getSourceLocation(ctx);
    auto children = ctx->multiplicative();
    
    if (children.size() == 1) {
        return visitMultiplicative(children[0]);
    }
    
    auto left = anyToNode<ExpressionNode>(visitMultiplicative(children[0]));

    for (size_t i = 1; i < children.size(); ++i) {
        auto right = anyToNode<ExpressionNode>(visitMultiplicative(children[i]));
        // Use positional index into raw children to get the correct operator
        auto* opNode = dynamic_cast<antlr4::tree::TerminalNode*>(ctx->children[2 * (i - 1) + 1]);
        BinaryOp op = (opNode && opNode->getText() == "+") ? BinaryOp::Add : BinaryOp::Sub;
        left = std::make_shared<BinaryExpression>(left, op, right, loc);
    }
    return std::static_pointer_cast<ASTNode>(left);
}

std::any ASTGenerator::visitMultiplicative(MingusParser::MultiplicativeContext* ctx) {
    auto loc = getSourceLocation(ctx);
    auto children = ctx->castExpression();
    
    if (children.size() == 1) {
        return visitCastExpression(children[0]);
    }
    
    auto left = anyToNode<ExpressionNode>(visitCastExpression(children[0]));

    for (size_t i = 1; i < children.size(); ++i) {
        auto right = anyToNode<ExpressionNode>(visitCastExpression(children[i]));
        // Use positional index into raw children to get the correct operator
        auto* opNode = dynamic_cast<antlr4::tree::TerminalNode*>(ctx->children[2 * (i - 1) + 1]);
        BinaryOp op = BinaryOp::Mul;
        if (opNode) {
            std::string opText = opNode->getText();
            if (opText == "/") op = BinaryOp::Div;
            else if (opText == "%") op = BinaryOp::Mod;
        }
        left = std::make_shared<BinaryExpression>(left, op, right, loc);
    }
    return std::static_pointer_cast<ASTNode>(left);
}

std::any ASTGenerator::visitCastExpression(MingusParser::CastExpressionContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    if (ctx->typeIdentifier()) {
        auto targetType = anyToNode<TypeNode>(visitTypeIdentifier(ctx->typeIdentifier()));
        auto operand = anyToNode<ExpressionNode>(visitCastExpression(ctx->castExpression()));
        
        return std::static_pointer_cast<ASTNode>(
            std::make_shared<CastExpression>(targetType, operand, loc)
        );
    }
    
    return visitUnaryExpression(ctx->unaryExpression());
}

std::any ASTGenerator::visitUnaryExpression(MingusParser::UnaryExpressionContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    if (ctx->prefixOperator()) {
        auto op = parseUnaryOperator(ctx->prefixOperator()->getText());
        auto operand = anyToNode<ExpressionNode>(visitUnaryExpression(ctx->unaryExpression()));
        
        return std::static_pointer_cast<ASTNode>(
            std::make_shared<UnaryExpression>(op, operand, loc)
        );
    }
    
    if (ctx->incrementDecrementOperator()) {
        auto text = ctx->incrementDecrementOperator()->getText();
        UnaryOp op = (text == "++") ? UnaryOp::PreIncrement : UnaryOp::PreDecrement;
        auto operand = anyToNode<ExpressionNode>(visitUnaryExpression(ctx->unaryExpression()));
        
        return std::static_pointer_cast<ASTNode>(
            std::make_shared<UnaryExpression>(op, operand, loc)
        );
    }
    
    if (ctx->typeSizeOrAlign()) {
        auto type = anyToNode<TypeNode>(visitTypeIdentifier(ctx->typeIdentifier()));
        
        if (ctx->typeSizeOrAlign()->SizeOfKeyword()) {
            return std::static_pointer_cast<ASTNode>(
                std::make_shared<SizeOfExpression>(type, loc)
            );
        } else {
            return std::static_pointer_cast<ASTNode>(
                std::make_shared<AlignOfExpression>(type, loc)
            );
        }
    }
    
    return visitPostfixExpression(ctx->postfixExpression());
}

std::any ASTGenerator::visitPostfixExpression(MingusParser::PostfixExpressionContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    auto result = anyToNode<ExpressionNode>(visitPrimaryExpression(ctx->primaryExpression()));
    
    for (auto* opCtx : ctx->postfixOperation()) {
        if (opCtx->callArguments()) {
            NodeList<ExpressionNode> args;
            if (opCtx->callArguments()->argumentList()) {
                for (auto* argExpr : opCtx->callArguments()->argumentList()->expression()) {
                    auto arg = anyToNode<ExpressionNode>(visitExpression(argExpr));
                    if (arg) args.push_back(arg);
                }
            }
            result = std::make_shared<CallExpression>(result, args, loc);
        } else if (opCtx->elementAccess()) {
            auto index = anyToNode<ExpressionNode>(visitExpression(opCtx->elementAccess()->expression()));
            result = std::make_shared<IndexExpression>(result, index, loc);
        } else if (opCtx->memberAccess()) {
            std::string memberName = opCtx->memberAccess()->Identifier()->getText();
            bool isArrow = opCtx->memberAccess()->ReferenceAccessOperator() != nullptr;
            result = std::make_shared<MemberAccessExpression>(result, memberName, isArrow, loc);
        } else if (opCtx->incrementDecrementOperator()) {
            auto text = opCtx->incrementDecrementOperator()->getText();
            UnaryOp op = (text == "++") ? UnaryOp::PostIncrement : UnaryOp::PostDecrement;
            result = std::make_shared<UnaryExpression>(op, result, loc);
        }
    }
    
    return std::static_pointer_cast<ASTNode>(result);
}

std::any ASTGenerator::visitPrimaryExpression(MingusParser::PrimaryExpressionContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    if (ctx->IntegerLiteral()) {
        int64_t value = parseIntegerLiteral(ctx->IntegerLiteral()->getText());
        return std::static_pointer_cast<ASTNode>(
            std::make_shared<IntegerLiteral>(value, loc)
        );
    } else if (ctx->FloatingLiteral()) {
        double value = std::stod(ctx->FloatingLiteral()->getText());
        return std::static_pointer_cast<ASTNode>(
            std::make_shared<FloatLiteral>(value, loc)
        );
    } else if (ctx->BooleanLiteral()) {
        bool value = (ctx->BooleanLiteral()->getText() == "true");
        return std::static_pointer_cast<ASTNode>(
            std::make_shared<BoolLiteral>(value, loc)
        );
    } else if (ctx->CharLiteral()) {
        std::string text = ctx->CharLiteral()->getText();
        char value = (text.length() >= 3) ? text[1] : '\0';
        return std::static_pointer_cast<ASTNode>(
            std::make_shared<CharLiteral>(value, loc)
        );
    } else if (ctx->string()) {
        return visitString(ctx->string());
    } else if (ctx->NullReference()) {
        return std::static_pointer_cast<ASTNode>(
            std::make_shared<NullLiteral>(loc)
        );
    } else if (ctx->ThisReference()) {
        return std::static_pointer_cast<ASTNode>(
            std::make_shared<ThisExpression>(loc)
        );
    } else if (ctx->Identifier()) {
        std::string name = ctx->Identifier()->getText();
        return std::static_pointer_cast<ASTNode>(
            std::make_shared<IdentifierExpression>(name, loc)
        );
    } else if (ctx->tupleExpression()) {
        return visitTupleExpression(ctx->tupleExpression());
    } else if (ctx->newExpression()) {
        return visitNewExpression(ctx->newExpression());
    } else if (ctx->matchExpression()) {
        return visitMatchExpression(ctx->matchExpression());
    } else if (ctx->expression()) {
        return visitExpression(ctx->expression());
    }
    
    return std::shared_ptr<ASTNode>(nullptr);
}

std::any ASTGenerator::visitNewExpression(MingusParser::NewExpressionContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    auto type = anyToNode<TypeNode>(visitTypeIdentifier(ctx->typeIdentifier()));
    
    if (ctx->SquareBracketLeft()) {
        auto size = anyToNode<ExpressionNode>(visitExpression(ctx->expression()));
        return std::static_pointer_cast<ASTNode>(
            std::make_shared<NewExpression>(type, size, loc)
        );
    } else {
        NodeList<ExpressionNode> args;
        if (ctx->callArguments() && ctx->callArguments()->argumentList()) {
            for (auto* argCtx : ctx->callArguments()->argumentList()->expression()) {
                auto arg = anyToNode<ExpressionNode>(visitExpression(argCtx));
                if (arg) args.push_back(arg);
            }
        }
        return std::static_pointer_cast<ASTNode>(
            std::make_shared<NewExpression>(type, args, loc)
        );
    }
}

std::any ASTGenerator::visitTupleExpression(MingusParser::TupleExpressionContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    NodeList<ExpressionNode> elements;
    for (auto* exprCtx : ctx->expression()) {
        auto expr = anyToNode<ExpressionNode>(visitExpression(exprCtx));
        if (expr) elements.push_back(expr);
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<TupleExpression>(elements, loc)
    );
}

std::any ASTGenerator::visitMatchExpression(MingusParser::MatchExpressionContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    auto subject = anyToNode<ExpressionNode>(visitExpression(ctx->expression()));
    
    std::vector<MatchArm> arms;
    for (auto* armCtx : ctx->matchArm()) {
        auto pattern = anyToNode<PatternNode>(visitPattern(armCtx->pattern()));
        
        NodePtr<ASTNode> body;
        if (armCtx->matchBody()->expression()) {
            body = anyToNode<ExpressionNode>(visitExpression(armCtx->matchBody()->expression()));
        } else if (armCtx->matchBody()->block()) {
            body = anyToNode<BlockStatement>(visitBlock(armCtx->matchBody()->block()));
        }
        
        arms.emplace_back(pattern, body);
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<MatchExpression>(subject, arms, loc)
    );
}

std::any ASTGenerator::visitMatchArm(MingusParser::MatchArmContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTGenerator::visitCallArguments(MingusParser::CallArgumentsContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTGenerator::visitElementAccess(MingusParser::ElementAccessContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTGenerator::visitMemberAccess(MingusParser::MemberAccessContext* ctx) {
    return visitChildren(ctx);
}


//================================================================================
// Patterns
//================================================================================

std::any ASTGenerator::visitPattern(MingusParser::PatternContext* ctx) {
    return visitGuardedPattern(ctx->guardedPattern());
}

std::any ASTGenerator::visitGuardedPattern(MingusParser::GuardedPatternContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    auto innerPattern = anyToNode<PatternNode>(visitBasePattern(ctx->basePattern()));
    
    if (ctx->ControlFlowIf() && ctx->expression()) {
        auto guard = anyToNode<ExpressionNode>(visitExpression(ctx->expression()));
        return std::static_pointer_cast<ASTNode>(
            std::make_shared<GuardedPattern>(innerPattern, guard, loc)
        );
    }
    
    return std::static_pointer_cast<ASTNode>(innerPattern);
}

std::any ASTGenerator::visitLiteralPattern(MingusParser::LiteralPatternContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    NodePtr<ExpressionNode> value;
    
    if (ctx->IntegerLiteral()) {
        value = std::make_shared<IntegerLiteral>(
            parseIntegerLiteral(ctx->IntegerLiteral()->getText()), loc
        );
    } else if (ctx->FloatingLiteral()) {
        value = std::make_shared<FloatLiteral>(
            std::stod(ctx->FloatingLiteral()->getText()), loc
        );
    } else if (ctx->BooleanLiteral()) {
        value = std::make_shared<BoolLiteral>(
            ctx->BooleanLiteral()->getText() == "true", loc
        );
    } else if (ctx->CharLiteral()) {
        std::string text = ctx->CharLiteral()->getText();
        char charValue = (text.length() >= 3) ? text[1] : '\0';
        value = std::make_shared<CharLiteral>(charValue, loc);
    } else if (ctx->string()) {
        value = anyToNode<ExpressionNode>(visitString(ctx->string()));
    } else if (ctx->NullReference()) {
        value = std::make_shared<NullLiteral>(loc);
    } else if (ctx->qualifiedName()) {
        auto qname = parseQualifiedName(ctx->qualifiedName());
        value = std::make_shared<QualifiedNameExpression>(qname, loc);
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<LiteralPattern>(value, loc)
    );
}

std::any ASTGenerator::visitRangePattern(MingusParser::RangePatternContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    int64_t low = parseIntegerLiteral(ctx->IntegerLiteral(0)->getText());
    int64_t high = parseIntegerLiteral(ctx->IntegerLiteral(1)->getText());
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<RangePattern>(low, high, loc)
    );
}

std::any ASTGenerator::visitWildcardPattern(MingusParser::WildcardPatternContext* ctx) {
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<WildcardPattern>(getSourceLocation(ctx))
    );
}

std::any ASTGenerator::visitBindingPattern(MingusParser::BindingPatternContext* ctx) {
    auto loc = getSourceLocation(ctx);
    std::string name = ctx->Identifier()->getText();
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<BindingPattern>(name, loc)
    );
}

std::any ASTGenerator::visitTuplePattern(MingusParser::TuplePatternContext* ctx) {
    auto loc = getSourceLocation(ctx);
    
    PatternList<PatternNode> elements;
    for (auto* elemCtx : ctx->pattern()) {
        auto elem = anyToNode<PatternNode>(visitPattern(elemCtx));
        if (elem) elements.push_back(elem);
    }
    
    return std::static_pointer_cast<ASTNode>(
        std::make_shared<TuplePattern>(elements, loc)
    );
}

} // namespace parser
} // namespace mingus
