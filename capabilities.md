# Flexible Parser Capabilities

## Overview

Flexible Parser is a C99 library and runtime for defining a programming language with an external EBNF-like grammar, tokenizing source code, constructing an abstract syntax tree (AST), and optionally executing that AST.

The processing pipeline is:

```text
grammar file -> runtime token registry -> scanner -> parser/AST -> tree-walking interpreter
```

The grammar notation supports:

- `,` for sequences
- `|` for alternatives
- `{...}` for zero-or-more repetition
- `[...]` for optional elements
- `(...)` for grouping

It uses conventional EBNF precedence: alternatives bind more loosely than sequences.

Originally, the program only loaded a grammar and tokenized a hardcoded source file. It did not invoke the parser or execute programs. The later commits expanded it into a configurable parser, interpreter, and interactive language environment.

## Command-Line Interface

The executable supports:

```text
main.run [options]
  -g <file>              grammar file
  -s <file>              source file
  --ast                  print the syntax tree
  --tokens               print the token stream
  --dump-grammar <file>  write compiled rule trees to a file
  --parse-only           parse without executing
  -i, --repl             start an interactive session
```

For example:

```sh
make ./bin/main.run
./bin/main.run -g ./resources/grammar.txt -s ./resources/demo.tl
./bin/main.run --parse-only --ast -s ./resources/script.tl
./bin/main.run --repl
./bin/main.run --repl --parse-only
```

## Tree-Walking Interpreter

Commit `91dac91` added a tree-walking interpreter and connected the parser to the main program.

The interpreter supports:

- Variable declaration and initialization
- Assignment and compound assignment
- Integer and floating-point values
- Strings, booleans, and `nil`
- Integer and mixed integer/float arithmetic
- String concatenation with `+`
- Arithmetic operator precedence
- Parenthesized expressions
- Unary numeric negation
- Boolean negation
- Comparisons and equality
- Short-circuit `and` and `or`
- `if`, `else if`, and `else`
- `while` loops
- C-style `for` loops
- `break`
- Functions with parameters
- Return values
- Recursive function calls
- Function hoisting, allowing calls before definitions
- Lexical scopes and variable shadowing
- Postfix increment and decrement
- Built-in `print(...)`
- `exit(...)` and top-level return values
- Runtime errors with source-line information

The runtime value representation includes:

- `nil`
- strings
- signed 64-bit integers
- double-precision floating-point numbers
- booleans
- pointers for internal use

## Runtime-Extensible Grammars

Grammar files can extend the language vocabulary without recompiling the C program.

They can declare new keywords and punctuation tokens:

```text
#keyword TK_LET "let";
#keyword TK_LOOP "loop";
#token TK_ASSIGN ":=";
```

They can also introduce new syntax-rule names. Unknown `STX_*` rule names receive runtime syntax IDs and can appear in parsed ASTs.

The bundled `resources/grammar-mini.txt` and `resources/mini.tl` demonstrate a different surface language using the same binary. The alternate language includes:

- `let` instead of built-in type names for declarations
- `:=` for initialization
- `loop` instead of `while`
- Conditions without mandatory parentheses
- A dynamically registered `STX_PROGRAM` top-level rule

Custom syntax rules do not automatically acquire new execution semantics. The interpreter treats dynamically created structural rules as transparent grouping nodes, while executable behavior remains attached to recognized built-in `STX_*` concepts.

## Parser and Scanner Improvements

Commit `91dac91` also substantially reworked the parser internals:

