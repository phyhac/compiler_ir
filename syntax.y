%{
    #include <stdio.h>
	#include "tree.h"
	#define YYSTYPE tree *

	int has_error = 0;
	int has_msg = 0;
	int err_lineno;
	char *err_msg;
	
	extern int yydebug;
	extern int yylex();
	extern void yyerror(const char* message);
	extern void parse_AST(tree *);
%}

%token INT FLOAT
%token LP RP LB RB LC RC DOT SEMI COMMA NOT
%token STAR DIV MINUS PLUS AND OR RELOP ASSIGNOP
%token RETURN IF ELSE WHILE
%token ID TYPE STRUCT

// %type <tree_node> Program ExtDefList ExtDef ExtDecList
// %type <tree_node> Specifier StructSpecifier OptTag Tag
// %type <tree_node> VarDec FunDec VarList ParamDec CompSt
// %type <tree_node> StmtList Stmt DefList Def DecList Dec
// %type <tree_node> Exp Args

%right ASSIGNOP
%left  LP RP LB RB DOT
%right STAR DIV
%left  MINUS
%left  PLUS
%left  RELOP
%left  AND
%left  OR
%right NOT

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%
/* syntax */

Program : ExtDefList { $$ = create_node("Program", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); if(has_error == 0) {/*show_tree($$, 0);*/ parse_AST($$);} }
;

ExtDefList : ExtDef ExtDefList { $$ = create_node("ExtDefList", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); }
	| /*empty*/ { $$ = create_node("ExtDefList", NO_VAL, &PUB_VALUE, @$.first_line); $$->son_cnt = -1;}
;

ExtDef : Specifier ExtDecList SEMI { $$ = create_node("ExtDef", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); }
	| Specifier SEMI { $$ = create_node("ExtDef", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); }
	| Specifier FunDec CompSt {$$ = create_node("ExtDef", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); }
	| error SEMI {$$ = NULL;}
;

ExtDecList : VarDec { $$ = create_node("ExtDecList", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); }
	| VarDec COMMA ExtDecList { $$ = create_node("ExtDecList", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); }
;

Specifier : TYPE {$$ = create_node("Specifier", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1);}
	| StructSpecifier { $$ = create_node("Specifier", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); }
;

StructSpecifier : STRUCT OptTag LC DefList RC { $$ = create_node("StructSpecifier", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); add_son($$, $4); add_son($$, $5); }
	| STRUCT Tag { $$ = create_node("StructSpecifier", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); }
;
OptTag : /* empty */ { $$ = create_node("OptTag", NO_VAL, &PUB_VALUE, @$.first_line); $$->son_cnt = -1;}
	| ID { $$ = create_node("OptTag", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); }
;

Tag : ID { $$ = create_node("Tag", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); }
;

VarDec : ID { $$ = create_node("VarDec", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); }
	| VarDec LB INT { err_lineno = @$.first_line; err_msg = "Missing \"]\""; has_msg = 1;} RB { has_msg = 0; $$ = create_node("VarDec", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); add_son($$, $5); }
	| error RB {$$ = NULL;}
;

FunDec : ID LP VarList RP { $$ = create_node("FunDec", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); add_son($$, $4); }
	| ID LP RP { $$ = create_node("FunDec", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); }
;

VarList : ParamDec COMMA VarList { $$ = create_node("VarList", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); }
	| ParamDec { $$ = create_node("VarList", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); }
;

ParamDec : Specifier VarDec {$$ = create_node("ParamDec", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); }
;

CompSt : LC DefList StmtList RC { $$ = create_node("CompSt", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); add_son($$, $4); }
	| error RC {$$ = NULL;}
;

StmtList : /* empty */ { $$ = create_node("StmtList", NO_VAL, &PUB_VALUE, @$.first_line); $$->son_cnt = -1;}
	| Stmt StmtList { $$ = create_node("StmtList", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); }
;

Stmt : Exp { err_lineno = @$.first_line; err_msg = "Missing \";\""; has_msg = 1;} SEMI { has_msg = 0; $$ = create_node("Stmt", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $3); }
	| CompSt { $$ = create_node("Stmt", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); }
	| RETURN Exp { err_lineno = @$.first_line; err_msg = "Missing \";\""; has_msg = 1;} SEMI { has_msg = 0; $$ = create_node("Stmt", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $4); }
	| IF LP Exp RP Stmt %prec LOWER_THAN_ELSE { $$ = create_node("Stmt", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); add_son($$, $4); add_son($$, $5); }
	| IF LP Exp RP Stmt ELSE Stmt { $$ = create_node("Stmt", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); add_son($$, $4); add_son($$, $5); add_son($$, $6); add_son($$, $7); }
	| WHILE LP Exp RP Stmt { $$ = create_node("Stmt", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); add_son($$, $4); add_son($$, $5); }
	| error SEMI { $$ = NULL;}
;

DefList : /* empty */ { $$ = create_node("DefList", NO_VAL, &PUB_VALUE, @$.first_line); $$->son_cnt = -1;}
	| Def DefList { $$ = create_node("DefList", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); }
;

Def : Specifier DecList  SEMI { has_msg = 0; $$ = create_node("Def", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); }
;

DecList : Dec { $$ = create_node("DecList", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); }
	| Dec COMMA DecList { $$ = create_node("DecList", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); }
;

Dec : VarDec { $$ = create_node("Dec", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); }
	| VarDec ASSIGNOP Exp { $$ = create_node("Dec", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); }
;

Exp : Exp ASSIGNOP Exp { $$ = create_node("Exp", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); }
	| Exp AND Exp { $$ = create_node("Exp", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); }
	| Exp OR Exp { $$ = create_node("Exp", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); }
	| Exp RELOP Exp { $$ = create_node("Exp", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); }
	| Exp PLUS Exp { $$ = create_node("Exp", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); }
	| Exp MINUS Exp { $$ = create_node("Exp", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); } 
	| Exp STAR Exp { $$ = create_node("Exp", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3);}
	| Exp DIV Exp { $$ = create_node("Exp", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); }
	| LP Exp RP { $$ = create_node("Exp", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); }
	| MINUS Exp { $$ = create_node("Exp", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); }
	| NOT Exp {$$ = create_node("Exp", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); }
	| ID LP Args RP { $$ = create_node("Exp", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); add_son($$, $4); }
	| Exp LB Exp { err_lineno = @$.first_line; err_msg = "Missing \"]\""; has_msg = 1;} RB { has_msg = 0; $$ = create_node("Exp", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); add_son($$, $5); }
	| Exp DOT ID { $$ = create_node("Exp", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); }
	| ID { $$ = create_node("Exp", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); }
	| INT { $$ = create_node("Exp", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); }
	| FLOAT { $$ = create_node("Exp", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); }
;

Args : Exp COMMA Args { $$ = create_node("Args", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); add_son($$, $2); add_son($$, $3); }
	| Exp { $$ = create_node("Args", NO_VAL, &PUB_VALUE, @$.first_line); add_son($$, $1); }
;

%%

#include "lex.yy.c"

extern int yylineno;
void yyerror(const char *msg) {
	has_error = 1;
	if (has_msg == 0) {
		printf("Error type B at Line %d: Syntax error.\n", yylineno);
	}
	else {
    	printf("Error type B at Line %d: %s.\n", err_lineno, err_msg);
		has_msg = 0;
	}
}

