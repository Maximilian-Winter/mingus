# Structs and Data Types

Mingus provides several composite data types: structs (with methods and operator overloading), tuples, fixed-size arrays, unions, and tagged unions. This chapter covers all of them.

## Structs

Structs are value types that group related fields together. They can have methods and operator overloads.

### Declaration and Field Access

```mingus
struct Vec3
{
    double x;
    double y;
    double z;
}

Vec3 a;
a.x = 1.0;
a.y = 2.0;
a.z = 3.0;
printf("(%f, %f, %f)\n", a.x, a.y, a.z);
```

### Methods

Structs can have methods that access fields through `this`:

```mingus
struct Vec3
{
    double x;
    double y;
    double z;

    func dot(Vec3 other) => double
    {
        return this.x * other.x + this.y * other.y + this.z * other.z;
    }

    func length() => double
    {
        return sqrt(this.dot(this));
    }
}

Vec3 v;
v.x = 3.0; v.y = 4.0; v.z = 0.0;
printf("length = %f\n", v.length());   // 5.000000
```

### Operator Overloading

Define custom behavior for operators on your struct:

```mingus
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

    func operator*(double scalar) => Vec3
    {
        Vec3 result;
        result.x = this.x * scalar;
        result.y = this.y * scalar;
        result.z = this.z * scalar;
        return result;
    }
}
```

Now you can use natural syntax:

```mingus
var c = a + b;             // operator+
var d = c * 2.5;           // operator*
var e = (a + b) * 2.5;    // chained operators
```

**Supported operators for overloading:** `+`, `-`, `*`, `/`, `==`, `!=`, `<`, `>`, `<=`, `>=`

### Chained Method Calls on Temporaries

Methods can be called directly on expression results:

```mingus
var n = a.cross(b).normalize();
printf("norm z = %f\n", n.z);   // 1.000000
```

### Complex Numbers Example

A complete example showing operator overloading for mathematical types:

```mingus
struct Complex
{
    double real;
    double imag;

    func operator+(Complex other) => Complex
    {
        Complex result;
        result.real = this.real + other.real;
        result.imag = this.imag + other.imag;
        return result;
    }

    func operator*(Complex other) => Complex
    {
        Complex result;
        // (a+bi)(c+di) = (ac-bd) + (ad+bc)i
        result.real = this.real * other.real - this.imag * other.imag;
        result.imag = this.real * other.imag + this.imag * other.real;
        return result;
    }

    func magnitude_squared() => double
    {
        return this.real * this.real + this.imag * this.imag;
    }
}

Complex a; a.real = 3.0; a.imag = 4.0;
Complex b; b.real = 1.0; b.imag = 2.0;

var sum = a + b;           // (4+6i)
var prod = a * b;          // (-5+10i)
var chained = (a + b) * b; // (-8+14i)
double mag2 = a.magnitude_squared();  // 25
```

## Tuples

Tuples are anonymous composite values for returning multiple results from a function.

### Returning Tuples

```mingus
func divmod(int a, int b) => (int, int)
{
    return (a / b, a % b);
}
```

### Destructuring

```mingus
(var quot, var rem) = divmod(17, 5);
printf("17 / 5 = %d remainder %d\n", quot, rem);   // 3 remainder 2
```

### Mixed Types

Tuples can contain different types:

```mingus
func classify(int n) => (string, int, bool)
{
    if (n > 0) { return ("positive", n, true); }
    if (n < 0) { return ("negative", 0 - n, false); }
    return ("zero", 0, false);
}

(var label, var abs_val, var is_pos) = classify(42);
puts(label);   // positive
```

### Recursive Functions with Tuples

```mingus
func fib_pair(int n) => (int, int)
{
    if (n <= 0) { return (0, 1); }
    (var a, var b) = fib_pair(n - 1);
    return (b, a + b);
}

(var fib8, var fib9) = fib_pair(8);
printf("fib(8) = %d\n", fib8);   // 21
printf("fib(9) = %d\n", fib9);   // 34
```

## Arrays

Fixed-size arrays with compile-time known length.

### Declaration and Indexing

```mingus
int[5] arr;
arr[0] = 10;
arr[1] = 20;
arr[2] = 30;
arr[3] = 40;
arr[4] = 50;
printf("arr[0] = %d\n", arr[0]);   // 10
```

### Array Literals

```mingus
int[4] arr = [10, 20, 30, 40];
var inferred = [1, 2, 3];   // type inferred as int[3]
```

### Arrays as Function Parameters and Return Values

```mingus
func sumArray(int[5] arr) => int
{
    int total = 0;
    for (int i = 0; i < 5; i = i + 1)
    {
        total = total + arr[i];
    }
    return total;
}

func makeArray() => int[3]
{
    int[3] result;
    result[0] = 100;
    result[1] = 200;
    result[2] = 300;
    return result;
}
```

