# mingus_ir_tool — User Guide

The Mingus IR tool is the compiler driver for Mingus v1. It takes a `.mingus` source file through the full pipeline: parsing, semantic analysis, LLVM IR generation, and verification. It can also compile and execute the result.

---

## Usage

```
mingus_ir_tool <source.mingus> [options]
```

### Options

| Flag | Description |
|------|-------------|
| *(no flags)* | Parse, analyze, generate IR, print IR to stdout, verify |
| `--emit <file.ll>` | Write the LLVM IR to a `.ll` file |
| `--entry <Module_func>` | Inject a C `main()` wrapper that calls the named function |
| `--run <Module_func>` | Like `--entry`, but also compile with clang and execute |
| `--opt <level>` | Optimization level: `0` (none, default), `1` (O1), `2` (O2) |

Options can be combined freely.

---

## Function Naming Convention

Mingus mangles function names as `Module_function`. When using `--entry` or `--run`, you must use the **mangled name**:

```mingus
module MyApp
{
    func main() => int { ... }
}
```

The mangled name is `MyApp_main`. Use it as:

```
mingus_ir_tool myapp.mingus --entry MyApp_main
```

---

## Examples

### Inspect the LLVM IR

Just pass the source file with no flags. The full IR is printed to stdout:

```
mingus_ir_tool hello.mingus
```

Output:
```
=== Parsing: hello.mingus ===
=== Running semantic analysis ===
Semantic analysis: 0 errors, 0 warnings

=== Generating LLVM IR ===

; ModuleID = 'mingus_module'
%Vec3 = type { double, double, double }
define %Vec3 @Math_operator_add(ptr %this, ptr %other) {
  ...
}

=== Verifying LLVM IR ===
LLVM IR verification passed.
```

### Save IR to a file

```
mingus_ir_tool hello.mingus --emit hello.ll
```

This prints the IR to stdout **and** writes it to `hello.ll`.

### Generate an executable (two-step)

**Step 1:** Generate IR with a `main()` entry point and O2 optimization:

```
mingus_ir_tool myapp.mingus --emit myapp.ll --entry MyApp_main --opt 2
```

This injects a C-compatible `main()` function that calls `MyApp_main()`, then writes the IR to `myapp.ll`. The IR is **not** printed to stdout when `--entry` is used.

**Step 2:** Compile with clang:

```
clang myapp.ll -o myapp.exe -O2
```

**Step 3:** Run:

```
myapp.exe
```

### Optimization

```
mingus_ir_tool myapp.mingus --emit myapp.ll --entry MyApp_main --opt 0   # no optimization (default)
mingus_ir_tool myapp.mingus --emit myapp.ll --entry MyApp_main --opt 1   # O1: mem2reg, basic simplifications
mingus_ir_tool myapp.mingus --emit myapp.ll --entry MyApp_main --opt 2   # O2: inlining, GVN, SROA, vectorization
```

Optimization runs between IR generation and LLVM verification. The test suite always uses `--opt 2`.

---

### One-shot compile and run

```
mingus_ir_tool myapp.mingus --run MyApp_main --opt 2
```

This does everything in one step:
1. Parse and analyze
2. Generate IR with `main()` wrapper
3. Write IR to `mingus_output.ll` (or the `--emit` path if given)
4. Find clang (checks PATH, then looks near the executable)
5. Compile with `clang -O2`
6. Execute the result

You can combine `--run` with `--emit` to control the output filename:

```
mingus_ir_tool myapp.mingus --run MyApp_main --emit myapp.ll
```

---

## Pipeline Steps

The tool runs these steps in order. If any step fails, it stops and returns a non-zero exit code.

```
1. Parse           Read source, run ANTLR4 lexer and parser, build AST
2. Sema            Four semantic analysis passes:
                     Pass 1: SymbolTableBuilder — scopes and symbols
                     Pass 2: TypeResolver — resolve type references
                     Pass 3: TypeChecker — expression types, overloads
                     Pass 4: SemanticValidator — RAII, control flow
3. Codegen         Walk annotated AST, emit LLVM IR
4. Entry wrapper   (if --entry or --run) Inject main() calling the entry function
5. Print IR        (if no --entry/--run) Print IR to stdout
6. Verify          Run LLVM module verifier
7. Emit            (if --emit or --entry/--run) Write IR to .ll file
8. Compile + Run   (if --run only) Invoke clang, execute result
```

---

## Writing a Mingus Program

A minimal program that can be compiled and executed:

```mingus
module Hello
{
    extern func puts(string s) => int;

    func main() => int
    {
        puts("Hello from Mingus!");
        return 0;
    }
}
```

Compile and run:

```
mingus_ir_tool hello.mingus --run Hello_main
```

Or step by step:

```
mingus_ir_tool hello.mingus --emit hello.ll --entry Hello_main
clang hello.ll -o hello.exe -O2
hello.exe
```

### Using printf

Mingus declares printf as a varargs function automatically (detected by name). Declare it with the format string parameter:

```mingus
module Demo
{
    extern func printf(string fmt, int val) => int;

    func main() => int
    {
        printf("The answer is %d\n", 42);
        return 0;
    }
}
```

Note: declare a separate `printf` extern for each signature you use. The first parameter is always the format string; additional parameters determine the types passed to the call. Mingus handles the varargs lowering internally.

---

## Batch Testing

The test suite uses `--entry` + `--emit` so the batch script controls compilation separately:

```bat
:: Generate IR with main wrapper and O2 optimization
mingus_ir_tool.exe %FILE%.mingus --emit %FILE%.ll --entry %ENTRY% --opt 2

:: Compile with clang
clang %FILE%.ll -o %FILE%.exe -O2

:: Run
%FILE%.exe
```

This is the pattern used by `run_all_tests.bat`. Using `--entry` (not `--run`) gives the batch script control over which clang to use and how to handle errors at each stage.

---

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Parse error, semantic error, verification failure, or compilation failure |

---

## Troubleshooting

**"entry function 'X' not found in module"**
The function name doesn't match. Check the mangling: it's `ModuleName_functionName`. The tool prints all available functions when this happens.

**LNK1561: entry point must be defined**
The `.ll` file has no `main()` function. Make sure you used `--entry` or `--run` when generating the IR.

**Semantic errors**
The tool prints all semantic errors with line and column numbers. Fix the source and re-run.

**Clang not found (--run only)**
The `--run` flag needs clang in your PATH or in the LLVM distribution directory near the executable. Use the two-step approach (`--entry` + manual clang) if auto-detection fails.
