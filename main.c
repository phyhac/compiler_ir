#include <stdio.h>

extern int yydebug;
extern void yyrestart (FILE *);
extern int yyparse(void);

int main(int argc, char **argv) {
    FILE *f;
    if (argc <= 1)
        return 1;
    else if (!(f = fopen(argv[1], "r"))) {
        perror(argv[1]);
        return 1;
    }
    yyrestart(f);
    // yydebug = 1;
    yyparse();
    return 0;
}
