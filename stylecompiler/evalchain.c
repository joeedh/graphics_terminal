#include "compiler.h"
#include "stylecompiler.gen.h"
#include "transform.h"

#include "ast.h"
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#include "../memarena.h"
#include "../strhash.h"
#include "../style_bytecode.h"
#include "eval.h"

typedef struct evalchain_state {
	CompilerState *cstate;
	Sym **out;
	int *totsym;
} evalchain_state;

Sym *evalchain_create_intern(evalchain_state *state, ASTNode *node, int depth, int *ret_break);
Sym *builtin_call(evalchain_state *state, ASTNode *node, ASTNode *builtin, int depth, int *ret_break) {
	CompilerState *cstate = state;
	Sym *s;

	switch (node->children[0]->totchildren) {
	case 1:
		s = sfunc1(node->value.s, builtin->value.i, evalchain_create_intern(state, node->children[0]->children[0], depth+1, ret_break));
		break;
	default:
		cstate->error(cstate, node, "Wrong number of function arguments");
		//we jump, jump away, but to silence warnings. . .
		return NULL;
	}
		
	s->realtype = S_FLOAT;
	s->op = builtin->value.i; //bytecode

	return s;
}

Sym *evalchain_create_intern(evalchain_state *state, ASTNode *node, int depth, int *ret_break) {
	CompilerState *cstate = state->cstate;

	switch (node->type) {
	case StatementList: {
		int i;

		for (i=0; i<node->totchildren; i++) {
			evalchain_create_intern(state, node->children[i], depth+1, ret_break);

			if (ret_break && *ret_break)
				break;
		}
		break;
	}
	case ReturnNode: {
		Sym *s;

		if (ret_break)
			*ret_break = 1;

		if (node->totchildren == 0) {
			s = sym(cstate->ma_r, "null");
		} else {
			s = evalchain_create_intern(state, node->children[0], depth+1, ret_break);
		}

		strhash_set(cstate->scope, "$__return", s);
		return s;
	}
	case FunctionNode: {
		char *key = memarena_malloc(cstate->ma_r, 256);
		key[0] = 0;

		gen_funckey(cstate, node, key, 256);

		//create function
		strhash_set(cstate->funcs, key, (intptr_t)node);
		break;
	}
	case BinOpNode: {
		Sym *a = evalchain_create_intern(state, node->children[0], depth+1, ret_break);
		Sym *b = evalchain_create_intern(state, node->children[1], depth+1, ret_break);

		if (!a || !b) {
			cstate->error(cstate, node, "Internal error");
			//jump, jump away. . .
		}

		return sbinop(a, b, node->value.i);
	}
	case IdentNode: {
		Sym *s = strhash_get(cstate->scope, node->value.s);
		if (!s) {
			cstate->error(cstate, node, "Unknown variable %s", node->value.s);
		}

		return s;
	}
	case VarDeclNode: {
		char *name = node->value.s;
		int type;

		if (strhash_has(cstate->globals, name, NULL) && cstate->scopestack_len == 0) {
			cstate->error(cstate, node, "Can't override built-in names at global level");
		}

		switch (node->children[0]->value.i) {
		case T_FLOAT:
			type = S_FLOAT;
			break;
		case T_INT:
			type = S_INT;
			break;
		default: {
			//check stored string name of type
			char *str = node->children[0]->children[0]->value.s;
			if (!strcmp(str, "float")) {
				type = S_FLOAT;
			} else if (!strcmp(str, "int")) {
				type = S_INT;
			} else {
				cstate->error(cstate, node, "Bad type");
			}
			break;
		}
		}

		Sym *s = sym(cstate->ma_r, name);
		s->realtype = type;

		if (node->totchildren < 2 || node->children[1]->type == EmptyNode) {
			s->type = s->realtype;
		} else {
			int realtype = s->realtype;

			s = sym_copy(evalchain_create_intern(state, node->children[1], depth+1, ret_break));
			s->realtype = realtype;
		}

		strhash_set(cstate->scope, name, (intptr_t)s);

		return s;
	}
	case FloatLitNode: {
		return sym_f(cstate->ma_r, node->value.f);
	}
	case IntLitNode: {
		return sym_f(cstate->ma_r, node->value.i);
	}
	case AssignNode: {
		Sym *b = evalchain_create_intern(state, node->children[1], depth+1, ret_break);

		if (node->children[0]->type != IdentNode) {
			cstate->error(cstate, node->children[0], "Bad assignment");
			//->error jumps away. . .   far away. . .
		}

		char *name = node->children[0]->value.s;
		Sym *old = strhash_get(cstate->scope, name);

		b->flag |= old->flag;
		strhash_set(cstate->scope, name, (intptr_t)b);

		Sym *s = sym(cstate->ma_r, name);
		s = sbinop(s, b, node->value.i);
		s->flag |= old->flag;

		return b;
	}

	case FuncCallNode: {
		char key[256];
		Sym *s;

		key[0] = 0;
		gen_funckey(cstate, node, key, sizeof(key));
		ASTNode *builtin = strhash_get(cstate->builtin_funcs, key);
		ASTNode *func;

		//implementme: handle user defined functions.  as. . .macros?  hrm.
		if (builtin) {
			s = builtin_call(state, node, builtin, depth+1, ret_break);
		} else if ((func = strhash_get(cstate->funcs, key))) {
			ASTNode *farglist = func->children[1];
			ASTNode *carglist = node->children[0];
			Sym *args[64], *s;
			char *names[64];
			int i=0;

			if (farglist->totchildren > sizeof(args)/sizeof(*args)) {
				cstate->error(cstate, node, "Can't have more than 64 arguments");
				//bye bye. . .
			}

			//build arguments in current scope
			for (i=0; i<farglist->totchildren; i++) {
				args[i] = evalchain_create_intern(state, carglist->children[i], depth+1, ret_break);
			}

			//push function scope
			scope_push(cstate);
			for (i=0; i<farglist->totchildren; i++) {
				char *var = farglist->children[i]->children[1]->value.s;
				
				strhash_set(cstate->scope, var, args[i]);
			}

			ASTNode *body = func->children[2];
			Sym *ret = NULL;
			int rbreak = 0;

			for (i=0; i<body->totchildren; i++) {
				Sym *s = evalchain_create_intern(state, func->children[2], depth+1, &rbreak);

				if (rbreak) {
					ret = strhash_get(cstate->scope, "$__return");
					break;
				}
			}

			//hoist globals back into previous scope
			HashIter iter;
			char *key;
			int totglobal=0;

			strhash_iter(cstate->scope, &iter, &key, (intptr_t*)&s);
			
			for (; !strhash_idone(&iter); strhash_inext(&iter, &key, (intptr_t*)&s)) {
				if (s->flag & AST_GLOBAL) {
					if (totglobal >= sizeof(args)/sizeof(*args)) {
						cstate->error(cstate, node, "Internal compiler error: too many globals\n");
					}

					//reuse args
					names[totglobal] = key;
					args[totglobal++] = s;
				}
			}

			scope_pop(cstate);

			for (i=0; i<totglobal; i++) {
				strhash_set(cstate->scope, names[i], (intptr_t)args[i]);
			}

			return ret;
		} else {
			cstate->error(cstate, node, "Unknown function %s (%s)", node->value.s, key);
		}

		return s;
	}
	default:
		cstate->error(cstate, node, "Unknown node");
	}

	return NULL;
}

