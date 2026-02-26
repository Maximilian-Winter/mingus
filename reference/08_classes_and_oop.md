# Classes and OOP

Mingus classes provide constructors, destructors (RAII), inheritance, interfaces, access modifiers, static methods, shared pointers, copy/move semantics, and forward type references. Classes are heap-allocated with `new` and cleaned up with `delete` or automatic reference counting.

## Basic Classes

A class has fields, a constructor, a destructor, and methods:

```mingus
class Counter
{
    int count;

    constructor(int initial)
    {
        this.count = initial;
    }

    destructor
    {
        printf("Counter destroyed\n");
    }

    func increment() => void
    {
        this.count = this.count + 1;
    }

    func value() => int
    {
        return this.count;
    }
}
```

### Heap Allocation

Classes are allocated with `new` and freed with `delete`:

```mingus
var c = new Counter(0);
c->increment();
c->increment();
printf("count = %d\n", c->value());   // 2
delete c;   // destructor fires
```

### Stack Allocation

Classes can also be allocated on the stack — the destructor fires automatically when the variable goes out of scope (RAII):

```mingus
{
    var c = Counter(0);
    c.increment();
    c.increment();
    printf("count = %d\n", c.value());
}   // destructor fires here
```

Note the difference: heap uses `->` for method calls, stack uses `.`.

## RAII (Resource Acquisition Is Initialization)

Destructors fire automatically when objects go out of scope:

```mingus
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
        puts("[DynamicArray destructor called]");
    }

    func push(int value) => void { ... }
    func operator[](int index) => int { ... }
}
```

Resources are acquired in the constructor and released in the destructor. The compiler ensures destructors are called at scope exit, even on early returns.

## Inheritance

Classes inherit from a base class using `:` syntax:

```mingus
class Animal
{
    int legs;

    constructor(int l) { this.legs = l; }
    destructor {}

    func speak() => void { puts("..."); }
    func getLegs() => int { return this.legs; }
}

class Dog : Animal
{
    constructor() : super(4) {}
    destructor {}

    func speak() => void { puts("Woof!"); }
}
```

### Virtual Dispatch

All methods are virtual by default. Calling through a base pointer dispatches to the derived method:

```mingus
func makeAnimalSpeak(Animal* a) => void
{
    a->speak();   // calls Dog::speak if a points to a Dog
}

var dog = new Dog();
makeAnimalSpeak(dog);   // "Woof!"
delete dog;
```

### Super Constructor

Derived constructors call the base constructor with `super(args)`:

```mingus
class Dog : Animal
{
    string breed;

    constructor(string n, int a, string b) : super(n, a)
    {
        this.breed = b;
    }
}
```

### Virtual Destructors and Chaining

Destructors are virtual and chain automatically. When deleting through a base pointer, the derived destructor fires first, then the base destructor:

```mingus
class Base    { destructor { puts("~Base"); } }
class Middle : Base { destructor { puts("~Middle"); } }
class Leaf : Middle { destructor { puts("~Leaf"); } }

Base* deep = new Leaf();
delete deep;
```

**Output:**
```
~Leaf
~Middle
~Base
```

### Covariant Return Types

A derived class can return a more specific type than the base:

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
    func clone(int newId) => Dog*    // Covariant: Dog* instead of Animal*
    {
        return new Dog(newId, this.barkVolume);
    }
}
```

## Abstract Classes

Mark a base class as `abstract` to indicate it is not meant to be instantiated directly — only through derived classes:

```mingus
abstract class AudioEffect
{
    double param;

    constructor(double p) { this.param = p; }

    func paramValue() => double { return this.param; }
}

class GainEffect : AudioEffect
{
    constructor(double g) : super(g) {}

    func process(double sample) => double
    {
        return sample * this.paramValue();
    }
}
```

Abstract classes can have fields, constructors, and method implementations. Derived classes call the abstract class constructor via `super(args)` and can override methods.

## Interfaces

Interfaces define method contracts that classes must implement:

```mingus
interface Drawable
{
    func draw() => void;
}

interface Resizable
{
    func resize(int factor) => int;
}
```

### Implementing Interfaces

A class can implement one or more interfaces:

```mingus
class Circle : Drawable, Resizable
{
    int radius;

    constructor(int r) { this.radius = r; }
    destructor {}

    func draw() => void { puts("Circle drawn"); }
    func resize(int factor) => int { return this.radius * factor; }
}
```

### Interface Pointers

Pass classes as interface pointers for polymorphic dispatch:

```mingus
func renderAll(Drawable* d) => void
{
    d->draw();
}

var circle = new Circle(5);
renderAll(circle);   // Automatic wrapping: Circle* -> Drawable*
delete circle;
```

When a class pointer is passed where an interface pointer is expected, the compiler automatically creates the interface dispatch wrapper.

### Combining Inheritance and Interfaces

```mingus
class Dog : Animal, Printable
{
    // Inherits from Animal, implements Printable interface
}
```

## Access Modifiers

Fields and methods can be `public`, `private`, or `protected`:

```mingus
class Account
{
    private int balance;       // Only this class
    protected int accountId;   // This class + derived
    public string name;        // Anyone
    int age;                   // No modifier = public by default

