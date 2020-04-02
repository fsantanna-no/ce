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
    {
        FILE* buf = stropen("-- c1\n--c2\n\n");
        assert(lex(buf) == TK_COMMENT);
        assert(lex(buf) == TK_NEWLINE);
        assert(lex(buf) == TK_COMMENT);
        assert(lex(buf) == TK_NEWLINE);
        assert(lex(buf) == TK_NEWLINE);
        assert(lex(buf) == TK_EOF);
        fclose(buf);
    }
    {
        FILE* buf = stropen(" c1\nc2 c3'  \n  \nc4");
        assert(lex(buf) == TK_VAR);         assert(!strcmp(lex_value, "c1"));
        assert(lex(buf) == TK_NEWLINE);
        assert(lex(buf) == TK_VAR);         assert(!strcmp(lex_value, "c2"));
        assert(lex(buf) == TK_VAR);         assert(!strcmp(lex_value, "c3'"));
        assert(lex(buf) == TK_NEWLINE);
        assert(lex(buf) == TK_NEWLINE);
        assert(lex(buf) == TK_VAR);         assert(!strcmp(lex_value, "c4"));
        assert(lex(buf) == TK_EOF);
        fclose(buf);
    }
    {
        FILE* buf = stropen(" c1 C1 C'a a'? C!!");
        assert(lex(buf) == TK_VAR);  assert(!strcmp(lex_value, "c1"));
        assert(lex(buf) == TK_DATA); assert(!strcmp(lex_value, "C1"));
        assert(lex(buf) == TK_DATA); assert(!strcmp(lex_value, "C'a"));
        assert(lex(buf) == TK_VAR);  assert(!strcmp(lex_value, "a'?"));
        assert(lex(buf) == TK_DATA); assert(!strcmp(lex_value, "C!!"));
        fclose(buf);
    }
}
