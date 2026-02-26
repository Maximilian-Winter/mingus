# Mingus Language Reference

Mingus is a compiled systems language with modern features: classes with RAII, closures, generics, pattern matching, a pipe operator, and a C foreign function interface. It compiles to native code via LLVM.

This reference covers the language from the perspective of a Mingus programmer. For compiler internals, see the `docs/` folder.

## Table of Contents

| # | Chapter | Description |
|---|---------|-------------|
| 1 | [Getting Started](01_getting_started.md) | Modules, main function, variables, basic arithmetic, output |
| 2 | [Types and Values](02_types_and_values.md) | Primitive types, integers, floats, literals, constants, type aliases |
| 3 | [Control Flow](03_control_flow.md) | if/else, for, while, do-while, switch, labeled loops |
| 4 | [Functions](04_functions.md) | Declarations, reference parameters, overloading, varargs |
| 5 | [Strings](05_strings.md) | C-style strings, String value type, interpolation, escape sequences |
| 6 | [Structs and Data Types](06_structs_and_data_types.md) | Structs, operator overloading, tuples, arrays, unions, tagged unions |
| 7 | [Enums and Pattern Matching](07_enums_and_pattern_matching.md) | Enum declarations, match expressions, guards, switch |
| 8 | [Classes and OOP](08_classes_and_oop.md) | Classes, RAII, inheritance, interfaces, access modifiers, shared pointers |
| 9 | [Lambdas and Closures](09_lambdas_and_closures.md) | Lambdas, captures, higher-order functions, escape analysis |
| 10 | [Pipe Operator](10_pipes.md) | Data-flow piping, chaining, pipe-to-method |
| 11 | [Generics](11_generics.md) | Generic functions, types, interfaces, type inference, constraints |
| 12 | [Pointers and Memory](12_pointers_and_memory.md) | Pointers, raw blocks, const pointers, sizeof, heap allocation |
| 13 | [Modules and Imports](13_modules_and_imports.md) | Module declarations, imports, global variables, static locals |
| 14 | [C FFI](14_c_ffi.md) | Extern functions, opaque types, extern structs/enums, calling conventions |

## Suggested Reading Order

**If you're new to Mingus**, read chapters 1 through 8 in order. That gives you everything needed to write substantial programs. Then explore chapters 9-14 based on what you need.

**If you're an experienced programmer**, jump to any chapter directly. Each chapter is self-contained with syntax reference, examples, and cross-links.

## Conventions

Throughout this reference:

- Code examples are shown in `mingus` code blocks
- **Output** sections show the actual runtime output of each example
- All examples are derived from the compiler's passing test suite
- **See also** links point to related chapters
- **Known Limitations** sections at the end of each chapter list current restrictions honestly