    private func validate() => bool { ... }
    protected func getBalance() => int { return this.balance; }
    public func deposit(int amount) => void { ... }
}
```

| Modifier | Same Class | Derived Class | External |
|----------|-----------|---------------|----------|
| `private` | Yes | No | No |
| `protected` | Yes | Yes | No |
| `public` | Yes | Yes | Yes |
| *(none)* | Yes | Yes | Yes |

## Static Methods

Static methods belong to the class itself, not to instances:

```mingus
class MathUtils
{
    static func add(int a, int b) => int { return a + b; }

    static func factorial(int n) => int
    {
        if (n <= 1) { return 1; }
        return n * MathUtils.factorial(n - 1);
    }
}

printf("5! = %d\n", MathUtils.factorial(5));   // 120
```

Structs can also have static methods:

```mingus
struct Point
{
    int x;
    int y;

    static func origin() => Point
    {
        Point p;
        p.x = 0;
        p.y = 0;
        return p;
    }
}

var p = Point.origin();
```

## Bare Field Access

Inside methods and constructors, fields can be accessed without the `this.` prefix:

```mingus
class Counter
{
    int count;
    double factor;

    constructor(int c, double f)
    {
        count = c;       // Same as this.count = c
        factor = f;
    }

    func scaled() => double
    {
        return count * factor;   // Same as this.count * this.factor
    }
}
```

Local variables shadow fields when they share a name:

```mingus
class Example
{
    int value;

    func shadowed() => int
    {
        int value = 999;   // Local shadows this.value
        return value;       // Returns 999
    }
}
```

Bare access also works for inherited fields in derived classes.

## Constructor Overloading

Classes can have multiple constructors with different parameter signatures:

```mingus
class Point
{
    int x;
    int y;

    constructor() { this.x = 0; this.y = 0; }
    constructor(int v) { this.x = v; this.y = v; }
    constructor(int px, int py) { this.x = px; this.y = py; }
}

var p0 = Point();        // Default
var p1 = Point(5);       // Single param
var p2 = Point(3, 7);    // Two params

var hp = new Point(10, 20);   // Heap allocation with overload
delete hp;
```

## Copy Constructors

Define how objects are copied with `constructor(Type& other)`:

```mingus
class Counter
{
    public int value;
    public int id;

    constructor(int v, int i) { this.value = v; this.id = i; }

    constructor(Counter& other)
    {
        this.value = other.value;
        this.id = other.id + 100;   // Custom copy logic
    }
}

var original = new Counter(42, 1);
var copy = new Counter(original);
printf("copy.id = %d\n", copy.id);   // 101
delete original;
delete copy;
```

## Move Semantics

Transfer resources from one object to another with `constructor(Type&& other)` and the `move()` function:

```mingus
class Resource
{
    public int value;
    public int moved;

    constructor(int v)
    {
        this.value = v;
        this.moved = 0;
    }

    constructor(Resource&& other)
    {
        this.value = other.value;
        this.moved = 0;
        other.value = 0;     // Drain source
        other.moved = 1;     // Mark as moved
    }
}

var a = new Resource(42);
var b = new Resource(move(a));
// b.value == 42, a.value == 0 (drained)
```

## Shared Pointers

Reference-counted shared pointers provide automatic lifetime management:

```mingus
shared Animal* a = new shared Animal(1, "Rex");
a->speak();
// a is automatically released when it goes out of scope
```

### Reassignment

When a shared pointer is reassigned, the old object's reference count decreases (and it's destroyed if it reaches zero):

```mingus
shared Animal* a = new shared Animal(1, "Max");
a->speak();
a = new shared Animal(2, "Bella");   // Animal(1) destroyed here
a->speak();
// Animal(2) destroyed at scope exit
```

### Polymorphism with Shared Pointers

Virtual dispatch works through shared pointers:

```mingus
shared Animal* a = new shared Dog(1, "Buddy", "Labrador");
a->speak();   // Calls Dog::speak
```

## Forward Type References

Types can be referenced before they are defined. This allows mutual references and any-order declarations:

```mingus
// Dog references Animal, but Animal is defined later
class Dog : Animal
{
    string breed;
    constructor(string n, int a, string b) : super(n, a) { this.breed = b; }
    func speak() => void { printf("Woof! %s\n", this.name); }
}

class Animal
{
    string name;
    int age;
    constructor(string n, int a) { this.name = n; this.age = a; }
    func speak() => void { puts("..."); }
}
```

Forward references also work with interfaces:

```mingus
class Printer : Printable { func print() => void { puts("hello"); } }
interface Printable { func print() => void; }
```

## Operator Overloading on Classes

Classes support operator overloading using `operator[]`:

```mingus
class DynamicArray
{
    func operator[](int index) => int
    {
        raw { return *(this.data + index); }
    }
}

var arr = DynamicArray(4);
arr.push(10);
printf("arr[0] = %d\n", arr[0]);   // 10
```

## Known Limitations

- No multiple inheritance (single base class + multiple interfaces)
- No `final` or `sealed` classes
- No property syntax (getters/setters are regular methods)
- No private constructors
- Shared pointers work with classes only, not structs
- No weak pointers for shared objects (weak captures exist for closures — see [Lambdas](09_lambdas_and_closures.md))

## See Also

- [Structs and Data Types](06_structs_and_data_types.md) for struct methods and operator overloading
- [Functions](04_functions.md) for reference parameters and overloading
- [Lambdas and Closures](09_lambdas_and_closures.md) for closures as class fields
- [Generics](11_generics.md) for generic classes and interfaces
- [Pointers and Memory](12_pointers_and_memory.md) for raw pointers and heap allocation
