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
            int len;
        } array;
        FieldList structure;
        FuncList function;
    } detail;
    int size;  // actually size in bytes.
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
char *ret_var;
FieldList fun_args;
char *truelabel;
char *falselabel;

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
    truelabel = NULL;
    falselabel = NULL;
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
                sprintf(string, "array<%d, %s>", type->detail.array.len, subtype_str);
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

Type array(int len, Type subtype) {
    Type tmp_type = malloc(sizeof(struct Type_));
    tmp_type->kind = ARRAY;
    tmp_type->detail.array.len = len;
    tmp_type->detail.array.elem = subtype;
    tmp_type->size = subtype->size * len;
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

typedef struct CodeElem_ *CodeElem;
typedef struct InterCode_ *InterCode;

struct CodeElem_ {
    enum {
        TARGET,
        ARG1,
        ARG2,
    } type; 
    char *detail;
    CodeElem next;
};

struct InterCode_ {
    enum {
        ASSIGNOP,
        PLUS,
        D_MINUS,
        D_STAR,
        DIV,
        S_MINUS,
        P_STAR,  // pointer
        
        JMP,
        T_JMP,
        F_JMP,
        LABEL,

        FUNC,
        PARAM,
        RETURN,
        DEC,
        CALL,
        ARG,
    } type;
    InterCode prev;
    InterCode next;
    CodeElem content;
};

char *i2s(int val) {
    char *s = malloc(sizeof(10));
    sprintf(s, "#%d", val);
    return s;
}

char *ptr(char *val) {
    char *s = malloc(strlen(val));
    sprintf(s, "*%s", val);
    return s;
}

char *addr(char *val) {
    char *s = malloc(strlen(val));
    sprintf(s, "&%s", val);
    return s;
}

char *relop(char *left, char *relop, char *right) {
    char *s = malloc(sizeof(10));
    sprintf(s, "%s %s %s", left, relop, right);
    return s;
}

int temp_cnt = 1;
char *temp_var() {
    char *s = malloc(sizeof(5));
    sprintf(s, "t%d", temp_cnt++);
    return s;
}

int label_cnt = 1;
char *new_label() {
    char *s = malloc(sizeof(8));
    sprintf(s, "label%d", label_cnt++);
    return s;
}

InterCode code_ASSIGNOP(char *target, char *arg1) {
    InterCode code = malloc(sizeof(struct InterCode_));
    code->type = ASSIGNOP;
    code->prev = NULL;
    code->next = NULL;

    CodeElem elem = malloc(sizeof(struct CodeElem_));
    code->content = elem;
    elem->type = TARGET;
    elem->detail = target;

    elem->next = malloc(sizeof(struct CodeElem_));
    elem = elem->next;
    elem->next = NULL;
    elem->type = ARG1;
    elem->detail = arg1;

    return code;
}

InterCode code_PLUS(char *target, char *arg1, char *arg2) {
    InterCode code = malloc(sizeof(struct InterCode_));
    code->type = PLUS;
    code->prev = NULL;
    code->next = NULL;

    CodeElem elem = malloc(sizeof(struct CodeElem_));
    elem->next = NULL;
    code->content = elem;
    elem->type = TARGET;
    elem->detail = target;

    elem->next = malloc(sizeof(struct CodeElem_));
    elem = elem->next;
    elem->next = NULL;
    elem->type = ARG1;
    elem->detail = arg1;

    elem->next = malloc(sizeof(struct CodeElem_));
    elem = elem->next;
    elem->next = NULL;
    elem->type = ARG2;
    elem->detail = arg2;

    return code;
}

InterCode code_D_MINUS(char *target, char *arg1, char *arg2) {
    InterCode code = malloc(sizeof(struct InterCode_));
    code->type = D_MINUS;
    code->prev = NULL;
    code->next = NULL;

    CodeElem elem = malloc(sizeof(struct CodeElem_));
    elem->next = NULL;
    code->content = elem;
    elem->type = TARGET;
    elem->detail = target;

    elem->next = malloc(sizeof(struct CodeElem_));
    elem = elem->next;
    elem->next = NULL;
    elem->type = ARG1;
    elem->detail = arg1;

    elem->next = malloc(sizeof(struct CodeElem_));
    elem = elem->next;
    elem->next = NULL;
    elem->type = ARG2;
    elem->detail = arg2;

    return code;
}

InterCode code_D_STAR(char *target, char *arg1, char *arg2) {
    InterCode code = malloc(sizeof(struct InterCode_));
    code->type = D_STAR;
    code->prev = NULL;
    code->next = NULL;

    CodeElem elem = malloc(sizeof(struct CodeElem_));
    elem->next = NULL;
    code->content = elem;
    elem->type = TARGET;
    elem->detail = target;

    elem->next = malloc(sizeof(struct CodeElem_));
    elem = elem->next;
    elem->next = NULL;
    elem->type = ARG1;
    elem->detail = arg1;

    elem->next = malloc(sizeof(struct CodeElem_));
    elem = elem->next;
    elem->next = NULL;
    elem->type = ARG2;
    elem->detail = arg2;

    return code;
}

InterCode code_DIV(char *target, char *arg1, char *arg2) {
    InterCode code = malloc(sizeof(struct InterCode_));
    code->type = DIV;
    code->prev = NULL;
    code->next = NULL;

    CodeElem elem = malloc(sizeof(struct CodeElem_));
    elem->next = NULL;
    code->content = elem;
    elem->type = TARGET;
    elem->detail = target;

    elem->next = malloc(sizeof(struct CodeElem_));
    elem = elem->next;
    elem->next = NULL;
    elem->type = ARG1;
    elem->detail = arg1;

    elem->next = malloc(sizeof(struct CodeElem_));
    elem = elem->next;
    elem->next = NULL;
    elem->type = ARG2;
    elem->detail = arg2;

    return code;
}

InterCode code_S_MINUS(char *target, char *arg1) {
    InterCode code = malloc(sizeof(struct InterCode_));
    code->type = S_MINUS;
    code->prev = NULL;
    code->next = NULL;

    CodeElem elem = malloc(sizeof(struct CodeElem_));
    elem->next = NULL;
    code->content = elem;
    elem->type = TARGET;
    elem->detail = target;

    elem->next = malloc(sizeof(struct CodeElem_));
    elem = elem->next;
    elem->next = NULL;
    elem->type = ARG1;
    elem->detail = arg1;

    return code;
}

InterCode code_P_STAR(char *target, char *arg1) {
    InterCode code = malloc(sizeof(struct InterCode_));
    code->type = P_STAR;
    code->prev = NULL;
    code->next = NULL;

    CodeElem elem = malloc(sizeof(struct CodeElem_));
    elem->next = NULL;
    code->content = elem;
    elem->type = TARGET;
    elem->detail = target;

    elem->next = malloc(sizeof(struct CodeElem_));
    elem = elem->next;
    elem->next = NULL;
    elem->type = ARG1;
    elem->detail = arg1;

    return code;
}

InterCode code_JMP(char *target) {
    InterCode code = malloc(sizeof(struct InterCode_));
    code->type = JMP;
    code->prev = NULL;
    code->next = NULL;

    CodeElem elem = malloc(sizeof(struct CodeElem_));
    elem->next = NULL;
    code->content = elem;
    elem->type = TARGET;
    elem->detail = target;

    return code;
}

InterCode code_T_JMP(char *target, char *arg1) {
    InterCode code = malloc(sizeof(struct InterCode_));
    code->type = T_JMP;
    code->prev = NULL;
    code->next = NULL;

    CodeElem elem = malloc(sizeof(struct CodeElem_));
    elem->next = NULL;
    code->content = elem;
    elem->type = TARGET;
    elem->detail = target;

    elem->next = malloc(sizeof(struct CodeElem_));
    elem = elem->next;
    elem->next = NULL;
    elem->type = ARG1;
    elem->detail = arg1;

    return code;
}

InterCode code_F_JMP(char *target, char *arg1) {
    InterCode code = malloc(sizeof(struct InterCode_));
    code->type = F_JMP;
    code->prev = NULL;
    code->next = NULL;

    CodeElem elem = malloc(sizeof(struct CodeElem_));
    elem->next = NULL;
    code->content = elem;
    elem->type = TARGET;
    elem->detail = target;

    elem->next = malloc(sizeof(struct CodeElem_));
    elem = elem->next;
    elem->next = NULL;
    elem->type = ARG1;
    elem->detail = arg1;

    return code;
}

InterCode code_LABEL(char *target) {
    InterCode code = malloc(sizeof(struct InterCode_));
    code->type = LABEL;
    code->prev = NULL;
    code->next = NULL;

    CodeElem elem = malloc(sizeof(struct CodeElem_));
    elem->next = NULL;
    code->content = elem;
    elem->type = TARGET;
    elem->detail = target;

    return code;
}

InterCode code_FUNC(char *target) {
    InterCode code = malloc(sizeof(struct InterCode_));
    code->type = FUNC;
    code->prev = NULL;
    code->next = NULL;

    CodeElem elem = malloc(sizeof(struct CodeElem_));
    elem->next = NULL;
    code->content = elem;
    elem->type = TARGET;
    elem->detail = target;

    return code;
}

InterCode code_PARAM(char *target) {
    InterCode code = malloc(sizeof(struct InterCode_));
    code->type = PARAM;
    code->prev = NULL;
    code->next = NULL;

    CodeElem elem = malloc(sizeof(struct CodeElem_));
    elem->next = NULL;
    code->content = elem;
    elem->type = TARGET;
    elem->detail = target;

    return code;
}

InterCode code_RETURN(char *target) {
    InterCode code = malloc(sizeof(struct InterCode_));
    code->type = RETURN;
    code->prev = NULL;
    code->next = NULL;

    CodeElem elem = malloc(sizeof(struct CodeElem_));
    elem->next = NULL;
    code->content = elem;
    elem->type = TARGET;
    elem->detail = target;

    return code;
}

InterCode code_DEC(char *target, char *arg1) {
    InterCode code = malloc(sizeof(struct InterCode_));
    code->type = DEC;
    code->prev = NULL;
    code->next = NULL;

    CodeElem elem = malloc(sizeof(struct CodeElem_));
    elem->next = NULL;
    code->content = elem;
    elem->type = TARGET;
    elem->detail = target;

    elem->next = malloc(sizeof(struct CodeElem_));
    elem = elem->next;
    elem->next = NULL;
    elem->type = ARG1;
    elem->detail = arg1;

    return code;
}

InterCode code_CALL(char *target, char *arg1) {
    InterCode code = malloc(sizeof(struct InterCode_));
    code->type = CALL;
    code->prev = NULL;
    code->next = NULL;

    CodeElem elem = malloc(sizeof(struct CodeElem_));
    elem->next = NULL;
    code->content = elem;
    elem->type = TARGET;
    elem->detail = target;

    elem->next = malloc(sizeof(struct CodeElem_));
    elem = elem->next;
    elem->next = NULL;
    elem->type = ARG1;
    elem->detail = arg1;

    return code;
}

InterCode code_ARG(char *target) {
    InterCode code = malloc(sizeof(struct InterCode_));
    code->type = ARG;
    code->prev = NULL;
    code->next = NULL;

    CodeElem elem = malloc(sizeof(struct CodeElem_));
    elem->next = NULL;
    code->content = elem;
    elem->type = TARGET;
    elem->detail = target;

    return code;
}

void print_code(InterCode insts) {
    InterCode cur_inst;
    CodeElem cur_elem;
    char *target;
    char *arg1;
    char *arg2;
    // char *inst_str;
    for (cur_inst = insts; cur_inst; cur_inst = cur_inst->next) {
        target = NULL;
        arg1 = NULL;
        arg2 = NULL;
        // inst_str = malloc(sizeof(30));
        for (cur_elem = cur_inst->content; cur_elem; cur_elem = cur_elem->next) {
            switch (cur_elem->type) {
                case TARGET:
                    target = cur_elem->detail;
                break;
                case ARG1:
                    arg1 = cur_elem->detail;
                break;
                case ARG2:
                    arg2 = cur_elem->detail;
                break;
                default:
                    printf("[alert] print instruction error..\n");
                    exit(0);
            }
        }   
        switch (cur_inst->type) {
            case ASSIGNOP:
                printf("%s := %s\n", target, arg1);
            break;
            case PLUS:
                printf("%s := %s + %s\n", target, arg1, arg2);
            break;
            case D_MINUS:
                printf("%s := %s - %s\n", target, arg1, arg2);
            break;
            case D_STAR:
                printf("%s := %s * %s\n", target, arg1, arg2);
            break;
            case DIV:
                printf("%s := %s / %s\n", target, arg1, arg2);
            break;
            case S_MINUS:
                printf("%s := -%s\n", target, arg1);
            break;
            case P_STAR:
                printf("%s := *%s\n", target, arg1);
            break;
            case JMP:
                printf("GOTO %s\n", target);
            break;
            case T_JMP:
                printf("IF %s GOTO %s\n", arg1, target);
            break;
            case F_JMP:
                printf("IF FALSE %s GOTO %s\n", arg1, target);
            break;
            case LABEL:
                printf("LABEL %s :\n", target);
            break;
            case FUNC:
                printf("FUNCTION %s :\n", target);
            break;
            case PARAM:
                printf("PARAM %s\n", target);
            break;
            case RETURN:
                printf("RETURN %s\n", target);
            break;
            case DEC:
                printf("DEC %s %s\n", target, arg1);
            break;
            case CALL:
                printf("%s := CALL %s\n", target, arg1);
            break;
            case ARG:
                printf("ARG %s\n", target);
            break;
            default:
                printf("[alert] print instruction error..\n");
                exit(0);
        }
    }
}

InterCode conn_code(InterCode prv, InterCode nxt) {
    if (!prv)
        return nxt;
    else if (!nxt)
        return prv;
    
    InterCode end;
    for (end = prv; end->next; end = end->next) {
        ;
    }
    end->next = nxt;
    nxt->prev = end;
    return prv;
}

InterCode Args();

int has_id;
InterCode Exp();

InterCode Dec();

InterCode DecList();

InterCode Def();

InterCode DefList();

InterCode Stmt();

InterCode StmtList();

InterCode CompSt();

InterCode ParamDec();

InterCode VarList();

InterCode FunDec();

int var_is_exist;
InterCode VarDec();

InterCode Tag();

InterCode OptTag();

InterCode StructSpecifier();

InterCode Specifier();

InterCode ExtDecList();

InterCode ExtDef();

InterCode ExtDefList();

InterCode Program();


InterCode Args() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("Args is empty.");
        return NULL;
    } else if (son_cnt == 1) {
        if (
            !strcmp(cur_node->son[0]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            InterCode sub_code0 = Exp();
            char *sub_var0 = ret_var;
            glb_node = cur_node;

            if (!fun_args || !is_same_type(cur_type, fun_args->type)) {
                syn_error(9, "Function is not applicable for arguments");
                
            } else {
                fun_args = fun_args->next;
            }

            ret_var = NULL;
            return conn_code(
                sub_code0,
                code_ARG(sub_var0)
            );
        }
    } else if (son_cnt == 3) {
        if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "COMMA") &&
            !strcmp(cur_node->son[2]->tag, "Args") 
        ) {

            glb_node = glb_node->son[0];
            InterCode sub_code0 = Exp();
            char *sub_var0 = ret_var;
            glb_node = cur_node;

            if (!fun_args || !is_same_type(cur_type, fun_args->type)) {
                syn_error(9, "Function is not applicable for arguments");
            }
            fun_args = fun_args->next;

            glb_node = glb_node->son[1];
            // COMMA
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            InterCode sub_code2 = Args();
            char *sub_var2 = ret_var;
            glb_node = cur_node;

            return conn_code(
                sub_code0,
                sub_code2
            );
        }
    }
    printf("%d ", cur_node->lineno);
    perror("Args error");
    return NULL;
}

