@echo off
setlocal enabledelayedexpansion

:: Mingus v1 Showcase — display source code and program output
:: Usage: showcase.bat [path_to_clang]
::
:: Run from the directory containing mingus_ir_tool.exe and the .mingus files

set CLANG=%1
if "%CLANG%"=="" set CLANG=clang

echo.
echo ################################################################
echo #                                                              #
echo #              M I N G U S   v1   S H O W C A S E             #
echo #                                                              #
echo #        A programming language for Charles Mingus             #
echo #                                                              #
echo ################################################################
echo.

call :showcase test_01_basics       Test01_main  "Basics"
call :showcase test_02_structs_operators Test02_main "Structs and Operators"
call :showcase test_03_classes_raii  Test03_main  "Classes and RAII"
call :showcase test_04_pipes_match  Test04_main  "Pipes and Match"
call :showcase test_05_enums_switch Test05_main  "Enums and Switch"
call :showcase test_06_lambdas_funcptr Test06_main "Lambdas and Func Ptrs"
call :showcase test_07_floats_math  Test07_main  "Floats and Math"
call :showcase test_08_pointers_raw Test08_main  "Pointers and Raw"
call :showcase test_09_enum_expressions Test09_main "Enum Expressions"
call :showcase test_10_closures     Test10_main  "Closures"
call :showcase test_11_dsp_showcase Test11_main  "DSP Showcase"
call :showcase test_12_imports     Test12_main  "Imports"
call :showcase test_13_inheritance Test13_main  "Inheritance"
call :showcase test_14_strings    Test14_main  "String Operations"
call :showcase test_15_interfaces Test15_main "Interfaces"
call :showcase test_16_dsp_wav        DspWav_main   "DSP WAV Synthesis"
call :showcase test_17_hex_literals   HexTest_main  "Hex Binary Octal Literals"

echo.
echo ################################################################
echo #                    Showcase complete.                        #
echo ################################################################

:: Clean up
del test_*.exe test_*.ll 2>nul
goto :eof

:showcase
set FILE=%~1
set ENTRY=%~2
set DESC=%~3

echo ============================================================
echo  %DESC%
echo ============================================================
echo.
echo  --- Source: %FILE%.mingus ---
echo.
type %FILE%.mingus
echo.

:: Compile and run
.\mingus_ir_tool.exe %FILE%.mingus --emit %FILE%.ll --entry %ENTRY% --opt 2 >nul 2>&1
if errorlevel 1 (
    echo  [ERROR: IR generation failed]
    echo.
    goto :eof
)

%CLANG% %FILE%.ll -o %FILE%.exe -O2 2>nul
if errorlevel 1 (
    echo  [ERROR: clang compilation failed]
    echo.
    goto :eof
)

echo  --- Output ---
echo.
.\%FILE%.exe 2>&1
echo.
echo.
goto :eof
