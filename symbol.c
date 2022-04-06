#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tree.h"

typedef struct Type_* Type;
typedef struct FieldList_* FieldList;
typedef struct FuncList_* FuncList;
typedef struct SymEntry_* SymEntry;

struct Type_ {
    enum {BASIC, ARRAY, STRUCTURE, FUNCTION} kind;
    union {
        int basic;
        struct {
            Type elem;
            int size;
        } array;
        FieldList structure;
        FuncList function;
    } detail;
};

struct FieldList_ {
    char *name;
    Type type;
    FieldList next;
};

struct FuncList_ {
    Type ret;
    FieldList args;
};

struct SymEntry_ {
    char *name;
    Type type;
    SymEntry next;
};

// the first block is a root symbol
SymEntry symtab;
SymEntry tail;

void init_symtab() {
    symtab = malloc(sizeof(struct SymEntry_));
    symtab->next = NULL;
    tail = symtab;
}

// return the type if exist, else NULL
Type lookup_symtab(char *symbol_name) {
    SymEntry entry;
    // the first valid entry is symtab->next
    for (entry = symtab->next; entry != NULL; entry = entry->next) {
        if (!strcmp(entry->name, symbol_name)) {
            return entry->type;
        }
    }
    return NULL;
}

// not including the lookup process
void append_symtab(char *symbol_name, Type type) {
    tail->next = malloc(sizeof(struct SymEntry_));
    tail = tail->next;
    tail->next = NULL;
    
    tail->name = symbol_name;
    tail->type = type;
    return;
}

tree *glb_node;

void Args();
void Exp();
void Dec();
void DecList();
void Def();
void DefList();
void Stmt();
void StmtList();
void CompSt();
void ParamDec();
void VarList();
void FunDec();
void VarDec();
void Tag();
void OptTag();
void StructSpecifier();
void Specifier();
void ExtDecList();
void ExtDef();
void ExtDefList();
void Program();


void Args() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("Args is empty.");
        return;
    } else if (son_cnt == 1) {
        if (!strcmp(cur_node->son[0]->tag, "Exp")) {

            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;

            return;
        }
    } else if (son_cnt == 3) {
        if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "COMMA") &&
            !strcmp(cur_node->son[2]->tag, "Args") 
        ) {

            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // COMMA
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Args();
            glb_node = cur_node;

            return;        
        }
    }
    printf("%d ", cur_node->lineno);
    perror("Args error");
    return;
}

void Exp() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("Exp is empty.");
        return;
    } else if (son_cnt == 1) {
        if (!strcmp(cur_node->son[0]->tag, "ID")) {

            glb_node = glb_node->son[0];
            // ID
            glb_node = cur_node;

            return;

        } else if (!strcmp(cur_node->son[0]->tag, "INT")) {

            glb_node = glb_node->son[0];
            // INT
            glb_node = cur_node;

            return;

        } else if (!strcmp(cur_node->son[0]->tag, "FLOAT")) {

            glb_node = glb_node->son[0];
            // FLOAT
            glb_node = cur_node;

            return;
        }
    } else if (son_cnt == 2) {
        if (
            !strcmp(cur_node->son[0]->tag, "MINUS") &&
            !strcmp(cur_node->son[1]->tag, "Exp") 
        ) {
            
            glb_node = glb_node->son[0];
            // MINUS
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            Exp();
            glb_node = cur_node;

            return;
        }
        else if (
            !strcmp(cur_node->son[0]->tag, "NOT") &&
            !strcmp(cur_node->son[1]->tag, "Exp") 
        ) {
            
            glb_node = glb_node->son[0];
            // NOT
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            Exp();
            glb_node = cur_node;

            return;
        }
    } else if (son_cnt == 3) {
        if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "ASSIGNOP") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // ASSIGNOP
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            return;
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "AND") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {
            
            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // AND
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            return;
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "OR") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // OR
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            return;
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "RELOP") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // RELOP
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            return;
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "PLUS") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // PLUS
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            return;
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "MINUS") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // MINUS
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            return;
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "STAR") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // STAR
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            return;
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "DIV") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // DIV
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            return;
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "DOT") &&
            !strcmp(cur_node->son[2]->tag, "ID")
        ) {

            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // DOT
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            // ID
            glb_node = cur_node;

            return;
        } else if (
            !strcmp(cur_node->son[0]->tag, "LP") &&
            !strcmp(cur_node->son[1]->tag, "Exp") &&
            !strcmp(cur_node->son[2]->tag, "RP")
        ) {

            glb_node = glb_node->son[0];
            // LP
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            Exp();
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            // RP
            glb_node = cur_node;

            return;
        }

    } else if (son_cnt == 4) {
        if (
            !strcmp(cur_node->son[0]->tag, "ID") &&
            !strcmp(cur_node->son[1]->tag, "LP") &&
            !strcmp(cur_node->son[2]->tag, "Args") &&
            !strcmp(cur_node->son[3]->tag, "RP")
        ) {

            glb_node = glb_node->son[0];
            // ID
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // LP
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Args();
            glb_node = cur_node;

            glb_node = glb_node->son[3];
            // RP
            glb_node = cur_node;

            return;
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "LB") &&
            !strcmp(cur_node->son[2]->tag, "Exp") &&
            !strcmp(cur_node->son[3]->tag, "RB")
        ) {
            
            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // LB
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            glb_node = glb_node->son[3];
            // RB
            glb_node = cur_node;

            return;
        }
    }
    printf("%d ", cur_node->lineno);
    perror("Exp error");
    return;
}

