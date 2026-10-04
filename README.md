# Amethyst

**Amethyst** is a small compiled systems-flavored language with a syntax halfway between high-level languages (Python, JS) and low-level ones (C, C++). It favors speed and predictability while staying minimally readable.

- **Compiled**, not interpreted.
- Compiler written in **C++17** (no LLVM).
- Emits **x86-64 GAS assembly**, assembled with `as`, linked with the system linker (`ld` via `gcc` driver).
- Static typing: `int` (64-bit), `bool`, `void` (return type only).

**Website & docs:** https://tomassgg494.github.io/Amethyst-site/
(source: [`tomassgg494/Amethyst-site`](https://github.com/tomassgg494/Amethyst-site))

## Build

```sh
make          # builds ./amethystc
make test     # examples + test suite
make deb      # builds the Dev Kit .deb (see below)
```

Requirements: `g++` (C++17), GNU `as`/`ld` (binutils), `gcc` (link driver).

## Dev Kit (.deb)

Beginners do **not** need to compile the compiler. The Dev Kit ships a ready binary, helper, examples and docs:

```sh
make deb
# → dist/amethyst-devkit_1.0.0_amd64.deb

sudo apt install ./dist/amethyst-devkit_1.0.0_amd64.deb
```

Installed layout:

| Path | What |
|------|------|
| `/usr/bin/amethystc` | compiler |
| `/usr/bin/amethyst-new` | creates a starter `.amt` from a template |
| `/usr/share/amethyst/examples/` | sample programs |
| `/usr/share/amethyst/templates/` | `hello.amt`, `fib.amt` |
| `/usr/share/doc/amethyst/README.md` | language reference |
| `man amethystc` / `man amethyst-new` | manual pages |

First session after install:

```sh
amethyst-new hello
amethystc hello.amt -o hello
./hello        # 42
```

Depends: `gcc`, `binutils` (the compiler invokes `as`/`ld` when building your programs).

## Usage

```sh
./amethystc program.amt -o program   # compile + link
./program                            # run

./amethystc -S program.amt -o program.s   # assembly only
./amethystc --emit-asm program.amt -o p   # keep intermediate .s/.o
```

## Language tour

```amethyst
fn fib(n: int) -> int {
    if n < 2 {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

fn sum(values: int[]) -> int {
    var t = 0;
    for i in 0..len(values) {
        t += values[i];
    }
    return t;
}

fn greet(name: string) -> string {
    if name == "world" {
        return "hello, world";
    }
    return "hello";
}

fn main() -> int {
    var nums: int[5] = [10, 20, 30, 40, 50];

    for i in 0..len(nums) {
        if nums[i] % 2 != 0 {
            continue;
        }
        print(nums[i]);
    }

    var total = 0;  // type inferred from initializer
    for i in 0..len(nums) {
        total += nums[i];
    }
    print(total);

    print(sum(nums));  // same total, through an array parameter
    print(greet("world"));

    print("done");
    return 0;
}
```

### Declarations

| Form | Meaning |
|------|---------|
| `fn name(a: int, b: bool) -> int { ... }` | Function (up to 6 register slots — an array parameter uses 2; the rest are passed on the stack) |
| `var x: int = expr;` | Local variable, initialized at the declaration |
| `var x: int;` | Local variable left unassigned until the first `x = expr` |
| `var x = expr;` | Type inference from initializer (an initializer is required when the type is omitted) |
| `var a: int[10] = [1, 2, ...];` | Fixed-size array (element type `int` or `bool`) |
| `fn f(a: int[])` | Array parameter: `int[]` / `bool[]`, passed as pointer + length |
| `fn f(a: string)` | String parameter: pointer to NUL-terminated text |
| `var s: string = "hi";` | String variable (no concatenation, no indexing) |
| `var x: float = 1.5;` | Floating-point variable (64-bit binary64) |
| `x = expr;` | Assignment |
| `x += expr;` | Compound assignment: `+=` `-=` `*=` `/=` `%=` (also on `a[i]`) |
| `a[i] = expr;` | Array element assignment |
| `print(expr);` | Print `int`, `bool`, `float` or `string` |

Entry point: `fn main() -> int` (no parameters).

### Builtins

| Form | Meaning |
|------|---------|
| `len(a)` | number of elements of an array, or byte length of a string; result is a plain `int`, so `for i in 0..len(a)` works |
| `float(n)` | `int` → `float` (widening) |
| `int(x)` | `float` → `int`, truncating toward zero |

### Types

- `int` — 64-bit signed integer
- `bool` — `true` / `false`
- `float` — 64-bit IEEE 754 binary64; literals are `1.5`, `0.0`, `2e-3`
  (`0..10` is still a range, not a float). Division by zero yields `inf` /
  `NaN` as IEEE-754 specifies — it does not trap — and every comparison with
  `NaN` is false except `!=`, which is true.
- `int[N]` / `bool[N]` — fixed-size stack arrays (bounds-checked at runtime)
- `int[]` / `bool[]` — array parameter type: pointer + length; it aliases the
  caller's array, so the callee can write through it, and `len()` works on it
- `string` — NUL-terminated text in `.rodata`; store it in variables, assign,
  compare with `==` / `!=` (compares contents, not pointers), pass it to and
  return it from functions. There is no concatenation, indexing or ordering.
- `void` — only as a function return type

No implicit conversions: `int`, `bool`, `float` and `string` never mix on
their own — write `float(n)` or `int(x)` to move between the two numeric
types. `%` is `int`-only.

### Statements

`var` (with optional inference), assignment, compound assignment
(`+=` `-=` `*=` `/=` `%=`), array element assignment,
`if` / `else if` / `else`, `while`, `for i in a..b` (half-open range),
`break`, `continue`, `return`, `print`, nested `{ }` blocks, expression
statements.

Conditions of `if` / `while` must be `bool` (no truthiness on integers).
`break` / `continue` only inside a loop.

### Expressions (precedence, low → high)

1. `||`
2. `&&` (short-circuit)
3. `==` `!=`
4. `<` `<=` `>` `>=`
5. `+` `-`
6. `*` `/` `%`
7. unary `-` `!`
8. literals, `ident`, `a[i]`, `call(...)`, `[...]`, `( ... )`

`==` / `!=` work on `int`, `bool`, `float` and `string`; `<` `<=` `>` `>=`
and `+` `-` `*` `/` work on two `int`s or two `float`s (never mixed).

### Comments

```amethyst
// line comment
```

## Compiler pipeline

```
.amt → Lexer → Parser → Sema (types, scopes, definite assignment, frame slots)
                   → Codegen (GAS x86-64, System V AMD64 ABI)
                   → as → .o → gcc -no-pie (ld + crt + libc) → executable
```

- Locals live in the stack frame (`-8(%rbp)`, `-16(%rbp)`, …).
- Register args: `rdi rsi rdx rcx r8 r9` (an array parameter occupies two of
  them) and `xmm0`-`xmm7` for `float`s; further args on the stack (copied into
  the frame on entry).
- `print` lowers to `puts` for strings, `printf("%.15g\n", ...)` for floats and
  `printf("%ld\n", ...)` for `int` / `bool`.

Errors are reported as `file:line:col: error: message`. Warnings
(`unused variable`, `unreachable code`) use the same prefix with `warning:`
and are printed to stderr without stopping the build.

Non-void functions must return on all control paths (checked for `return`, blocks, and `if`/`else`; `while` alone does not count as returning).

Reading a variable before it is *definitely assigned* is an error. A
variable declared with `var x: int;` counts as assigned only after `x = ...`
on **every** path that reaches the read: an `if`/`else` needs both branches to
assign (a branch that always `return`s is not a path), and a loop body does
not count, because it may run zero times — initialize those variables instead.
Array variables and type-inferred declarations always need an initializer.

Array out-of-bounds access is caught at runtime: prints
`Amethyst runtime error: index N out of bounds for array of size M` and exits 1.

Division or modulo by zero prints
`Amethyst runtime error: division by zero` and exits 1.

## Roadmap (not in v1.1)

Pointers, structs, heap allocation, modules, optimizations.

## Project layout

```
src/token.hpp    token kinds
src/lexer.*      source → tokens
src/ast.hpp      AST nodes
src/parser.*     recursive-descent parser
src/sema.*       static analysis (types, symbols, slots)
src/codegen.*    AST → x86-64 assembly
src/main.cpp     driver (pipeline + as/ld)
examples/        sample programs
tests/           error-case sources for the test suite
scripts/         run_tests.sh, bench.sh, package_deb.sh
```
