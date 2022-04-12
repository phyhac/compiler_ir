#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tree.h"

typedef struct Type_* Type;
typedef struct FieldList_* FieldList;
typedef struct FuncList_* FuncList;

struct Type_ {
    enum {BASIC, ARRAY, STRUCTURE, FUNCTION} kind;
    union {
        enum {INT, FLOAT} basic;
        struct {
            Type elem;
            int size;
        } array;
        FieldList structure;
        FuncList function;
    } detail;
};

int is_basic(Type type) {
    return type->kind == BASIC;
}
int is_int(Type type) {
    return type->kind == BASIC && type->detail.basic == INT;
}
int is_array(Type type) {
    return type->kind == ARRAY;
}
int is_struct(Type type) {
    return type->kind == STRUCTURE;
}
int is_func(Type type) {
    return type->kind == FUNCTION;
}
int is_same_type(Type type1, Type type2) {
    return type1->kind == type2->kind 
        && ((type1->kind == BASIC) ? type1->detail.basic == type2->detail.basic : 1); 
}


struct FieldList_ {
    char *name;
    Type type;
    FieldList next;
};

struct FuncList_ {
    Type ret;
    FieldList args;
};

// the first block is a root symbol
FieldList symtab;
tree *glb_node;
Type cur_type;
char *sym_id;
Type const_type;
char *err_msg;
FieldList fun_args;

void syn_error(int err_no, char *err_msg) {
    printf("Error type %d in Line %d: %s.\n", err_no, glb_node->lineno, err_msg);
}

FieldList symtab_stack[10];
int symtab_type_stack[10];
int symtab_cnt;
int cur_symtab_type;
void push_symtab() {
    symtab_stack[symtab_cnt] = symtab;
    symtab = NULL;
    symtab_type_stack[symtab_cnt] = cur_symtab_type;
    cur_symtab_type = 0;
    symtab_cnt++;
}

void pop_symtab() {
    symtab_cnt--;
    symtab = symtab_stack[symtab_cnt];
    cur_symtab_type = symtab_type_stack[symtab_cnt];
}

void init_symtab() {
    symtab = NULL;
    symtab_cnt = 0;
    cur_symtab_type = 0;
    const_type = malloc(sizeof(struct Type_));
    err_msg = malloc(30);
}

char *type_string(Type type) {
    switch(type->kind) {
        case BASIC:
            switch (type->detail.basic) {
                case INT:
                {
                    char *string = malloc(4);
                    strcpy(string, "int");
                    return string;
                }
                case FLOAT:
                {
                    char *string = malloc(6);
                    strcpy(string, "float");
                    return string;
                }
            }
            break;
        case ARRAY:
            {
                char *subtype_str = type_string(type->detail.array.elem);
                int str_len = strlen(subtype_str);
                char *string = malloc(str_len + 20);
                sprintf(string, "array<%d, %s>", type->detail.array.size, subtype_str);
                free(subtype_str);
                return string;
            }
            break;
        case STRUCTURE:
            {
                FieldList partner;
                char *string = malloc(100);
                sprintf(string, "struct{");
                int bios = 7;
                for (partner = type->detail.structure; partner != NULL; partner = partner->next) {
                    char *subtype_str = type_string(partner->type);
                    bios += sprintf(string + bios, "%s: %s, ", partner->name, subtype_str);
                    free(subtype_str);
                }
                sprintf(string + bios - 2, "}");
                return string;
            }
            break;
        case FUNCTION:
            return "";
    }
    printf("kind not identified\n");
    exit(1);
}

void print_symtab(FieldList symtab) {
    FieldList entry;
    int i;
    printf("\n--[symtab]--\n");
    // the first valid entry is symtab->next
    for (i = 0, entry = symtab; entry != NULL; i++, entry = entry->next) {
        printf("[%d] %s: %s\n", i, entry->name, type_string(entry->type));
    }
    printf("entry cnt: %d\n", i);
}

int size_stack[10];
int size_cnt = 0;
void push_size(int size) {
    size_stack[size_cnt++] = size;
}
int pop_size() {
    return size_stack[--size_cnt];
}

Type array(int size, Type subtype) {
    Type tmp_type = malloc(sizeof(struct Type_));
    tmp_type->kind = ARRAY;
    tmp_type->detail.array.size = size;
    tmp_type->detail.array.elem = subtype;
    return tmp_type;
}


