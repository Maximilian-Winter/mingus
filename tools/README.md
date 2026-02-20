# Mingus Tools

This directory contains the C++ source for the Mingus compiler toolchain and supporting API examples. The main build produces `mingus_ir_tool.exe`, which is automatically copied to `examples/` and `tests/` by CMake.

## Tools

| Source | What it does |
|--------|-------------|
| `mingus_ir_tool.cpp` | **The compiler.** Full pipeline: parse, sema, LLVM IR gen, optional compile+run. |
| `mingus_sema_tool.cpp` | Dumps the semantic analysis output (symbol tables, types, scopes). |
| `mingus_ast_tool.cpp` | Dumps the raw AST from a parsed `.mingus` file. |

See [TOOL_GUIDE.md](TOOL_GUIDE.md) for detailed `mingus_ir_tool` usage (flags, entry points, optimization levels).

## API Examples

| Source | What it demonstrates |
|--------|---------------------|
| `simple_example.cpp` | Constructs a minimal AST programmatically and prints it. |
| `factorial_example.cpp` | Builds a factorial function AST by hand, generates LLVM IR. |
| `parser_example.cpp` | Parses `hello.mingus` or `mingus_v1_samples.mingus` and prints the AST. |

## Building

These are built automatically as part of the main Mingus CMake build:

```bash
cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DLLVM_DIR=./extern/clang+llvm-21.1.8-x86_64-pc-windows-msvc/lib/cmake/llvm
cmake --build . --config Release
```

To skip building tools, pass `-DBUILD_TOOLS=OFF` to CMake.

After a successful build, `mingus_ir_tool.exe` is copied to:
- `examples/mingus_ir_tool.exe` — for running the showcase
- `tests/mingus_ir_tool.exe` — for the test suite

## Support Files

- `hello.mingus` — minimal Mingus source used by `parser_example`
- `mingus_v1_samples.mingus` — comprehensive sample file exercising most language features
