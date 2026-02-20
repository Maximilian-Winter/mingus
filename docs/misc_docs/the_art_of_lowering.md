# The Art of Lowering

### A practical guide to transforming high-level language constructs into machine operations

*For language designers, compiler hobbyists, and anyone who ever built a parser and then stared at an AST wondering "now what?"*

---

## The Problem

You've built a parser. You have an AST. You understand that *somehow* this tree of nodes needs to become executable code — whether that's bytecode for a VM, LLVM IR, or raw machine instructions.

Some things are obvious. An integer literal `42` becomes... the number 42. `a + b` becomes an add instruction. Great.

Then you try to lower a class with inheritance, a pattern matching expression, or a closure, and suddenly nothing makes sense. The construct feels monolithic. You can't see the seams. You don't know where to start.

This guide teaches you the thinking process — not specific algorithms, but how to look at any high-level construct and systematically decompose it into operations your target can execute.


## The Core Insight

Every high-level language feature is a **bundle of simpler operations pretending to be one thing**.

The syntax of a language is designed to hide mechanical detail from the programmer. That's its job. But the compiler writer has to *reverse* that hiding — to unbundle each construct into its constituent operations.

**Lowering is unbundling.**

When lowering feels easy, it's because the construct hides very little. When it feels impossible, it's because the construct is hiding more operations than you've identified.

That's it. That's the whole secret. The rest of this guide is learning to see what's hidden.


## The Three-Layer Model

When working through any lowering problem, think in three layers:

**Layer 1 — Language Constructs.** What the programmer writes. This is the world of classes, match expressions, for loops, closures, operator overloading, destructors.

**Layer 2 — Abstract Operations.** What the construct *means* in terms of generic operations that any machine could perform: "reserve memory," "compare two values," "jump to an address," "call a function," "copy bytes." This is the crucial middle layer where unbundling happens.

**Layer 3 — Target Operations.** What your specific target provides. For LLVM IR: `alloca`, `load`, `store`, `getelementptr`, `icmp`, `br`, `call`, `ret`, `phi`. For a bytecode VM: `PUSH`, `POP`, `LOAD`, `STORE`, `JUMP`, `JUMP_IF`, `CALL`, `RETURN`.

The mistake that makes lowering feel impossible is trying to jump from Layer 1 directly to Layer 3. Always go through Layer 2 first. Write out, in plain language, every operation the machine needs to perform. The Layer 3 mapping usually becomes obvious once Layer 2 is clear.


## The Process

For any construct you need to lower:

**Step 1: Write the source code.** Start with a concrete example in your language. Not the abstract grammar rule — a real, specific piece of code.

**Step 2: Ask "What does the machine actually do here?"** Not what the programmer *intends*. Not what the feature *means conceptually*. What physically happens — what memory moves, what values get compared, what addresses get computed, what functions get called.

**Step 3: Write out every operation in plain language.** Number them. Be explicit about every step, especially the ones that feel "obvious" — those are usually the hidden operations that make lowering hard.

**Step 4: Map each operation to your target.** Each plain-language operation from Step 3 maps to one or a few target instructions. This step is usually mechanical once Step 3 is thorough.

**Step 5: Verify by mentally executing.** Walk through your lowered code with concrete values. Does it produce the right result? Did you miss a step?

Let's apply this process to progressively more complex constructs.


## Difficulty Tier 1: Direct Mappings

These constructs map almost one-to-one to target operations. They hide nothing.

### Integer arithmetic: `a + b`

```
Source:    a + b    (where a and b are integers)

Step 2:   What does the machine do?
          Load the value of a.
          Load the value of b.
          Add them.

Step 3:   1. Read a from its memory location
          2. Read b from its memory location
          3. Perform integer addition

Step 4:   (LLVM)
          %a = load i32, i32* %a.addr
          %b = load i32, i32* %b.addr
          %result = add i32 %a, %b

          (Bytecode VM)
          LOAD a
          LOAD b
          ADD
```

Trivial. The syntax hides nothing — `+` means add, and that's exactly what the machine does.

### Variable declaration: `int x = 42;`

