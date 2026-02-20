@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64
cd /d H:\language_dev\mingus\tests

echo === Test 04 WITHOUT optimization ===
..\examples\mingus_ir_tool.exe stress_04_early_return_raii.mingus --emit stress_04_noopt.ll --entry Stress04_main --opt 0
clang stress_04_noopt.ll -o stress_04_noopt.exe -O0
.\stress_04_noopt.exe

echo.
echo === Test 04 WITH optimization ===
..\examples\mingus_ir_tool.exe stress_04_early_return_raii.mingus --emit stress_04_opt.ll --entry Stress04_main --opt 2
clang stress_04_opt.ll -o stress_04_opt.exe -O2
.\stress_04_opt.exe

echo.
echo === Test 08 WITHOUT optimization ===
..\examples\mingus_ir_tool.exe stress_08_destructor_closure.mingus --emit stress_08_noopt.ll --entry Stress08_main --opt 0
clang stress_08_noopt.ll -o stress_08_noopt.exe -O0
.\stress_08_noopt.exe
