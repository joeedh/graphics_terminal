#include <stdio.h>
#include <stdarg.h>
#include "../memarena.h"
#include "../hash.h"
#include "../strhash.h"
#include "transform.h"

#include "stylecompiler.gen.h"
#include "compiler.h"
#include <setjmp.h>

void yyset_in (FILE *  in_str , yyscan_t yyscanner);

static errorprint_t errorprint = NULL;
static void *errorarg = NULL;

void set_errorprint(void *arg, errorprint_t func) {
	errorprint = func;
	errorarg = arg;
}

void yyerror (CompilerState *cstate, YYLTYPE *yylloc, yyscan_t *scanner, char const *message) {
	if (!errorprint) {
		return; //no error printing function
	}

	errorprint(errorarg, "Line %d: Syntax Error\n", yylloc->lineno+1);
}

char *_compiler_strdup(struct CompilerState *cstate, const char *str) {
	int len = strlen(str);
	char *ret = memarena_malloc(cstate->ma_r, len+1);

	memcpy(ret, str, len);
	ret[len] = 0;

	return ret;
}

static void cerror(CompilerState *cstate, ASTNode *node, const char *msg, ...) {
	cstate->haderror = 1;
	char buf[256];

	if (!errorprint) return;
	
	va_list va;
	va_start(va, msg);

	vsprintf_s(buf, sizeof(buf), msg, va);

	va_end(va);

	if (node) {
		errorprint(errorarg, "Error: line %d: %s\n", node->line, buf);
	} else {
		errorprint(errorarg, "Error: (at unknown line): %s\n", buf);
	}

	longjmp(*cstate->jmpbuf, 1);
}

char *compilestyle(const char *source, int len, int *len_out) {
#if defined(YYDEBUG) && YYDEBUG
	extern int yydebug;
	yydebug = 1;
#endif

	CompilerState _state={0,}, *state = &_state;
	StrReader reader = {source, 0, len};
	YYLTYPE yylloc = {0,};

	yyscan_t scanner;

	if (!errorprint) {
		set_errorprint((void*) stderr, (errorprint_t)fprintf);
	}

	
	state->ma_r = memarena_new(4096);
	yylex_init(&scanner);
	yyset_in((FILE*)&reader, scanner);

	printf("compiling AST tree. . .\n");

	if (yyparse(state, &yylloc, scanner)==0) {
		astnode_print(state->result, 0, stdout, fprintf);
	} else { //error
		errorprint(errorarg, "Parse error!\n");
		return NULL;
	}

	state->index_globals =  memarena_malloc(state->ma_r, sizeof(*state->globals));
	state->global_indices = memarena_malloc(state->ma_r, sizeof(*state->globals));
	state->globals = memarena_malloc(state->ma_r, sizeof(*state->globals));
	state->scope = memarena_malloc(state->ma_r, sizeof(*state->scope));
	state->builtin_funcs = memarena_malloc(state->ma_r, sizeof(*state->builtin_funcs));
	state->funcs = memarena_malloc(state->ma_r, sizeof(*state->funcs));
	
	strhash_init(state->index_globals);
	strhash_init(state->global_indices);
	strhash_init(state->globals);
	strhash_init(state->scope);
	strhash_init(state->builtin_funcs);
	strhash_init(state->funcs);

	state->error = cerror;

	//set longjump
	state->jmpbuf = memarena_malloc(state->ma_r, sizeof(jmp_buf));
	setjmp(*state->jmpbuf);

	char *buf = NULL;
	int len2 = 0;

	//see if an exception happened
	if (state->haderror) {
		errorprint(errorarg, "compilation failed; aborting\n");
	} else {
		setup_builtins(state);
		transform_ast(state);
		buf = emit_bytecode(state, &len2);
	}

	strhash_release(state->index_globals);
	strhash_release(state->global_indices);
	strhash_release(state->globals);
	strhash_release(state->scope);
	strhash_release(state->builtin_funcs);
	strhash_release(state->funcs);

	yylex_destroy(scanner);
	memarena_free(state->ma_r);

	*len_out = len2;
	return buf;
}

ASTNode *find_node(ASTNode *node, int type) {
	int i;

	if (node->type == type) {
		return node;
	}

	for (i=0; i<node->totchildren; i++) {
		ASTNode *ret = find_node(node->children[i], type);

		if (ret) {
			return ret;
		}
	}

	return NULL;
}

ASTNode *cctemplate(CompilerState *cstate, int startnode, const char *fmt, ...) {
	int len = strlen(fmt);
	char *source = memarena_calloc(cstate->ma_r, len*5); //XXX potential buffer overrun here
	int i, c=0;
	va_list va;

	va_start(va, fmt);

	for (i=0; i<len; i++) {
		if (fmt[i] == '%') {
			switch (fmt[++i]) {
				case 's': {
					char *s = va_arg(va, char*);
					while (*s) {
						source[c++] = *s;
						s++;
					}
					break;
				}
			}
		} else {
			source[c++] = fmt[i];
		}
	}
	
	va_end(va);
	
	source[c++] = 0;
	len = c;

	CompilerState cstate2 = *cstate;
	StrReader reader = {source, 0, len};
	YYLTYPE yylloc = {0,};
	yyscan_t scanner;

	yylex_init(&scanner);
	yyset_in((FILE*)&reader, scanner);

	if (yyparse(&cstate2, &yylloc, scanner)==0) {
		astnode_print(cstate2.result, 0, stdout, fprintf);
		
		cstate2.result = find_node(cstate2.result, startnode);

		return cstate2.result;
	} else { //error
		errorprint(errorarg, "Parse error!\n");
		return NULL;
	}
}

