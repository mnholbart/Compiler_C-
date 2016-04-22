%{ 
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <string>
#include "util.h"
#include "Token.h"
#include "symbolTable.h"
#include "genCode.h"

using namespace std;

extern int yylex();
extern int yylineno;
extern FILE *yyin;
extern char *yytext;
extern void initTokenMaps();

#define YYERROR_VERBOSE
/*void yyerror(const char *msg)
{
  printf("ERROR(PARSER): %s, LINENO: %i\n", msg, yylineno);
  exit(-1);
}*/

static char *savedName;
static int savedLineNo;
static TreeNode *savedTree;
static std::map<std::string, char*> niceTokenNameMap;
extern int numErrors;
extern int numWarnings;
extern int globalOffset;

int split(char *s, char *strs[], char breakchar) {
    int num;
    
    strs[0] = s;
    num = 1;
    for (char *p = s; *p; p++) {
        if (*p==breakchar) {
            strs[num++] = p+1;
            *p = '\0';
        }
    }
    strs[num] = NULL;
    
    return num;
}

void trim(char *s) {
	s[strlen(s)-1] = '\0';
}

void initTokenMaps() {
    niceTokenNameMap["NOTEQ"] = (char *)"'!='";
    niceTokenNameMap["MULASS"] = (char *)"'*='";
    niceTokenNameMap["INC"] = (char *)"'++'";
    niceTokenNameMap["ADDASS"] = (char *)"'+='";
    niceTokenNameMap["DEC"] = (char *)"'--'";
    niceTokenNameMap["SUBASS"] = (char *)"'-='";
    niceTokenNameMap["DIVASS"] = (char *)"'/='";
    niceTokenNameMap["LESSEQ"] = (char *)"'<='";
    niceTokenNameMap["EQ"] = (char *)"'=='";
    niceTokenNameMap["GRTEQ"] = (char *)"'>='";
    niceTokenNameMap["BOOL"] = (char *)"bool";
    niceTokenNameMap["BREAK"] = (char *)"break";
    niceTokenNameMap["CHAR"] = (char *)"char";
    niceTokenNameMap["ELSE"] = (char *)"else";
    niceTokenNameMap["FOREACH"] = (char *)"foreach";
    niceTokenNameMap["IF"] = (char *)"if";
    niceTokenNameMap["IN"] = (char *)"in";
    niceTokenNameMap["INT"] = (char *)"int";
    niceTokenNameMap["RETURN"] = (char *)"return";
    niceTokenNameMap["STATIC"] = (char *)"static";
    niceTokenNameMap["WHILE"] = (char *)"while";
    niceTokenNameMap["BOOLCONST"] = (char *)"Boolean constant";
    niceTokenNameMap["NUMCONST"] = (char *)"numeric constant";
    niceTokenNameMap["ID"] = (char *)"identifier";
    niceTokenNameMap["CHARCONST"] = (char *)"character constant";
    niceTokenNameMap["STRINGCONST"] = (char *)"string constant";
    niceTokenNameMap["$end"] = (char *)"end of input";
   
    niceTokenNameMap["OPAREN"] = (char *)"(";
    niceTokenNameMap["CPAREN"] = (char *)")";
    niceTokenNameMap["SCOLON"] = (char *)";";
    niceTokenNameMap["COLON"] = (char *)":";
    niceTokenNameMap["COMMA"] = (char *)",";
    niceTokenNameMap["OBRACKET"] = (char *)"[";
    niceTokenNameMap["CBRACKET"] = (char *)"]";
    niceTokenNameMap["OBRACE"] = (char *)"{";
    niceTokenNameMap["CBRACE"] = (char *)"}";
    niceTokenNameMap["TERNARY"] = (char *)"?";
    niceTokenNameMap["GREATER"] = (char *)">";
    niceTokenNameMap["LESS"] = (char *)"<";
    niceTokenNameMap["MODULUS"] = (char *)"%";
    niceTokenNameMap["EQUALS"] = (char *)"=";
    niceTokenNameMap["DIV"] = (char *)"/";
    niceTokenNameMap["PLUS"] = (char *)"+";
    niceTokenNameMap["MINUS"] = (char *)"-";
    niceTokenNameMap["MULT"] = (char *)"*";
    niceTokenNameMap["AND"] = (char *)"&";
    niceTokenNameMap["NOT"] = (char *)"!";
    niceTokenNameMap["OR"] = (char *)"|";
}

char *niceTokenStr(char *tokenName ) {
    if (tokenName[0] == '\'') return tokenName;
    if (niceTokenNameMap.find(tokenName) == niceTokenNameMap.end()) {
        printf("ERROR(SYSTEM): niceTokenStr fails to find string '%s'\n", tokenName); 
        fflush(stdout);
        exit(1);
    }
    return niceTokenNameMap[tokenName];
}

bool elaborate(char *s)
{
    return (strstr(s, "constant") || strstr(s, "identifier"));
}

void tinySort(char *base[], int num, int step, bool up)
{
    for (int i=step; i<num; i+=step) {
        for (int j=0; j<i; j+=step) {
            if (up ^ (strcmp(base[i], base[j])>0)) {
                char *tmp;
                tmp = base[i]; base[i] = base[j]; base[j] = tmp;
            }
        }
    }
}

void yyerror(const char *msg)
{
    char *space;
    char *strs[100];
    int numstrs;

    // make a copy of msg string
    space = strdup(msg);

    // split out components
    numstrs = split(space, strs, ' ');
    if (numstrs>4) trim(strs[3]);

    // translate components
    for (int i=3; i<numstrs; i+=2) {
        strs[i] = niceTokenStr(strs[i]);
    }
//	printf("\n%s\n\n", strs[5]);
    // print components
    if (strlen(strs[3]) == 1)
	printf("ERROR(%d): Syntax error, unexpected \'%s\'", yylineno, strs[3]);
    else
	printf("ERROR(%d): Syntax error, unexpected %s", yylineno, strs[3]);
    if (elaborate(strs[3])) {
        if (yytext[0]=='\'' || yytext[0]=='"') printf(" %s", yytext); 
        else printf(" \'%s\'", yytext);
    }
    if (numstrs>4) printf(",");
    tinySort(strs+5, numstrs-5, 2, true); 
    for (int i=4; i<numstrs; i++) {
    	if (strlen(strs[i]) == 1)
		printf(" \'%s\'", strs[i]);
	else
	   	printf(" %s", strs[i]);
    }
    printf(".\n");
    fflush(stdout);   // force a dump of the error

    numErrors++;

    free(space);
}

%}

