@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64
cd /d H:\language_dev\mingus\tests
H:\language_dev\mingus\tests\run_stress_tests.bat
