//================================================================================
// MINGUS v1 - LLVM IR Tool
// Parses a Mingus source file, runs semantic analysis, generates LLVM IR,
// and prints the IR as text. Optionally runs the LLVM verifier.
//
// Usage: mingus_ir_tool <source_file.mingus> [options]
//
// --entry <func>  Inject a C main() that calls the given function.
// --run <func>    Like --entry, but also compile with clang and execute.
// --emit <out.ll> Write the LLVM IR to a .ll file.
// --opt <level>   Optimization level: 0 (none), 1 (basic), 2 (full O2).
//                 Default: 0 (unoptimized, matches prior behavior).
//================================================================================

#include "mingus/parser/ASTGenerator.h"
#include "mingus/Sema.h"
#include "mingus/Codegen.h"
#include "MingusLexer.h"
#include "MingusParser.h"

#include <antlr4-runtime.h>

#pragma warning(push, 0)
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/Analysis/CGSCCPassManager.h>
#pragma warning(pop)

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <cstdlib>
#include <set>

using namespace mingus;
using namespace mingus::ast;
using namespace mingus::parser;
using namespace mingus::sema;
using namespace mingus::codegen;

//================================================================================
// Error listener to capture parse errors
//================================================================================
class VerboseErrorListener : public antlr4::BaseErrorListener {
public:
    bool hasErrors = false;

    void syntaxError(antlr4::Recognizer*, antlr4::Token*,
                     size_t line, size_t charPositionInLine,
                     const std::string& msg, std::exception_ptr) override {
        hasErrors = true;
        std::cerr << "Parse Error: Line " << line << ":" << charPositionInLine
                  << " " << msg << "\n";
    }
};

//================================================================================
// File reading
//================================================================================
static std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

//================================================================================
// Parse source file to AST
//================================================================================
static std::shared_ptr<ProgramNode> parseFile(const std::string& filename) {
    std::string source = readFile(filename);

    antlr4::ANTLRInputStream input(source);
    input.name = filename;

    AntlrMingusParser::MingusLexer lexer(&input);
    VerboseErrorListener lexerErrors;
    lexer.removeErrorListeners();
    lexer.addErrorListener(&lexerErrors);

    antlr4::CommonTokenStream tokens(&lexer);

    AntlrMingusParser::MingusParser parser(&tokens);
    VerboseErrorListener parserErrors;
    parser.removeErrorListeners();
    parser.addErrorListener(&parserErrors);

    auto* tree = parser.program();

    if (lexerErrors.hasErrors || parserErrors.hasErrors) {
        std::cerr << "Parse failed.\n";
        return nullptr;
    }

    ASTGenerator generator;
    auto ast = generator.generate(tree);

    if (generator.hasErrors()) {
        std::cerr << "AST Generation errors:\n";
        for (const auto& err : generator.errors) {
            std::cerr << "  Line " << err.location.line << ":" << err.location.column
                      << " " << err.message << "\n";
        }
    }

    return ast;
}

//================================================================================
// Import resolution: discover, parse, and merge imported modules
//================================================================================
static std::string getDirectory(const std::string& filepath) {
    auto lastSep = filepath.find_last_of("/\\");
    if (lastSep != std::string::npos)
        return filepath.substr(0, lastSep + 1);
    return "";
}

