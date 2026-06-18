# Mingus -- Language Status Report

**Date:** June 2026
**Status:** Compiles and executes optimized native binaries -- **74 feature tests + 21 stress tests passing (95/95)**

---

## 1. Overview

Mingus is a compiled systems programming language targeting LLVM IR. It combines familiar C/C++ syntax with modern features including closures with explicit capture lists, pattern matching, pipe operators, interfaces with fat-pointer dispatch, and automatic RAII-based resource management.

### Goals

- Provide a C-level systems language with modern ergonomics
- Support closures, pattern matching, and higher-order functions as first-class features
- Deliver deterministic memory management via RAII and reference counting (no garbage collector)
- Target LLVM IR for native performance with full optimization support

### Compiler Pipeline

```
Source (.mingus) -> ANTLR4 Parser -> AST -> Semantic Analysis (4 passes) -> LLVM IR -> Optimized IR -> Native Code
```

| Stage | Implementation |
|-------|----------------|
| Lexer/Parser | ANTLR4 grammar (MingusLexer.g4, MingusParser.g4) |
| AST | 69 node types with full visitor pattern |
| Sema Pass 1 | SymbolTableBuilder -- scopes, symbols, type declarations, import resolution |
| Sema Pass 2 | TypeResolver -- resolve all type references to TypeSymbol |
| Sema Pass 3 | TypeChecker -- expression types, overload resolution, access modifier enforcement |
| Sema Pass 4 | SemanticValidator -- RAII analysis, control flow validation, escape analysis, self-capture detection |
| Codegen | LLVM 21.1.8 IR generation via AST visitor, optional DIBuilder debug info |
| Optimization | LLVM PassBuilder with configurable O0/O1/O2 pipeline |
| Compilation | Clang (from LLVM distribution) compiles IR to native executable |

**Build System:** CMake + Ninja + MSVC (Windows), CLion IDE or standalone `build.bat`
**Test Runner:** `run_tests.bat` (combined feature + stress), `tests/run_v2_tests.bat` (74 feature), `tests/run_v2_stress_tests.bat` (21 stress) -- supports `--code`, `--ir`, `--output` flags
**Showcase:** `examples/showcase.bat` -- 14 example programs (including multi-module import demo)

---

## 2. Language Features

### 2.1 Basic Types

Mingus provides the following primitive types:

| Type | Description | LLVM Representation |
|------|-------------|---------------------|
| `int` | 32-bit signed integer | `i32` |
| `double` | 64-bit floating point | `double` |
| `float` | 32-bit floating point | `float` |
| `byte` | 8-bit unsigned integer | `i8` |
| `char` | 8-bit character | `i8` |
| `string` | Null-terminated string pointer | `ptr` |
| `bool` | Boolean value | `i1` |
| `void` | No return value | `void` |

**Literal formats:**
- Decimal integers: `42`, `-7`
- Hex literals: `0xFF`, `0x1A2B` (Test 17)
- Binary literals: `0b1010`, `0b11110000` (Test 17)
- Octal literals: `0o77`, `0o755` (Test 17)
- Floating-point: `3.14`, `-2.5`
- Strings: `"hello"`, `"value=${expr}"` (interpolation)
- Booleans: `true`, `false`
- Null: `null`

### 2.2 Variables and Constants

```mingus
// Explicit type
int x = 42;

// Type inference
var y = 10;
var name = "hello";

// Constants (immutable after initialization)
const int MAX = 100;
const pi = 3.14;          // inferred const
const greeting = "hello"; // const string

// Mutable variables can be reassigned
var count = 0;
count = count + 1;
```

Constants are declared with the `const` keyword and can use either explicit types or type inference. They are immutable after initialization (Test 36).

**Assignment operators:** `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`

**Increment/decrement:** `i++`, `++i`, `i--`, `--i`

### 2.3 Functions

```mingus
// Basic function with typed parameters and return type
func add(int a, int b) => int
{
    return a + b;
}

// Void return
func greet(string name) => void
{
    printf("Hello, %s!\n", name);
}

// Recursive functions
func factorial(int n) => int
{
    if (n <= 1) { return 1; }
    return n * factorial(n - 1);
}
```

**Extern declarations** for C interop, with full varargs support:

```mingus
// Single extern with varargs (... syntax)
extern func printf(string fmt, ...) => int;

// Grouped extern block
extern {
    func sin(double x) => double;
    func cos(double x) => double;
    func malloc(int size) => byte*;
    func free(byte* ptr) => void;
}
```

The `...` varargs syntax allows calling C variadic functions like `printf` with any number of extra arguments of any type (Test 34).

**Reference parameters** allow functions to modify the caller's variables:

```mingus
func swap(int& a, int& b) => void
{
    var tmp = a;
    a = b;
    b = tmp;
}

func increment(int& x) => void
{
    x = x + 1;
}

// Multiple output parameters
func divmod(int a, int b, int& quotient, int& remainder) => void
{
    quotient = a / b;
    remainder = a - (quotient * b);
}
```

Reference parameters pass a pointer to the caller's alloca. The callee reads and writes through this pointer, so modifications persist after the call returns (Tests 29, 32).

**Static methods** can be declared on both classes and structs:

```mingus
class MathUtils
{
    static func factorial(int n) => int
    {
        if (n <= 1) { return 1; }
        return n * MathUtils.factorial(n - 1);
    }
}

var result = MathUtils.factorial(5);  // 120
```

Static methods have no `this` pointer and are called via `ClassName.method()` syntax (Test 24).

### 2.4 Control Flow

**If/else chains:**

```mingus
if (x > 0)
{
    puts("positive");
}
else if (x == 0)
{
    puts("zero");
}
else
{
    puts("negative");
}
```

**For loops** with single or multiple initializers:

```mingus
// Single initializer
for (int i = 0; i < 10; i++)
{
    total = total + i;
}

// Multiple initializers (converging loops, etc.)
for (int i = 0, int j = 10; i < j; i = i + 1, j = j - 1)
{
    sum = sum + i + j;
}

// Mixed typed and inferred initializers
for (int x = 0, var y = 100; x < 5; x = x + 1)
{
    total = total + y;
    y = y - 10;
}
```

