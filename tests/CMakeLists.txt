cmake_minimum_required(VERSION 3.14)

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
)
FetchContent_MakeAvailable(googletest)

# Test executable
add_executable(ast_tests
    test_ast.cpp
)

target_link_libraries(ast_tests
    PRIVATE
        mingus_ast
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(ast_tests)