%union {
   Token token;
   TreeNode * t;
   int integer;
   char *str;
}

%token <token> ID
%token <token> NUMCONST
%token <token> BOOLCONST
%token <token> STRINGCONST
%token <token> CHARCONST
%token <token> OPERATOR
%token <token> ERRORTOKEN

%token <token> BOOL BREAK CHAR ELSE FOREACH IF IN INT RETURN STATIC WHILE
%token <token> NOTEQ EQ LESSEQ GRTEQ INC DEC ADDASS SUBASS MULASS DIVASS
%token <token> SCOLON COLON COMMA PLUS MINUS MULT DIV 
%token <token> AND NOT OR EQUALS MODULUS LESS GREATER TERNARY
%token <token> OBRACE CBRACE OPAREN CPAREN OBRACKET CBRACKET



%type <t>	constant
		arg_list
		args
		call
		immutable
		mutable
		factor
		unary_expression
		term
		sum_expression
		rel_expression
		unary_rel_expression
		and_expression
		simple_expression
		expression
		break_stmt
		return_stmt
		unmatched_iteration_stmt
		matched_iteration_stmt
		expression_stmt
		statement_list
		local_declarations
		compound_stmt
		unmatched_if_stmt
		matched_if_stmt
		unmatched
		matched
		statement
		param_id
		param_id_list
		param_type_list
		param_list
		params
		fun_declaration
		var_decl_id
		var_decl_initialize
		var_decl_list
		scoped_var_declaration
		var_declaration
		declaration_list
		declaration
		program
		scoped_type_specifier

%type <token>	unaryop
		relop
		mulop
		sumop
		binary_exp
		unary_exp

%type <integer> type_specifier


%%

program			: declaration_list
				{ savedTree = $1; }
			; 

declaration_list	: declaration_list declaration
				{ TreeNode * t = $1;
					if ( t != NULL)
					{ while (t->sibling != NULL)
						t = t->sibling;
					t->sibling = $2;
					$$ = $1; }
					else $$ = $2;
				}
			| declaration
				{ $$ = $1; }
			;

