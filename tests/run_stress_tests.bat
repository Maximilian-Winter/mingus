@echo off
setlocal enabledelayedexpansion

:: Run all Mingus stress tests
:: Usage: run_stress_tests.bat [options]
::
:: Options:
::   --code     Print Mingus source code for each test
::   --ir       Print generated LLVM IR for each test
::   --output   Print program output for each test

:: Parse options
set SHOW_CODE=0
set SHOW_IR=0
set SHOW_OUTPUT=0

:parse_args
if "%~1"=="" goto :args_done
if "%~1"=="--code" (set SHOW_CODE=1& shift & goto :parse_args)
if "%~1"=="--ir" (set SHOW_IR=1& shift & goto :parse_args)
if "%~1"=="--output" (set SHOW_OUTPUT=1& shift & goto :parse_args)
if "%~1"=="--help" goto :usage
shift
goto :parse_args
:args_done

set TOOL=.\mingus_ir_tool.exe

set PASS=0
set FAIL=0
set WARN=0
set TOTAL=0

echo ============================================================
echo  Mingus Stress Test Suite
echo ============================================================
echo.

call :run_test stress_01_closure_churn     Stress01_main  "Closure Churn (50k iterations)"
call :run_test stress_02_nested_capture    Stress02_main  "Nested Capture Cascade (20k)"
call :run_test stress_03_reassignment      Stress03_main  "Reassignment Hammer (30k)"
call :run_test stress_04_early_return_raii  Stress04_main  "Early Return + RAII"
call :run_test stress_05_interface_closure  Stress05_main  "Interface + Closure Coexistence (20k)"
call :run_test stress_06_recursive_match   Stress06_main  "Recursive Match (fib)"
call :run_test stress_07_temporary_leak    Stress07_main  "Temporary Closure Leak (50k)"
call :run_test stress_08_destructor_closure Stress08_main  "Destructor + Closure Interleave"
call :run_test stress_09_triple_reassign   Stress09_main  "Triple Reassignment"
call :run_test stress_10_closure_in_struct Stress10_main  "Closure in Struct Field (20k)"
call :run_test stress_11_closure_in_class  Stress11_main  "Closure in Class Field (20k)"
call :run_test stress_13_break_continue_raii Stress13_main "Break/Continue + RAII"
call :run_test stress_14_match_guard_raii   Stress14_main  "Match Guard + RAII"
call :run_test stress_15_struct_ptr_copy    Stress15_main  "Struct Ptr Shallow Copy"
call :run_test stress_16_shadow_capture     Stress16_main  "Variable Shadow Capture"
call :run_test stress_17_long_running       Stress17_main  "Long-Running Stability (100k)"

echo.
echo ============================================================
if !FAIL! gtr 0 (
    echo  Results: !PASS! passed, !FAIL! FAILED out of !TOTAL! tests
) else (
    echo  Results: !PASS! passed, !FAIL! failed out of !TOTAL! tests
)
if !WARN! gtr 0 echo  (!WARN! without .expected file^)
echo ============================================================

:: Clean up temporary files
del stress_*.actual stress_*.exe stress_*.ll 2>nul

if !FAIL! gtr 0 exit /b 1
exit /b 0

:usage
echo Usage: run_stress_tests.bat [options]
echo.
echo Options:
echo   --code     Print Mingus source code for each test
echo   --ir       Print generated LLVM IR for each test
echo   --output   Print program output for each test
echo   --help     Show this help
exit /b 0

:run_test
set /a TOTAL+=1
set FILE=%~1
set ENTRY=%~2
set DESC=%~3

echo -------- %DESC% [%FILE%.mingus] --------

if "!SHOW_CODE!"=="1" (
    echo.
    echo   --- Source ---
    type %FILE%.mingus
    echo.
    echo   -------------
)

:: Step 1: Generate IR with main wrapper (--opt 2 enables O2 optimization)
%TOOL% %FILE%.mingus --emit %FILE%.ll --entry %ENTRY% --opt 2 >nul 2>&1
if errorlevel 1 (
    echo   FAIL: IR generation failed
    set /a FAIL+=1
    echo.
    goto :eof
)

if "!SHOW_IR!"=="1" (
    echo.
    echo   --- LLVM IR ---
    type %FILE%.ll
    echo.
    echo   ---------------
)

:: Step 2: Compile with clang
clang %FILE%.ll -o %FILE%.exe -O2 2>nul
if errorlevel 1 (
    echo   FAIL: clang compilation failed
    set /a FAIL+=1
    echo.
    goto :eof
)

:: Step 3: Run and capture output
.\%FILE%.exe > %FILE%.actual 2>&1

if "!SHOW_OUTPUT!"=="1" (
    echo.
    echo   --- Output ---
    type %FILE%.actual
    echo   --------------
)

:: Step 4: Compare against expected output
if exist %FILE%.expected (
    fc /b %FILE%.expected %FILE%.actual >nul 2>&1
    if errorlevel 1 (
        echo   FAIL: output mismatch
        echo.
        echo   --- Expected ---
        type %FILE%.expected
        echo   --- Actual ---
        type %FILE%.actual
        echo   ---------------
        echo.
        set /a FAIL+=1
    ) else (
        echo   PASS
        set /a PASS+=1
    )
) else (
    echo   WARN: no .expected file, showing output:
    echo.
    type %FILE%.actual
    echo.
    set /a PASS+=1
    set /a WARN+=1
)
echo.
goto :eof
