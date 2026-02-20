@echo off
setlocal enabledelayedexpansion

:: Run all Mingus stress tests
:: Usage: run_stress_tests.bat [path_to_clang]
::
:: Run from the tests\ directory.
:: Each test needs a matching .expected file for automated validation.

set CLANG=%1
if "%CLANG%"=="" set CLANG=clang

set TOOL=..\examples\mingus_ir_tool.exe

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

:run_test
set /a TOTAL+=1
set FILE=%~1
set ENTRY=%~2
set DESC=%~3

echo -------- %DESC% [%FILE%.mingus] --------

:: Step 1: Generate IR with main wrapper (--opt 2 enables O2 optimization)
%TOOL% %FILE%.mingus --emit %FILE%.ll --entry %ENTRY% --opt 2 >nul 2>&1
if errorlevel 1 (
    echo   FAIL: IR generation failed
    set /a FAIL+=1
    echo.
    goto :eof
)

:: Step 2: Compile with clang
%CLANG% %FILE%.ll -o %FILE%.exe -O2 2>nul
if errorlevel 1 (
    echo   FAIL: clang compilation failed
    set /a FAIL+=1
    echo.
    goto :eof
)

:: Step 3: Run and capture output
.\%FILE%.exe > %FILE%.actual 2>&1

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
