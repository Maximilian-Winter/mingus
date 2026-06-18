# Mingus Next Improvements

Prioritized roadmap for fixing issues and removing limitations, ranked by impact-to-effort ratio.
Excludes generics/templates and standard library (deferred to later milestones).
Current baseline: 95 passing tests (74 feature + 21 stress).

---

## Tier 1: Quick Bug Fixes [COMPLETED]

All three Tier 1 fixes implemented and verified in test_52. 95/95 tests pass.

### 1.1 Char Literal Escape Processing

**Source**: Sema Edge Case 5.13
**Severity**: Low | **Effort**: ~30 min

**Problem**: `ASTGenerator` reads `text[1]` for char literals without processing escape
sequences. `'\n'`, `'\t'`, `'\0'`, `'\\'`, `'\''` do not produce the expected control
characters.

**Fix**: Add escape sequence handling in `ASTGenerator::visitPrimaryExpression()` where
`CharLiteral` is parsed. Check for `\` prefix and map to the correct char value.

**Files**: `src/mingus/parser/ASTGenerator.cpp`

---

### 1.2 Duplicate Cross-Module Externs

**Source**: Codegen Edge Case 6.1
**Severity**: Medium | **Effort**: ~1 hr

**Problem**: If two modules both declare `extern func sin(double x) => double;`, codegen
creates `@sin` and `@sin.1` (LLVM name deduplication). The linker then fails because
`@sin.1` has no definition.

**Fix**: In `visit(ExternFunctionDeclaration&)`, before creating a new LLVM function
declaration, check if a function with the same name already exists in the LLVM module.
If it does and has a compatible signature, reuse it instead of creating a duplicate.

**Files**: `src/mingus/codegen/IRGenerator.cpp`

**Current workaround**: Declare the extern in one module and `import` it in others.

---

### 1.3 Operator Imports Not Transferred

**Source**: Sema Edge Case 5.7
**Severity**: Low | **Effort**: ~1-2 hrs

**Problem**: Whole-module import (`import Mod;`) transfers named symbols but does not
transfer operator overload definitions. Operators from imported modules are not available
for use in the importing module.

**Fix**: When resolving imports, also transfer operator overload registrations from the
source module's scope to the importing module's scope. Operator resolution in TypeChecker
must look through imported scopes.

**Files**: `src/mingus/sema/SymbolTableBuilder.cpp`, `src/mingus/sema/TypeChecker.cpp`

---

## Tier 2: Medium Fixes

Significant impact, moderate effort. Each is a focused campaign.

### 2.1 Forward Declarations / Forward Type References

**Source**: Sema Edge Cases 5.1, 5.2
**Severity**: Medium | **Effort**: ~3-4 hrs

**Problem**: A class used as a base must be defined before the derived class in source
order. A struct referencing another struct defined later in the file will fail. Mutual
class/struct references within a single module are not supported.

**Fix**: Two-pass approach in SymbolTableBuilder:
- Pass 1a: Walk all declarations and create type shells (ClassSymbol, StructSymbol) with
  names registered in scope, but without resolving fields or base classes.
- Pass 1b: Walk again to resolve fields, base classes, and method signatures using the
  now-complete type name registry.

This enables:
```
class Node {
    Node* next;      // self-reference (already works)
    Tree* owner;     // forward reference (currently fails)
}

class Tree {
    Node* root;      // back-reference
}
```

**Files**: `src/mingus/sema/SymbolTableBuilder.cpp`, possibly `src/mingus/sema/TypeResolver.cpp`

---

### 2.2 Constructor Overloading

**Source**: Sema Edge Case 5.8
**Severity**: Low | **Effort**: ~2-3 hrs

**Problem**: Each class supports only one regular constructor, one copy constructor, and
one move constructor. General constructor overloading with arbitrary signatures is not
supported.

**Fix**: Extend the existing function overloading mechanism (which already works for free
functions and methods) to constructors. ClassSymbol would hold a list of constructors
instead of a single `constructor` pointer. Overload resolution in TypeChecker already
has scoring — reuse it for constructor calls in `new` expressions and direct construction.

```
class Vec3 {
    double x; double y; double z;

