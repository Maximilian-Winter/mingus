# Getting Started

Every Mingus program lives inside a **module**. A module contains function declarations, type definitions, and a `main` function that serves as the entry point.

## Your First Program

```mingus
module Hello
{
    extern func puts(string s) => int;

    func main() => int
    {
        puts("Hello, Mingus!");
        return 0;
    }
}
```

**Output:**
```
Hello, Mingus!
```

Key elements:
- `module Hello { ... }` declares a module named `Hello`
- `extern func puts(...)` imports the C standard library function for printing
- `func main() => int` is the entry point; it returns an integer exit code
- `return 0` signals successful completion

## Variables and Type Inference

Declare variables with `var` (type inferred) or with an explicit type:

```mingus
var x = 10;          // inferred as int
var name = "Alice";  // inferred as string
var pi = 3.14159;    // inferred as double

int count = 0;       // explicit type
double rate = 0.05;  // explicit type
bool done = false;   // explicit type
```

Variables declared with `var` take their type from the initializer. Once declared, a variable's type is fixed.

## Constants

Use `const` to declare values that cannot be reassigned:

```mingus
const int MAX_SIZE = 1024;
const pi = 3.14159;           // type inferred
const string GREETING = "Hi";
```

Constants can be declared at module level (accessible throughout the module) or inside functions.

## Basic Arithmetic

Mingus supports standard arithmetic operators on integers and floating-point numbers:

| Operator | Meaning | Example |
|----------|---------|---------|
| `+` | Addition | `10 + 20` |
| `-` | Subtraction | `20 - 10` |
| `*` | Multiplication | `10 * 20` |
| `/` | Division | `20 / 10` |
| `%` | Modulo (remainder) | `20 % 3` |
| `++` | Increment | `count++` |
| `--` | Decrement | `count--` |

### Example

```mingus
module Arithmetic
{
    extern func printf(string fmt, ...) => int;

    func main() => int
    {
        var x = 10;
        var y = 20;

        printf("10 + 20 = %d\n", x + y);
        printf("20 - 10 = %d\n", y - x);
        printf("10 * 20 = %d\n", x * y);
        printf("20 / 10 = %d\n", y / x);
        printf("20 %% 3  = %d\n", y % 3);

        return 0;
    }
}
```

**Output:**
```
10 + 20 = 30
20 - 10 = 10
10 * 20 = 200
20 / 10 = 2
20 % 3  = 2
```

## Comparison and Logical Operators

| Operator | Meaning |
|----------|---------|
| `==` | Equal |
| `!=` | Not equal |
| `<`, `>` | Less than, greater than |
| `<=`, `>=` | Less or equal, greater or equal |
| `&&` | Logical AND |
| `\|\|` | Logical OR |
| `!` | Logical NOT |

## Output

Mingus does not have a built-in print statement. Instead, you use C standard library functions via `extern` declarations:

```mingus
extern func printf(string fmt, ...) => int;  // Formatted output
extern func puts(string s) => int;           // Print a line
```

Common `printf` format specifiers:

| Specifier | Type | Example |
|-----------|------|---------|
| `%d` | int | `printf("%d", 42)` |
| `%u` | unsigned int | `printf("%u", 42)` |
| `%f` | double | `printf("%f", 3.14)` |
| `%.2f` | double (2 decimals) | `printf("%.2f", 3.14)` |
| `%s` | string | `printf("%s", name)` |
| `%lld` | long (64-bit signed) | `printf("%lld", bigNum)` |
| `%llu` | ulong (64-bit unsigned) | `printf("%llu", bigNum)` |

## Compiling and Running

```bash
# Compile Mingus source to LLVM IR
mingus_ir_tool.exe hello.mingus --emit hello.ll --entry Hello_main --opt 2

# Link and produce executable
clang -O2 -o hello.exe hello.ll

# Run
./hello.exe
```

The `--entry` flag specifies the entry point as `ModuleName_main`. The `--opt` flag sets the optimization level (0-2).

### Compiler Flags

| Flag | Description |
|------|-------------|
| `--emit <file.ll>` | Output LLVM IR to file |
| `--entry <name>` | Set entry point (format: `ModuleName_main`) |
| `--opt <0\|1\|2>` | Optimization level |
| `--debug` / `-g` | Emit debug information (DWARF 4) |
| `--link <lib>` | Link against a library |
| `--lib-path <dir>` | Add library search path |
| `--include-path <dir>` | Add include path |

### Compiler Diagnostics

The compiler warns about common mistakes at compile time:

- **Uninitialized variable use**: `warning: variable 'x' may be used before being assigned`
- **Type mismatches**: Reported as errors with file location
- **Multiple errors per compilation**: Up to 20 errors collected across all analysis passes

## See Also

- [Types and Values](02_types_and_values.md) for the full type system
- [Control Flow](03_control_flow.md) for if/else, loops, and switch
- [Functions](04_functions.md) for defining your own functions
- [C FFI](14_c_ffi.md) for details on `extern` declarations
