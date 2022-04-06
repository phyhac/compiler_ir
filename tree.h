#define MAX_SON 10

union value {
    int i;
    float f;
    char *s;
};
typedef union value value;
value PUB_VALUE;

enum node_val_type {
    NO_VAL = 0,
    INT_VAL = 1,
    FLOAT_VAL = 2,
    STR_VAL = 3,
};

struct tree {
    char *tag;
    int val_type;
    // 0 is null, 1 is int, 2 is float, 3 is string
    value val;
    int lineno;
    int son_cnt;
    struct tree *son[MAX_SON];
};
typedef struct tree tree;

tree *create_node(char *tag, int val_type, value *val, int lineno);

int add_son(tree *father, tree *son);

void show_tree(tree *cur_node, int tab);