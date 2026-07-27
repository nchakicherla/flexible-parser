#ifndef INTERP_H
#define INTERP_H

/* interp.h - a tree-walking interpreter over the parsed AST.
 *
 * It switches on the built-in STX_* types, so it understands whatever subset of
 * the grammar uses those names. A grammar that renames or invents rules still
 * parses and dumps fine; it just has no semantics attached here.
 *
 * An Interp holds all program state, so the REPL can create one and reuse it
 * across entries while a batch run creates one and throws it away.
 */

#include "common.h"
#include "mempool.h"
#include "registry.h"
#include "ast_node.h"

typedef struct s_Interp Interp;

/* Allocated from `pool`; freed when the pool is torn down. */
Interp *interpCreate(Registry *reg, MemPool *pool);

typedef enum {
	EXEC_OK = 0,
	EXEC_ERROR,   /* a runtime error was reported */
	EXEC_EXITED,  /* the program called exit(...) */
} ExecResult;

/* Runs a tree. `out_exit` receives the program's exit value, which is set from
 * exit(...) or a top-level return. */
ExecResult interpExec(Interp *in, const SyntaxNode *root, int *out_exit);

/* REPL form: runs a tree and, when it is an expression rather than a
 * statement, prints the resulting value. */
ExecResult interpExecEcho(Interp *in, const SyntaxNode *root, int *out_exit);

/* True for node types the interpreter evaluates to a value. The REPL uses this
 * to decide between running a statement and echoing an expression. */
bool interpIsExpression(const SyntaxNode *node);

/* Convenience wrapper for a one-shot run: creates an Interp, hoists function
 * definitions, executes. Returns 0 on success. */
int runProgram(const SyntaxNode *root, Registry *reg, MemPool *pool, int *out_exit);

#endif // INTERP_H
