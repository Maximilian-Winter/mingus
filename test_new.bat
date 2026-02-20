@echo off
cd /d H:\language_dev\mingus\tests
set TOOL=.\mingus_ir_tool.exe

echo === stress_10 debug ===
%TOOL% stress_10_closure_in_struct.mingus --emit stress_10_closure_in_struct.ll --entry Stress10_main --opt 2 >nul 2>&1
clang stress_10_closure_in_struct.ll -o stress_10_closure_in_struct.exe -O2 2>nul
echo Running...
.\stress_10_closure_in_struct.exe 2>&1
echo exitcode=%errorlevel%

echo.
echo === stress_11 debug ===
%TOOL% stress_11_closure_in_class.mingus --emit stress_11_closure_in_class.ll --entry Stress11_main --opt 2 2>&1
echo exitcode=%errorlevel%

del /q stress_*.exe stress_*.ll 2>nul