void Dec() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("Dec is empty.");
        return;
    } else if (son_cnt == 1) {
        if (!strcmp(cur_node->son[0]->tag, "VarDec")) {

            glb_node = glb_node->son[0];
            VarDec();
            glb_node = cur_node;

            return;
        }
    } else if (son_cnt == 3) {
        if (
            !strcmp(cur_node->son[0]->tag, "VarDec") &&
            !strcmp(cur_node->son[1]->tag, "ASSIGNOP") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            VarDec();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // ASSIGNOP
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            return;
        } 
    }
    printf("%d ", cur_node->lineno);
    perror("Dec error");
    return;
}

void DecList() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;
    if (son_cnt == -1) {
        // empty
        perror("DecList is empty.");
        return;
    } else if (son_cnt == 1) {
        if (
            !strcmp(cur_node->son[0]->tag, "Dec")
        ) {

            glb_node = glb_node->son[0];
            Dec();
            glb_node = cur_node;
            
            return;
        }
    } else if (son_cnt == 3) {
        if (
            !strcmp(cur_node->son[0]->tag, "Dec") &&
            !strcmp(cur_node->son[1]->tag, "COMMA") &&
            !strcmp(cur_node->son[2]->tag, "DecList")
        ) {

            glb_node = glb_node->son[0];
            Dec();
            glb_node = cur_node;
            
            glb_node = glb_node->son[1];
            // COMMA
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            DecList();
            glb_node = cur_node;

            return;
        }
    }
    printf("%d ", cur_node->lineno);
    perror("DecList error");
    return;
}

void Def() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("Def is empty.");
        return;
    } else if (son_cnt == 3) {
        if (
            !strcmp(cur_node->son[0]->tag, "Specifier") &&
            !strcmp(cur_node->son[1]->tag, "DecList") &&
            !strcmp(cur_node->son[2]->tag, "SEMI")
        ) {

            glb_node = glb_node->son[0];
            Specifier();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            DecList();
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            // SEMI
            glb_node = cur_node;

            return;
        }
    }
    printf("%d ", cur_node->lineno);
    perror("Def error");
    return;
}

void DefList() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        return;
    } else if (son_cnt == 2) {
        if (
            !strcmp(cur_node->son[0]->tag, "Def") &&
            !strcmp(cur_node->son[1]->tag, "DefList")
        ) {

            glb_node = glb_node->son[0];
            Def();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            DefList();
            glb_node = cur_node;

            return;
        }
    }
    printf("%d ", cur_node->lineno);
    perror("DefList error");
    return;
}

