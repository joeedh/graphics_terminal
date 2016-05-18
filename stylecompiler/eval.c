#include "eval.h"
#include "../memarena.h"
#include "../style_bytecode.h"

//get tokens
#include "stylecompiler.gen.h"

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>

#include <float.h>
#include <math.h>

static Sym *_new_sym(MemArena *ma) {
	Sym *ret = memarena_calloc(ma, sizeof(*ret));
	ret->ma = ma;

	return ret;
}

Sym *sym(MemArena *ma, char *name) {
	Sym *ret = _new_sym(ma);
	
	ret->type = ret->realtype = S_ID;
	strncpy_s(ret->value.name, sizeof(ret->value.name), name, strlen(name));

	return ret;
}

Sym *sym_f(MemArena *ma, double f) {
	Sym *ret = _new_sym(ma);


	ret->type = ret->realtype = S_FLOAT;
	ret->value.fval = f;

	return ret;
}

Sym *sym_i(MemArena *ma, int i) {
	Sym *ret = _new_sym(ma);


	ret->type = ret->realtype = S_INT;
	ret->value.fval = i;

	return ret;
}

Sym *sym_copy(Sym *sym) {
	Sym *newsym = _new_sym(sym->ma);
	
	*newsym = *sym;

	if (newsym->a)
		newsym->a = sym_copy(newsym->a);
	if (newsym->b)
		newsym->b = sym_copy(newsym->b);

	return newsym;
}

static double get_numval(Sym *s) {
	if (s->type == S_FLOAT)
		return s->value.fval;
	else if (s->type == S_INT)
		return s->value.ival;

	return -1.0;
}

static double do_op(double a, double b, int op) {
	switch (op) {
	case ADD:
		return a + b;
	case SUB:
		return a - b;
	case MUL:
		return a * b;
	case DIV:
		return b != 0 ? a / b : NAN;
	case LT:
		return a < b ? 1 : 0;
	case GT:
		return a > b ? 1 : 0;
	case GTE:
		return a >= b ? 1 : 0;
	case LTE:
		return a <= b ? 1 : 0;
	case EQ:
		return a == b ? 1 : 0;
	}

	return NAN;
}

Sym *sbinop(Sym *a, Sym *b, int op) {
	Sym *c = _new_sym(a->ma);

	//fold constants
	if ((a->type == S_INT || a->type == S_FLOAT) && (b->type == S_INT || b->type == S_FLOAT)) {
		double d = get_numval(a), e = get_numval(b);
		double f = do_op(d, e, op);

		if (a->type == S_INT && b->type == S_INT) {
			c->type = c->realtype = S_INT;
			c->value.ival = (int)f;
		} else {
			c->type = c->realtype = S_FLOAT;
			c->value.fval = f;
		}

		return c;
	}

	c->realtype = (a->realtype == S_FLOAT || b->realtype == S_FLOAT) ? S_FLOAT : S_INT;
	c->type = S_CHAIN;

	//always copy when inheriting chains
	c->a = sym_copy(a);
	c->b = sym_copy(b);
	c->op = op;

	return c;
}

const char *get_op(int op) {
	switch (op) {
	case ADD:
		return "+";
	case SUB:
		return "-";
	case MUL:
		return "*";
	case DIV:
		return "/";
	case LT:
		return "<";
	case GT:
		return ">";
	case LTE:
		return "<=";
	case GTE:
		return ">=";
	case EQ:
		return "==";
	case ASSIGN:
		return "=";
	default:
		return "(unknown op)";
	}
}
Sym *sfunc1(char *name, int bytecode, Sym *arg) {
	Sym *s = _new_sym(arg->ma);

	s->type = S_FUNC;
	strncpy_s(s->value.name, sizeof(s->value.name), name, strlen(name));

	s->a = sym_copy(arg);
	return s;
}

void sym_debug_print(Sym *sym, int depth) {
	switch (sym->type) {
	case S_FLOAT:
		printf("%f", sym->value.fval);
		break;
	case S_INT:
		printf("%i", sym->value.ival);
		break;
	case S_FUNC:
		printf("%s(", sym->value.name);

		if (sym->a) {
			sym_debug_print(sym->a, depth+1);
		}
		if (sym->b) {
			printf(", ");
			sym_debug_print(sym->b, depth+1);
		}

		printf(")");
		break;
	case S_ID:
		printf("%s", sym->value.name);
		break;
	case S_CHAIN:
		printf("(");

		if (sym->a) {
			sym_debug_print(sym->a, depth+1);
		}

		if (sym->b) {
			printf(" %s ", get_op(sym->op));
			sym_debug_print(sym->b, depth+1);
		}

		printf(")");
		break;
	default:
		printf("(symbolic error)");
	}

	if (depth == 0) {
		printf("\n");
	}
}