InterCode Exp() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("Exp is empty.");
        return NULL;
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
                    ret_var = sym_id;
                }
            }
            glb_node = cur_node;

            return NULL;

        } else if (!strcmp(cur_node->son[0]->tag, "INT")) {
            
            glb_node = glb_node->son[0];
            // INT
            ret_var = i2s(glb_node->val.i);
            glb_node = cur_node;
            
            cur_type = const_type;
            cur_type->kind = BASIC;
            cur_type->detail.basic = INT;
            cur_type->size = 4;
            
            return NULL;
        } else if (!strcmp(cur_node->son[0]->tag, "FLOAT")) {

            glb_node = glb_node->son[0];
            // FLOAT
            glb_node = cur_node;

            cur_type = const_type;
            cur_type->kind = BASIC;
            cur_type->detail.basic = FLOAT;
            cur_type->size = 8;

            return NULL;
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
            InterCode sub_code = Exp();
            glb_node = cur_node;

            {
                if (!is_basic(cur_type)) {
                    syn_error(7, "Type mismatched for operands");
                }
            }
            char *sub_var = ret_var;
            ret_var = temp_var();
            return conn_code(sub_code, code_S_MINUS(ret_var, sub_var));
        }
        else if (
            !strcmp(cur_node->son[0]->tag, "NOT") &&
            !strcmp(cur_node->son[1]->tag, "Exp") 
        ) {
            
            glb_node = glb_node->son[0];
            // NOT
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            InterCode sub_code1 = Exp();
            char *tmp = truelabel;
            truelabel = falselabel;
            falselabel = tmp;
            glb_node = cur_node;

            {
                if (!is_int(cur_type)) {
                    syn_error(7, "Type mismatched for operands");
                }
            }

            return sub_code1;
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
            InterCode sub_code0 = Exp();
            char *sub_var0 = ret_var;
            glb_node = cur_node;

            if (!has_id) {
                syn_error(6, "The left-hand side of an assignment must be a variable");
            }
            Type left_type = cur_type;

            glb_node = glb_node->son[1];
            // ASSIGNOP
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            InterCode sub_code2 = Exp();
            char *sub_var2 = ret_var;
            glb_node = cur_node;

            if (!is_same_type(left_type, cur_type)) {
                syn_error(5, "Type mismatched for assignment");
            }

            ret_var = NULL;
            return conn_code(
                conn_code(
                    sub_code0, 
                    sub_code2
                ),
                code_ASSIGNOP(sub_var0, sub_var2)    
            );
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "AND") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {
            
            glb_node = glb_node->son[0];
            InterCode sub_code0 = Exp();
            char *tmp_truelabel0 = truelabel;
            truelabel = NULL;
            // falselabel should flow over.
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
            InterCode sub_code2 = Exp();
            glb_node = cur_node;

            {
                if (!is_int(cur_type)) {
                    syn_error(7, "Type mismatched for operands");
                }
            }

            /*
            exp0
            label true0
            exp2
            */
            return conn_code(
                sub_code0,
                conn_code(
                    code_LABEL(tmp_truelabel0),
                    sub_code2
                )
            );
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "OR") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            InterCode sub_code0 = Exp();
            char *tmp_falselabel0 = falselabel;
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
            InterCode sub_code2 = Exp();
            glb_node = cur_node;

            {
                if (!is_int(cur_type)) {
                    syn_error(7, "Type mismatched for operands");
                }
            }

            /*
            exp0
            label true0
            exp2
            */
            return conn_code(
                sub_code0,
                conn_code(
                    code_LABEL(tmp_falselabel0),
                    sub_code2
                )
            );
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "RELOP") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            InterCode sub_code0 = Exp();
            char *sub_var0 = ret_var;
            glb_node = cur_node;

            {
                if (!is_int(cur_type)) {
                    syn_error(7, "Type mismatched for operands");
                }
            }

            glb_node = glb_node->son[1];
            // RELOP
            char *relop_s = glb_node->val.s;
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            InterCode sub_code2 = Exp();
            char *sub_var2 = ret_var;
            glb_node = cur_node;

            {
                if (!is_int(cur_type)) {
                    syn_error(7, "Type mismatched for operands");
                }
            }
            if (!truelabel)
                truelabel = new_label();
            if (!falselabel)
                falselabel = new_label();
            return conn_code(
                conn_code(
                    sub_code0,
                    sub_code2
                ),
                conn_code(
                    code_T_JMP(truelabel, relop(sub_var0, relop_s, sub_var2)),
                    code_JMP(falselabel)
                )
            );
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "PLUS") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            InterCode sub_code0 = Exp();
            char *sub_var0 = ret_var;
            glb_node = cur_node;

            if (!is_basic(cur_type)) {
                syn_error(7, "Type mismatched for operands");
            }
            Type left_type = cur_type;

            glb_node = glb_node->son[1];
            // PLUS
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            InterCode sub_code2 = Exp();
            char *sub_var2 = ret_var;
            glb_node = cur_node;

            {
                if (!is_same_type(left_type, cur_type)) {
                    syn_error(7, "Type mismatched for operands");
                }
            }

            ret_var = temp_var();
            return conn_code(
                conn_code(
                    sub_code0,
                    sub_code2
                ),
                code_PLUS(ret_var, sub_var0, sub_var2)
            );
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "MINUS") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            InterCode sub_code0 = Exp();
            char *sub_var0 = ret_var;
            glb_node = cur_node;

            
            if (!is_basic(cur_type)) {
                syn_error(7, "Type mismatched for operands");
            }
            Type left_type = cur_type;

            glb_node = glb_node->son[1];
            // MINUS
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            InterCode sub_code2 = Exp();
            char *sub_var2 = ret_var;
            glb_node = cur_node;

            {
                if (!is_same_type(left_type, cur_type)) {
                    syn_error(7, "Type mismatched for operands");
                }
            }

            ret_var = temp_var();
            return conn_code(
                conn_code(
                    sub_code0,
                    sub_code2    
                ),
                code_D_MINUS(ret_var, sub_var0, sub_var2)
            );
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "STAR") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            InterCode sub_code0 = Exp();
            char *sub_var0 = ret_var;
            glb_node = cur_node;

            
            if (!is_basic(cur_type)) {
                syn_error(7, "Type mismatched for operands");
            }
            Type left_type = cur_type;

            glb_node = glb_node->son[1];
            // STAR
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            InterCode sub_code2 = Exp();
            char *sub_var2 = ret_var;
            glb_node = cur_node;

            {
                if (!is_same_type(left_type, cur_type)) {
                    syn_error(7, "Type mismatched for operands");
                }
            }

            ret_var = temp_var();
            return conn_code(
                conn_code(
                    sub_code0,
                    sub_code2    
                ),
                code_D_STAR(ret_var, sub_var0, sub_var2)
            );
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "DIV") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            InterCode sub_code0 = Exp();
            char *sub_var0 = ret_var;
            glb_node = cur_node;

            
            if (!is_basic(cur_type)) {
                syn_error(7, "Type mismatched for operands");
            }
            Type left_type = cur_type;

            glb_node = glb_node->son[1];
            // DIV
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            InterCode sub_code2 = Exp();
            char *sub_var2 = ret_var;
            glb_node = cur_node;

            {
                if (!is_same_type(left_type, cur_type)) {
                    syn_error(7, "Type mismatched for operands");
                }
            }

            ret_var = temp_var();
            return conn_code(
                conn_code(
                    sub_code0,
                    sub_code2    
                ),
                code_DIV(ret_var, sub_var0, sub_var2)
            );
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

            return NULL;
        } else if (
            !strcmp(cur_node->son[0]->tag, "LP") &&
            !strcmp(cur_node->son[1]->tag, "Exp") &&
            !strcmp(cur_node->son[2]->tag, "RP")
        ) {

            glb_node = glb_node->son[0];
            // LP
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            InterCode sub_code1 = Exp();
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            // RP
            glb_node = cur_node;

            return sub_code1;
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

            char *func_name = sym_id;
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
            InterCode sub_code2 = Args();
            glb_node = cur_node;

            glb_node = glb_node->son[3];
            // RP
            glb_node = cur_node;

            cur_type = tmp->type->detail.function->ret;

            ret_var = temp_var();
            return conn_code(
                sub_code2,
                code_CALL(ret_var, func_name)
            );
        } else if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "LB") &&
            !strcmp(cur_node->son[2]->tag, "Exp") &&
            !strcmp(cur_node->son[3]->tag, "RB")
        ) {
            glb_node = glb_node->son[0];
            InterCode sub_code0 = Exp();
            char *sub_var0 = ret_var;
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
            InterCode sub_code2 = Exp();
            char *sub_var2 = ret_var;
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

            char *offset = temp_var();
            char *base_addr = temp_var();
            ret_var = ptr(base_addr);
            return conn_code(
                conn_code( 
                    conn_code(
                        sub_code0,
                        sub_code2
                    ),
                    code_D_STAR(offset, sub_var2, i2s(arr_type->detail.array.elem->size))
                ),
                code_PLUS(base_addr, addr(sub_var0), offset)
            );
        }
    }
    printf("%d ", cur_node->lineno);
    perror("Exp error");
    return NULL;
}