void Stmt() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("Stmt is empty.");
        return;
    } else if (son_cnt == 1) {
        if (
            !strcmp(cur_node->son[0]->tag, "CompSt")
        ) {

            glb_node = glb_node->son[0];
            CompSt();
            glb_node = cur_node;

            return;
        }
    } else if (son_cnt == 2) {
        if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "SEMI")
        ) {
            
            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // SEMI
            glb_node = cur_node;

            return;
        }
    } else if (son_cnt == 3) {
        if (
            !strcmp(cur_node->son[0]->tag, "RETURN") &&
            !strcmp(cur_node->son[1]->tag, "Exp") &&
            !strcmp(cur_node->son[2]->tag, "SEMI")
        ) {

            glb_node = glb_node->son[0];
            // RETURN
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            Exp();
            glb_node = cur_node;
            
            glb_node = glb_node->son[2];
            // SEMI
            glb_node = cur_node;

            return;
        }
    } else if (son_cnt == 5) {
        if (
            !strcmp(cur_node->son[0]->tag, "IF") &&
            !strcmp(cur_node->son[1]->tag, "LP") &&
            !strcmp(cur_node->son[2]->tag, "Exp") &&
            !strcmp(cur_node->son[3]->tag, "RP") &&
            !strcmp(cur_node->son[4]->tag, "Stmt")
        ) {

            glb_node = glb_node->son[0];
            // IF
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // LP
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            glb_node = glb_node->son[3];
            // RP
            glb_node = cur_node;

            glb_node = glb_node->son[4];
            Stmt();
            glb_node = cur_node;

            return;
        } else if (
            !strcmp(cur_node->son[0]->tag, "WHILE") &&
            !strcmp(cur_node->son[1]->tag, "LP") &&
            !strcmp(cur_node->son[2]->tag, "Exp") &&
            !strcmp(cur_node->son[3]->tag, "RP") &&
            !strcmp(cur_node->son[4]->tag, "Stmt")
        ) {

            glb_node = glb_node->son[0];
            // WHILE
            glb_node = cur_node;
            
            glb_node = glb_node->son[1];
            // LP
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            glb_node = glb_node->son[3];
            // RP
            glb_node = cur_node;

            glb_node = glb_node->son[4];
            Stmt();
            glb_node = cur_node;

            return;
        }
    } else if (son_cnt == 7) {
        if (
            !strcmp(cur_node->son[0]->tag, "IF") &&
            !strcmp(cur_node->son[1]->tag, "LP") &&
            !strcmp(cur_node->son[2]->tag, "Exp") &&
            !strcmp(cur_node->son[3]->tag, "RP") &&
            !strcmp(cur_node->son[4]->tag, "Stmt") &&
            !strcmp(cur_node->son[5]->tag, "ELSE") &&
            !strcmp(cur_node->son[6]->tag, "Stmt")
        ) {

            glb_node = glb_node->son[0];
            // IF
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // LP
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            glb_node = glb_node->son[3];
            // RP
            glb_node = cur_node;

            glb_node = glb_node->son[4];
            Stmt();
            glb_node = cur_node;

            glb_node = glb_node->son[5];
            // ELSE
            glb_node = cur_node;

            glb_node = glb_node->son[6];
            Stmt();
            glb_node = cur_node;

            return;
        }
    }
    printf("%d ", cur_node->lineno);
    perror("Stmt error");
    return;
}

void StmtList() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        return;
    } else if (son_cnt == 2) {
        if (
            !strcmp(cur_node->son[0]->tag, "Stmt") &&
            !strcmp(cur_node->son[1]->tag, "StmtList")
        ) {

            glb_node = glb_node->son[0];
            Stmt();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            StmtList();
            glb_node = cur_node;

            return;
        }
    }
    printf("%d ", cur_node->lineno);
    perror("StmtList error");
    return;
}

void CompSt() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("CompSt is empty.");
        return;
    } else if (son_cnt == 4) {
        if (
            !strcmp(cur_node->son[0]->tag, "LC") &&
            !strcmp(cur_node->son[1]->tag, "DefList") &&
            !strcmp(cur_node->son[2]->tag, "StmtList") &&
            !strcmp(cur_node->son[3]->tag, "RC")
        ) {

            glb_node = glb_node->son[0];
            // LC
            glb_node = cur_node;
            
            glb_node = glb_node->son[1];
            DefList();
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            StmtList();
            glb_node = cur_node;

            glb_node = glb_node->son[3];
            // RC
            glb_node = cur_node;

            return;
        }
    }
    printf("%d ", cur_node->lineno);
    perror("CompSt error");
    return;
}

void ParamDec() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("ParamDec is empty.");
        return;
    } else if (son_cnt == 2) {
        if (
            !strcmp(cur_node->son[0]->tag, "Specifier") &&
            !strcmp(cur_node->son[1]->tag, "VarDec")
        ) {

            glb_node = glb_node->son[0];
            Specifier();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            VarDec();
            glb_node = cur_node;

            return;
        }
    }
    printf("%d ", cur_node->lineno);
    perror("ParamDec error");
    return;
}

