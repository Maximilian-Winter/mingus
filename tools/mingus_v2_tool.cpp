//================================================================================
// MINGUS V2 - Compiler Tool
//
// Entry point that orchestrates the full compilation pipeline:
//   1. ANTLR4 lexer/parser
//   2. ASTGenerator (parse tree -> V2 AST)
//   3. Import resolution (recursive .mingus discovery)
//   4. SymbolTableBuilder (Pass 1)
//   5. TypeResolver       (Pass 2)
//   6. TypeChecker         (Pass 3)
//   7. SemanticValidator   (Pass 4)
//   8. IRGenerator         (LLVM codegen)
//   9. LLVM verification + optional optimization
//  10. Optional: emit .ll, compile with clang, execute
//================================================================================

// Parser
#include "mingus/parser/ASTGenerator.h"
#include "MingusLexer.h"
#include "MingusParser.h"

// Core
#include "mingus/AstNode.h"
#include "mingus/Expressions.h"
#include "mingus/Statements.h"
#include "mingus/Declarations.h"
#include "mingus/SymbolTable.h"

// Sema passes
#include "mingus/sema/ErrorReporter.h"
#include "mingus/sema/SymbolTableBuilder.h"
#include "mingus/sema/TypeResolver.h"
#include "mingus/sema/TypeChecker.h"
#include "mingus/sema/SemanticValidator.h"

// Codegen
#include "mingus/codegen/IRGenerator.h"

// ANTLR4
#include "antlr4-runtime.h"

// LLVM
#pragma warning(push, 0)
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar/GVN.h>
#pragma warning(pop)

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace AntlrMingusParser;
namespace fs = std::filesystem;

//================================================================================
// ANTLR4 Error Listener
//================================================================================

class VerboseErrorListener : public antlr4::BaseErrorListener {
public:
    bool hasErrors = false;

    void syntaxError(antlr4::Recognizer* /*recognizer*/,
                     antlr4::Token* offendingSymbol,
                     size_t line, size_t charPositionInLine,
                     const std::string& msg,
                     std::exception_ptr /*e*/) override
    {
        hasErrors = true;
        std::cerr << "syntax error at " << line << ":" << charPositionInLine;
        if (offendingSymbol) {
            std::cerr << " near '" << offendingSymbol->getText() << "'";
        }
        std::cerr << ": " << msg << "\n";
    }
};

//================================================================================
// File I/O
//================================================================================

static std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "error: cannot open file '" << path << "'\n";
        return "";
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

//================================================================================
// Parse a single .mingus file into a ProgramNode
//================================================================================

static std::shared_ptr<mingus::ProgramNode> parseFile(
    const std::string& source, const std::string& fileName)
{
    antlr4::ANTLRInputStream input(source);
    MingusLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    MingusParser parser(&tokens);

    // Error handling
    parser.removeErrorListeners();
    VerboseErrorListener errorListener;
    parser.addErrorListener(&errorListener);

    auto* tree = parser.program();

    if (errorListener.hasErrors) {
        std::cerr << "error: parse errors in '" << fileName << "'\n";
        return nullptr;
    }

    mingus::parser::ASTGenerator generator;
    auto program = generator.generate(tree);

    if (generator.hasErrors()) {
        std::cerr << "error: AST generation errors in '" << fileName << "':\n";
        for (const auto& err : generator.errors) {
            std::cerr << "  " << err.line << ":" << err.column
                      << ": " << err.message << "\n";
        }
        return nullptr;
    }

    return program;
}

//================================================================================
// Import Resolution — recursively discover and parse imported modules
//================================================================================

static bool resolveImports(
    std::shared_ptr<mingus::ProgramNode>& program,
    const std::string& sourceDir,
    std::set<std::string>& loadedFiles)
{
    bool changed = true;
    while (changed) {
        changed = false;

        // Collect import source paths from all modules
        std::vector<std::string> needed;
        for (auto& mod : program->modules) {
            for (auto& decl : mod->declarations) {
                if (auto imp = std::dynamic_pointer_cast<mingus::ImportDeclaration>(decl)) {
                    if (!imp->sourcePath.empty()) {
                        std::string moduleName = imp->sourcePath.back();
                        if (loadedFiles.find(moduleName) == loadedFiles.end()) {
                            needed.push_back(moduleName);
                        }
                    }
                }
            }
        }

        for (const auto& moduleName : needed) {
            // Look for ModuleName.mingus in source directory
            std::string filePath = sourceDir + "/" + moduleName + ".mingus";
            if (!fs::exists(filePath)) {
                std::cerr << "error: cannot find imported module '"
                          << moduleName << "' (expected at " << filePath << ")\n";
                return false;
            }

            std::string source = readFile(filePath);
            if (source.empty()) return false;

            auto imported = parseFile(source, filePath);
            if (!imported) return false;

            // Merge imported modules into main program
            for (auto& mod : imported->modules) {
                program->modules.push_back(mod);
            }

            loadedFiles.insert(moduleName);
            changed = true;
        }
    }

    return true;
}

