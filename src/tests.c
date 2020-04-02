#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "lex.h"

FILE* stropen (char* str) {
    return fmemopen(str, strlen(str), "r");
}

int main (void) {
    {
        FILE* buf = stropen("-- foobar");
        assert(lex(buf) == TK_COMMENT);
        assert(lex(buf) == TK_EOF);
        fclose(buf);
    }
}
