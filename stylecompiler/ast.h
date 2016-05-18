#ifndef _AST_H
#define _AST_H

struct MemArena;

enum {
	IdentNode,
	IntLitNode,
	FloatLitNode,
	StrLitNode,
	FunctionNode,
	StatementList,
	IfNode,
	ElseNode,
	BinOpNode,
	ArgListNode,
	PairNode,
	ScopeBlock,
	EmptyNode,
	FuncCallNode,
	ExprListNode,
	BracketLiteral,
	VarDeclNode,
	TypeRefNode,
	NullOpNode,
	AssignNode,
	ReturnNode
};

enum {
	T_UNKNOWN,
	T_INT,
	T_FLOAT,
	T_STRUCT,
	T_ARRAY
};

struct ASTNode;
typedef union {
	int i;
	double d;
	float f;
	unsigned int u;
	char *s;
	struct ASTNode *n;
} ValueUnion;

typedef struct ASTNode {
	struct ASTNode *parent;
	int type, line, flag;

	ValueUnion value;

	struct ASTNode **children;
	int totchildren;
} ASTNode;

//ASTNode->flag, is a bitmask
enum {
	AST_GLOBAL = 1
};

static ValueUnion VS(char *s) {
	ValueUnion ret;
	ret.s = s;
	return ret;
}

static ValueUnion VN(ASTNode *n) {
	ValueUnion ret;
	ret.n = n;
	return ret;
}

static ValueUnion VI(int i) {
	ValueUnion ret;
	ret.i = i;
	return ret;
}
static ValueUnion VU(unsigned int u) {
	ValueUnion ret;
	ret.u = u;
	return ret;
}
static ValueUnion VF(float f) {
	ValueUnion ret;
	ret.f = f;
	return ret;
}

void astnode_append(struct MemArena *ma, ASTNode *node, ASTNode *child);

/*
	Fmt controls how child nodes are instantiated.  Each %[code] matches to an argument.

	Valid codes:
		%n : argument is a straight ASTNode*, will be used straight (without copying)
		%i : argument is an integer, creates IntLitNode
		%f : argument is a float, creates FloatLitNode
		%s : argument is an identifiyer, creates IdentNode
		%S : argument is a string literal, creates StrLitNode
*/
ASTNode *astnode_create(struct MemArena *ma, int line, int type, ValueUnion value, char *fmt, ...);
void astnode_setchild(struct MemArena *ma, ASTNode *node, int index, ASTNode *child);

typedef int (*ast_printfunc)(void *arg, const char *fmt, ...);

//returns 1 if traverser should skip the children of this node, zero otherwise
typedef int (*ast_traversecb)(void *arg, ASTNode *node);

//returns number of characters written
int astnode_print(ASTNode *node, int tablevel, void *arg, ast_printfunc printfunc);

#endif /*_AST_H*/