    constructor() { x = 0.0; y = 0.0; z = 0.0; }
    constructor(double x, double y, double z) {
        this.x = x; this.y = y; this.z = z;
    }
    constructor(double scalar) {
        this.x = scalar; this.y = scalar; this.z = scalar;
    }
}
```

**Files**: `include/mingus/Symbols.h` (ClassSymbol), `src/mingus/sema/SymbolTableBuilder.cpp`,
`src/mingus/sema/TypeChecker.cpp`, `src/mingus/codegen/IRGenerator.cpp`

---

### 2.3 Definite Assignment Analysis

**Source**: Sema Edge Case 5.5
**Severity**: Medium | **Effort**: ~3-4 hrs

**Problem**: Variables can be read before assignment without a compiler error.
Uninitialized variables contain whatever was in the alloca (zero for zero-initialized
structs, undefined for primitives). This can cause subtle bugs.

**Fix**: Add a new sema pass (or extend SemanticValidator) that tracks assignment status
for local variables. At each read of a variable, check whether all control flow paths
leading to that point have assigned the variable. Warn or error on uninitialized reads.

Scope:
- Local variables only (not fields or parameters — those are always initialized)
- Straight-line code and simple if/else branches
- Conservative for loops (assume loop body may not execute)
- No interprocedural analysis

```
int x;
if (cond) { x = 1; }
printf("%d", x);     // WARNING: 'x' may be used uninitialized
```

**Files**: New pass or extension of `src/mingus/sema/SemanticValidator.cpp`

---

### 2.4 Float Literal Suffix

**Source**: Language Limitation 1.10
**Severity**: Low | **Effort**: ~1-2 hrs

**Problem**: `1.0` is always `double`. No `1.0f` suffix for `float`. Requires explicit
cast for `float` assignment.

**Fix**: In the lexer, allow `f` or `F` suffix on floating-point literals. In
ASTGenerator, detect the suffix and set the literal type to `float` instead of `double`.
TypeChecker already handles float vs double — just needs the literal to carry the
correct type.

```
float x = 1.0f;      // direct assignment, no cast needed
double y = 1.0;      // default: double (unchanged)
```

**Files**: `antlr4_grammar/MingusLexer.g4`, `src/mingus/parser/ASTGenerator.cpp`,
`src/mingus/sema/TypeChecker.cpp`

---

## Tier 3: Larger Features (Deferred)

Significant architectural decisions or broad scope. Defer until core language is more stable.

### 3.1 Exception Handling

**Source**: Language Limitation 1.7
**Severity**: Medium

**Problem**: No `try`/`catch`/`throw`. Error handling must use return codes or error enums.

**Why defer**: Requires architectural decision (LLVM `invoke`/`landingpad` stack unwinding
vs. `setjmp`/`longjmp` vs. algebraic `Result<T, E>` types). Stack unwinding interacts
with RAII cleanup. Each approach has trade-offs for code size, performance, and
complexity. Best tackled after the type system is more mature (generics would enable
`Result<T, E>`).

---

### 3.2 Null Safety

**Source**: Sema Edge Case 5.6
**Severity**: Medium

**Problem**: Pointers and nullable closures can be dereferenced without null checks.
No `?.` safe-navigation operator or nullable type system.

**Why defer**: Requires nullable type system (`T?` vs `T`), which touches type
compatibility, pattern matching, and every pointer operation. Better to design alongside
generics (`Optional<T>`). The `?.` operator also needs grammar, AST, sema, and codegen
changes.

---

### 3.3 Minimal Error Recovery

**Source**: Tooling Limitation 4.1
**Severity**: Medium

**Problem**: First parse or semantic error typically stops compilation. No multi-error
reporting.

**Why defer**: ANTLR4 has error recovery mechanisms (`DefaultErrorStrategy`), but
integrating them cleanly with the AST generation requires careful design. Sema error
recovery (skip bad declarations, continue checking) needs sentinel/error types
throughout the pipeline. High effort, many edge cases.

---

### 3.4 String Type

**Source**: Language Limitation 1.4
**Severity**: Medium

**Problem**: Strings are C-style `char*`. No length tracking, slicing, or Unicode.

**Why defer**: A proper String class benefits greatly from generics (for string views,
iterators, etc.) and operator overloading improvements. The current `char*` + RAII
string concat is functional for basic use. Best revisited after generics.

---

### 3.5 DWARF/PDB Debug Info

**Source**: Tooling Limitation 4.2
**Severity**: Medium

**Problem**: DIBuilder infrastructure exists but debug info emission is not complete.
Binaries cannot be stepped through in a debugger.

**Why defer**: Requires systematic debug location attachment to every IR instruction,
scope tracking for variables, and platform-specific testing (DWARF on Linux, PDB on
Windows via `lld-link`). Functional but low urgency while the language is still evolving
rapidly. Better to complete when the IR generation is more stable.

---

## Implementation Order

Recommended sequence:

1. **Tier 1 batch**: Char escapes + duplicate externs + operator imports (~3-4 hrs total)
2. **Forward declarations** (Tier 2.1): Enables mutual type references (~3-4 hrs)
3. **Constructor overloading** (Tier 2.2): Natural extension of existing overloading (~2-3 hrs)
4. **Float literal suffix** (Tier 2.4): Small quality-of-life fix (~1-2 hrs)
5. **Definite assignment analysis** (Tier 2.3): Compiler hardening (~3-4 hrs)

Total estimated effort for Tiers 1-2: ~13-17 hours across multiple sessions.
