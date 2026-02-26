# Lambdas and Closures

Mingus supports first-class functions through lambdas (anonymous functions) and closures (lambdas that capture variables from their enclosing scope). The capture system provides by-value, by-reference, and weak capture modes.

## Lambda Syntax

A lambda is written as `[captures](parameters) => { body }`:

```mingus
var doubler = [](double x) => { return x * 2.0; };
var result = doubler(21.0);
printf("result = %f\n", result);   // 42.000000
```

The capture list `[]` specifies what variables from the surrounding scope the lambda can access. An empty `[]` means no captures.

## Function Types

Function types are written as `(ParamTypes) => ReturnType`:

```mingus
(double) => double              // Takes double, returns double
(int, int) => int               // Takes two ints, returns int
(int&) => void                  // Takes int by reference, returns void
(Vec2) => double                // Takes a struct, returns double
() => void                      // Takes nothing, returns void
```

Function-typed variables can hold lambdas, closures, or be `null`:

```mingus
(int) => int f = null;

if (f == null)
{
    puts("f is null");
}

f = [](int x) => { return x * 2; };

if (f != null)
{
    printf("f(21) = %d\n", f(21));   // 42
}
```

## Higher-Order Functions

Functions that take or return function-typed values:

```mingus
func apply(double x, (double) => double transform) => double
{
    return transform(x);
}

var result = apply(5.0, [](double x) => { return x * x; });
printf("5^2 = %f\n", result);   // 25.000000
```

### Applying Twice

```mingus
func applyTwice(double x, (double) => double f) => double
{
    return f(f(x));
}

var r = applyTwice(3.0, [](double x) => { return x * 2.0; });
printf("double twice = %f\n", r);   // 12.000000
```

## Capture Modes

The capture list controls how outer variables are captured.

### No Captures: `[]`

The lambda is a pure function with no access to outer variables:

```mingus
var r = apply([](int x) => { return x * 2; }, 21);   // 42
```

### By Value: `[=]`

Copies all referenced outer variables at the point of definition. The closure sees the values as they were when the lambda was created:

```mingus
var base = 10;
var addBase = [=](int x) => { return x + base; };
base = 999;   // Mutation after definition
printf("addBase(5) = %d\n", addBase(5));   // 15 (uses original base=10)
```

### By Reference: `[&]`

Captures pointers to outer variables. Mutations to the outer variable are visible inside the closure, and vice versa:

```mingus
var x = 1;
var valueLambda = [=]() => { return x; };   // Freezes x=1
var refLambda = [&]() => { return x; };     // Sees current x
x = 13;
printf("[=] frozen: %d\n", valueLambda());   // 1
printf("[&] current: %d\n", refLambda());    // 13
```

### Explicit Named Captures

Capture specific variables by name:

```mingus
// [x] — capture x by value
var factor = 3;
var scale = [factor](int x) => { return x * factor; };

// [&total] — capture total by reference
var total = 0;
var adder = [&total](int x) => {
    total = total + x;
    return total;
};
adder(10);   // total = 10
adder(20);   // total = 30
```

### Mixed Captures

Combine default and explicit modes:

```mingus
// [=, &accum] — all by value, but accum by reference
var scale = 2;
var accum = 0;
var mixed = [=, &accum](int x) => {
    accum = accum + x * scale;
    return accum;
};
mixed(3);   // accum = 6, scale still 2
mixed(5);   // accum = 16

// [&, x] — all by reference, but x by value
var a = 10;
var b = 20;
var x = 100;
var fn = [&, x]() => {
    a = a + x;   // a modified by ref, x frozen by value
    return a + b;
};
```

## Capture Write-Back

Reference captures enable the accumulator pattern:

```mingus
var counter = 0;
var inc = [&counter]() => {
    counter = counter + 1;
};
inc(); inc(); inc(); inc(); inc();
printf("counter = %d\n", counter);   // 5
```

### Multiple Reference Captures

```mingus
var min = 999;
var max = 0;
var track = [&min, &max](int x) => {
    if (x < min) { min = x; }
    if (x > max) { max = x; }
    return x;
};
track(50); track(10); track(90); track(30);
printf("min=%d max=%d\n", min, max);   // min=10 max=90
```

## Self-Capturing Closures

Closures can reference themselves for recursion:

```mingus
(int) => int fib = [=](int n) => {
    if (n <= 1) { return n; }
    return fib(n - 1) + fib(n - 2);
};

printf("fib(10) = %d\n", fib(10));   // 55
```