InterCode Dec() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("Dec is empty.");
        return NULL;
    } else if (son_cnt == 1) {
        if (!strcmp(cur_node->son[0]->tag, "VarDec")) {

            glb_node = glb_node->son[0];
            InterCode sub_code0 = VarDec();
            char *sub_var0 = ret_var;
            glb_node = cur_node;

            int i, j = 4;
            for (i = 0; i < size_cnt; i++) {
                j *= size_stack[i];
            }
            return conn_code(
                sub_code0,
                code_DEC(sub_var0, i2s(j))
            );
        }
    } else if (son_cnt == 3) {
        if (
            !strcmp(cur_node->son[0]->tag, "VarDec") &&
            !strcmp(cur_node->son[1]->tag, "ASSIGNOP") &&
            !strcmp(cur_node->son[2]->tag, "Exp")
        ) {

            glb_node = glb_node->son[0];
            InterCode sub_code0 = VarDec();
            char *sub_var0 = ret_var;
            glb_node = cur_node;

            Type left_type = cur_type;

            glb_node = glb_node->son[1];
            // ASSIGNOP
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            InterCode sub_code2 = Exp();
            char *sub_var2 = ret_var;
            glb_node = cur_node;

            {
                if (!is_same_type(left_type, cur_type)) {
                    syn_error(5, "Type mismatched for assignment");
                }
            }

            return conn_code(
                conn_code(
                    code_DEC(sub_var0, i2s(left_type->size)),
                    conn_code(
                        sub_code0,
                        sub_code2
                    )
                ),
                code_ASSIGNOP(sub_var0, sub_var2)
            );
        }
    }
    printf("%d ", cur_node->lineno);
    perror("Dec error");
    return NULL;
}

