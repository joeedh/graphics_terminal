%define api.pure full
%parse-param {CompilerState *cstate}
%parse-param {YYLTYPE *yylloc}
%parse-param {yyscan_t yyscanner}

%lex-param   {CompilerState *cstate}
%lex-param   {YYLTYPE *yylloc}
%lex-param   {yyscan_t yyscanner}

%code requires {
  #include "ast.h"
  #include <setjmp.h>

  struct ASTNode;
  struct MemArena;
  struct HashTable;

  typedef struct CompilerState {
	struct MemArena *ma_r;
	struct ASTNode *result;
	void (*error)(struct CompilerState *self, ASTNode *node, const char *msg, ...);
	void (*warning)(struct CompilerState *self, ASTNode *node, const char *msg, ...);
	
	jmp_buf *jmpbuf;
	int haderror;

	struct HashTable *globals, *funcs, *builtin_funcs, *global_indices, *index_globals;
	struct HashTable *scope, **scopestack;
	int scopestack_size, scopestack_len;
  } CompilerState;

  typedef struct StrReader {
	  const char *str;
	  int cur, len;
  } StrReader;

  //used by lexer
  char *_compiler_strdup(struct CompilerState *cstate, const char *str);

  static int StrReader_read(StrReader *r, const char *buf, int *readcount, int max_size) {
    char *c = (char*) buf;
	int count = 0;

	while (max_size > 0 && r->cur < r->len) {
		*c = r->str[r->cur];

		c++;
		r->cur++;
		max_size--;
		count++;
	}

	*readcount = count;
	return count;
  }

  typedef struct YYLTYPE {
     //int linestart, lineend;
	 //int lexstart, lexend;
	 int lineno, lexpos;
   } YYLTYPE;

   #ifndef YY_TYPEDEF_YY_SCANNER_T
   #define YY_TYPEDEF_YY_SCANNER_T
   typedef void* yyscan_t;
   #endif

   int yylex_init (yyscan_t *scanner);
   int yylex_destroy (yyscan_t yyscanner);
   int yylex (ValueUnion *yylval, CompilerState *cstate, YYLTYPE *yylloc, yyscan_t scanner);
   void yyerror (CompilerState *cstate, YYLTYPE *yylloc, yyscan_t *scanner, char const *);
}

%{
  //#define YYDEBUG 1
  //#define YYERROR_VERBOSE 1

  #include <math.h>
  #include <stdio.h>
  #include "../memarena.h"

  #define vnode(...) VN(astnode_create(ma, yylloc->lineno, __VA_ARGS__))
  #define node(...) astnode_create(ma, yylloc->lineno, __VA_ARGS__)
  #define append(...) astnode_append(ma, __VA_ARGS__)

%}

/* Bison declarations.  */
%define api.value.type {ValueUnion}

%token COMMA
%token ID
%token ADD
%token SUB
%token MUL
%token DIV
%token GT
%token LT
%token GTE
%token LTE
%token EQ
%token ASSIGN
%token ASSIGN_ADD
%token ASSIGN_SUB
%token ASSIGN_MUL
%token ASSIGN_DIV
%token IF
%token ELSE
%token WHILE
%token DO
%token FOR
%token RETURN
%token LBRACKET
%token RBRACKET
%token LSBRACKET
%token RSBRACKET
%token LPAREN
%token RPAREN
%token DOT
%token COLON
%token QUESTION
%token SEMI
%token UNIFORM
%token INTNUM
%token FLOATNUM
%token STRLIT

%left SUB ADD
%left MUL DIV
%left GT LT GTE LTE EQ
%left ASSIGN ASSIGN_ADD ASSIGN_SUB ASSIGN_MUL ASSIGN_DIV
%left QUESTION COLON

%precedence NEG   /* negation--unary minus */

%code {
  #define ma cstate->ma_r

  int is_builtin_type(char *type) {
	if (!strcmp(type, "int"))
		return T_INT;
	if (!strcmp(type, "float"))
		return T_FLOAT;

	return T_UNKNOWN;
  }
}

%% /* The grammar follows.  */

script        : statementlist             {cstate->result = $1.n; $$ = $1;}

statementlist : %empty                    {$$ = vnode(StatementList, VI(0), NULL);}
              | statementlist statement   {append($1.n, $2.n); $$ = $1;}
;

statement : SEMI               {$$ = vnode(NullOpNode, VI(0), NULL);}
		  | expr SEMI          {$$ = $1;}
		  | function           {$$ = $1;}
		  | vardecl SEMI       {$$ = $1;}
		  | RETURN expr SEMI   {$$ = vnode(ReturnNode, VI(0), "%n", $2.n);}
;

function : id ID LPAREN func_arglist RPAREN LBRACKET statementlist RBRACKET {$$ = vnode(FunctionNode, $2, "%n %n %n", $1.n, $4.n, $7.n);}
;

