# README.md

```markdown
# compilerc

A lightweight ahead-of-time (AOT) compiler written in C that translates a custom procedural, dynamically-typed imperative language directly into **x86-64 Intel assembly**, linked via GCC.

Numbers are represented internally as 64-bit IEEE 754 floating-point values using hardware SSE registers (`xmm0`–`xmm7`).

---

## Features

- **Direct Assembly Generation**: Emits standalone `.intel_syntax noprefix` x86-64 assembly.
- **Floating-Point Arithmetic by Default**: All numerical values are treated as double-precision floats (`double`) using SSE instructions (`addsd`, `mulsd`, `ucomisd`, etc.).
- **Rich Operator Set**:
  - Arithmetic: `+`, `-`, `*`, `/`, `%` (modulo via C `fmod`), `^` (exponentiation via C `pow`).
  - Unary negation: `-x`.
  - Postfix factorial operator: `n!`.
  - Relational & Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`.
  - Logical with Short-Circuiting: `&&`, `||`.
- **Control Flow**:
  - `if` / `else` branch blocks.
  - `while` loops with nested loop support.
  - `break` and `continue` control statements.
- **Functions & Subroutines**:
  - `fn` definitions with multi-argument support (up to 8 floating-point parameters passed in `xmm0`–`xmm7`).
  - `return` statements.
  - Recursive and nested function calls.
- **I/O & Built-ins**:
  - `print(...)`: Variadic output supporting strings and expressions formatted via `%g`.
  - `scan(var)`: Interactive input via `scanf("%lf")`.
  - `time()`: High-resolution cycle count benchmark primitive via the x86 `rdtsc` instruction.
- **Developer-Friendly Diagnostics**:
  - Rust/Clang-inspired diagnostic reporting with source snippets, colored ANSI error banners, and token-range carets (`^~~~^`).
  - Flags for inspecting intermediate compiler stages (Lexer tokens and AST graphs).

---

## Language Overview & Syntax

### Variables & Expressions
Variables are dynamically declared on their first assignment and scoped to their respective stack frame:

```c
// Variable assignment
a = 10;
b = 2.5;
c = (a + b) * 2;

// Exponentiation and factorial
pow_val = 2 ^ 8;
fact_val = 5!; // 120

print("Result: ", c, " Power: ", pow_val, " Fact: ", fact_val);
```

### Control Flow
Conditional statements and loops require braces `{ ... }`:

```c
counter = 0;

while (counter < 10) {
    if (counter == 5) {
        counter = counter + 1;
        continue;
    }
    
    print("Counter: ", counter);
    counter = counter + 1;
}
```

### Functions
Functions are declared using the `fn` keyword:

```c
fn fibonacci(n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

start = time();
res = fibonacci(15);
elapsed = time() - start;

print("Fibonacci(15): ", res);
print("CPU cycles elapsed: ", elapsed);
```

### Input & Output
```c
print("Enter a number:");
scan(user_val);

if (user_val % 2 == 0) {
    print("Even number!");
} else {
    print("Odd number!");
}
```

---

## Architecture Pipeline

```text
Source Code (.txt / .src)
       │
       ▼
 [1] Lexer (lexer.c) ───────────► Generates flat Token stream
       │
       ▼
 [2] Parser (parser.c) ─────────► Builds Abstract Syntax Tree (AST)
       │
       ▼
 [3] Code Generator (gencode.c) ─► Emits x86-64 Intel Assembly (.s)
       │
       ▼
 [4] GCC Driver (main.c) ───────► Assembles & links with libc (-lm)
       │
       ▼
 Executable Binary
```

1. **Lexer**: Scans characters, handles single-line (`//`) and block (`/* ... */`) comments, string literals, identifiers, keywords, and floating-point/integer constants.
2. **Parser**: A recursive-descent parser enforcing operator precedence from logical operators down to primary terms and postfix operators.
3. **Symbol Table (`symtable.c`)**: Computes local stack offsets relative to `rbp` (e.g., `[rbp - 8]`, `[rbp - 16]`). Detects reads to unassigned variables prior to execution.
4. **Code Generator (`gencode.c`)**: Manages the x86-64 calling convention, stack frame alignment, label generation for control structures, and standard C library interop (`printf`, `scanf`, `pow`, `fmod`).

---

## Getting Started

### Prerequisites
- **OS**: Linux (x86-64 architecture).
- **Toolchain**: `gcc`, `make`.

### Building the Compiler

Clone the repository and run `make`:

```bash
git clone https://github.com/your-username/compilerc.git
cd compilerc
make
```

The resulting binary will be placed at the project root: `./compilerc`.

---

## Usage & CLI Options

```bash
./compilerc <source_file> [options]
```

### Options

| Flag | Long Option | Description |
|---|---|---|
| `-o <file>` | `--output <file>` | Specify output binary file name (default: source file name without extension). |
| `-s` | `--save-asm` | Keep generated `.s` assembly file after linking. |
| `-pl` | `--print-lexer` | Print token stream produced by the lexer to `stdout`. |
| `-pa` | `--print-ast` | Print visual ASCII representation of the AST. |
| `-h` | `--help` | Display usage instructions and exit. |

### Examples

**Compile and run a program:**
```bash
./compilerc examples/test.src -o test_bin
./test_bin
```

**Keep the generated assembly for inspection:**
```bash
./compilerc examples/test.src -s -o test_bin
cat test_bin.s
```

**Debug parsing using the visual AST dump:**
```bash
./compilerc examples/test.src -pa
```

*Example AST output:*
```text
AST TREE

Statement 1
[ASSIGN] '='
  │ L: [IDENTIFIER] 'x'
  │ R: [OP] '+'
  │   │ L: [NUMBER] '10'
  │   │ R: [NUMBER] '5'
```

---

## Project Structure

```text
.
├── include/
│   ├── ast.h         # AST node types and tree structures
│   ├── gencode.h     # Assembly emitter interface & loop contexts
│   ├── lexer.h       # Lexer definitions
│   ├── parser.h      # Recursive descent parser functions
│   ├── symtable.h    # Variable tracking and stack offset manager
│   └── token.h       # Token enumeration and token structures
├── src/
│   ├── ast.c         # AST construction, printing, and cleanup
│   ├── gencode.c     # x86-64 Intel code generator
│   ├── lexer.c       # Tokenizer implementation
│   ├── main.c        # CLI driver, pipeline coordinator, and diagnostics
│   ├── parser.c      # Grammar parsing & expression precedence
│   ├── symtable.c    # Stack offset calculation
│   └── token.c       # Token string helpers
├── makefile          # Build configuration
└── README.md
```