declaration		: var_declaration
				{ $$ = $1; }
			| fun_declaration
				{ $$ = $1; }
			| error { $$ = NULL; }
			;

var_declaration 	: type_specifier var_decl_list SCOLON 
				{ yyerrok;
				if ($2 != NULL) {
				TreeNode * t = $2;
				t->type = DeclType($1);
					while ( t->sibling != NULL) {
						t = t->sibling;
						t->type = (DeclType)$1;
					} 
				$$ = $2;
				} else {$$ = NULL; }				
				}
			| error SCOLON { yyerrok; $$ = NULL; }
			;

scoped_var_declaration 	: scoped_type_specifier var_decl_list SCOLON
				{ yyerrok;
				if ($2 != NULL) {
				TreeNode * t = $2;
				t->type = $1->type;
				t->isStatic = $1->isStatic;
					while ( t->sibling != NULL) {
						t = t->sibling;
						t->type = $1->type;
						t->isStatic = $1->isStatic;
					}
				$$ = $2;
				} else { $$ = NULL; }
				}
			| scoped_type_specifier error { $$ = NULL; }
			;

var_decl_list		: var_decl_list COMMA var_decl_initialize
				{ TreeNode * t = $1;
				if ( t != NULL)
					{ while (t->sibling != NULL)
						t = t-> sibling;
					t->sibling = $3;
					$$ = $1; }
					else $$ = $3;
				}
			| var_decl_initialize 
				{ $$ = $1; }
			| error COMMA var_decl_initialize { yyerrok; $$ = NULL; }
			| var_decl_list COMMA error { $$ = NULL; }
			;

var_decl_initialize	: var_decl_id
				{ $$ = $1; }
			| var_decl_id COLON simple_expression
				{ TreeNode *t = $1;	
				t->child[0] = $3;
				$$ = t;
				}
			| var_decl_id COLON error { $$ = NULL; }
			| error COLON simple_expression 
				{ yyerrok; $$ = NULL; }
			| error { $$ = NULL; }
			;

var_decl_id		: ID 
				{ yyerrok;
				TreeNode * t = newDeclNode(VarK);
				t->lineno = $1.lineno;
				t->attr.name = $1.rawString;
				$$ = t;
				}
			| ID OBRACKET NUMCONST CBRACKET
				{ yyerrok; 
				TreeNode * t = newDeclNode(VarK);
				t->lineno = $1.lineno;
				t->attr.name = $1.rawString;
				t->isArray = true;
				t->arrayLength = $3.type.iValue;
				$$ = t;
				}
			| ID OBRACKET error { $$ = NULL; }
			| error CBRACKET { yyerrok; $$ = NULL; }
			;

scoped_type_specifier	: STATIC type_specifier
				{ TreeNode * t = newDeclNode(VarK);
				t->isStatic = true;
				t->type = (DeclType)$2;
				$$ = t; }
			| type_specifier 
				{ TreeNode * t = newDeclNode(VarK);
				t->isStatic = false;
				t->type = (DeclType)$1;
				$$ = t; }
			;

type_specifier		: INT
				{ $$ = Int; }
			| BOOL
				{ $$ = Bool; }
			| CHAR
				{ $$ = Char; } 
			;

fun_declaration		: type_specifier ID OPAREN params CPAREN statement
				{ TreeNode * t = newDeclNode(FuncK);
				t->lineno = $3.lineno;
				t->type = (DeclType)$1;
				t->attr.name = $2.rawString;
				t->child[0] = $4;
				t->child[1] = $6;
				$$ = t;
				}				
			| ID OPAREN params CPAREN statement
				{ TreeNode * t = newDeclNode(FuncK);
				t->lineno = $2.lineno;
				t->type = Void;
				t->attr.name = $1.rawString;
				t->child[0] = $3;
				t->child[1] = $5;
				$$ = t;
				}
			| type_specifier ID OPAREN error { $$ = NULL; }
			| type_specifier ID OPAREN params CPAREN error { $$ = NULL; }
			| ID OPAREN params CPAREN error { $$ = NULL; }
			| ID OPAREN error { $$ = NULL; }
			| type_specifier error { $$ = NULL; }
			;

params			: param_list
				{ $$ = $1; }
			| 	{ $$ = NULL; }
			;

