# Pipe Operator

The pipe operator `|>` passes the result of one expression as the first argument to the next function. It enables left-to-right data flow, making chains of transformations easier to read.

## Basic Pipe

The expression `a |> f` is equivalent to `f(a)`:

```mingus
func double_it(double x) => double
{
    return x * 2.0;
}

var result = 5.0 |> double_it;
printf("result = %f\n", result);   // 10.000000
```

## Pipe with Additional Arguments

When the target function takes multiple arguments, the piped value becomes the **first** argument:

```mingus
func applyGain(double sample, double gain) => double
{
    return sample * gain;
}

// 0.5 |> applyGain(2.0) means applyGain(0.5, 2.0)
var gained = 0.5 |> applyGain(2.0);
printf("gained = %f\n", gained);   // 1.000000
```

## Chaining Pipes

Pipes chain naturally, flowing left to right:

```mingus
func add_ten(double x) => double
{
    return x + 10.0;
}

// Read: start with 5.0, double it, add 10
var result = 5.0 |> double_it |> add_ten;
printf("result = %f\n", result);   // 20.000000
```

Compare with nested calls (harder to read):

```mingus
// Same thing without pipes:
var result = add_ten(double_it(5.0));
```

### Multi-Stage Processing

```mingus
func applyGain(double sample, double gain) => double
{
    return sample * gain;
}

func softClip(double sample) => double
{
    return match sample {
        var x if x > 1.0  => 1.0,
        var x if x < -1.0 => -1.0,
        var x => x - (x * x * x) / 3.0,
    };
}

func processSample(double sample) => double
{
    return sample
        |> applyGain(1.5)
        |> softClip;
}

var out = 0.7 |> applyGain(1.5) |> softClip;
printf("output = %f\n", out);   // 1.000000 (clipped)
```

## Pipe to Methods

The pipe operator works with class methods via the `->` syntax:

```mingus
class Transform
{
    double scaleX;
    double offsetX;

    constructor(double sx, double ox)
    {
        this.scaleX = sx;
        this.offsetX = ox;
    }

    func scale(double val) => double
    {
        return val * this.scaleX;
    }

    func offset(double val) => double
    {
        return val + this.offsetX;
    }

    func apply(double val, double extra) => double
    {
        return val * this.scaleX + extra;
    }
}

Transform* t = new Transform(2.0, 100.0);

// Pipe to method: 3.0 |> t->scale means t->scale(3.0)
var r1 = 3.0 |> t->scale;
printf("scale = %f\n", r1);   // 6.000000

var r2 = 5.0 |> t->offset;
printf("offset = %f\n", r2);  // 105.000000
```

### Chaining Method Pipes

```mingus
// 5.0 -> scale(5.0)=10.0 -> offset(10.0)=110.0
var r = 5.0 |> t->scale |> t->offset;
printf("chained = %f\n", r);   // 110.000000
```

### Mixing Free Functions and Methods

```mingus
// 5.0 -> double_it(5.0)=10.0 -> offset(10.0)=110.0
var r = 5.0 |> double_it |> t->offset;
printf("mixed = %f\n", r);   // 110.000000
```

### Method Pipe with Extra Arguments

```mingus
// 3.0 |> t->apply(7.0) means t->apply(3.0, 7.0) = 3.0*2.0 + 7.0 = 13.0
var r = 3.0 |> t->apply(7.0);
printf("apply = %f\n", r);   // 13.000000
```

## Pipe with Lambdas

Pipes work with lambda expressions and closure variables:

```mingus
func apply(double x, (double) => double f) => double
{
    return f(x);
}

var piped = 7.0 |> apply([](double x) => { return x + 3.0; });
printf("piped = %f\n", piped);   // 10.000000
```

Inside class methods, the pipe can feed values into function-typed parameters:

```mingus
class DynamicArray
{
    func map((int) => int transform) => DynamicArray
    {
        var result = DynamicArray(this.capacity);
        for (int i = 0; i < this.size; i++)
        {
            var val = this[i] |> transform;   // Pipe element to function
            result.push(val);
        }
        return result;
    }
}

var doubled = arr.map([](int x) => { return x * 2; });
```

## Pipe Syntax Summary

| Pattern | Meaning |
|---------|---------|
| `a \|> f` | `f(a)` |
| `a \|> f(b)` | `f(a, b)` |
| `a \|> f(b, c)` | `f(a, b, c)` |
| `a \|> obj->m` | `obj->m(a)` |
| `a \|> obj->m(b)` | `obj->m(a, b)` |
| `a \|> f \|> g` | `g(f(a))` |

The piped value always becomes the **first** argument.

## Known Limitations

- Cannot pipe into constructors
- Cannot pipe into static methods
- No placeholder syntax (the piped value is always the first parameter)
- No bidirectional pipes
- Cannot use `|>` with overloaded functions where the piped type is ambiguous

## See Also

- [Functions](04_functions.md) for function declarations
- [Lambdas and Closures](09_lambdas_and_closures.md) for using lambdas in pipes
- [Enums and Pattern Matching](07_enums_and_pattern_matching.md) for `match` expressions combined with pipes
