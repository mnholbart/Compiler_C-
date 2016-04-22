%{

#include <stdio.h>
#include "Token.h"
#include "util.h"
#include "c-.tab.h"

int numWarnings;
int numErrors;

char* StringEscape(char *in) {
	yylval.token.rawString = strdup(in);

	char *temp = (char*)malloc(strlen(in)*sizeof(char));
	int length=0;
	
	for (int i=1; i<strlen(in)-1; i++) {	
		int c = in[i];
		if (c == '\\') {
			if (in[i+1] == 'n') 
				c = '\n';
			else if(in[i+1] == '0')
				c = '\0';
			else c = in[i+1];
			i++;	
		}
		temp[length] = c;
	length++;
	}
	//yylval.token.type.sValue = strdup(temp);	

	yylval.token.sLength = length;
	return temp;
}

char* checkCharErr(char * yytext) {

	int charLen = strlen(yytext);
	if (charLen == 4 && yytext[1] == '\\')
		charLen--;

	if (charLen != 3) {
		printf("WARNING(%u): character is %u characters and not a single character: %s\n", yylineno, strlen(yytext)-2, yytext);
		numWarnings++;
		
	}

}

%}

%option yylineno
%option noyywrap
 
%%

[ \n\t]		;
\/\/(.)*"\n"	;	
\"((\\\")|[^\"\n])*\"	{ yylval.token.type.sValue = StringEscape(yytext); yylval.token.lineno = yylineno; return STRINGCONST; }
'((\\\')|[^\'\n])*\'	{ yylval.token.type.cValue = StringEscape(yytext)[0]; yylval.token.lineno = yylineno; checkCharErr(yytext); return CHARCONST; }
"true"		{ yylval.token.type.iValue = 1; yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return BOOLCONST; }
"false"		{ yylval.token.type.iValue = 0; yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno;  return BOOLCONST; }
[0-9]+		{ yylval.token.type.iValue = atoi(yytext); yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return NUMCONST; }
"bool"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return BOOL; }
"break"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return BREAK; }
"char"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return CHAR; }
"else"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return ELSE; }
"foreach"	{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return FOREACH; }
"if"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return IF; }
"in"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return IN; }
"int"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return INT; }
"return"	{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return RETURN; }
"static" 	{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return STATIC; }
"while"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return WHILE; }
"*="		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return MULASS; }
"/="		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return DIVASS; }
"+="		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return ADDASS; }
"-="		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return SUBASS; }
"!="		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return NOTEQ; }
"=="		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return EQ; }
"++"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return INC; }
"--"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return DEC; }
"<="		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return LESSEQ; }
">="		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return GRTEQ; }
";"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return SCOLON; }
":"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return COLON; }
","		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return COMMA; }
"["		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return OBRACKET; }
"]"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return CBRACKET; }
"("		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return OPAREN; }
")"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return CPAREN; }
"+"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return PLUS; }
"-"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return MINUS; }
"*"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return MULT; }
"/"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return DIV; }
"&"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return AND; }
"!"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return NOT; }
"{"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return OBRACE; }
"}"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return CBRACE; }
"|"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return OR; }
"="		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return EQUALS; }
"%"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return MODULUS; }
"<"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return LESS; }
">"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return GREATER; }
"?"		{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return TERNARY; }
[a-zA-Z]+[0-9a-zA-Z]*	{ yylval.token.rawString = strdup(yytext); yylval.token.lineno = yylineno; return ID; }
.		{ printf("WARNING(%u): Invalid input character: \'%c\'. Character ignored.\n", yylineno, yytext[0]); numWarnings++; }

%%