param_list		: param_list SCOLON param_type_list
				{ yyerrok; 
				TreeNode * t = $1;
				if ( t != NULL)
				{ while ( t->sibling != NULL)
					t = t->sibling;
				t->sibling = $3;
				$$ = $1; }
				else $$ = $3;
				}
			| param_type_list 
				{ $$ = $1; }
			| error SCOLON param_type_list { yyerrok; $$ = NULL; }
			| param_list SCOLON error { $$ = NULL; }
			;

param_type_list		: type_specifier param_id_list {
				TreeNode * t = $2;
				if ($2 != NULL) {
					t->type = (DeclType)$1;
					while (t->sibling != NULL) {
						t = t->sibling;
						t->type = (DeclType)$1;
					}	
				$$ = $2;			
				} else 
					$$ = NULL;
				}
			;

param_id_list		: param_id_list COMMA param_id 
				{ yyerrok;
				TreeNode * t = $1;
				if (t != NULL) 
				{ while ( t->sibling != NULL)
					t = t->sibling;
				t->sibling = $3;
				$$ = $1; }
				else $$ = $3;
				}
			| param_id
				{ $$ = $1; }
			| error COMMA param_id { yyerrok; $$ = NULL; }
			| param_id_list COMMA error { $$ = NULL; }
			;

param_id		: ID 
				{ yyerrok;
				TreeNode * t = newDeclNode(ParamK);
				t->attr.name = $1.rawString;
				t->isArray = false;
				$$ = t;
				}
			| ID OBRACKET CBRACKET 
				{ yyerrok; 
				TreeNode * t = newDeclNode(ParamK);
				t->attr.name = $1.rawString;
				t->isArray = true;
				$$ = t;
				}
			| error { $$ = NULL; }
			;

//	statement		: statement_non_conditional
//				| conditional
//				;

//	statement_non_conditional : expression_stmt
//				| compound_stmt
//				| return_stmt
//				| break_stmt
//				;

//	conditional		: complete_conditional
//				| incomplete_conditional
//				;

//	complete_conditional	: IF '(' simple_expression ')' statement_non_conditional ELSE statement
//				| IF '(' simple_expression ')' complete_conditional ELSE statement
//				;

//	incomplete_conditional	: IF '(' simple_expression ')' statement
//				;

statement		: matched
				{ $$ = $1; }
			| unmatched
				{ $$ = $1; }
			;

matched			: expression_stmt
				{ $$ = $1; }
			| matched_if_stmt
				{ $$ = $1; }
			| matched_iteration_stmt
				{ $$ = $1; }
			| return_stmt
				{ $$ = $1; }
			| break_stmt
				{ $$ = $1; }
			| compound_stmt
				{ $$ = $1; }
			;

unmatched		: unmatched_if_stmt
				{ $$ = $1; }
			| unmatched_iteration_stmt
				{ $$ = $1; }
			;

matched_if_stmt 	: IF OPAREN simple_expression CPAREN matched ELSE matched
				{ TreeNode * t = newStmtNode(IfK);
				t->lineno = $1.lineno;
				t->attr.name = $1.rawString;
				t->child[0] = $3;
				t->child[1] = $5;
				t->child[2] = $7;
				$$ = t;
				}
			| IF OPAREN error CPAREN matched ELSE matched { $$ = NULL; }
			| error { $$ = NULL; }
			;

unmatched_if_stmt 	: IF OPAREN simple_expression CPAREN statement
				{ TreeNode * t = newStmtNode(IfK);
				t->lineno = $1.lineno;
				t->attr.name = $1.rawString;
				t->child[0] = $3;
				t->child[1] = $5;
				$$ = t;
				}
			| IF OPAREN simple_expression CPAREN matched ELSE unmatched 
				{ TreeNode * t = newStmtNode(IfK);
				t->lineno = $1.lineno;
				t->attr.name = $1.rawString;
				t->child[0] = $3;
				t->child[1] = $5;
				t->child[2] = $7;
				$$ = t;
				}
			| IF OPAREN error CPAREN matched ELSE unmatched { $$ = NULL; }
			| IF OPAREN error CPAREN statement { $$ = NULL; }
			;


compound_stmt		: OBRACE local_declarations statement_list CBRACE 
				{ yyerrok;
				TreeNode * t = newStmtNode(CompK);
				t->lineno = $1.lineno;
				t->child[0] = $2;
				t->child[1] = $3;
				$$ = t;
				}
			| OBRACE local_declarations error CBRACE { yyerrok; $$ = NULL; }
			| OBRACE error statement_list CBRACE { $$ = NULL; }
			;

