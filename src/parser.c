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

///////////////////////////////////////////////////////////////////////////////

Tk push () {
    STK stk = { ftell(LX.buf), 1, 1, lexer() };

    if (PR.stkn > 0) {
        STK prv = PR.stk[PR.stkn-1];
        if (prv.tk.sym == TK_LINE) {
            stk.lin = prv.lin + 1;
            stk.col = 1;
        } else {
            stk.lin = prv.lin;
            stk.col = prv.col + (stk.off - prv.off);
        }
    }

    assert(PR.stkn < sizeof(PR.stk)/sizeof(PR.stk[0]));
    PR.stk[PR.stkn++] = stk;
printf("%ld %ld %s\n", stk.lin, stk.col, lexer_tk2str(&stk.tk));
    return stk.tk;
}

void pop () {
    STK stk = PR.stk[--PR.stkn];
    fseek(LX.buf, stk.off, SEEK_SET);
}

///////////////////////////////////////////////////////////////////////////////

Error expected (const char* v) {
    Error ret;
    STK stk = PR.stk[PR.stkn-1];
    ret.off = stk.off;
    sprintf(ret.msg, "(ln %ld, col %ld): expected %s", stk.lin, stk.col+lexer_tk2len(&stk.tk), v);
    return ret;
}

Error unexpected (const char* v) {
    Error ret;
    STK stk = PR.stk[PR.stkn-1];
    ret.off = stk.off;
    sprintf(ret.msg, "(ln %ld, col %ld): unexpected %s", stk.lin, stk.col, v);
    return ret;
}

///////////////////////////////////////////////////////////////////////////////

Type parser_type () {
    Tk tk = push();
    switch (tk.sym) {
        case '(':
            tk = push();
            if (tk.sym == ')') {
                return (Type) { TYPE_UNIT, {} };
            } else {
                return (Type) { TYPE_NONE, .err=unexpected(lexer_tk2str(&tk)) };
            }
    }
    return (Type) { TYPE_NONE, .err=expected("type") };
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

    Type tp = parser_type();
    if (tp.sub == TYPE_NONE) {
        return (Decl) { DECL_NONE, .err=tp.err };
    }

    return (Decl) { DECL_SIG, .var=var, .type=tp };
}
