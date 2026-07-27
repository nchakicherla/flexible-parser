#ifndef REPL_H
#define REPL_H

/* repl.h - interactive read/eval/print loop.
 *
 * State persists across entries: variables declared at the prompt stay
 * declared, and functions stay defined.
 */

#include "parser.h"

/* Takes over stdin until the user exits. Returns the process exit code. */
int runRepl(Parser *parser, const char *grammar_file);

#endif // REPL_H