InterCode DecList() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;
    if (son_cnt == -1) {
        // empty
        perror("DecList is empty.");
        return NULL;
    } else if (son_cnt == 1) {
        if (
            !strcmp(cur_node->son[0]->tag, "Dec")
        ) {

            glb_node = glb_node->son[0];
            InterCode sub_code0 = Dec();
            glb_node = cur_node;
            
            {
                if (!var_is_exist) {
                    enter(sym_id);
                }
            }

            return sub_code0;
        }
    } else if (son_cnt == 3) {
        if (
            !strcmp(cur_node->son[0]->tag, "Dec") &&
            !strcmp(cur_node->son[1]->tag, "COMMA") &&
            !strcmp(cur_node->son[2]->tag, "DecList")
        ) {

            glb_node = glb_node->son[0];
            InterCode sub_code0 = Dec();
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
            InterCode sub_code2 = DecList();
            glb_node = cur_node;


            return conn_code(
                sub_code0,
                sub_code2
            );
        }
    }
    printf("%d ", cur_node->lineno);
    perror("DecList error");
    return NULL;
}

InterCode Def() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("Def is empty.");
        return NULL;
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
            InterCode sub_code1 = DecList();
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            // SEMI
            glb_node = cur_node;

            return sub_code1;
        }
    }
    printf("%d ", cur_node->lineno);
    perror("Def error");
    return NULL;
}

