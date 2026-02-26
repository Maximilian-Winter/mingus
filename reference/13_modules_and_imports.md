# Modules and Imports

Every Mingus source file declares a module. Modules provide namespacing, import/export of symbols, module-level variables, and static local storage.

## Module Declaration

Every Mingus file begins with a `module` declaration:

```mingus
module MyApp
{
    extern func printf(string fmt, ...) => int;

    func main() => int
    {
        printf("Hello from MyApp!\n");
        return 0;
    }
}
```

The module name determines the entry point mangling: a module named `MyApp` has entry point `MyApp_main`.

## Library Modules

A module without a `main` function is a library — it exports functions, structs, and other types for use by other modules:

```mingus
module MathLib
{
    func add(int a, int b) => int
    {
        return a + b;
    }

    func square(int x) => int
    {
        return x * x;
    }

    func clampInt(int val, int lo, int hi) => int
    {
        if (val < lo) { return lo; }
        if (val > hi) { return hi; }
        return val;
    }
}
```

Library modules can export any combination of functions, structs, classes, and constants.

## Selective Import

Import specific symbols from another module with `import ... from`:

```mingus
module MyApp
{
    extern func printf(string fmt, ...) => int;

    import add, square from MathLib;

    func main() => int
    {
        printf("add(3, 4) = %d\n", add(3, 4));           // 7
        printf("square(5) = %d\n", square(5));             // 25
        printf("3^2 + 4^2 = %d\n", add(square(3), square(4)));  // 25
        return 0;
    }
}
```

Imported symbols become directly available by name — no module qualification needed.

### Compiling with Imports

When a module imports from another, pass both source files to the compiler:

```bash
mingus_v2_tool.exe main.mingus MathLib.mingus --emit main.ll --entry MyApp_main --opt 2
```

## Full Module Import

Import all symbols from a module with `import ModuleName;`:

```mingus
module MyApp
{
    import MathLib;

    func main() => int
    {
        // All MathLib symbols available
        printf("add(3, 4) = %d\n", add(3, 4));
        return 0;
    }
}
```

## Module-Level Variables

Variables declared at module scope are accessible from all functions in the module:

```mingus
module MyApp
{
    extern func printf(string fmt, ...) => int;

    int globalCounter = 0;

    func increment() => void
    {
        globalCounter = globalCounter + 1;
    }

    func getCounter() => int
    {
        return globalCounter;
    }

    func main() => int
    {
        printf("initial: %d\n", globalCounter);   // 0
        increment();
        increment();
        increment();
        printf("after 3 calls: %d\n", getCounter());   // 3

        // Direct access also works
        globalCounter = 42;
        printf("direct: %d\n", globalCounter);   // 42

        return 0;
    }
}
```

### Module-Level Constants

Immutable values declared at module scope:

```mingus
module MyApp
{
    const int MAX_VALUE = 100;
    const double PI = 3.14159;

    func main() => int
    {
        printf("max = %d\n", MAX_VALUE);   // 100
        printf("pi = %f\n", PI);           // 3.141590
        return 0;
    }
}
```

Constants declared at module level are the Mingus equivalent of C `#define` values. They can be accessed from importing modules.

## Static Local Variables

A `static` variable inside a function persists across calls:

```mingus
func nextId() => int
{
    static int id = 0;
    id = id + 1;
    return id;
}

printf("id1: %d\n", nextId());   // 1
printf("id2: %d\n", nextId());   // 2
printf("id3: %d\n", nextId());   // 3
```

Static locals are initialized to their declared value on first call and retain their value between subsequent calls. They are useful for counters, caches, and singleton patterns.

## Module-Level Structs and Classes

Types defined at module scope are exported and available for import:

```mingus
module OpLib
{
    struct Vec2
    {
        double x;
        double y;

        func operator+(Vec2 other) => Vec2
        {
            Vec2 result;
            result.x = this.x + other.x;
            result.y = this.y + other.y;
            return result;
        }

        func operator*(double s) => Vec2
        {
            Vec2 result;
            result.x = this.x * s;
            result.y = this.y * s;
            return result;
        }
    }

    func makeVec2(double x, double y) => Vec2
    {
        Vec2 v;
        v.x = x;
        v.y = y;
        return v;
    }
}
```

## Known Limitations

- No module aliases (cannot rename an imported module)
- No private module-level symbols (all module members are exported)
- No circular module imports
- No conditional imports
- Module-level variables have no synchronization — not thread-safe
- No `namespace` nesting within modules
- The module name must match the expected entry point format (`ModuleName_main`)

## See Also

- [Getting Started](01_getting_started.md) for basic module structure
- [Functions](04_functions.md) for function declarations
- [C FFI](14_c_ffi.md) for extern declarations and linking