```
Source:    int x = 42;

Step 2:   What does the machine do?
          Reserve space for an integer.
          Put 42 in that space.

Step 3:   1. Allocate stack space for one integer
          2. Store the value 42 into that space

Step 4:   (LLVM)
          %x = alloca i32
          store i32 42, i32* %x

          (Bytecode VM)
          PUSH 42
          STORE_LOCAL 0    ; slot 0 = x
```

Still almost 1:1. The "declaration" dissolves into allocation + initialization.


## Difficulty Tier 2: Control Flow Rewrites

These constructs hide a moderate amount of structure. The key insight is that all control flow is ultimately conditional and unconditional jumps. Everything else — `if`, `while`, `for`, `switch` — is syntactic sugar over jump patterns.

### If-else: `if (cond) { A } else { B }`

```
Source:    if (x > 0) { y = 1; } else { y = 2; }

Step 2:   What does the machine do?
          Evaluate the condition.
          If true, jump to the "then" code.
          If false, jump to the "else" code.
          After either executes, continue at the same place.

Step 3:   1. Compare x to 0
          2. If x > 0, go to step 3a. Otherwise, go to step 3b.
          3a. Store 1 into y. Go to step 4.
          3b. Store 2 into y. Go to step 4.
          4. Continue with next statement.

Step 4:   (LLVM)
          %cmp = icmp sgt i32 %x, 0
          br i1 %cmp, label %then, label %else

          then:
            store i32 1, i32* %y
            br label %merge

          else:
            store i32 2, i32* %y
            br label %merge

          merge:
            ; continue...

          (Bytecode VM)
          LOAD x
          PUSH 0
          COMPARE_GT
          JUMP_IF_FALSE else_label
          PUSH 1
          STORE y
          JUMP merge_label
          else_label:
          PUSH 2
          STORE y
          merge_label:
          ...
```

The `if` keyword disappears. It was just a conditional jump. The `else` keyword disappears — it was just the target of the "false" branch. The curly braces disappear — they were just labels.

### While loop: `while (cond) { body }`

```
Source:    while (i < 10) { sum += i; i++; }

Step 2:   What does the machine do?
          Check the condition.
          If false, skip past the body.
          If true, execute the body, then jump back and check again.

Step 3:   1. Go to step 2.
          2. Compare i to 10. If i >= 10, go to step 5.
          3. Add i to sum.
          4. Increment i. Go to step 2.
          5. Continue with next statement.

Step 4:   (LLVM)
          br label %cond

          cond:
            %i = load i32, i32* %i.addr
            %cmp = icmp slt i32 %i, 10
            br i1 %cmp, label %body, label %exit

          body:
            ; sum += i
            ; i++
            br label %cond

          exit:
            ; continue...
```

A loop is a conditional jump backwards. That's all it ever was.

### For loop: `for (init; cond; iter) { body }`

```
Source:    for (int i = 0; i < 10; i++) { doWork(i); }

Step 2:   What does the machine do?
          Execute the initializer once.
          Check the condition.
          If false, skip to after the loop.
          If true, execute the body.
          Execute the iterator.
          Jump back to the condition check.

Step 3:   1. Allocate i, store 0.
          2. Compare i to 10. If i >= 10, go to step 6.
          3. Call doWork with i.
          4. Increment i.
          5. Go to step 2.
          6. Continue.
```

A for loop is just a while loop with the initializer and iterator made explicit. In fact, many compilers literally rewrite `for` to `while` during lowering. The for loop is syntactic sugar — it contributes zero new operations.

### Break and Continue

```
Step 2:   break  = unconditional jump to the loop's exit label.
          continue = unconditional jump to the loop's iterator/condition label.
```

This is why your code generator needs to track the current loop context — break and continue don't know where to jump without knowing which loop they're inside.


## Difficulty Tier 3: Data Layout

These constructs involve memory layout decisions. The difficulty comes from the fact that the programmer thinks in terms of "objects" and "fields" while the machine thinks in terms of "byte offsets from base addresses."

### Struct field access: `point.x`