void evalchain_create(CompilerState *cstate, Sym *out[32], int *totsym) {
	HashIter iter;
	char *key;
	intptr_t val;

	//reset scope hashtable
	strhash_release(cstate->scope);
	strhash_init(cstate->scope);

	strhash_iter(cstate->globals, &iter, &key, &val);
	for (; !strhash_idone(&iter); strhash_inext(&iter, &key, &val)) {
		Sym *s = sym(cstate->ma_r, key);
		
		s->realtype = S_FLOAT;
		s->flag |= AST_GLOBAL;

		strhash_set(cstate->scope, key, (intptr_t)s);
	}

	evalchain_state state;
	
	state.cstate = cstate;
	state.out = out;
	state.totsym = totsym;

	evalchain_create_intern(&state, cstate->result, 0, NULL);

	int i=0;

	strhash_iter(cstate->globals, &iter, &key, &val);
	for (; !strhash_idone(&iter); strhash_inext(&iter, &key, &val)) {
		Sym *s = strhash_get(cstate->scope, key);
		
		if (s->type == S_ID && !strcmp(s->value.name, key)) {
			continue;
		}

		printf("%s = ", key);
		sym_debug_print(s, 0);

		out[i] = s;

		strhash_set(cstate->global_indices, key, i);
		hashtable_set(cstate->index_globals, (uintptr_t)i, (intptr_t)key);
		i++;
	}

	*totsym = i;
}
