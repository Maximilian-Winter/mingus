cmake_minimum_required(VERSION 3.20)
project(mingus_v2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
)

target_include_directories(mingus_v2_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ---- Compile test (ensure headers are self-contained) ----
# This executable just instantiates the core types to verify linkage.
add_executable(mingus_v2_test
    test/smoke_test.cpp
)

target_link_libraries(mingus_v2_test PRIVATE mingus_v2_core)
