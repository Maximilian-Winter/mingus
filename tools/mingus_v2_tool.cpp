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
#include <llvm/TargetParser/Triple.h>
#include <llvm/TargetParser/Host.h>
#pragma warning(pop)

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
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
    std::vector<mingus::Diagnostic> syntaxErrors;

    void syntaxError(antlr4::Recognizer* /*recognizer*/,
                     antlr4::Token* offendingSymbol,
                     size_t line, size_t charPositionInLine,
                     const std::string& msg,
                     std::exception_ptr /*e*/) override
    {
        auto loc = std::make_shared<mingus::DebugInfo>(
            static_cast<int>(line), static_cast<int>(charPositionInLine));
        std::string fullMsg = msg;
        if (offendingSymbol) {
            fullMsg = "near '" + offendingSymbol->getText() + "': " + msg;
        }
        syntaxErrors.push_back({mingus::DiagnosticLevel::Error, fullMsg, loc});
    }

    bool hasErrors() const { return !syntaxErrors.empty(); }
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
    const std::string& source, const std::string& fileName,
    mingus::ErrorReporter* errors = nullptr)
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

    if (errorListener.hasErrors()) {
        if (errors) {
            for (auto& diag : errorListener.syntaxErrors) {
                if (diag.location) diag.location->sourceFile = fileName;
                errors->error(diag.message, diag.location);
            }
        } else {
            for (const auto& diag : errorListener.syntaxErrors) {
                std::cerr << "syntax error at ";
                if (diag.location) std::cerr << diag.location->toString();
                std::cerr << ": " << diag.message << "\n";
            }
            std::cerr << "error: parse errors in '" << fileName << "'\n";
        }
        return nullptr;
    }

    mingus::parser::ASTGenerator generator;
    generator.setSourceFile(fileName);
    auto program = generator.generate(tree);

    if (generator.hasErrors()) {
        if (errors) {
            for (const auto& err : generator.errors) {
                auto loc = std::make_shared<mingus::DebugInfo>(err.line, err.column);
                loc->sourceFile = fileName;
                errors->error(err.message, loc);
            }
        } else {
            std::cerr << "error: AST generation errors in '" << fileName << "':\n";
            for (const auto& err : generator.errors) {
                std::cerr << "  " << err.line << ":" << err.column
                          << ": " << err.message << "\n";
            }
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
    const std::vector<std::string>& extraSearchPaths,
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
            // Search source directory first, then extra include paths
            std::string filePath;
            std::string candidate = sourceDir + "/" + moduleName + ".mingus";
            if (fs::exists(candidate)) {
                filePath = candidate;
            } else {
                for (const auto& dir : extraSearchPaths) {
                    candidate = dir + "/" + moduleName + ".mingus";
                    if (fs::exists(candidate)) {
                        filePath = candidate;
                        break;
                    }
                }
            }

            if (filePath.empty()) {
                std::cerr << "error: cannot find imported module '"
                          << moduleName << "' (searched " << sourceDir;
                for (const auto& dir : extraSearchPaths) {
                    std::cerr << ", " << dir;
                }
                std::cerr << ")\n";
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
                  << "  --opt <0|1|2>     Optimization level (default: 0)\n"
                  << "  --debug / -g      Emit DWARF debug info (use with --opt 0)\n"
                  << "  --link <lib>      Link a C library (e.g. --link SDL2)\n"
                  << "  --lib-path <dir>  Add library search path (e.g. --lib-path /usr/lib)\n"
                  << "  --include-path <dir>  Add import search path for .mingus files\n"
                  << "  --bench [N]           Benchmark: run N times (default 10) and report timing\n"
                  << "  --target <triple>     Target triple (default: host platform)\n";
        return 1;
    }

    std::string sourceFile = argv[1];
    std::string emitFile;
    std::string entryFunc;
    std::string runFunc;
    int optLevel = 0;
    bool debugMode = false;
    std::vector<std::string> cliLinkLibs;
    std::vector<std::string> libPaths;
    std::vector<std::string> importPaths;
    int benchIterations = 0;
    std::string targetTriple;

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
        } else if (arg == "--debug" || arg == "-g") {
            debugMode = true;
        } else if (arg == "--link" && i + 1 < argc) {
            cliLinkLibs.push_back(argv[++i]);
        } else if (arg == "--lib-path" && i + 1 < argc) {
            libPaths.push_back(argv[++i]);
        } else if (arg == "--include-path" && i + 1 < argc) {
            importPaths.push_back(argv[++i]);
        } else if (arg == "--bench") {
            benchIterations = 10;  // default
            if (i + 1 < argc) {
                try { benchIterations = std::stoi(argv[i + 1]); ++i; }
                catch (...) {}  // next arg isn't a number, keep default
            }
        } else if (arg == "--target" && i + 1 < argc) {
            targetTriple = argv[++i];
        }
    }

    if (debugMode && optLevel > 0) {
        std::cerr << "warning: debug info with --opt " << optLevel
                  << " may produce limited debug data; use --opt 0 for best results\n";
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

    if (!resolveImports(program, sourceDir, importPaths, loadedFiles)) {
        return 1;
    }

    // ---- Collect link directives from AST ----
    for (auto& mod : program->modules) {
        for (auto& decl : mod->declarations) {
            if (auto link = std::dynamic_pointer_cast<mingus::LinkDirective>(decl)) {
                program->linkLibraries.push_back(link->libraryName);
            }
        }
    }
    // Also add CLI --link libraries
    for (const auto& lib : cliLinkLibs) {
        program->linkLibraries.push_back(lib);
    }

    // ---- Target triple + platform types ----
    if (targetTriple.empty()) {
        targetTriple = llvm::sys::getDefaultTargetTriple();
    }
    llvm::Triple triple(targetTriple);
    unsigned pointerWidth = triple.isArch64Bit() ? 8 : 4;

    // ---- Semantic Analysis (4 passes, multi-error recovery) ----
    mingus::SymbolTable symbolTable;
    symbolTable.registerPlatformTypes(pointerWidth);
    mingus::ErrorReporter errors;
    errors.setDefaultSourceFile(sourceFile);
    errors.setErrorLimit(20);

    // Pass 1: Build scope tree and symbols
    mingus::SymbolTableBuilder pass1(symbolTable, errors);
    pass1.build(*program);

    // Pass 2: Resolve types (continue even after Pass 1 errors — ErrorType prevents cascading)
    if (!errors.atErrorLimit()) {
        try {
            mingus::TypeResolver pass2(symbolTable, errors);
            pass2.resolve(*program);
        } catch (const std::exception& e) {
            errors.error(std::string("internal: type resolver crashed: ") + e.what());
        } catch (...) {
            errors.error("internal: type resolver crashed with unknown exception");
        }
    }

    // Pass 3: Type checking
    if (!errors.atErrorLimit()) {
        try {
            mingus::TypeChecker pass3(symbolTable, errors);
            pass3.check(*program);
        } catch (const std::exception& e) {
            errors.error(std::string("internal: type checker crashed: ") + e.what());
        } catch (...) {
            errors.error("internal: type checker crashed with unknown exception");
        }
    }

    // Pass 4: Semantic validation + RAII analysis
    // Declared outside try-catch because pass4.getRAIIInfo() is needed by codegen
    mingus::SemanticValidator pass4(symbolTable, errors);
    if (!errors.atErrorLimit()) {
        try {
            pass4.validate(*program);
        } catch (const std::exception& e) {
            errors.error(std::string("internal: semantic validator crashed: ") + e.what());
        } catch (...) {
            errors.error("internal: semantic validator crashed with unknown exception");
        }
    }

    // Report ALL accumulated errors
    if (errors.hasErrors()) {
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
    mingus::codegen::IRGenerator irgen(symbolTable, pass4.getRAIIInfo(),
                                        debugMode, sourceFile, targetTriple);
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

        // Append library search paths (-L)
        for (const auto& dir : libPaths) {
            cmd += " -L\"" + dir + "\"";
        }

        // Append link libraries (-l) from both source directives and CLI
        for (const auto& lib : program->linkLibraries) {
            cmd += " -l" + lib;
        }

        int compileResult = std::system(cmd.c_str());
        if (compileResult != 0) {
            std::cerr << "error: clang compilation failed (exit code "
                      << compileResult << ")\n";
            if (emitFile.empty()) fs::remove(tempLL);
            return 1;
        }

        // Run (use .\ prefix on Windows so cmd.exe finds it in CWD)
        std::string runCmd = ".\\" + exeFile;
        int runResult = 0;

        if (benchIterations > 0) {
            std::vector<double> timings;
            timings.reserve(benchIterations);

            for (int iter = 0; iter < benchIterations; iter++) {
                std::string iterCmd = runCmd;
                if (iter > 0) iterCmd += " > NUL 2>&1";

                auto start = std::chrono::high_resolution_clock::now();
                int rc = std::system(iterCmd.c_str());
                auto end = std::chrono::high_resolution_clock::now();

                double ms = std::chrono::duration<double, std::milli>(end - start).count();
                timings.push_back(ms);

                if (rc != 0 && iter == 0) {
                    runResult = rc;
                    break;
                }
            }

            if (!timings.empty()) {
                double minT = timings[0], maxT = timings[0], sum = 0;
                for (double t : timings) {
                    if (t < minT) minT = t;
                    if (t > maxT) maxT = t;
                    sum += t;
                }
                double avg = sum / timings.size();

                std::cerr << "\n--- benchmark (" << timings.size() << " runs) ---\n";
                std::cerr << "  min:   " << std::fixed << std::setprecision(3) << minT << " ms\n";
                std::cerr << "  avg:   " << avg << " ms\n";
                std::cerr << "  max:   " << maxT << " ms\n";
                std::cerr << "  total: " << sum << " ms\n";
            }
        } else {
            runResult = std::system(runCmd.c_str());
        }

        // Cleanup
        if (emitFile.empty()) fs::remove(tempLL);
        fs::remove(exeFile);

        return runResult;
    }

    return 0;
}
