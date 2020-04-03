#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

typedef struct {
    long off;   // position before token (to fallback)
    long lin;   // line before token
    long col;   // column before token
    Tk   tk;
} STK;

struct {
    STK stk[256];
    int stkn;
} PR = { {}, 0 };

Tk push () {
    STK stk = { ftell(LX.buf), 1, 1, lexer() };

    if (PR.stkn > 0) {
        STK prv = PR.stk[PR.stkn-1];
        stk.lin = prv.lin;
        stk.col = prv.col + (stk.off - prv.off);
        if (prv.tk.sym == TK_LINE) {
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

Error expected (const char* msg) {
    Error ret;
    STK stk = PR.stk[PR.stkn-1];
    ret.off = stk.off;
    sprintf(ret.msg, "(ln %ld, col %ld): expected %s", stk.lin, stk.col, msg);
    return ret;
}

///////////////////////////////////////////////////////////////////////////////

TYPE parser_type () {
    Tk tk = push();
    switch (tk.sym) {
        case '(':
            tk = push();
            return (tk.sym == ')') ? TYPE_UNIT : TYPE_NONE;
    }
    return TYPE_NONE;
}

///////////////////////////////////////////////////////////////////////////////

Expr parser_expr () {
    Tk tk = push();
    switch (tk.sym) {
        case '(':
            tk = push();
            if (tk.sym == ')') {
                return (Expr) { EXPR_UNIT, {} };
            } else {
                pop();
                Expr ret = parser_expr();
                tk = push();
                if (tk.sym == ')') {
                    return ret;
                } else {
                    return (Expr) { EXPR_NONE, .err=expected("`)`") };
                }
            }
        case TK_VAR:
            return (Expr) { EXPR_VAR, .tk=tk };
        case TK_DATA:
            return (Expr) { EXPR_CONS, .tk=tk };
    }
    return (Expr) { EXPR_NONE, {} };
}

///////////////////////////////////////////////////////////////////////////////

Decl parser_decl () {
    Tk var = push();
    if (var.sym != TK_VAR) {
        return (Decl) { DECL_NONE, .err=expected("declaration") };
    }

    Tk dcl = push();
    if (dcl.sym != TK_DECL) {
        return (Decl) { DECL_NONE, .err=expected("`::`") };
    }

    TYPE tp = parser_type();
    if (tp == TYPE_NONE) {
        return (Decl) { DECL_NONE, .err=expected("declaration type") };
    }

    return (Decl) { DECL_SIG, .var=var, .type=tp };
}
