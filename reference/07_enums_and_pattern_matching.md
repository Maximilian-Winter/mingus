# Enums and Pattern Matching

Mingus provides enums with explicit underlying types and `match` expressions for concise multi-way branching with pattern matching and guard clauses.

## Enum Declarations

An enum associates named constants with an underlying type:

```mingus
enum Color : int
{
    Red = 0,
    Green = 1,
    Blue = 2,
}
```

### Underlying Types

Enums can be backed by `int`, `byte`, or `string`:

```mingus
// Integer-backed enum
enum TokenKind : int
{
    Number = 0,
    Plus = 2,
    Star = 4,
}

// Byte-backed enum (smaller storage)
enum SmallFlags : byte
{
    None = 0,
    Read = 1,
    Write = 2,
    Execute = 4,
}

// String-backed enum
enum HttpMethod : string
{
    Get = "GET",
    Post = "POST",
    Delete = "DELETE",
}
```

### Using Enums

Access enum values with dot syntax:

```mingus
var r = Color.Red;
var g = Color.Green;
printf("Red = %d\n", r);      // 0
printf("Green = %d\n", g);    // 1
```

Enum values behave as their underlying type. Integer-backed enums support arithmetic and comparison:

```mingus
var next = Color.Red + 1;
printf("Red + 1 = %d\n", next);   // 1

if (r == Color.Red)
{
    puts("match!");
}
```

Byte-backed enums support bitwise combination:

```mingus
var combined = SmallFlags.Read + SmallFlags.Write;
printf("Read+Write = %d\n", combined);   // 3
```

String-backed enums can be printed directly:

```mingus
puts(HttpMethod.Get);      // GET
puts(HttpMethod.Post);     // POST
puts(HttpMethod.Delete);   // DELETE
```

## Match Expressions

`match` is a powerful expression that tests a value against multiple patterns and returns the result of the matching branch.

### Basic Match

```mingus
var name = match Color.Blue {
    Color.Red   => 10,
    Color.Green => 20,
    Color.Blue  => 30,
    _ => -1,
};
printf("match Blue = %d\n", name);   // 30
```

The `_` is a wildcard that matches anything not covered by earlier patterns.

### Match as Return Value

```mingus
func tokenPrecedence(int kind) => int
{
    return match kind {
        TokenKind.Plus  => 1,
        TokenKind.Minus => 1,
        TokenKind.Star  => 2,
        TokenKind.Slash => 2,
        _ => 0,
    };
}
```

### Boolean Match Results

```mingus
func isOperator(int kind) => bool
{
    return match kind {
        TokenKind.Plus  => true,
        TokenKind.Minus => true,
        TokenKind.Star  => true,
        TokenKind.Slash => true,
        _ => false,
    };
}
```

### Match with Guard Clauses

Add conditions to patterns with `if`:

```mingus
func softClip(double sample) => double
{
    return match sample {
        var x if x > 1.0  => 1.0,
        var x if x < -1.0 => -1.0,
        var x => x - (x * x * x) / 3.0,
    };
}
```

Here `var x` binds the matched value to a name, and `if x > 1.0` adds a guard condition. The first pattern whose guard is true wins.

### Classification with Guards

```mingus
func classify(double value) => int
{
    return match value {
        var x if x > 0.9  => 4,   // clipping
        var x if x > 0.7  => 3,   // hot
        var x if x > 0.3  => 2,   // nominal
        var x if x > 0.01 => 1,   // quiet
        _ => 0,                    // silent
    };
}
```

### Match with Tagged Unions

Match is the primary way to destructure tagged unions (see also [Structs and Data Types](06_structs_and_data_types.md)):

```mingus
tagged union Option {
    Some(int value),
    None
}

var opt = Option.Some(42);
match opt {
    Option.Some(var v) => printf("Some(%d)\n", v),
    Option.None => printf("None\n"),
};
```

**Output:**
```
Some(42)
```

## Switch Statements

For simple integer-based multi-way branching, use `switch`:

```mingus
var val = 2;
switch (val)
{
    case 1: puts("one");
    case 2: puts("two");       // This prints
    case 3: puts("three");
    default: puts("other");
}
```

**Output:**
```
two
```

There is **no fallthrough** — only the matched case executes. No `break` is needed.

### When to Use Match vs Switch

| Feature | `match` | `switch` |
|---------|---------|----------|
| Is an expression (returns value) | Yes | No |
| Guard clauses | Yes | No |
| Variable binding | Yes | No |
| Tagged union destructuring | Yes | No |
| Integer branching | Yes | Yes |

Use `match` when you need expressions, guards, or pattern destructuring. Use `switch` for simple integer dispatch.

## Known Limitations

- Enums cannot have methods
- No automatic value assignment (all values must be explicit)
- No flags/bitfield enum type (combine manually with arithmetic)
- `switch` only works on integers, not strings or enum types directly
- Match does not check exhaustiveness — always include a wildcard `_` or `default` case
- No nested pattern matching (e.g., matching a tuple of enums)

## See Also

- [Control Flow](03_control_flow.md) for `if`/`else` and loops
- [Structs and Data Types](06_structs_and_data_types.md) for tagged unions
- [Pipe Operator](10_pipes.md) for combining `match` with pipes
- [C FFI](14_c_ffi.md) for extern enums