func_arglist : id id                      {ASTNode *n = node(PairNode, VI(0), "%n %n", $1.n, $2.n);
                                           $$ = vnode(ArgListNode, VI(0), "%n", n);}
             | func_arglist COMMA id id   {ASTNode *n = node(PairNode, VI(0), "%n %n", $2.n, $3.n);
			                               
										   if ($1.n->type != ArgListNode) {
										     $1 = vnode(ArgListNode, VI(0), "%x", $1.n);
										   }

								           append($1.n, n);
			                               $$ = $1;}
			 | %empty                     {$$ = vnode(ArgListNode, VI(0), NULL);}
;

vardecl            : id vardecl_list                    {int i;
                                                         $$ = $2;

                                                         //type children
														 if ($$.n->type == ExprListNode) {
															 for (i=0; i<$$.n->totchildren; i++) {
																ASTNode *n = $$.n->children[i]->children[0];
																n->value.i = is_builtin_type($1.n->value.s);
																append(n, node(IdentNode, $1.n->value, NULL));
															 }
														  } else {
														    append($$.n->children[0], $1.n);
														  }
                                                        }
;

vardecl_list       : vardecl_single                     {$$ = $1;}
                   | vardecl_list COMMA vardecl_single  {if ($1.n->type != ExprListNode) {
				                                             $1 = vnode(ExprListNode, VI(0), "%n", $1.n);
				                                         }

														 $$ = $1;
														 append($$.n, $3.n);
                                                        }
;

/* first child is type, second is assignment.  name is stored in .value.s */
vardecl_single     : ID vardecl_assign_opt {$$ = vnode(VarDeclNode, $1, "%n%n", node(TypeRefNode, VI(0), NULL), $2.n);}
;


funccall           : ID LPAREN expr RPAREN {if ($3.n->type != ExprListNode) {
                                                $3.n = node(ExprListNode, VS(0), "%n", $3.n);
                                            }
											$$ = vnode(FuncCallNode, $1, "%n", $3.n);
										   }

vardecl_assign_opt : ASSIGN assign_expr { $$ = $2; }
                   | %empty             { $$ = vnode(EmptyNode, VI(0), NULL);}
;

assign_expr        : simple_expr
                   | bracket_literal
				   
;

comma_opt          : COMMA
                   | %empty
;

bracket_literal    : LBRACKET expr_opt comma_opt RBRACKET {$$ = vnode(BracketLiteral, VI(0), "%n", $1.n);}
;

expr_opt    : expr                              {$$ = $1;}
            | %empty                            {$$ = vnode(EmptyNode, VI(0), NULL);}
;

expr        : simple_expr                       {$$ = $1;}
            | id ASSIGN expr                    {$$ = vnode(AssignNode, VI(ASSIGN), "%n%n", $1.n, $3.n);}
            | expr COMMA simple_expr            {if ($1.n->type != ExprListNode) {
									               $$ = vnode(ExprListNode, VI(0), "%n", $1.n);
			                                     } else {
									               $$ = $1;
												 }

										         append($$.n, $3.n);
									            }
;

simple_expr : num                              {$$ = $1;}
            | id                               {$$ = $1;}
			| strlit                           {$$ = $1;}
	        | simple_expr ADD simple_expr      {$$ = vnode(BinOpNode, VI(ADD), "%n %n", $1.n, $3.n);}
	        | simple_expr SUB simple_expr      {$$ = vnode(BinOpNode, VI(SUB), "%n %n", $1.n, $3.n);}
	        | simple_expr MUL simple_expr      {$$ = vnode(BinOpNode, VI(MUL), "%n %n", $1.n, $3.n);}
	        | simple_expr DIV simple_expr      {$$ = vnode(BinOpNode, VI(DIV), "%n %n", $1.n, $3.n);}
	        | simple_expr EQ  simple_expr      {$$ = vnode(BinOpNode, VI(EQ), "%n %n", $1.n, $3.n);}
	        | simple_expr GT  simple_expr      {$$ = vnode(BinOpNode, VI(GT), "%n %n", $1.n, $3.n);}
	        | simple_expr LT  simple_expr      {$$ = vnode(BinOpNode, VI(LT), "%n %n", $1.n, $3.n);}
	        | simple_expr GTE simple_expr      {$$ = vnode(BinOpNode, VI(GTE), "%n %n", $1.n, $3.n);}
	        | simple_expr LTE simple_expr      {$$ = vnode(BinOpNode, VI(LTE), "%n %n", $1.n, $3.n);}
			| LPAREN expr RPAREN               {$$ = $2;}
			| funccall                         {$$ = $1;}
;

id          : ID               {$$ = vnode(IdentNode, $1, NULL);}
;

strlit      : STRLIT           {$$ = vnode(StrLitNode, $1, NULL);}
;

num         : INTNUM           {$$ = vnode(IntLitNode, $1, NULL);}
	        | FLOATNUM         {$$ = vnode(FloatLitNode, $1, NULL);}
;

 %%