static void resolveImports(ProgramNode& program, const std::string& mainFile) {
    std::string baseDir = getDirectory(mainFile);
    std::set<std::string> loadedFiles;
    loadedFiles.insert(mainFile);

    // Collect module names that already exist in the program
    std::set<std::string> knownModules;
    for (auto& mod : program.modules) {
        knownModules.insert(mod->name);
    }

    // Process imports iteratively (newly added modules may have their own imports)
    // Use index-based loop since program.modules grows during iteration
    for (size_t mi = 0; mi < program.modules.size(); ++mi) {
        auto& mod = program.modules[mi];

        for (auto& imp : mod->imports) {
            // Determine which module name(s) this import references
            std::vector<std::string> moduleNames;

            if (imp->isFromImport()) {
                // "import add from Math;" -- source is the module name
                moduleNames.push_back(imp->source.getSimpleName());
            } else {
                // "import Math;" -- each target name IS a module name
                for (auto& target : imp->targets) {
                    moduleNames.push_back(target.name);
                }
            }

            for (auto& moduleName : moduleNames) {
                // Skip if this module is already in the program
                if (knownModules.count(moduleName)) continue;

                // Look for <ModuleName>.mingus in the same directory as the main file
                std::string importFile = baseDir + moduleName + ".mingus";

                // Check if already loaded (prevents cycles)
                if (loadedFiles.count(importFile)) continue;
                loadedFiles.insert(importFile);

                // Check if file exists
                {
                    std::ifstream test(importFile);
                    if (!test.good()) {
                        std::cerr << "Import error: cannot find module '" << moduleName
                                  << "' (looked for " << importFile << ")\n";
                        continue;
                    }
                }

                // Parse the imported file
                std::cout << "  Importing: " << importFile << "\n";
                auto importedProgram = parseFile(importFile);
                if (!importedProgram) {
                    std::cerr << "Import error: failed to parse " << importFile << "\n";
                    continue;
                }

                // Merge all modules from the imported file into the main program
                for (auto& importedMod : importedProgram->modules) {
                    if (knownModules.count(importedMod->name)) {
                        std::cerr << "Import warning: module '" << importedMod->name
                                  << "' from " << importFile
                                  << " already exists, skipping.\n";
                        continue;
                    }
                    knownModules.insert(importedMod->name);
                    program.modules.push_back(std::move(importedMod));
                }
            }
        }
    }
}

//================================================================================
// Inject a C-compatible main() that calls the entry function
//================================================================================
static void injectMainWrapper(llvm::Module& module, const std::string& entryName) {
    auto& ctx = module.getContext();

    // Find the entry function
    auto* entryFn = module.getFunction(entryName);
    if (!entryFn) {
        std::cerr << "Warning: entry function '" << entryName << "' not found in module.\n";
        std::cerr << "Available functions:\n";
        for (auto& fn : module) {
            if (!fn.isDeclaration()) {
                std::cerr << "  " << fn.getName().str() << "\n";
            }
        }
        return;
    }

    // Create: int main() { return entry(); } or int main() { entry(); return 0; }
    auto* i32Ty = llvm::Type::getInt32Ty(ctx);
    auto* mainTy = llvm::FunctionType::get(i32Ty, false);
    auto* mainFn = llvm::Function::Create(mainTy,
        llvm::Function::ExternalLinkage, "main", &module);

    auto* bb = llvm::BasicBlock::Create(ctx, "entry", mainFn);
    llvm::IRBuilder<> builder(bb);

    if (entryFn->getReturnType()->isIntegerTy()) {
        auto* ret = builder.CreateCall(entryFn);
        builder.CreateRet(ret);
    } else {
        builder.CreateCall(entryFn);
        builder.CreateRet(llvm::ConstantInt::get(i32Ty, 0));
    }
}

//================================================================================
// LLVM Optimization Pipeline
//================================================================================
static void optimizeModule(llvm::Module& module, int optLevel) {
    if (optLevel <= 0) return;

    // Create the four analysis managers
    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;

    // Create the pass builder and register analyses
    llvm::PassBuilder PB;
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    // Build the optimization pipeline based on level
    llvm::OptimizationLevel level;
    if (optLevel == 1) {
        level = llvm::OptimizationLevel::O1;
    } else {
        level = llvm::OptimizationLevel::O2;
    }

    llvm::ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(level);
    MPM.run(module, MAM);
}

