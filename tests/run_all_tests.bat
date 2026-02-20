@echo off
setlocal enabledelayedexpansion

:: Run all Mingus feature tests
:: Usage: run_all_tests.bat [options]
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

set PASS=0
set FAIL=0
set WARN=0
set TOTAL=0

echo ============================================================
echo  Mingus v1 Feature Test Suite
echo ============================================================
echo.

call :run_test test_01_basics       Test01_main  "Basics"
call :run_test test_02_structs_operators Test02_main "Structs and Operators"
call :run_test test_03_classes_raii  Test03_main  "Classes and RAII"
call :run_test test_04_pipes_match  Test04_main  "Pipes and Match"
call :run_test test_05_enums_switch Test05_main  "Enums and Switch"
call :run_test test_06_lambdas_funcptr Test06_main "Lambdas and Func Ptrs"
call :run_test test_07_floats_math  Test07_main  "Floats and Math"
call :run_test test_08_pointers_raw Test08_main  "Pointers and Raw"
call :run_test test_09_enum_expressions Test09_main "Enum Expressions"
call :run_test test_10_closures Test10_main "Closures"
call :run_test test_11_dsp_showcase Test11_main "DSP Showcase"
call :run_test test_12_imports Test12_main "Imports"
call :run_test test_13_inheritance Test13_main "Inheritance"
call :run_test test_14_strings Test14_main "String Operations"
call :run_test test_15_interfaces Test15_main "Interfaces"
call :run_test test_16_dsp_wav        DspWav_main   "DSP WAV Synthesis"
call :run_test test_17_hex_literals   HexTest_main  "Hex Binary Octal Literals"
call :run_test test_18_tuples         Test18_main   "Tuples"
call :run_test test_19_dynamic_array_map Test19_main "DynamicArray Map"
call :run_test test_20_complex_numbers Test20_main  "Complex Numbers"

echo.
echo ============================================================
if !FAIL! gtr 0 (
    echo  Results: !PASS! passed, !FAIL! FAILED out of !TOTAL! tests
) else (
    echo  Results: !PASS! passed, !FAIL! failed out of !TOTAL! tests
)
if !WARN! gtr 0 echo  (!WARN! without .expected file^)
echo ============================================================

:: Clean up temporary files (only test artifacts, not mingus_ir_tool.exe)
del test_*.actual test_*.exe test_*.ll 2>nul

if !FAIL! gtr 0 exit /b 1
exit /b 0

:usage
echo Usage: run_all_tests.bat [options]
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
.\mingus_ir_tool.exe %FILE%.mingus --emit %FILE%.ll --entry %ENTRY% --opt 2 >nul 2>&1
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