local_declarations	: local_declarations scoped_var_declaration 
				{ 
				TreeNode * t = $1;
				if ( t != NULL) 
				{ while ( t->sibling != NULL)
					t = t->sibling;
				t->sibling = $2;
				$$ = $1; }
				else $$ = $2;
				}
			| { $$ = NULL; }
			;

statement_list		: statement_list statement 
				{ 
				TreeNode * t = $1;
				if (t != NULL)
				{ while ( t->sibling != NULL)
					t = t->sibling;
				t->sibling = $2;
				$$ = $1; }
				else $$ = $2;
				}
			| { $$ = NULL; }
			;

expression_stmt		: expression SCOLON
				{ $$ = $1; yyerrok; }
			| SCOLON
				{ $$ = NULL; yyerrok; }
			| error SCOLON { yyerrok; $$ = NULL; }
			;

matched_iteration_stmt	: WHILE OPAREN simple_expression CPAREN matched
				{ TreeNode * t = newStmtNode(WhileK);
				t->lineno = $1.lineno;
				t->attr.name = $1.rawString;
				t->child[0] = $3;
				t->child[1] = $5;
				$$ = t;
				}
			| FOREACH OPAREN mutable IN simple_expression CPAREN matched
				{ TreeNode * t = newStmtNode(ForeachK);
				t->lineno = $1.lineno;
				t->attr.name = $1.rawString;
				t->child[0] = $3;
				t->child[1] = $5;
				t->child[2] = $7;
				$$ = t;
				}
			| WHILE OPAREN error CPAREN matched { $$ = NULL; }
			| FOREACH OPAREN error CPAREN matched { $$ = NULL; }
			;
				

unmatched_iteration_stmt: WHILE OPAREN simple_expression CPAREN unmatched
				{ TreeNode * t = newStmtNode(WhileK);
				t->lineno = $1.lineno;
				t->attr.name = $1.rawString;
				t->child[0] = $3;
				t->child[1] = $5;
				$$ = t;
				}
			| FOREACH OPAREN mutable IN simple_expression CPAREN unmatched
				{ TreeNode * t = newStmtNode(ForeachK);
				t->lineno = $1.lineno;
				t->attr.name = $1.rawString;
				t->child[0] = $3;
				t->child[1] = $5;
				t->child[2] = $7;
				$$ = t;
				}
			| WHILE OPAREN error CPAREN unmatched { $$ = NULL; }
			| FOREACH OPAREN error CPAREN unmatched { $$ = NULL; }
			;

return_stmt		: RETURN SCOLON
				{ yyerrok;
				 TreeNode * t = newStmtNode(ReturnK);
				t->attr.name = $1.rawString;
				t->lineno = $1.lineno;
				$$ = t; 
				}
			| RETURN expression SCOLON
				{ yyerrok;
				TreeNode * t = newStmtNode(ReturnK);
				t->attr.name = $1.rawString;
				t->lineno = $1.lineno;
				t->child[0] = $2;
				$$ = t;
				}				
			;

break_stmt		: BREAK SCOLON
				{ yyerrok;
				 TreeNode * t = newStmtNode(BreakK);
				t->attr.name = $1.rawString;
				//t->lineno = $1.lineno;
				$$ = t;
				 }
			;

expression		: mutable binary_exp expression 
				{ TreeNode * t = newExpNode(AssignK);
				t->attr.name = $2.rawString;
				t->lineno = $2.lineno;
				t->child[0] = $1;
				t->child[1] = $3;
				$$ = t;
				}				
			| mutable unary_exp				
				{ TreeNode * t = newExpNode(AssignK);
				t->attr.name = $2.rawString;
				t->lineno = $2.lineno;
				t->child[0] = $1;
				$$ = t;
				}				
			| simple_expression { $$ = $1; }
			| error binary_exp { yyerrok; $$ = NULL; }
			| error unary_exp { yyerrok; $$ = NULL; }
			| mutable binary_exp error { $$ = NULL; }
			| error binary_exp error { $$ = NULL; }
			;

binary_exp		: EQUALS { $$ = $1; }
			| ADDASS { $$ = $1; }
			| SUBASS { $$ = $1; }
			| MULASS { $$ = $1; }
			| DIVASS { $$ = $1; }
			;

unary_exp		: INC { $$ = $1; }
			| DEC { $$ = $1; }
			;

