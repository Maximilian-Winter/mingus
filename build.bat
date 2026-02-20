@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64

cd /d H:\language_dev\mingus

if not exist build (
    mkdir build
    cd build
    cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DLLVM_DIR=./extern/clang+llvm-21.1.8-x86_64-pc-windows-msvc/lib/cmake/llvm
) else (
    cd build
)

cmake --build . --config Release