Multi-init for loops support both typed (`int i = 0`) and inferred (`var j = 10`) declarations, as well as multiple iterator expressions separated by commas (Test 35).

**While loops:**

```mingus
while (n < 100)
{
    n = n * 2;
    count++;
}
```

**Break and continue:**

```mingus
for (int i = 0; i < 100; i++)
{
    if (i % 2 == 0) { continue; }
    if (i > 50) { break; }
    total = total + i;
}
```

Break and continue correctly interact with RAII -- destructors for objects created inside the loop body are called when exiting via break or continue (Stress 13, 18).

**Switch statements:**

```mingus
switch (val)
{
    case 1: puts("one");
    case 2: puts("two");
    case 3: puts("three");
    default: puts("other");
}
```

Switch compiles to LLVM `switch` instruction for constant-case optimization (Test 05).

**Ternary expressions:**

```mingus
var abs_val = x > 0.0 ? x : -x;
```

### 2.5 Structs

Structs are value types with fields, methods, constructors, and operator overloading:

```mingus
struct Vec3
{
    double x;
    double y;
    double z;

    func length() => double
    {
        return sqrt(x * x + y * y + z * z);
    }

    func dot(Vec3 other) => double
    {
        return x * other.x + y * other.y + z * other.z;
    }

    operator+(Vec3 other) => Vec3
    {
        return Vec3 { x: x + other.x, y: y + other.y, z: z + other.z };
    }

    operator*(double s) => Vec3
    {
        return Vec3 { x: x * s, y: y * s, z: z * s };
    }

    operator[](int i) => double
    {
        // Index operator
    }

    static func origin() => Vec3
    {
        return Vec3 { x: 0.0, y: 0.0, z: 0.0 };
    }
}
```

**Overloadable operators:** `+`, `-`, `*`, `/`, `%`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `[]`

Operator chaining works naturally: `(a + b) * 2.5`, `a.cross(b).normalize()` (Test 02).

Struct methods can access fields directly by name without the `this.` prefix (bare field access), just like class methods. Local variables shadow field names when both exist in the same scope (Test 38).

All struct construction uses `zeroinitializer` to prevent undefined value propagation.

### 2.6 Classes

Classes support single inheritance, virtual dispatch, constructors, destructors, RAII, and access control:

```mingus
class Animal
{
    int legs;

    constructor(int l)
    {
        legs = l;
    }

    func speak() => void
    {
        puts("...");
    }

    func getLegs() => int
    {
        return legs;
    }
}

class Dog : Animal
{
    constructor() : super(4) { }

    func speak() => void
    {
        puts("Woof!");
    }
}
```

**Key class features:**

- **Single inheritance:** `class Dog : Animal { ... }` -- inherits all fields and methods
- **Virtual dispatch:** all non-static, non-constructor, non-destructor methods are virtual by default (Java-style). Dispatch through base pointer uses vtable lookup.
- **Virtual destructors:** destructor occupies vtable slot 0. `delete basePtr` dispatches through the vtable, ensuring the most-derived destructor runs (Test 23).
- **Constructor chaining:** `constructor() : super(4) { ... }` calls base constructor before vtable store.
- **Destructor chaining:** derived destructor body runs first, then base destructor called automatically.
- **Auto-generated constructors/destructors:** if a class omits either, SymbolTableBuilder injects a synthetic one with an empty body.
- **RAII:** stack-allocated class objects have their destructor called automatically at scope exit. Early return correctly triggers cleanup for all active RAII objects.
- **Heap allocation:** `new Dog()` allocates on the heap, `delete dog` dispatches virtual destructor and frees memory.
- **Arrow operator:** `node->left` for pointer member access.
- **Abstract classes:** `abstract class Shape { ... }` cannot be instantiated; concrete subclasses must override all abstract methods (Test 13).

**Bare field access:** Fields can be read and written by name alone within methods and constructors, without requiring the `this.` prefix. Inherited fields are also accessible this way. Local variables shadow field names when both exist in the same scope (Test 38).

```mingus
class Counter
{
    int count;

    constructor(int c)
    {
        count = c;       // bare field write (no this. needed)
    }

    func increment() => void
    {
        count = count + 1;  // bare field read + write
    }
}
```

**Access modifiers** control visibility of fields and methods:

```mingus
class Secret
{
    private int hidden;
    protected int family;
    public int open;       // public is the default

    private func internal() => void { }
    protected func inherited() => void { }
    func accessible() => void { }
}
```

- `private` -- accessible only within the declaring class
- `protected` -- accessible from the class and all derived classes (walks the `baseClass` chain)
- `public` (default) -- accessible from anywhere

Access modifiers are enforced by the TypeChecker on `MemberAccessExpression` for both fields and methods (Test 22).

### 2.7 Enums

Enums support underlying types (`int`, `byte`, `string`) and are used in match expressions and arithmetic:

```mingus
enum TokenKind : int
{
    Number     = 0,
    Identifier = 1,
    Plus       = 2,
    Minus      = 3,
}

enum HttpMethod : string
{
    Get  = "GET",
    Post = "POST",
}

enum Flags : byte
{
    Read  = 1,
    Write = 2,
    Exec  = 4,
}
```

Enum members are accessed via `EnumName.Member` syntax and can be used in match patterns, expressions, comparisons, and arithmetic (Tests 05, 09).

### 2.8 Tuples and Destructuring

Functions can return tuple types and callers can destructure them:

```mingus
func divmod(int a, int b) => (int, int)
{
    return (a / b, a % b);
}

(var quot, var rem) = divmod(17, 5);

// Mixed-type tuples
func classify(int n) => (string, int, bool) { ... }
(var label, var abs_val, var is_pos) = classify(42);
```

Tuples compile to LLVM struct types with `insertvalue`/`extractvalue` instructions. Works with recursive functions (fibonacci pair returning `(int, int)`) (Test 18).

### 2.9 Pipes

The pipe operator `|>` threads a value through a chain of transformations. Mingus supports piping into both free functions and method calls:

