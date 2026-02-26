# Types and Values

Mingus has a rich type system with primitive types, extended integer types, platform-specific types, floating-point types, constants, and type aliases. This chapter covers the building blocks that all other features are built on.

## Primitive Types

| Type | Size | Description |
|------|------|-------------|
| `int` | 32-bit | Signed integer |
| `double` | 64-bit | Double-precision floating point |
| `float` | 32-bit | Single-precision floating point |
| `bool` | 1-bit | `true` or `false` |
| `byte` | 8-bit | Unsigned byte |
| `string` | pointer | Null-terminated C string (`char*`) |

```mingus
int count = 42;
double pi = 3.14159265;
float half = 0.5f;
bool active = true;
byte ch = 65;           // ASCII 'A'
string name = "Alice";
```

## Extended Integer Types

For situations where you need specific widths or unsigned semantics:

| Type | Size | Range |
|------|------|-------|
| `short` | 16-bit signed | -32768 to 32767 |
| `ushort` | 16-bit unsigned | 0 to 65535 |
| `uint` | 32-bit unsigned | 0 to 4,294,967,295 |
| `long` | 64-bit signed | -2^63 to 2^63-1 |
| `ulong` | 64-bit unsigned | 0 to 2^64-1 |

### Example

```mingus
short s = 1000;
ushort us = 50000;
uint ui = 3000000000;
long l = 9000000000;
ulong ul = 18000000000;

printf("short: %d\n", s);
printf("ushort: %u\n", us);
printf("uint: %u\n", ui);
printf("long: %lld\n", l);
printf("ulong: %llu\n", ul);
```

**Output:**
```
short: 1000
ushort: 50000
uint: 3000000000
long: 9000000000
ulong: 18000000000
```

Unsigned types use unsigned arithmetic. This matters for comparison and division — a `uint` value of 4,000,000,000 is correctly compared as positive, not treated as a negative signed value.

```mingus
uint big = 4000000000;
uint small = 1;
if (big > small)
{
    printf("uint cmp: correct\n");   // This prints
}
```

## Platform Types

For C interoperability, Mingus provides pointer-width types:

| Type | Description |
|------|-------------|
| `size_t` | Unsigned, same width as a pointer (64-bit on x64) |
| `intptr_t` | Signed integer that can hold a pointer value |
| `uintptr_t` | Unsigned integer that can hold a pointer value |

```mingus
extern func strlen(string s) => size_t;

size_t len = strlen("hello");
printf("strlen = %llu\n", len);    // 5
```

## Numeric Literals

### Integer Literals

```mingus
int dec = 255;          // Decimal
int hex = 0xFF;         // Hexadecimal (prefix 0x)
int bin = 0b11111111;   // Binary (prefix 0b)
int oct = 0o377;        // Octal (prefix 0o)
```

**Output:**
```
hex:    255 57005 2147483647
binary: 10 255 170
octal:  8 511 438
```

Integer literals that exceed the 32-bit range automatically promote to `long`.

### Float Literals

```mingus
double d = 3.14;        // double (64-bit) — no suffix
float f = 3.14f;        // float (32-bit) — 'f' or 'F' suffix
float g = 1.5e2f;       // Scientific notation: 150.0 as float
float h = 42.0F;        // Capital F works too
```

Use `sizeof` to verify:
```mingus
printf("sizeof(float) = %d\n", sizeof(float));    // 4
printf("sizeof(double) = %d\n", sizeof(double));   // 8
```

## Constants

Declare immutable values with `const`:

```mingus
const int MAX_SIZE = 1024;
const pi = 3.14;                // type inferred as double
const string NAME = "Alice";

// Constants can use hex and float suffix
const int FLAG_A = 0x01;
const int FLAG_B = 0x02;
const uint BIG_HEX = 0xDEADBEEF;
const float HALF = 0.5f;
```

Constants can be used in expressions:
```mingus
var flags = FLAG_A + FLAG_B;
printf("Flags: %d\n", flags);    // 3
```