```mingus
(int) => int factorial = [=](int n) => {
    if (n <= 1) { return 1; }
    return n * factorial(n - 1);
};

printf("12! = %d\n", factorial(12));   // 479001600
```

Self-captures are reference-counted correctly even under deep recursion (tested up to 5000 levels).

## Weak Captures

Weak captures create non-owning references that don't prevent cleanup:

```mingus
var callback = [=]() => { return 42; };
var checker = [weak callback]() => {
    if (callback != null)
    {
        return callback();
    }
    return -1;
};

printf("alive: %d\n", checker());   // 42
```

### Breaking Cycles

```mingus
var value = 100;
var producer = [=]() => { return value; };
var consumer = [weak producer]() => {
    if (producer != null)
    {
        return producer() + 1;
    }
    return 0;
};
printf("result = %d\n", consumer());   // 101
```

### Mixed Strong and Weak

```mingus
var x = 10;
var multiplier = [=](int n) => { return n * x; };
var combined = [=, weak multiplier]() => {
    if (multiplier != null)
    {
        return multiplier(x);
    }
    return x;
};
printf("combined = %d\n", combined());   // 100
```

## Closures with Struct Parameters

Closures can accept struct-typed parameters:

```mingus
struct Vec2
{
    double x;
    double y;
}

func applyToVec(Vec2 v, (Vec2) => double f) => double
{
    return f(v);
}

Vec2 p;
p.x = 3.0;
p.y = 4.0;

var dist = [=](Vec2 v) => { return v.x * v.x + v.y * v.y; };
printf("dist^2 = %f\n", dist(p));   // 25.000000

// Through higher-order function
var r = applyToVec(p, [](Vec2 v) => { return v.x + v.y; });
printf("sum = %f\n", r);   // 7.000000
```

## Closures with Reference Parameters

Closures can take reference-typed parameters:

```mingus
var inc = [](int& n) => { n = n + 1; };
var x = 10;
inc(x);
printf("x = %d\n", x);   // 11

var swap = [](int& a, int& b) => {
    var tmp = a;
    a = b;
    b = tmp;
};
var a = 100;
var b = 200;
swap(a, b);
printf("a=%d b=%d\n", a, b);   // a=200 b=100
```

## Closure Factories

Functions can return closures — the captured values persist:

```mingus
func makeScaler(double factor) => (double) => double
{
    return [=](double x) => { return x * factor; };
}

var doubler = makeScaler(2.0);
var tripler = makeScaler(3.0);
printf("doubler(5) = %f\n", doubler(5.0));   // 10.000000
printf("tripler(5) = %f\n", tripler(5.0));   // 15.000000
```

### Composition

```mingus
func compose((double) => double f, (double) => double g) => (double) => double
{
    return [=](double x) => { return f(g(x)); };
}

var times6 = compose(doubler, tripler);
printf("times6(7) = %f\n", times6(7.0));   // 42.000000
```

## Pipe with Lambdas

The pipe operator works with both named closures and lambda literals:

```mingus
var result = 10.0 |> apply(doubler);
printf("10 |> doubler = %f\n", result);   // 20.000000
```

## Capture Mode Summary

| Capture | Syntax | Semantics |
|---------|--------|-----------|
| None | `[]` | Pure function, no outer access |
| All by value | `[=]` | Copies all referenced vars at definition |
| All by reference | `[&]` | Pointers to all referenced vars |
| Named by value | `[x, y]` | Copies specific vars |
| Named by reference | `[&x, &y]` | Pointers to specific vars |
| Mixed (value default) | `[=, &x]` | All by value, except x by reference |
| Mixed (ref default) | `[&, x]` | All by reference, except x by value |
| Weak | `[weak x]` | Non-owning reference to closure |
| Mixed with weak | `[=, weak x]` | By value default, x as weak reference |

## Known Limitations

- No shorthand lambda syntax (always need `[] ... => { ... }`)
- No `auto` return type for lambdas — the return type is inferred from the body
- By-reference captures are only safe within the lifetime of the captured variable
- Closures cannot be compared for equality (only null comparison works)
- No partial application or currying syntax
- Self-capturing closures add reference counting overhead

## See Also

- [Functions](04_functions.md) for regular function declarations
- [Pipe Operator](10_pipes.md) for using lambdas with pipes
- [Classes and OOP](08_classes_and_oop.md) for lambdas as class fields
- [Generics](11_generics.md) for generic higher-order functions
