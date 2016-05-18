#include "compiler.h"
#include "stylecompiler.gen.h"
#include "transform.h"

#include "../alloc.h"
#include "ast.h"
#include "eval.h"

#include "../memarena.h"
#include "../strhash.h"
#include "../style_bytecode.h"

#include "../floatutil.h"

static char fa(float f) {
	short f16 = f32_to_f16(f);
	char *c = (char*)&f16;

	return c[0];
}

static char fb(float f) {
	short f16 = f32_to_f16(f);
	char *c = (char*)&f16;

	return c[1];
}

char *get_node_type(CompilerState *cstate, ASTNode *node) {
	ASTNode *n = node;

	while (n) {
		switch (n->type) {
		case IdentNode: {
			Sym *var = strhash_get(cstate->scope, n->value.s);
			if (!var) {
				cstate->error(cstate, node, "Unknown variable %s", n->value.s);
			}

			switch (var->realtype) {
			case S_FLOAT:
				return "float";
			case S_INT:
				return "int";
			default:
				cstate->error(cstate, node, "Bad internal symbolic type %d", var->realtype);
			}
			break;
		}
		case IntLitNode:
			return "int";
		case FloatLitNode:
			return "float";
		case ReturnNode: {
			ASTNode *func = n;

			//find function
			while (n && n->type != FunctionNode) {
				n = n->parent;
			}

			if (!n) {
				cstate->error(cstate, n, "return statement outside of a function");
				//up, up, and away. . .
			}

			return n->children[0]->value.s;
		}
		case ArgListNode: {
			int i;
			for (i=0; i<n->totchildren; i++) {
				if (n->children[i]->children[1] == n) {
					return n->children[i]->children[0]->value.s;
				}
			}
			break;
		}

		case VarDeclNode: {
			return n->children[0]->value.s;
		}
		case AssignNode: {
			switch (n->children[0]->type) {
			case IdentNode: {
				return get_node_type(cstate, n->children[0]);
			}
			default:
				cstate->error(cstate, node, "Failed to find type");
				//jumps away. . .
			}

			break;
		}
		}

		n = n->parent;
	}

	//default to float, except for function calls
	return node->type == FuncCallNode ? "void" : "float";

	//cstate->error(cstate, node, "Failed to infer function call return type");
	//return NULL;
}

int gen_funckey_call(CompilerState *cstate, ASTNode *node, char *buf, int buflen) {
	ASTNode *arglist = node->children[0];
	int c=0, i;
	char *ret_type = get_node_type(cstate, node);

	buf[0] = 0;

	c += strncpy_s(buf, buflen, node->value.s, buflen);
	c += strncat_s(buf, buflen, "_", 1);
	c += strncat_s(buf, buflen, ret_type, buflen);

	c += strncat_s(buf, buflen, "_", 1);

	ASTNode *args = node->children[1];

	for (i=0; i<arglist->totchildren; i++) {
		char *type = get_node_type(cstate, arglist->children[i]);

		c += strncat_s(buf, buflen, "_", 1);

		if (type) {
			strncat_s(buf, buflen, type, buflen);
		} else {
			strncat_s(buf, buflen, "?!?!?!", buflen);
		}
	}

	return c;
}

int gen_funckey(CompilerState *cstate, ASTNode *node, char *buf, int buflen) {
	if (node->type == FuncCallNode) {
		return gen_funckey_call(cstate, node, buf, buflen);
	}

	ASTNode *args = node->children[1];
	int c = 0;
	int i;

	buf[0] = 0;

	c += strncpy_s(buf, buflen, node->value.s, buflen);
	c += strncat_s(buf, buflen, "_", 1);
	
	//write type
	c += strncat_s(buf, buflen, node->children[0]->value.s, buflen);
	c += strncat_s(buf, buflen, "_", buflen);

	for (i=0; i<args->totchildren; i++) {
		c += strncat_s(buf, buflen, "_", buflen);
		c += strncat_s(buf, buflen, args->children[i]->children[0]->value.s, buflen);
	}

	return c;
}

int scope_push(struct CompilerState *cstate) {
	if (cstate->scopestack_len+1 >= cstate->scopestack_size) {
		int newsize = (cstate->scopestack_size + 1)*2;
		if (newsize < 8) {
			newsize = 8;
		}

		void *newstack = memarena_malloc(cstate->ma_r, newsize*sizeof(void*));

		if (cstate->scopestack_len) {
			memcpy(newstack, cstate->scopestack, cstate->scopestack_len*sizeof(void*));
		}
		cstate->scopestack = newstack;
	}

	HashTable *newscope = memarena_malloc(cstate->ma_r, sizeof(*newscope));
	strhash_init(newscope);

	HashIter iter;
	char *key;
	intptr_t val;
		
	strhash_iter(cstate->scope, &iter, &key, &val);
	for (; !strhash_idone(&iter); strhash_inext(&iter, &key, &val)) {
		strhash_set(newscope, key, val);
	}

	cstate->scopestack[cstate->scopestack_len++] = cstate->scope;
	cstate->scope = newscope;

	return 0;
}

