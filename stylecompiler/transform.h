#ifndef _TRANSFORM_H
#define _TRANSFORM_H

#include "ast.h"

struct CompilerState;

int setup_builtins(struct CompilerState *cstate);
int transform_ast(struct CompilerState *cstate);
unsigned char *emit_bytecode(struct CompilerState *cstate, int *len_out);

int ntraverse(struct ASTNode *root, int type, void *arg, ast_traversecb callback, int depth_first);

int scope_pop(struct CompilerState *cstate);
int scope_push(struct CompilerState *cstate);
int gen_funckey(struct CompilerState *cstate, struct ASTNode *node, char *buf, int buflen);
char *get_node_type(struct CompilerState *cstate, struct ASTNode *node);

#endif /* _TRANSFORM_H */
