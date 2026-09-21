# Flexible Parser

Flexible Parser is an experimental C99 parser and language runtime whose syntax is defined by an external, EBNF-like grammar file. One executable can load different grammars at runtime, tokenize source code, build an abstract syntax tree (AST), and execute syntax understood by its tree-walking interpreter.

The repository includes a default language, an interactive REPL, and a small alternate grammar that changes keywords and punctuation without recompiling the program.

## Features

- Runtime-loaded grammars with sequences, alternatives, repetition, optional elements, and grouping
- Grammar-defined keywords, punctuation, and syntax-rule names
- Token and AST inspection from the command line or REPL
- A tree-walking interpreter with variables, functions, recursion, lexical scopes, conditionals, and loops
- Integers, floating-point numbers, strings, booleans, and `nil`
- Arithmetic, comparisons, boolean operators, assignment, and postfix increment/decrement
- An interactive REPL with persistent state, history, and multiline input
- Arena-based memory management and no required runtime dependencies

## Build and run

You need a C99 compiler and `make`. GCC is used by default; override `CC` if needed.

```sh
make ./bin/main.run
./bin/main.run -g ./resources/grammar.txt -s ./resources/demo.tl
```

The executable defaults to `resources/grammar.txt` and `resources/script.tl`, so this also works:

```sh
make run
```

On Linux, the default `make` target runs the program under Valgrind and therefore requires `valgrind`. Building the executable directly or using `make run` does not.

To remove build artifacts:

```sh
make clean
```

## Command-line usage

```text
main.run [options]
  -g <file>              grammar file
  -s <file>              source file
  --ast                  print the syntax tree
  --tokens               print the token stream
  --dump-grammar <file>  write compiled rule trees to a file
  --parse-only           parse without executing
  -i, --repl             start an interactive session
  -h, --help             show help
```

Examples:

```sh
# Execute the full language demo
./bin/main.run -s ./resources/demo.tl

# Inspect tokens and the AST without running the program
./bin/main.run --tokens --ast --parse-only -s ./resources/script.tl

# Use the alternate grammar with the same executable
./bin/main.run -g ./resources/grammar-mini.txt -s ./resources/mini.tl

# Start a REPL using the default grammar
./bin/main.run --repl
```

Unless changed with `--dump-grammar`, the compiled grammar is written to `debug/grammar_tree.log` each time the program starts.

## Language example

The default grammar describes a small, dynamically evaluated language:

```text
scope main {
    fn fib (int n) => int {
        if (n < 2) {
            return n;
        }
        return fib(n - 1) + fib(n - 2);
    }

    Str message = "fib(7) =";
    print(message, fib(7));

    for (int i = 0; i < 3; i++) {
        print("iteration", i);
    }

    return 0;
}
```

The interpreter supports variable declaration and assignment, integer and floating-point arithmetic, string concatenation, comparisons, short-circuit `and`/`or`, `if`/`else`, `while`, C-style `for`, `break`, functions, recursion, and `return`. `print(...)` is built in, and `exit(...)` sets the program's exit status.

Type names are currently syntactic rather than enforced: runtime values are dynamically typed.

## Grammar format

Rules use an EBNF-like notation:

| Form | Meaning |
| --- | --- |
| `a, b` | sequence |
| `a \| b` | alternative |
| `{a}` | zero or more |
| `[a]` | optional |
| `(a)` | grouping |

Alternatives bind more loosely than sequences. Each rule ends with a semicolon:

```text
STX_INIT = STX_VTYPE, STX_VAR, TK_EQUAL, STX_EXPR, TK_SEMICOLON ;
```

The first rule in a grammar is its start rule. A grammar can also register new keywords and punctuation:

```text
#keyword TK_LET "let" ;
#keyword TK_LOOP "loop" ;
#token TK_ASSIGN ":=" ;
```

`resources/grammar-mini.txt` demonstrates these directives by defining a variant language with `let`, `:=`, and `loop`:

```text
{
    let total := 0;
    let n := 5;

    loop n > 0 {
        total += n;
        n -= 1;
    }

    print(total);
}
```

New grammar rules can change how accepted programs are shaped, but they do not automatically add new interpreter behavior. Dynamically registered rules are treated as transparent grouping nodes; executable semantics remain tied to the built-in `STX_*` concepts in `src/syntax.def`.

## REPL

Start an interactive session with:

```sh
./bin/main.run --repl
```

Variables and functions persist between entries. Expressions print their results automatically, and input continues while brackets are unbalanced. A trailing backslash also forces continuation. History is saved to `~/.flexible_parser_history`.

Available commands:

```text
:help            show help
:quit, :q        exit
:ast             toggle AST output
:tokens          toggle token output
:rules           list loaded grammar rules
:load <file>     execute a file in the current session
:reset           clear variables and functions
```

Use Ctrl-C to abandon a partial entry and Ctrl-D to exit.

## How it works

```text
grammar file -> runtime registry -> scanner -> parser/AST -> interpreter
```

- `src/grammar.c` reads grammar directives and compiles rule trees.
- `src/registry.c` manages built-in and runtime-defined token and syntax names.
- `src/scanner.c` tokenizes grammar and source text.
- `src/parser.c` matches the token stream against compiled rules.
- `src/ast.c` and `src/ast_node.c` construct and display syntax trees.
- `src/interp.c` executes supported AST nodes.
- `src/repl.c` provides the interactive environment using the vendored `linenoise` library.

Built-in token and syntax definitions live in `src/token.def` and `src/syntax.def`. These files generate the corresponding enums and name tables through the C preprocessor.

## Current limitations

- Classes, member access, method calls, indexing, `this`, `switch`, range-based `for`, and arrays are parsed by parts of the default grammar but are not implemented by the interpreter.
- Custom grammar files cannot register new execution semantics.
- The project does not yet have an automated test suite or an installation target for use as a standalone library.
- Parsing stops at the first error; there is no error recovery.

## Included examples

- `resources/demo.tl` exercises the implemented interpreter features.
- `resources/script.tl` is a compact default-grammar sample.
- `resources/grammar.txt` defines the default language.
- `resources/grammar-mini.txt` and `resources/mini.tl` demonstrate runtime grammar extension.

## License

This project is available under the [MIT License](LICENSE). Portions of the scanner and the vendored `linenoise` code have their own attribution; see [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES/).
