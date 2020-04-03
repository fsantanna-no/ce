#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

typedef struct {
    long off;   // position before token (to fallback)
    long line;  // line after token (if TK_LINE, already counts next)
    long col;   // column after token
    TK   tk;
} STK;

struct {
    STK stk[256];
    int stkn;
} PR = { {}, 0 };

TK push () {
    long ln = (PR.stkn == 0) ? 1 : PR.stk[PR.stkn-1].line;
    STK stk = { ftell(LX.buf), ln, 0, lexer() };
    if (stk.tk == TK_LINE) {
        stk.line++;
    }
    assert(PR.stkn < sizeof(PR.stk)/sizeof(PR.stk[0]));
    PR.stk[PR.stkn++] = stk;
    return stk.tk;
}

void pop () {
    STK stk = PR.stk[--PR.stkn];
    fseek(LX.buf, stk.off, SEEK_SET);
}

void expected (const char* v) {
    STK stk = PR.stk[PR.stkn-1];
    sprintf(LX.value, "(ln %ld, col %ld): expected `%s`", stk.line, stk.col, v);
}

EXP parser_exp () {
    TK tk = push();
printf(">>> %d %c\n", tk);
    switch (tk) {
        case '(':
            tk = push();
            if (tk == ')') {
                return EXP_UNIT;
            } else {
                pop();
                EXP ret = parser_exp();
                tk = push();
                if (tk == ')') {
                    return ret;
                } else {
                    expected(")");
puts(LX.value);
                    return EXP_NONE;
                }
            }
        case TK_VAR:
            return EXP_VAR;
        case TK_DATA:
            return EXP_CONS;
    }
    return EXP_NONE;
}
