//================================================================================
// MINGUS v1 - Program Structure Nodes
// ProgramNode, ModuleNode, ImportNode
//================================================================================

#pragma once

#include "mingus/ast/ASTNode.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mingus {
namespace ast {

// Forward declarations
class DeclarationNode;
class ImportNode;
class ModuleNode;

//================================================================================
// QualifiedName - resolves to a symbol during name resolution
//================================================================================
struct QualifiedName {
    std::vector<std::string> parts;

    QualifiedName() = default;
    explicit QualifiedName(std::vector<std::string> p) : parts(std::move(p)) {}
    explicit QualifiedName(const std::string& single) : parts{single} {}

    bool isSimple() const { return parts.size() == 1; }
    const std::string& getSimpleName() const { return parts.empty() ? empty_ : parts.back(); }
    
    std::string toString() const {
        std::string result;
        for (size_t i = 0; i < parts.size(); ++i) {
            if (i > 0) result += ".";
            result += parts[i];
        }
        return result;
    }

    bool empty() const { return parts.empty(); }

private:
    static const std::string empty_;
};

//================================================================================
// ImportTarget - single item in an import statement
//================================================================================
struct ImportTarget {
    std::string name;
    std::optional<std::string> alias;

    ImportTarget(const std::string& n, std::optional<std::string> a = std::nullopt)
        : name(n), alias(a) {}

    std::string getEffectiveName() const {
        return alias.value_or(name);
    }
};

//================================================================================
// ImportNode - import statements
//================================================================================
class ImportNode : public ASTNode {
public:
    std::vector<ImportTarget> targets;
    QualifiedName source;  // Empty for "import X", filled for "from X import Y"

    // import module.name
    explicit ImportNode(QualifiedName src,
                        const SourceLocation& loc = SourceLocation())
        : ASTNode(loc), source(std::move(src)) {}

    // from module import target1, target2
    ImportNode(QualifiedName src, std::vector<ImportTarget> tgts,
               const SourceLocation& loc = SourceLocation())
        : ASTNode(loc), targets(std::move(tgts)), source(std::move(src)) {}

    bool isFromImport() const { return !source.empty() && !targets.empty(); }
    bool isModuleImport() const { return !source.empty() && targets.empty(); }

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// ModuleNode - a single module containing declarations
//================================================================================
class ModuleNode : public ASTNode {
public:
    std::string name;
    NodeList<ImportNode> imports;
    NodeList<DeclarationNode> declarations;

    ModuleNode(const std::string& n, 
               NodeList<ImportNode> imps,
               NodeList<DeclarationNode> decls,
               const SourceLocation& loc = SourceLocation())
        : ASTNode(loc), name(n), imports(std::move(imps)), declarations(std::move(decls)) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

//================================================================================
// ProgramNode - root of the AST
//================================================================================
class ProgramNode : public ASTNode {
public:
    NodeList<ModuleNode> modules;

    explicit ProgramNode(NodeList<ModuleNode> mods,
                         const SourceLocation& loc = SourceLocation())
        : ASTNode(loc), modules(std::move(mods)) {}

    void accept(ASTVisitor& visitor) override;
    void accept(ConstASTVisitor& visitor) const override;
};

} // namespace ast
} // namespace mingus
