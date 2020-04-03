#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "lexer.h"
#include "parser.h"

FILE* stropen (char* str) {
    return fmemopen(str, strlen(str), "r");
}

void t_lexer (void) {
    {
        LX.buf = stropen("-- foobar");
        assert(lexer() == TK_COMMENT);
        assert(lexer() == TK_EOF);
        fclose(LX.buf);
    }
    {
        LX.buf = stropen("-- c1\n--c2\n\n");
        assert(lexer() == TK_COMMENT);
        assert(lexer() == TK_LINE);
        assert(lexer() == TK_COMMENT);
        assert(lexer() == TK_LINE);
        assert(lexer() == TK_LINE);
        assert(lexer() == TK_EOF);
        fclose(LX.buf);
    }
    {
        LX.buf = stropen(" c1\nc2 c3'  \n  \nc4");
        assert(lexer() == TK_VAR);         assert(!strcmp(LX.val.s, "c1"));
        assert(lexer() == TK_LINE);
        assert(lexer() == TK_VAR);         assert(!strcmp(LX.val.s, "c2"));
        assert(lexer() == TK_VAR);         assert(!strcmp(LX.val.s, "c3'"));
        assert(lexer() == TK_LINE);
        assert(lexer() == TK_LINE);
        assert(lexer() == TK_VAR);         assert(!strcmp(LX.val.s, "c4"));
        assert(lexer() == TK_EOF);
        fclose(LX.buf);
    }
    {
        LX.buf = stropen(" c1 C1 C'a a'? C!!");
        assert(lexer() == TK_VAR);  assert(!strcmp(LX.val.s, "c1"));
        assert(lexer() == TK_DATA); assert(!strcmp(LX.val.s, "C1"));
        assert(lexer() == TK_DATA); assert(!strcmp(LX.val.s, "C'a"));
        assert(lexer() == TK_VAR);  assert(!strcmp(LX.val.s, "a'?"));
        assert(lexer() == TK_DATA); assert(!strcmp(LX.val.s, "C!!"));
        assert(lexer() == TK_EOF);
        fclose(LX.buf);
    }
    {
        LX.buf = stropen("let xlet letx");
        assert(lexer() == TK_LET);
        assert(lexer() == TK_VAR); assert(!strcmp(LX.val.s, "xlet"));
        assert(lexer() == TK_VAR); assert(!strcmp(LX.val.s, "letx"));
        assert(lexer() == TK_EOF);
        fclose(LX.buf);
    }
}

void t_parser_exp (void) {
    {
        LX.buf = stropen("(())");
        assert(parser_exp() == EXP_UNIT);
        fclose(LX.buf);
    }
    {
        LX.buf = stropen("( ( ) )");
        assert(parser_exp() == EXP_UNIT);
        fclose(LX.buf);
    }
    {
        LX.buf = stropen("(\n( \n");
        assert(parser_exp() == EXP_NONE); assert(!strcmp(LX.val.s, "(ln 2, col 2): expected `)`"));
        fclose(LX.buf);
    }
}

void t_parser (void) {
    {
        LX.buf = stropen("xxx (  ) XXX");
        assert(parser_exp() == EXP_VAR);  assert(!strcmp(LX.val.s, "xxx"));
        assert(parser_exp() == EXP_UNIT);
        assert(parser_exp() == EXP_CONS); assert(!strcmp(LX.val.s, "XXX"));
        fclose(LX.buf);
    }
    t_parser_exp();
}

int main (void) {
    t_lexer();
    t_parser();
}