```mingus
// Pipe into free functions
var result = 5.0 |> double_it |> add_ten;

// Pipe into methods on an object (obj->method syntax)
Transform* t = new Transform(2.0, 1.0, 100.0, 0.0);
var r1 = 3.0 |> t->scale;
var r2 = 5.0 |> t->offset;

// Chained: pipe through method then another method
var r3 = 5.0 |> t->scale |> t->offset;

// Mixed: free function then method
var r4 = 5.0 |> double_it |> t->offset;

// Methods with extra arguments
var r5 = 3.0 |> t->apply(7.0);

// Pipe with closure arguments
var r6 = 10.0 |> apply(doubler);
var r7 = 7.0 |> apply([](double x) => { return x + 3.0; });
```

The piped value becomes the first argument to each stage. Extra arguments in parentheses are appended after it. The `obj->method` syntax calls the method on `obj` with the piped value as the first explicit argument (Tests 04, 37).

### 2.10 Match Expressions

Pattern matching with value semantics:

```mingus
var result = match kind {
    TokenKind.Plus  => 1,
    TokenKind.Minus => 1,
    TokenKind.Star  => 2,
    _ => 0,
};

// Binding patterns with guards
var clamped = match x {
    var v if v > 1.0 => 1.0,
    var v if v < -1.0 => -1.0,
    _ => x,
};

// Range patterns
var category = match score {
    0..59   => "fail",
    60..79  => "pass",
    80..100 => "excellent",
    _ => "unknown",
};
```

Match can be used both as an expression (returning a value) and as a statement. Supports literal patterns, enum patterns, wildcard (`_`), binding patterns with optional guards, and range patterns (`1..10`) (Tests 04, 05).

### 2.11 Lambdas and Closures

Mingus requires **mandatory C++ capture lists** on all lambdas:

```mingus
// No captures
var doubler = [](double x) => { return x * 2.0; };

// Capture all by value (copy at capture time)
var offset = 10;
var add_offset = [=](int x) => { return x + offset; };

// Capture all by reference (writes persist to outer scope)
var counter = 0;
var inc = [&]() => { counter = counter + 1; };

// Named captures
var scale = [multiplier](int x) => { return x * multiplier; };

// By-reference named capture
var total = 0;
var adder = [&total](int x) => { total = total + x; return total; };

// Mixed: all by value, specific by reference
var mixed = [=, &accum](int x) => { accum = accum + x * scale; return accum; };

// Mixed: all by reference, specific by value
var mixedRef = [&, b](int x) => { a = a + x; return a + b; };
```

**Capture modes:**

| Syntax | Meaning |
|--------|---------|
| `[]` | No captures (pure function) |
| `[=]` | All referenced outer variables captured by value |
| `[&]` | All referenced outer variables captured by reference |
| `[x]` | Specific variable `x` captured by value |
| `[&x]` | Specific variable `x` captured by reference |
| `[=, &x]` | All by value, except `x` by reference |
| `[&, x]` | All by reference, except `x` by value |

**Capture semantics:**
- **By-value** (`[=]`, `[x]`): freezes the variable's value at capture time. Mutations inside the lambda do not affect the outer variable.
- **By-reference** (`[&]`, `[&x]`): stores a pointer to the original variable's alloca. Reads and writes inside the lambda operate on the original variable, enabling stateful patterns (counters, accumulators, min/max trackers) (Tests 28, 30).

**Higher-order functions:**

```mingus
func apply(double x, (double) => double f) => double
{
    return f(x);
}

func compose((double) => double f, (double) => double g) => (double) => double
{
    return [=](double x) => { return f(g(x)); };
}
```

**Lambda literal assignment** allows reassigning closure variables:

```mingus
(int) => int f = [](int x) => { return x; };
f = [=](int x) => { return x * 2; };  // reassign
```

**Implementation details:**
- All function-typed values use fat pointer representation: `{ fnPtr, envPtr }` (two-pointer struct)
- All lambdas receive `ptr %env` as their final hidden parameter (uniform calling convention)
- Closure capture structs are heap-allocated with a reference-counted header: `{ i64 refcount, ptr cleanup_fn, ...fields }`
- Retain/release at assignment boundaries. Per-closure cleanup functions handle nested closures.
- Closures with struct params and ref params both work correctly (Tests 31, 32).

**Advanced closure features:**
- **Nullable closures:** `(int) => int f = null;` -- function-type variables can be null-initialized and compared: `f == null`, `f != null`, `null == f` (Test 21)
- **Closures in struct/class fields:** synthetic cleanup functions release closure fields at scope exit; class destructors auto-release closure fields (Stress 10, 11)
- **Escape analysis:** temporary closures passed directly as function arguments are RAII-wrapped to prevent leaks (Test 25)
- **Self-capturing closures:** letrec-style indirection for recursive closures: `(int) => int fib = [=](int n) => { return fib(n-1) + fib(n-2); };` (Test 26)
- **Nested capture propagation:** when inner lambdas reference outer-scope variables, all intermediate lambdas automatically capture them too (Test 28)

### 2.12 Interfaces

Interfaces define method contracts. Classes implement interfaces and can be used polymorphically via fat pointer dispatch:

```mingus
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

// Interface pointer (fat pointer: { objPtr, itablePtr })
Drawable* d = new Circle(10);
d->draw();

// Pass interface pointer to function
func renderAll(Drawable* d) => void
{
    d->draw();
}
```

- Classes can implement multiple interfaces: `class Circle : Drawable, Resizable { ... }`
- SemanticValidator enforces completeness: error if a class does not implement all interface methods
- Interface pointers compile to `{ ptr, ptr }` fat pointers (object pointer + itable pointer)
- Per-(class, interface) itable globals are compile-time constant `[N x ptr]` arrays
- `delete d` on interface pointer correctly extracts and frees the underlying object
- Interface parameters: passing a `Dog*` where `Printable*` is expected automatically wraps to fat pointer via `emitWrapToInterfacePtr()` (Tests 15, 33)

### 2.13 Pointers and Raw Blocks

