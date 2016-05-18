#ifndef _COMPILER_H
#define _COMPILER_H

#include <stdio.h>

struct ASTNode;
struct CompilerState;
typedef int (*errorprint_t)(void *arg, const char *fmt, ...);

char *compilestyle(const char *source, int len, int *len_out);
struct ASTNode *cctemplate(struct CompilerState *cstate, int startnode, const char *fmt, ...);

#endif // !_COMPILER_H