int scope_pop(struct CompilerState *cstate) {
	strhash_release(cstate->scope);
	cstate->scope = cstate->scopestack[--cstate->scopestack_len];
}

static int add_builtin_var(struct CompilerState *cstate, const char *name, const char *tname, int type) {
	ASTNode *node = astnode_create(cstate->ma_r, -1, TypeRefNode, VI(type), "%s", tname);

	node->flag |= AST_GLOBAL;
	strhash_set(cstate->globals, name, (intptr_t)node);

	return 0;
}

//since builtin funcs use special bytecode, 
//we store that code as the func's value (name) field,
//which is normally a string.

static int builtin_func1(struct CompilerState *cstate, int bytecode, char *type, char *name) {
	ASTNode *func = cctemplate(cstate, FunctionNode, ""
	"%s %s(float arg) {"
	"}", type, name);

	return func;
}

int add_builtin_func1(struct CompilerState *cstate, int bytecode, char *type, char *name) {
	ASTNode *node = builtin_func1(cstate, bytecode, type, name);

	char *key = memarena_malloc(cstate->ma_r, 256);
	key[0] = 0;

	gen_funckey(cstate, node, key, 256);	
	strhash_set(cstate->builtin_funcs, key, (intptr_t) node);

	node->value.i = bytecode;
	return 1;
}

int setup_builtins(struct CompilerState *cstate) {
	add_builtin_var(cstate, "x", "float", T_FLOAT);
	add_builtin_var(cstate, "y", "float", T_FLOAT);
	add_builtin_var(cstate, "u", "float", T_FLOAT);
	add_builtin_var(cstate, "v", "float", T_FLOAT);
	add_builtin_var(cstate, "r", "float", T_FLOAT);
	add_builtin_var(cstate, "g", "float", T_FLOAT);
	add_builtin_var(cstate, "b", "float", T_FLOAT);
	add_builtin_var(cstate, "a", "float", T_FLOAT);

	add_builtin_func1(cstate, SIN_RR, "float", "sin");
	add_builtin_func1(cstate, COS_RR, "float", "cos");
	add_builtin_func1(cstate, ASIN_RR, "float", "asin");
	add_builtin_func1(cstate, ACOS_RR, "float", "acos");
	add_builtin_func1(cstate, TAN_RR, "float", "tan");
	add_builtin_func1(cstate, ATAN_RR, "float", "atan");
	add_builtin_func1(cstate, SQRT_RR, "float", "sqrt");
	add_builtin_func1(cstate, LOG_RR, "float", "log");
	//add_builtin_func2(cstate, POW_RRR, "pow", "pow");
	add_builtin_func1(cstate, LOG2_RR, "float", "log2");
	add_builtin_func1(cstate, CBT_RR, "float", "cbrt");
	add_builtin_func1(cstate, FRC_RR, "float", "fract");
	add_builtin_func1(cstate, MIN_RR, "float", "min");
	add_builtin_func1(cstate, MAX_RR, "float", "max");
	add_builtin_func1(cstate, ABS_RR, "float", "abs");
}

int ntraverse(ASTNode *root, int type, void *arg, ast_traversecb callback, int depth_first) {
	int i, ret=0;

	if (!depth_first && root->type == type) {
		if (callback(arg, root)) {
			return;
		}
		ret = 1;
	}

	for (i=0; i<root->totchildren; i++) {
		ret |= ntraverse(root->children[i], type, arg, callback, depth_first);
	}

	if (depth_first && root->type == type) {
		callback(arg, root);
		ret = 1;
	}

	return ret;
}

struct Sym;
void evalchain_create(CompilerState *cstate, struct Sym *out[32], int *totsym);

int transform_ast(struct CompilerState *cstate) {
	struct Sym *out[32];
	int totsym;

	return 0;
}

typedef struct writer {
	char *buf;
	int cur, size;
	int regmap[128];
	CompilerState *cstate;
} writer;

int get_reg(writer *w) {
	int i=0;

	//XXX: need to come up with proper range to check here. . .
	for (i=15; i<110; i++) {
		if (!w->regmap[i]) {
			w->regmap[i] = 1;
			return i;
		}
	}

	w->cstate->error(w->cstate, NULL, "eek2!");
	return -1;
}