simple_expression	: simple_expression OR and_expression				
				{ TreeNode * t = newExpNode(OpK);
				t->attr.name = $2.rawString;
				t->child[0] = $1;
				t->child[1] = $3;
				$$ = t;
				}
			| and_expression
				{ $$ = $1; }
			| error OR and_expression { yyerrok; $$ = NULL; }
			| error OR error { $$ = NULL; }
			| simple_expression OR error { $$ = NULL; }
			;

and_expression		: and_expression AND unary_rel_expression
				{ TreeNode * t = newExpNode(OpK);
				t->attr.name = $2.rawString;
				t->child[0] = $1;
				t->child[1] = $3;
				$$ = t;
				}
			| unary_rel_expression
				{ $$ = $1; }
			| error AND unary_rel_expression 
				{ yyerrok; $$ = NULL; }
			| error AND error 
				{ $$ = NULL; }
			| and_expression AND error 
				{ $$ = NULL; }
			;

unary_rel_expression	: NOT unary_rel_expression 
				{ TreeNode * t = newExpNode(OpK);
				t->attr.name = $1.rawString;
				t->child[0] = $2;
				$$ = t;
				}
			| rel_expression 
				{ $$ = $1; }
			| NOT error { $$ = NULL; }
			;

rel_expression		: sum_expression relop sum_expression
				{ TreeNode * t = newExpNode(OpK);
				t->attr.name = $2.rawString;
				t->child[0] = $1;
				t->child[1] = $3;
				$$ = t;
				}
			| sum_expression
				{ $$ = $1; }
			| error relop sum_expression { yyerrok; $$ = NULL; }
			| error relop error { $$ = NULL; }
			| sum_expression relop error { $$ = NULL; }
			;

relop			: LESSEQ
				{ $$ = $1; }
			| LESS
				{ $$ = $1; }
			| GREATER
				{ $$ = $1; }
			| GRTEQ
				{ $$ = $1; }
			| EQ
				{ $$ = $1; }
			| NOTEQ
				{ $$ = $1; }
			;

sum_expression		: sum_expression sumop term
				{ TreeNode * t = newExpNode(OpK);
				t->attr.name = $2.rawString;
				t->child[0] = $1;
				t->child[1] = $3;
				$$ = t;
				}
			| term 
				{ $$ = $1; }
			| error sumop term { yyerrok; $$ = NULL; }
			| error sumop error { $$ = NULL; }
			| sum_expression sumop error { $$ = NULL; }
			;

sumop			: PLUS
				{ $$ = $1;}
			| MINUS
				{ $$ = $1; }
			;

term			: term mulop unary_expression
				{ TreeNode * t = newExpNode(OpK);
				t->attr.name = $2.rawString;
				t->lineno = $2.lineno;
				t->child[0] = $1;
				t->child[1] = $3;
				$$ = t; 
				}
			| unary_expression 
				{ $$ = $1; }
			| error mulop unary_expression { yyerrok; $$ = NULL; }
			| error mulop error { $$ = NULL; }
			| term mulop error { $$ = NULL; }
			;

mulop			: MULT
				{ $$ = $1; }
			| DIV
				{ $$ = $1; }
			| MODULUS
				{ $$ = $1; }
			;

unary_expression	: unaryop unary_expression
				{ TreeNode * t = newExpNode(OpK);
				t->attr.name = $1.rawString;
				t->child[0] = $2;
				$$ = t;
				} 
			| factor
				{ $$ = $1; }
			| unaryop error { $$ = NULL; }
			;

unaryop			: MINUS
				{ $$ = $1; }
			| MULT
				{ $$ = $1; }
			| TERNARY
				{ $$ = $1; }
			;

factor			: immutable 
				{ $$ = $1; }
			| mutable
				{ $$ = $1; }
			;

mutable			: ID
				{ yyerrok;
				 TreeNode * t = newExpNode(IdK);
				t->attr.name = $1.rawString;
				$$ = t;
				}
			| ID OBRACKET expression CBRACKET
				{ yyerrok;
				TreeNode * t = newExpNode(IdK);
				t->attr.name = $1.rawString;
				t->child[0] = $3;
				$$ = t;
				}
			| error CBRACKET { yyerrok; $$ = NULL; }
			| ID OBRACKET error { $$ = NULL; }
			;

