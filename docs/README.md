# Mingus Documentation

This directory is the documentation hub for Mingus. It is organized by audience:

## For Mingus programmers

**[Language Reference](reference/00_index.md)** — the canonical, programmer-facing documentation for the Mingus language. 15 chapters covering everything from `Hello, World!` through generics, lambdas, RAII, and the C FFI.

| # | Chapter | Description |
|---|---------|-------------|
| 1 | [Getting Started](reference/01_getting_started.md) | Modules, main, variables, arithmetic, output |
| 2 | [Types and Values](reference/02_types_and_values.md) | Primitives, extended integers, literals, constants, typedef |
| 3 | [Control Flow](reference/03_control_flow.md) | if/else, for/while/do-while, switch, labeled loops |
| 4 | [Functions](reference/04_functions.md) | Declarations, ref params, overloading, varargs |
| 5 | [Strings](reference/05_strings.md) | C-strings, the `String` value type, interpolation |
| 6 | [Structs and Data Types](reference/06_structs_and_data_types.md) | Structs, operator overloading, tuples, arrays, unions |
| 7 | [Enums and Pattern Matching](reference/07_enums_and_pattern_matching.md) | Enums, `match`, guards, `switch` |
| 8 | [Classes and OOP](reference/08_classes_and_oop.md) | Classes, RAII, inheritance, interfaces, access modifiers |
| 9 | [Lambdas and Closures](reference/09_lambdas_and_closures.md) | Lambdas, captures, higher-order functions, escape analysis |
| 10 | [Pipe Operator](reference/10_pipes.md) | Data-flow piping, chaining, pipe-to-method |
| 11 | [Generics](reference/11_generics.md) | Generic functions and types, constraints, inference |
| 12 | [Pointers and Memory](reference/12_pointers_and_memory.md) | Pointers, raw blocks, const pointers, `sizeof`, heap allocation |
| 13 | [Modules and Imports](reference/13_modules_and_imports.md) | Module declarations, imports, globals, static locals |
| 14 | [C FFI](reference/14_c_ffi.md) | `extern` functions, opaque types, calling conventions, packed layout |

**Suggested reading order:** chapters 1–8 in sequence for a working knowledge of the language. Then explore chapters 9–14 by need.

## For compiler developers

These documents describe how the Mingus compiler is built. Read these if you want to modify the compiler, write a new backend, or understand the implementation choices.

- [V2 Architecture](V2_ARCHITECTURE.md) — overall design of the v2 compiler
- [Grammar and AST](GRAMMAR_AND_AST.md) — ANTLR4 grammar, AST node types, visitor pattern
- [Semantic Analysis](SEMANTIC_ANALYSIS.md) — the four-pass type checker
- [Type System and Dispatch](TYPE_SYSTEM_AND_DISPATCH.md) — type representation, overload resolution, vtable layout
- [Codegen Patterns](CODEGEN_PATTERNS.md) — LLVM IR generation patterns for each language feature
- [Memory and Lifetimes](MEMORY_AND_LIFETIMES.md) — RAII lowering, escape analysis, destructor placement
- [Memory Model](MEMORY_MODEL.md) — value semantics, move/copy, ownership rules

## Project status and roadmap

- [Mingus Status](MINGUS_STATUS.md) — current state, test counts, what works, what doesn't
- [Known Limitations](KNOWN_LIMITATIONS.md) — consolidated list of every known issue with severity
- [Next Improvements](NEXT_IMPROVEMENTS.md) — short- and long-term roadmap

## Essays

- [misc_docs/the_art_of_lowering.md](misc_docs/the_art_of_lowering.md) — *"Lowering is unbundling."* A practical guide to transforming high-level language constructs into machine operations, written from the perspective of building Mingus.

---

If you find an error or want to suggest a doc improvement, please open an issue. The language reference is the source of truth for what Mingus programs should look like; the architecture docs are the source of truth for how the compiler works.
