# Mingus

A compiled systems programming language that combines low-level control with expressive high-level abstractions. Named after Charles Mingus — bold, structured, uncompromising.

Mingus compiles to native code via LLVM. It has pipes for data flow, pattern matching with guards, RAII resource management, closures, operator overloading, interfaces, inheritance with virtual dispatch, and raw blocks for when you need to get close to the metal.

Mingus is still in very early development and its syntax and features are subject to change.


```
func processSample(double sample) => double
{
    return sample
        |> applyGain(1.5)
        |> softClip;
}

func softClip(double sample) => double
{
    return match sample {
        var x if x > 1.0  => 1.0,
        var x if x < -1.0 => -1.0,
        var x => x - (x * x * x) / 3.0,
    };
}
```

## What It Looks Like

### Structs with operator overloading

```
struct Vec3
{
    double x;
    double y;
    double z;

    func operator+(Vec3 other) => Vec3
    {
        Vec3 result;
        result.x = this.x + other.x;
        result.y = this.y + other.y;
        result.z = this.z + other.z;
        return result;
    }

    func dot(Vec3 other) => double
    {
        return this.x * other.x + this.y * other.y + this.z * other.z;
    }

    func length() => double
    {
        return sqrt(this.dot(this));
    }
}
```

### Classes with RAII

```
class DynamicArray
{
    private int* data;
    private int size;
    private int capacity;

    constructor(int initialCapacity)
    {
        this.capacity = initialCapacity;
        this.size = 0;
        raw { this.data = (int*)malloc(initialCapacity * sizeof(int)); }
    }

    destructor
    {
        if (this.data != null)
        {
            raw { free((byte*)this.data); }
        }
    }

    func push(int value) => void
    {
        if (this.size >= this.capacity) { this.grow(); }
        raw { *(this.data + this.size) = value; }
        this.size++;
    }

    func operator[](int index) => int
    {
        raw { return *(this.data + index); }
    }
}
```

Destructors run automatically at scope exit. No garbage collector, no manual free — resources clean up when they go out of scope.

```
{
    var arr = DynamicArray(8);
    arr.push(10);
    arr.push(20);
    // arr.destructor called automatically here
}
```

### Closures and higher-order functions

```
func makeScaler(double factor) => (double) => double
{
    return [=](double x) => { return x * factor; };
}

func compose((double) => double f, (double) => double g) => (double) => double
{
    return [=](double x) => { return f(g(x)); };
}

var doubler = makeScaler(2.0);
var tripler = makeScaler(3.0);
var times6  = compose(doubler, tripler);

var result = 7.0 |> apply(times6);   // 42.0
```

Lambdas use C++ capture lists. `[=]` copies by value, `[&]` captures by reference, and you can mix them:

```
var total = 0;
var accumulate = [&total](int x) => {
    total = total + x;   // writes persist to outer variable
    return total;
};
accumulate(10);
accumulate(20);
// total is now 30

var scale = 2;
var mixed = [=, &total](int x) => {
    total = total + x * scale;  // scale frozen, total mutable
    return total;
};
```

### Reference parameters

```
func swap(int& a, int& b) => void
{
    var tmp = a;
    a = b;
    b = tmp;
}

var x = 10;
var y = 20;
swap(x, y);  // x=20, y=10
```

### Interfaces

```
interface Drawable
{
    func draw() => void;
}

interface Resizable
{
    func resize(int factor) => int;
}

class Circle : Drawable, Resizable
{
    int radius;
    constructor(int r) { this.radius = r; }
    func draw() => void { puts("Circle drawn"); }
    func resize(int factor) => int { return this.radius * factor; }
}

func renderAll(Drawable* d) => void
{
    d->draw();  // virtual dispatch through interface
}
```

### Enums and pattern matching

```
enum Wave : int
{
    Sine     = 0,
    Square   = 1,
    Triangle = 2,
    Saw      = 3,
}

func oscillator(int wave, double phase) => double
{
    return match wave {
        Wave.Sine     => sin(phase * twoPi()),
        Wave.Square   => phase < 0.5 ? 1.0 : -1.0,
        Wave.Triangle => phase < 0.5
            ? phase * 4.0 - 1.0
            : 3.0 - phase * 4.0,
        _ => phase * 2.0 - 1.0,
    };
}
```

### Raw blocks for unsafe operations

Safe by default. When you need pointer arithmetic, you ask for it explicitly:

```
raw
{
    var data = (int*)malloc(5 * sizeof(int));
    *(data + 0) = 100;
    *(data + 2) = 300;
    free((byte*)data);
}
```

Pointer dereference for assignment and pointer arithmetic only compile inside `raw` blocks. Address-of (`&`), null checks, and arrow access (`->`) work everywhere.

## Examples

The `examples/` directory contains 9 showcase programs demonstrating every major feature:

| Example | Features shown |
|---------|---------------|
| Audio Effects (DSP Pipeline) | Structs, operator overloading, closures, pipes, enums |
| AI State Machine | Enums, pattern matching, closures, structs |
| Iterator Pipeline | Higher-order functions, pipes, closures, composition |
| Expression Parser | Classes, RAII, raw blocks, recursion, enums |
| Custom Allocator | Raw blocks, pointer arithmetic, RAII, classes |
| Capture List Showcase | All capture modes (`[]`, `[=]`, `[&]`, `[x]`, `[&x]`, mixed), composition |
| Data Structures (List & Stack) | Classes, `new`/`delete`, destructors, ref params, RAII |
| Particle Simulation | Structs, classes, RAII, raw blocks, closures, ref params |
| **Mingus Groove** (Walking Bass) | **Multi-module imports**, classes, ADSR envelopes, pattern matching, `[=]`/`[&]` captures, ref params, raw blocks, WAV output |

