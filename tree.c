#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"

tree *create_node(char *tag, int val_type, value *val, int lineno) {
    tree *cur_tree = (tree *)malloc(sizeof(tree));
    if (!cur_tree)
        return NULL;
    
    cur_tree->tag = (char *)malloc(strlen(tag) + 1);
    strcpy(cur_tree->tag, tag);

    cur_tree->val_type = val_type;
    
    switch (val_type) {
        case STR_VAL:
            cur_tree->val.s = (char *)malloc(strlen((*val).s) + 1);
            strcpy(cur_tree->val.s, (*val).s);
            break;
        case INT_VAL:
            cur_tree->val.i = val->i;
            break;
        case FLOAT_VAL:
            cur_tree->val.f = val->f;
            break;
        case NO_VAL:
            break;
        default:
            free(cur_tree);
            return NULL;
    }
    

    cur_tree->son_cnt = 0;
    memset(cur_tree->son, 0, 10 * sizeof(tree *));

    cur_tree->lineno = lineno;
    return cur_tree;
}

int add_son(tree *father, tree *son) {
    if (!father) {
        return 1;
    }
    int son_cnt = father->son_cnt;
    if (son_cnt >= MAX_SON || !son) {
        return 1;
    }
    father->son[son_cnt] = son;
    father->son_cnt++;
    return 0;
}

static char *space(int n) {
    int len = (2 * n + 1) * sizeof(char);
    char *str = (char *)malloc(len);
    memset(str, ' ', len);
    str[len - 1] = '\0';
    return str;
}

void show_tree(tree *cur_node, int tab) {
    int son_cnt = cur_node->son_cnt;
    // empty nonleaf
    if (son_cnt == -1)
        return;
    printf("%s", space(tab));
    printf("%s", cur_node->tag);
    switch (cur_node->val_type) {
        case NO_VAL:
            if (son_cnt > 0)
                printf(" (%d)\n", cur_node->lineno);
            else
                printf("\n");
            break;
        case INT_VAL:
            printf(": %d\n", cur_node->val.i);
            break;
        case FLOAT_VAL:
            printf(": %f\n", cur_node->val.f);
            break;
        case STR_VAL:
            printf(": %s\n", cur_node->val.s);
            break;
        default:
            printf("\n");
    }

    int i;
    for (i = 0; i < son_cnt; i++) {
        // printf("%p->son[%d]: %p\n", cur_node, i, cur_node->son[i]);
        show_tree(cur_node->son[i], tab + 1);
    }
}

// int main() {
//     tree *root = create_node("R1", NO_VAL, NULL, 0);
//     tree *son1 = create_node("S1", NO_VAL, NULL, 3);
//     tree *son2 = create_node("S2", NO_VAL, NULL, 4);
//     tree *son3 = create_node("S3", NO_VAL, NULL, 5);
//     PUB_VALUE.i = 0;
//     tree *grandson1 = create_node("G1", INT_VAL, &PUB_VALUE, 6);
//     add_son(root, son1);
//     add_son(root, son2);
//     add_son(root, son3);
//     add_son(son1, grandson1);

//     show_tree(root, 0);    
// }