InterCode DefList() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        return NULL;
    } else if (son_cnt == 2) {
        if (
            !strcmp(cur_node->son[0]->tag, "Def") &&
            !strcmp(cur_node->son[1]->tag, "DefList")
        ) {

            glb_node = glb_node->son[0];
            InterCode sub_code0 = Def();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            InterCode sub_code1 = DefList();
            glb_node = cur_node;

            return conn_code(
                sub_code0,
                sub_code1
            );
        }
    }
    printf("%d ", cur_node->lineno);
    perror("DefList error");
    return NULL;
}

InterCode Stmt() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("Stmt is empty.");
        return NULL;
    } else if (son_cnt == 1) {
        if (
            !strcmp(cur_node->son[0]->tag, "CompSt")
        ) {

            glb_node = glb_node->son[0];
            InterCode sub_code0 = CompSt();
            glb_node = cur_node;

            return sub_code0;
        }
    } else if (son_cnt == 2) {
        if (
            !strcmp(cur_node->son[0]->tag, "Exp") &&
            !strcmp(cur_node->son[1]->tag, "SEMI")
        ) {
            
            glb_node = glb_node->son[0];
            InterCode sub_code0 = Exp();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // SEMI
            glb_node = cur_node;

            return sub_code0;
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
            InterCode sub_code1 = Exp();
            char *sub_var1 = ret_var;
            glb_node = cur_node;

            FieldList tmp = symtab_stack[symtab_cnt-1];
            if (!is_same_type(cur_type, tmp->type->detail.function->ret)) {
                syn_error(8, "Type mismatched for return");
            }
            
            glb_node = glb_node->son[2];
            // SEMI
            glb_node = cur_node;

            ret_var = NULL;
            return conn_code(sub_code1, code_RETURN(sub_var1));
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
            InterCode sub_code2 = Exp();
            char *tmp_truelabel = truelabel;
            char *tmp_falselabel = falselabel;
            truelabel = NULL;
            falselabel = NULL;
            glb_node = cur_node;

            glb_node = glb_node->son[3];
            // RP
            glb_node = cur_node;

            glb_node = glb_node->son[4];
            InterCode sub_code4 = Stmt();
            glb_node = cur_node;

            /*
            condition exp
            true_label
            stmt
            false_label
            */
            return conn_code(
                sub_code2,
                conn_code(
                    code_LABEL(tmp_truelabel),
                    conn_code(
                        sub_code4,
                        code_LABEL(tmp_falselabel)
                    )
                )
            );
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
            InterCode sub_code2 = Exp();
            char *tmp_truelabel = truelabel;
            char *tmp_falselabel = falselabel;
            truelabel = NULL;
            falselabel = NULL;
            glb_node = cur_node;

            glb_node = glb_node->son[3];
            // RP
            glb_node = cur_node;

            glb_node = glb_node->son[4];
            InterCode sub_code4 = Stmt();
            glb_node = cur_node;

            char *back_label = new_label();
            /*
            back_label
            condition exp
            true_label
            stmt
            goto back_label
            false_label
            */
            return conn_code(
                code_LABEL(back_label),
                conn_code(
                    sub_code2,
                    conn_code(
                        code_LABEL(tmp_truelabel),
                        conn_code(
                            sub_code4,
                            conn_code(
                                code_JMP(back_label),
                                code_LABEL(tmp_falselabel)
                            )
                        )
                    )
                )
            );
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
            InterCode sub_code2 = Exp();
            char *tmp_truelabel = truelabel;
            char *tmp_falselabel = falselabel;
            truelabel = NULL;
            falselabel = NULL;
            glb_node = cur_node;

            glb_node = glb_node->son[3];
            // RP
            glb_node = cur_node;

            glb_node = glb_node->son[4];
            InterCode sub_code4 = Stmt();
            glb_node = cur_node;

            glb_node = glb_node->son[5];
            // ELSE
            glb_node = cur_node;

            glb_node = glb_node->son[6];
            InterCode sub_code6 = Stmt();
            glb_node = cur_node;

            char *end_label = new_label();
            /*
            condition exp
            true_label
            stmt
            goto end_label
            false_label
            stmt
            end_label
            */
            return conn_code(
                sub_code2,
                conn_code(
                    code_LABEL(tmp_truelabel),
                    conn_code(
                        sub_code4,
                        conn_code(
                            code_JMP(end_label),
                            conn_code(
                                code_LABEL(tmp_falselabel),
                                conn_code(
                                    sub_code6,
                                    code_LABEL(end_label)
                                )
                            )
                        )
                    )
                )
            );
        }
    }
    printf("%d ", cur_node->lineno);
    perror("Stmt error");
    return NULL;
}