```
Source:    struct Point { double x; double y; };
          Point p;
          ... = p.x;

Step 2:   What does the machine do?
          Find where p lives in memory (its base address).
          Add the byte offset of field x within Point.
          Read the value at that address.

Step 3:   1. Get the base address of p.
          2. Field x is at offset 0 (it's the first field, size 0 bytes before it).
          3. Load 8 bytes (a double) from base + 0.

Step 4:   (LLVM)
          %addr = getelementptr %Point, %Point* %p, i32 0, i32 0
          %val = load double, double* %addr
```

The dot operator `.` hides a pointer arithmetic operation. The field name `x` hides a numeric byte offset. These are determined at compile time by the type's layout.

### Array indexing: `arr[i]`

```
Source:    int arr[10];
          ... = arr[i];

Step 2:   What does the machine do?
          Find the base address of the array.
          Multiply the index by the element size.
          Add that offset to the base address.
          Read the value at the computed address.

Step 3:   1. Get base address of arr.
          2. Compute offset: i * sizeof(int) = i * 4.
          3. Compute element address: base + offset.
          4. Load 4 bytes (an int) from that address.

Step 4:   (LLVM)
          %addr = getelementptr [10 x i32], [10 x i32]* %arr, i32 0, i32 %i
          %val = load i32, i32* %addr
```

Square brackets `[]` hide multiplication and pointer addition. The LLVM `getelementptr` instruction encapsulates this, but conceptually it's just arithmetic on addresses.

### Heap allocation: `new Point(1.0, 2.0)`

```
Source:    var p = new Point(1.0, 2.0);

Step 2:   What does the machine do?
          Ask the OS for enough bytes to hold a Point.
          Get back a pointer to that memory.
          Initialize the memory with the constructor.

Step 3:   1. Call malloc(sizeof(Point)) → returns byte*.
          2. Cast the byte* to Point*.
          3. Call Point's constructor, passing the pointer and arguments.
          4. The result is the pointer.

Step 4:   (LLVM)
          %raw = call i8* @malloc(i32 16)        ; sizeof(Point) = 16
          %p = bitcast i8* %raw to %Point*
          call void @Point_constructor(%Point* %p, double 1.0, double 2.0)
```

`new` hides three operations: allocation, type casting, and construction. This is why it felt like "one thing" but was hard to lower — it's three things wearing one keyword.


## Difficulty Tier 4: Hidden Indirection

This is where lowering starts to feel genuinely hard. These constructs hide *indirection* — the machine doesn't know at compile time exactly what code to execute or exactly where data lives.

### Method calls: `obj.method(arg)`

```
Source:    obj.doWork(42);    (where obj is of type Worker)

Step 2:   What does the machine do?
          Find the address of obj (this becomes the 'this' pointer).
          Find the address of Worker::doWork (the function to call).
          Call that function, passing obj's address as a hidden first argument.

Step 3:   1. Get address of obj → this pointer.
          2. Resolve Worker::doWork → known function address (static dispatch).
          3. Call the function: doWork(obj_address, 42).

Step 4:   (LLVM)
          %result = call void @Worker_doWork(%Worker* %obj, i32 42)
```

The dot in `obj.method()` hides the fact that the object's address is secretly being passed as the first argument. Every method call is actually a function call with a hidden parameter. This is the single most important insight for lowering OOP — **methods are functions with a hidden `this` argument.**

### Virtual method calls (abstract/override)

```
Source:    Shape* shape = getShape();    ; could be Circle or Rectangle
          shape->area();                 ; which area() to call?

Step 2:   What does the machine do?
          We don't know which area() to call at compile time.
          Each object carries a pointer to its class's vtable (virtual method table).
          Load the vtable pointer from the object.
          Look up area() in the vtable by index.
          Call the function pointer found there.

Step 3:   1. Load the vtable pointer from the object's memory (usually at offset 0).
          2. Index into the vtable to find the function pointer for area().
          3. Call through the function pointer, passing the object as 'this'.

Step 4:   (LLVM)
          ; load vtable pointer from object
          %vtable_ptr = getelementptr %Shape, %Shape* %shape, i32 0, i32 0
          %vtable = load %VTable*, %VTable** %vtable_ptr

          ; load function pointer from vtable (area is at index 0)
          %func_slot = getelementptr %VTable, %VTable* %vtable, i32 0, i32 0
          %func = load double(%Shape*)*, double(%Shape*)** %func_slot

          ; call through function pointer
          %result = call double %func(%Shape* %shape)
```

