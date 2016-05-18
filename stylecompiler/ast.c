#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../memarena.h"
#include "ast.h"
static const char *nodetype_to_str(int type) {
	static char retbufs[16][64] = {0,};
	static int retcur = 0;

#ifdef _
#undef _
#endif
#define _(val) case val: return #val;

	switch (type) {
	_(IdentNode)
	_(IntLitNode)
	_(FloatLitNode)
	_(StrLitNode)
	_(FunctionNode)
	_(StatementList)
	_(IfNode)
	_(ElseNode)
	_(BinOpNode)
	_(ArgListNode)
	_(PairNode)
	_(EmptyNode)
	_(FuncCallNode)
	_(ExprListNode)
	_(BracketLiteral)
	_(VarDeclNode)
	_(TypeRefNode)
	_(NullOpNode)
	_(AssignNode)
	_(ReturnNode)

	default: {
		char *buf = (char*) retbufs[retcur];
		retcur = (retcur + 1) % 16;

		sprintf_s(buf, sizeof(*retbufs), "(bad type name: %d)", type);
		buf[sizeof(*retbufs)] = 0;

		return buf;
	}
	}

#undef _
}

void astnode_append(MemArena *ma, ASTNode *node, ASTNode *child) {
	child->parent = node;

	ASTNode **newchilds = memarena_malloc(ma, sizeof(void*)*(node->totchildren+1));
	
	if (node->children != NULL) {
		memcpy(newchilds, node->children, sizeof(void*)*node->totchildren);
	}

	node->children = newchilds;
	node->children[node->totchildren++] = child;
}

ASTNode *astnode_create(MemArena *ma, int line, int type, ValueUnion value, char *fmt, ...) {
	ASTNode *node = memarena_malloc(ma, sizeof(*node));
	va_list va;

	node->line = line;
	node->parent = NULL;
	node->totchildren = 0;
	node->children = NULL;
	node->type = type;
	node->value = value;

	if (!fmt) {
		return node;
	}

	int flen = strlen(fmt), i;
	va_start(va, fmt);

	for (i=0; i<flen; i++) {
		if (fmt[i] == '%' && i < flen-1) {
			i++;

			switch (fmt[i]) {
				case 'n': { //node
					astnode_append(ma, node, va_arg(va, void*));
					break;
				}
				case 'd':  //intlitnode
				case 'i':  //intlitnode
				case 'f':  //floatlitnode
				case 's':  //identnode
				case 'S':  //strlitnode
				{
					ValueUnion val;
					int type;

					switch (fmt[i]) {
						case 'd':
						case 'i':
							type = IntLitNode;
							val.i = va_arg(va, int);
							break;
						case 'f':
							type = FloatLitNode;
							val.f = va_arg(va, double); //remember cdecl never ever passes floats! only doubles!
							break;
						case 'S':
						case 's': { //strlitnode, copies strlit value
							char *str = va_arg(va, char*);
							int len = strlen(str);
							char *nstr = memarena_malloc(ma, len+1);

							type = fmt[i] == 'S' ? StrLitNode : IdentNode;

							memcpy(nstr, str, len);
							nstr[len] = 0;

							val.s = nstr;
						}
					}

					ASTNode *lit = astnode_create(ma, line, type, val, ""); 
				}
			}
		}
	}

	va_end(va);
	return node;
}

static int indent(int wid, void *arg, ast_printfunc printfunc) {
	int i, c=0;

	for (i=0; i<wid; i++) {
		c += printfunc(arg, "  ");
	}

	return c;
}

void astnode_setchild(struct MemArena *ma, ASTNode *node, int index, ASTNode *child) {
	node->children[index] = child;
	child->parent = node;
}

//returns number of characters written
int astnode_print(ASTNode *node, int tablevel, void *arg, ast_printfunc printfunc) {
	int c=0;

	c += indent(tablevel, arg, printfunc);
	c += printfunc(arg, "%s {\n", nodetype_to_str(node->type));

	int i;
	for (i=0; i<node->totchildren; i++) {
		c += astnode_print(node->children[i], tablevel+1, arg, printfunc);
	}

	c += indent(tablevel, arg, printfunc);
	c += printfunc(arg, "}\n");

	return c;
}
