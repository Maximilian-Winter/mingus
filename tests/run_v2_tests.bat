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
echo  Mingus V2 Feature Test Suite
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
call :run_test test_21_fat_ptr_null    Test21_main  "Fat Pointer Null Comparison"
call :run_test test_22_access_modifiers Test22_main "Access Modifiers"
call :run_test test_23_virtual_destructor Test23_main "Virtual Destructor"
call :run_test test_24_static_methods   Test24_main  "Static Methods"
call :run_test test_25_escape_analysis  Test25_main  "Escape Analysis"
call :run_test test_26_self_capture     Test26_main  "Self-Capturing Closures"
call :run_test test_27_debug_info      Test27_main  "Debug Info"
call :run_test test_28_explicit_captures Test28_main "Explicit Captures"
call :run_test test_29_ref_params      Test29_main  "Reference Parameters"
call :run_test test_30_capture_writeback Test30_main "Capture Write-Back"
call :run_test test_31_closure_struct_params Test31_main "Closure + Struct Params"
call :run_test test_32_closure_ref_params Test32_main "Closure + Ref Params"
call :run_test test_33_interface_params Test33_main "Interface Params"
call :run_test test_34_varargs           Test34_main  "Varargs"
call :run_test test_35_for_multi_init   Test35_main  "For Multi-Init"
call :run_test test_36_const            Test36_main  "Const Variables"
call :run_test test_37_pipe_methods    Test37_main  "Pipe Methods"
call :run_test test_38_bare_fields    Test38_main  "Bare Field Access"
call :run_test test_39_do_while      Test39_main  "Do-While Loop"
call :run_test test_40_covariant_returns Test40_main "Covariant Return Types"
call :run_test test_41_typedef          Test41_main  "Typedef / Type Alias"
call :run_test test_42_labeled_loops   Test42_main  "Labeled Break/Continue"
call :run_test test_43_copy_constructors Test43_main "Copy Constructors"
call :run_test test_44_overloading       Test44_main  "Function Overloading"
call :run_test test_45_move_semantics    Test45_main  "Move Semantics"
call :run_test test_46_temp_closure_fix  Test46_main  "Temp Closure Fix"
call :run_test test_47_self_capture_safety Test47_main "Self-Capture Safety"
call :run_test test_48_ref_capture_escape Test48_main "Ref Capture Escape Analysis"
call :run_test test_49_weak_captures Test49_main "Weak Captures"
call :run_test test_50_shared_pointers Test50_main "Shared Pointers"
call :run_test test_51_arrays Test51_main "Arrays"
call :run_test test_52_tier1_fixes Test52_main "Tier 1 Fixes"
call :run_test test_53_forward_refs Test53_main "Forward Type References"
call :run_test test_54_float_suffix Test54_main "Float Literal Suffix"
call :run_test test_55_ctor_overloading Test55_main "Constructor Overloading"
call :run_test test_56_definite_assign Test56_main "Definite Assignment"
call :run_test test_57_string_type Test57_main "String Type"
call :run_error_test test_58_error_recovery "Error Recovery"
call :run_debug_test test_59_debug_info Test59_main "Debug Info"
call :run_test test_60_ffi Test60_main "C FFI"
call :run_test test_61_integer_types Test61_main "Integer Types"
call :run_test test_62_ffi_phase1 Test62_main "FFI Phase 1"
call :run_test test_63_globals Test63_main "Module Globals"
call :run_test test_64_extern_globals Test64_main "Extern Globals"
call :run_test test_65_unions Test65_main "Unions"

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
.\mingus_v2_tool.exe %FILE%.mingus --emit %FILE%.ll --entry %ENTRY% --opt 2 >nul 2>&1
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

:run_error_test
set /a TOTAL+=1
set FILE=%~1
set DESC=%~2

echo -------- %DESC% [%FILE%.mingus] --------

:: Step 1: Run compiler — expect it to FAIL (nonzero exit)
.\mingus_v2_tool.exe %FILE%.mingus --emit %FILE%.ll --entry Test58_main --opt 2 >nul 2>%FILE%.actual
if not errorlevel 1 (
    echo   FAIL: expected compilation to fail, but it succeeded
    set /a FAIL+=1
    echo.
    goto :eof
)

:: Step 2: Check that all expected error patterns appear
if exist %FILE%.errors (
    set ERROR_MISSING=0
    for /f "usebackq delims=" %%P in (%FILE%.errors) do (
        findstr /c:"%%P" %FILE%.actual >nul 2>&1
        if errorlevel 1 (
            echo   FAIL: missing expected error: %%P
            set ERROR_MISSING=1
        )
    )
    if !ERROR_MISSING! equ 0 (
        echo   PASS
        set /a PASS+=1
    ) else (
        echo   --- Actual errors ---
        type %FILE%.actual
        echo   --------------------
        set /a FAIL+=1
    )
) else (
    echo   WARN: no .errors file
    set /a PASS+=1
    set /a WARN+=1
)
echo.
goto :eof

:run_debug_test
set /a TOTAL+=1
set FILE=%~1
set ENTRY=%~2
set DESC=%~3

echo -------- %DESC% [%FILE%.mingus] --------

:: Step 1: Generate IR with --debug --opt 0
.\mingus_v2_tool.exe %FILE%.mingus --emit %FILE%.ll --entry %ENTRY% --debug --opt 0 >nul 2>&1
if errorlevel 1 (
    echo   FAIL: IR generation failed
    set /a FAIL+=1
    echo.
    goto :eof
)

:: Step 2: Check for debug metadata in IR
findstr /c:"!dbg" %FILE%.ll >nul 2>&1
if errorlevel 1 (
    echo   FAIL: no !dbg metadata found in IR
    set /a FAIL+=1
    echo.
    goto :eof
)

:: Step 3: Compile with clang -g
clang %FILE%.ll -o %FILE%.exe -g 2>nul
if errorlevel 1 (
    echo   FAIL: clang compilation failed
    set /a FAIL+=1
    echo.
    goto :eof
)

:: Step 4: Run and capture output
.\%FILE%.exe > %FILE%.actual 2>&1

:: Step 5: Compare against expected output
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