`shape->area()` looks like a simple method call, but it hides a double indirection — through the vtable pointer and then through the function pointer. The machine is chasing two pointers before it even knows what code to execute.

This is why virtual dispatch is harder to lower than static dispatch. Static dispatch: the function address is known at compile time. Virtual dispatch: the function address is computed at runtime through table lookups.

### Closures: `(x) => { return x + captured_var; }`

```
Source:    func makeAdder(int n) => (int) => int
          {
              return (int x) => { return x + n; };
          }
          var add5 = makeAdder(5);
          add5(10);    // returns 15

Step 2:   What does the machine do?
          When makeAdder is called:
            It needs to return "a function" — but this function needs
            to remember the value of n (which is 5).
            After makeAdder returns, its stack frame is gone.
            So n must be saved somewhere that outlives the stack frame.
          When add5(10) is called:
            It needs to call the lambda's code.
            The lambda's code needs to access the saved value of n.

Step 3:   1. Allocate a "closure struct" that holds: a function pointer + captured variables.
              struct Closure { void* func; int captured_n; };
          2. Generate an anonymous function that takes the closure struct as a hidden arg:
              anon_func(Closure* env, int x) { return x + env->captured_n; }
          3. Create the closure: store func pointer and n's value into the struct.
          4. Return the closure struct.
          5. When calling add5(10): extract func pointer from closure, call it,
             passing the closure itself as the hidden environment argument.

Step 4:   (LLVM, simplified)
          ; The anonymous function
          define i32 @anon_func(%Closure* %env, i32 %x) {
              %n_ptr = getelementptr %Closure, %Closure* %env, i32 0, i32 1
              %n = load i32, i32* %n_ptr
              %result = add i32 %x, %n
              ret i32 %result
          }

          ; Creating the closure in makeAdder
          %closure = call i8* @malloc(i32 12)    ; sizeof(Closure)
          %typed = bitcast i8* %closure to %Closure*
          ; store function pointer
          %fp = getelementptr %Closure, %Closure* %typed, i32 0, i32 0
          store void* @anon_func, void** %fp
          ; store captured n
          %np = getelementptr %Closure, %Closure* %typed, i32 0, i32 1
          store i32 %n, i32* %np

          ; Calling the closure
          %func = load void*, void** %fp_slot
          %result = call i32 %func(%Closure* %typed, i32 10)
```

A lambda that captures variables looks like "just a function" in the source code, but it's actually **a struct containing a function pointer plus data.** The captured variables are copied into the struct when the closure is created, and accessed through the struct when the closure is called.

This is the unbundling: a closure = allocation + function generation + data capture + indirect call with hidden environment parameter. Five operations behind two curly braces and an arrow.


## Difficulty Tier 5: Compound Constructs

These are the hardest because they combine multiple Tier 3 and Tier 4 patterns into a single syntactic feature.

### Pattern matching

```
Source:    match value {
               0        => "zero",
               1..10    => "small",
               var x if x > 100 => "big",
               _        => "other",
           }

Step 2:   What does the machine do?
          Evaluate the subject once.
          Test each pattern in order:
            - Literal: compare for equality.
            - Range: compare for >= low AND <= high.
            - Binding with guard: always bind (assign), then test the guard.
            - Wildcard: always matches.
          For the first match, evaluate the arm's body.
          The whole thing produces a value (the arm's result).

Step 3:   1. Evaluate 'value', store result.
          2. Compare result == 0. If yes, produce "zero", go to step 7.
          3. Compare result >= 1 AND result <= 10. If yes, produce "small", go to step 7.
          4. Bind result to x. Evaluate guard: x > 100.
             If yes, produce "big", go to step 7.
          5. Wildcard: always matches. Produce "other", go to step 7.
          7. Merge: result is whichever arm executed.
```

