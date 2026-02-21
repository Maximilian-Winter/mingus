# Mingus Semantic Analysis Pipeline

The Mingus compiler performs four sequential semantic analysis passes over the AST before code generation. Each pass is implemented as an `ASTVisitor` that walks the entire AST, reading data deposited by prior passes and annotating new information onto AST nodes and symbol objects.

**Source files:**

| File | Role |
|------|------|
| `src/mingus/sema/SymbolTableBuilder.cpp` | Pass 1 (~900 lines) |
| `src/mingus/sema/TypeResolver.cpp` | Pass 2 (~670 lines) |
| `src/mingus/sema/TypeChecker.cpp` | Pass 3 (~1260 lines) |
| `src/mingus/sema/SemanticValidator.cpp` | Pass 4 (~880 lines) |
| `include/mingus/Symbol.h` | Symbol base classes |
| `include/mingus/Symbols.h` | Concrete symbol types |
| `include/mingus/TypeSymbol.h` | Type-as-symbol hierarchy |
| `include/mingus/Scope.h` | Scope hierarchy |
| `include/mingus/SymbolTable.h` | Scope tree owner, type interning |
| `include/mingus/sema/ErrorReporter.h` | Diagnostic collection |

---

## Table of Contents

1. [Overview](#1-overview)
2. [Symbol System](#2-symbol-system)
3. [Scope System](#3-scope-system)
4. [Pass 1: SymbolTableBuilder](#4-pass-1-symboltablebuilder)
5. [Pass 2: TypeResolver](#5-pass-2-typeresolver)
6. [Pass 3: TypeChecker](#6-pass-3-typechecker)
7. [Pass 4: SemanticValidator](#7-pass-4-semanticvalidator)
8. [Error Reporting](#8-error-reporting)
9. [Key Invariants](#9-key-invariants)

---

## 1. Overview

### Four-Pass Architecture

```
AST (from ANTLR4/ASTGenerator)
  |
  v
Pass 1: SymbolTableBuilder
  -> Scope tree, symbol objects, vtables, auto ctor/dtor, imports
  |
  v
Pass 2: TypeResolver
  -> Resolves TypeNode -> TypeSymbol for all annotations
  |
  v
Pass 3: TypeChecker
  -> Types all expressions, resolves identifiers, enforces compatibility
  |
  v
Pass 4: SemanticValidator
  -> RAII tracking, lambda captures, control flow, exhaustiveness
  |
  v
IRGenerator (codegen)
```

### Why This Order

The four passes form a strict dependency chain. Each pass requires data from all prior passes but never modifies their output:

1. **Pass 1 must run first** because all subsequent passes need the scope tree and symbol objects. You cannot resolve a type name until the named symbol exists in a scope. Import resolution requires all module scopes to exist, so Pass 1 internally splits into Phase 1a (build everything) and Phase 1b (resolve imports).

2. **Pass 2 must follow Pass 1** because it maps `TypeNode` AST annotations to `TypeSymbol` instances by looking up names in the scope tree. It sets the `type` field on `VariableSymbol` and `returnType` on `FunctionSymbol` using only explicit annotations -- it does not enter function bodies or perform inference.

3. **Pass 3 must follow Pass 2** because expression type inference depends on knowing the declared types of variables, function parameters, and return types. When type-checking `x + y`, Pass 3 looks up the types that Pass 2 resolved. For `var`-declared variables, Pass 3 performs inference from the initializer expression.

4. **Pass 4 must follow Pass 3** because lambda capture analysis needs to know which identifiers resolve to which symbols (set by Pass 3), and RAII tracking needs to know the fully-resolved types of local variables (also set by Pass 3). Return completeness checking needs the return type established in Pass 2.

Each pass is **strictly additive** -- no pass modifies data written by a prior pass. This makes the pipeline easy to reason about and debug: if a bug appears in expression types, it is in Pass 3; if a capture is wrong, it is in Pass 4.

### Invocation

The compiler driver invokes the four passes in sequence:

```cpp
SymbolTable symbolTable;
ErrorReporter errors;

SymbolTableBuilder pass1(symbolTable, errors);
pass1.build(program);  // Phase 1a + 1b

TypeResolver pass2(symbolTable, errors);
pass2.resolve(program);

TypeChecker pass3(symbolTable, errors);
pass3.check(program);

SemanticValidator pass4(symbolTable, errors);
pass4.validate(program);

// pass4.getRAIIInfo() -> handed to IRGenerator
```

---

## 2. Symbol System

The symbol system uses two core abstractions from `Symbol.h`:

- **`Symbol`** -- abstract interface for any named entity (variable, function, type, module)
- **`SymbolWithScope`** -- the key dual-nature pattern: inherits from both `BaseScope` and `Symbol` via multiple inheritance, meaning a single object IS both a named symbol and a scope that contains other symbols

### Symbol Hierarchy

```
Symbol (abstract interface)
+-- BaseSymbol (leaf symbols -- no scope of their own)
|   +-- TypedSymbol (abstract -- getType()/setType() for TypeSymbol)
|       +-- VariableSymbol
+-- SymbolWithScope (MI: BaseScope + Symbol -- the dual-nature pattern)
    +-- TypeSymbol (base for ALL types -- every type IS a scope)
    |   +-- PrimitiveTypeSymbol (int, double, float, byte, char, string, bool, void)
    |   +-- PointerTypeSymbol (T*)
    |   +-- ArrayTypeSymbol (T[N])
    |   +-- TupleTypeSymbol ((T1, T2, ...))
    |   +-- FunctionTypeSymbol ((T1, T2) => R)
    |   +-- ReferenceTypeSymbol (T& -- transient, unwrapped by Pass 2)
    |   +-- ErrorTypeSymbol (sentinel for error recovery)
    |   +-- NullTypeSymbol (compatible with pointers and closures)
    |   +-- ClassSymbol (class declarations)
    |   +-- StructSymbol (struct declarations)
    |   +-- EnumSymbol (enum declarations)
    |   +-- InterfaceSymbol (interface declarations)
    +-- FunctionSymbol (named functions)
    |   +-- MethodSymbol (instance methods)
    |   +-- ConstructorSymbol (class constructors)
    |   +-- DestructorSymbol (class destructors)
    |   +-- OperatorSymbol (operator overloads)
    +-- ModuleSymbol (module declarations)
```

### VariableSymbol

Defined in `Symbols.h`. Represents locals, parameters, and struct/class fields.

| Field | Type | Description |
|-------|------|-------------|
| `role` | `VariableRole` | `Local`, `Parameter`, or `Field` |
| `isReference` | `bool` | True for `T&` parameters -- pass-by-pointer semantics |
| `isInferred` | `bool` | True for `var x = expr` declarations |
| `isMutable` | `bool` | True unless declared `const` |
| `isInitialized` | `bool` | Tracks definite assignment |
| `fieldIndex` | `int` | GEP index for struct/class fields (-1 if not a field) |
| `accessLevel` | `AccessModifier` | `Public`, `Private`, or `Protected` |

The `role` field is set in Pass 1:
- Inside a struct/class scope (`inTypeScope_ == true`): `Field`
- Function parameters: `Parameter`
- Everything else: `Local`

### FunctionSymbol

Defined in `Symbols.h`. Extends `SymbolWithScope`, meaning a function IS the scope for its body. Parameters are `VariableSymbol` instances defined within this scope.

| Field | Type | Description |
|-------|------|-------------|
| `returnType` | `TypeSymbolPtr` | Set by Pass 2 |
| `parameters` | `vector<shared_ptr<VariableSymbol>>` | Ordered parameter list |
| `isMethod` | `bool` | True if declared inside a class/struct |
| `isExtern` | `bool` | True for `extern func` declarations |
| `isStatic` | `bool` | Static methods do not receive `this` |
| `isAbstract` | `bool` | No body, must be overridden |
| `isVirtual` | `bool` | Has a vtable slot |
| `isVariadic` | `bool` | Accepts variable arguments (extern only) |
| `hasThisParam` | `bool` | True for non-static methods |
| `vtableIndex` | `int` | Slot in the class vtable (-1 if not virtual) |
| `accessLevel` | `AccessModifier` | Public/Private/Protected |

`buildFunctionType()` constructs a `FunctionTypeSymbol` from the function's parameter list and return type, preserving `ParameterInfo::isReference` for each parameter. This is used whenever a function reference is stored in a variable or passed as an argument.

Subclasses:
- **`MethodSymbol`**: Adds `classOfThisMethod` to link back to the owning class.
- **`ConstructorSymbol`**: Always `isMethod = true`, `hasThisParam = true`. Name is `"constructor"`.
- **`DestructorSymbol`**: Always `isMethod = true`, `hasThisParam = true`. Name is `"destructor"`.
- **`OperatorSymbol`**: Adds `op` (the `OverloadableOp` enum value) and `ownerType`.

### ClassSymbol

Defined in `Symbols.h`. Extends `TypeSymbol` (which extends `SymbolWithScope`), so a class IS both a type and a scope for its members.

| Field | Type | Description |
|-------|------|-------------|
| `baseClassNames` | `vector<string>` | Unresolved names from the parse tree |
| `resolvedBaseClass` | `ClassSymbol*` | Resolved in Pass 1 |
| `isAbstract` | `bool` | Cannot be instantiated directly |
| `fields` | `vector<shared_ptr<VariableSymbol>>` | Own fields only |
| `allFields` | `vector<shared_ptr<VariableSymbol>>` | Inherited + own fields (LLVM GEP order) |
| `constructor` | `shared_ptr<ConstructorSymbol>` | Always present (auto-generated if missing) |
| `destructor` | `shared_ptr<DestructorSymbol>` | Always present (auto-generated if missing) |
| `vtable` | `vector<shared_ptr<FunctionSymbol>>` | Virtual method slots (slot 0 = destructor) |
| `vtableSize` | `int` | Number of vtable entries |
| `implementedInterfaces` | `vector<shared_ptr<InterfaceSymbol>>` | Interfaces this class implements |

Key methods:
- `hasRAII()`: Returns true if the class has a destructor (always true due to auto-generation).
- `hasVtable()`: Returns true if `vtableSize > 0`.
- `resolve(name)`: Overridden to walk the inheritance chain -- checks own scope, then base class scope recursively.

### StructSymbol

Defined in `Symbols.h`. Extends `TypeSymbol`. Simpler than `ClassSymbol`: no inheritance, no vtable, no constructor/destructor.

| Field | Type | Description |
|-------|------|-------------|
| `fields` | `vector<shared_ptr<VariableSymbol>>` | All fields |

`needsCleanup()` returns true if any field is closure-typed (requires synthetic cleanup function in codegen).

### EnumSymbol

Defined in `Symbols.h`. Extends `TypeSymbol`.

| Field | Type | Description |
|-------|------|-------------|
| `members` | `vector<MemberInfo>` | Name, integer value, string value, explicit flag |
| `underlyingType` | `TypeSymbolPtr` | Default `int`; can be `string` for string-backed enums |

Each `MemberInfo` has:
- `name`: The member identifier
- `intValue`: Auto-incremented integer value (or explicit)
- `stringValue`: For string-backed enums
- `hasExplicitValue`: Whether the value was explicitly assigned

`findMember(name)` searches the members vector by name.

### InterfaceSymbol

Defined in `Symbols.h`. Extends `TypeSymbol`.

| Field | Type | Description |
|-------|------|-------------|
| `methods` | `vector<shared_ptr<FunctionSymbol>>` | Abstract method declarations |

Each method gets a `vtableIndex` assigned sequentially (0-based) for itable dispatch at runtime.

### ModuleSymbol

Defined in `Symbols.h`. Extends `SymbolWithScope`, so a module IS the scope for its top-level declarations.

| Field | Type | Description |
|-------|------|-------------|
| `importedModules` | `vector<shared_ptr<ModuleSymbol>>` | Modules imported by this module |

---

## 3. Scope System

### Scope Hierarchy

Defined in `Scope.h`:

```
Scope (abstract interface)
+-- BaseScope (concrete: symbol map, enclosing chain, nesting)
    +-- GlobalScope (root scope, one per compilation)
    +-- BlockScope (anonymous { } blocks, for bodies, match arms)
```

`SymbolWithScope` also extends `BaseScope` -- this is the key multiple-inheritance pattern. `ClassSymbol`, `FunctionSymbol`, and `ModuleSymbol` are all both symbols and scopes.

### Scope Interface

Every scope provides:

```cpp
// Name resolution: walk up enclosing chain
SymbolPtr resolve(const string& name) const;

// Define a symbol in this scope
void define(const SymbolPtr& sym);

// All symbols directly in this scope
vector<SymbolPtr> getAllSymbols() const;

// Scope chain navigation
ScopePtr getEnclosingScope() const;
void setEnclosingScope(const ScopePtr& scope);

// Nest a child scope (block scopes)
void nest(const ScopePtr& childScope);

// Operator overload management (separate namespace)
void defineOperator(const shared_ptr<OperatorSymbol>& op);
shared_ptr<OperatorSymbol> resolveOperator(OverloadableOp op) const;
```

### Name Resolution (`resolve`)

`BaseScope::resolve(name)` implements the standard scope chain walk:

1. Check this scope's local symbol map
2. If not found, delegate to `enclosingScope_->resolve(name)`
3. Continue until `GlobalScope` (which has no enclosing scope)

`ClassSymbol` overrides `resolve()` to also walk the inheritance chain: after checking own symbols, it checks `resolvedBaseClass->resolve(name)`.

### How Class Scope Enables Bare Field Access

Inside a class method, the method's `FunctionSymbol` scope has the `ClassSymbol` as its enclosing scope. When code references a bare identifier like `field`, the resolution chain is:

1. Check the function's local scope (parameters, local variables)
2. Check the class scope (fields, methods)
3. Check the module scope
4. Check the global scope

This means `field` resolves to the class field without requiring `this.field`, because the class scope is in the enclosing chain.

### Scope Kinds in Mingus

| Construct | Scope Kind | Notes |
|-----------|-----------|-------|
| Program root | `GlobalScope` | One per compilation |
| `module X { }` | `ModuleSymbol` (is scope) | All top-level decls live here |
| `class X { }` | `ClassSymbol` (is scope) | Fields, methods, ctor, dtor |
| `struct X { }` | `StructSymbol` (is scope) | Fields, methods, operators |
| `interface X { }` | `InterfaceSymbol` (is scope) | Abstract method signatures |
| `func f() { }` | `FunctionSymbol` (is scope) | Parameters + body statements |
| `constructor() { }` | `ConstructorSymbol` (is scope) | Parameters + body |
| `destructor() { }` | `DestructorSymbol` (is scope) | Body only (no params) |
| `operator+() { }` | `OperatorSymbol` (is scope) | Parameters + body |
| `{ }` block | `BlockScope` | Anonymous child scope |
| `for (...) { }` | `BlockScope` | Loop variable visible in body |
| match arm | `BlockScope` | Binding pattern variables |
| lambda `[=](x) => { }` | `BlockScope` (label `__lambda`) | Parameter symbols |

### SymbolTable

Defined in `SymbolTable.h`. The `SymbolTable` is the root owner of:

1. **The `GlobalScope`** -- root of the entire scope tree
2. **The current scope pointer** -- maintained via `pushScope()`/`popScope()` during Pass 1 construction
3. **The type interning map** -- ensures structural type equality (same `T*` produces the same `PointerTypeSymbol` instance)

**Primitive types** are pre-registered on construction: `getIntType()`, `getDoubleType()`, `getFloatType()`, `getByteType()`, `getCharType()`, `getStringType()`, `getBoolType()`, `getVoidType()`.

**Compound types** are interned by structural key:
- `getPointerType(baseType)` -- interns `T*`
- `getArrayType(elementType, size)` -- interns `T[N]`
- `getTupleType(elementTypes)` -- interns `(T1, T2, ...)`
- `getFunctionType(params, returnType)` -- interns `(T1, T2) => R`
- `getReferenceType(baseType)` -- interns `T&` (transient, unwrapped by Pass 2)

**Sentinel types**: `getErrorType()` (absorbs cascading errors) and `getNullType()` (compatible with pointers and closures).

**User types** are registered by name via `registerType(name, typeSymbol)` and looked up via `resolveType(name)`.

**Type compatibility** is checked via `isCompatible(from, to)`:
1. Same type instance: always compatible
2. Either side is `ErrorTypeSymbol`: compatible (prevents cascading errors)
3. `NullTypeSymbol` to `PointerTypeSymbol` or `FunctionTypeSymbol`: compatible
4. Numeric widening: `byte` -> `int`, `char` -> `int`, `int` -> `float`/`double`, `float` -> `double`
5. Enum to underlying type and vice versa
6. `byte*` as universal pointer (like `void*`)
7. Inheritance: `Derived*` -> `Base*`, `Derived` -> `Base`
8. Interface: `ConcreteClass*` -> `Interface*` if class implements interface
9. Reference: `T` -> `T&` (implicit address-of at call site)

---

## 4. Pass 1: SymbolTableBuilder

**Header:** `include/mingus/sema/SymbolTableBuilder.h`
**Source:** `src/mingus/sema/SymbolTableBuilder.cpp`

### Purpose

Pass 1 creates all named symbols, builds the scope tree, resolves imports, builds vtables, and auto-generates constructors/destructors. After Pass 1 completes, every AST node has an `astScopeNode` pointer linking it to its enclosing scope, and every declaration node has a `resolvedXxx` pointer to its corresponding symbol.

### Execution Structure

```cpp
void SymbolTableBuilder::build(ProgramNode& program) {
    // Phase 1a: Build scope tree and symbols
    visit(program);

    // Phase 1b: Resolve imports (all module scopes now exist)
    resolveAllImports(program);
}
```

Phase 1a visits every module and declaration, creating symbols and building the scope tree. Phase 1b runs separately because imports require all module scopes to exist before cross-module symbol injection can work.

### State Tracking

```cpp
bool inTypeScope_ = false;          // true when inside struct/class members
shared_ptr<ClassSymbol> currentClass_;  // for ctor/dtor context
```

`inTypeScope_` determines whether a `VariableDeclaration` creates a `Field` or `Local` symbol.

### Scope Helpers

Two key helpers establish scope context:

- `setScope(node)`: Sets `node.astScopeNode = symbolTable_.getCurrentScope()` -- stamps the current scope onto the node without creating a new scope.
- `pushBlockScope(node)`: Creates a new `BlockScope`, nests it in the current scope, pushes it onto the scope stack, and sets it as the node's scope.

### Symbol Creation for Declarations

**Variables:**

```cpp
void SymbolTableBuilder::visit(VariableDeclaration& node) {
    auto varSym = make_shared<VariableSymbol>(node.name, nullptr);
    varSym->role = inTypeScope_ ? VariableRole::Field : VariableRole::Local;
    varSym->isInferred = node.isInferred;
    varSym->isMutable = !node.isConst;
    varSym->accessLevel = node.accessModifier;
    symbolTable_.defineSymbol(varSym);
    node.resolvedVariable = varSym;
    // Walk initializer for nested lambdas/declarations
}
```

The type is left as `nullptr` -- Pass 2 fills it in.

**Functions:**

```cpp
void SymbolTableBuilder::visit(FunctionDeclaration& node) {
    // If inTypeScope_: create MethodSymbol with classOfThisMethod
    // Otherwise: create FunctionSymbol
    // Set isStatic, isAbstract, isMethod, hasThisParam, accessLevel
    symbolTable_.defineSymbol(funcSym);
    node.resolvedFunction = funcSym;

    // Push function as scope, create parameter symbols inside it
    symbolTable_.pushScope(funcSym);
    createParameterSymbols(node.parameters, funcSym);
    visitStatements(node.body->statements);
    symbolTable_.popScope();
}
```

Key detail: the `FunctionSymbol` IS the scope -- `pushScope(funcSym)` pushes the function symbol itself.

**Parameters:**

```cpp
void SymbolTableBuilder::createParameterSymbols(...) {
    for (auto& param : params) {
        auto paramSym = make_shared<VariableSymbol>(param->name, nullptr);
        paramSym->role = VariableRole::Parameter;
        paramSym->isReference = param->isReference;
        symbolTable_.defineSymbol(paramSym);
        funcSym->parameters.push_back(paramSym);
        param->resolvedSymbol = paramSym;  // V2: direct link
    }
}
```

The `param->resolvedSymbol = paramSym` line is critical. In V2, every `ParameterNode` has a direct pointer to its `VariableSymbol`. This eliminates the V1 `scanForParamSymbols` hack where codegen had to search the AST to find parameter-symbol mappings.

**Structs:**

Pass 1 enters the struct as a scope (`pushScope(structSym)`), sets `inTypeScope_ = true`, then visits fields (which become `Field` role variables with ascending `fieldIndex`), methods, and operators. After all members, it restores the previous state.

**Classes:**

Similar to structs but with additional steps:

1. Resolve base class by looking up `baseClassNames[0]` in the current scope. If found, copy inherited `allFields`.
2. Remaining base class names are treated as implemented interfaces.
3. Visit fields (appended to both `fields` and `allFields`).
4. Visit or auto-generate constructor and destructor.
5. Visit copy constructor if present (`node.copyConstructor`): when the `ConstructorDeclaration` has `isCopyConstructor=true`, the constructor symbol gets `isCopyConstructor=true` and is stored on `ClassSymbol::copyConstructor`.
6. Visit move constructor if present (`node.moveConstructor`): when `isMoveConstructor=true`, the constructor symbol gets `isMoveConstructor=true` and is stored on `ClassSymbol::moveConstructor`.
7. Visit methods and operators.
8. Call `buildVtable(classSym)` after all members are registered.

**Enums:**

Enum members are evaluated eagerly with auto-increment:

```cpp
int64_t nextValue = 0;
for (auto& member : node.members) {
    if (member->value is IntegerLiteral) {
        info.intValue = intLit->value;
        nextValue = info.intValue + 1;
    } else if (member->value is StringLiteral) {
        info.stringValue = strLit->value;
    } else {
        info.intValue = nextValue++;
    }
    enumSym->members.push_back(info);
}
```

**Interfaces:**

All interface methods are marked `isAbstract = true`. Each method gets a sequential `vtableIndex` for itable dispatch.

**Lambdas:**

A lambda creates a `BlockScope` labeled `__lambda`, not a `FunctionSymbol` scope. Parameters are created as `VariableSymbol` instances with `role = Parameter` inside this scope. The lambda body is visited within this scope.

**Match Arms:**

Each match arm gets its own `BlockScope`. `IdentifierPattern` nodes create `Local` variables in the arm scope (for binding patterns like `x` in `match val { x => ... }`).

**MoveExpression:**

`visit(MoveExpression&)` must be implemented in Pass 1 to call `setScope()` on the node and then visit the operand. Without this, the inner `IdentifierExpression` has a null `astScopeNode`, causing later passes (identifier resolution, capture analysis) to fail.

**Function Overloads:**

When multiple functions share the same name in a scope, Pass 1 tracks them via a parallel `functionOverloads_` map. Each scope maintains a list of `FunctionSymbol` entries keyed by function name. When a new function is defined with a name that already exists in the current scope, it is added to the overload list rather than producing a redefinition error. Overload resolution is deferred to Pass 3.

**Typedef Declarations:**

`visit(TypedefDeclaration&)` creates a type alias that resolves to the underlying type. The alias name is registered in the type registry via `symbolTable_.registerType()`, so subsequent type resolution in Passes 2 and 3 transparently resolves the alias to its target type.

**Do-While Statements:**

`visit(DoWhileStatement&)` scopes the body like a regular `while` statement -- it pushes a `BlockScope` for the body and visits the condition and body within it.

### Auto-Generated Constructors and Destructors

If a `ClassDeclaration` lacks an explicit constructor:

```cpp
void SymbolTableBuilder::autoGenerateConstructor(ClassDeclaration& node, ...) {
    auto ctorSym = make_shared<ConstructorSymbol>(classSym->getName());
    symbolTable_.defineSymbol(ctorSym);
    classSym->constructor = ctorSym;

    // Create empty body AST node for consistency
    auto body = make_shared<BlockStatementNode>();
    body->astScopeNode = ctorSym;
    node.constructor = make_shared<ConstructorDeclaration>();
    node.constructor->body = body;
    node.constructor->resolvedConstructor = ctorSym;
}
```

The same pattern applies to destructors. This guarantees that `classSym->constructor` and `classSym->destructor` are never null, which means `hasRAII()` always returns true for classes, ensuring all class instances get automatic destructor calls at scope exit.

### Vtable Building

`buildVtable(ClassSymbol*)` runs after all class members are registered:

1. **Inherit base vtable**: Copy the base class's vtable vector.
2. **Slot 0 = destructor**: Root classes insert the destructor at slot 0; derived classes override slot 0 with their own destructor. The destructor always gets `vtableIndex = 0`.
3. **Instance methods**: For each non-static, non-ctor/dtor method:
   - If the base vtable has a slot with the same name: override that slot, set `vtableIndex`.
   - Otherwise: append as a new slot, set `isVirtual = true`.
4. Set `vtableSize = vtable.size()`.

All instance methods are automatically virtual. This differs from C++ where you must explicitly mark methods as `virtual`.

### Import Resolution (Phase 1b)

`resolveAllImports()` runs after all modules have been visited:

**Whole-module import** (`import Module;`):
1. Look up source module in the root scope.
2. Iterate all symbols in the source module.
3. Copy all `Public` symbols into the importing module's scope.

**Selective import** (`import x, y from Module;`):
1. Look up source module in the root scope.
2. For each target name, look up in the source module's scope.
3. Define the symbol in the importing module's scope.
4. Aliased imports are supported but currently define the original symbol.

---

## 5. Pass 2: TypeResolver

**Header:** `include/mingus/sema/TypeResolver.h`
**Source:** `src/mingus/sema/TypeResolver.cpp`

### Purpose

Pass 2 converts all `TypeNode` AST annotations into concrete `TypeSymbol` instances. It sets the `type` field on `VariableSymbol` for explicitly-typed variables, the `returnType` on `FunctionSymbol`, and the `underlyingType` on `EnumSymbol`. It does NOT perform type inference for `var`-declared variables -- that is deferred to Pass 3.

### Core Type Resolution

`resolveTypeNode(TypeNode*)` dispatches based on the TypeNode subclass:

| TypeNode | Resolution Strategy |
|----------|-------------------|
| `PrimitiveTypeNode` | Direct lookup: `symbolTable_.getIntType()`, etc. |
| `NamedTypeNode` | Check type registry first, then scope chain lookup |
| `PointerTypeNode` | Recursive resolve of base type, then `symbolTable_.getPointerType(base)` |
| `ArrayTypeNode` | Recursive resolve of element type, evaluate literal size, then `symbolTable_.getArrayType(elem, size)` |
| `TupleTypeNode` | Resolve all element types, then `symbolTable_.getTupleType(types)` |
| `FunctionTypeNode` | Resolve parameter types (with reference unwrapping) and return type, then `symbolTable_.getFunctionType(params, ret)` |

Named type resolution follows two paths:
1. **Single name**: Check `symbolTable_.resolveType(name)` first (type registry), then fall back to scope resolution via `scope->resolve(name)`.
2. **Qualified name** (e.g., `Module.TypeName`): Resolve the first part as a module, then look up the type within the module's scope.

Unresolvable types become `ErrorTypeSymbol` to prevent cascading errors.

### The Critical ReferenceType Unwrap

When a parameter type annotation is `T&` (e.g., `int& x`), the parser produces a `PointerTypeNode` with `isReference = true`. Pass 2 resolves this to a `ReferenceTypeSymbol`, but then **must unwrap it**:

```cpp
void TypeResolver::resolveParameters(...) {
    for (auto& param : params) {
        auto resolvedType = resolveTypeNode(param->type);
        if (auto* refType = resolvedType->as<ReferenceTypeSymbol>()) {
            varSym->setType(refType->baseType);    // Store BASE type
            varSym->isReference = true;              // Carry ref semantics as flag
        } else {
            varSym->setType(resolvedType);
        }
    }
}
```

This unwrapping is **critical**. Without it, codegen's `mapType()` would receive `ReferenceTypeSymbol(int)` and map it to `ptr` (LLVM pointer type) instead of `i32`. The reference semantic is carried entirely by the `isReference` flag on `VariableSymbol`, not by the type itself.

`resolveParameters()` also propagates `isRvalueReference` from `ParameterNode` to `VariableSymbol`, parallel to the existing `isReference` propagation. This supports move constructor parameters declared as `T&&`.

The same unwrapping applies in two locations:
1. `resolveParameters()` -- for regular function/method parameters
2. `visit(LambdaExpression&)` -- for lambda parameters

And within `FunctionTypeNode` resolution, reference parameters in function type annotations are also unwrapped into `FunctionTypeSymbol::ParameterInfo::isReference`.

### Variable Type Resolution

For variables with explicit type annotations:

```cpp
void TypeResolver::visit(VariableDeclaration& node) {
    if (node.type && node.resolvedVariable) {
        auto resolvedType = resolveTypeNode(node.type);
        node.resolvedVariable->setType(resolvedType);
    }
    // Walk initializer for nested lambdas/var-decl-exprs
    if (node.initializer) node.initializer->accept(*this);
}
```

For `var`-declared variables (`isInferred = true`), the type is left as `nullptr`. Pass 3 infers it from the initializer expression.

### What Pass 2 Does and Does Not Do

**Does:**
- Resolves all explicit type annotations to `TypeSymbol` instances
- Sets `VariableSymbol::type` for explicitly-typed variables and parameters
- Sets `FunctionSymbol::returnType` (defaults to `void` if no annotation)
- Sets `EnumSymbol::underlyingType` (defaults to `int`)
- Unwraps `ReferenceTypeSymbol` on parameters
- Propagates `isRvalueReference` from `ParameterNode` to `VariableSymbol`
- Resolves `CastExpression::targetType` and `NewExpression::type`
- Resolves `SizeOfExpression::targetType`
- Visits `ClassDeclaration::moveConstructor` if present (resolves move constructor parameter types)

**Does NOT:**
- Infer types for `var`-declared variables (deferred to Pass 3)
- Set `resolvedType` on expression nodes (deferred to Pass 3)
- Evaluate constant expressions for array sizes (only integer literals)
- Resolve identifiers to symbols

---

## 6. Pass 3: TypeChecker

**Header:** `include/mingus/sema/TypeChecker.h`
**Source:** `src/mingus/sema/TypeChecker.cpp`

### Purpose

The most complex pass. Pass 3 performs bottom-up expression type inference, identifier resolution, call resolution, operator overload dispatch, type compatibility checking, and `var` type inference. After Pass 3, every `ExpressionBaseNode` has a non-null `resolvedType`.

### Context Tracking

```cpp
shared_ptr<FunctionSymbol> currentFunction_;   // Current function being checked
TypeSymbolPtr currentReturnType_;              // Expected return type
TypeSymbol* currentClass_ = nullptr;           // Current class/struct context
```

These are saved and restored at each function/class/struct boundary.

### Expression Type Inference (Bottom-Up)

Pass 3 visits children first, then infers the parent node's type from child types.

**Literals:**

| Literal | Resolved Type |
|---------|--------------|
| `IntegerLiteral` | `int` |
| `FloatLiteral` | `double` |
| `BoolLiteral` | `bool` |
| `CharLiteral` | `char` |
| `StringLiteral` | `string` |
| `NullLiteral` | `NullType` |
| `InterpolatedStringExpression` | `string` |

**Identifier Resolution:**

```cpp
void TypeChecker::visit(IdentifierExpression& node) {
    auto sym = scope->resolve(node.name);
    node.resolvedSymbol = sym;
    node.resolvedType = getSymbolType(sym);
}
```

`getSymbolType()` maps symbols to types:
- `VariableSymbol` -> its declared/inferred type
- `FunctionSymbol` -> `buildFunctionType()` result
- `TypeSymbol` -> the type itself

**This Expression:**

`this` resolves to the current class type (looked up via `symbolTable_.resolveType(currentClass_->getName())`). Reports an error if used outside a class context.

**Qualified Names:**

`QualifiedNameExpression` walks a chain of scope lookups. Special handling for enum member access: `Color.Red` resolves the enum symbol, finds the member, and sets `isEnumAccess = true` with the resolved integer or string value.

### Call Resolution

`visit(CallExpression&)` is the most critical resolution in Pass 3. It must handle multiple calling patterns:

1. **Direct function call**: Callee is an `IdentifierExpression` that resolved to a `FunctionSymbol`. Extract `FunctionTypeSymbol` via `buildFunctionType()`.

2. **Method call**: Callee is a `MemberAccessExpression` whose `resolvedSymbol` is a `FunctionSymbol`. Same extraction.

3. **String builtin method call**: Callee is a `MemberAccessExpression` with `isStringBuiltinMethod = true`. Return type is determined by method name (`length` -> `int`, `substring` -> `string`, etc.).

4. **Closure/function variable call**: Callee type is already a `FunctionTypeSymbol` (variable holding a closure).

5. **Struct construction**: Callee type is a `StructSymbol`. Result is the struct type (zero-initialized value).

6. **Constructor call**: Callee type is a `ClassSymbol`. Looks up `classSym->constructor` and uses its function type. Result is the class type.

After resolving the function type, Pass 3:
- Sets `node.resolvedCallee = funcSym` (used by codegen)
- Sets `node.resolvedType = funcType->returnType`
- Validates argument count (exact match for non-variadic, minimum for variadic)
- Sets `node.arguments->isReference[i]` for each argument (from `FunctionTypeSymbol::ParameterInfo::isReference`)
- Type-checks each argument against its parameter type

**The `funcTypeHolder` pattern**: `buildFunctionType()` returns a `shared_ptr<FunctionTypeSymbol>` that must be kept alive for the duration of the call resolution. The local variable `funcTypeHolder` prevents the function type from being destroyed before it is used:

```cpp
shared_ptr<FunctionTypeSymbol> funcTypeHolder;
// ...
funcTypeHolder = fSym->buildFunctionType();
if (funcTypeHolder) funcType = funcTypeHolder.get();
```

### Binary Expression Type Rules

Pass 3 first checks for operator overloads on the left operand's type. If found, it sets `isOperatorOverload = true` and `resolvedOperatorFunction`, and the result type is the operator's return type.

Without an overload:

| Operator | Result Type |
|----------|------------|
| `+` (with string) | `string` (concatenation) |
| `+` (with pointer) | pointer type (pointer arithmetic) |
| `+`, `-`, `*`, `/`, `%` | wider of the two operand types |
| `&`, `\|`, `^`, `<<`, `>>` | wider of the two operand types |
| `==`, `!=`, `<`, `>`, `<=`, `>=` | `bool` |
| `&&`, `\|\|` | `bool` |

Widening follows the rank: `byte(1) < char(2) < int(3) < float(4) < double(5)`. Enum types are unwrapped to their underlying type for arithmetic.

### Unary Expression Type Rules

| Operator | Result Type |
|----------|------------|
| `-` (negate) | same as operand |
| `!` (logical not) | `bool` |
| `~` (bitwise not) | same as operand |
| `*` (dereference) | pointer's base type (error if non-pointer) |
| `&` (address-of) | `PointerTypeSymbol(operandType)` |

### Member Access Resolution

`visit(MemberAccessExpression&)` resolves in priority order:

1. **Auto-dereference**: If the object type is a pointer, unwrap one level.
2. **Enum member access**: If the object type is an enum, look up the member by name.
3. **String builtin methods**: If the object type is `string`, check for `length`, `charAt`, `substring`, `indexOf`, `toInt`, `toDouble`.
4. **Struct/class member**: Cast the type to a `Scope*` and call `resolve(memberName)`.
5. **Static access detection**: If the resolved member is a static function, set `isStaticAccess = true`.

### Assignment Checking

Pass 3 validates:
- **L-value**: Target must be an `IdentifierExpression`, `MemberAccessExpression`, `IndexExpression`, or dereference `UnaryExpression`.
- **Mutability**: If the target symbol is a `VariableSymbol` with `isMutable = false`, report an error.
- **Type compatibility**: The value type must be compatible with the target type.

### Variable Type Inference

For `var x = expr;` declarations:

```cpp
if (node.isInferred && node.initializer) {
    auto initType = node.initializer->resolvedType;
    if (initType is NullTypeSymbol) -> error("cannot infer type from null")
    else if (initType is void) -> error("cannot declare variable with void type")
    else -> varSym->setType(initType);
}
```

For explicitly-typed variables with initializers, Pass 3 checks compatibility between the initializer type and the declared type.

### Tuple Destructuring

When the initializer type is a `TupleTypeSymbol`, Pass 3 assigns each element type to the corresponding destructured variable:

```cpp
for (size_t i = 0; i < node.resolvedVariables.size(); i++) {
    node.resolvedVariables[i]->setType(tupType->elementTypes[i]);
}
```

### Lambda Return Type Inference

Lambda expressions get special handling:

1. Save `currentReturnType_` and set it to `nullptr`.
2. Visit the lambda body:
   - **Block body**: Visit all statements. The first `return` statement sets `currentReturnType_` (inferred from the return value's type).
   - **Expression body**: The expression's type becomes the return type directly.
3. Build a `FunctionTypeSymbol` from the parameter types and inferred return type.
4. Set `node.resolvedType` to this function type.
5. Restore `currentReturnType_`.

### Condition Type Checking

`if` and `while` conditions are checked to ensure they produce a `bool` type:

```cpp
if (!symbolTable_.isCompatible(condition->resolvedType.get(),
                                symbolTable_.getBoolType().get())) {
    errors_.error("if condition must be bool", node.debugInfo);
}
```

### Function Overload Resolution

The TypeChecker maintains overload candidate lists populated by Pass 1's `functionOverloads_` map. When a `CallExpression` callee resolves to an overloaded name, Pass 3 performs overload resolution using a scoring system:

- **Exact type match**: Scores highest (e.g., calling with `int` when parameter is `int`).
- **Compatible type match**: Scores lower (e.g., calling with `int` when parameter is `double`, requiring implicit widening).
- **Parameter count**: Must match exactly -- no implicit default arguments.

The candidate with the highest total score wins. If no candidate matches or if there is an ambiguity, an error is reported.

### Covariant Return Type Checking

When checking method overrides (derived class method overriding a base class method), the TypeChecker validates that the return type of the override is covariant: it must be either the same type as the base method's return type, or a subclass pointer of the base method's return type. For example, if the base method returns `Animal*`, the override may return `Dog*` (where `Dog` extends `Animal`).

### MoveExpression

`visit(MoveExpression&)` visits the operand expression and sets `resolvedType` to the operand's resolved type. The move expression is a pass-through for type purposes -- it only signals move semantics to codegen.

### String Indexing

`visit(IndexExpression&)` recognizes when the object type is `PrimitiveKind::String`. String indexing with an integer subscript returns `charType`, enabling character access via `str[i]`.

### Delete Statement Checking

The target of `delete` must be a `PointerTypeSymbol` or `ClassSymbol`:

```cpp
if (!target->resolvedType->is<PointerTypeSymbol>() &&
    !target->resolvedType->is<ClassSymbol>()) {
    errors_.error("delete requires pointer or class type", ...);
}
```

---

## 7. Pass 4: SemanticValidator

**Header:** `include/mingus/sema/SemanticValidator.h`
**Source:** `src/mingus/sema/SemanticValidator.cpp`

### Purpose

Pass 4 performs five categories of validation that all require the fully-typed AST from Passes 1-3:

- **4a**: Lambda capture resolution (the most critical invariant)
- **4b**: RAII variable tracking (destructor calls at scope exit)
- **4c**: Return completeness checking
- **4d**: Control flow validation (break/continue in loops)
- **4e**: Class/interface implementation checking + match exhaustiveness

### Context Tracking

```cpp
int loopDepth_ = 0;                                    // For break/continue validation
TypeSymbolPtr currentReturnType_;                       // For return completeness
unordered_map<Scope*, ScopeRAIIInfo> raiiInfo_;         // Per-scope RAII tracking
Scope* currentScope_ = nullptr;                         // Current scope for RAII

struct LambdaContext {
    LambdaExpression* lambda;
    set<Symbol*> localSymbols;  // params + declarations owned by this lambda
};
vector<LambdaContext> lambdaStack_;                     // Lambda capture context stack
```

### 4a: Lambda Capture Analysis

This is the most critical part of Pass 4. When a `LambdaExpression` is visited, a new `LambdaContext` is pushed onto `lambdaStack_` with the lambda's parameters registered as local symbols. When any `IdentifierExpression` is visited inside the lambda body, `checkLambdaCapture()` is called.

**The Algorithm:**

```cpp
void SemanticValidator::checkLambdaCapture(IdentifierExpression& node) {
    // Only capture VariableSymbol with role Local or Parameter (not Field)
    if (lambdaStack_.empty()) return;
    auto* varSym = node.resolvedSymbol->as<VariableSymbol>();
    if (!varSym || varSym->role == VariableRole::Field) return;

    // CRITICAL: Walk from innermost lambda outward
    for (int i = lambdaStack_.size() - 1; i >= 0; --i) {
        auto& ctx = lambdaStack_[i];

        // If this lambda owns the variable locally, stop
        if (ctx.localSymbols.count(node.resolvedSymbol.get()) > 0) break;

        // Determine capture mode from capture specification
        CaptureMode mode = ...;  // From [x], [&x], [=], [&]
        bool allowed = ...;

        if (!allowed) {
            // [] capture list blocks capture -- report error for innermost
            break;
        }

        // Add to captured variables (avoid duplicates)
        if (!alreadyCaptured) {
            lambda->capturedVariables.push_back(node.resolvedSymbol);
            lambda->captureModesResolved.push_back(mode);
        }
    }
}
```

**CRITICAL INVARIANT: Full-stack propagation.** The loop walks the ENTIRE `lambdaStack_` from innermost to outermost. If variable `x` is used in an inner lambda but defined outside all lambdas, every lambda in the chain must capture it. Without full-stack propagation, outer lambdas get a null environment pointer, causing a segfault at runtime.

Example:
```
func outer() {
    var x = 42;
    var f = [=]() => {           // Must capture x
        var g = [=]() => {       // Must capture x
            return x;            // Uses x
        };
    };
}
```

When `x` is visited inside `g`, the loop walks: `g` (captures x), then `f` (also captures x), then stops because `x` is a local of `outer` (outside all lambdas).

**Capture Mode Resolution:**

For each lambda in the stack, the capture mode is determined by:
1. **Explicit capture items**: `[x]` (by value) or `[&x]` (by reference) -- checked first by matching the variable name.
2. **Default capture mode**: `[=]` (by copy for all) or `[&]` (by reference for all) -- used as fallback.
3. **Empty capture list** `[]`: No captures allowed. If the variable needs to be captured, an error is reported (but only for the innermost lambda that needs it).

**Self-Capture Detection:**

After visiting a `VariableDeclaration` whose initializer is a lambda, `checkSelfCapture()` checks if the lambda captured the variable being declared:

```
var f = [=](int x) => { return f(x - 1); };
```

If `f` appears in the lambda's captured variables, `lambda->selfCapture = true`. This tells codegen to handle the letrec pattern (the closure must reference itself).

**Escape Analysis:**

In `visit(CallExpression&)`, lambdas passed directly as function arguments are marked `escapes = false`:

```cpp
if (auto* lambda = arg->as<LambdaExpression>()) {
    lambda->escapes = false;
}
```

All other lambdas default to `escapes = true`. Non-escaping lambdas can potentially be stack-allocated rather than heap-allocated.

### 4b: RAII Variable Tracking

When a `VariableDeclaration` is visited, `trackRAIIVariable()` checks if the variable needs destructor cleanup:

```cpp
void SemanticValidator::trackRAIIVariable(VariableSymbol* var) {
    if (!var || !var->getType() || !currentScope_) return;

    auto* cls = var->getType()->as<ClassSymbol>();
    if (!cls || !cls->hasRAII()) return;

    auto& info = raiiInfo_[currentScope_];
    info.destructibles.push_back({var, cls->destructor.get()});
}
```

A variable is tracked only if:
1. Its type is a `ClassSymbol` (not a primitive, struct, or pointer)
2. The class has a destructor (`hasRAII()` -- always true due to auto-generation)
3. Its role is `Local` (not `Field` or `Parameter`)

The `raiiInfo_` map is exported via `getRAIIInfo()` and consumed by the IRGenerator for scope-exit cleanup. Codegen reverses the vector for LIFO destruction order.

**Scope tracking**: `currentScope_` is updated at every scope boundary (module, function, block). The RAII info is keyed by scope pointer, so each scope independently tracks which variables need destruction when it exits.

### 4c: Return Completeness Checking

Three-level reachability classification:

```cpp
enum class Reachability {
    AlwaysReturns,      // All code paths reach a return
    SometimesReturns,   // Some paths return, some don't
    NeverReturns        // No return statement reached
};
```

Classification rules:

| Statement | Classification |
|-----------|---------------|
| `return` | `AlwaysReturns` |
| `if/else` (all branches return) | `AlwaysReturns` |
| `if` (no else) | At best `SometimesReturns` |
| `switch` (all cases + default return) | `AlwaysReturns` |
| `for`/`while` | `NeverReturns` (body may not execute) |
| `break`/`continue`/`delete`/expression | `NeverReturns` |

For blocks, the classification is the first statement that returns `AlwaysReturns` or `SometimesReturns` (short-circuit).

Non-void functions whose body classifies as anything other than `AlwaysReturns` produce a warning: "not all code paths in 'funcName' return a value".

### 4d: Break/Continue Validation

A `loopDepth_` counter is incremented on entering `for`/`while` loops and decremented on exit. `break` and `continue` statements report an error if `loopDepth_ == 0`:

```cpp
void SemanticValidator::visit(BreakStatement& node) {
    if (loopDepth_ == 0) {
        errors_.error("'break' statement outside of loop", node.debugInfo);
    }
}
```

**Labeled Break/Continue:**

Labeled break and continue statements (e.g., `break outer;`, `continue outer;`) are validated against a map of active labeled loops. When a labeled loop (e.g., `outer: for (...)`) is entered, the label is registered in the active label map. When a labeled `break` or `continue` is encountered, the validator checks that the label refers to an enclosing labeled loop in the active map. If the label is not found or does not refer to an enclosing loop, an error is reported. Labels are removed from the map when their loop scope exits.

### 4d.1: MoveExpression and ClassDeclaration Visitors

`visit(MoveExpression&)` visits the operand expression to ensure any nested identifiers are subjected to capture analysis and RAII tracking.

`visit(ClassDeclaration&)` visits the `moveConstructor` node if present, ensuring that move constructor bodies are subjected to the same validation (lambda captures, RAII tracking, return completeness) as regular constructors.

### 4e: Abstract/Interface Implementation Checking

**Abstract method checking** (`checkAbstractImplementation`):

For every non-abstract class with a base class, walks up the entire inheritance chain collecting abstract methods. For each, checks if the concrete class implements it (via scope resolution). Reports an error for each unimplemented abstract method.

**Interface implementation checking** (`checkInterfaceImplementation`):

For every interface in `cls->implementedInterfaces`, checks that the class has a method with the same name for each interface method. Reports an error for missing implementations.

### 4e (continued): Match Exhaustiveness

`checkExhaustiveness(MatchExpression&)` determines if a match expression covers all cases:

1. If any arm has an unguarded `WildcardPattern` (`_`) or an unguarded `IdentifierPattern` (binding pattern) -> exhaustive.
2. Guarded patterns do NOT count as exhaustive (the guard might be false).
3. For enum subjects: collect covered member names from `LiteralPattern` arms, compare against all enum members. Report missing members as a warning.
4. For non-enum subjects without a wildcard -> warning: "match expression may not be exhaustive".

### Access Modifier Checking

Access modifiers (`Public`, `Private`, `Protected`) are stored on symbols via `accessLevel` and set during Pass 1. Import resolution in Pass 1b filters to `Public` symbols only when doing whole-module imports.

---

## 8. Error Reporting

**Header:** `include/mingus/sema/ErrorReporter.h`

The `ErrorReporter` is a shared diagnostic collector used by all four passes:

```cpp
class ErrorReporter {
public:
    void error(const string& message, const shared_ptr<DebugInfo>& loc = nullptr);
    void warning(const string& message, const shared_ptr<DebugInfo>& loc = nullptr);
    void note(const string& message, const shared_ptr<DebugInfo>& loc = nullptr);

    bool hasErrors() const;
    int errorCount() const;
    const vector<Diagnostic>& diagnostics() const;
    void dump(ostream& out = cerr) const;
    void clear();
};
```

Each diagnostic has:
- `level`: `Error`, `Warning`, or `Note`
- `message`: Human-readable description
- `location`: Optional `DebugInfo` (file, line, column)

The `dump()` method prints all diagnostics in the format:
```
error: path/to/file.mingus:42:10: undefined identifier 'x'
warning: path/to/file.mingus:55:1: not all code paths in 'foo' return a value
```

**Error recovery strategy**: Unresolvable types become `ErrorTypeSymbol`, which is compatible with everything (via `isCompatible()`). This prevents a single unresolved type from producing a cascade of secondary type errors throughout the program.

---

## 9. Key Invariants

These are critical behaviors that must be preserved. Violating any of these causes compiler crashes, incorrect code generation, or runtime failures.

### 9.1 ReferenceType Unwrapping in Pass 2

`TypeResolver::resolveParameters()` MUST unwrap `ReferenceTypeSymbol` into base type + `isReference` flag:

```cpp
if (auto* refType = resolvedType->as<ReferenceTypeSymbol>()) {
    varSym->setType(refType->baseType);    // NOT the ReferenceTypeSymbol
    varSym->isReference = true;
}
```

Without this, `mapType(ReferenceType(int))` in codegen returns `ptr` instead of `i32`, causing LLVM verification failures.

### 9.2 Nested Lambda Capture Propagation

`checkLambdaCapture()` MUST walk the **entire** `lambdaStack_` from innermost to outermost, adding the variable as a capture at each level that does not locally own it.

Only checking `lambdaStack_.back()` (the innermost lambda) causes outer lambdas to have null environment pointers, leading to segfaults at runtime.

### 9.3 ParameterNode::resolvedSymbol Set in Pass 1

`createParameterSymbols()` MUST set `param->resolvedSymbol = paramSym`. This is the V2 replacement for V1's `scanForParamSymbols` hack. Without it, codegen cannot map parameter AST nodes to their LLVM allocas.

### 9.4 Auto-Generated Constructors and Destructors

Every class MUST have a constructor and destructor after Pass 1 completes (auto-generated if not explicitly declared). This ensures:
- `classSym->hasRAII()` always returns `true`
- RAII tracking in Pass 4 works for all class-typed locals
- Codegen can always emit vtable slot 0 (destructor) and constructor calls

### 9.5 Vtable Slot 0 = Destructor

`buildVtable()` always places the destructor at vtable index 0. All user method `vtableIndex` values start at 1+. `DeleteStatement` dispatches through vtable slot 0. Changing the vtable layout breaks virtual destructor dispatch.

### 9.6 FunctionSymbol IS the Scope

`FunctionSymbol` extends `SymbolWithScope`. When Pass 1 visits a function, it pushes the `FunctionSymbol` itself as the scope (`symbolTable_.pushScope(funcSym)`). Parameters and body statements are defined within this scope. The function body's `astScopeNode` is set to the function symbol.

### 9.7 ClassSymbol resolve() Walks Inheritance

`ClassSymbol::resolve(name)` overrides `BaseScope::resolve()` to additionally search the base class chain. This is essential for inherited member access -- without it, `derivedObj.baseMethod()` would fail to resolve.

### 9.8 The funcTypeHolder Lifetime in CallExpression

In `TypeChecker::visit(CallExpression&)`, the `funcTypeHolder` local variable keeps the `buildFunctionType()` result alive. Without it, the `FunctionTypeSymbol` is destroyed immediately, and the raw pointer `funcType` becomes dangling. This causes use-after-free crashes.

### 9.9 Self-Capture Detection Order

`checkSelfCapture()` must run AFTER the initializer lambda has been visited (so its captured variables are already populated). In `visit(VariableDeclaration&)`:

```cpp
// 1. Visit initializer first (populates lambda captures)
if (node.initializer) node.initializer->accept(*this);

// 2. Register as local symbol in current lambda context
if (!lambdaStack_.empty() && node.resolvedVariable)
    lambdaStack_.back().localSymbols.insert(node.resolvedVariable.get());

// 3. Check self-capture (relies on captures being populated)
checkSelfCapture(node);
```

### 9.10 Each Pass is Strictly Additive

No pass modifies data written by a prior pass. This is maintained by convention and is essential for reasoning about the pipeline:

```
Pass 1 produces:
  +-- Scope tree (Global -> Module -> ClassSymbol -> FunctionSymbol -> BlockScope)
  +-- Symbol objects for all declarations (astScopeNode, resolvedXxx links)
  +-- Vtable slots and allFields for classes
  +-- Auto-generated ctor/dtor for classes
  +-- Import aliases in module scopes
  +-- ParameterNode::resolvedSymbol links

Pass 2 reads Pass 1 scopes/symbols, produces:
  +-- VariableSymbol::type for explicitly-typed fields and parameters
  +-- FunctionSymbol::returnType and parameter types
  +-- EnumSymbol::underlyingType
  +-- TypeNode::resolvedType at declaration and expression sites

Pass 3 reads Pass 1+2 data, produces:
  +-- ExpressionBaseNode::resolvedType on ALL expressions
  +-- ExpressionBaseNode::resolvedSymbol on identifiers and members
  +-- VariableSymbol::type for var-inferred variables
  +-- CallExpression::resolvedCallee and ArgumentsNode::isReference
  +-- BinaryExpression::isOperatorOverload + resolvedOperatorFunction
  +-- MemberAccessExpression flags (isEnumAccess, isStaticAccess, etc.)
  +-- LambdaExpression::resolvedType (FunctionTypeSymbol)
  +-- TupleDestructuringDeclaration variable types

Pass 4 reads Pass 1+2+3 data, produces:
  +-- raiiInfo_ map (scope -> RAII variables with destructor symbols)
  +-- LambdaExpression::capturedVariables and captureModesResolved
  +-- LambdaExpression::selfCapture and escapes flags
  +-- Diagnostic errors/warnings for all validation checks
```
