%option reentrant
%option noyywrap

%{
#define YY_DECL int yylex(ValueUnion *yylval_param, CompilerState *cstate,
                          YYLTYPE *yylloc_param, yyscan_t yyscanner)

#include "stylecompiler.gen.h"
#include "ctype.h"

#define fileno(file)

#include <stdio.h>

int isatty() {
	return 0;
}

#define YY_INPUT(buf, result, max_size) StrReader_read((StrReader*)yyin, buf, &result, max_size)\

%}

%%

"return"           return RETURN;
"if"               return IF;
"else"             return ELSE;
"do"               return DO;
"while"            return WHILE;
"for"              return FOR;

[0-9]+             {int val=0; myatoi(yytext, 10, &val); yylval->i = val; return INTNUM;}
0x[0-9]+           {int val=0; myatoi(yytext, 16, &val); yylval->i = val; return INTNUM;}
[0-9]+\.[0-9]*     {double val=0; sscanf_s(yytext, "%lf", &val); yylval->f = val; return FLOATNUM;}
";"                return SEMI;
"="                return ASSIGN;
"+"                return ADD;
"-"                return SUB;
"*"                return MUL;
"/"                return DIV;
"=="               return EQ;
">"                return GT;
"<"                return LT;
">="               return GTE;
"<="               return LTE;
"{"                return LBRACKET;
"}"                return RBRACKET;
"["                return LSBRACKET;
"]"                return RSBRACKET;
"("                return LPAREN;
")"                return RPAREN;
"if"               return IF;
"else"             return ELSE;
"+="               return ASSIGN_ADD;
"-="               return ASSIGN_SUB;
"*="               return ASSIGN_MUL;
"/="               return ASSIGN_DIV;
","                return COMMA;
"\n"               {yylloc->lineno++; yylineno++; yylloc->lexpos = (int)(yy_cp - yyg->yytext_r);} //drop token
[\t\r \v\b\x7f\0]  /*ignore token*/
[a-zA-Z_]+[a-zA-Z0-9_]*  {yylval->s = _compiler_strdup(cstate, yytext); return ID;}

%%