The match expression is a chain of comparisons and branches — structurally similar to an if/else-if/else chain, but with a richer vocabulary of comparison patterns. Each pattern type (literal, range, binding, wildcard, tuple, guarded) has its own lowering to comparison instructions, but they all produce the same thing: a boolean "did this match?" that controls a conditional branch.

### Constructor + destructor + RAII

```
Source:    {
               var file = File("test.txt", "r");
               var data = file.read(1024);
           }    // file automatically cleaned up

Step 2:   What does the machine do?
          Allocate stack space for a File object.
          Call File's constructor with arguments.
          Call file.read (method call with hidden 'this').
          — Block is ending —
          Call File's destructor for every destructible local, in reverse order.

Step 3:   1. alloca for File struct on the stack.
          2. Call File_constructor(file_address, "test.txt", "r").
          3. Call File_read(file_address, 1024), store result.
          4. [COMPILER INSERTED] Call File_destructor(file_address).
```

The invisible operation here is step 4. The programmer never writes it, but the compiler must insert it at *every exit point* from the scope — not just the end of the block, but also every `return` statement, every `break`, every `continue`, and every exception path. This is what makes RAII tricky: the destructor call is implicit, and the compiler has to find every possible exit and inject cleanup code.

The implementation strategy: maintain a stack of "active destructible locals" per scope in your code generator. When entering a block, push a new scope. When a destructible variable is declared, record it. When the block ends (or a return/break/continue is reached), walk the scope stack and emit destructor calls in reverse declaration order.

### Operator overloading in expressions

```
Source:    var result = a + b * c;
          (where a, b, c are Complex numbers)

Step 2:   What does the machine do?
          This is NOT add and multiply.
          Precedence: b * c first, then add a to the result.
          But * and + are overloaded, so they're method calls.

Step 3:   1. Call Complex_operator_mul(b, c) → temp1.
          2. Call Complex_operator_add(a, temp1) → result.
          3. Store result.
```

Operator overloading is a *type-checking concern*, not a code generation concern. By the time code generation sees this expression, the type checker has already determined that `+` on Complex means "call operator+" and marked the AST node accordingly. The code generator just sees two function calls. The lowering is trivial — the complexity was in the type resolution phase.

This is an important general lesson: **not all difficulty lives in code generation.** Some constructs are hard to type-check but trivial to lower. Others are trivial to type-check but hard to lower. Knowing where the difficulty actually lives saves you from trying to solve it in the wrong phase.


## The Rule of Thumb

**If you can't see how to lower something, the construct is hiding more operations than you've identified.**

Go back to Layer 2. Ask: "What *else* is secretly happening here?" There is almost always a hidden operation in one of these categories:

**Hidden memory operations.** Something is being allocated, copied, or freed that the syntax doesn't show. Examples: `new` allocates + constructs. Closures allocate an environment struct. Passing a struct by value copies it. String concatenation allocates a new buffer.

**Hidden control flow.** An implicit branch or jump that the syntax doesn't show. Examples: Short-circuit `&&` and `||` don't evaluate the right side if the left side determines the result. The ternary operator `?:` branches. Destructors insert cleanup code at scope exit.

**Hidden function calls.** Something that looks like a built-in operation is actually a function call in disguise. Examples: Operator overloading turns `+` into a method call. String interpolation calls a conversion/formatting function. Array bounds checking calls a validation function.

**Hidden indirection.** The target of an operation isn't known at compile time and must be resolved at runtime. Examples: Virtual method dispatch goes through a vtable. Closure calls go through a function pointer. Interface method calls go through a dispatch table.

If your lowering attempt is stuck, one of these four things is happening and you haven't identified it yet. Name the hidden operation, add it to your Step 3 list, and the path forward usually becomes clear.


## The Lowering Ladder

When lowering a complex feature, don't try to go from the highest level to the lowest in one step. Use intermediate representations — each step lowering only one level of abstraction.

