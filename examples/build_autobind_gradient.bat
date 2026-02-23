@echo off
:: Build the Gradient PNG example using auto-generated bindings
:: Demonstrates the full pipeline: generate bindings → compile → link → run

echo === Building Gradient PNG (Auto-Generated Bindings) ===
echo.

:: Step 1: Auto-generate Mingus bindings from stb_image_write.h
echo [1/4] Generating Mingus bindings from stb_image_write.h...
python ..\mingus_bind_gen.py extern_c\stb_image_write.h --module StbImageWrite -o StbImageWrite.mingus
if errorlevel 1 (
    echo FAIL: binding generation failed
    exit /b 1
)
echo       Generated StbImageWrite.mingus

:: Step 2: Compile stb_image_write implementation to object file
echo [2/4] Compiling stb_image_write...
clang -c extern_c\stb_impl.c -o extern_c\stb_impl.obj -O2
if errorlevel 1 (
    echo FAIL: could not compile stb_impl.c
    exit /b 1
)

:: Step 3: Compile Mingus source (auto-imports StbImageWrite.mingus)
echo [3/4] Compiling Mingus source...
.\mingus_v2_tool.exe example_12_autobind_gradient.mingus --emit gradient.ll --entry Gradient_main --opt 2
if errorlevel 1 (
    echo FAIL: Mingus compilation failed
    exit /b 1
)

:: Step 4: Link LLVM IR with stb object file
echo [4/4] Linking with clang...
clang -O2 -o gradient.exe gradient.ll extern_c\stb_impl.obj
if errorlevel 1 (
    echo FAIL: clang linking failed
    exit /b 1
)

echo.
echo === Build successful! ===
echo.

:: Run the example
echo Running gradient.exe...
echo.
.\gradient.exe

:: Cleanup build artifacts
del gradient.ll 2>nul