void VarList() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("VarList is empty.");
        return;
    } else if (son_cnt == 1) {
        if (
            !strcmp(cur_node->son[0]->tag, "ParamDec")
        ) {

            glb_node = glb_node->son[0];
            ParamDec();
            glb_node = cur_node;

            return;
        }
    } else if (son_cnt == 3) {
        if (
            !strcmp(cur_node->son[0]->tag, "ParamDec") &&
            !strcmp(cur_node->son[1]->tag, "COMMA") &&
            !strcmp(cur_node->son[2]->tag, "VarList")
        ) {

            glb_node = glb_node->son[0];
            ParamDec();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // COMMA
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            VarList();
            glb_node = cur_node;

            return;
        }
    } 
    printf("%d ", cur_node->lineno);
    perror("VarList error");
    return;
}

void FunDec() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("FunDec is empty.");
        return;
    } else if (son_cnt == 3) {
        if (
            !strcmp(cur_node->son[0]->tag, "ID") &&
            !strcmp(cur_node->son[1]->tag, "LP") &&
            !strcmp(cur_node->son[2]->tag, "RP")
        ) {

            glb_node = glb_node->son[0];
            // ID
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // LP
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            // RP
            glb_node = cur_node;

            return;
        }
    } else if (son_cnt == 4) {
        if (
            !strcmp(cur_node->son[0]->tag, "ID") &&
            !strcmp(cur_node->son[1]->tag, "LP") &&
            !strcmp(cur_node->son[2]->tag, "VarList") &&
            !strcmp(cur_node->son[3]->tag, "RP")
        ) {

            glb_node = glb_node->son[0];
            // ID
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // LP
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            VarList();
            glb_node = cur_node;

            glb_node = glb_node->son[3];
            // RP
            glb_node = cur_node;

            return;
        }
    }
    printf("%d ", cur_node->lineno);
    perror("FunDec error");
    return;
}

void VarDec() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("VarDec is empty.");
        return;
    } else if (son_cnt == 1) {
        if (
            !strcmp(cur_node->son[0]->tag, "ID")
        ) {

            glb_node = glb_node->son[0];
            // ID
            glb_node = cur_node;

            return;
        }
    } else if (son_cnt == 4) {
        if (
            !strcmp(cur_node->son[0]->tag, "VarDec") &&
            !strcmp(cur_node->son[1]->tag, "LB") &&
            !strcmp(cur_node->son[2]->tag, "INT") &&
            !strcmp(cur_node->son[3]->tag, "RB")
        ) {

            glb_node = glb_node->son[0];
            VarDec();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // LB
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            // INT
            glb_node = cur_node;

            glb_node = glb_node->son[3];
            // RB
            glb_node = cur_node;

            return;
        }
    } 
    printf("%d ", cur_node->lineno);
    perror("VarDec error");
    return;
}

void Tag() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("Program is empty.");
        return;
    } else if (son_cnt == 1) {
        if (
            !strcmp(cur_node->son[0]->tag, "ID")
        ) {

            glb_node = glb_node->son[0];
            // ID
            glb_node = cur_node;

            return;
        }
    }
    printf("%d ", cur_node->lineno);
    perror("Tag error");
    return;
}

void OptTag() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        return;
    } else if (son_cnt == 1) { 
        if (
            !strcmp(cur_node->son[0]->tag, "ID")
        ) {

            glb_node = glb_node->son[0];
            // ID
            glb_node = cur_node;

            return;
        }
    } 
    printf("%d ", cur_node->lineno);
    perror("OptTag error");
    return;
}

void StructSpecifier() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("StructSpecifier is empty.");
        return;
    } else if (son_cnt == 2) {
        if (
            !strcmp(cur_node->son[0]->tag, "STRUCT") &&
            !strcmp(cur_node->son[1]->tag, "Tag")
        ) {

            glb_node = glb_node->son[0];
            // STRUCT
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            Tag();
            glb_node = cur_node;

            return;
        }
    } else if (son_cnt == 5) {
        if (
            !strcmp(cur_node->son[0]->tag, "STRUCT") &&
            !strcmp(cur_node->son[1]->tag, "OptTag") &&
            !strcmp(cur_node->son[2]->tag, "LC") &&
            !strcmp(cur_node->son[3]->tag, "DefList") &&
            !strcmp(cur_node->son[4]->tag, "RC")
        ) {

            glb_node = glb_node->son[0];
            // STRUCT
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            OptTag();
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            // LC
            glb_node = cur_node;

            glb_node = glb_node->son[3];
            DefList();
            glb_node = cur_node;

            glb_node = glb_node->son[4];
            // RC
            glb_node = cur_node;

            return;
        }
    }
    printf("%d ", cur_node->lineno);
    perror("StructSpecifier error");
    return;
}