- Replaced the old grammar builder with a single-pass recursive-descent parser
- Adopted conventional EBNF precedence
- Changed source tokenization from a count-and-rescan process to one pass
- Replaced global scanner state with explicit scanner instances
- Added a runtime name registry for tokens and syntax rules
- Added grammar-defined keywords and punctuation
- Added longest-match handling for punctuation
- Consolidated built-in token definitions in `src/token.def`
- Consolidated built-in syntax definitions in `src/syntax.def`
- Generated enums and name tables from those shared definitions
- Added recursion-depth limits to the grammar loader and AST matcher
- Added clearer grammar, scanning, parsing, and runtime diagnostics
- Added file and line information to errors
- Added safe handling for unknown type IDs
- Replaced an error-sentinel convention for anonymous AST nodes with an explicit flag
- Fixed bounds errors, unsigned underflow, unbounded scanning, read-past-end behavior, and a grammar-node switch fallthrough
- Corrected the compiler optimization flag from `-o3` to `-O3`
- Enabled stricter compiler warnings with `-Wextra` and `-Werror`
- Removed unused `array.c` and `array.h` files

## Interactive REPL

Commit `b2d8be7` added an interactive REPL based on the vendored `linenoise` library.

Start it with:

```sh
./bin/main.run --repl
```

REPL capabilities include:

- Parse-only operation that prints every AST without invoking the interpreter

- Variables and functions persist between entries
- Expression results are printed automatically
- Command history is available with the arrow keys
- History persists in `~/.flexible_parser_history`
- Multi-line blocks are supported
- Input continues automatically while brackets are unbalanced
- Brackets inside strings and comments do not affect continuation detection
- A trailing backslash forces continuation
- Shift+Enter forces continuation in terminals supporting the relevant keyboard protocols
- Ctrl-C abandons a partially entered block
- Ctrl-D exits the session
- A custom grammar can be loaded before entering the REPL

Available REPL commands are:

- `:help` or `:h`
- `:quit` or `:q`
- `:ast` to toggle AST printing
- `:tokens` to toggle token printing
- `:rules` to list grammar rules
- `:load <file>` to execute a source file in the current session
- `:reset` to clear interpreter state

## Claude-Generated Recommendations

Commit `e1ae435`, named `add claude recs for improvements`, did not add Claude or Anthropic integration to the application.

It added `todo.txt`, a refactoring plan apparently generated with Claude. There is no Anthropic SDK, Claude API call, model prompt, network request, or AI dependency in the current runtime.

Several recommendations from that document were later implemented, including:

- Removing global scanner state
- Improving error handling and diagnostics
- Invoking the parser from the main program
- Supporting runtime token and syntax registration
- Adding recursion limits
- Cleaning up public APIs
- Adding runnable examples

In this repository, "interpretation" refers to the local C tree-walking interpreter executing the AST. It does not refer to Claude interpreting source code.

## Current Limitations

Some constructs can appear in the grammar but do not yet have interpreter implementations:

- Classes produce a `classes are not supported` runtime error
- Member access is unsupported
- Method calls are unsupported
- Indexing is unsupported
- `this` is unsupported
- `switch` and case execution are unsupported
- Range-based `for` execution is unsupported
- Arrays are not implemented as runtime values
- Source-language type declarations are largely syntactic and are not enforced by the dynamically typed interpreter
- Arbitrary custom grammar rules can parse, but custom executable semantics cannot be registered from a grammar file

The project also currently lacks:

- An automated test suite
- A public library-style installation target
- A formal grammar specification separate from comments and example files
- A main README with setup and architecture documentation

## Bundled Examples

- `resources/script.tl` is the original parser sample.
- `resources/demo.tl` demonstrates interpreter behavior, including arithmetic precedence, strings, booleans, loops, functions, floats, and recursion.
- `resources/grammar-mini.txt` defines an alternate language using runtime-registered tokens and syntax rules.
- `resources/mini.tl` runs against the alternate grammar without requiring a different executable.

## Verification Status

The current revision was verified by:

- Building with `-Wall -Wextra -Wpedantic -Werror -O3`
- Running the full interpreter demo
- Running the alternate mini-language and grammar
- Parsing the original sample in parse-only mode

These checks completed successfully. The repository does not yet include an automated regression-test target, so this verification is based on the bundled integration examples and strict compilation.