InterCode StmtList() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        return NULL;
    } else if (son_cnt == 2) {
        if (
            !strcmp(cur_node->son[0]->tag, "Stmt") &&
            !strcmp(cur_node->son[1]->tag, "StmtList")
        ) {

            glb_node = glb_node->son[0];
            InterCode sub_code0 = Stmt();
            glb_node = cur_node;
            
            glb_node = glb_node->son[1];
            InterCode sub_code1 = StmtList();
            glb_node = cur_node;

            return conn_code(
                sub_code0,
                sub_code1
            );
        }
    }
    printf("%d ", cur_node->lineno);
    perror("StmtList error");
    return NULL;
}

InterCode CompSt() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("CompSt is empty.");
        return NULL;
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
            InterCode sub_code1 = DefList();
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            InterCode sub_code2 = StmtList();
            glb_node = cur_node;

            glb_node = glb_node->son[3];
            // RC
            glb_node = cur_node;

            ret_var = NULL;
            return conn_code(
                sub_code1,
                sub_code2
            );
        }
    }
    printf("%d ", cur_node->lineno);
    perror("CompSt error");
    return NULL;
}

InterCode ParamDec() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("ParamDec is empty.");
        return NULL;
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
            char *sub_var1 = ret_var;
            glb_node = cur_node;

            if (!var_is_exist) {
                enter(sym_id);
            }
            return code_PARAM(sub_var1);
        }
    }
    printf("%d ", cur_node->lineno);
    perror("ParamDec error");
    return NULL;
}

