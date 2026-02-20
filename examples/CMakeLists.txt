cmake_minimum_required(VERSION 3.14)

# Example 1: Simple AST construction
add_executable(ast_example_simple simple_example.cpp)
target_link_libraries(ast_example_simple PRIVATE mingus_ast)

# Example 2: Factorial function AST construction
add_executable(ast_example_factorial factorial_example.cpp)
target_link_libraries(ast_example_factorial PRIVATE mingus_ast)

# Example 3: Parser example - parse Mingus source and generate AST
add_executable(parser_example parser_example.cpp)
target_link_libraries(parser_example PRIVATE mingus_parser)

# Add dependency on antlr4_runtime
add_dependencies(parser_example antlr4_static)

# On Windows, need to link against the static library directly
if(MSVC)
    target_link_libraries(parser_example PRIVATE ${ANTLR4_STATIC_LIBRARIES})
else()
    target_link_libraries(parser_example PRIVATE antlr4_static)
endif()

# Example 4: AST Tool - command-line utility to parse and print AST
add_executable(mingus_ast_tool mingus_ast_tool.cpp)
target_link_libraries(mingus_ast_tool PRIVATE mingus_parser)
add_dependencies(mingus_ast_tool antlr4_static)

if(MSVC)
    target_link_libraries(mingus_ast_tool PRIVATE ${ANTLR4_STATIC_LIBRARIES})
else()
    target_link_libraries(mingus_ast_tool PRIVATE antlr4_static)
endif()

# Example 5: Sema Tool - semantic analysis dump
add_executable(mingus_sema_tool mingus_sema_tool.cpp)
target_link_libraries(mingus_sema_tool PRIVATE mingus_parser mingus_sema)
add_dependencies(mingus_sema_tool antlr4_static)

if(MSVC)
    target_link_libraries(mingus_sema_tool PRIVATE ${ANTLR4_STATIC_LIBRARIES})
else()
    target_link_libraries(mingus_sema_tool PRIVATE antlr4_static)
endif()

# Example 6: IR Tool - LLVM IR generation dump
add_executable(mingus_ir_tool mingus_ir_tool.cpp)
target_link_libraries(mingus_ir_tool PRIVATE mingus_parser mingus_codegen)
add_dependencies(mingus_ir_tool antlr4_static)

if(MSVC)
    target_link_libraries(mingus_ir_tool PRIVATE ${ANTLR4_STATIC_LIBRARIES})
else()
    target_link_libraries(mingus_ir_tool PRIVATE antlr4_static)
endif()

# Copy sample files for parser example
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/mingus_v1_samples.mingus
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/hello.mingus
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Post-build: copy mingus_ir_tool.exe to source examples folder and tests folder
add_custom_command(TARGET mingus_ir_tool POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:mingus_ir_tool>
        ${CMAKE_CURRENT_SOURCE_DIR}/mingus_ir_tool.exe
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:mingus_ir_tool>
        ${CMAKE_SOURCE_DIR}/tests/mingus_ir_tool.exe
    COMMENT "Copying mingus_ir_tool to examples and tests folders"
)
