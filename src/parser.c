#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

typedef struct {
    long off;   // position before token (to fallback)
    long lin;   // line before token
    long col;   // column before token
    TK   tk;
} STK;

struct {
    STK stk[256];
    int stkn;
} PR = { {}, 0 };

TK push () {
    STK stk = { ftell(LX.buf), 1, 1, lexer() };

    if (PR.stkn > 0) {
        STK prv = PR.stk[PR.stkn-1];
        stk.lin = prv.lin;
        stk.col = prv.col + (stk.off - prv.off);
        if (prv.tk == TK_LINE) {
            stk.lin++;
            stk.col--;  // TODO \r\n
        }
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
    sprintf(LX.val.s, "(ln %ld, col %ld): expected `%s`", stk.lin, stk.col, v);
}

///////////////////////////////////////////////////////////////////////////////

EXPR parser_expr () {
    TK tk = push();
    switch (tk) {
        case '(':
            tk = push();
            if (tk == ')') {
                return EXPR_UNIT;
            } else {
                pop();
                EXPR ret = parser_expr();
                tk = push();
                if (tk == ')') {
                    return ret;
                } else {
                    expected(")");
                    return EXPR_NONE;
                }
            }
        case TK_VAR:
            return EXPR_VAR;
        case TK_DATA:
            return EXPR_CONS;
    }
    return EXPR_NONE;
}
