/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_F_DEV_ASMBOOT_GRAPHICSTERMINAL_STYLECOMPILER_STYLECOMPILER_GEN_H_INCLUDED
# define YY_YY_F_DEV_ASMBOOT_GRAPHICSTERMINAL_STYLECOMPILER_STYLECOMPILER_GEN_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 10 "F:\\dev\\asmboot\\graphicsterminal\\stylecompiler\\stylecompiler.y" /* yacc.c:1909  */

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

#line 108 "F:\\\\dev\\asmboot\\graphicsterminal\\stylecompiler\\\\stylecompiler.gen.h" /* yacc.c:1909  */

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    COMMA = 258,
    ID = 259,
    ADD = 260,
    SUB = 261,
    MUL = 262,
    DIV = 263,
    GT = 264,
    LT = 265,
    GTE = 266,
    LTE = 267,
    EQ = 268,
    ASSIGN = 269,
    ASSIGN_ADD = 270,
    ASSIGN_SUB = 271,
    ASSIGN_MUL = 272,
    ASSIGN_DIV = 273,
    IF = 274,
    ELSE = 275,
    WHILE = 276,
    DO = 277,
    FOR = 278,
    RETURN = 279,
    LBRACKET = 280,
    RBRACKET = 281,
    LSBRACKET = 282,
    RSBRACKET = 283,
    LPAREN = 284,
    RPAREN = 285,
    DOT = 286,
    COLON = 287,
    QUESTION = 288,
    SEMI = 289,
    UNIFORM = 290,
    INTNUM = 291,
    FLOATNUM = 292,
    STRLIT = 293,
    NEG = 294
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef ValueUnion YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int yyparse (CompilerState *cstate, YYLTYPE *yylloc, yyscan_t yyscanner);

#endif /* !YY_YY_F_DEV_ASMBOOT_GRAPHICSTERMINAL_STYLECOMPILER_STYLECOMPILER_GEN_H_INCLUDED  */