Constants can appear at module level (accessible throughout the module) or inside function bodies. Module-level constants are the Mingus equivalent of C `#define` values.

## Type Aliases (typedef)

Create named aliases for existing types with `typedef`:

```mingus
typedef int Count;
typedef double Temperature;
typedef bool Flag;
```

Aliases are fully interchangeable with their underlying type:

```mingus
Count items = 42;
int raw = items;         // OK — Count is just int
Temperature f = toFahrenheit(100.0);
```

Aliases can be used as struct field types, function parameter types, and return types:

```mingus
typedef double Temperature;

struct Point
{
    Temperature x;
    Temperature y;
}

func toFahrenheit(Temperature celsius) => Temperature
{
    return celsius * 1.8 + 32.0;
}
```

**Output:**
```
100C = 212.0F
```

## Type Widening and Coercion

Mingus automatically widens values when mixing numeric types in expressions:

- **Integer to float/double**: `int * double` produces `double`
- **Narrow to wide integer**: `short + int` produces `int`
- **Float to double**: `float + double` produces `double`

```mingus
var intVal = 7;
var floatResult = intVal * 2.0;
printf("7 * 2.0 = %f\n", floatResult);   // 14.000000
```

### Explicit Casts

Use C-style casts for explicit conversion:

```mingus
int fromSigned = 42;
uint toUnsigned = (uint)fromSigned;

ulong ul = 18000000000;
double uf = (double)ul;

float x = 3.14f;
printf("%.2f\n", (double)x);    // Cast needed for printf varargs
```

## The Ternary Operator

```mingus
var x = 5.0;
var abs_x = x > 0.0 ? x : -x;
printf("|5.0| = %f\n", abs_x);   // 5.000000
```

## Bitwise Operators

| Operator | Meaning |
|----------|---------|
| `&` | Bitwise AND |
| `\|` | Bitwise OR |
| `^` | Bitwise XOR |
| `~` | Bitwise NOT |
| `<<` | Left shift |
| `>>` | Right shift |

```mingus
int v = 255;
int masked = v & 0xFF;
int shifted = v & 0xFF00;
int combined = (v & 0xFF) | (0 & 0xFF00);
printf("ops: %d %d %d\n", masked, shifted, combined);  // 255 0 255
```

## Math Functions

Mingus accesses math functions through C library `extern` declarations:

```mingus
extern func sin(double x) => double;
extern func cos(double x) => double;
extern func sqrt(double x) => double;
extern func pow(double base, double exp) => double;

printf("sin(0) = %f\n", sin(0.0));           // 0.000000
printf("cos(0) = %f\n", cos(0.0));           // 1.000000
printf("sqrt(144) = %f\n", sqrt(144.0));     // 12.000000
printf("pow(2,10) = %f\n", pow(2.0, 10.0));  // 1024.000000
```

## sizeof Operator

Returns the size in bytes of a type:

```mingus
printf("sizeof(int) = %d\n", sizeof(int));          // 4
printf("sizeof(double) = %d\n", sizeof(double));     // 8
printf("sizeof(float) = %d\n", sizeof(float));       // 4
printf("sizeof(MyStruct) = %d\n", sizeof(MyStruct)); // struct-dependent
```

## Known Limitations

- No 128-bit integer types
- No complex number type (but can be built as a struct with operator overloading — see [Structs](06_structs_and_data_types.md))
- No user-defined literal suffixes
- No `char` type — use `byte` for single characters
- No `unsigned int` alias — use `uint`
- Array sizes must be literal integers, not constants

## See Also

- [Getting Started](01_getting_started.md) for variable declaration basics
- [Structs and Data Types](06_structs_and_data_types.md) for composite types
- [Pointers and Memory](12_pointers_and_memory.md) for pointer types and `sizeof`
- [C FFI](14_c_ffi.md) for platform types in FFI context