Array literals can be passed directly as arguments:

```mingus
func sumThree(int[3] arr) => int
{
    return arr[0] + arr[1] + arr[2];
}

int s = sumThree([100, 200, 300]);   // 600
```

### Arrays in Structs

```mingus
struct Vec4
{
    int[4] data;
}

Vec4 v;
v.data[0] = 5;
v.data[1] = 10;
v.data[2] = 15;
v.data[3] = 20;
```

Array fields can be assigned with literals:

```mingus
struct Point3
{
    int[3] coords;
}

Point3 p;
p.coords = [10, 20, 30];
```

### Arrays in Classes

```mingus
class Buffer
{
    int[4] data;
    int count;

    constructor(int a, int b, int c, int d)
    {
        this.data[0] = a;
        this.data[1] = b;
        this.data[2] = c;
        this.data[3] = d;
        this.count = 4;
    }

    func get(int i) => int { return this.data[i]; }

    func sum() => int
    {
        int total = 0;
        for (int i = 0; i < this.count; i = i + 1)
        {
            total = total + this.data[i];
        }
        return total;
    }
}

var buf = Buffer(10, 20, 30, 40);
printf("buf.sum() = %d\n", buf.sum());   // 100
```

### Array Reassignment

```mingus
int[3] arr = [1, 2, 3];
arr = [4, 5, 6];   // overwrites entire array
```

## Unions

Unions share memory between fields — only one field is valid at a time. The size of a union is the size of its largest field.

```mingus
union Value
{
    int i;
    float f;
    byte b;
}

Value v;
v.i = 42;
printf("int: %d\n", v.i);     // 42

v.f = 3.14f;
printf("float: %f\n", v.f);   // 3.140000
```

### Unions as Function Parameters

```mingus
func readInt(Value v) => int
{
    return v.i;
}
```

### Extern Unions

For C interoperability, unions can be declared inside `extern` blocks:

```mingus
extern {
    union RawValue {
        int asInt;
        float asFloat;
        double asDouble;
    }
}

RawValue rv;
rv.asDouble = 2.718;
printf("double: %f\n", rv.asDouble);
```

## Tagged Unions

Tagged unions (also called discriminated unions or sum types) associate a tag with each variant, allowing safe pattern matching.

### Declaration

```mingus
tagged union Option {
    Some(int value),
    None
}

tagged union Result {
    Ok(int value),
    Err(string message)
}
```

### Construction

```mingus
var some = Option.Some(42);
var none = Option.None;

var ok = Result.Ok(100);
var err = Result.Err("not found");
```

### Pattern Matching with `match`

```mingus
match some {
    Option.Some(var v) => printf("Some(%d)\n", v),
    Option.None => printf("None\n"),
};
```

**Output:**
```
Some(42)
```

```mingus
match err {
    Result.Ok(var v) => printf("Ok(%d)\n", v),
    Result.Err(var msg) => printf("Err(%s)\n", msg),
};
```

**Output:**
```
Err(not found)
```

### Functions with Tagged Unions

Tagged unions work as return types and parameters:

```mingus
func makeOption(bool hasSome, int val) => Option
{
    if (hasSome) { return Option.Some(val); }
    return Option.None;
}

func printOption(Option opt) => void
{
    match opt {
        Option.Some(var v) => printf("Some(%d)\n", v),
        Option.None => printf("None\n"),
    };
}

var opt1 = makeOption(true, 7);
printOption(opt1);    // Some(7)

var opt2 = makeOption(false, 0);
printOption(opt2);    // None
```

### Reassignment

Tagged union variables can be reassigned to a different variant:

```mingus
var x = Option.None;
printOption(x);           // None
x = Option.Some(123);
printOption(x);           // Some(123)
```

## Known Limitations

- Struct fields cannot have default initializers
- No anonymous structs
- Array sizes must be literal integers, not constants or expressions
- No dynamic arrays or slices (use pointers for dynamic allocation — see [Pointers and Memory](12_pointers_and_memory.md))
- Unions are untagged by default — the programmer is responsible for tracking which field is active
- Tagged unions only support single-field variants (no multi-field variants)
- No generic structs without explicit turbofish syntax (see [Generics](11_generics.md))

## See Also

- [Types and Values](02_types_and_values.md) for primitive types and type aliases
- [Enums and Pattern Matching](07_enums_and_pattern_matching.md) for `match` expressions with enums
- [Classes and OOP](08_classes_and_oop.md) for classes with constructors, destructors, and inheritance
- [Generics](11_generics.md) for generic structs and classes
- [C FFI](14_c_ffi.md) for extern structs, enums, and unions
