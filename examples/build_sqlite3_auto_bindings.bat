@echo off
:: Build the SQLite3 Auto-Generated Bindings example
:: Demonstrates the full pipeline: generate bindings -> import -> compile -> link -> run
::
:: Prerequisites:
::   - sqlite3.c and sqlite3.h in extern_c\ (see extern_c\README_sqlite3.txt)
::   - mingus_v2_tool.exe in this directory (copied by CMake post-build)
::   - clang on PATH
::   - pip install libclang

echo === Building SQLite3 Auto-Generated Bindings Example ===
echo.

:: Step 1: Compile SQLite3 amalgamation to object file (cached)
if not exist extern_c\sqlite3.obj (
    echo [1/5] Compiling sqlite3.c ^(first time, may take a moment^)...
    clang -c extern_c\sqlite3.c -o extern_c\sqlite3.obj -O2
    if errorlevel 1 (
        echo FAIL: could not compile sqlite3.c
        exit /b 1
    )
) else (
    echo [1/5] Using cached sqlite3.obj
)

:: Step 2: Auto-generate Mingus bindings from sqlite3.h
echo [2/5] Generating Mingus bindings from sqlite3.h...
python ..\mingus_bind_gen.py extern_c\sqlite3.h --prefix sqlite3_ --prefix SQLITE_ --module SQLite3 -o SQLite3.mingus
if errorlevel 1 (
    echo FAIL: binding generation failed ^(requires: pip install libclang^)
    exit /b 1
)
echo       Generated SQLite3.mingus

:: Step 3: Compile Mingus source (auto-imports SQLite3.mingus)
echo [3/5] Compiling Mingus source...
.\mingus_v2_tool.exe example_14_sqlite3_auto_bindings.mingus --emit sqlite3_auto.ll --entry SQLite3AutoBind_main --opt 2
if errorlevel 1 (
    echo FAIL: Mingus compilation failed
    exit /b 1
)

:: Step 4: Link LLVM IR with SQLite3 object file
echo [4/5] Linking with clang...
clang -O2 -o sqlite3_auto.exe sqlite3_auto.ll extern_c\sqlite3.obj
if errorlevel 1 (
    echo FAIL: clang linking failed
    exit /b 1
)

echo.
echo === Build successful! ===
echo.

:: Step 5: Run the example
echo [5/5] Running sqlite3_auto.exe...
echo.
.\sqlite3_auto.exe

:: Cleanup build artifacts
del sqlite3_auto.ll 2>nul