```
LEVEL 4:   for (int i = 0; i < 10; i++) { sum += arr[i]; }
              ↓  desugar for → while
LEVEL 3:   int i = 0; while (i < 10) { sum += arr[i]; i++; }
              ↓  desugar while → jumps
LEVEL 2:   int i = 0;
           loop_start:
              if (i >= 10) goto loop_end;
              sum += arr[i];
              i++;
              goto loop_start;
           loop_end:
              ↓  lower operations
LEVEL 1:   alloca i, store 0
           cond: load i, icmp sge 10, br exit
           body: load i, gep arr[i], load, add to sum, store sum
                 load i, add 1, store i, br cond
           exit: ...
```

Each step only changes one thing. You never have to think about "how does a for loop become LLVM IR" — you only need to think about "how does a for loop become a while loop" and separately "how does a while loop become jumps" and separately "how do jumps become basic blocks."

This is especially useful for compound features. A match expression can be lowered in stages:

```
LEVEL 4:   match value { 0 => "zero", 1..10 => "small", _ => "other" }
              ↓  desugar match → if/else chain
LEVEL 3:   if (value == 0) { result = "zero"; }
           else if (value >= 1 && value <= 10) { result = "small"; }
           else { result = "other"; }
              ↓  desugar if/else → jumps
LEVEL 2:   compare value == 0, branch to arm0 or test1
           test1: compare value >= 1 and value <= 10, branch to arm1 or arm2
           arm0: result = "zero", jump merge
           arm1: result = "small", jump merge
           arm2: result = "other", jump merge
           merge: use result
              ↓  lower to IR
LEVEL 1:   (basic blocks with icmp, br, alloca, store, load)
```

Each level is a small, verifiable step. If something breaks, you know exactly which transformation introduced the bug.


## Checklist for New Features

When you design a new language feature and need to lower it, ask these questions in order:

1. **What is the simplest possible example of this feature?** Write concrete code, not the grammar rule.

2. **Can it be desugared to existing features?** If yes, desugar it in the AST and you're done. Pipes (`|>`) desugar to nested function calls. For loops desugar to while loops. Compound assignment (`+=`) desugars to load + operate + store.

3. **What memory does it need?** Does it allocate? On the stack or heap? Does it create hidden data structures (vtables, closure environments, string buffers)?

4. **What hidden control flow does it have?** Does it branch? Short-circuit? Insert cleanup code? Jump to an unknown target?

5. **What hidden function calls does it have?** Does it call constructors, destructors, operator overloads, runtime helpers, or conversion functions?

6. **What does the machine need to know at compile time vs runtime?** Static dispatch vs dynamic dispatch. Known sizes vs runtime-computed sizes. Constant offsets vs computed offsets.

7. **Can I write the lowered version by hand?** If you can write the equivalent C code for what the feature does, you can lower it. Translate that C code to your target's operations.


## A Note on Where Difficulty Lives

Not all complexity in a compiler lives in code generation. For each feature, the difficulty concentrates in different phases:

| Feature | Hard Phase | Easy Phase |
|---------|-----------|------------|
| Type inference | Type checking | Code generation (types are resolved, just emit) |
| Operator overloading | Type checking (resolution) | Code generation (just a function call) |
| Pattern matching | Code generation (branch structure) | Type checking (straightforward) |
| Closures | Code generation (environment capture) | Parsing (simple syntax) |
| Generics/Templates | All phases (the hardest feature to implement in any language) | — |
| RAII/Destructors | Code generation (scope tracking, exit point injection) | Parsing (trivial syntax) |
| Inheritance | Type checking (method resolution order) + codegen (vtable layout) | Parsing |

Knowing where the difficulty lives tells you where to invest your debugging time. If your operator overloading doesn't work, the bug is almost certainly in the type checker, not the code generator.


## Final Thought

Lowering is pattern recognition. Every construct you successfully lower teaches you to see the hidden operations in the next construct. The first time you lower a class, it feels impossible. The second time, you remember: "it's a struct for data + free functions with a hidden this pointer." The third time, you don't even think about it.

Build that vocabulary. Every hidden allocation, every implicit branch, every secret function call — once you've seen it once, you'll recognize it forever. The art of lowering is the art of seeing what the syntax was designed to hide.

---

*Written during the development of Mingus, a statically typed programming language named after Charles Mingus — fierce, precise, uncompromising.*
