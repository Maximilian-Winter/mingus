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


# Source files for AST
set(AST_SOURCES
    src/mingus/ast/ASTNode.cpp
    src/mingus/ast/Program.cpp
    src/mingus/ast/TypeNode.cpp
    src/mingus/ast/Declarations.cpp
    src/mingus/ast/Statements.cpp
    src/mingus/ast/Expressions.cpp
    src/mingus/ast/Patterns.cpp
)

# Source files for Parser
set(PARSER_SOURCES
    src/mingus/parser/ASTGenerator.cpp
)

# ANTLR4 generated sources
set(ANTLR_SOURCES
    AntlrMingusParser/MingusLexer.cpp
    AntlrMingusParser/MingusParser.cpp
    AntlrMingusParser/MingusParserBaseVisitor.cpp
    AntlrMingusParser/MingusParserVisitor.cpp
)

# Header files (for IDE support)
set(AST_HEADERS
    include/mingus/AST.h
    include/mingus/ast/ASTNode.h
    include/mingus/ast/ASTVisitor.h
    include/mingus/ast/Type.h
    include/mingus/ast/TypeNode.h
    include/mingus/ast/Program.h
    include/mingus/ast/Declarations.h
    include/mingus/ast/Statements.h
    include/mingus/ast/Expressions.h
    include/mingus/ast/Patterns.h
)

set(PARSER_HEADERS
    include/mingus/parser/ASTGenerator.h
)

# LLVM (set LLVM_DIR in CMake settings to point at the local LLVM distribution)
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(LLVM_LIBS
    core support irreader passes
    analysis transformutils scalaropts instcombine ipo vectorize
    x86codegen x86asmparser x86info x86desc)

# Source files for Codegen
set(CODEGEN_SOURCES
    src/mingus/codegen/IRGenerator.cpp
)

# Header files for Codegen
set(CODEGEN_HEADERS
    include/mingus/Codegen.h
    include/mingus/codegen/IRGenerator.h
)

# Source files for Semantic Analysis
set(SEMA_SOURCES
    src/mingus/sema/TypeRegistry.cpp
    src/mingus/sema/Scope.cpp
    src/mingus/sema/SymbolTableBuilder.cpp
    src/mingus/sema/TypeResolver.cpp
    src/mingus/sema/TypeChecker.cpp
    src/mingus/sema/SemanticValidator.cpp
)

# Header files for Semantic Analysis
set(SEMA_HEADERS
    include/mingus/Sema.h
    include/mingus/sema/TypeRegistry.h
    include/mingus/sema/Symbol.h
    include/mingus/sema/Scope.h
    include/mingus/sema/ErrorReporter.h
    include/mingus/sema/SymbolTable.h
    include/mingus/sema/SymbolTableBuilder.h
    include/mingus/sema/TypeResolver.h
    include/mingus/sema/TypeChecker.h
    include/mingus/sema/SemanticValidator.h
)

# Create static library for AST
add_library(mingus_ast STATIC ${AST_SOURCES} ${AST_HEADERS})

target_include_directories(mingus_ast
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Create static library for Parser
add_library(mingus_parser STATIC ${PARSER_SOURCES} ${ANTLR_SOURCES})

target_include_directories(mingus_parser
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/AntlrMingusParser>
        ${ANTLR4_INCLUDE_DIRS}
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(mingus_parser
    PUBLIC
        mingus_ast
)

# Add dependency on antlr4_runtime being built first
add_dependencies(mingus_parser antlr4_static)

# Link against antlr4_static
if(MSVC)
    target_link_libraries(mingus_parser PUBLIC ${ANTLR4_STATIC_LIBRARIES})
else()
    target_link_libraries(mingus_parser PUBLIC antlr4_static)
endif()

# Create static library for Semantic Analysis
add_library(mingus_sema STATIC ${SEMA_SOURCES} ${SEMA_HEADERS})

target_include_directories(mingus_sema
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(mingus_sema
    PUBLIC
        mingus_ast
)

set_target_properties(mingus_sema PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Create static library for Codegen (LLVM IR Generation)
add_library(mingus_codegen STATIC ${CODEGEN_SOURCES} ${CODEGEN_HEADERS})

target_include_directories(mingus_codegen
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(mingus_codegen
    PUBLIC
        mingus_ast
        mingus_sema
        ${LLVM_LIBS}
)

set_target_properties(mingus_codegen PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Set target properties
set_target_properties(mingus_ast PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

set_target_properties(mingus_parser PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Install targets
install(TARGETS mingus_ast mingus_parser mingus_sema mingus_codegen
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ${AST_HEADERS} DESTINATION include/mingus/ast)
install(FILES ${PARSER_HEADERS} DESTINATION include/mingus/parser)
install(FILES ${SEMA_HEADERS} DESTINATION include/mingus/sema)
install(FILES ${CODEGEN_HEADERS} DESTINATION include/mingus/codegen)
install(FILES include/mingus/AST.h include/mingus/Sema.h include/mingus/Codegen.h DESTINATION include/mingus)

# Optional: Build examples
option(BUILD_EXAMPLES "Build example programs" ON)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

