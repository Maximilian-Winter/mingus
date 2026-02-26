# Strings

Mingus has two string types: `string` (a C-compatible `char*` pointer) and `String` (a managed value type with length, capacity, and automatic memory management). Both interoperate seamlessly.

## C-Style Strings (`string`)

The `string` type is a null-terminated `char*`. String literals produce `string` values:

```mingus
string name = "Alice";
var greeting = "Hello";    // inferred as string
puts(name);
```

### Concatenation

The `+` operator concatenates strings, producing a new heap-allocated string:

```mingus
var hello = "Hello";
var world = " World";
var greeting = hello + world;
puts(greeting);   // Hello World
```

The `+=` operator appends in place:

```mingus
var s = "foo";
s += "bar";
puts(s);   // foobar
```

### Comparison

Strings compare by content (not pointer identity):

```mingus
var a = "abc";
var b = "abc";
var c = "xyz";
if (a == b) { puts("a == b: true"); }   // prints
if (a != c) { puts("a != c: true"); }   // prints
```

### Length

```mingus
var greeting = "Hello World";
printf("length: %d\n", greeting.length());   // 11
```

### Substring

Extract a portion of a string by start index and length:

```mingus
var greeting = "Hello World";
var sub = greeting.substring(0, 5);
puts(sub);   // Hello
```

### String Interpolation

Embed expressions in strings using `${...}`:

```mingus
var x = 42;
var msg = "value=${x}";
puts(msg);   // value=42
```

Interpolation works with integer variables. The result is a new string with the value substituted.

## String Value Type (`String`)

The `String` type is a managed value type with `{ data, length, capacity }`. It owns its buffer and frees it automatically when the variable goes out of scope (RAII).

### Construction

```mingus
String s = "hello";
printf("s: %s\n", s);           // hello
printf("len: %d\n", s.length()); // 5
```

### Concatenation

```mingus
String t = s + " world";
printf("concat: %s\n", t);        // hello world
printf("concat len: %d\n", t.length());  // 11

// Mixed: String + string literal
String u = s + "!";
printf("mixed: %s\n", u);   // hello!

// Compound assignment
String r = "foo";
r += "bar";
printf("+=: %s\n", r);   // foobar
```

### Equality

```mingus
String a = "abc";
String b = "abc";
String c = "xyz";
if (a == b) { puts("a == b: true"); }   // prints
if (a != c) { puts("a != c: true"); }   // prints
```

### Slicing

Extract a substring by start and end indices:

```mingus
String t = "hello world";
String sub = t.slice(0, 5);
printf("slice: %s\n", sub);   // hello
```

### Character Access

Get the integer value of a character at a given index:

```mingus
String s = "hello";
int ch = s.charAt(0);
printf("charAt(0): %c\n", ch);   // h
```

### C Interop

`String` values are automatically convertible to `string` (`char*`) when passed to functions expecting C strings:

```mingus
String s = "hello";
puts(s);   // Works — implicit conversion to char*
```

For explicit conversion, use `.cstr()`:

```mingus
string cval = s.cstr();
puts(cval);
```

## Escape Sequences

Both string types support standard C escape sequences in literals:

| Sequence | Meaning |
|----------|---------|
| `\n` | Newline |
| `\t` | Tab |
| `\\` | Backslash |
| `\"` | Double quote |
| `\0` | Null character |
| `%%` | Literal `%` in printf format |

```mingus
printf("line1\nline2\n");
printf("tab\there\n");
printf("100%%\n");
```

## String Methods Summary

| Method | Available On | Description |
|--------|-------------|-------------|
| `.length()` | `string`, `String` | Number of characters |
| `.substring(start, len)` | `string` | Extract substring by start index and length |
| `.slice(start, end)` | `String` | Extract substring by start and end indices |
| `.charAt(index)` | `String` | Get character as integer at index |
| `.cstr()` | `String` | Convert to C-style `string` |

## Known Limitations

- No raw string literals (all strings process escape sequences)
- No multi-line string literals
- String interpolation only supports `${variable}` — not arbitrary expressions
- No regular expression support
- No Unicode-aware string operations (strings are byte sequences)
- `substring()` is on `string`; `slice()` is on `String` — different APIs

## See Also

- [Getting Started](01_getting_started.md) for basic string output with `printf` and `puts`
- [Types and Values](02_types_and_values.md) for the `string` primitive type
- [C FFI](14_c_ffi.md) for passing strings to C functions