InterCode VarList() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("VarList is empty.");
        return NULL;
    } else if (son_cnt == 1) {
        if (
            !strcmp(cur_node->son[0]->tag, "ParamDec")
        ) {

            glb_node = glb_node->son[0];
            InterCode sub_code0 = ParamDec();
            glb_node = cur_node;

            return sub_code0;
        }
    } else if (son_cnt == 3) {
        if (
            !strcmp(cur_node->son[0]->tag, "ParamDec") &&
            !strcmp(cur_node->son[1]->tag, "COMMA") &&
            !strcmp(cur_node->son[2]->tag, "VarList")
        ) {

            glb_node = glb_node->son[0];
            InterCode sub_code0 = ParamDec();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            // COMMA
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            InterCode sub_code2 = VarList();
            glb_node = cur_node;

            return conn_code(
                sub_code0,
                sub_code2
            );
        }
    } 
    printf("%d ", cur_node->lineno);
    perror("VarList error");
    return NULL;
}

InterCode FunDec() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("FunDec is empty.");
        return NULL;
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

            return code_FUNC(sym_id);
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
            char *func_name = sym_id;

            glb_node = glb_node->son[1];
            // LP
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            InterCode sub_code2 = VarList();
            glb_node = cur_node;

            // record the args
            func->args = symtab;

            glb_node = glb_node->son[3];
            // RP
            glb_node = cur_node;

            return conn_code(
                code_FUNC(func_name),
                sub_code2
            );
        }
    }
    printf("%d ", cur_node->lineno);
    perror("FunDec error");
    return NULL;
}