//================================================================================
// Inject C main() wrapper that calls the entry function
//================================================================================

static void injectMainWrapper(
    llvm::Module& module,
    llvm::LLVMContext& context,
    const std::string& entryFunc)
{
    auto* callee = module.getFunction(entryFunc);
    if (!callee) {
        std::cerr << "warning: entry function '" << entryFunc
                  << "' not found in module\n";
        return;
    }

    // Create main() -> i32
    auto* i32Type = llvm::Type::getInt32Ty(context);
    auto* mainType = llvm::FunctionType::get(i32Type, false);
    auto* mainFn = llvm::Function::Create(
        mainType, llvm::Function::ExternalLinkage, "main", module);

    auto* bb = llvm::BasicBlock::Create(context, "entry", mainFn);
    llvm::IRBuilder<> builder(bb);

    auto* result = builder.CreateCall(callee);

    // If entry returns int, use it as exit code; otherwise return 0
    if (callee->getReturnType() == i32Type) {
        builder.CreateRet(result);
    } else {
        builder.CreateRet(llvm::ConstantInt::get(i32Type, 0));
    }
}

//================================================================================
// LLVM Optimization Pipeline
//================================================================================

static void optimizeModule(llvm::Module& module, int optLevel) {
    if (optLevel <= 0) return;

    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;

    llvm::PassBuilder PB;
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    llvm::OptimizationLevel level;
    if (optLevel == 1) {
        level = llvm::OptimizationLevel::O1;
    } else {
        level = llvm::OptimizationLevel::O2;
    }

    auto MPM = PB.buildPerModuleDefaultPipeline(level);
    MPM.run(module, MAM);
}

//================================================================================
// Find clang executable
//================================================================================

static std::string findClang() {
    // Try relative to this executable (sibling in LLVM distribution)
    auto exePath = fs::current_path();

    // Check common locations
    std::vector<std::string> candidates = {
        "clang", "clang.exe",
        (exePath / "clang.exe").string(),
        (exePath / ".." / "bin" / "clang.exe").string(),
    };

    // Also check the LLVM we built against
    std::string llvmClang = (fs::path(
        "H:/language_dev/mingus/extern/clang+llvm-21.1.8-x86_64-pc-windows-msvc/bin/clang.exe"
    )).string();
    candidates.push_back(llvmClang);

    for (const auto& c : candidates) {
        if (fs::exists(c)) return c;
    }

    // Fall back to PATH
    return "clang";
}

//================================================================================
// Main
//================================================================================