```mingus
// Pointer types
int* p = &x;
*p = 42;

// Array allocation
int[10] arr;
arr[5] = 99;

// Pointer arithmetic
var val = *(ptr + 5);

// Heap allocation via extern
byte* data = (byte*)malloc(1024);
free(data);

// Raw blocks for unsafe operations
raw {
    var p = (int*)malloc(40);
    *(p + 5) = 42;
    free((byte*)p);
}

// sizeof operator
var size = sizeof(int);

// Null checks
if (ptr != null) { ... }

// new/delete for class instances
Animal* a = new Dog();
delete a;  // dispatches through vtable destructor

// Arrow operator for pointer member access
a->speak();
a->legs;
```

Pointer types, address-of (`&`), dereference (`*`), arithmetic, casts, fixed-size arrays, `malloc`/`free`, and `new`/`delete` are all supported (Test 08).

### 2.14 String Operations

```mingus
var hello = "hello";
var world = "world";

// Concatenation
var greeting = hello + " " + world;

// Content comparison (not pointer comparison)
if (a == b) { puts("equal"); }
if (a != b) { puts("different"); }

// Built-in methods
var len = greeting.length();           // character count
var sub = greeting.substring(0, 5);    // "hello"

// Compound assignment
var s = "hello";
s += " world";

// String interpolation
var msg = "value=${x}, name=${name}";
```

String concatenation results are registered for RAII cleanup via `__mingus_string_free` (Test 14).

### 2.15 Multi-Module Imports

```mingus
// Whole-module import
import MathLib;

// Selective import
import add, square from MathLib;

// Aliased import
import add as myAdd from MathLib;
```

- Automatic file discovery: `import X from MathLib;` finds `MathLib.mingus` in the same directory
- Transitive imports: imported files can have their own imports
- "Compile together" approach: all modules merged into one LLVM module
- Two-sub-pass in SymbolTableBuilder: Pass 1a builds all module scopes, Pass 1b resolves imports (Test 12)

### 2.16 Dynamic Arrays and Map

```mingus
class DynamicArray
{
    int* data;
    int size;
    int capacity;

    func push(int value) => void { ... }
    func map((int) => int transform) => DynamicArray { ... }

    operator[](int index) => int { return *(data + index); }
}
```

`DynamicArray.map()` returns a new array with mapped values, using pipe integration inside class methods (`this[i] |> transform`). Capacity growth via `memcpy` for efficient buffer reallocation (Test 19).

### 2.17 Complex Number Arithmetic

```mingus
struct Complex
{
    double real;
    double imag;

    operator+(Complex other) => Complex
    {
        return Complex { real: real + other.real, imag: imag + other.imag };
    }

    operator*(Complex other) => Complex
    {
        return Complex {
            real: real * other.real - imag * other.imag,
            imag: real * other.imag + imag * other.real
        };
    }

    func magnitude_squared() => double
    {
        return real * real + imag * imag;
    }
}
```

Demonstrates struct return values, operator overloading, and operator composition for mathematical types: `(a + b) * b` works via chained method calls (Test 20).

### 2.18 Debug Information

Optional `--debug` flag on `mingus_ir_tool` enables DWARF debug info generation:

- LLVM DIBuilder integration: `DICompileUnit`, `DIFile`, `DISubprogram`, `DILocalVariable`
- Function-level debug info: each function gets a `DISubprogram` with subroutine type
- Variable-level debug info: `dbg.declare` intrinsic for local variables and parameters
- Type mapping: Mingus types mapped to DI types (`int` -> DW_ATE_signed 32-bit, `double` -> DW_ATE_float 64-bit, etc.)
- Source locations: `emitDebugLocation()` on all statement visitors
- CodeView format on Windows via `module->addModuleFlag("CodeView", 1)`
- Debug info does not alter runtime behavior (Test 27)

### 2.19 Optimization Pipeline

LLVM PassBuilder integration with configurable optimization levels:

| Level | Description |
|-------|-------------|
| `--opt 0` | No optimization (default) |
| `--opt 1` | O1 pipeline (basic simplifications, mem2reg) |
| `--opt 2` | O2 pipeline (inlining, GVN, SROA, instcombine, vectorization, DCE) |

Optimization runs between IR generation and LLVM verification. All tests run with `--opt 2` enabled.

### 2.20 Do-While Loops

The `do-while` loop executes its body at least once before checking the condition:

```mingus
// Body executes at least once
var i = 0;
do {
    i++;
} while (i < 5);

// Even when condition is immediately false
var once = 0;
do {
    once = 42;
} while (false);
// once == 42
```

Do-while loops support `break` and `continue` with the same semantics as other loops, including RAII cleanup on break/continue (Test 39).

### 2.21 Covariant Return Types

Overriding a virtual method can return a more derived pointer type than the base class:

```mingus
class Animal
{
    func clone(int newId) => Animal*
    {
        return new Animal(newId);
    }
}

class Dog : Animal
{
    // Covariant: returns Dog* where base returns Animal*
    func clone(int newId) => Dog*
    {
        return new Dog(newId, 10);
    }
}
```

The TypeChecker validates that the override's return type is a subclass of the base method's return type. Non-pointer covariant returns or unrelated types are rejected (Test 40).

### 2.22 Typedef / Type Alias

Type aliases create named synonyms for existing types:

```mingus
// Primitive typedef
typedef int Count;
typedef double Temperature;
typedef bool Flag;

// Typedef in function signatures
func addCounts(Count a, Count b) => Count
{
    return a + b;
}

// Typedef in struct fields
struct Point
{
    Temperature x;
    Temperature y;
}

// Usage
Count items = 42;
Temperature temp = 100.0;
```

Typedefs are transparent aliases -- `Count` is fully interchangeable with `int` in all contexts. No new type is created; the alias resolves to the underlying type during semantic analysis (Test 41).

### 2.23 Labeled Break/Continue

Labels on loops enable breaking or continuing an outer loop from within nested loops:

```mingus
// Labeled break exits the named outer loop
outer: for (int i = 0; i < 5; i++)
{
    for (int j = 0; j < 5; j++)
    {
        if (j == 2) { break outer; }
    }
}

// Labeled continue skips to the next iteration of the named loop
outer: for (int i = 0; i < 3; i++)
{
    for (int j = 0; j < 3; j++)
    {
        if (j == 1) { continue outer; }
        printf("i=%d j=%d\n", i, j);
    }
}
```

