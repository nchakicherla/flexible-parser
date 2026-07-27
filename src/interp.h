#ifndef INTERP_H
#define INTERP_H

/* interp.h - a tree-walking interpreter over the parsed AST.
 *
 * It switches on the built-in STX_* types, so it understands whatever subset of
 * the grammar uses those names. A grammar that renames or invents rules still
 * parses and dumps fine; it just has no semantics attached here.
 */

#include "common.h"
#include "mempool.h"
#include "registry.h"
#include "ast_node.h"

/* Runs `root`. Returns 0 on success, non-zero if a runtime error was reported.
 * The program's own exit value (from `exit(...)` or a top-level `return`) is
 * written to out_exit. */
int runProgram(const SyntaxNode *root, Registry *reg, MemPool *pool, int *out_exit);

#endif // INTERP_H