immutable		: OPAREN expression CPAREN
				{ yyerrok; $$ = $2; }
			| call
				{ $$ = $1; }
			| constant
				{ $$ = $1; }
			| OPAREN error { $$ = NULL; }
			;

call			: ID OPAREN args CPAREN
				{ yyerrok;
				TreeNode * t = newExpNode(CallK);
				t->attr.name = $1.rawString;
				t->child[0] = $3;
				$$ = t;
				}
			| ID OPAREN error { $$ = NULL; }
			;

args			: arg_list
				{ $$ = $1; }
			|
				{ $$ = NULL; }
			;

arg_list		: arg_list COMMA expression
				{ yyerrok; 
				TreeNode * t = $1;
					if (t != NULL) {
						while (t->sibling != NULL)
							t = t->sibling;
						t->sibling = $3;
						$$ = $1;
					}
					else $$ = $3;
				}
			| expression
				{ $$ = $1; }
			| error COMMA expression 
				{ yyerrok; $$ = NULL; }
			| arg_list COMMA error { $$ = NULL; }
			;

constant		: NUMCONST 
				{ yyerrok; 
				TreeNode * t = newExpNode(ConstK);
				//t->consttype = NUMC;
				t->type = Int;
				t->attr.iValue = $1.type.iValue;
				$$ = t; }
			| CHARCONST
				{ yyerrok;
				TreeNode * t = newExpNode(ConstK);
				//t->consttype = CHARC;
				t->type = Char;
				t->attr.cValue = $1.type.cValue;
				$$ = t; }
			| STRINGCONST
				{ yyerrok;
				TreeNode * t = newExpNode(ConstK);
				//t->consttype = STRINGC;
				t->type = Char;
				t->isArray = true;
				t->attr.sValue = $1.type.sValue;
				$$ = t; }
			| BOOLCONST
				{ yyerrok; 
				TreeNode * t = newExpNode(ConstK);
				//t->consttype = BOOLC;
				t->type = Bool;
				t->attr.iValue = $1.type.iValue;
				$$ = t; }
			;

%%

int lineno = 0;


int main(int argc, char** argv)
{   
  initTokenMaps();
  bool printT = false;
  bool printAnT = false;
  bool printMem = false;

  char* inFile = NULL;
  char* outFile = NULL;

  char c;
  while (optind < argc) {
	c = getopt(argc, argv, "dpPSo:");
	//printf("%c\n", c);
	if (c != -1) {
		switch (c) {
		case 'd': yydebug = 1; break;
		case 'p': printT = true; break;
		case 'P': printAnT = true; 
		case 'S': printMem = true; break;
		case 'o': outFile = optarg; break;
		default: break;;
		}
	} else {
		//printf("-%s\n", argv[optind]);
		inFile = argv[optind];
		optind++;	
	}
  }

  //printf("%s\n", inFile == NULL ? "null" : inFile);
  
  if (inFile == NULL)  
	return -1;

  //printf("%s\n", outFile != NULL ? outFile : "stdout");
  //printf("%s\n", inFile == NULL ? "null" : inFile);  
 
  if (argc > 1) {
    //FILE *file = fopen(argv[argc-1], "r");
    FILE *file = fopen(inFile, "r");
    if (file != NULL)
      yyin = file;
  }

  do {
	yyparse();
  } while (!feof(yyin));
/*
  if (outFile == NULL) {
	string s(inFile);
	s = s.substr(0, s.length()-2);
	s += "tm";
	outFile = (char*)s.c_str();
  } else if (strcmp(outFile, "-") == 0) {
	outFile = NULL;
  } */
  //savedTree = CreateIOFunctions(savedTree);

  if (printT && numErrors == 0)
  	printTree(savedTree, -1, 0);


  if (numErrors == 0 && numWarnings == 0) {
        savedTree = CreateIOFunctions(savedTree);
        startScopeAndType(savedTree, numErrors, numWarnings); 
  }
  if ((printAnT || printMem) && numErrors == 0 && numWarnings == 0) {
	printAnTree(savedTree, -1, 0, printMem);
  }

  if (printMem)
	printf("Offset for end of global space: %i\n", globalOffset); 

  if (numErrors == 0) {
	generateCode(inFile, outFile, savedTree, GetSymbolTable());
  }

  printf("Number of warnings: %i\n", numWarnings);
  printf("Number of errors: %i\n", numErrors);

   return 0;
}


