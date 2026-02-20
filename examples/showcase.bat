@echo off
setlocal enabledelayedexpansion

:: Run all Mingus example programs
:: Usage: showcase.bat [options]
::
:: Options:
::   --code     Print Mingus source code for each example
::   --ir       Print generated LLVM IR for each example
::   --output   Print program output for each example (default)
::   --help     Show this help

:: Parse options
set SHOW_CODE=0
set SHOW_IR=0
set SHOW_OUTPUT=1

:parse_args
if "%~1"=="" goto :args_done
if "%~1"=="--code" (set SHOW_CODE=1& shift & goto :parse_args)
if "%~1"=="--ir" (set SHOW_IR=1& shift & goto :parse_args)
if "%~1"=="--output" (set SHOW_OUTPUT=1& shift & goto :parse_args)
if "%~1"=="--help" goto :usage
shift
goto :parse_args
:args_done

echo ============================================================
echo  Mingus Examples Showcase
echo ============================================================
echo.

:: Track results
set SUCCESS=0
set FAILED=0
set TOTAL=0

call :run_example example_01_audio_effects    AudioEffects_main      "Audio Effects (DSP Pipeline)"
call :run_example example_02_state_machine    StateMachine_main      "AI State Machine"
call :run_example example_03_iterator_pipeline IteratorPipeline_main "Iterator Pipeline"
call :run_example example_04_expression_parser ExpressionParser_main "Expression Parser"
call :run_example example_05_custom_allocator  CustomAllocator_main   "Custom Allocator"
call :run_example example_06_capture_showcase  CaptureShowcase_main   "Capture List Showcase"
call :run_example example_07_data_structures   DataStructures_main    "Data Structures - List and Stack"
call :run_example example_08_particle_sim      ParticleSim_main       "Particle Simulation"
call :run_example example_09_mingus_groove     MingusGroove_main      "Mingus Groove - Walking Bass"

echo.
echo ============================================================
if !FAILED! gtr 0 (
    echo  Results: !SUCCESS! succeeded, !FAILED! FAILED out of !TOTAL! examples
) else (
    echo  Results: !SUCCESS! succeeded, !FAILED! failed out of !TOTAL! examples
)
echo ============================================================

:: Clean up temporary files
del example_*.exe example_*.ll example_*.actual 2>nul

if !FAILED! gtr 0 exit /b 1
exit /b 0

:usage
echo Usage: showcase.bat [options]
echo.
echo Options:
echo   --code     Print Mingus source code for each example
echo   --ir       Print generated LLVM IR for each example
echo   --output   Print program output for each example (default)
echo   --help     Show this help
exit /b 0

:run_example
set /a TOTAL+=1
set FILE=%~1
set ENTRY=%~2
set DESC=%~3

echo -------- %DESC% [%FILE%.mingus] --------
echo.

if "!SHOW_CODE!"=="1" (
    echo   --- Source ---
    type %FILE%.mingus
    echo.
    echo   -------------
)

:: Step 1: Generate IR with main wrapper
.\mingus_ir_tool.exe %FILE%.mingus --emit %FILE%.ll --entry %ENTRY% --opt 2 >nul 2>&1
if errorlevel 1 (
    echo   FAIL: IR generation failed
    set /a FAILED+=1
    echo.
    goto :eof
)

if "!SHOW_IR!"=="1" (
    echo   --- LLVM IR ---
    type %FILE%.ll
    echo.
    echo   ---------------
)

:: Step 2: Compile with clang
clang %FILE%.ll -o %FILE%.exe -O2 2>nul
if errorlevel 1 (
    echo   FAIL: clang compilation failed
    set /a FAILED+=1
    echo.
    goto :eof
)

:: Step 3: Run and capture output
.\%FILE%.exe > %FILE%.actual 2>&1

if "!SHOW_OUTPUT!"=="1" (
    echo   --- Output ---
    type %FILE%.actual
    echo   --------------
)

echo   SUCCESS
echo.
set /a SUCCESS+=1
goto :eof