// return the entry if exist, else NULL
FieldList lookup(char *symbol_name) {
    FieldList entry;
    // the first valid entry is symtab->next
    for (entry = symtab; entry != NULL; entry = entry->next) {
        if (!strcmp(entry->name, symbol_name)) {
            return entry;
        }
    }
    return NULL;
}

// not including the lookup process
void enter(char *symbol_name) {
    while (size_cnt > 0) {
        int size = pop_size();
        cur_type = array(size, cur_type);
    }
    FieldList tmp = symtab;
    symtab = malloc(sizeof(struct FieldList_));
    symtab->next = tmp;
    symtab->name = symbol_name;
    symtab->type = cur_type;

    return;
}


void Args();

int has_id;
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

int var_is_exist;
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
        if (
            !strcmp(cur_node->son[0]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;

            if (!fun_args || !is_same_type(cur_type, fun_args->type)) {
                syn_error(9, "Function is not applicable for arguments");
                
            } else {
                fun_args = fun_args->next;
            }

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

            if (!fun_args || !is_same_type(cur_type, fun_args->type)) {
                syn_error(9, "Function is not applicable for arguments");
            }
            fun_args = fun_args->next;

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
        if (
            !strcmp(cur_node->son[0]->tag, "ID")
        ) {

            glb_node = glb_node->son[0];
            // ID
            {
                sym_id = glb_node->val.s;

                FieldList tmp = lookup(sym_id);
                if (!tmp) {
                    sprintf(err_msg, "Undefined variable \"%s\"", sym_id);
                    syn_error(1, err_msg);
                }
                else {
                    cur_type = tmp->type;
                    has_id = 1;
                }
            }
            glb_node = cur_node;

            return;

        } else if (!strcmp(cur_node->son[0]->tag, "INT")) {
            
            glb_node = glb_node->son[0];
            // INT
            glb_node = cur_node;
            
            cur_type = const_type;
            cur_type->kind = BASIC;
            cur_type->detail.basic = INT;

            return;
        } else if (!strcmp(cur_node->son[0]->tag, "FLOAT")) {

            glb_node = glb_node->son[0];
            // FLOAT
            glb_node = cur_node;

            cur_type = const_type;
            cur_type->kind = BASIC;
            cur_type->detail.basic = FLOAT;

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

            {
                if (!is_basic(cur_type)) {
                    syn_error(7, "Type mismatched for operands");
                }
            }

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

            {
                if (!is_int(cur_type)) {
                    syn_error(7, "Type mismatched for operands");
                }
            }

            return;
        }
    } else if (son_cnt == 3) {
        if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "ASSIGNOP") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {
            {
                has_id = 0;
            }
            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;

            if (!has_id) {
                syn_error(6, "The left-hand side of an assignment must be a variable");
            }
            Type left_type = cur_type;

            glb_node = glb_node->son[1];
            // ASSIGNOP
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            if (!is_same_type(left_type, cur_type)) {
                syn_error(5, "Type mismatched for assignment");
            }

            return;
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "AND") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {
            
            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;

            {
                if (!is_int(cur_type)) {
                    syn_error(7, "Type mismatched for operands");
                }
            }

            glb_node = glb_node->son[1];
            // AND
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            {
                if (!is_int(cur_type)) {
                    syn_error(7, "Type mismatched for operands");
                }
            }

            return;
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "OR") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;
            
            {
                if (!is_int(cur_type)) {
                    syn_error(7, "Type mismatched for operands");
                }
            }

            glb_node = glb_node->son[1];
            // OR
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            {
                if (!is_int(cur_type)) {
                    syn_error(7, "Type mismatched for operands");
                }
            }

            return;
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "RELOP") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;

            {
                if (!is_int(cur_type)) {
                    syn_error(7, "Type mismatched for operands");
                }
            }

            glb_node = glb_node->son[1];
            // RELOP
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            {
                if (!is_int(cur_type)) {
                    syn_error(7, "Type mismatched for operands");
                }
            }

            return;
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "PLUS") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;

            if (!is_basic(cur_type)) {
                syn_error(7, "Type mismatched for operands");
            }
            Type left_type = cur_type;

            glb_node = glb_node->son[1];
            // PLUS
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            {
                if (!is_same_type(left_type, cur_type)) {
                    syn_error(7, "Type mismatched for operands");
                }
            }

            return;
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "MINUS") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;

            
            if (!is_basic(cur_type)) {
                syn_error(7, "Type mismatched for operands");
            }
            Type left_type = cur_type;

            glb_node = glb_node->son[1];
            // MINUS
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            {
                if (!is_same_type(left_type, cur_type)) {
                    syn_error(7, "Type mismatched for operands");
                }
            }

            return;
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "STAR") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;

            
            if (!is_basic(cur_type)) {
                syn_error(7, "Type mismatched for operands");
            }
            Type left_type = cur_type;

            glb_node = glb_node->son[1];
            // STAR
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            {
                if (!is_same_type(left_type, cur_type)) {
                    syn_error(7, "Type mismatched for operands");
                }
            }

            return;
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "DIV") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;

            
            if (!is_basic(cur_type)) {
                syn_error(7, "Type mismatched for operands");
            }
            Type left_type = cur_type;

            glb_node = glb_node->son[1];
            // DIV
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            {
                if (!is_same_type(left_type, cur_type)) {
                    syn_error(7, "Type mismatched for operands");
                }
            }

            return;
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "DOT") &&
            !strcmp(cur_node->son[2]->tag, "ID")
        ) {

            glb_node = glb_node->son[0];
            Exp();
            glb_node = cur_node;

            {
                if (!is_struct(cur_type)) {
                    syn_error(13, "Illegal use of \".\"");
                    exit(0);
                }
            }

            glb_node = glb_node->son[1];
            // DOT
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            // ID
            sym_id = glb_node->val.s;
            glb_node = cur_node;

            {
                push_symtab();
                symtab = cur_type->detail.structure;
                cur_symtab_type = STRUCTURE;
                FieldList entry = lookup(sym_id);
                if (!entry) {
                    sprintf(err_msg, "Non-existent field \"%s\"", sym_id);
                    syn_error(14, err_msg);
                    exit(0);
                }
                cur_type = entry->type;
                pop_symtab();
            }

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
            sym_id = glb_node->val.s;
            glb_node = cur_node;


            FieldList tmp = lookup(sym_id);
            if (!tmp) {
                sprintf(err_msg, "Undefined function \"%s\"", sym_id);
                syn_error(2, err_msg);
                exit(0);
            } else if (!is_func(tmp->type)) {
                sprintf(err_msg, "\"%s\" is not a function", sym_id);
                syn_error(11, err_msg);
                exit(0);
            }
            fun_args = tmp->type->detail.function->args;


            glb_node = glb_node->son[1];
            // LP
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Args();
            glb_node = cur_node;

            glb_node = glb_node->son[3];
            // RP
            glb_node = cur_node;

            cur_type = tmp->type->detail.function->ret;

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

            if (!is_array(cur_type)) {
                sprintf(err_msg, "\"%s\" is not an array", sym_id);
                syn_error(10, err_msg);
            }
            Type arr_type = cur_type;

            glb_node = glb_node->son[1];
            // LB
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            {
                if (!is_int(cur_type)) {
                    syn_error(12, "Index is not an integer");
                }
                cur_type = arr_type->detail.array.elem;
            }

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

            Type left_type = cur_type;

            glb_node = glb_node->son[1];
            // ASSIGNOP
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            Exp();
            glb_node = cur_node;

            {
                if (!is_same_type(left_type, cur_type)) {
                    syn_error(5, "Type mismatched for assignment");
                }
            }

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
            
            {
                if (!var_is_exist) {
                    enter(sym_id);
                }
            }

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

            {
                if (!var_is_exist) {
                    enter(sym_id);
                }
            }
            
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

            FieldList tmp = symtab_stack[symtab_cnt-1];
            if (!is_same_type(cur_type, tmp->type->detail.function->ret)) {
                syn_error(8, "Type mismatched for return");
            }
            
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

            if (!var_is_exist) {
                enter(sym_id);
            }
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
            FuncList func = malloc(sizeof(struct FuncList_));
            func->ret = cur_type;
            func->args = NULL; // get the global variables.

            glb_node = glb_node->son[0];
            // ID
            sym_id = glb_node->val.s;
            glb_node = cur_node;

            FieldList tmp = lookup(sym_id);
            if (tmp) {
                sprintf(err_msg, "Redefined function \"%s\"", sym_id);
                syn_error(4, err_msg);
                exit(0);
            } else {
                cur_type = malloc(sizeof(struct Type_));
                cur_type->kind = FUNCTION;
                cur_type->detail.function = func;
                enter(sym_id);
                tmp = symtab;

                push_symtab(); // record the function field.
                symtab = tmp;
                cur_symtab_type = FUNCTION;

                func->args = symtab; // point to itself because of no args.
            }

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
            FuncList func = malloc(sizeof(struct FuncList_));
            func->ret = cur_type;
            func->args = NULL;

            glb_node = glb_node->son[0];
            // ID
            sym_id = glb_node->val.s;
            glb_node = cur_node;

            FieldList tmp = lookup(sym_id);
            if (tmp) {
                sprintf(err_msg, "Redefined function \"%s\"", sym_id);
                syn_error(4, err_msg);
            } else {
                cur_type = malloc(sizeof(struct Type_));
                cur_type->kind = FUNCTION;
                cur_type->detail.function = func;
                enter(sym_id);
                tmp = symtab;

                push_symtab(); // record the function field.
                symtab = tmp;
                cur_symtab_type = FUNCTION;
            }

            glb_node = glb_node->son[1];
            // LP
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            VarList();
            glb_node = cur_node;

            // record the args
            func->args = symtab;

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
            // id=ID.val
            {
                var_is_exist = 0;
                sym_id = glb_node->val.s;
                if (lookup(sym_id)) {
                    var_is_exist = 1;
                    if (symtab_cnt == 0 || cur_symtab_type == FUNCTION) {
                        sprintf(err_msg, "Redefined variable \"%s\"", sym_id);
                        syn_error(3, err_msg);
                    } else if (cur_symtab_type == STRUCTURE) {
                        sprintf(err_msg, "Redefined field \"%s\"", sym_id);
                        syn_error(15, err_msg);
                    }
                }
            }
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
            int size = glb_node->val.i;
            glb_node = cur_node;

            glb_node = glb_node->son[3];
            // RB
            glb_node = cur_node;

            {
                push_size(size);
            }
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
            sym_id = glb_node->val.s;
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
        sym_id = NULL;
        return;
    } else if (son_cnt == 1) { 
        if (
            !strcmp(cur_node->son[0]->tag, "ID")
        ) {

            glb_node = glb_node->son[0];
            // ID
            sym_id = glb_node->val.s;
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
            {
                FieldList en = lookup(sym_id);
                if (!en) {
                    syn_error(17, "Undeclare structure");
                }
                else {
                    cur_type = en->type;
                }
            }
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

            char *tag_name = sym_id;

            glb_node = glb_node->son[2];
            // LC
            glb_node = cur_node;

            {
                push_symtab();
                cur_symtab_type = STRUCTURE;
            }

            glb_node = glb_node->son[3];
            DefList();
            glb_node = cur_node;

            glb_node = glb_node->son[4];
            // RC
            glb_node = cur_node;
            
            {
                cur_type = malloc(sizeof(struct Type_));
                cur_type->kind = STRUCTURE;
                cur_type->detail.structure = symtab;

                pop_symtab();
                if (tag_name) {
                    if (lookup(tag_name)) {
                        sprintf(err_msg, "Duplicated name \"%s\"", tag_name);
                        syn_error(16, err_msg);
                    }
                    else {
                        enter(tag_name);
                    }
                }
            }

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
            {
                cur_type = malloc(sizeof(struct Type_));
                cur_type->kind = BASIC;
                char *type_name = glb_node->val.s;
                
                if (!strcmp(type_name, "int")) {
                    cur_type->detail.basic = INT;
                } else if (!strcmp(type_name, "float")) {
                    cur_type->detail.basic = FLOAT;
                } else {
                    printf("no such TYPE: %s\n", type_name);
                    exit(1);
                }
            }
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

            {
                if (!var_is_exist) {
                    enter(sym_id);
                }
            }

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
            
            // enter(id, t)
            {
                if (!var_is_exist) {
                    enter(sym_id);
                }
            }

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

            FieldList comp_symtab = symtab;
            pop_symtab();
            FuncList func = symtab->type->detail.function;
            FieldList lastarg = func->args;
            
            // delete local var
            FieldList tmp = comp_symtab;
            FieldList next;
            while (tmp != lastarg) {
                next = tmp->next;
                free(tmp);
                tmp = next;
            }

            // reverse args
            FieldList prev = NULL;
            while (tmp != symtab) {
                next = tmp->next;
                tmp->next = prev;
                prev = tmp;
                tmp = next;
            }
            func->args = prev;

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
    if (!root) {
        perror("root is NULL\n");
        return;
    }
    init_symtab();
    glb_node = root;
    Program();
    // print_symtab(symtab);
    
    return;
}