//================================================================================
// Main
//================================================================================
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: mingus_ir_tool <source.mingus> [options]\n"
                  << "  --emit <out.ll>       Write LLVM IR to file\n"
                  << "  --entry <Module_func> Inject a C main() that calls the given function\n"
                  << "  --run <Module_func>   Like --entry, but also compile and execute\n"
                  << "  --opt <0|1|2>         Optimization level (default: 0)\n"
                  << "  --debug               Emit debug information (DWARF/CodeView)\n"
                  << "                          0 = none, 1 = basic (O1), 2 = full (O2)\n";
        return 1;
    }

    std::string filename = argv[1];
    std::string entryFunc;  // --entry or --run <entry_function>
    std::string emitFile;   // --emit <output.ll>
    bool doRun = false;     // --run (compile + execute)
    int optLevel = 0;       // --opt <level> (0=none, 1=O1, 2=O2)
    bool debugInfo = false;  // --debug (emit DWARF/CodeView debug info)

    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--run" && i + 1 < argc) {
            entryFunc = argv[++i];
            doRun = true;
        } else if (arg == "--entry" && i + 1 < argc) {
            entryFunc = argv[++i];
        } else if (arg == "--emit" && i + 1 < argc) {
            emitFile = argv[++i];
        } else if (arg == "--opt" && i + 1 < argc) {
            optLevel = std::atoi(argv[++i]);
            if (optLevel < 0 || optLevel > 2) {
                std::cerr << "Warning: --opt level clamped to range [0, 2]\n";
                optLevel = std::clamp(optLevel, 0, 2);
            }
        } else if (arg == "--debug") {
            debugInfo = true;
        }
    }

    // 1. Parse
    std::cout << "=== Parsing: " << filename << " ===\n";
    auto program = parseFile(filename);
    if (!program) return 1;

    // 1b. Resolve imports (discover, parse, merge imported modules)
    resolveImports(*program, filename);

    // 2. Semantic analysis (all 4 passes)
    SymbolTable symbolTable;
    TypeRegistry typeRegistry;
    ErrorReporter errors;

    std::cout << "=== Running semantic analysis ===\n";

    SymbolTableBuilder builder(symbolTable, errors);
    builder.build(*program);

    TypeResolver resolver(symbolTable, typeRegistry, errors);
    resolver.resolve(*program);

    TypeChecker checker(symbolTable, typeRegistry, errors);
    checker.check(*program);

    SemanticValidator validator(symbolTable, typeRegistry, errors);
    validator.validate(*program);

    if (errors.hasErrors()) {
        std::cerr << "\nSemantic errors (" << errors.getErrorCount() << "):\n";
        for (const auto& diag : errors.getDiagnostics()) {
            if (diag.severity == Severity::Error) {
                std::cerr << "  " << diag.location.line << ":" << diag.location.column
                          << " ERROR: " << diag.message << "\n";
            }
        }
        std::cerr << "\nAborting IR generation due to semantic errors.\n";
        return 1;
    }

    std::cout << "Semantic analysis: " << errors.getErrorCount() << " errors, "
              << errors.getWarningCount() << " warnings\n\n";

    // 3. LLVM IR Generation
    std::cout << "=== Generating LLVM IR ===\n\n";

    IRGenerator irGen(symbolTable, typeRegistry, debugInfo, filename);
    auto module = irGen.generate(*program);

    // 4. Inject main wrapper if --run was specified
    if (!entryFunc.empty()) {
        injectMainWrapper(*module, entryFunc);
    }

    // 5. Run optimization pipeline
    if (optLevel > 0) {
        std::cout << "=== Optimizing (O" << optLevel << ") ===\n";
        optimizeModule(*module, optLevel);
    }

    // 6. Print IR (unless --run, then just show summary)
    if (entryFunc.empty()) {
        std::string irOutput;
        llvm::raw_string_ostream irStream(irOutput);
        module->print(irStream, nullptr);
        std::cout << irOutput << "\n";
    }

    // 7. Verify
    std::cout << "=== Verifying LLVM IR ===\n";
    std::string verifyErrors;
    llvm::raw_string_ostream verifyStream(verifyErrors);
    if (llvm::verifyModule(*module, &verifyStream)) {
        std::cerr << "LLVM Verification FAILED:\n" << verifyErrors << "\n";
        return 1;
    }
    std::cout << "LLVM IR verification passed.\n";

    // 8. Emit .ll file
    if (!emitFile.empty() || !entryFunc.empty()) {
        std::string llFile = emitFile.empty() ? "mingus_output.ll" : emitFile;

        std::error_code ec;
        llvm::raw_fd_ostream out(llFile, ec, llvm::sys::fs::OF_Text);
        if (ec) {
            std::cerr << "Error writing " << llFile << ": " << ec.message() << "\n";
            return 1;
        }
        module->print(out, nullptr);
        out.flush();
        std::cout << "Wrote IR to: " << llFile << "\n";
    }

    // 9. Compile and run if --run
    if (doRun) {
        std::string llFile = emitFile.empty() ? "mingus_output.ll" : emitFile;
        std::string exeFile = "mingus_output.exe";

        // Find clang
        std::string clang;

        // First: try clang from PATH (silent check using "where" on Windows)
#ifdef _WIN32
        if (std::system("where clang >nul 2>nul") == 0) {
            clang = "clang";
        }
#else
        if (std::system("which clang >/dev/null 2>&1") == 0) {
            clang = "clang";
        }
#endif

        // Second: search relative to this executable
        if (clang.empty()) {
            std::string exeDir;
            {
                std::string argv0 = argv[0];
                auto lastSep = argv0.find_last_of("/\\");
                if (lastSep != std::string::npos)
                    exeDir = argv0.substr(0, lastSep + 1);
            }

            std::string candidates[] = {
                exeDir + "..\\..\\clang+llvm-21.1.8-x86_64-pc-windows-msvc\\bin\\clang.exe",
                exeDir + "..\\clang+llvm-21.1.8-x86_64-pc-windows-msvc\\bin\\clang.exe",
                "..\\clang+llvm-21.1.8-x86_64-pc-windows-msvc\\bin\\clang.exe",
                "..\\..\\clang+llvm-21.1.8-x86_64-pc-windows-msvc\\bin\\clang.exe",
            };
            for (auto& c : candidates) {
                std::ifstream test(c);
                if (test.good()) {
                    // Resolve to absolute path (Windows _fullpath removes "..")
#ifdef _WIN32
                    char resolved[_MAX_PATH];
                    if (_fullpath(resolved, c.c_str(), _MAX_PATH)) {
                        clang = resolved;
                    } else {
                        clang = c;
                    }
#else
                    char resolved[PATH_MAX];
                    if (realpath(c.c_str(), resolved)) {
                        clang = resolved;
                    } else {
                        clang = c;
                    }
#endif
                    break;
                }
            }
        }

        if (clang.empty()) {
            std::cerr << "Error: Could not find clang. Make sure clang is in your PATH\n"
                      << "or the LLVM distribution is in the project root.\n";
            return 1;
        }

        // On Windows, cmd.exe needs the entire command wrapped in extra quotes
        // when the executable path itself is quoted
        std::string clangCmd = "\"" + clang + "\" \"" + llFile + "\" -o \"" + exeFile + "\" -O2";
#ifdef _WIN32
        clangCmd = "\"" + clangCmd + "\"";
#endif

        std::cout << "\n=== Compiling with clang ===\n";
        std::cout << clangCmd << "\n";
        int compileResult = std::system(clangCmd.c_str());
        if (compileResult != 0) {
            std::cerr << "Compilation failed (exit code " << compileResult << ")\n";
            return 1;
        }

        std::cout << "\n=== Running " << exeFile << " ===\n\n";
        int runResult = std::system(exeFile.c_str());
        std::cout << "\n=== Program exited with code " << runResult << " ===\n";
    }

    return 0;
}
