@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64
cd /d H:\language_dev\mingus\build
cmake --build . --config Release
