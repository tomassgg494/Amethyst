# Amethyst

**Amethyst** is a small compiled systems-flavored language with a syntax halfway between high-level languages (Python, JS) and low-level ones (C, C++). It favors speed and predictability while staying minimally readable.

- **Compiled**, not interpreted.
- Compiler written in **C++17** (no LLVM).
- Emits **x86-64 GAS assembly**, assembled with `as`, linked with the system linker (`ld` via `gcc` driver).
- Static typing: `int` (64-bit), `bool`, `void` (return type only).

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

fn main() -> int {
    var nums: int[5] = [10, 20, 30, 40, 50];

    for i in 0..5 {
        if nums[i] % 2 != 0 {
            continue;
        }
        print(nums[i]);
    }

    var total = 0;  // type inferred from initializer
    for i in 0..5 {
        total = total + nums[i];
    }
    print(total);

    print("done");
    return 0;
}
```

### Declarations

| Form | Meaning |
|------|---------|
| `fn name(a: int, b: bool) -> int { ... }` | Function (max 6 params in registers; more are passed on the stack) |
| `var x: int = expr;` | Local variable (must be initialized) |
| `var x = expr;` | Type inference from initializer |
| `var a: int[10] = [1, 2, ...];` | Fixed-size array (element type `int` or `bool`) |
| `x = expr;` | Assignment |
| `a[i] = expr;` | Array element assignment |
| `print(expr);` | Print `int`, `bool`, or a string literal |

Entry point: `fn main() -> int` (no parameters).

### Types

- `int` — 64-bit signed integer
- `bool` — `true` / `false`
- `int[N]` / `bool[N]` — fixed-size stack arrays (bounds-checked at runtime)
- `string` — literals only, usable only with `print` (not storable in variables)
- `void` — only as a function return type

No implicit conversions: `int` and `bool` never mix.

### Statements

`var` (with optional inference), assignment, array element assignment,
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

### Comments

```amethyst
// line comment
```

## Compiler pipeline

```
.amt → Lexer → Parser → Sema (types, scopes, frame slots)
                   → Codegen (GAS x86-64, System V AMD64 ABI)
                   → as → .o → gcc -no-pie (ld + crt + libc) → executable
```

- Locals live in the stack frame (`-8(%rbp)`, `-16(%rbp)`, …).
- Register args: `rdi rsi rdx rcx r8 r9`; further args on the stack (copied into the frame on entry).
- `print` lowers to `printf("%ld\n", ...)`.

Errors are reported as `file:line:col: error: message`.

Non-void functions must return on all control paths (checked for `return`, blocks, and `if`/`else`; `while` alone does not count as returning).

Array out-of-bounds access is caught at runtime: prints
`Amethyst runtime error: index N out of bounds for array of size M` and exits 1.

## Roadmap (not in v1.1)

Pointers, structs, floats, string values (variables, comparisons), heap
allocation, modules, optimizations, definite-assignment analysis, passing
arrays to functions.

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