Labels work with `for`, `while`, and `do-while` loops. Unlabeled `break`/`continue` still affects only the innermost loop. RAII cleanup correctly destroys objects in all scopes between the break/continue site and the target loop (Test 42).

### 2.24 Copy Constructors

A constructor whose sole parameter is a reference to the same class type is recognized as a copy constructor:

```mingus
class Counter
{
    public int value;
    public int id;

    constructor(int v, int i)
    {
        this.value = v;
        this.id = i;
    }

    // Copy constructor: parameter is ClassName&
    constructor(Counter& other)
    {
        this.value = other.value;
        this.id = other.id + 100;  // user-defined logic
    }

    destructor {}
}

var original = new Counter(42, 1);
var copy = new Counter(original);     // invokes copy constructor
// copy.value == 42, copy.id == 101
```

The copy constructor is invoked when `new ClassName(existingInstance)` is called. The compiler detects the copy constructor pattern during AST generation by matching the parameter type against the enclosing class name. Mangled name: `ClassName_copy_constructor` (Test 43).

### 2.25 Function Overloading

Multiple functions can share the same name if they differ in parameter count or types:

```mingus
// Overloading by parameter count
func add(int a, int b) => int => a + b;
func add(int a, int b, int c) => int => a + b + c;

// Overloading by parameter type
func describe(int x) => int { printf("int: %d\n", x); return 1; }
func describe(double x) => int { printf("double: %.1f\n", x); return 2; }
func describe(string x) => int { printf("string: %s\n", x); return 3; }

// Overloaded class methods
class Calculator
{
    public int base;
    constructor(int b) { this.base = b; }
    destructor {}

    func compute(int x) => int => this.base + x;
    func compute(int x, int y) => int => this.base + x * y;
}
```

Overload resolution uses a scoring system in the TypeChecker: exact type matches score highest, compatible types (e.g., `int` to `double`) score lower. Parameter count must match exactly. Overloaded functions use `$_type` mangled name suffixes for LLVM disambiguation (Test 44).

### 2.26 Move Semantics

Move constructors enable efficient ownership transfer using the `move()` expression and `&&` rvalue reference parameter syntax:

```mingus
class Resource
{
    public int value;
    public int moved;

    constructor(int v) { this.value = v; this.moved = 0; }

    // Move constructor: parameter is ClassName&&
    constructor(Resource&& other)
    {
        this.value = other.value;
        this.moved = 0;
        other.value = 0;      // zero out source
        other.moved = 1;      // mark as moved
    }

    destructor {}
}

var a = new Resource(42);
var b = new Resource(move(a));   // invokes move constructor
// b.value == 42, a.value == 0, a.moved == 1
```

The `move(expr)` syntax wraps an expression as an rvalue reference, signaling that the value can be moved from. At `new ClassName(move(x))`, if the class has a move constructor and the argument is wrapped in `move()`, the move constructor is dispatched. The move constructor body is user-written and typically transfers ownership of resources while zeroing the source. Mangled name: `ClassName_move_constructor` (Test 45).

---

## 3. Test Suite

### Feature Tests (45/45 passing)

