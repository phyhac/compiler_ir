%{
    #include <stdlib.h>
    #include <stdio.h>
    extern tree *create_node(char *, int, value *, int);
    extern int has_error;
    int yycolumn = 1;
    #define YY_USER_ACTION \
        yylloc.first_line = yylloc.last_line = yylineno; \
        yylloc.first_column = yycolumn; \
        yylloc.last_column = yycolumn + yyleng - 1; \
        yycolumn += yyleng;
%}

%option yylineno

dec_digit [0-9]
oct_digit [0-7]
hex_digit [0-9a-fA-F]

INT [+-]?(0|([1-9]{dec_digit}*)|(0[xX][1-9a-fA-F]{hex_digit}*)|(0[1-7]{oct_digit}*))
FLOAT [+-]?(({dec_digit}+\.{dec_digit}*)|(\.{dec_digit}+))([eE][+-]?{dec_digit}+)?
ID [_a-zA-Z][_a-zA-Z0-9]*
SEMI ";"
COMMA ","
ASSIGNOP "="
RELOP (>=)|(<=)|(==)|(!=)|>|<
PLUS "+"
MINUS "-"
STAR "*"
DIV "/"
AND &&
OR "||"
DOT "."
NOT "!"
TYPE (int)|(float)
LP "("
RP ")"
LB "["
RB "]"
LC "{"
RC "}"
STRUCT "struct"
RETURN "return"
IF "if"
ELSE "else"
WHILE "while"

%%

"//" {
    char c;
    do {
        c = input();
    } while (c != 0 && c != '\n');
}
"/*" {
    char c;
    char b = '/';
    do {
        c = input();
        if (b == '*' && c == '/')
            break;
        b = c;
    } while (c != 0);
}
{INT} { 
    if (yytext[0] == '0') {
        if (yytext[1] == 'x' || yytext[1] == 'X') {
            PUB_VALUE.i = strtol(yytext, NULL, 16);
        }
        else {
            PUB_VALUE.i = strtol(yytext, NULL, 8);
        }
    }
    else {
        // dec
        PUB_VALUE.i = atoi(yytext);
    }
    yylval = create_node("INT", INT_VAL, &PUB_VALUE, yylineno);
    return INT;
}
{FLOAT} { 
    PUB_VALUE.f = atof(yytext);
    yylval = create_node("FLOAT", FLOAT_VAL, &PUB_VALUE, yylineno);
    return FLOAT;
}
{SEMI} { 
    yylval = create_node("SEMI", NO_VAL, &PUB_VALUE, yylineno);
    return SEMI;
}
{COMMA} {
    yylval = create_node("COMMA", NO_VAL, &PUB_VALUE, yylineno);
    return COMMA;
}
{ASSIGNOP} { 
    yylval = create_node("ASSIGNOP", NO_VAL, &PUB_VALUE, yylineno);
    return ASSIGNOP;
}
{RELOP} { 
    PUB_VALUE.s = yytext;
    yylval = create_node("RELOP", STR_VAL, &PUB_VALUE, yylineno);
    return RELOP;
}
{PLUS} {  
    yylval = create_node("PLUS", NO_VAL, &PUB_VALUE, yylineno);
    return PLUS;
}
{MINUS} {  
    yylval = create_node("MINUS", NO_VAL, &PUB_VALUE, yylineno);
    return MINUS;
}
{STAR} { 
    yylval = create_node("STAR", NO_VAL, &PUB_VALUE, yylineno);
    return STAR;
}
{DIV} { 
    yylval = create_node("DIV", NO_VAL, &PUB_VALUE, yylineno);
    return DIV;
}
{AND} {  
    yylval = create_node("AND", NO_VAL, &PUB_VALUE, yylineno);
    return AND;
}
{OR} { 
    yylval = create_node("OR", NO_VAL, &PUB_VALUE, yylineno);
    return OR;
}
{DOT} { 
    yylval = create_node("DOT", NO_VAL, &PUB_VALUE, yylineno);
    return DOT;
}
{NOT} { 
    yylval = create_node("NOT", NO_VAL, &PUB_VALUE, yylineno);
    return NOT;
}
{TYPE} {  
    PUB_VALUE.s = yytext;
    yylval = create_node("TYPE", STR_VAL, &PUB_VALUE, yylineno);
    return TYPE;
}
{LP} {
    yylval = create_node("LP", NO_VAL, &PUB_VALUE, yylineno);
    return LP;
}
{RP} {
    yylval = create_node("RP", NO_VAL, &PUB_VALUE, yylineno);
    return RP;
}
{LB} { 
    yylval = create_node("LB", NO_VAL, &PUB_VALUE, yylineno);
    return LB;
}
{RB} { 
    yylval = create_node("RB", NO_VAL, &PUB_VALUE, yylineno);
    return RB;
}
{LC} {  
    yylval = create_node("LC", NO_VAL, &PUB_VALUE, yylineno);
    return LC;
}
{RC} { 
    yylval = create_node("RC", NO_VAL, &PUB_VALUE, yylineno);
    return RC;
}
{STRUCT} {  
    yylval = create_node("STRUCT", NO_VAL, &PUB_VALUE, yylineno);
    return STRUCT;
}
{RETURN} { 
    yylval = create_node("RETURN", NO_VAL, &PUB_VALUE, yylineno);
    return RETURN;
}
{IF} { 
    yylval = create_node("IF", NO_VAL, &PUB_VALUE, yylineno);
    return IF;
}
{ELSE} { 
    yylval = create_node("ELSE", NO_VAL, &PUB_VALUE, yylineno);
    return ELSE;
}
{WHILE} { 
    yylval = create_node("WHILE", NO_VAL, &PUB_VALUE, yylineno);
    return WHILE;
}
{ID} { 
    PUB_VALUE.s = yytext;
    yylval = create_node("ID", STR_VAL, &PUB_VALUE, yylineno);
    return ID;
}
"\n" { yycolumn = 1; }
" " {}
"\t" {}
. { has_error = 1; printf("Error type A at Line %d: Mysterious characters \"%s\".\n", yylineno, yytext); }

%%
