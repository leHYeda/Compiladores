/* Lexer para uma calculadora avancada */
%option noyywrap nodefault yylineno

%{
#include "calcAvancada.h"
#include "calcAvancada.tab.h"
%}

/* expoente float */
EXP ([Ee][-+]?[0-9]+)

%%
 /* operadores de caracter unico */
"+" |
"-" |
"*" |
"/" |
"=" |
"|" |
"," |
";" |
"(" |
")" { return yytext[0]; }

 /* operadores de comparacao, todos sao token CMP */
">"  { yylval.fn = 1; return CMP; }
"<"  { yylval.fn = 2; return CMP; }
"!=" { yylval.fn = 3; return CMP; }
"==" { yylval.fn = 4; return CMP; }
">=" { yylval.fn = 5; return CMP; }
"<=" { yylval.fn = 6; return CMP; }

 /* palavras-chave */
"if"    { return IF; }
"then"  { return THEN; }
"else"  { return ELSE; }
"while" { return WHILE; }
"do"    { return DO; }
"let"   { return LET; }

 /* funcoes pre-definidas */
"sqrt"  { yylval.fn = B_sqrt; return FUNC; }
"exp"   { yylval.fn = B_exp; return FUNC; }
"log"   { yylval.fn = B_log; return FUNC; }
"print" { yylval.fn = B_print; return FUNC; }

 /* nomes */
[a-zA-Z][a-zA-Z0-9]* { yylval.s = lookup(yytext); return NAME; }

[0-9]+"."[0-9]*{EXP}? |
"."?[0-9]+{EXP}? { yylval.d = atof(yytext); return NUMBER; }

"//".* { /* ignora comentarios */ }
[ \t]   { /* ignora espaco em branco */ }

 /* ignora continuacao de linha com barra invertida */
\\\n    { printf("c> "); }

\n      { return EOL; }

.       { yyerror("Caracter desconhecido %c\n", *yytext); }
%%