| # | Test File | Description |
|---|-----------|-------------|
| 01 | `test_01_basics` | Integer arithmetic, if/else, for loops, while loops, nested loops |
| 02 | `test_02_structs_operators` | Struct fields, methods, operator+/*, dot product, cross product, length, normalize, chaining |
| 03 | `test_03_classes_raii` | Classes, constructors, destructors, RAII scope cleanup, new/delete, DynamicArray, tree recursion |
| 04 | `test_04_pipes_match` | Pipe operator, chained pipes, match with guards, wildcard, classification |
| 05 | `test_05_enums_switch` | Enum match patterns, switch statement, boolean match |
| 06 | `test_06_lambdas_funcptr` | Lambdas (no captures), higher-order functions, applyTwice, pipe with lambda |
| 07 | `test_07_floats_math` | Float/double arithmetic, sin/cos/sqrt/pow, integer widening, ternary |
| 08 | `test_08_pointers_raw` | Stack arrays, pointer arithmetic, raw blocks, malloc/free, null check, sizeof |
| 09 | `test_09_enum_expressions` | Enum member access in expressions, byte/string enums, enum arithmetic |
| 10 | `test_10_closures` | Fat pointer closures, compose, apply, applyTwice, pipe with closures |
| 11 | `test_11_dsp_showcase` | Structs+operators, enums+match, closures, pipes, math, composed effects, stereo DSP |
| 12 | `test_12_imports` | Multi-file imports, selective import from MathLib, composed calls across modules |
| 13 | `test_13_inheritance` | Single inheritance, vtable virtual dispatch, super() constructor, polymorphic calls through base pointer, inherited fields, abstract class |
| 14 | `test_14_strings` | String concatenation (+), content comparison (==, !=), .length(), .substring(), +=, interpolation |
| 15 | `test_15_interfaces` | Interfaces (Drawable, Resizable), multiple implementation per class, fat pointer dispatch, interface pointer as function parameter |
| 16 | `test_16_dsp_wav` | Inheritance + interfaces (Effect, Named) + oscillator classes producing WAV file output |
| 17 | `test_17_hex_literals` | Hex (0xFF), binary (0b1010), octal (0o77) integer literals; bitwise operations |
| 18 | `test_18_tuples` | Tuple return types (int, int), destructuring (var a, var b) = ..., mixed-type tuples, recursive fibonacci pair |
| 19 | `test_19_dynamic_array_map` | DynamicArray with map() method, capacity growth via memcpy, lambda+pipe integration, operator[] |
| 20 | `test_20_complex_numbers` | Complex struct with operator+, operator*, magnitude squared, chained operator expressions |
| 21 | `test_21_fat_ptr_null` | Fat pointer null comparison: f == null, f != null, null == f, after assignment |
| 22 | `test_22_access_modifiers` | Private/protected/public fields and methods, inheritance access, no-modifier default |
| 23 | `test_23_virtual_destructor` | Virtual destructor dispatch through vtable slot 0; two-level and three-level inheritance chains; delete through Base*, Middle*, and direct |
| 24 | `test_24_static_methods` | ClassName.staticMethod() syntax, recursive static, struct static method |
| 25 | `test_25_escape_analysis` | Temporary closure RAII wrapping, named closures, chained calls, non-escaping detection |
| 26 | `test_26_self_capture` | Self-capturing closures: recursive fibonacci, factorial, countdown via letrec-style env patching |
| 27 | `test_27_debug_info` | --debug flag produces correct runtime behavior, verifies debug info does not break compilation |
| 28 | `test_28_explicit_captures` | [], [=], [&], [x], [&x], [=, &x], [&, x], nested captures, capture-time vs call-time semantics |
| 29 | `test_29_ref_params` | func swap(int& a, int& b), func increment(int& x), mixed ref/value params, divmod with output params |
| 30 | `test_30_capture_writeback` | [&counter] increment, [&sum] accumulator, [&min, &max] tracker, [=, &total] mixed, [&] default ref, write-back through HOF |
| 31 | `test_31_closure_struct_params` | Closures taking struct params (Vec2), direct call, through HOF, lambda literal, capturing closure |
| 32 | `test_32_closure_ref_params` | Closures taking ref params (int&), direct call, through HOF, addStep, double, swap with multiple refs |
| 33 | `test_33_interface_params` | Passing class pointers (Dog*, Cat*) as interface-typed params (Printable*), single and multi-param, interface var passthrough |
| 34 | `test_34_varargs` | Extern varargs (...) with printf: int, double, string, mixed types, no extra args, multiple args |
| 35 | `test_35_for_multi_init` | For loops with multiple initializers: two typed, two inferred, mixed typed+inferred, single init regression |
| 36 | `test_36_const` | Const variables: typed const, inferred const, const in expressions, const string, const inside loops, mutable regression |
| 37 | `test_37_pipe_methods` | Pipe into method calls (x \|> obj->method), chained method pipes, mixed free+method, methods with extra args |
| 38 | `test_38_bare_fields` | Bare field access in class methods/constructors (no this. prefix), inherited field access, local shadowing |
| 39 | `test_39_do_while` | Do-while loop: body-first execution, break/continue inside do-while, nested do-while |
| 40 | `test_40_covariant_returns` | Covariant return types: Dog* overriding Animal*, polymorphic calls through base pointer |
| 41 | `test_41_typedef` | Typedef type aliases: primitive typedefs, typedef in function params, struct fields, interchangeability |
| 42 | `test_42_labeled_loops` | Labeled break/continue: outer loop targeting, for/while/do-while labels, RAII cleanup across label jumps |
| 43 | `test_43_copy_constructors` | Copy constructors: `constructor(T& other)`, user-defined copy logic, regular ctor regression |
| 44 | `test_44_overloading` | Function overloading: by param count, by param type, overloaded class methods |
| 45 | `test_45_move_semantics` | Move semantics: `constructor(T&& other)`, `move(x)` expression, source zeroing, ownership transfer |

### Stress Tests (21/21 passing)

| # | Test File | Description |
|---|-----------|-------------|
| 01 | `stress_01_closure_churn` | 50k closure create/call/discard cycles |
| 02 | `stress_02_nested_capture` | 20k nested closure chains (closure capturing closure) |
| 03 | `stress_03_reassignment` | 30k closure variable reassignment with release-before-assign |
| 04 | `stress_04_early_return_raii` | Early return from function with active RAII objects |
| 05 | `stress_05_interface_closure` | 20k iterations mixing interface dispatch and closure calls |
| 06 | `stress_06_recursive_match` | Recursive fibonacci via match expressions |
| 07 | `stress_07_temporary_leak` | 50k temporary closure creation (leak detection) |
| 08 | `stress_08_destructor_closure` | Interleaved destructor calls and closure invocations |
| 09 | `stress_09_triple_reassign` | Triple closure reassignment verifying release ordering |
| 10 | `stress_10_closure_in_struct` | 20k iterations storing closures in struct fields with RAII cleanup |
| 11 | `stress_11_closure_in_class` | 20k iterations storing closures in class fields with destructor cleanup |
| 13 | `stress_13_break_continue_raii` | RAII destructor cleanup on break/continue inside nested loops |
| 14 | `stress_14_match_guard_raii` | RAII objects active during match expressions with guards |
| 15 | `stress_15_struct_ptr_copy` | Struct with raw pointer -- shallow copy semantics verification |
| 16 | `stress_16_shadow_capture` | Variable shadowing with closure capture in nested scopes |
| 17 | `stress_17_long_running` | 100k iterations combining closures, RAII, interfaces, recursion |
| 18 | `stress_18_break_outer_raii` | Break from inner loop preserves outer-scope RAII objects |
| 19 | `stress_19_null_closure` | Null-initialized closure variable, reassignment, and call |
| 20 | `stress_20_reentrant_closure` | 20k recursive closure wrapping (5 levels deep per iteration) |
| 21 | `stress_21_cyclic_capture` | 10k heap objects with closure fields, no explicit ctor/dtor (auto-generated) |
| 22 | `stress_22_destructor_reentrant` | 10k destructor bodies calling closure fields before epilogue releases them |

**Note:** There is no `stress_12` -- numbering was preserved from development history.

**All 95 tests produce correct output validated against `.expected` files with `--opt 2` enabled.**

---

## 4. Build Instructions

### Prerequisites

- **LLVM 21.1.8** (pre-built, in `extern/clang+llvm-21.1.8-x86_64-pc-windows-msvc/`)
- **MSVC** (Visual Studio 2022 with C++17 support)
- **CMake** (3.20+)
- **Ninja** build system
- **ANTLR4** C++ runtime (built as part of CMake)

### Building from Command Line

```bat
:: Initialize MSVC environment and build
build.bat
```

The `build.bat` script initializes `vcvarsall.bat`, configures CMake with Ninja, and runs an incremental build. The build directory is `build/`.

**Manual build steps:**

```bat
cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DLLVM_DIR=./extern/clang+llvm-21.1.8-x86_64-pc-windows-msvc/lib/cmake/llvm
cmake --build . --config Release
```

**Important:** Must use `-G Ninja` (not the default Visual Studio generator) because the ANTLR4 library path differs with MSBuild. Cannot build from plain bash -- needs `vcvarsall.bat` via `cmd.exe //c`.

### Building with CLion

CLion uses `cmake-build-release/` as its build directory and works out of the box with the CMakeLists.txt.

### Post-Build

CMake post-build copies the compiler binary (`mingus_ir_tool.exe`) to both `examples/` and `tests/` directories, so tests and examples can be run directly.

### Running Tests

```bat
:: Run all 74 feature tests
cd tests
run_v2_tests.bat

:: Run all 21 stress tests
cd tests
run_v2_stress_tests.bat

:: Or run the combined suite from the project root
run_tests.bat

:: Optional flags
run_v2_tests.bat --code      :: Show source code
run_v2_tests.bat --ir        :: Show generated LLVM IR
run_v2_tests.bat --output    :: Show program output
```

Each test compiles `.mingus` to `.ll` (with `--opt 2`), then uses `clang` to produce a native `.exe`, runs it, and compares output against the `.expected` file.

### Running Examples

```bat
cd examples
showcase.bat              :: Run all 14 examples
showcase.bat --code       :: Show source code
showcase.bat --ir         :: Show generated LLVM IR
```

---

## 5. Architecture Summary

### Compiler Pipeline

```
                  +-----------+
Source (.mingus) ->| ANTLR4    |-> Parse Tree
                  | Lexer +   |
                  | Parser    |
                  +-----------+
                       |
                  +-----------+
                  | AST       |-> 69 node types
                  | Generator |   (Expressions, Statements, Declarations,
                  +-----------+    Types, Patterns, Structural)
                       |
              +--------+--------+--------+
              |        |        |        |
           Pass 1   Pass 2   Pass 3   Pass 4
           Symbol   Type     Type     Semantic
           Table    Resolver Checker  Validator
           Builder
              |        |        |        |
              +--------+--------+--------+
                       |
                  +-----------+
                  | LLVM IR   |-> LLVM Module
                  | Generator |   (~4400 lines)
                  +-----------+
                       |
                  +-----------+
                  | LLVM      |-> Optimized IR
                  | PassBuilder|   (O0/O1/O2)
                  +-----------+
                       |
                  +-----------+
                  | Clang     |-> Native executable
                  +-----------+
```

### Semantic Analysis Passes

| Pass | Class | Responsibility |
|------|-------|----------------|
| 1 | `SymbolTableBuilder` | Builds scope tree, creates all symbols (variables, functions, classes, structs, enums, interfaces), auto-generates constructors/destructors, builds vtables, resolves imports (Phase 1a/1b), sets `ParameterNode::resolvedSymbol`, detects copy/move constructors, registers function overloads, resolves typedef aliases |
| 2 | `TypeResolver` | Resolves all `TypeNode` to `TypeSymbol`, sets `VariableSymbol::type`, `FunctionSymbol::returnType`, unwraps `ReferenceType` and rvalue reference on parameters (base type + `isReference=true` / `isRvalueReference=true`) |
| 3 | `TypeChecker` | Bottom-up expression type inference, literal types, identifier resolution via scope chain, call resolution (`resolvedCallee` + `ArgumentsNode::isReference`), function overload resolution with scoring, binary/unary ops, operator overload dispatch, member access, var inference, lambda return type inference, access modifier enforcement, covariant return type validation |
| 4 | `SemanticValidator` | Lambda capture analysis (walks entire lambda stack), self-capture detection, non-escaping lambda detection, RAII variable tracking (per-scope destructibles for codegen), return completeness, labeled break/continue validation, abstract/interface method implementation checking, match exhaustiveness |

### Key Architectural Patterns

- **Fat pointers** `{ ptr, ptr }` are shared by closures and interfaces: closures use `{ fnPtr, envPtr }`, interfaces use `{ objPtr, itablePtr }`
- **RAII scope stack:** `registerRAII(ptr, dtor_fn)` with LIFO cleanup at scope exit, including break/continue handling via `loopRAIIScopeDepth_`
- **Closure reference counting:** capture structs have `{ i64 refcount, ptr cleanup_fn, ...fields }` header; retain on field store, release before reassignment
- **Vtable layout:** slot 0 = destructor, methods at index 1+. Virtual dispatch: load vtable pointer -> GEP to slot -> indirect call
- **Struct universal zero-init:** all struct construction uses `zeroinitializer` to prevent undefined value propagation

### File Map

```
mingus/
+-- MingusLexer.g4                          # ANTLR4 lexer grammar
+-- MingusParser.g4                         # ANTLR4 parser grammar
+-- README.md                               # Project overview and quick start
+-- build.bat                               # Standalone build script (Ninja + MSVC)
+-- run_tests.bat                           # Combined test runner (feature + stress)
+-- docs/
|   +-- MINGUS_STATUS.md                    # This file
|   +-- GRAMMAR_AND_AST.md                  # Grammar rules, operator precedence, AST node inventory
|   +-- SEMANTIC_ANALYSIS.md                # All 4 sema passes
|   +-- MEMORY_AND_LIFETIMES.md             # Stack vs heap, RAII, closure RC, zero-init
|   +-- TYPE_SYSTEM_AND_DISPATCH.md         # LLVM types, vtables, interface dispatch, fat pointers
|   +-- CODEGEN_PATTERNS.md                 # Lambda codegen, pipe operator, match, imports
|   +-- KNOWN_LIMITATIONS.md               # All known limitations
+-- include/mingus/
|   +-- AstNode.h                           # AST base classes, TypeNode, PatternNode, visitor
|   +-- Expressions.h                       # Expression AST nodes (26 concrete types)
|   +-- Statements.h                        # Statement AST nodes (11 concrete types)
|   +-- Declarations.h                      # Declaration AST nodes (14 concrete types)
|   +-- Forward.h                           # Forward declarations and enums
|   +-- Symbol.h                            # Symbol base classes
|   +-- Symbols.h                           # Concrete symbol types
|   +-- TypeSymbol.h                        # Type-as-symbol hierarchy
|   +-- SymbolTable.h                       # Scope tree + symbol lookup
|   +-- Scope.h                             # Scope hierarchy
|   +-- DebugInfo.h                         # Source location tracking
|   +-- sema/
|   |   +-- ErrorReporter.h                 # Diagnostic collection
|   |   +-- SymbolTableBuilder.h            # Pass 1
|   |   +-- TypeResolver.h                  # Pass 2
|   |   +-- TypeChecker.h                   # Pass 3
|   |   +-- SemanticValidator.h             # Pass 4
|   +-- codegen/
|   |   +-- IRGenerator.h                   # LLVM IR generation visitor
+-- src/mingus/
|   +-- sema/*.cpp                          # Sema implementations (4 passes)
|   +-- codegen/IRGenerator.cpp             # ~4400 lines of codegen
|   +-- parser/ASTGenerator.cpp             # Parse tree -> AST
+-- tools/
|   +-- mingus_ir_tool.cpp                  # CLI: parse -> sema -> codegen -> optimize -> verify -> emit
|   +-- mingus_sema_tool.cpp                # Semantic analysis dump tool
|   +-- mingus_ast_tool.cpp                 # AST dump tool
|   +-- simple_example.cpp                  # Minimal AST construction API example
|   +-- factorial_example.cpp               # Factorial AST + IR generation API example
|   +-- parser_example.cpp                  # Parser API example
|   +-- TOOL_GUIDE.md                       # mingus_ir_tool usage reference
|   +-- README.md                           # Tools overview and build guide
|   +-- CMakeLists.txt                      # Build config for all tools
+-- examples/
|   +-- DSPLib.mingus                       # Reusable DSP library (Envelope, Oscillator, WAV writer)
|   +-- example_01..09_*.mingus             # 9 showcase programs
|   +-- showcase.bat                        # Run all 14 examples
+-- tests/
|   +-- test_01..test_74_*.mingus           # 74 feature tests
|   +-- stress_01..stress_22_*.mingus       # 21 stress tests
|   +-- *.expected                          # Expected output files
|   +-- MathLib.mingus                      # Library file for test_12 imports
|   +-- run_v2_tests.bat                    # Feature test runner (v2)
|   +-- run_v2_stress_tests.bat             # Stress test runner (v2)
+-- CMakeLists.txt                          # Root build system
```

---

## 6. Known Limitations

### Language Limitations

| Feature | Status | Notes |
|---------|--------|-------|
| **Generics/templates** | Not supported | No generic types or functions. |
| **Multiple class inheritance** | Not supported | `class C : A, B` where both A and B are classes is a sema error. Multiple interface implementation is supported. |
| **Closures (C ABI)** | Not supported | No C-compatible function pointer extraction from closures. All function-typed values use fat pointers. |

### Codegen Limitations

| Area | Limitation |
|------|------------|
| **Reference lifetime** | `[&x]` captures that escape their scope produce dangling references. This is the programmer's responsibility, same as C++. |
| **Self-capture lifetime** | Self-capturing closures use an unretained self-reference to avoid RC cycles. The closure is valid only while the owning variable is in scope. |
| **Temporary closure leak** | Closures passed directly as function arguments without variable storage leak one refcount. |
| **Duplicate cross-module externs** | If two modules both declare the same `extern func` (e.g. `sin`), codegen creates duplicate LLVM declarations that get name-mangled (`sin.3`), causing linker errors. Workaround: declare externs in one module only, import them in others. |
| **ABI** | Struct return by value relies on LLVM's default ABI lowering. Not tested with very large structs. |
| **Error recovery** | Parser and sema generally stop at the first error. No multi-error recovery or cascading diagnostics. |
| **Module visibility** | `public`/`private` on module-level symbols is parsed but only partially enforced (whole-module import skips non-public). No separate compilation or linking -- all imported files compiled together. |

---

## 7. Compiler Documentation

Detailed technical documentation of the compiler internals:

| Document | Covers |
|----------|--------|
| [GRAMMAR_AND_AST.md](GRAMMAR_AND_AST.md) | Grammar rules, operator precedence, AST node inventory, ASTGenerator mapping |
| [SEMANTIC_ANALYSIS.md](SEMANTIC_ANALYSIS.md) | All 4 sema passes: SymbolTableBuilder, TypeResolver, TypeChecker, SemanticValidator |
| [MEMORY_AND_LIFETIMES.md](MEMORY_AND_LIFETIMES.md) | Stack vs heap, RAII system, closure reference counting, string memory, zero-init |
| [TYPE_SYSTEM_AND_DISPATCH.md](TYPE_SYSTEM_AND_DISPATCH.md) | LLVM type representations, vtables, interface dispatch, fat pointers, operators |
| [CODEGEN_PATTERNS.md](CODEGEN_PATTERNS.md) | Lambda codegen, pipe operator, match expressions, imports, string interpolation |
| [KNOWN_LIMITATIONS.md](KNOWN_LIMITATIONS.md) | All known limitations across grammar, sema, codegen, and runtime |

---

## 8. Advised Next Steps

### Short-term (High impact, moderate effort)

1. **Error Recovery** -- Improve parser and sema to report multiple errors per compilation instead of stopping at the first critical one. Parser error messages should include context about what was expected and where.

2. **Generic Types** -- `class Array<T>`, `func map<T, U>(...)` -- requires monomorphization or type erasure strategy.

### Medium-term

3. **Standard Library** -- Collections (Array, Map, Set), I/O, and math utilities written in Mingus itself, using extern for OS primitives.

4. **Separate Compilation** -- Support compiling modules independently and linking them. Requires stable ABI for module boundaries and a header/interface file format.

### Long-term

5. **REPL / JIT Mode** -- Use LLVM's ORC JIT for interactive evaluation. Useful for exploration and teaching.

6. **Cross-Platform Support** -- Test and fix codegen for Linux/macOS targets. The core LLVM IR is portable, but ABI conventions and debug info formats differ.