int main(int argc, char* argv[]) {
    // ---- Parse command line ----
    if (argc < 2) {
        std::cerr << "usage: mingus_v2_tool <source.mingus> [options]\n"
                  << "  --emit <out.ll>   Write LLVM IR to file\n"
                  << "  --entry <func>    Inject C main() wrapper calling <func>\n"
                  << "  --run <func>      Compile and run (implies --entry)\n"
                  << "  --opt <0|1|2>     Optimization level (default: 0)\n";
        return 1;
    }

    std::string sourceFile = argv[1];
    std::string emitFile;
    std::string entryFunc;
    std::string runFunc;
    int optLevel = 0;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--emit" && i + 1 < argc) {
            emitFile = argv[++i];
        } else if (arg == "--entry" && i + 1 < argc) {
            entryFunc = argv[++i];
        } else if (arg == "--run" && i + 1 < argc) {
            runFunc = argv[++i];
            entryFunc = runFunc;  // --run implies --entry
        } else if (arg == "--opt" && i + 1 < argc) {
            optLevel = std::stoi(argv[++i]);
        }
    }

    // ---- Read source file ----
    std::string source = readFile(sourceFile);
    if (source.empty()) return 1;

    // ---- Parse ----
    auto program = parseFile(source, sourceFile);
    if (!program) return 1;

    // ---- Resolve imports ----
    std::string sourceDir = fs::path(sourceFile).parent_path().string();
    if (sourceDir.empty()) sourceDir = ".";

    std::set<std::string> loadedFiles;
    // Mark the primary file's modules as loaded
    for (auto& mod : program->modules) {
        loadedFiles.insert(mod->name);
    }

    if (!resolveImports(program, sourceDir, loadedFiles)) {
        return 1;
    }

    // ---- Semantic Analysis (4 passes) ----
    mingus::SymbolTable symbolTable;
    mingus::ErrorReporter errors;

    // Pass 1: Build scope tree and symbols
    mingus::SymbolTableBuilder pass1(symbolTable, errors);
    pass1.build(*program);
    if (errors.hasErrors()) {
        std::cerr << "Pass 1 (SymbolTableBuilder) errors:\n";
        errors.dump();
        return 1;
    }

    // Pass 2: Resolve types
    mingus::TypeResolver pass2(symbolTable, errors);
    pass2.resolve(*program);
    if (errors.hasErrors()) {
        std::cerr << "Pass 2 (TypeResolver) errors:\n";
        errors.dump();
        return 1;
    }

    // Pass 3: Type checking
    mingus::TypeChecker pass3(symbolTable, errors);
    pass3.check(*program);
    if (errors.hasErrors()) {
        std::cerr << "Pass 3 (TypeChecker) errors:\n";
        errors.dump();
        return 1;
    }

    // Pass 4: Semantic validation + RAII analysis
    mingus::SemanticValidator pass4(symbolTable, errors);
    pass4.validate(*program);
    if (errors.hasErrors()) {
        std::cerr << "Pass 4 (SemanticValidator) errors:\n";
        errors.dump();
        return 1;
    }

    // Emit warnings (if any) even when no errors
    for (const auto& d : errors.diagnostics()) {
        if (d.level == mingus::DiagnosticLevel::Warning) {
            std::cerr << "warning: ";
            if (d.location) std::cerr << d.location->toString() << ": ";
            std::cerr << d.message << "\n";
        }
    }

    // ---- IR Generation ----
    mingus::codegen::IRGenerator irgen(symbolTable, pass4.getRAIIInfo());
    auto llvmModule = irgen.generate(*program);
    if (!llvmModule) {
        std::cerr << "error: IR generation failed\n";
        return 1;
    }

    // ---- Main wrapper injection ----
    if (!entryFunc.empty()) {
        injectMainWrapper(*llvmModule, irgen.getContext(), entryFunc);
    }

    // ---- Optimization ----
    optimizeModule(*llvmModule, optLevel);

    // ---- LLVM Verification ----
    std::string verifyErrors;
    llvm::raw_string_ostream verifyStream(verifyErrors);
    if (llvm::verifyModule(*llvmModule, &verifyStream)) {
        std::cerr << "LLVM verification failed:\n" << verifyErrors << "\n";
        return 1;
    }

    // ---- Emit IR ----
    if (!emitFile.empty()) {
        std::error_code EC;
        llvm::raw_fd_ostream out(emitFile, EC, llvm::sys::fs::OF_Text);
        if (EC) {
            std::cerr << "error: cannot write to '" << emitFile
                      << "': " << EC.message() << "\n";
            return 1;
        }
        llvmModule->print(out, nullptr);
        out.flush();
    } else if (runFunc.empty()) {
        // Print to stdout if not running
        llvmModule->print(llvm::outs(), nullptr);
    }

    // ---- Compile & Run ----
    if (!runFunc.empty()) {
        // Write temporary .ll file
        std::string tempLL = emitFile.empty() ? "mingus_v2_temp.ll" : emitFile;
        if (emitFile.empty()) {
            std::error_code EC;
            llvm::raw_fd_ostream out(tempLL, EC, llvm::sys::fs::OF_Text);
            if (EC) {
                std::cerr << "error: cannot write temp file: "
                          << EC.message() << "\n";
                return 1;
            }
            llvmModule->print(out, nullptr);
            out.flush();
        }

        // Compile with clang
        std::string clang = findClang();
        std::string exeFile = fs::path(sourceFile).stem().string() + ".exe";
        std::string cmd = clang + " -O2 -o " + exeFile + " " + tempLL;

        int compileResult = std::system(cmd.c_str());
        if (compileResult != 0) {
            std::cerr << "error: clang compilation failed (exit code "
                      << compileResult << ")\n";
            if (emitFile.empty()) fs::remove(tempLL);
            return 1;
        }

        // Run (use .\ prefix on Windows so cmd.exe finds it in CWD)
        std::string runCmd = ".\\" + exeFile;
        int runResult = std::system(runCmd.c_str());

        // Cleanup
        if (emitFile.empty()) fs::remove(tempLL);
        fs::remove(exeFile);

        return runResult;
    }

    return 0;
}
