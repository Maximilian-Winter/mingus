# Pointers and Memory

Mingus provides C-compatible pointers for low-level memory operations. Pointer arithmetic and heap allocation are available inside `raw` blocks, while typed pointers and the address-of operator work in normal code.

## Typed Pointers

Declare a pointer with `Type*`:

```mingus
int x = 42;
int* p = &x;      // Address-of operator
```

### Null Pointers

```mingus
int* p = null;
if (p == null)
{
    puts("null check: PASS");
}
```

## Address-Of Operator

The `&` operator takes the address of a variable or struct field:

```mingus
int x = 42;
int* p = &x;

// Address of a struct field
struct Vec2 { int x; int y; }
Vec2 point;
point.x = 10;
int* px = &point.x;
```

## Pointer Dereferencing

Use `*` to read or write through a pointer:

```mingus
func set_value(int* p, int val) => void
{
    *p = val;
}

int x = 0;
set_value(&x, 42);
printf("x = %d\n", x);   // 42
```

### Multi-Level Pointers

```mingus
int val = 99;
int* p = &val;
int** pp = &p;

printf("*p = %d\n", *p);     // 99
printf("**pp = %d\n", **pp); // 99

**pp = 77;
printf("val = %d\n", val);   // 77
```

### Swap via Pointers

```mingus
func swap(int* a, int* b) => void
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

int a = 10;
int b = 20;
swap(&a, &b);
printf("a=%d b=%d\n", a, b);   // a=20 b=10
```

## Pointer Indexing

Pointers support array-style indexing:

```mingus
func readConstPtr(const int* p) => int
{
    return p[0];
}
```

## Struct Pointer Access

Access struct fields through pointers with `.` (automatic dereference):

```mingus
Vec2 point;
point.x = 3;
point.y = 7;
Vec2* vp = &point;
printf("(%d, %d)\n", vp.x, vp.y);   // (3, 7)
```

For class pointers, use `->`:

```mingus
var dog = new Dog();
dog->speak();
```

## Raw Blocks

The `raw { ... }` block enables low-level pointer arithmetic and type casting that would otherwise be unsafe:

```mingus
raw
{
    var data = (int*)malloc(5 * sizeof(int));
    *(data + 0) = 100;
    *(data + 1) = 200;
    *(data + 2) = 300;

    printf("heap[0] = %d\n", *(data + 0));   // 100
    printf("heap[2] = %d\n", *(data + 2));   // 300

    free((byte*)data);
}
```

### Stack Array with Raw Pointer Access

```mingus
int[10] arr;
for (int i = 0; i < 10; i++)
{
    arr[i] = i * i;
}

var ptr = &arr[0];
raw
{
    var val = *(ptr + 5);
    printf("*(ptr+5) = %d\n", val);   // 25
}
```

### Heap Allocation Pattern

```mingus
extern func malloc(int size) => byte*;
extern func free(byte* ptr) => void;

raw
{
    var data = (int*)malloc(5 * sizeof(int));
    // ... use data ...
    free((byte*)data);
}
```

## Const Pointers

Declare pointers to immutable data with `const`:

```mingus
int x = 42;
const int* p = &x;
int val = p[0];   // Reading is allowed
```

### Non-Const to Const Widening

A non-const pointer can be implicitly assigned to a const pointer (widening):

```mingus
int* mp = &x;
const int* cp = mp;   // OK: widening from int* to const int*
printf("value = %d\n", cp[0]);
```

The reverse (stripping `const`) is not allowed.

### Const in Extern Functions

C library functions often use const pointers:

```mingus
extern func strlen(string s) => size_t;
extern func memcmp(const byte* a, const byte* b, size_t n) => int;

size_t len = strlen("hello world");
printf("strlen = %llu\n", len);   // 11

int result = memcmp("abc", "abd", 3);
printf("result < 0: %d\n", result < 0);   // 1
```

## sizeof Operator

Returns the size in bytes of a type:

```mingus
printf("sizeof(int) = %d\n", sizeof(int));       // 4
printf("sizeof(double) = %d\n", sizeof(double));  // 8
printf("sizeof(MyStruct) = %d\n", sizeof(MyStruct));
```

## Packed Structs

Use `@packed` to remove padding between struct fields:

```mingus
@packed
struct PackedPoint
{
    byte x;
    int y;
    byte z;
}

struct NormalPoint
{
    byte x;
    int y;
    byte z;
}

printf("sizeof(PackedPoint) = %d\n", sizeof(PackedPoint));   // 6
printf("sizeof(NormalPoint) = %d\n", sizeof(NormalPoint));    // 12
```

Packed structs are useful for binary file formats, network protocols, and C interoperability where exact memory layout matters.

## Platform Types

For C interoperability, Mingus provides pointer-width types:

| Type | Description |
|------|-------------|
| `size_t` | Unsigned, same width as a pointer (64-bit on x64) |
| `intptr_t` | Signed integer that can hold a pointer value |
| `uintptr_t` | Unsigned integer that can hold a pointer value |

```mingus
size_t sz = 42;
intptr_t ptr = 100;
uintptr_t uptr = 200;
printf("size_t = %llu\n", sz);
printf("intptr_t = %lld\n", ptr);
printf("uintptr_t = %llu\n", uptr);
```

## Known Limitations

- No smart pointers for non-class types (shared pointers are class-only — see [Classes](08_classes_and_oop.md))
- No garbage collection
- No pointer-to-member syntax
- Raw blocks are required for pointer arithmetic — this is intentional for safety
- No `restrict` qualifier
- Array sizes must be compile-time literals

## See Also

- [Types and Values](02_types_and_values.md) for primitive types and platform types
- [Classes and OOP](08_classes_and_oop.md) for class pointers and shared pointers
- [Structs and Data Types](06_structs_and_data_types.md) for struct layout and arrays
- [C FFI](14_c_ffi.md) for extern functions and opaque pointer types