InterCode VarDec() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("VarDec is empty.");
        return NULL;
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
            ret_var = sym_id;
            glb_node = cur_node;

            return NULL;
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
            return NULL;
        }
    } 
    printf("%d ", cur_node->lineno);
    perror("VarDec error");
    return NULL;
}

InterCode Tag() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("Program is empty.");
        return NULL;
    } else if (son_cnt == 1) {
        if (
            !strcmp(cur_node->son[0]->tag, "ID")
        ) {

            glb_node = glb_node->son[0];
            // ID
            sym_id = glb_node->val.s;
            glb_node = cur_node;

            return NULL;
        }
    }
    printf("%d ", cur_node->lineno);
    perror("Tag error");
    return NULL;
}

InterCode OptTag() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        sym_id = NULL;
        return NULL;
    } else if (son_cnt == 1) { 
        if (
            !strcmp(cur_node->son[0]->tag, "ID")
        ) {

            glb_node = glb_node->son[0];
            // ID
            sym_id = glb_node->val.s;
            glb_node = cur_node;

            return NULL;
        }
    } 
    printf("%d ", cur_node->lineno);
    perror("OptTag error");
    return NULL;
}

InterCode StructSpecifier() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("StructSpecifier is empty.");
        return NULL;
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

            return NULL;
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

            return NULL;
        }
    }
    printf("%d ", cur_node->lineno);
    perror("StructSpecifier error");
    return NULL;
}

InterCode Specifier() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("Program is empty.");
        return NULL;
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
                    cur_type->size = 4;
                } else if (!strcmp(type_name, "float")) {
                    cur_type->detail.basic = FLOAT;
                    cur_type->size = 8;
                } else {
                    printf("no such TYPE: %s\n", type_name);
                    exit(1);
                }
            }
            glb_node = cur_node;

            return NULL;
        } else if (
            !strcmp(cur_node->son[0]->tag, "StructSpecifier")
        ) {
            
            glb_node = glb_node->son[0];
            StructSpecifier();
            glb_node = cur_node;

            return NULL;
        }
    } 
    printf("%d ", cur_node->lineno);
    perror("Specifier error");
    return NULL;
}

InterCode ExtDecList() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("ExtDecList is empty.");
        return NULL;
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

            return NULL;
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

            return NULL;
        }
    }
    printf("%d ", cur_node->lineno);
    perror("ExtDecList error");
    return NULL;
}

InterCode ExtDef() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("ExtDef is empty.");
        return NULL;
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

            return NULL;
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

            return NULL;
        } else if (
            !strcmp(cur_node->son[0]->tag, "Specifier") &&
            !strcmp(cur_node->son[1]->tag, "FunDec") &&
            !strcmp(cur_node->son[2]->tag, "CompSt")
        ) {
            
            glb_node = glb_node->son[0];
            Specifier();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            InterCode sub_code1 = FunDec();
            glb_node = cur_node;

            glb_node = glb_node->son[2];
            InterCode sub_code2 = CompSt();
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

            return conn_code(
                sub_code1,
                sub_code2
            );
        }
    } 
    printf("%d ", cur_node->lineno);
    perror("ExtDef error");
    return NULL;
}

InterCode ExtDefList() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        return NULL;
    } else if (son_cnt == 2) {
        if (
            !strcmp(cur_node->son[0]->tag, "ExtDef") &&
            !strcmp(cur_node->son[1]->tag, "ExtDefList")
        ) {

            glb_node = glb_node->son[0];
            InterCode sub_code0 = ExtDef();
            glb_node = cur_node;

            glb_node = glb_node->son[1];
            InterCode sub_code1 = ExtDefList();
            glb_node = cur_node;

            return conn_code(
                sub_code0,
                sub_code1
            );
        }
    }
    printf("%d ", cur_node->lineno);
    perror("ExtDefList error");
    return NULL;
}

InterCode Program() {
    tree *cur_node = glb_node;
    int son_cnt = cur_node->son_cnt;

    if (son_cnt == -1) {
        // empty
        perror("Program is empty.");
        return NULL;
    } else if (son_cnt == 1) {
        if (
            !strcmp(cur_node->son[0]->tag, "ExtDefList")
        ) {

            glb_node = glb_node->son[0];
            InterCode sub_code0 = ExtDefList();
            glb_node = cur_node;

            return sub_code0;
        }
    } 
    printf("%d ", cur_node->lineno);
    perror("Program error");
    return NULL;
}

void parse_AST(tree *root) {
    if (!root) {
        perror("root is NULL\n");
        return;
    }
    init_symtab();
    glb_node = root;
    InterCode code = Program();
    print_code(code);
    // print_symtab(symtab);
    
    return;
}

// int main(int argc, char const *argv[])
// {
//     InterCode code;
//     code = code_ASSIGNOP("t1", "v1");
//     code = conn_code(code, code_LABEL("label1"));
//     code = conn_code(code, code_D_STAR("t2", "v2", "v3"));
//     code = conn_code(code, code_T_JMP("label1", relop("v2", "<=", i2s(2))));
//     print_code(code);
//     return 0;
// }
