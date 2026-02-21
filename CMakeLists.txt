cmake_minimum_required(VERSION 3.20)
project(MingusAST VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use static CRT (/MT) in ALL configurations to match pre-built LLVM libraries.
# LLVM is release-only (/MT), so we must use /MT even in Debug builds.
# Debug info (PDB) still works — only the CRT debug checks are lost.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

# Add cmake folder to module path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# required if linking to static library
add_definitions(-DANTLR4CPP_STATIC)

# Use static CRT for antlr4_runtime to match LLVM
set(ANTLR4_WITH_STATIC_CRT ON)

# Specify the version of the antlr4 library needed for this project.
# By default the latest version of antlr4 will be used.  You can specify a
# specific, stable version by setting a repository tag value or a link
# to a zip file containing the libary source.
# set(ANTLR4_TAG 4.11.1)
set(ANTLR4_ZIP_REPOSITORY ${PROJECT_SOURCE_DIR}/extern/antlr4-4.13.2.zip)
set(DOWNLOAD_EXTRACT_TIMESTAMP true)
# add external build for antlrcpp
include(ExternalAntlr4Cpp [DOWNLOAD_EXTRACT_TIMESTAMP])
# add antrl4cpp artifacts to project environment
include_directories(${ANTLR4_INCLUDE_DIRS})


# ---- V2 Core Library ----
# The foundation: Symbol/Scope/Type hierarchy and AST base nodes.
# No dependencies on LLVM, ANTLR4, or V1 code.

add_library(mingus_v2_core STATIC
        # Scope hierarchy
        src/mingus/Scope.cpp

        # Symbol hierarchy
        src/mingus/Symbol.cpp

        # TypeSymbol hierarchy (types AS symbols)
        src/mingus/TypeSymbol.cpp

        # Concrete symbol types (Variable, Function, Class, etc.)
        src/mingus/Symbols.cpp

        # SymbolTable (root owner, type interning)
        src/mingus/SymbolTable.cpp

        # AST base nodes
        src/mingus/AstNode.cpp

        # Sema passes
        src/mingus/sema/SymbolTableBuilder.cpp
        src/mingus/sema/TypeResolver.cpp
        src/mingus/sema/TypeChecker.cpp
        src/mingus/sema/SemanticValidator.cpp
)

target_include_directories(mingus_v2_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ---- V2 Parser Library ----
# ANTLR4 parse tree → V2 AST conversion.
# Reuses generated parser from V1's AntlrMingusParser/ directory.

set(ANTLR_GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/AntlrMingusParser)

add_library(mingus_v2_parser STATIC
        src/mingus/parser/ASTGenerator.cpp
        ${ANTLR_GENERATED_DIR}/MingusLexer.cpp
        ${ANTLR_GENERATED_DIR}/MingusParser.cpp
        ${ANTLR_GENERATED_DIR}/MingusParserBaseVisitor.cpp
        ${ANTLR_GENERATED_DIR}/MingusParserVisitor.cpp
)

target_include_directories(mingus_v2_parser PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${ANTLR_GENERATED_DIR}
        ${ANTLR4_INCLUDE_DIRS}
)

target_link_libraries(mingus_v2_parser PUBLIC mingus_v2_core)
add_dependencies(mingus_v2_parser antlr4_static)

if(MSVC)
    target_link_libraries(mingus_v2_parser PUBLIC ${ANTLR4_STATIC_LIBRARIES})
else()
    target_link_libraries(mingus_v2_parser PUBLIC antlr4_static)
endif()

# ---- LLVM Integration ----
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(LLVM_LIBS
        core support irreader passes
        analysis transformutils scalaropts instcombine ipo vectorize
        x86codegen x86asmparser x86info x86desc)

# ---- V2 Codegen Library ----
# LLVM IR generation. Links against core + LLVM.

add_library(mingus_v2_codegen STATIC
        src/mingus/codegen/IRGenerator.cpp
)

target_include_directories(mingus_v2_codegen PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(mingus_v2_codegen
        PUBLIC
        mingus_v2_core
        ${LLVM_LIBS}
)

# ---- V2 Compiler Tool ----
add_executable(mingus_v2_tool
        tools/mingus_v2_tool.cpp
)

target_link_libraries(mingus_v2_tool PRIVATE
        mingus_v2_parser
        mingus_v2_codegen
)

# Post-build: copy tool to tests/ and examples/
add_custom_command(TARGET mingus_v2_tool POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:mingus_v2_tool>
        ${CMAKE_SOURCE_DIR}/../tests/mingus_v2_tool.exe
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:mingus_v2_tool>
        ${CMAKE_SOURCE_DIR}/../examples/mingus_v2_tool.exe
        COMMENT "Copying mingus_v2_tool to tests/ and examples/"
)