int free_reg(writer *w, int r) {
	w->regmap[r] = 0;

	return 0;
}

int get_global_reg(CompilerState *cstate, char *r) {
	if (!strcmp(r, "r"))
		return OUTR;
	if (!strcmp(r, "g"))
		return OUTG;
	if (!strcmp(r, "b"))
		return OUTB;
	if (!strcmp(r, "c"))
		return OUTA;
	if (!strcmp(r, "x"))
		return INX;
	if (!strcmp(r, "y"))
		return INY;
	if (!strcmp(r, "u"))
		return INU;
	if (!strcmp(r, "v"))
		return INV;

	cstate->error(cstate, NULL, "Bad register %s in transform.c:get_global_reg!\n", r);
	return -1;
}
unsigned char *emit_stream(Sym *s, writer *w, int accum) {
	if (w->cur + 32 >= w->size) {
		int newsize = (w->cur+16)*2.0;
		char *newbuf = MEM_malloc(newsize);

		if (w->buf) {
			memcpy(newbuf, w->buf, w->cur+1);
			MEM_free(w->buf);
		}

		w->buf = newbuf;
	}

	//if (s->type != S_CHAIN && s->type != S_FUNC) {
	//	w->cstate->error(w->cstate, NULL, "eek!");
	//	return;
	//}

	//register accum is reserved for result of calculations

	if (s->type == S_FUNC) {
		printf("OP::%d\n", s->op);
		if (!s->b) {
			emit_stream(s->a, w, accum);
			
			w->buf[w->cur++] = s->op;
			w->buf[w->cur++] = accum;
			w->buf[w->cur++] = accum;
		} else {
			emit_stream(s->a, w, accum);
			int reg = get_reg(w);
			emit_stream(s->b, w, reg);
			
			w->buf[w->cur++] = s->op;
			w->buf[w->cur++] = accum;
			w->buf[w->cur++] = reg;

			free_reg(w, reg);
		}
	} else if (s->type == S_ID) {
		w->buf[w->cur++] = MOV_RR;

		w->buf[w->cur++] = accum;
		w->buf[w->cur++] = get_global_reg(w->cstate, s->value.name);
	} else if (s->type == S_FLOAT) {
		w->buf[w->cur++] = MOV_RC;
		
		w->buf[w->cur++] = accum;

		w->buf[w->cur++] = fa((float)s->value.fval);
		w->buf[w->cur++] = fb((float)s->value.fval);
	} else if (s->type == S_INT) {
		w->buf[w->cur++] = MOV_RC;
		
		w->buf[w->cur++] = accum;

		w->buf[w->cur++] = fa((float)s->value.ival);
		w->buf[w->cur++] = fb((float)s->value.ival);
	} else if (s->type == S_CHAIN) {
		emit_stream(s->a, w, accum);
		int reg = get_reg(w);

		emit_stream(s->b, w, reg);

		switch (s->op) {
		case ADD:
			w->buf[w->cur++] = ADD_RR;
			w->buf[w->cur++] = accum;
			w->buf[w->cur++] = reg;
			break;
		case SUB:
			w->buf[w->cur++] = SUB_RR;
			w->buf[w->cur++] = accum;
			w->buf[w->cur++] = reg;
			break;
		case MUL:
			w->buf[w->cur++] = MUL_RR;
			w->buf[w->cur++] = accum;
			w->buf[w->cur++] = reg;
			break;
		case DIV:
			w->buf[w->cur++] = DIV_RR;
			w->buf[w->cur++] = accum;
			w->buf[w->cur++] = reg;
			break;
		}

		free_reg(w, reg);
	} else {
		w->cstate->error(w->cstate, NULL, "Bad sym type %d", s->type);
	}
}

unsigned char *emit_bytecode(struct CompilerState *cstate, int *len_out) {
	struct Sym *out[32];
	int totsym=0, i;
	writer w = {0,};

	w.cstate = cstate;
	evalchain_create(cstate, out, &totsym);

	int accum = 5;
	w.regmap[accum] = 1;

	for (i=0; i<totsym; i++) {
		emit_stream(out[i], &w, 5);

		w.buf[w.cur++] = MOV_RR;
		w.buf[w.cur++] = get_global_reg(cstate, (char*)hashtable_get(cstate->index_globals, (uintptr_t)i));
		w.buf[w.cur++] = 5;
	}

	style_disasm(w.buf, w.cur);

	*len_out = w.cur;
	return w.buf;
}
