#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "lexer.h"
#include "parser.h"
#include "code.h"

int main (int argc, char* argv[]) {
    assert(argc == 2);

    FILE* fsrc = fopen(argv[1], "r");
    //FILE* fgcc = fopen("/tmp/x.c", "w");
    FILE* fgcc = popen("gcc -xc -", "w");
    assert(fsrc!=NULL && fgcc!=NULL);

    init(fgcc, fsrc);

    Prog prog;
    if (!parser_prog(&prog)) {
        fprintf(stderr, "%s\n", ALL.err);
        fclose(fsrc);
        fclose(fgcc);
        return -1;
    }
    code(prog);

    return 0;
}
