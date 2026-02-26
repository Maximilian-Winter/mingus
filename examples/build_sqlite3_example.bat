@echo off
:: Build the SQLite3 FFI example
:: Demonstrates Mingus C FFI with a real-world C library (SQLite3)
::
:: Prerequisites:
::   - sqlite3.c and sqlite3.h in extern_c\ (see extern_c\README_sqlite3.txt)
::   - mingus_v2_tool.exe in this directory (copied by CMake post-build)
::   - clang on PATH

echo === Building SQLite3 FFI Example ===
echo.

:: Step 1: Compile SQLite3 amalgamation to object file (cached)
if not exist extern_c\sqlite3.obj (
    echo [1/4] Compiling sqlite3.c ^(first time, may take a moment^)...
    clang -c extern_c\sqlite3.c -o extern_c\sqlite3.obj -O2
    if errorlevel 1 (
        echo FAIL: could not compile sqlite3.c
        exit /b 1
    )
) else (
    echo [1/4] Using cached sqlite3.obj
)

:: Step 2: Auto-generate Mingus bindings from sqlite3.h
echo [2/4] Generating Mingus bindings from sqlite3.h...
python ..\mingus_bind_gen.py extern_c\sqlite3.h --prefix sqlite3_ --prefix SQLITE_ --module SQLite3 -o SQLite3.mingus
if errorlevel 1 (
    echo FAIL: binding generation failed ^(requires: pip install libclang^)
    exit /b 1
)
echo       Generated SQLite3.mingus

:: Step 3: Compile Mingus source to LLVM IR
echo [3/4] Compiling Mingus source...
.\mingus_v2_tool.exe example_13_sqlite3.mingus --emit sqlite3_example.ll --entry SQLite3Example_main --opt 2
if errorlevel 1 (
    echo FAIL: Mingus compilation failed
    exit /b 1
)

:: Step 4: Link LLVM IR with SQLite3 object file
echo [4/4] Linking with clang...
clang -O2 -o sqlite3_example.exe sqlite3_example.ll extern_c\sqlite3.obj
if errorlevel 1 (
    echo FAIL: clang linking failed
    exit /b 1
)

echo.
echo === Build successful! ===
echo.

:: Run the example
echo Running sqlite3_example.exe...
echo.
.\sqlite3_example.exe

:: Cleanup build artifacts
del sqlite3_example.ll 2>nul
