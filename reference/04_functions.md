# Functions

Functions are the basic unit of code organization in Mingus. They support explicit return types, reference parameters, function overloading, variadic arguments, and expression-body shorthand.

## Function Declarations

A function is declared with `func`, followed by name, parameters, return type after `=>`, and body:

```mingus
func add(int a, int b) => int
{
    return a + b;
}
```

### Void Functions

Functions that return nothing use `=> void`:

```mingus
func greet(string name) => void
{
    printf("Hello, %s!\n", name);
}
```

### Expression-Body Shorthand

For simple one-expression functions, use `=>` followed by the expression:

```mingus
func add(int a, int b) => int => a + b;
func add(int a, int b, int c) => int => a + b + c;
```

This is equivalent to writing a full body with `return`.

## Reference Parameters

Pass parameters by reference using `&` after the type. The callee receives a pointer to the caller's variable and can modify it directly:

```mingus
func swap(int& a, int& b) => void
{
    var tmp = a;
    a = b;
    b = tmp;
}

var x = 10;
var y = 20;
swap(x, y);
printf("x=%d y=%d\n", x, y);   // x=20 y=10
```

### Multiple Output Parameters

Reference parameters work well for returning multiple values (an alternative to tuples):

```mingus
func divmod(int a, int b, int& quotient, int& remainder) => void
{
    quotient = a / b;
    remainder = a - (quotient * b);
}

var q = 0;
var r = 0;
divmod(17, 5, q, r);
printf("17/5 = %d remainder %d\n", q, r);   // 3 remainder 2
```

### Mixed Reference and Value Parameters

```mingus
func addAndReturn(int& accum, int value) => int
{
    var old = accum;
    accum = accum + value;
    return old;
}

var total = 0;
var old1 = addAndReturn(total, 10);   // total=10, old1=0
var old2 = addAndReturn(total, 20);   // total=30, old2=10
var old3 = addAndReturn(total, 30);   // total=60, old3=30
```

## Function Overloading

Functions can be overloaded by parameter count or parameter types. The compiler resolves the correct version at the call site:

### By Parameter Count

```mingus
func add(int a, int b) => int => a + b;
func add(int a, int b, int c) => int => a + b + c;

printf("add(1,2) = %d\n", add(1, 2));       // 3
printf("add(1,2,3) = %d\n", add(1, 2, 3));  // 6
```

### By Parameter Type

```mingus
func describe(int x) => int
{
    printf("int: %d\n", x);
    return 1;
}

func describe(double x) => int
{
    printf("double: %.1f\n", x);
    return 2;
}

func describe(string x) => int
{
    printf("string: %s\n", x);
    return 3;
}

describe(42);       // "int: 42"
describe(3.14);     // "double: 3.1"
describe("hello");  // "string: hello"
```

### Method Overloading

Overloading also works for class methods:

```mingus
class Calculator
{
    public int base;

    constructor(int b) { this.base = b; }
    destructor {}

    func compute(int x) => int => this.base + x;
    func compute(int x, int y) => int => this.base + x * y;
}

var calc = new Calculator(10);
printf("compute(5) = %d\n", calc->compute(5));       // 15
printf("compute(3,4) = %d\n", calc->compute(3, 4));  // 22
delete calc;
```

## Variadic Functions (Varargs)

Mingus supports variadic functions through `extern` declarations using `...`:

```mingus
extern func printf(string fmt, ...) => int;
```

Call them with any number of additional arguments:

```mingus
printf("int: %d\n", 42);
printf("two ints: %d %d\n", 10, 20);
printf("double: %f\n", 3.14);
printf("mixed: %d %f %d\n", 1, 2.718, 3);
printf("string: %s\n", "hello");
printf("no args\n");
```

**Output:**
```
int: 42
two ints: 10 20
double: 3.140000
mixed: 1 2.718000 3
string: hello
no args
```

Varargs follow C calling conventions: `float` is promoted to `double`, and small integers (`byte`, `short`) are promoted to `int`.

## Extern Functions

Import C library functions with `extern func`:

```mingus
extern func printf(string fmt, ...) => int;
extern func puts(string s) => int;
extern func sqrt(double x) => double;
extern func malloc(size_t size) => byte*;
```

See [C FFI](14_c_ffi.md) for the full extern system including extern blocks, opaque types, and calling conventions.

## Known Limitations

- No default parameter values
- No named arguments at call sites
- No generic functions without explicit turbofish syntax (see [Generics](11_generics.md))
- User-defined variadic functions are not supported — only `extern` functions can be variadic
- No `inline` hint (the optimizer handles inlining)

## See Also

- [Getting Started](01_getting_started.md) for basic function syntax
- [Structs and Data Types](06_structs_and_data_types.md) for struct methods
- [Classes and OOP](08_classes_and_oop.md) for class methods and constructors
- [Lambdas and Closures](09_lambdas_and_closures.md) for anonymous functions
- [Generics](11_generics.md) for generic functions
- [C FFI](14_c_ffi.md) for extern function details
