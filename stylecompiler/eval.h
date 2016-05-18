#ifndef _EVAL_H
#define _EVAL_H

struct MemArena;

typedef union SymValue {
	char name[32];
	double fval;
	int ival;
} SymValue;

enum {
	S_ID,
	S_FLOAT,
	S_INT,
	S_CHAIN,
	S_FUNC
};

typedef struct Sym {
	SymValue value;
	int type, realtype;
	
	struct Sym *a, *b;
	int op, flag;
	struct MemArena *ma;
} Sym;

Sym *sym(struct MemArena *ma1, char *name);
Sym *sym_f(struct MemArena *ma1, double f);
Sym *sym_i(struct MemArena *ma1, int i);

Sym *sym_copy(Sym *sym);
Sym *sbinop(Sym *a, Sym *b, int op);
Sym *sfunc1(char *name, int bytecode, Sym *arg);

void sym_debug_print(Sym *sym, int depth);

#endif /* _EVAL_H */