Run them all: `cd examples && showcase.bat`

The `tools/` directory contains the compiler source and C++ API examples (see `tools/README.md`).

## Building

### Requirements

- **CMake** 3.20+
- **C++17 compiler** (MSVC 19+ on Windows, GCC 11+ or Clang 14+ on Linux/Mac)
- **LLVM 21** (development libraries and headers)
- **ANTLR4** C++ runtime
- **Clang** (from the LLVM distribution, for compiling generated `.ll` files to native)

### Steps

```bash
git clone https://github.com/Maximilian-Winter/mingus.git
cd mingus
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DLLVM_DIR=./extern/clang+llvm-21.1.8-x86_64-pc-windows-msvc/lib/cmake/llvm
cmake --build . --config Release
```

This produces `mingus_ir_tool.exe` (or `mingus_ir_tool` on Linux/Mac).

> **Windows:** MSVC is required. Run from a Visual Studio developer command prompt, or pass the appropriate generator to CMake (e.g. `-G "Visual Studio 17 2022"`). The bundled LLVM distribution targets `x86_64-pc-windows-msvc`.

### Compiling a Mingus program

```bash
# Generate LLVM IR
mingus_ir_tool.exe hello.mingus --emit hello.ll

# Compile to native executable
clang hello.ll -o hello.exe -O2

# Run
./hello.exe
```

### Running the test suite

```bash
# Full suite (30 feature tests + 21 stress tests)
run_tests.bat           # Windows — from project root

# Or individually:
cd tests
run_all_tests.bat       # Feature tests only
run_stress_tests.bat    # Stress tests only
```

All 51 tests should pass. Use `--ir` to inspect generated LLVM IR, `--output` to see program output, or `--code` to display Mingus source.

## Feature Summary

| Feature | Status |
|---------|--------|
| Integer and float arithmetic | ✓ |
| Type inference (`var x = 42`) | ✓ |
| Control flow (if/else, for, while, switch) | ✓ |
| Functions with typed parameters and returns | ✓ |
| Structs with methods and operator overloading | ✓ |
| Classes with constructors and destructors (auto-generated if omitted) | ✓ |
| RAII (automatic destructor calls at scope exit) | ✓ |
| Heap allocation (`new` / `delete`) | ✓ |
| Inheritance with virtual dispatch | ✓ |
| Interfaces with multiple implementation | ✓ |
| Pipe operator (`\|>`) | ✓ |
| Pattern matching with guards | ✓ |
| Enums with underlying types | ✓ |
| Lambdas with C++ capture lists (`[=]`, `[&]`, `[x, &y]`) | ✓ |
| By-reference captures — writes persist to outer scope | ✓ |
| Reference parameters (`func swap(int& a, int& b)`) | ✓ |
| Higher-order functions and composition | ✓ |
| Nullable closures (`(int) => int f = null;`) | ✓ |
| Lambda literal assignment (`f = [=](int x) => { ... };`) | ✓ |
| Pointers and raw blocks | ✓ |
| Fixed-size arrays | ✓ |
| String operations (concat, compare, length, substring) | ✓ |
| String interpolation (`"value=${x}"`) | ✓ |
| C interop via `extern` declarations | ✓ |
| Multi-module imports | ✓ |
| Tuples and destructuring | ✓ |
| DynamicArray.map with lambda+pipe | ✓ |
| Complex number operator arithmetic | ✓ |
| Hex, binary, octal integer literals | ✓ |

## Architecture

```
Source (.mingus)
  → ANTLR4 Lexer/Parser
  → AST (62 node types)
  → Semantic Analysis (4 passes)
      Pass 1: Symbol table building (+ auto-generated constructors/destructors)
      Pass 2: Type resolution
      Pass 3: Type checking + overload resolution
      Pass 4: RAII analysis + control flow validation
  → LLVM IR Generation
  → Clang → Native executable
```

## Known Limitations

- **No generics** — no template or generic type support yet.
- **Strings are heap-allocated** — no small string optimization.
- **Single compilation unit** — each `.mingus` file compiles independently. Cross-file linking uses `import`.
- **Error recovery is minimal** — the first parse or semantic error often stops compilation. Error messages lack detailed context.
- **Temporary closure leak** — closures passed directly as function arguments without variable storage leak one refcount.
- **Reference lifetime** — `[&x]` captures that escape their scope produce dangling references (programmer responsibility, same as C++).


# Detailed Current Status
Under docs/MINGUS_STATUS.md is a detailed report about the current limitations and issues, with short- and long-term goals.


## Why "Mingus"?

Charles Mingus composed music that was technically rigorous and emotionally unrestrained at the same time. He demanded discipline from his musicians but insisted they improvise wildly within the structure. The language follows the same philosophy: strict types and RAII provide the structure, while pipes, closures, and pattern matching give you freedom to express solutions naturally.

> "Making the simple complicated is commonplace; making the complicated simple, awesomely simple, that's creativity." — Charles Mingus

## License

MIT