void Specifier() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("Program is empty.");
        return;
    } else if (son_cnt == 1) {
        if (
            !strcmp(cur_node->son[0]->tag, "TYPE")
        ) {

            glb_node = glb_node->son[0];
            // TYPE
            glb_node = cur_node;

            return;
        } else if (
            !strcmp(cur_node->son[0]->tag, "StructSpecifier")
        ) {
            
            glb_node = glb_node->son[0];
            StructSpecifier();
            glb_node = cur_node;

            return;
        }
    } 
    printf("%d ", cur_node->lineno);
    perror("Specifier error");
    return;
}

void ExtDecList() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("ExtDecList is empty.");
        return;
    } else if (son_cnt == 1) {
        if (
            !strcmp(cur_node->son[0]->tag, "VarDec")
        ) {

            glb_node = glb_node->son[0];
            VarDec();
            glb_node = cur_node;

            return;
        }
    } else if (son_cnt == 3) {
        if (
            !strcmp(cur_node->son[0]->tag, "VarDec") &&
            !strcmp(cur_node->son[1]->tag, "COMMA") &&
            !strcmp(cur_node->son[2]->tag, "ExtDecList")
        ) {

            glb_node = glb_node->son[0];
            VarDec();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // COMMA
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            ExtDecList();
            glb_node = cur_node;

            return;
        }
    }
    printf("%d ", cur_node->lineno);
    perror("ExtDecList error");
    return;
}

void ExtDef() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("ExtDef is empty.");
        return;
    } else if (son_cnt == 2) {
        if (
            !strcmp(cur_node->son[0]->tag, "Specifier") &&
            !strcmp(cur_node->son[1]->tag, "SEMI")
        ) {

            glb_node = glb_node->son[0];
            Specifier();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // SEMI
            glb_node = cur_node;

            return;
        }
    } else if (son_cnt == 3) {
        if (
            !strcmp(cur_node->son[0]->tag, "Specifier") &&
            !strcmp(cur_node->son[1]->tag, "ExtDecList") &&
            !strcmp(cur_node->son[2]->tag, "SEMI")
        ) {

            glb_node = glb_node->son[0];
            Specifier();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            ExtDecList();
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            // SEMI
            glb_node = cur_node;

            return;
        } else if (
            !strcmp(cur_node->son[0]->tag, "Specifier") &&
            !strcmp(cur_node->son[1]->tag, "FunDec") &&
            !strcmp(cur_node->son[2]->tag, "CompSt")
        ) {
            
            glb_node = glb_node->son[0];
            Specifier();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            FunDec();
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            CompSt();
            glb_node = cur_node;

            return;
        }
    } 
    printf("%d ", cur_node->lineno);
    perror("ExtDef error");
    return;
}

void ExtDefList() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        return;
    } else if (son_cnt == 2) {
        if (
            !strcmp(cur_node->son[0]->tag, "ExtDef") &&
            !strcmp(cur_node->son[1]->tag, "ExtDefList")
        ) {

            glb_node = glb_node->son[0];
            ExtDef();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            ExtDefList();
            glb_node = cur_node;

            return;
        }
    } 
    printf("%d ", cur_node->lineno);
    perror("ExtDefList error");
    return;
}

void Program() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("Program is empty.");
        return;
    } else if (son_cnt == 1) {
        if (
            !strcmp(cur_node->son[0]->tag, "ExtDefList")
        ) {

            glb_node = glb_node->son[0];
            ExtDefList();
            glb_node = cur_node;

            return;
        }
    } 
    printf("%d ", cur_node->lineno);
    perror("Program error");
    return;
}

void parse_AST(tree *root) {
    printf("=> parse_ast..\n");
    if (!root) {
        perror("root is NULL\n");
        return;
    }

    glb_node = root;
    Program();
    return;
}