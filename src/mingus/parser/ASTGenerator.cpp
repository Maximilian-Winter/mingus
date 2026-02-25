//================================================================================
// MINGUS V2 - AST Generator Implementation
//
// Converts ANTLR4 parse trees into V2 AST nodes.
// Reuses V1 grammar unchanged; produces V2 AST node hierarchy.
//
// V2 patterns (distinct from V1):
//   - Default-constructed nodes with field assignment (no constructor args)
//   - DebugInfo with full source ranges (not SourceLocation)
//   - ArgumentsNode with per-arg isReference tracking
//   - CaptureDefault/CaptureItem on lambdas
//   - DeclarationBaseNode : StatementBaseNode (declarations ARE statements)
//   - Typed std::any returns (ExpressionBaseNode, StatementBaseNode, etc.)
//================================================================================

#include "mingus/parser/ASTGenerator.h"
#include "mingus/Expressions.h"
#include "mingus/Statements.h"
#include "mingus/Declarations.h"
#include "mingus/DebugInfo.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace mingus {
namespace parser {

using namespace AntlrMingusParser;

//================================================================================
// Utility Functions
//================================================================================

static int64_t parseIntegerLiteral(const std::string& text) {
    if (text.size() >= 2 && text[0] == '0') {
        char p = text[1];
        if (p == 'b' || p == 'B') return std::stoll(text.substr(2), nullptr, 2);
        if (p == 'o' || p == 'O') return std::stoll(text.substr(2), nullptr, 8);
    }
    return std::stoll(text, nullptr, 0);
}

static char parseCharLiteral(const std::string& text) {
    // text includes surrounding quotes: 'x' or '\n'
    if (text.length() < 3) return '\0';
    if (text[1] == '\\' && text.length() >= 4) {
        switch (text[2]) {
            case 'n':  return '\n';
            case 't':  return '\t';
            case 'r':  return '\r';
            case '0':  return '\0';
            case '\\': return '\\';
            case '\'': return '\'';
            case '"':  return '"';
            case 'a':  return '\a';
            case 'b':  return '\b';
            case 'f':  return '\f';
            case 'v':  return '\v';
            default:   return text[2];  // unknown escape, pass through
        }
    }
    return text[1];
}

//================================================================================
// DebugInfo Helper
//================================================================================

std::shared_ptr<DebugInfo> ASTGenerator::makeDebugInfo(antlr4::ParserRuleContext* ctx) {
    if (!ctx) return nullptr;
    auto* start = ctx->getStart();
    auto* stop = ctx->getStop();
    if (!start) return nullptr;

    int startLine = static_cast<int>(start->getLine());
    int startCol  = static_cast<int>(start->getCharPositionInLine());
    int endLine   = stop ? static_cast<int>(stop->getLine()) : startLine;
    int endCol    = stop ? static_cast<int>(stop->getCharPositionInLine()
                          + stop->getText().length()) : startCol;

    auto info = std::make_shared<DebugInfo>(startLine, startCol, endLine, endCol);
    info->sourceFile = sourceFile_;
    return info;
}

//================================================================================
// Error Reporting
//================================================================================

void ASTGenerator::reportError(antlr4::ParserRuleContext* ctx,
                               const std::string& message) {
    Error err;
    if (ctx && ctx->getStart()) {
        err.line   = static_cast<int>(ctx->getStart()->getLine());
        err.column = static_cast<int>(ctx->getStart()->getCharPositionInLine());
    }
    err.message = message;
    errors.push_back(err);
}

//================================================================================
// Entry Point
//================================================================================

std::shared_ptr<ProgramNode> ASTGenerator::generate(
    MingusParser::ProgramContext* ctx)
{
    errors.clear();
    if (!ctx) {
        reportError(nullptr, "Null program context");
        return nullptr;
    }
    try {
        auto result = visitProgram(ctx);
        if (!result.has_value()) {
            reportError(ctx, "visitProgram returned empty result");
            return nullptr;
        }
        auto node = anyToNode<ProgramNode>(result);
        if (!node) {
            reportError(ctx, "Failed to extract ProgramNode from visitor result");
            return nullptr;
        }
        return node;
    } catch (const std::exception& e) {
        reportError(ctx, std::string("Exception during AST generation: ") + e.what());
        return nullptr;
    }
}

//================================================================================
// Parse Helpers
//================================================================================

std::vector<std::string> ASTGenerator::parseQualifiedName(
    MingusParser::QualifiedNameContext* ctx)
{
    if (!ctx) return {};
    std::vector<std::string> parts;
    for (auto* id : ctx->Identifier()) {
        parts.push_back(id->getText());
    }
    return parts;
}

AccessModifier ASTGenerator::parseAccessModifier(
    MingusParser::AccessModifierContext* ctx)
{
    if (!ctx) return AccessModifier::Public;
    if (ctx->DeclarePublic())    return AccessModifier::Public;
    if (ctx->DeclarePrivate())   return AccessModifier::Private;
    if (ctx->DeclareProtected()) return AccessModifier::Protected;
    return AccessModifier::Public;
}

AssignOp ASTGenerator::parseAssignmentOperator(
    MingusParser::AssignmentOperatorContext* ctx)
{
    if (!ctx) return AssignOp::Assign;
    if (ctx->AssignOperator())                  return AssignOp::Assign;
    if (ctx->PlusAssignOperator())              return AssignOp::AddAssign;
    if (ctx->MinusAssignOperator())             return AssignOp::SubAssign;
    if (ctx->MultiplyAssignOperator())          return AssignOp::MulAssign;
    if (ctx->DivideAssignOperator())            return AssignOp::DivAssign;
    if (ctx->ModuloAssignOperator())            return AssignOp::ModAssign;
    if (ctx->BitwiseAndAssignOperator())        return AssignOp::AndAssign;
    if (ctx->BitwiseOrAssignOperator())         return AssignOp::OrAssign;
    if (ctx->BitwiseXorAssignOperator())        return AssignOp::XorAssign;
    if (ctx->BitwiseLeftShiftAssignOperator())  return AssignOp::ShiftLeftAssign;
    if (ctx->BitwiseRightShiftAssignOperator()) return AssignOp::ShiftRightAssign;
    return AssignOp::Assign;
}

BinaryOp ASTGenerator::parseBinaryOperator(const std::string& op) {
    if (op == "+")  return BinaryOp::Add;
    if (op == "-")  return BinaryOp::Sub;
    if (op == "*")  return BinaryOp::Mul;
    if (op == "/")  return BinaryOp::Div;
    if (op == "%")  return BinaryOp::Mod;
    if (op == "==") return BinaryOp::Equal;
    if (op == "!=") return BinaryOp::NotEqual;
    if (op == "<")  return BinaryOp::Less;
    if (op == "<=") return BinaryOp::LessEqual;
    if (op == ">")  return BinaryOp::Greater;
    if (op == ">=") return BinaryOp::GreaterEqual;
    if (op == "&&") return BinaryOp::LogicalAnd;
    if (op == "||") return BinaryOp::LogicalOr;
    if (op == "&")  return BinaryOp::BitwiseAnd;
    if (op == "|")  return BinaryOp::BitwiseOr;
    if (op == "^")  return BinaryOp::BitwiseXor;
    if (op == "<<") return BinaryOp::ShiftLeft;
    if (op == ">>") return BinaryOp::ShiftRight;
    return BinaryOp::Add;
}

UnaryOp ASTGenerator::parseUnaryOperator(const std::string& op) {
    if (op == "-")  return UnaryOp::Negate;
    if (op == "!")  return UnaryOp::LogicalNot;
    if (op == "~")  return UnaryOp::BitwiseNot;
    if (op == "&")  return UnaryOp::AddressOf;
    if (op == "*")  return UnaryOp::Dereference;
    if (op == "++") return UnaryOp::PreIncrement;
    if (op == "--") return UnaryOp::PreDecrement;
    return UnaryOp::Negate;
}

OverloadableOp ASTGenerator::parseOperatorKind(
    MingusParser::OverloadableOperatorContext* ctx)
{
    if (!ctx) return OverloadableOp::Add;
    if (ctx->PlusOperator())         return OverloadableOp::Add;
    if (ctx->MinusOperator())        return OverloadableOp::Sub;
    if (ctx->StarOperator())         return OverloadableOp::Mul;
    if (ctx->DivideOperator())       return OverloadableOp::Div;
    if (ctx->ModuloOperator())       return OverloadableOp::Mod;
    if (ctx->EqualOperator())        return OverloadableOp::Equal;
    if (ctx->UnequalOperator())      return OverloadableOp::NotEqual;
    if (ctx->SmallerOperator())      return OverloadableOp::Less;
    if (ctx->SmallerEqualOperator()) return OverloadableOp::LessEq;
    if (ctx->GreaterOperator())      return OverloadableOp::Greater;
    if (ctx->GreaterEqualOperator()) return OverloadableOp::GreaterEq;
    if (ctx->SquareBracketLeft() && ctx->SquareBracketRight())
        return OverloadableOp::Index;
    return OverloadableOp::Add;
}

void ASTGenerator::parseLayoutAttributes(
    const std::vector<MingusParser::AttributeContext*>& attrs,
    bool& isPacked, unsigned& alignment)
{
    for (auto* attr : attrs) {
        std::string name = attr->Identifier()->getText();
        if (name == "packed") {
            isPacked = true;
        } else if (name == "align") {
            if (attr->IntegerLiteral()) {
                alignment = std::stoul(attr->IntegerLiteral()->getText());
            }
        }
    }
}

//================================================================================
// Program Structure
//================================================================================

std::any ASTGenerator::visitProgram(MingusParser::ProgramContext* ctx) {
    auto program = std::make_shared<ProgramNode>();
    program->debugInfo = makeDebugInfo(ctx);

    for (auto* moduleCtx : ctx->module()) {
        auto mod = anyToNode<ModuleNode>(visitModule(moduleCtx));
        if (mod) program->modules.push_back(mod);
    }

    return std::any(program);
}

std::any ASTGenerator::visitModule(MingusParser::ModuleContext* ctx) {
    auto mod = std::make_shared<ModuleNode>();
    mod->debugInfo = makeDebugInfo(ctx);
    mod->name = ctx->Identifier()->getText();

    if (ctx->moduleBlock()) {
        for (auto* declCtx : ctx->moduleBlock()->moduleDeclaration()) {
            if (declCtx->importDefinition()) {
                auto imp = anyToNode<ImportDeclaration>(
                    visitImportDefinition(declCtx->importDefinition()));
                if (imp) mod->declarations.push_back(imp);

            } else if (declCtx->classDeclaration()) {
                auto cls = anyToNode<ClassDeclaration>(
                    visitClassDeclaration(declCtx->classDeclaration()));
                if (cls) mod->declarations.push_back(cls);

            } else if (declCtx->interfaceDeclaration()) {
                auto iface = anyToNode<InterfaceDeclaration>(
                    visitInterfaceDeclaration(declCtx->interfaceDeclaration()));
                if (iface) mod->declarations.push_back(iface);

            } else if (declCtx->structDeclaration()) {
                auto strct = anyToNode<StructDeclaration>(
                    visitStructDeclaration(declCtx->structDeclaration()));
                if (strct) mod->declarations.push_back(strct);

            } else if (declCtx->unionDeclaration()) {
                auto uni = anyToNode<UnionDeclaration>(
                    visitUnionDeclaration(declCtx->unionDeclaration()));
                if (uni) mod->declarations.push_back(uni);

            } else if (declCtx->taggedUnionDeclaration()) {
                auto tu = anyToNode<TaggedUnionDeclaration>(
                    visitTaggedUnionDeclaration(declCtx->taggedUnionDeclaration()));
                if (tu) mod->declarations.push_back(tu);

            } else if (declCtx->enumDeclaration()) {
                auto enm = anyToNode<EnumDeclaration>(
                    visitEnumDeclaration(declCtx->enumDeclaration()));
                if (enm) mod->declarations.push_back(enm);

            } else if (declCtx->functionDeclaration()) {
                auto fn = anyToNode<FunctionDeclaration>(
                    visitFunctionDeclaration(declCtx->functionDeclaration()));
                if (fn) mod->declarations.push_back(fn);

            } else if (declCtx->externDeclaration()) {
                auto* externCtx = declCtx->externDeclaration();
                if (externCtx->externBody()) {
                    auto* body = externCtx->externBody();

                    // Single-line forms (not in block)
                    if (body->externFunctionDeclaration()) {
                        auto ext = anyToNode<ExternFunctionDeclaration>(
                            visitExternFunctionDeclaration(body->externFunctionDeclaration()));
                        if (ext) mod->declarations.push_back(ext);
                    }
                    if (body->externLinkDirective()) {
                        auto link = std::make_shared<LinkDirective>();
                        link->debugInfo = makeDebugInfo(body->externLinkDirective());
                        auto* strCtx = body->externLinkDirective()->string();
                        if (strCtx) {
                            std::string raw;
                            for (auto* part : strCtx->stringPart()) {
                                if (part->TEXT()) raw += part->TEXT()->getText();
                                if (part->ESCAPE_SEQUENCE()) raw += part->ESCAPE_SEQUENCE()->getText();
                            }
                            link->libraryName = raw;
                        }
                        mod->declarations.push_back(link);
                    }
                    if (body->externOpaqueTypeDeclaration()) {
                        auto opaque = std::make_shared<OpaqueTypeDeclaration>();
                        opaque->debugInfo = makeDebugInfo(body->externOpaqueTypeDeclaration());
                        opaque->name = body->externOpaqueTypeDeclaration()->Identifier()->getText();
                        mod->declarations.push_back(opaque);
                    }
                    if (body->externVariableDeclaration()) {
                        auto extVar = std::make_shared<ExternVariableDeclaration>();
                        auto* evCtx = body->externVariableDeclaration();
                        extVar->debugInfo = makeDebugInfo(evCtx);
                        extVar->name = evCtx->Identifier()->getText();
                        extVar->type = anyToNode<TypeNode>(
                            visitTypeIdentifier(evCtx->typeIdentifier()));
                        mod->declarations.push_back(extVar);
                    }

                    // Block form: extern { ... }
                    for (auto* memberCtx : body->externMember()) {
                        if (memberCtx->externFunctionDeclaration()) {
                            auto ext = anyToNode<ExternFunctionDeclaration>(
                                visitExternFunctionDeclaration(memberCtx->externFunctionDeclaration()));
                            if (ext) mod->declarations.push_back(ext);
                        }
                        if (memberCtx->externLinkDirective()) {
                            auto link = std::make_shared<LinkDirective>();
                            link->debugInfo = makeDebugInfo(memberCtx->externLinkDirective());
                            auto* strCtx = memberCtx->externLinkDirective()->string();
                            if (strCtx) {
                                std::string raw;
                                for (auto* part : strCtx->stringPart()) {
                                    if (part->TEXT()) raw += part->TEXT()->getText();
                                    if (part->ESCAPE_SEQUENCE()) raw += part->ESCAPE_SEQUENCE()->getText();
                                }
                                link->libraryName = raw;
                            }
                            mod->declarations.push_back(link);
                        }
                        if (memberCtx->externOpaqueTypeDeclaration()) {
                            auto opaque = std::make_shared<OpaqueTypeDeclaration>();
                            opaque->debugInfo = makeDebugInfo(memberCtx->externOpaqueTypeDeclaration());
                            opaque->name = memberCtx->externOpaqueTypeDeclaration()->Identifier()->getText();
                            mod->declarations.push_back(opaque);
                        }
                        if (memberCtx->externStructDeclaration()) {
                            auto extStruct = std::make_shared<ExternStructDeclaration>();
                            auto* esCtx = memberCtx->externStructDeclaration();
                            extStruct->debugInfo = makeDebugInfo(esCtx);
                            extStruct->name = esCtx->Identifier()->getText();
                            if (!esCtx->attribute().empty()) {
                                parseLayoutAttributes(esCtx->attribute(),
                                    extStruct->isPacked, extStruct->alignment);
                            }
                            for (auto* fieldCtx : esCtx->externFieldDeclaration()) {
                                auto param = std::make_shared<ParameterNode>();
                                param->debugInfo = makeDebugInfo(fieldCtx);
                                param->name = fieldCtx->Identifier()->getText();
                                param->type = anyToNode<TypeNode>(
                                    visitTypeIdentifier(fieldCtx->typeIdentifier()));
                                extStruct->fields.push_back(param);
                            }
                            mod->declarations.push_back(extStruct);
                        }
                        if (memberCtx->externUnionDeclaration()) {
                            auto extUnion = std::make_shared<ExternUnionDeclaration>();
                            auto* euCtx = memberCtx->externUnionDeclaration();
                            extUnion->debugInfo = makeDebugInfo(euCtx);
                            extUnion->name = euCtx->Identifier()->getText();
                            if (!euCtx->attribute().empty()) {
                                parseLayoutAttributes(euCtx->attribute(),
                                    extUnion->isPacked, extUnion->alignment);
                            }
                            for (auto* fieldCtx : euCtx->externFieldDeclaration()) {
                                auto param = std::make_shared<ParameterNode>();
                                param->debugInfo = makeDebugInfo(fieldCtx);
                                param->name = fieldCtx->Identifier()->getText();
                                param->type = anyToNode<TypeNode>(
                                    visitTypeIdentifier(fieldCtx->typeIdentifier()));
                                extUnion->fields.push_back(param);
                            }
                            mod->declarations.push_back(extUnion);
                        }
                        if (memberCtx->externEnumDeclaration()) {
                            auto* eeCtx = memberCtx->externEnumDeclaration();
                            auto ed = std::make_shared<EnumDeclaration>();
                            ed->debugInfo = makeDebugInfo(eeCtx);
                            ed->name = eeCtx->Identifier()->getText();
                            ed->isExtern = true;
                            if (eeCtx->typeIdentifier()) {
                                ed->underlyingType = anyToNode<TypeNode>(
                                    visitTypeIdentifier(eeCtx->typeIdentifier()));
                            }
                            for (auto* memCtx : eeCtx->enumMember()) {
                                auto mem = std::make_shared<EnumMemberNode>();
                                mem->debugInfo = makeDebugInfo(memCtx);
                                mem->name = memCtx->Identifier()->getText();
                                if (memCtx->expression()) {
                                    mem->value = anyToNode<ExpressionBaseNode>(
                                        memCtx->expression()->accept(this));
                                }
                                ed->members.push_back(mem);
                            }
                            mod->declarations.push_back(ed);
                        }
                        if (memberCtx->externVariableDeclaration()) {
                            auto extVar = std::make_shared<ExternVariableDeclaration>();
                            auto* evCtx = memberCtx->externVariableDeclaration();
                            extVar->debugInfo = makeDebugInfo(evCtx);
                            extVar->name = evCtx->Identifier()->getText();
                            extVar->type = anyToNode<TypeNode>(
                                visitTypeIdentifier(evCtx->typeIdentifier()));
                            mod->declarations.push_back(extVar);
                        }
                    }
                }

            } else if (declCtx->variableDeclaration()) {
                auto var = anyToNode<VariableDeclaration>(
                    visitVariableDeclaration(declCtx->variableDeclaration()));
                if (var) mod->declarations.push_back(var);

            } else if (declCtx->typedefDeclaration()) {
                auto td = anyToNode<TypedefDeclaration>(
                    visitTypedefDeclaration(declCtx->typedefDeclaration()));
                if (td) mod->declarations.push_back(td);
            }
        }
    }

    return std::any(std::static_pointer_cast<DeclarationBaseNode>(mod));
}

std::any ASTGenerator::visitModuleBlock(MingusParser::ModuleBlockContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTGenerator::visitImportDefinition(
    MingusParser::ImportDefinitionContext* ctx)
{
    auto imp = std::make_shared<ImportDeclaration>();
    imp->debugInfo = makeDebugInfo(ctx);

    // Parse import targets: import name1, name2 from Module;
    for (auto* targetCtx : ctx->importTarget()) {
        ImportTarget target;
        target.name = targetCtx->Identifier(0)->getText();
        if (targetCtx->AsKeyword()) {
            target.alias = targetCtx->Identifier(1)->getText();
        }
        imp->targets.push_back(target);
    }

    // Source module path
    if (ctx->FromDirective() && ctx->qualifiedName()) {
        // import X from Module.Sub;
        imp->sourcePath = parseQualifiedName(ctx->qualifiedName());
    } else if (!ctx->FromDirective() && !imp->targets.empty()) {
        // import Module; — whole-module import
        // Grammar parses "OpLib" as an importTarget, but without "from",
        // the target is actually the module name.
        imp->sourcePath.push_back(imp->targets[0].name);
        imp->targets.clear();
        imp->isWholeModule = true;
    }

    return std::any(std::static_pointer_cast<DeclarationBaseNode>(imp));
}

std::any ASTGenerator::visitImportTarget(MingusParser::ImportTargetContext* ctx) {
    return visitChildren(ctx);
}

//================================================================================
// Typedef Declaration
//================================================================================

std::any ASTGenerator::visitTypedefDeclaration(
    MingusParser::TypedefDeclarationContext* ctx)
{
    auto node = std::make_shared<TypedefDeclaration>();
    node->debugInfo = makeDebugInfo(ctx);

    // Grammar: DeclareTypedef typeIdentifier Identifier SemicolonSeparator
    if (ctx->typeIdentifier()) {
        node->underlyingType = anyToNode<TypeNode>(
            visitTypeIdentifier(ctx->typeIdentifier()));
    }
    if (ctx->Identifier()) {
        node->aliasName = ctx->Identifier()->getText();
    }

    return std::any(std::static_pointer_cast<DeclarationBaseNode>(node));
}

//================================================================================
// Type Visitors
//================================================================================

std::any ASTGenerator::visitTypeIdentifier(
    MingusParser::TypeIdentifierContext* ctx)
{
    std::shared_ptr<TypeNode> baseType;
    bool isSharedType = (ctx->SharedKeyword() != nullptr);
    bool isConstType = (ctx->DeclareConst() != nullptr);

    if (ctx->primitiveType()) {
        baseType = anyToNode<TypeNode>(visitPrimitiveType(ctx->primitiveType()));

    } else if (ctx->qualifiedName()) {
        auto named = std::make_shared<NamedTypeNode>();
        named->debugInfo = makeDebugInfo(ctx);
        named->qualifiedName = parseQualifiedName(ctx->qualifiedName());

        // Parse type arguments: Pair<int, double>, Box<int>
        if (ctx->typeArgumentList()) {
            for (auto* typeIdCtx : ctx->typeArgumentList()->typeIdentifier()) {
                auto typeArg = anyToNode<TypeNode>(visitTypeIdentifier(typeIdCtx));
                if (typeArg) named->typeArguments.push_back(typeArg);
            }
        }

        baseType = named;

    } else if (ctx->tupleType()) {
        baseType = anyToNode<TypeNode>(visitTupleType(ctx->tupleType()));

    } else if (ctx->functionType()) {
        baseType = anyToNode<TypeNode>(visitFunctionType(ctx->functionType()));
    }

    if (!baseType) return std::any(std::shared_ptr<TypeNode>(nullptr));

    // Apply type modifiers: arrays, pointers, references
    bool sharedApplied = false;
    bool constApplied = false;
    for (auto* modifier : ctx->typeModifier()) {
        if (modifier->arrayDimension()) {
            auto* arrCtx = modifier->arrayDimension();
            auto arrType = std::make_shared<ArrayTypeNode>();
            arrType->debugInfo = makeDebugInfo(arrCtx);
            arrType->elementType = baseType;
            if (arrCtx->IntegerLiteral()) {
                auto sizeExpr = std::make_shared<IntegerLiteral>();
                sizeExpr->debugInfo = makeDebugInfo(arrCtx);
                sizeExpr->value = parseIntegerLiteral(
                    arrCtx->IntegerLiteral()->getText());
                arrType->sizeExpr = sizeExpr;
            }
            baseType = arrType;

        } else if (modifier->pointerLevel()) {
            auto ptrType = std::make_shared<PointerTypeNode>();
            ptrType->debugInfo = makeDebugInfo(modifier);
            ptrType->baseType = baseType;
            ptrType->isReference = false;
            // Apply 'shared' to the first pointer modifier: shared Foo*
            if (isSharedType && !sharedApplied) {
                ptrType->isShared = true;
                sharedApplied = true;
            }
            // Apply 'const' to the first pointer modifier: const int*
            if (isConstType && !constApplied) {
                ptrType->isConst = true;
                constApplied = true;
            }
            baseType = ptrType;

        } else if (modifier->rvalueReferenceLevel()) {
            auto refType = std::make_shared<PointerTypeNode>();
            refType->debugInfo = makeDebugInfo(modifier);
            refType->baseType = baseType;
            refType->isReference = true;
            refType->isRvalueReference = true;
            baseType = refType;

        } else if (modifier->referenceLevel()) {
            auto refType = std::make_shared<PointerTypeNode>();
            refType->debugInfo = makeDebugInfo(modifier);
            refType->baseType = baseType;
            refType->isReference = true;
            baseType = refType;
        }
    }

    return std::any(baseType);
}

std::any ASTGenerator::visitPrimitiveType(MingusParser::PrimitiveTypeContext* ctx) {
    auto prim = std::make_shared<PrimitiveTypeNode>();
    prim->debugInfo = makeDebugInfo(ctx);

    if      (ctx->IntegerType()) prim->kind = PrimitiveKind::Int;
    else if (ctx->DoubleType())  prim->kind = PrimitiveKind::Double;
    else if (ctx->FloatType())   prim->kind = PrimitiveKind::Float;
    else if (ctx->ByteType())    prim->kind = PrimitiveKind::Byte;
    else if (ctx->StringType())  prim->kind = PrimitiveKind::String;
    else if (ctx->CharType())    prim->kind = PrimitiveKind::Char;
    else if (ctx->BoolType())    prim->kind = PrimitiveKind::Bool;
    else if (ctx->VoidType())    prim->kind = PrimitiveKind::Void;
    else if (ctx->ShortType())   prim->kind = PrimitiveKind::Short;
    else if (ctx->UShortType())  prim->kind = PrimitiveKind::UShort;
    else if (ctx->UIntType())    prim->kind = PrimitiveKind::UInt;
    else if (ctx->LongType())    prim->kind = PrimitiveKind::Long;
    else if (ctx->ULongType())   prim->kind = PrimitiveKind::ULong;
    else                         prim->kind = PrimitiveKind::Int;

    return std::any(std::static_pointer_cast<TypeNode>(prim));
}

std::any ASTGenerator::visitTupleType(MingusParser::TupleTypeContext* ctx) {
    auto tuple = std::make_shared<TupleTypeNode>();
    tuple->debugInfo = makeDebugInfo(ctx);

    for (auto* typeId : ctx->typeIdentifier()) {
        auto elem = anyToNode<TypeNode>(visitTypeIdentifier(typeId));
        if (elem) tuple->elementTypes.push_back(elem);
    }

    return std::any(std::static_pointer_cast<TypeNode>(tuple));
}

std::any ASTGenerator::visitFunctionType(MingusParser::FunctionTypeContext* ctx) {
    auto fnType = std::make_shared<FunctionTypeNode>();
    fnType->debugInfo = makeDebugInfo(ctx);

    if (ctx->typeList()) {
        for (auto* typeId : ctx->typeList()->typeIdentifier()) {
            auto param = anyToNode<TypeNode>(visitTypeIdentifier(typeId));
            if (param) fnType->parameterTypes.push_back(param);
        }
    }

    fnType->returnType = anyToNode<TypeNode>(visitReturnType(ctx->returnType()));

    return std::any(std::static_pointer_cast<TypeNode>(fnType));
}

std::any ASTGenerator::visitArrayDimension(
    MingusParser::ArrayDimensionContext* ctx)
{
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
    // Default: void
    auto voidType = std::make_shared<PrimitiveTypeNode>();
    voidType->debugInfo = makeDebugInfo(ctx);
    voidType->kind = PrimitiveKind::Void;
    return std::any(std::static_pointer_cast<TypeNode>(voidType));
}

//================================================================================
// Utility Visitors
//================================================================================

std::any ASTGenerator::visitQualifiedName(
    MingusParser::QualifiedNameContext* ctx)
{
    return visitChildren(ctx);
}

std::any ASTGenerator::visitAccessModifier(
    MingusParser::AccessModifierContext* ctx)
{
    return visitChildren(ctx);
}

std::any ASTGenerator::visitStaticModifier(
    MingusParser::StaticModifierContext* ctx)
{
    return visitChildren(ctx);
}

std::any ASTGenerator::visitAbstractModifier(
    MingusParser::AbstractModifierContext* ctx)
{
    return visitChildren(ctx);
}

std::any ASTGenerator::visitOverloadableOperator(
    MingusParser::OverloadableOperatorContext* ctx)
{
    return visitChildren(ctx);
}

std::any ASTGenerator::visitString(MingusParser::StringContext* ctx) {
    std::vector<InterpolatedPart> parts;
    std::string currentText;

    for (auto* partCtx : ctx->stringPart()) {
        if (partCtx->TEXT()) {
            currentText += partCtx->TEXT()->getText();

        } else if (partCtx->ESCAPE_SEQUENCE()) {
            std::string esc = partCtx->ESCAPE_SEQUENCE()->getText();
            if      (esc == "\\n")  currentText += '\n';
            else if (esc == "\\t")  currentText += '\t';
            else if (esc == "\\r")  currentText += '\r';
            else if (esc == "\\\\") currentText += '\\';
            else if (esc == "\\\"") currentText += '"';
            else if (esc == "\\'")  currentText += '\'';
            else if (esc == "\\0")  currentText += '\0';
            else if (esc.length() >= 2) currentText += esc[1];

        } else if (partCtx->BACKSLASH_PAREN()) {
            // String interpolation: "\(expression)"
            if (!currentText.empty()) {
                InterpolatedPart textPart;
                textPart.kind = InterpolatedPartKind::Text;
                textPart.text = currentText;
                parts.push_back(textPart);
                currentText.clear();
            }
            auto expr = anyToNode<ExpressionBaseNode>(
                visitExpression(partCtx->expression()));
            if (expr) {
                InterpolatedPart exprPart;
                exprPart.kind = InterpolatedPartKind::Expression;
                exprPart.expression = expr;
                parts.push_back(exprPart);
            }
        }
    }

    if (!currentText.empty()) {
        InterpolatedPart textPart;
        textPart.kind = InterpolatedPartKind::Text;
        textPart.text = currentText;
        parts.push_back(textPart);
    }

    // Plain string (no interpolation) -> StringLiteral
    if (parts.size() == 1 && parts[0].kind == InterpolatedPartKind::Text) {
        auto strLit = std::make_shared<StringLiteral>();
        strLit->debugInfo = makeDebugInfo(ctx);
        strLit->value = parts[0].text;
        return std::any(std::static_pointer_cast<ExpressionBaseNode>(strLit));
    }

    // Interpolated string
    auto interp = std::make_shared<InterpolatedStringExpression>();
    interp->debugInfo = makeDebugInfo(ctx);
    interp->parts = std::move(parts);
    return std::any(std::static_pointer_cast<ExpressionBaseNode>(interp));
}

//================================================================================
// Declaration Visitors
//================================================================================

std::any ASTGenerator::visitClassDeclaration(
    MingusParser::ClassDeclarationContext* ctx)
{
    auto cls = std::make_shared<ClassDeclaration>();
    cls->debugInfo = makeDebugInfo(ctx);
    cls->name = ctx->Identifier()->getText();
    cls->accessModifier = parseAccessModifier(ctx->accessModifier());
    cls->isStatic = ctx->staticModifier() != nullptr;
    cls->isAbstract = ctx->abstractModifier() != nullptr;

    // Generic type parameters: class Box<T> { ... }
    if (ctx->typeParameterList()) {
        for (auto* id : ctx->typeParameterList()->Identifier()) {
            cls->typeParameters.push_back(id->getText());
        }
    }

    // Base classes / interfaces (with optional type arguments for generics)
    if (ctx->inheritance()) {
        for (auto* itemCtx : ctx->inheritance()->inheritanceItem()) {
            auto parts = parseQualifiedName(itemCtx->qualifiedName());
            // Store the simple name; sema resolves to ClassSymbol/InterfaceSymbol
            if (!parts.empty()) cls->baseClasses.push_back(parts.back());

            // Parse type arguments: Getter<int>, Comparable<double>
            std::vector<std::shared_ptr<TypeNode>> typeArgs;
            if (itemCtx->typeArgumentList()) {
                for (auto* typeIdCtx : itemCtx->typeArgumentList()->typeIdentifier()) {
                    auto typeArg = anyToNode<TypeNode>(visitTypeIdentifier(typeIdCtx));
                    if (typeArg) typeArgs.push_back(typeArg);
                }
            }
            cls->baseClassTypeArgs.push_back(std::move(typeArgs));
        }
    }

    // Class members
    if (ctx->classBlock()) {
        for (auto* memberCtx : ctx->classBlock()->classMember()) {
            if (memberCtx->constructorDeclaration()) {
                auto ctor = anyToNode<ConstructorDeclaration>(
                    visitConstructorDeclaration(
                        memberCtx->constructorDeclaration()));
                // Detect copy/move constructor: exactly 1 ref param of same class type
                // Copy: constructor(ClassName& other)  — isReference && !isRvalueReference
                // Move: constructor(ClassName&& other) — isReference && isRvalueReference
                bool isCopyCtor = false;
                bool isMoveCtor = false;
                if (ctor && ctor->parameters.size() == 1 &&
                    ctor->parameters[0]->isReference) {
                    auto* typeNode = ctor->parameters[0]->type.get();
                    // Unwrap PointerTypeNode for reference types
                    if (auto* ptrNode = dynamic_cast<PointerTypeNode*>(typeNode)) {
                        typeNode = ptrNode->baseType.get();
                    }
                    if (auto* named = dynamic_cast<NamedTypeNode*>(typeNode)) {
                        if (!named->qualifiedName.empty() &&
                            named->qualifiedName.back() == cls->name) {
                            if (ctor->parameters[0]->isRvalueReference) {
                                isMoveCtor = true;
                            } else {
                                isCopyCtor = true;
                            }
                        }
                    }
                }
                if (isMoveCtor) {
                    ctor->isMoveConstructor = true;
                    cls->moveConstructor = ctor;
                } else if (isCopyCtor) {
                    ctor->isCopyConstructor = true;
                    cls->copyConstructor = ctor;
                } else {
                    cls->constructors.push_back(ctor);
                }

            } else if (memberCtx->destructorDeclaration()) {
                cls->destructor = anyToNode<DestructorDeclaration>(
                    visitDestructorDeclaration(
                        memberCtx->destructorDeclaration()));

            } else if (memberCtx->operatorDeclaration()) {
                auto op = anyToNode<OperatorDeclaration>(
                    visitOperatorDeclaration(memberCtx->operatorDeclaration()));
                if (op) cls->operators.push_back(op);

            } else if (memberCtx->functionDeclaration()) {
                auto method = anyToNode<FunctionDeclaration>(
                    visitFunctionDeclaration(memberCtx->functionDeclaration()));
                if (method) cls->methods.push_back(method);

            } else if (memberCtx->variableDeclaration()) {
                auto field = anyToNode<VariableDeclaration>(
                    visitVariableDeclaration(memberCtx->variableDeclaration()));
                if (field) cls->fields.push_back(field);
            }
        }
    }

    return std::any(std::static_pointer_cast<DeclarationBaseNode>(cls));
}

std::any ASTGenerator::visitInterfaceDeclaration(
    MingusParser::InterfaceDeclarationContext* ctx)
{
    auto iface = std::make_shared<InterfaceDeclaration>();
    iface->debugInfo = makeDebugInfo(ctx);
    iface->name = ctx->Identifier()->getText();
    iface->accessModifier = parseAccessModifier(ctx->accessModifier());

    // Generic type parameters: interface Getter<T> { ... }
    if (ctx->typeParameterList()) {
        for (auto* id : ctx->typeParameterList()->Identifier()) {
            iface->typeParameters.push_back(id->getText());
        }
    }

    if (ctx->interfaceBlock()) {
        for (auto* memberCtx : ctx->interfaceBlock()->interfaceMember()) {
            if (memberCtx->functionDeclaration()) {
                auto method = anyToNode<FunctionDeclaration>(
                    visitFunctionDeclaration(memberCtx->functionDeclaration()));
                if (method) iface->methods.push_back(method);
            }
        }
    }

    return std::any(std::static_pointer_cast<DeclarationBaseNode>(iface));
}

std::any ASTGenerator::visitStructDeclaration(
    MingusParser::StructDeclarationContext* ctx)
{
    auto strct = std::make_shared<StructDeclaration>();
    strct->debugInfo = makeDebugInfo(ctx);
    strct->name = ctx->Identifier()->getText();
    strct->accessModifier = parseAccessModifier(ctx->accessModifier());
    if (!ctx->attribute().empty()) {
        parseLayoutAttributes(ctx->attribute(), strct->isPacked, strct->alignment);
    }

    // Generic type parameters: struct Pair<T, U> { ... }
    if (ctx->typeParameterList()) {
        for (auto* id : ctx->typeParameterList()->Identifier()) {
            strct->typeParameters.push_back(id->getText());
        }
    }

    if (ctx->structBlock()) {
        for (auto* memberCtx : ctx->structBlock()->structMember()) {
            if (memberCtx->operatorDeclaration()) {
                auto op = anyToNode<OperatorDeclaration>(
                    visitOperatorDeclaration(memberCtx->operatorDeclaration()));
                if (op) strct->operators.push_back(op);

            } else if (memberCtx->functionDeclaration()) {
                auto method = anyToNode<FunctionDeclaration>(
                    visitFunctionDeclaration(memberCtx->functionDeclaration()));
                if (method) strct->methods.push_back(method);

            } else if (memberCtx->variableDeclaration()) {
                auto field = anyToNode<VariableDeclaration>(
                    visitVariableDeclaration(memberCtx->variableDeclaration()));
                if (field) strct->fields.push_back(field);
            }
        }
    }

    return std::any(std::static_pointer_cast<DeclarationBaseNode>(strct));
}

std::any ASTGenerator::visitUnionDeclaration(
    MingusParser::UnionDeclarationContext* ctx)
{
    auto uni = std::make_shared<UnionDeclaration>();
    uni->debugInfo = makeDebugInfo(ctx);
    uni->name = ctx->Identifier()->getText();
    uni->accessModifier = parseAccessModifier(ctx->accessModifier());
    if (!ctx->attribute().empty()) {
        parseLayoutAttributes(ctx->attribute(), uni->isPacked, uni->alignment);
    }

    if (ctx->unionBlock()) {
        for (auto* memberCtx : ctx->unionBlock()->unionMember()) {
            if (memberCtx->functionDeclaration()) {
                auto method = anyToNode<FunctionDeclaration>(
                    visitFunctionDeclaration(memberCtx->functionDeclaration()));
                if (method) uni->methods.push_back(method);

            } else if (memberCtx->variableDeclaration()) {
                auto field = anyToNode<VariableDeclaration>(
                    visitVariableDeclaration(memberCtx->variableDeclaration()));
                if (field) uni->fields.push_back(field);
            }
        }
    }

    return std::any(std::static_pointer_cast<DeclarationBaseNode>(uni));
}

std::any ASTGenerator::visitTaggedUnionDeclaration(
    MingusParser::TaggedUnionDeclarationContext* ctx)
{
    auto tu = std::make_shared<TaggedUnionDeclaration>();
    tu->debugInfo = makeDebugInfo(ctx);
    tu->name = ctx->Identifier()->getText();
    tu->accessModifier = parseAccessModifier(ctx->accessModifier());

    for (auto* variantCtx : ctx->taggedUnionVariant()) {
        auto variant = anyToNode<TaggedUnionVariantNode>(
            visitTaggedUnionVariant(variantCtx));
        if (variant) tu->variants.push_back(variant);
    }

    return std::any(std::static_pointer_cast<DeclarationBaseNode>(tu));
}

std::any ASTGenerator::visitTaggedUnionVariant(
    MingusParser::TaggedUnionVariantContext* ctx)
{
    auto variant = std::make_shared<TaggedUnionVariantNode>();
    variant->debugInfo = makeDebugInfo(ctx);
    variant->name = ctx->Identifier()->getText();

    for (auto* fieldCtx : ctx->taggedUnionField()) {
        visitTaggedUnionField(fieldCtx);
        // Extract field info directly
        TaggedUnionFieldNode field;
        field.name = fieldCtx->Identifier()->getText();
        field.type = anyToNode<TypeNode>(
            visitTypeIdentifier(fieldCtx->typeIdentifier()));
        variant->fields.push_back(std::move(field));
    }

    return std::any(variant);
}

std::any ASTGenerator::visitTaggedUnionField(
    MingusParser::TaggedUnionFieldContext* ctx)
{
    // Field info extracted inline in visitTaggedUnionVariant
    return {};
}

std::any ASTGenerator::visitEnumDeclaration(
    MingusParser::EnumDeclarationContext* ctx)
{
    auto enm = std::make_shared<EnumDeclaration>();
    enm->debugInfo = makeDebugInfo(ctx);
    enm->name = ctx->Identifier()->getText();
    enm->accessModifier = parseAccessModifier(ctx->accessModifier());

    if (ctx->typeIdentifier()) {
        enm->underlyingType = anyToNode<TypeNode>(
            visitTypeIdentifier(ctx->typeIdentifier()));
    }

    for (auto* memberCtx : ctx->enumMember()) {
        auto member = anyToNode<EnumMemberNode>(visitEnumMember(memberCtx));
        if (member) enm->members.push_back(member);
    }

    return std::any(std::static_pointer_cast<DeclarationBaseNode>(enm));
}

std::any ASTGenerator::visitEnumMember(MingusParser::EnumMemberContext* ctx) {
    auto member = std::make_shared<EnumMemberNode>();
    member->debugInfo = makeDebugInfo(ctx);
    member->name = ctx->Identifier()->getText();

    if (ctx->AssignOperator() && ctx->expression()) {
        member->value = anyToNode<ExpressionBaseNode>(
            visitExpression(ctx->expression()));
    }

    return std::any(member);
}

std::any ASTGenerator::visitFunctionDeclaration(
    MingusParser::FunctionDeclarationContext* ctx)
{
    auto fn = std::make_shared<FunctionDeclaration>();
    fn->debugInfo = makeDebugInfo(ctx);
    fn->name = ctx->Identifier()->getText();
    fn->accessModifier = parseAccessModifier(ctx->accessModifier());
    fn->isStatic   = ctx->staticModifier()   != nullptr;
    fn->isAbstract = ctx->abstractModifier() != nullptr;

    // Generic type parameters: func foo<T, U>(...)
    if (ctx->typeParameterList()) {
        for (auto* id : ctx->typeParameterList()->Identifier()) {
            fn->typeParameters.push_back(id->getText());
        }
    }

    // Parameters
    if (ctx->definitionParameters() &&
        ctx->definitionParameters()->parameterList()) {
        for (auto* paramCtx :
             ctx->definitionParameters()->parameterList()->parameter()) {
            auto param = anyToNode<ParameterNode>(visitParameter(paramCtx));
            if (param) fn->parameters.push_back(param);
        }
    }

    // Return type
    fn->returnType = anyToNode<TypeNode>(visitReturnType(ctx->returnType()));

    // Body: block, expression body, or abstract (no body)
    if (ctx->block()) {
        fn->body = anyToNode<BlockStatementNode>(visitBlock(ctx->block()));

    } else if (ctx->expression()) {
        // Expression-bodied function: wrap in block with return
        auto expr = anyToNode<ExpressionBaseNode>(
            visitExpression(ctx->expression()));
        if (expr) {
            auto retStmt = std::make_shared<ReturnStatement>();
            retStmt->debugInfo = expr->debugInfo;
            retStmt->value = expr;

            fn->body = std::make_shared<BlockStatementNode>();
            fn->body->debugInfo = expr->debugInfo;
            fn->body->statements.push_back(retStmt);
        }
    }

    return std::any(std::static_pointer_cast<DeclarationBaseNode>(fn));
}

std::any ASTGenerator::visitConstructorDeclaration(
    MingusParser::ConstructorDeclarationContext* ctx)
{
    auto ctor = std::make_shared<ConstructorDeclaration>();
    ctor->debugInfo = makeDebugInfo(ctx);
    ctor->accessModifier = parseAccessModifier(ctx->accessModifier());

    // Parameters
    if (ctx->definitionParameters() &&
        ctx->definitionParameters()->parameterList()) {
        for (auto* paramCtx :
             ctx->definitionParameters()->parameterList()->parameter()) {
            auto param = anyToNode<ParameterNode>(visitParameter(paramCtx));
            if (param) ctor->parameters.push_back(param);
        }
    }

    // Super call: constructor(params) : super(args)
    if (ctx->SuperKeyword()) {
        ctor->hasSuperCall = true;
        if (ctx->callArguments() && ctx->callArguments()->argumentList()) {
            for (auto* argCtx :
                 ctx->callArguments()->argumentList()->expression()) {
                auto arg = anyToNode<ExpressionBaseNode>(
                    visitExpression(argCtx));
                if (arg) ctor->superArgs.push_back(arg);
            }
        }
    }

    ctor->body = anyToNode<BlockStatementNode>(visitBlock(ctx->block()));

    return std::any(std::static_pointer_cast<DeclarationBaseNode>(ctor));
}

std::any ASTGenerator::visitDestructorDeclaration(
    MingusParser::DestructorDeclarationContext* ctx)
{
    auto dtor = std::make_shared<DestructorDeclaration>();
    dtor->debugInfo = makeDebugInfo(ctx);
    dtor->body = anyToNode<BlockStatementNode>(visitBlock(ctx->block()));
    return std::any(std::static_pointer_cast<DeclarationBaseNode>(dtor));
}

std::any ASTGenerator::visitOperatorDeclaration(
    MingusParser::OperatorDeclarationContext* ctx)
{
    auto opDecl = std::make_shared<OperatorDeclaration>();
    opDecl->debugInfo = makeDebugInfo(ctx);
    opDecl->op = parseOperatorKind(ctx->overloadableOperator());

    if (ctx->definitionParameters() &&
        ctx->definitionParameters()->parameterList()) {
        for (auto* paramCtx :
             ctx->definitionParameters()->parameterList()->parameter()) {
            auto param = anyToNode<ParameterNode>(visitParameter(paramCtx));
            if (param) opDecl->parameters.push_back(param);
        }
    }

    opDecl->returnType = anyToNode<TypeNode>(visitReturnType(ctx->returnType()));

    if (ctx->block()) {
        opDecl->body = anyToNode<BlockStatementNode>(visitBlock(ctx->block()));
    } else if (ctx->expression()) {
        auto expr = anyToNode<ExpressionBaseNode>(
            visitExpression(ctx->expression()));
        if (expr) {
            auto retStmt = std::make_shared<ReturnStatement>();
            retStmt->debugInfo = expr->debugInfo;
            retStmt->value = expr;

            opDecl->body = std::make_shared<BlockStatementNode>();
            opDecl->body->debugInfo = expr->debugInfo;
            opDecl->body->statements.push_back(retStmt);
        }
    }

    return std::any(std::static_pointer_cast<DeclarationBaseNode>(opDecl));
}

std::any ASTGenerator::visitExternFunctionDeclaration(
    MingusParser::ExternFunctionDeclarationContext* ctx)
{
    auto ext = std::make_shared<ExternFunctionDeclaration>();
    ext->debugInfo = makeDebugInfo(ctx);
    ext->name = ctx->Identifier()->getText();

    // Parse calling convention from attributes (@cdecl, @stdcall, @fastcall)
    for (auto* attr : ctx->attribute()) {
        std::string name = attr->Identifier()->getText();
        if (name == "cdecl" || name == "stdcall" || name == "fastcall") {
            ext->callingConvention = name;
        }
    }

    if (ctx->parameterList()) {
        for (auto* paramCtx : ctx->parameterList()->parameter()) {
            auto param = anyToNode<ParameterNode>(visitParameter(paramCtx));
            if (param) ext->parameters.push_back(param);
        }
    }

    ext->isVariadic = (ctx->Ellipsis() != nullptr);
    ext->returnType = anyToNode<TypeNode>(visitReturnType(ctx->returnType()));

    return std::any(std::static_pointer_cast<DeclarationBaseNode>(ext));
}

std::any ASTGenerator::visitParameter(MingusParser::ParameterContext* ctx) {
    auto param = std::make_shared<ParameterNode>();
    param->debugInfo = makeDebugInfo(ctx);
    param->name = ctx->Identifier()->getText();

    // Detect reference modifier (&) or rvalue reference (&&) in type modifiers
    for (auto* modifier : ctx->typeIdentifier()->typeModifier()) {
        if (modifier->rvalueReferenceLevel()) {
            param->isReference = true;
            param->isRvalueReference = true;
            break;
        } else if (modifier->referenceLevel()) {
            param->isReference = true;
            break;
        }
    }

    param->type = anyToNode<TypeNode>(visitTypeIdentifier(ctx->typeIdentifier()));

    // Default value
    if (ctx->AssignOperator() && ctx->expression()) {
        param->defaultValue = anyToNode<ExpressionBaseNode>(
            visitExpression(ctx->expression()));
    }

    return std::any(param);
}

std::any ASTGenerator::visitVariableDeclaration(
    MingusParser::VariableDeclarationContext* ctx)
{
    if (ctx->typedVariableDeclaration()) {
        return visitTypedVariableDeclaration(ctx->typedVariableDeclaration());
    } else if (ctx->inferredVariableDeclaration()) {
        return visitInferredVariableDeclaration(
            ctx->inferredVariableDeclaration());
    } else if (ctx->constVariableDeclaration()) {
        return visitConstVariableDeclaration(ctx->constVariableDeclaration());
    } else if (ctx->tupleDestructuring()) {
        return visitTupleDestructuring(ctx->tupleDestructuring());
    }
    return std::any();
}

std::any ASTGenerator::visitTypedVariableDeclaration(
    MingusParser::TypedVariableDeclarationContext* ctx)
{
    auto var = std::make_shared<VariableDeclaration>();
    var->debugInfo = makeDebugInfo(ctx);
    var->name = ctx->Identifier()->getText();
    var->isInferred = false;
    var->type = anyToNode<TypeNode>(visitTypeIdentifier(ctx->typeIdentifier()));

    // Access and static from parent variableDeclaration context
    auto* parent = dynamic_cast<MingusParser::VariableDeclarationContext*>(
        ctx->parent);
    var->accessModifier = parseAccessModifier(
        parent ? parent->accessModifier() : nullptr);
    var->isStatic = parent && parent->staticModifier() != nullptr;

    // Initializer: = expression
    if (ctx->AssignOperator() && ctx->exprStatement()) {
        if (ctx->exprStatement()->expression()) {
            var->initializer = anyToNode<ExpressionBaseNode>(
                visitExpression(ctx->exprStatement()->expression()));
        }
    } else if (ctx->callArguments()) {
        // Constructor-style init: Type name(args)
        auto argsNode = std::make_shared<ArgumentsNode>();
        argsNode->debugInfo = makeDebugInfo(ctx);
        if (ctx->callArguments()->argumentList()) {
            for (auto* argCtx :
                 ctx->callArguments()->argumentList()->expression()) {
                auto arg = anyToNode<ExpressionBaseNode>(
                    visitExpression(argCtx));
                if (arg) argsNode->expressions.push_back(arg);
            }
        }

        auto typeExpr = std::make_shared<IdentifierExpression>();
        typeExpr->debugInfo = makeDebugInfo(ctx);
        if (auto named = std::dynamic_pointer_cast<NamedTypeNode>(var->type)) {
            typeExpr->name = named->qualifiedName.empty()
                ? "" : named->qualifiedName.back();
        }

        auto callExpr = std::make_shared<CallExpression>();
        callExpr->debugInfo = makeDebugInfo(ctx);
        callExpr->callee = typeExpr;
        callExpr->arguments = argsNode;
        var->initializer = callExpr;
    }

    return std::any(std::static_pointer_cast<DeclarationBaseNode>(var));
}

std::any ASTGenerator::visitInferredVariableDeclaration(
    MingusParser::InferredVariableDeclarationContext* ctx)
{
    auto var = std::make_shared<VariableDeclaration>();
    var->debugInfo = makeDebugInfo(ctx);
    var->name = ctx->Identifier()->getText();
    var->isInferred = true;

    auto* parent = dynamic_cast<MingusParser::VariableDeclarationContext*>(
        ctx->parent);
    var->accessModifier = parseAccessModifier(
        parent ? parent->accessModifier() : nullptr);
    var->isStatic = parent && parent->staticModifier() != nullptr;

    if (ctx->exprStatement() && ctx->exprStatement()->expression()) {
        var->initializer = anyToNode<ExpressionBaseNode>(
            visitExpression(ctx->exprStatement()->expression()));
    }

    return std::any(std::static_pointer_cast<DeclarationBaseNode>(var));
}

std::any ASTGenerator::visitConstVariableDeclaration(
    MingusParser::ConstVariableDeclarationContext* ctx)
{
    auto var = std::make_shared<VariableDeclaration>();
    var->debugInfo = makeDebugInfo(ctx);
    var->isConst = true;

    if (ctx->typeIdentifier()) {
        // const int x = expr;
        var->name = ctx->Identifier()->getText();
        var->isInferred = false;
        var->type = anyToNode<TypeNode>(visitTypeIdentifier(ctx->typeIdentifier()));
    } else {
        // const x = expr;  (inferred)
        var->name = ctx->Identifier()->getText();
        var->isInferred = true;
    }

    auto* parent = dynamic_cast<MingusParser::VariableDeclarationContext*>(
        ctx->parent);
    var->accessModifier = parseAccessModifier(
        parent ? parent->accessModifier() : nullptr);
    var->isStatic = parent && parent->staticModifier() != nullptr;

    if (ctx->exprStatement() && ctx->exprStatement()->expression()) {
        var->initializer = anyToNode<ExpressionBaseNode>(
            visitExpression(ctx->exprStatement()->expression()));
    }

    return std::any(std::static_pointer_cast<DeclarationBaseNode>(var));
}

std::any ASTGenerator::visitTupleDestructuring(
    MingusParser::TupleDestructuringContext* ctx)
{
    auto decl = std::make_shared<TupleDestructuringDeclaration>();
    decl->debugInfo = makeDebugInfo(ctx);

    for (auto* elemCtx : ctx->tupleDestructureElement()) {
        DestructureElement elem;
        elem.name = elemCtx->Identifier()->getText();
        if (elemCtx->DeclareVariable()) {
            elem.isInferred = true;
        } else {
            elem.type = anyToNode<TypeNode>(
                visitTypeIdentifier(elemCtx->typeIdentifier()));
            elem.isInferred = false;
        }
        decl->elements.push_back(elem);
    }

    if (ctx->exprStatement() && ctx->exprStatement()->expression()) {
        decl->initializer = anyToNode<ExpressionBaseNode>(
            visitExpression(ctx->exprStatement()->expression()));
    }

    return std::any(std::static_pointer_cast<StatementBaseNode>(decl));
}

std::any ASTGenerator::visitTupleDestructureElement(
    MingusParser::TupleDestructureElementContext* ctx)
{
    return visitChildren(ctx);
}

//================================================================================
// Statement Visitors
//================================================================================

std::any ASTGenerator::visitBlock(MingusParser::BlockContext* ctx) {
    auto block = std::make_shared<BlockStatementNode>();
    block->debugInfo = makeDebugInfo(ctx);

    for (auto* stmtCtx : ctx->statement()) {
        auto stmt = anyToNode<StatementBaseNode>(visitStatement(stmtCtx));
        if (stmt) block->statements.push_back(stmt);
    }

    return std::any(std::static_pointer_cast<StatementBaseNode>(block));
}

std::any ASTGenerator::visitStatement(MingusParser::StatementContext* ctx) {
    if (ctx->exprStatement()) {
        return visitExprStatement(ctx->exprStatement());

    } else if (ctx->variableDeclaration()) {
        // V2: DeclarationBaseNode IS-A StatementBaseNode — no wrapping needed
        return visitVariableDeclaration(ctx->variableDeclaration());

    } else if (ctx->forStatement()) {
        return visitForStatement(ctx->forStatement());

    } else if (ctx->whileStatement()) {
        return visitWhileStatement(ctx->whileStatement());

    } else if (ctx->doWhileStatement()) {
        return visitDoWhileStatement(ctx->doWhileStatement());

    } else if (ctx->ifStatement()) {
        return visitIfStatement(ctx->ifStatement());

    } else if (ctx->switchStatement()) {
        return visitSwitchStatement(ctx->switchStatement());

    } else if (ctx->matchStatement()) {
        // Match used as statement: wrap in ExpressionStatement
        auto matchExpr = anyToNode<ExpressionBaseNode>(
            visitMatchExpression(ctx->matchStatement()->matchExpression()));
        auto stmt = std::make_shared<ExpressionStatement>();
        stmt->debugInfo = makeDebugInfo(ctx);
        stmt->expression = matchExpr;
        return std::any(std::static_pointer_cast<StatementBaseNode>(stmt));

    } else if (ctx->returnStatement()) {
        return visitReturnStatement(ctx->returnStatement());

    } else if (ctx->labeledStatement()) {
        return visitLabeledStatement(ctx->labeledStatement());

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

    return std::any();
}

std::any ASTGenerator::visitExprStatement(
    MingusParser::ExprStatementContext* ctx)
{
    auto stmt = std::make_shared<ExpressionStatement>();
    stmt->debugInfo = makeDebugInfo(ctx);
    stmt->expression = anyToNode<ExpressionBaseNode>(
        visitExpression(ctx->expression()));
    return std::any(std::static_pointer_cast<StatementBaseNode>(stmt));
}

std::any ASTGenerator::visitReturnStatement(
    MingusParser::ReturnStatementContext* ctx)
{
    auto stmt = std::make_shared<ReturnStatement>();
    stmt->debugInfo = makeDebugInfo(ctx);
    if (ctx->expression()) {
        stmt->value = anyToNode<ExpressionBaseNode>(
            visitExpression(ctx->expression()));
    }
    return std::any(std::static_pointer_cast<StatementBaseNode>(stmt));
}

std::any ASTGenerator::visitIfStatement(MingusParser::IfStatementContext* ctx) {
    auto stmt = std::make_shared<IfStatement>();
    stmt->debugInfo = makeDebugInfo(ctx);
    stmt->condition = anyToNode<ExpressionBaseNode>(
        visitExpression(ctx->expression()));
    stmt->thenBody = anyToNode<StatementBaseNode>(
        visitStatement(ctx->trueBody));

    // Else-if clauses
    for (auto* elseIfCtx : ctx->elseIfClause()) {
        ElseIfClause clause;
        clause.condition = anyToNode<ExpressionBaseNode>(
            visitExpression(elseIfCtx->expression()));
        clause.body = anyToNode<StatementBaseNode>(
            visitStatement(elseIfCtx->statement()));
        stmt->elseIfClauses.push_back(clause);
    }

    // Else clause
    if (ctx->elseClause()) {
        stmt->elseBody = anyToNode<StatementBaseNode>(
            visitElseClause(ctx->elseClause()));
    }

    return std::any(std::static_pointer_cast<StatementBaseNode>(stmt));
}

std::any ASTGenerator::visitElseIfClause(
    MingusParser::ElseIfClauseContext* ctx)
{
    return visitChildren(ctx);
}

std::any ASTGenerator::visitElseClause(MingusParser::ElseClauseContext* ctx) {
    return visitStatement(ctx->statement());
}

std::any ASTGenerator::visitSwitchStatement(
    MingusParser::SwitchStatementContext* ctx)
{
    auto stmt = std::make_shared<SwitchStatement>();
    stmt->debugInfo = makeDebugInfo(ctx);
    stmt->subject = anyToNode<ExpressionBaseNode>(
        visitExpression(ctx->expression()));

    for (auto* caseCtx : ctx->switchCase()) {
        SwitchCase sc;
        sc.value = anyToNode<ExpressionBaseNode>(
            visitExpression(caseCtx->expression()));
        for (auto* bodyStmt : caseCtx->statement()) {
            auto s = anyToNode<StatementBaseNode>(visitStatement(bodyStmt));
            if (s) sc.body.push_back(s);
        }
        stmt->cases.push_back(sc);
    }

    if (ctx->switchDefault()) {
        for (auto* bodyStmt : ctx->switchDefault()->statement()) {
            auto s = anyToNode<StatementBaseNode>(visitStatement(bodyStmt));
            if (s) stmt->defaultCase.push_back(s);
        }
    }

    return std::any(std::static_pointer_cast<StatementBaseNode>(stmt));
}

std::any ASTGenerator::visitSwitchCase(MingusParser::SwitchCaseContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTGenerator::visitSwitchDefault(
    MingusParser::SwitchDefaultContext* ctx)
{
    return visitChildren(ctx);
}

std::any ASTGenerator::visitForStatement(
    MingusParser::ForStatementContext* ctx)
{
    auto stmt = std::make_shared<ForStatement>();
    stmt->debugInfo = makeDebugInfo(ctx);

    // Initializer
    if (ctx->forInitializer()) {
        auto* initCtx = ctx->forInitializer();
        if (initCtx->localVarInitializer()) {
            for (auto* localVarCtx :
                 initCtx->localVarInitializer()->localVarDeclaration()) {
                auto varDecl = std::make_shared<VariableDeclaration>();
                varDecl->debugInfo = makeDebugInfo(localVarCtx);
                varDecl->name = localVarCtx->Identifier()->getText();

                if (localVarCtx->DeclareConst()) {
                    varDecl->isConst = true;
                    if (localVarCtx->typeIdentifier()) {
                        varDecl->type = anyToNode<TypeNode>(
                            visitTypeIdentifier(localVarCtx->typeIdentifier()));
                    } else {
                        varDecl->isInferred = true;
                    }
                } else if (localVarCtx->DeclareVariable()) {
                    varDecl->isInferred = true;
                } else {
                    varDecl->type = anyToNode<TypeNode>(
                        visitTypeIdentifier(localVarCtx->typeIdentifier()));
                }

                if (localVarCtx->expression()) {
                    varDecl->initializer = anyToNode<ExpressionBaseNode>(
                        visitExpression(localVarCtx->expression()));
                }

                stmt->initDeclarations.push_back(varDecl);
            }
        } else if (!initCtx->expression().empty()) {
            for (auto* exprCtx : initCtx->expression()) {
                auto expr = anyToNode<ExpressionBaseNode>(
                    visitExpression(exprCtx));
                if (expr) stmt->initExpressions.push_back(expr);
            }
        }
    }

    // Condition
    if (ctx->expression()) {
        stmt->condition = anyToNode<ExpressionBaseNode>(
            visitExpression(ctx->expression()));
    }

    // Iterators
    if (ctx->forIterator()) {
        for (auto* exprCtx : ctx->forIterator()->expression()) {
            auto expr = anyToNode<ExpressionBaseNode>(visitExpression(exprCtx));
            if (expr) stmt->iterators.push_back(expr);
        }
    }

    // Body
    stmt->body = anyToNode<StatementBaseNode>(visitStatement(ctx->statement()));

    return std::any(std::static_pointer_cast<StatementBaseNode>(stmt));
}

std::any ASTGenerator::visitWhileStatement(
    MingusParser::WhileStatementContext* ctx)
{
    auto stmt = std::make_shared<WhileStatement>();
    stmt->debugInfo = makeDebugInfo(ctx);
    stmt->condition = anyToNode<ExpressionBaseNode>(
        visitExpression(ctx->expression()));

    if (ctx->block()) {
        stmt->body = anyToNode<StatementBaseNode>(visitBlock(ctx->block()));
    } else if (ctx->statement()) {
        stmt->body = anyToNode<StatementBaseNode>(
            visitStatement(ctx->statement()));
    }

    return std::any(std::static_pointer_cast<StatementBaseNode>(stmt));
}

std::any ASTGenerator::visitDoWhileStatement(
    MingusParser::DoWhileStatementContext* ctx)
{
    auto stmt = std::make_shared<DoWhileStatement>();
    stmt->debugInfo = makeDebugInfo(ctx);

    if (ctx->block()) {
        stmt->body = anyToNode<StatementBaseNode>(visitBlock(ctx->block()));
    } else if (ctx->statement()) {
        stmt->body = anyToNode<StatementBaseNode>(
            visitStatement(ctx->statement()));
    }

    stmt->condition = anyToNode<ExpressionBaseNode>(
        visitExpression(ctx->expression()));

    return std::any(std::static_pointer_cast<StatementBaseNode>(stmt));
}

std::any ASTGenerator::visitLabeledStatement(
    MingusParser::LabeledStatementContext* ctx)
{
    auto stmt = std::make_shared<LabeledStatement>();
    stmt->debugInfo = makeDebugInfo(ctx);
    if (ctx->Identifier()) {
        stmt->label = ctx->Identifier()->getText();
    }
    // The inner loop statement
    if (ctx->forStatement()) {
        stmt->statement = anyToNode<StatementBaseNode>(
            visitForStatement(ctx->forStatement()));
    } else if (ctx->whileStatement()) {
        stmt->statement = anyToNode<StatementBaseNode>(
            visitWhileStatement(ctx->whileStatement()));
    } else if (ctx->doWhileStatement()) {
        stmt->statement = anyToNode<StatementBaseNode>(
            visitDoWhileStatement(ctx->doWhileStatement()));
    }
    return std::any(std::static_pointer_cast<StatementBaseNode>(stmt));
}

std::any ASTGenerator::visitBreakStatement(
    MingusParser::BreakStatementContext* ctx)
{
    auto stmt = std::make_shared<BreakStatement>();
    stmt->debugInfo = makeDebugInfo(ctx);
    if (ctx->Identifier()) {
        stmt->label = ctx->Identifier()->getText();
    }
    return std::any(std::static_pointer_cast<StatementBaseNode>(stmt));
}

std::any ASTGenerator::visitContinueStatement(
    MingusParser::ContinueStatementContext* ctx)
{
    auto stmt = std::make_shared<ContinueStatement>();
    stmt->debugInfo = makeDebugInfo(ctx);
    if (ctx->Identifier()) {
        stmt->label = ctx->Identifier()->getText();
    }
    return std::any(std::static_pointer_cast<StatementBaseNode>(stmt));
}

std::any ASTGenerator::visitDeleteStatement(
    MingusParser::DeleteStatementContext* ctx)
{
    auto stmt = std::make_shared<DeleteStatement>();
    stmt->debugInfo = makeDebugInfo(ctx);
    stmt->target = anyToNode<ExpressionBaseNode>(
        visitExpression(ctx->expression()));
    return std::any(std::static_pointer_cast<StatementBaseNode>(stmt));
}

std::any ASTGenerator::visitRawBlock(MingusParser::RawBlockContext* ctx) {
    // V2 has no separate RawBlock node; treat as regular block
    return visitBlock(ctx->block());
}

//================================================================================
// Expression Visitors
//================================================================================

std::any ASTGenerator::visitExpression(MingusParser::ExpressionContext* ctx) {
    if (ctx->assignment()) {
        return visitAssignment(ctx->assignment());
    } else if (ctx->lambdaExpression()) {
        return visitLambdaExpression(ctx->lambdaExpression());
    }
    return std::any();
}

std::any ASTGenerator::visitAssignment(MingusParser::AssignmentContext* ctx) {
    if (ctx->unaryExpression() && ctx->assignmentOperator()) {
        auto target = anyToNode<ExpressionBaseNode>(
            visitUnaryExpression(ctx->unaryExpression()));

        // RHS: recursive assignment or lambda
        std::shared_ptr<ExpressionBaseNode> value;
        if (ctx->lambdaExpression()) {
            value = anyToNode<ExpressionBaseNode>(
                visitLambdaExpression(ctx->lambdaExpression()));
        } else if (ctx->assignment()) {
            value = anyToNode<ExpressionBaseNode>(
                visitAssignment(ctx->assignment()));
        }

        auto assign = std::make_shared<AssignmentExpression>();
        assign->debugInfo = makeDebugInfo(ctx);
        assign->target = target;
        assign->op = parseAssignmentOperator(ctx->assignmentOperator());
        assign->value = value;
        return std::any(std::static_pointer_cast<ExpressionBaseNode>(assign));

    } else if (ctx->pipe()) {
        return visitPipe(ctx->pipe());
    }

    return std::any();
}

std::any ASTGenerator::visitLambdaExpression(
    MingusParser::LambdaExpressionContext* ctx)
{
    auto lambda = std::make_shared<LambdaExpression>();
    lambda->debugInfo = makeDebugInfo(ctx);

    // ---- Capture list ----
    if (ctx->captureList()) {
        auto* capCtx = ctx->captureList();

        if (capCtx->captureDefault()) {
            auto* defCtx = capCtx->captureDefault();
            if (defCtx->AssignOperator()) {
                lambda->captureDefault = CaptureDefault::ByCopy;
            } else if (defCtx->SingleAndOperator()) {
                lambda->captureDefault = CaptureDefault::ByRef;
            }
        }

        for (auto* itemCtx : capCtx->captureItem()) {
            CaptureItem item;
            item.name = itemCtx->Identifier()->getText();
            if (itemCtx->SingleAndOperator()) {
                item.mode = CaptureMode::ByReference;
            } else if (itemCtx->WeakKeyword()) {
                item.mode = CaptureMode::Weak;
            } else {
                item.mode = CaptureMode::ByValue;
            }
            lambda->captureItems.push_back(item);
        }
    }

    // ---- Parameters ----
    if (ctx->lambdaParameterList()) {
        for (auto* paramCtx :
             ctx->lambdaParameterList()->lambdaParameter()) {
            auto param = std::make_shared<ParameterNode>();
            param->debugInfo = makeDebugInfo(paramCtx);
            param->name = paramCtx->Identifier()->getText();
            if (paramCtx->typeIdentifier()) {
                param->type = anyToNode<TypeNode>(
                    visitTypeIdentifier(paramCtx->typeIdentifier()));
            }
            lambda->parameters.push_back(param);
        }
    }

    // ---- Body ----
    if (ctx->block()) {
        lambda->body = anyToNode<BlockStatementNode>(visitBlock(ctx->block()));
    } else if (ctx->expression()) {
        lambda->body = anyToNode<ExpressionBaseNode>(
            visitExpression(ctx->expression()));
    }

    return std::any(std::static_pointer_cast<ExpressionBaseNode>(lambda));
}

std::any ASTGenerator::visitPipe(MingusParser::PipeContext* ctx) {
    auto left = anyToNode<ExpressionBaseNode>(visitTernary(ctx->ternary()));

    if (ctx->pipeTarget().empty()) {
        return std::any(left);
    }

    auto pipe = std::make_shared<PipeExpression>();
    pipe->debugInfo = makeDebugInfo(ctx);
    pipe->input = left;

    for (auto* targetCtx : ctx->pipeTarget()) {
        PipeStage stage;

        // Build base expression from qualifiedName
        auto nameParts = parseQualifiedName(targetCtx->qualifiedName());
        auto memberIds = targetCtx->Identifier();
        bool hasMemberAccess = !memberIds.empty();

        std::shared_ptr<ExpressionBaseNode> funcExpr;
        if (hasMemberAccess && nameParts.size() == 1) {
            // Single identifier with member access: x |> obj->method
            // Use IdentifierExpression so codegen can load the variable
            auto identExpr = std::make_shared<IdentifierExpression>();
            identExpr->debugInfo = makeDebugInfo(targetCtx->qualifiedName());
            identExpr->name = nameParts[0];
            funcExpr = identExpr;
        } else {
            // Qualified name: x |> func, x |> Enum.Member, etc.
            auto baseExpr = std::make_shared<QualifiedNameExpression>();
            baseExpr->debugInfo = makeDebugInfo(targetCtx->qualifiedName());
            baseExpr->parts = std::move(nameParts);
            funcExpr = baseExpr;
        }

        // Chain member accesses: obj.method or obj->method
        {

            // Pair operators with identifiers by walking children in order
            size_t memberIdx = 0;
            bool isArrow = false;
            for (auto* child : targetCtx->children) {
                auto* termNode = dynamic_cast<antlr4::tree::TerminalNode*>(child);
                if (!termNode) continue;
                auto tokenType = termNode->getSymbol()->getType();
                if (tokenType == MingusParser::DotOperator) {
                    isArrow = false;
                } else if (tokenType == MingusParser::ReferenceAccessOperator) {
                    isArrow = true;
                } else if (tokenType == MingusParser::Identifier) {
                    // Check if this is one of our member identifiers (not qualifiedName)
                    bool isMember = false;
                    for (auto* id : memberIds) {
                        if (id == termNode) { isMember = true; break; }
                    }
                    if (isMember) {
                        auto memberAccess = std::make_shared<MemberAccessExpression>();
                        memberAccess->debugInfo = makeDebugInfo(targetCtx);
                        memberAccess->object = funcExpr;
                        memberAccess->memberName = termNode->getText();
                        memberAccess->isArrow = isArrow;
                        funcExpr = memberAccess;
                    }
                }
            }
        }
        stage.function = funcExpr;

        if (targetCtx->callArguments() &&
            targetCtx->callArguments()->argumentList()) {
            for (auto* argCtx :
                 targetCtx->callArguments()->argumentList()->expression()) {
                auto arg = anyToNode<ExpressionBaseNode>(
                    visitExpression(argCtx));
                if (arg) stage.extraArguments.push_back(arg);
            }
        }

        pipe->stages.push_back(stage);
    }

    return std::any(std::static_pointer_cast<ExpressionBaseNode>(pipe));
}

std::any ASTGenerator::visitTernary(MingusParser::TernaryContext* ctx) {
    auto cond = anyToNode<ExpressionBaseNode>(visitLogicOr(ctx->logicOr()));

    if (ctx->QuestionMarkOperator()) {
        auto ternary = std::make_shared<TernaryExpression>();
        ternary->debugInfo = makeDebugInfo(ctx);
        ternary->condition = cond;
        ternary->thenExpr = anyToNode<ExpressionBaseNode>(
            visitExpression(ctx->expression(0)));
        ternary->elseExpr = anyToNode<ExpressionBaseNode>(
            visitExpression(ctx->expression(1)));
        return std::any(
            std::static_pointer_cast<ExpressionBaseNode>(ternary));
    }

    return std::any(cond);
}

//--------------------------------------------------------------------------------
// Binary operators — left-associative chaining
//--------------------------------------------------------------------------------

std::any ASTGenerator::visitLogicOr(MingusParser::LogicOrContext* ctx) {
    auto children = ctx->logicAnd();
    if (children.size() == 1) return visitLogicAnd(children[0]);

    auto left = anyToNode<ExpressionBaseNode>(visitLogicAnd(children[0]));
    for (size_t i = 1; i < children.size(); ++i) {
        auto right = anyToNode<ExpressionBaseNode>(visitLogicAnd(children[i]));
        auto bin = std::make_shared<BinaryExpression>();
        bin->debugInfo = makeDebugInfo(ctx);
        bin->left  = left;
        bin->op    = BinaryOp::LogicalOr;
        bin->right = right;
        left = bin;
    }
    return std::any(std::static_pointer_cast<ExpressionBaseNode>(left));
}

std::any ASTGenerator::visitLogicAnd(MingusParser::LogicAndContext* ctx) {
    auto children = ctx->bitwiseOr();
    if (children.size() == 1) return visitBitwiseOr(children[0]);

    auto left = anyToNode<ExpressionBaseNode>(visitBitwiseOr(children[0]));
    for (size_t i = 1; i < children.size(); ++i) {
        auto right = anyToNode<ExpressionBaseNode>(
            visitBitwiseOr(children[i]));
        auto bin = std::make_shared<BinaryExpression>();
        bin->debugInfo = makeDebugInfo(ctx);
        bin->left  = left;
        bin->op    = BinaryOp::LogicalAnd;
        bin->right = right;
        left = bin;
    }
    return std::any(std::static_pointer_cast<ExpressionBaseNode>(left));
}

std::any ASTGenerator::visitBitwiseOr(MingusParser::BitwiseOrContext* ctx) {
    auto children = ctx->bitwiseXor();
    if (children.size() == 1) return visitBitwiseXor(children[0]);

    auto left = anyToNode<ExpressionBaseNode>(visitBitwiseXor(children[0]));
    for (size_t i = 1; i < children.size(); ++i) {
        auto right = anyToNode<ExpressionBaseNode>(
            visitBitwiseXor(children[i]));
        auto bin = std::make_shared<BinaryExpression>();
        bin->debugInfo = makeDebugInfo(ctx);
        bin->left  = left;
        bin->op    = BinaryOp::BitwiseOr;
        bin->right = right;
        left = bin;
    }
    return std::any(std::static_pointer_cast<ExpressionBaseNode>(left));
}

std::any ASTGenerator::visitBitwiseXor(MingusParser::BitwiseXorContext* ctx) {
    auto children = ctx->bitwiseAnd();
    if (children.size() == 1) return visitBitwiseAnd(children[0]);

    auto left = anyToNode<ExpressionBaseNode>(visitBitwiseAnd(children[0]));
    for (size_t i = 1; i < children.size(); ++i) {
        auto right = anyToNode<ExpressionBaseNode>(
            visitBitwiseAnd(children[i]));
        auto bin = std::make_shared<BinaryExpression>();
        bin->debugInfo = makeDebugInfo(ctx);
        bin->left  = left;
        bin->op    = BinaryOp::BitwiseXor;
        bin->right = right;
        left = bin;
    }
    return std::any(std::static_pointer_cast<ExpressionBaseNode>(left));
}

std::any ASTGenerator::visitBitwiseAnd(MingusParser::BitwiseAndContext* ctx) {
    auto children = ctx->equality();
    if (children.size() == 1) return visitEquality(children[0]);

    auto left = anyToNode<ExpressionBaseNode>(visitEquality(children[0]));
    for (size_t i = 1; i < children.size(); ++i) {
        auto right = anyToNode<ExpressionBaseNode>(visitEquality(children[i]));
        auto bin = std::make_shared<BinaryExpression>();
        bin->debugInfo = makeDebugInfo(ctx);
        bin->left  = left;
        bin->op    = BinaryOp::BitwiseAnd;
        bin->right = right;
        left = bin;
    }
    return std::any(std::static_pointer_cast<ExpressionBaseNode>(left));
}

std::any ASTGenerator::visitEquality(MingusParser::EqualityContext* ctx) {
    auto children = ctx->relational();
    if (children.size() == 1) return visitRelational(children[0]);

    auto left = anyToNode<ExpressionBaseNode>(visitRelational(children[0]));
    for (size_t i = 1; i < children.size(); ++i) {
        auto right = anyToNode<ExpressionBaseNode>(
            visitRelational(children[i]));
        auto* opNode = dynamic_cast<antlr4::tree::TerminalNode*>(
            ctx->children[2 * (i - 1) + 1]);
        BinaryOp op = (opNode && opNode->getText() == "==")
            ? BinaryOp::Equal : BinaryOp::NotEqual;

        auto bin = std::make_shared<BinaryExpression>();
        bin->debugInfo = makeDebugInfo(ctx);
        bin->left  = left;
        bin->op    = op;
        bin->right = right;
        left = bin;
    }
    return std::any(std::static_pointer_cast<ExpressionBaseNode>(left));
}

std::any ASTGenerator::visitRelational(MingusParser::RelationalContext* ctx) {
    auto children = ctx->shift();
    if (children.size() == 1) return visitShift(children[0]);

    auto left = anyToNode<ExpressionBaseNode>(visitShift(children[0]));
    for (size_t i = 1; i < children.size(); ++i) {
        auto right = anyToNode<ExpressionBaseNode>(visitShift(children[i]));
        auto* opNode = dynamic_cast<antlr4::tree::TerminalNode*>(
            ctx->children[2 * (i - 1) + 1]);
        BinaryOp op = BinaryOp::Less;
        if (opNode) {
            std::string opText = opNode->getText();
            if      (opText == "<=") op = BinaryOp::LessEqual;
            else if (opText == ">")  op = BinaryOp::Greater;
            else if (opText == ">=") op = BinaryOp::GreaterEqual;
        }

        auto bin = std::make_shared<BinaryExpression>();
        bin->debugInfo = makeDebugInfo(ctx);
        bin->left  = left;
        bin->op    = op;
        bin->right = right;
        left = bin;
    }
    return std::any(std::static_pointer_cast<ExpressionBaseNode>(left));
}

std::any ASTGenerator::visitShift(MingusParser::ShiftContext* ctx) {
    auto children = ctx->additive();
    if (children.size() == 1) return visitAdditive(children[0]);

    auto left = anyToNode<ExpressionBaseNode>(visitAdditive(children[0]));
    for (size_t i = 1; i < children.size(); ++i) {
        auto right = anyToNode<ExpressionBaseNode>(visitAdditive(children[i]));
        auto* opNode = dynamic_cast<antlr4::tree::TerminalNode*>(
            ctx->children[2 * (i - 1) + 1]);
        BinaryOp op = (opNode && opNode->getText() == "<<")
            ? BinaryOp::ShiftLeft : BinaryOp::ShiftRight;

        auto bin = std::make_shared<BinaryExpression>();
        bin->debugInfo = makeDebugInfo(ctx);
        bin->left  = left;
        bin->op    = op;
        bin->right = right;
        left = bin;
    }
    return std::any(std::static_pointer_cast<ExpressionBaseNode>(left));
}

std::any ASTGenerator::visitAdditive(MingusParser::AdditiveContext* ctx) {
    auto children = ctx->multiplicative();
    if (children.size() == 1) return visitMultiplicative(children[0]);

    auto left = anyToNode<ExpressionBaseNode>(
        visitMultiplicative(children[0]));
    for (size_t i = 1; i < children.size(); ++i) {
        auto right = anyToNode<ExpressionBaseNode>(
            visitMultiplicative(children[i]));
        auto* opNode = dynamic_cast<antlr4::tree::TerminalNode*>(
            ctx->children[2 * (i - 1) + 1]);
        BinaryOp op = (opNode && opNode->getText() == "+")
            ? BinaryOp::Add : BinaryOp::Sub;

        auto bin = std::make_shared<BinaryExpression>();
        bin->debugInfo = makeDebugInfo(ctx);
        bin->left  = left;
        bin->op    = op;
        bin->right = right;
        left = bin;
    }
    return std::any(std::static_pointer_cast<ExpressionBaseNode>(left));
}

std::any ASTGenerator::visitMultiplicative(
    MingusParser::MultiplicativeContext* ctx)
{
    auto children = ctx->castExpression();
    if (children.size() == 1) return visitCastExpression(children[0]);

    auto left = anyToNode<ExpressionBaseNode>(
        visitCastExpression(children[0]));
    for (size_t i = 1; i < children.size(); ++i) {
        auto right = anyToNode<ExpressionBaseNode>(
            visitCastExpression(children[i]));
        auto* opNode = dynamic_cast<antlr4::tree::TerminalNode*>(
            ctx->children[2 * (i - 1) + 1]);
        BinaryOp op = BinaryOp::Mul;
        if (opNode) {
            std::string opText = opNode->getText();
            if      (opText == "/") op = BinaryOp::Div;
            else if (opText == "%") op = BinaryOp::Mod;
        }

        auto bin = std::make_shared<BinaryExpression>();
        bin->debugInfo = makeDebugInfo(ctx);
        bin->left  = left;
        bin->op    = op;
        bin->right = right;
        left = bin;
    }
    return std::any(std::static_pointer_cast<ExpressionBaseNode>(left));
}

//--------------------------------------------------------------------------------
// Cast, Unary, Postfix, Primary
//--------------------------------------------------------------------------------

std::any ASTGenerator::visitCastExpression(
    MingusParser::CastExpressionContext* ctx)
{
    if (ctx->typeIdentifier()) {
        auto cast = std::make_shared<CastExpression>();
        cast->debugInfo = makeDebugInfo(ctx);
        cast->targetType = anyToNode<TypeNode>(
            visitTypeIdentifier(ctx->typeIdentifier()));
        cast->operand = anyToNode<ExpressionBaseNode>(
            visitCastExpression(ctx->castExpression()));
        return std::any(std::static_pointer_cast<ExpressionBaseNode>(cast));
    }
    return visitUnaryExpression(ctx->unaryExpression());
}

std::any ASTGenerator::visitUnaryExpression(
    MingusParser::UnaryExpressionContext* ctx)
{
    // move(expr) — MoveExpression
    if (ctx->MoveKeyword()) {
        auto moveExpr = std::make_shared<MoveExpression>();
        moveExpr->debugInfo = makeDebugInfo(ctx);
        moveExpr->operand = anyToNode<ExpressionBaseNode>(
            visitExpression(ctx->expression()));
        return std::any(std::static_pointer_cast<ExpressionBaseNode>(moveExpr));
    }

    if (ctx->prefixOperator()) {
        auto unary = std::make_shared<UnaryExpression>();
        unary->debugInfo = makeDebugInfo(ctx);
        unary->op = parseUnaryOperator(ctx->prefixOperator()->getText());
        unary->operand = anyToNode<ExpressionBaseNode>(
            visitUnaryExpression(ctx->unaryExpression()));
        return std::any(std::static_pointer_cast<ExpressionBaseNode>(unary));
    }

    if (ctx->incrementDecrementOperator()) {
        std::string text = ctx->incrementDecrementOperator()->getText();
        auto unary = std::make_shared<UnaryExpression>();
        unary->debugInfo = makeDebugInfo(ctx);
        unary->op = (text == "++") ? UnaryOp::PreIncrement
                                   : UnaryOp::PreDecrement;
        unary->operand = anyToNode<ExpressionBaseNode>(
            visitUnaryExpression(ctx->unaryExpression()));
        return std::any(std::static_pointer_cast<ExpressionBaseNode>(unary));
    }

    if (ctx->typeSizeOrAlign()) {
        auto sizeOf = std::make_shared<SizeOfExpression>();
        sizeOf->debugInfo = makeDebugInfo(ctx);
        sizeOf->targetType = anyToNode<TypeNode>(
            visitTypeIdentifier(ctx->typeIdentifier()));
        return std::any(std::static_pointer_cast<ExpressionBaseNode>(sizeOf));
    }

    return visitPostfixExpression(ctx->postfixExpression());
}

std::any ASTGenerator::visitPostfixExpression(
    MingusParser::PostfixExpressionContext* ctx)
{
    auto result = anyToNode<ExpressionBaseNode>(
        visitPrimaryExpression(ctx->primaryExpression()));

    for (auto* opCtx : ctx->postfixOperation()) {
        if (opCtx->TurbofishOperator()) {
            // Generic function call with turbofish: foo::<int, double>(args)
            auto call = std::make_shared<CallExpression>();
            call->debugInfo = makeDebugInfo(opCtx);
            call->callee = result;

            // Parse type arguments
            for (auto* typeIdCtx : opCtx->typeIdentifier()) {
                auto typeArg = anyToNode<TypeNode>(visitTypeIdentifier(typeIdCtx));
                if (typeArg) call->typeArguments.push_back(typeArg);
            }

            // Parse call arguments
            auto argsNode = std::make_shared<ArgumentsNode>();
            argsNode->debugInfo = makeDebugInfo(opCtx);
            if (opCtx->callArguments() && opCtx->callArguments()->argumentList()) {
                for (auto* argExpr :
                     opCtx->callArguments()->argumentList()->expression()) {
                    auto arg = anyToNode<ExpressionBaseNode>(
                        visitExpression(argExpr));
                    if (arg) argsNode->expressions.push_back(arg);
                }
            }
            call->arguments = argsNode;
            result = call;

        } else if (opCtx->callArguments()) {
            // Function call: callee(args)
            auto argsNode = std::make_shared<ArgumentsNode>();
            argsNode->debugInfo = makeDebugInfo(opCtx);
            if (opCtx->callArguments()->argumentList()) {
                for (auto* argExpr :
                     opCtx->callArguments()->argumentList()->expression()) {
                    auto arg = anyToNode<ExpressionBaseNode>(
                        visitExpression(argExpr));
                    if (arg) argsNode->expressions.push_back(arg);
                }
            }

            auto call = std::make_shared<CallExpression>();
            call->debugInfo = makeDebugInfo(opCtx);
            call->callee = result;
            call->arguments = argsNode;
            result = call;

        } else if (opCtx->elementAccess()) {
            // Index: expr[index]
            auto idx = std::make_shared<IndexExpression>();
            idx->debugInfo = makeDebugInfo(opCtx);
            idx->object = result;
            idx->index = anyToNode<ExpressionBaseNode>(
                visitExpression(opCtx->elementAccess()->expression()));
            result = idx;

        } else if (opCtx->memberAccess()) {
            // Member: expr.member or expr->member
            auto mem = std::make_shared<MemberAccessExpression>();
            mem->debugInfo = makeDebugInfo(opCtx);
            mem->object = result;
            mem->memberName = opCtx->memberAccess()->Identifier()->getText();
            mem->isArrow =
                opCtx->memberAccess()->ReferenceAccessOperator() != nullptr;
            result = mem;

        } else if (opCtx->incrementDecrementOperator()) {
            // Postfix ++/--
            std::string text = opCtx->incrementDecrementOperator()->getText();
            auto unary = std::make_shared<UnaryExpression>();
            unary->debugInfo = makeDebugInfo(opCtx);
            unary->op = (text == "++") ? UnaryOp::PostIncrement
                                       : UnaryOp::PostDecrement;
            unary->operand = result;
            result = unary;
        }
    }

    return std::any(std::static_pointer_cast<ExpressionBaseNode>(result));
}

std::any ASTGenerator::visitPrimaryExpression(
    MingusParser::PrimaryExpressionContext* ctx)
{
    if (ctx->IntegerLiteral()) {
        auto lit = std::make_shared<IntegerLiteral>();
        lit->debugInfo = makeDebugInfo(ctx);
        lit->value = parseIntegerLiteral(ctx->IntegerLiteral()->getText());
        return std::any(std::static_pointer_cast<ExpressionBaseNode>(lit));
    }

    if (ctx->FloatingLiteral()) {
        auto lit = std::make_shared<FloatLiteral>();
        lit->debugInfo = makeDebugInfo(ctx);
        std::string text = ctx->FloatingLiteral()->getText();
        if (!text.empty() && (text.back() == 'f' || text.back() == 'F')) {
            lit->isFloat = true;
            text.pop_back();
        }
        lit->value = std::stod(text);
        return std::any(std::static_pointer_cast<ExpressionBaseNode>(lit));
    }

    if (ctx->BooleanLiteral()) {
        auto lit = std::make_shared<BoolLiteral>();
        lit->debugInfo = makeDebugInfo(ctx);
        lit->value = (ctx->BooleanLiteral()->getText() == "true");
        return std::any(std::static_pointer_cast<ExpressionBaseNode>(lit));
    }

    if (ctx->CharLiteral()) {
        auto lit = std::make_shared<CharLiteral>();
        lit->debugInfo = makeDebugInfo(ctx);
        std::string text = ctx->CharLiteral()->getText();
        lit->value = parseCharLiteral(text);
        return std::any(std::static_pointer_cast<ExpressionBaseNode>(lit));
    }

    if (ctx->string()) {
        return visitString(ctx->string());
    }

    if (ctx->NullReference()) {
        auto lit = std::make_shared<NullLiteral>();
        lit->debugInfo = makeDebugInfo(ctx);
        return std::any(std::static_pointer_cast<ExpressionBaseNode>(lit));
    }

    if (ctx->ThisReference()) {
        auto expr = std::make_shared<ThisExpression>();
        expr->debugInfo = makeDebugInfo(ctx);
        return std::any(std::static_pointer_cast<ExpressionBaseNode>(expr));
    }

    if (ctx->Identifier()) {
        auto expr = std::make_shared<IdentifierExpression>();
        expr->debugInfo = makeDebugInfo(ctx);
        expr->name = ctx->Identifier()->getText();
        return std::any(std::static_pointer_cast<ExpressionBaseNode>(expr));
    }

    if (ctx->tupleExpression()) {
        return visitTupleExpression(ctx->tupleExpression());
    }

    if (ctx->newExpression()) {
        return visitNewExpression(ctx->newExpression());
    }

    if (ctx->matchExpression()) {
        return visitMatchExpression(ctx->matchExpression());
    }

    if (ctx->arrayLiteral()) {
        return visitArrayLiteral(ctx->arrayLiteral());
    }

    if (ctx->expression()) {
        // Parenthesized expression: ( expr )
        return visitExpression(ctx->expression());
    }

    return std::any(std::shared_ptr<ExpressionBaseNode>(nullptr));
}

std::any ASTGenerator::visitArrayLiteral(
    MingusParser::ArrayLiteralContext* ctx)
{
    auto arrLit = std::make_shared<ArrayLiteralExpression>();
    arrLit->debugInfo = makeDebugInfo(ctx);
    for (auto* exprCtx : ctx->expression()) {
        auto elem = anyToNode<ExpressionBaseNode>(visitExpression(exprCtx));
        if (elem) arrLit->elements.push_back(elem);
    }
    return std::any(std::static_pointer_cast<ExpressionBaseNode>(arrLit));
}

std::any ASTGenerator::visitNewExpression(
    MingusParser::NewExpressionContext* ctx)
{
    auto newExpr = std::make_shared<NewExpression>();
    newExpr->debugInfo = makeDebugInfo(ctx);
    newExpr->isShared = (ctx->SharedKeyword() != nullptr);
    newExpr->type = anyToNode<TypeNode>(
        visitTypeIdentifier(ctx->typeIdentifier()));

    if (ctx->SquareBracketLeft()) {
        // Array: new Type[size]
        newExpr->isArray = true;
        newExpr->arraySize = anyToNode<ExpressionBaseNode>(
            visitExpression(ctx->expression()));
    } else {
        // Object: new Type(args)
        auto argsNode = std::make_shared<ArgumentsNode>();
        argsNode->debugInfo = makeDebugInfo(ctx);
        if (ctx->callArguments() && ctx->callArguments()->argumentList()) {
            for (auto* argCtx :
                 ctx->callArguments()->argumentList()->expression()) {
                auto arg = anyToNode<ExpressionBaseNode>(
                    visitExpression(argCtx));
                if (arg) argsNode->expressions.push_back(arg);
            }
        }
        newExpr->arguments = argsNode;
    }

    return std::any(std::static_pointer_cast<ExpressionBaseNode>(newExpr));
}

std::any ASTGenerator::visitTupleExpression(
    MingusParser::TupleExpressionContext* ctx)
{
    auto tuple = std::make_shared<TupleExpression>();
    tuple->debugInfo = makeDebugInfo(ctx);

    for (auto* exprCtx : ctx->expression()) {
        auto elem = anyToNode<ExpressionBaseNode>(visitExpression(exprCtx));
        if (elem) tuple->elements.push_back(elem);
    }

    return std::any(std::static_pointer_cast<ExpressionBaseNode>(tuple));
}

std::any ASTGenerator::visitMatchExpression(
    MingusParser::MatchExpressionContext* ctx)
{
    auto match = std::make_shared<MatchExpression>();
    match->debugInfo = makeDebugInfo(ctx);
    match->subject = anyToNode<ExpressionBaseNode>(
        visitExpression(ctx->expression()));

    for (auto* armCtx : ctx->matchArm()) {
        MatchArm arm;
        arm.pattern = anyToNode<PatternNode>(
            visitPattern(armCtx->pattern()));

        if (armCtx->matchBody()->expression()) {
            arm.body = anyToNode<ExpressionBaseNode>(
                visitExpression(armCtx->matchBody()->expression()));
        } else if (armCtx->matchBody()->block()) {
            arm.body = anyToNode<BlockStatementNode>(
                visitBlock(armCtx->matchBody()->block()));
        }

        match->arms.push_back(arm);
    }

    return std::any(std::static_pointer_cast<ExpressionBaseNode>(match));
}

std::any ASTGenerator::visitMatchArm(MingusParser::MatchArmContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTGenerator::visitCallArguments(
    MingusParser::CallArgumentsContext* ctx)
{
    return visitChildren(ctx);
}

std::any ASTGenerator::visitElementAccess(
    MingusParser::ElementAccessContext* ctx)
{
    return visitChildren(ctx);
}

std::any ASTGenerator::visitMemberAccess(
    MingusParser::MemberAccessContext* ctx)
{
    return visitChildren(ctx);
}

//================================================================================
// Pattern Visitors
//================================================================================

std::any ASTGenerator::visitPattern(MingusParser::PatternContext* ctx) {
    return visitGuardedPattern(ctx->guardedPattern());
}

std::any ASTGenerator::visitGuardedPattern(
    MingusParser::GuardedPatternContext* ctx)
{
    auto* baseCtx = ctx->basePattern();

    // Dispatch to the specific pattern alternative
    std::shared_ptr<PatternNode> pattern;
    if (baseCtx->literalPattern()) {
        pattern = anyToNode<PatternNode>(
            visitLiteralPattern(baseCtx->literalPattern()));
    } else if (baseCtx->rangePattern()) {
        pattern = anyToNode<PatternNode>(
            visitRangePattern(baseCtx->rangePattern()));
    } else if (baseCtx->wildcardPattern()) {
        pattern = anyToNode<PatternNode>(
            visitWildcardPattern(baseCtx->wildcardPattern()));
    } else if (baseCtx->bindingPattern()) {
        pattern = anyToNode<PatternNode>(
            visitBindingPattern(baseCtx->bindingPattern()));
    } else if (baseCtx->tuplePattern()) {
        pattern = anyToNode<PatternNode>(
            visitTuplePattern(baseCtx->tuplePattern()));
    }

    // Guard: only IdentifierPattern supports guards in V2
    if (pattern && ctx->ControlFlowIf() && ctx->expression()) {
        auto guard = anyToNode<ExpressionBaseNode>(
            visitExpression(ctx->expression()));
        if (auto* idPat = dynamic_cast<IdentifierPattern*>(pattern.get())) {
            idPat->guard = guard;
        }
    }

    return std::any(pattern);
}

std::any ASTGenerator::visitLiteralPattern(
    MingusParser::LiteralPatternContext* ctx)
{
    auto pat = std::make_shared<LiteralPattern>();
    pat->debugInfo = makeDebugInfo(ctx);

    if (ctx->IntegerLiteral()) {
        auto lit = std::make_shared<IntegerLiteral>();
        lit->debugInfo = makeDebugInfo(ctx);
        lit->value = parseIntegerLiteral(ctx->IntegerLiteral()->getText());
        pat->value = lit;

    } else if (ctx->FloatingLiteral()) {
        auto lit = std::make_shared<FloatLiteral>();
        lit->debugInfo = makeDebugInfo(ctx);
        std::string text = ctx->FloatingLiteral()->getText();
        if (!text.empty() && (text.back() == 'f' || text.back() == 'F')) {
            lit->isFloat = true;
            text.pop_back();
        }
        lit->value = std::stod(text);
        pat->value = lit;

    } else if (ctx->BooleanLiteral()) {
        auto lit = std::make_shared<BoolLiteral>();
        lit->debugInfo = makeDebugInfo(ctx);
        lit->value = (ctx->BooleanLiteral()->getText() == "true");
        pat->value = lit;

    } else if (ctx->CharLiteral()) {
        auto lit = std::make_shared<CharLiteral>();
        lit->debugInfo = makeDebugInfo(ctx);
        std::string text = ctx->CharLiteral()->getText();
        lit->value = parseCharLiteral(text);
        pat->value = lit;

    } else if (ctx->string()) {
        pat->value = anyToNode<ExpressionBaseNode>(visitString(ctx->string()));

    } else if (ctx->NullReference()) {
        auto lit = std::make_shared<NullLiteral>();
        lit->debugInfo = makeDebugInfo(ctx);
        pat->value = lit;

    } else if (ctx->qualifiedName()) {
        // Check if this is a variant pattern: qualifiedName ( variantPatternField, ... )
        if (ctx->OpeningRoundBracket()) {
            auto vp = std::make_shared<VariantPattern>();
            vp->debugInfo = makeDebugInfo(ctx);
            vp->variantPath = parseQualifiedName(ctx->qualifiedName());
            for (auto* fieldCtx : ctx->variantPatternField()) {
                auto fieldPat = anyToNode<PatternNode>(
                    visitVariantPatternField(fieldCtx));
                if (fieldPat) vp->fieldPatterns.push_back(fieldPat);
            }
            return std::any(std::static_pointer_cast<PatternNode>(vp));
        }

        // Plain qualified name literal pattern
        auto qname = std::make_shared<QualifiedNameExpression>();
        qname->debugInfo = makeDebugInfo(ctx);
        qname->parts = parseQualifiedName(ctx->qualifiedName());
        pat->value = qname;
    }

    return std::any(std::static_pointer_cast<PatternNode>(pat));
}

std::any ASTGenerator::visitRangePattern(
    MingusParser::RangePatternContext* ctx)
{
    auto pat = std::make_shared<RangePattern>();
    pat->debugInfo = makeDebugInfo(ctx);

    auto low = std::make_shared<IntegerLiteral>();
    low->debugInfo = makeDebugInfo(ctx);
    low->value = parseIntegerLiteral(ctx->IntegerLiteral(0)->getText());
    pat->low = low;

    auto high = std::make_shared<IntegerLiteral>();
    high->debugInfo = makeDebugInfo(ctx);
    high->value = parseIntegerLiteral(ctx->IntegerLiteral(1)->getText());
    pat->high = high;

    return std::any(std::static_pointer_cast<PatternNode>(pat));
}

std::any ASTGenerator::visitWildcardPattern(
    MingusParser::WildcardPatternContext* ctx)
{
    auto pat = std::make_shared<WildcardPattern>();
    pat->debugInfo = makeDebugInfo(ctx);
    return std::any(std::static_pointer_cast<PatternNode>(pat));
}

std::any ASTGenerator::visitBindingPattern(
    MingusParser::BindingPatternContext* ctx)
{
    // V2 maps BindingPattern → IdentifierPattern
    auto pat = std::make_shared<IdentifierPattern>();
    pat->debugInfo = makeDebugInfo(ctx);
    pat->name = ctx->Identifier()->getText();
    return std::any(std::static_pointer_cast<PatternNode>(pat));
}

std::any ASTGenerator::visitTuplePattern(
    MingusParser::TuplePatternContext* ctx)
{
    // V2 AST does not yet have a TuplePattern node
    reportError(ctx, "Tuple patterns not yet supported in V2");
    return std::any(std::shared_ptr<PatternNode>(nullptr));
}

std::any ASTGenerator::visitVariantPatternField(
    MingusParser::VariantPatternFieldContext* ctx)
{
    if (ctx->bindingPattern()) {
        return visitBindingPattern(ctx->bindingPattern());
    } else if (ctx->wildcardPattern()) {
        return visitWildcardPattern(ctx->wildcardPattern());
    } else if (ctx->literalPattern()) {
        return visitLiteralPattern(ctx->literalPattern());
    }
    return std::any(std::shared_ptr<PatternNode>(nullptr));
}

} // namespace parser
} // namespace mingus
