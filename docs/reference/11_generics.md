# Generics

Mingus supports generics through monomorphization — each generic instantiation generates a specialized version at compile time. Generics work with functions, structs, classes, and interfaces, with optional type inference and constraint bounds.

## Generic Functions

Declare type parameters in angle brackets after the function name:

```mingus
func identity<T>(T value) => T
{
    return value;
}

func max<T>(T a, T b) => T
{
    if (a > b) { return a; }
    return b;
}

func add<T>(T a, T b) => T
{
    return a + b;
}
```

### Turbofish Syntax

Call generic functions with explicit type arguments using `::<Types>`:

```mingus
int a = identity::<int>(42);
double b = identity::<double>(3.14);
int m = max::<int>(10, 20);
double d = max::<double>(2.71, 3.14);
```

**Output:**
```
identity<int>(42) = 42
identity<double>(3.14) = 3.14
max<int>(10, 20) = 20
max<double>(2.71, 3.14) = 3.14
```

The turbofish `::< >` is a single lexer token that avoids ambiguity with comparison operators.

### Multiple Type Parameters

```mingus
func first<T, U>(T a, U b) => T
{
    return a;
}

int r = first::<int, double>(42, 3.14);   // 42
```

### Monomorphization Caching

Identical instantiations are cached — `max::<int>` called in two different places produces only one specialized function.

## Type Inference

In most cases, type arguments can be inferred from the call arguments, eliminating the need for turbofish:

```mingus
int a = identity(42);            // T inferred as int
double b = identity(3.14);       // T inferred as double
int m = max(10, 20);             // T inferred as int
double d = max(2.71, 3.14);     // T inferred as double
```

Multi-parameter inference:

```mingus
int r = first(42, 3.14);   // T=int, U=double (each inferred separately)
```

Explicit turbofish still works when inference is ambiguous or when you want to be explicit:

```mingus
int m = max::<int>(5, 3);   // Explicit, overrides inference
```

## Generic Structs

Declare type parameters on structs:

```mingus
struct Pair<T, U>
{
    T first;
    U second;
}
```

### Instantiation

```mingus
Pair<int, double> p;
p.first = 42;
p.second = 3.14;
printf("(%d, %f)\n", p.first, p.second);   // (42, 3.140000)

// Different instantiation
Pair<double, int> q;
q.first = 2.71;
q.second = 100;
```

### Turbofish for Struct Construction

```mingus
var r = Pair::<int, int>();
r.first = 10;
r.second = 20;
```

## Generic Classes

Classes can be generic too, with constructors and methods that use type parameters:

```mingus
class Box<T>
{
    T value;

    constructor(T val)
    {
        this.value = val;
    }

    func get() => T
    {
        return this.value;
    }

    func set(T val) => void
    {
        this.value = val;
    }
}
```

### Using Generic Classes

```mingus
Box<int>* bi = new Box<int>(42);
printf("Box<int>.get() = %d\n", bi->get());   // 42
bi->set(100);
printf("after set = %d\n", bi->get());         // 100
delete bi;

Box<double>* bd = new Box<double>(3.14);
printf("Box<double>.get() = %f\n", bd->get()); // 3.140000
delete bd;
```

## Generic Interfaces

Interfaces can be parameterized by type:

```mingus
interface Getter<T>
{
    func get() => T;
}

interface Setter<T>
{
    func set(T val) => void;
}
```

### Implementing Generic Interfaces

A class can implement specific instantiations of generic interfaces:

```mingus
class IntStore : Getter<int>, Setter<int>
{
    int value;

    constructor(int v) { this.value = v; }
    destructor {}

    func get() => int { return this.value; }
    func set(int val) => void { this.value = val; }
}

class DoubleStore : Getter<double>
{
    double value;

    constructor(double v) { this.value = v; }
    destructor {}

    func get() => double { return this.value; }
}
```

### Interface Polymorphism

```mingus
func printInt(Getter<int>* g) => void
{
    printf("value = %d\n", g->get());
}

var store = new IntStore(42);
Getter<int>* gi = store;   // Implicit interface conversion
printInt(store);             // Also works directly
delete store;
```

## Constraint Bounds

Type parameters can be constrained to require interface implementations:

### Single Constraint

```mingus
interface Printable
{
    func print() => void;
}

// T must implement Printable
func printIt<T: Printable>(T* obj) => void
{
    obj->print();
}
```

### Multiple Constraints

Use `+` to require multiple interfaces:

```mingus
interface HasSize
{
    func size() => int;
}

// T must implement both Printable AND HasSize
func printWithSize<T: Printable + HasSize>(T* obj) => void
{
    printf("[size=%d] ", obj->size());
    obj->print();
}
```

### Mixed Constrained and Unconstrained

```mingus
// T is unconstrained, U must implement Printable
func wrapPrint<T, U: Printable>(T value, U* printer) => void
{
    printf("wrap(%d): ", value);
    printer->print();
}
```

### Implementing Constrained Types

```mingus
class Message : Printable
{
    string text;
    constructor(string t) { this.text = t; }
    destructor {}
    func print() => void { puts(this.text); }
}

class Container : Printable, HasSize
{
    int count;
    constructor(int c) { this.count = c; }
    destructor {}
    func print() => void { printf("Container(%d)\n", this.count); }
    func size() => int { return this.count; }
}

var msg = new Message("hello");
var c = new Container(5);

printIt(msg);            // OK: Message implements Printable
printWithSize(c);        // OK: Container implements Printable + HasSize
wrapPrint(42, msg);      // OK: 42 is int (unconstrained), msg implements Printable
printIt::<Container>(c); // Explicit turbofish with constraint
```

**Output:**
```
hello
[size=5] Container(5)
Container(5)
wrap(42): hello
Container(5)
```

## Generics Summary

| Feature | Syntax | Example |
|---------|--------|---------|
| Generic function | `func f<T>(T x) => T` | `func max<T>(T a, T b) => T` |
| Turbofish call | `f::<Type>(args)` | `max::<int>(10, 20)` |
| Inferred call | `f(args)` | `max(10, 20)` |
| Generic struct | `struct S<T> { T field; }` | `struct Pair<T, U> { ... }` |
| Generic class | `class C<T> { ... }` | `class Box<T> { ... }` |
| Generic interface | `interface I<T> { ... }` | `interface Getter<T> { ... }` |
| Single constraint | `<T: Interface>` | `<T: Printable>` |
| Multi-constraint | `<T: I1 + I2>` | `<T: Printable + HasSize>` |
| Mixed params | `<T, U: Interface>` | `<T, U: Printable>` |

## Known Limitations

- No generic type inference for struct/class instantiation (must specify type arguments)
- No default type arguments
- No generic lambdas (closures are not parameterizable)
- No higher-kinded types
- No specialization (all instantiations use the same template body)
- Constraint bounds only support interfaces, not structural typing
- No variadic type parameters

## See Also

- [Functions](04_functions.md) for non-generic function declarations
- [Structs and Data Types](06_structs_and_data_types.md) for non-generic structs
- [Classes and OOP](08_classes_and_oop.md) for non-generic classes and interfaces
