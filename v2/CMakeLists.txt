cmake_minimum_required(VERSION 3.20)
project(mingus_v2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- MSVC CRT configuration (must match LLVM's /MT) ----
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

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

# ---- Smoke test executable ----
add_executable(mingus_v2_test
    test/smoke_test.cpp
)

target_link_libraries(mingus_v2_test PRIVATE mingus_v2_codegen)
