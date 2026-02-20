@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64
cd /d H:\language_dev\mingus\tests

echo === Test 04 ===
..\examples\mingus_ir_tool.exe stress_04_early_return_raii.mingus --emit stress_04.ll --entry Stress04_main --opt 2

echo.
echo === Test 06 ===
..\examples\mingus_ir_tool.exe stress_06_recursive_match.mingus --emit stress_06.ll --entry Stress06_main --opt 2

echo.
echo === Test 08 ===
..\examples\mingus_ir_tool.exe stress_08_destructor_closure.mingus --emit stress_08.ll --entry Stress08_main --opt 2
