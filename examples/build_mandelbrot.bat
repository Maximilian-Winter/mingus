@echo off
:: Build the Mandelbrot PNG example
:: Demonstrates Mingus floating-point computation with C FFI (stb_image_write)

echo === Building Mandelbrot PNG Example ===
echo.

:: Step 1: Compile stb_image_write implementation to object file
echo [1/3] Compiling stb_image_write...
clang -c extern_c\stb_impl.c -o extern_c\stb_impl.obj -O2
if errorlevel 1 (
    echo FAIL: could not compile stb_impl.c
    exit /b 1
)

:: Step 2: Compile Mingus source to LLVM IR
echo [2/3] Compiling Mingus source...
.\mingus_v2_tool.exe example_11_mandelbrot.mingus --emit mandelbrot.ll --entry Mandelbrot_main --opt 2
if errorlevel 1 (
    echo FAIL: Mingus compilation failed
    exit /b 1
)

:: Step 3: Link LLVM IR with stb object file
echo [3/3] Linking with clang...
clang -O2 -o mandelbrot.exe mandelbrot.ll extern_c\stb_impl.obj
if errorlevel 1 (
    echo FAIL: clang linking failed
    exit /b 1
)

echo.
echo === Build successful! ===
echo.

:: Run the example
echo Running mandelbrot.exe...
echo.
.\mandelbrot.exe

:: Cleanup build artifacts
del mandelbrot.ll 2>nul
