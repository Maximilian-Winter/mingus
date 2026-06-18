# Control Flow

Mingus provides familiar control flow constructs: conditional branching with `if`/`else`, three loop forms (`for`, `while`, `do-while`), multi-way branching with `switch`, and labeled loops for breaking out of nested structures.

## If / Else

The standard conditional. Parentheses around the condition are required; braces around the body are required.

```mingus
if (x > 0)
{
    printf("positive\n");
}
else if (x == 0)
{
    printf("zero\n");
}
else
{
    printf("negative\n");
}
```

Conditions can use any comparison or logical operator:

```mingus
if (sum > 25)
{
    puts("sum > 25: PASS");
}

if (a > 0 && b > 0)
{
    puts("both positive");
}

if (!done || retries > 3)
{
    puts("try again");
}
```

## For Loops

### Basic For Loop

C-style `for` with init, condition, and update:

```mingus
var total = 0;
for (int i = 0; i < 10; i++)
{
    total = total + i;
}
printf("sum(0..9) = %d\n", total);   // 45
```

The loop variable can use an explicit type or `var`:

```mingus
for (var i = 0; i < 5; i++)
{
    printf("%d\n", i);
}
```

### Multi-Init For Loops

A `for` loop can declare multiple variables in its init clause, separated by commas. Each variable needs its own type or `var`:

```mingus
// Two typed declarations
var sum = 0;
for (int i = 0, int j = 10; i < j; i = i + 1, j = j - 1)
{
    sum = sum + i + j;
}
printf("converge sum = %d\n", sum);   // 50
```

```mingus
// Two inferred declarations
var product = 1;
for (var a = 1, var b = 5; a <= b; a = a + 1, b = b - 1)
{
    product = product * a;
}
printf("product = %d\n", product);   // 6
```

```mingus
// Mixed: typed + inferred
var total = 0;
for (int x = 0, var y = 100; x < 5; x = x + 1)
{
    total = total + y;
    y = y - 10;
}
printf("total = %d\n", total);   // 400
```

The update clause also supports multiple expressions separated by commas.

### Nested Loops

Loops nest naturally:

```mingus
var matrix_sum = 0;
for (int row = 0; row < 3; row++)
{
    for (int col = 0; col < 3; col++)
    {
        matrix_sum = matrix_sum + (row * 3 + col);
    }
}
printf("matrix sum = %d\n", matrix_sum);   // 36
```

## While Loops

Execute the body as long as the condition is true:

```mingus
var count = 0;
var n = 1;
while (n < 100)
{
    n = n * 2;
    count++;
}
printf("2^k >= 100: k = %d\n", count);   // 7
```

## Do-While Loops

Execute the body **at least once**, then repeat while the condition holds:

```mingus
var i = 0;
do {
    i++;
} while (i < 5);
printf("count to 5: %d\n", i);   // 5
```

The key difference from `while`: the body always runs at least once, even if the condition is immediately false:

```mingus
var once = 0;
do {
    once = 42;
} while (false);
printf("once: %d\n", once);   // 42
```

### Break and Continue in Do-While

Both `break` and `continue` work inside `do-while` loops:

```mingus
// Break exits the loop
var sum = 0;
var j = 0;
do {
    j++;
    if (j == 4)
    {
        break;
    }
    sum = sum + j;
} while (j < 10);
printf("break sum: %d\n", sum);   // 6 (1+2+3)
```

```mingus
// Continue skips to the condition check
var total = 0;
var k = 0;
do {
    k++;
    if (k == 3)
    {
        continue;     // skips adding 3
    }
    total = total + k;
} while (k < 5);
printf("continue total: %d\n", total);   // 12 (1+2+4+5)
```

### Nested Do-While

```mingus
var outer = 0;
var inner_sum = 0;
do {
    outer++;
    var inner = 0;
    do {
        inner++;
        inner_sum = inner_sum + 1;
    } while (inner < 3);
} while (outer < 2);
printf("nested: outer=%d inner_sum=%d\n", outer, inner_sum);
// outer=2 inner_sum=6
```

## Switch Statements

Multi-way branching on integer values. Each `case` is a separate branch with **no fallthrough** (unlike C):

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

Only the matched case executes. If no case matches, `default` executes. There is no `break` needed between cases.

## Labeled Loops

When working with nested loops, you can label outer loops and use `break label` or `continue label` to control which loop is affected.

### Labeled Break

Break out of an outer loop from inside an inner loop:

```mingus
outer: for (int i = 0; i < 5; i++)
{
    for (int j = 0; j < 5; j++)
    {
        if (j == 2)
        {
            printf("Breaking outer at i=%d\n", i);
            break outer;
        }
    }
}
printf("After outer loop\n");
```

**Output:**
```
Breaking outer at i=0
After outer loop
```

### Labeled Continue

Skip to the next iteration of an outer loop:

```mingus
outer: for (int i = 0; i < 3; i++)
{
    for (int j = 0; j < 3; j++)
    {
        if (j == 1)
        {
            continue outer;   // skips rest of inner loop
        }
        printf("i=%d j=%d\n", i, j);
    }
}
```

**Output:**
```
i=0 j=0
i=1 j=0
i=2 j=0
```

Only `j=0` prints for each `i` because `continue outer` fires when `j==1`.

### Labels on All Loop Types

Labels work on `for`, `while`, and `do-while` loops:

```mingus
// Labeled while
var i = 0;
outer: while (i < 5)
{
    var j = 0;
    while (j < 5)
    {
        if (i == 1 && j == 1)
        {
            break outer;
        }
        j++;
    }
    i++;
}
printf("Exited at i=%d\n", i);   // 1
```

```mingus
// Labeled do-while
var i = 0;
outer: do
{
    var j = 0;
    do
    {
        if (i == 2 && j == 0)
        {
            break outer;
        }
        j++;
    } while (j < 3);
    i++;
} while (i < 5);
printf("Exited at i=%d\n", i);   // 2
```

### Triple-Nested Labeled Break

Labels can break out of arbitrarily deep nesting:

```mingus
var count = 0;
outer: for (int i = 0; i < 10; i++)
{
    for (int j = 0; j < 10; j++)
    {
        for (int k = 0; k < 10; k++)
        {
            count++;
            if (count == 15)
            {
                break outer;
            }
        }
    }
}
printf("Total iterations: %d\n", count);   // 15
```

## Known Limitations

- No `for-in` or range-based loops
- No `break` with value (use a variable instead)
- Switch only works on integer values, not strings or enums directly
- No computed `goto`
- Labels must be immediately followed by a loop — you cannot label arbitrary statements

## See Also

- [Getting Started](01_getting_started.md) for basic arithmetic and output
- [Enums and Pattern Matching](07_enums_and_pattern_matching.md) for `match` expressions and `switch` with enums
- [Functions](04_functions.md) for organizing loop logic into functions
