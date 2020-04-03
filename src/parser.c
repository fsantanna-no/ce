#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

///////////////////////////////////////////////////////////////////////////////

void next () {
    long off = ftell(LX.buf);
    Tk   tk  = lexer();

    if (LX.off == 0) {
        LX.lin = 1;
        LX.col = 1;
    } else {
        if (LX.tk.sym == TK_LINE) {
            LX.lin++;
            LX.col = 1;
        } else {
            LX.col += (off - LX.off);
        }
    }
    LX.tk  = tk;
    LX.off = off;
//printf("%ld %ld %s\n", LX.lin, stk.col, lexer_tk2str(&stk.tk));
}

#if 0
void back () {
    fseek(LX.buf, LX.off, SEEK_SET);
}
#endif

///////////////////////////////////////////////////////////////////////////////

Error expected (const char* v) {
    Error ret;
    ret.off = LX.off;
    sprintf(ret.msg, "(ln %ld, col %ld): expected %s", LX.lin, LX.col+lexer_tk2len(&LX.tk), v);
    return ret;
}

Error unexpected (const char* v) {
    Error ret;
    ret.off = LX.off;
    sprintf(ret.msg, "(ln %ld, col %ld): unexpected %s", LX.lin, LX.col, v);
    return ret;
}

///////////////////////////////////////////////////////////////////////////////

void parser_init (FILE* buf) {
    LX = (Lexer) { buf,0,0,0,{} };
    next();
}

///////////////////////////////////////////////////////////////////////////////

Type parser_type (void) {
    switch (LX.tk.sym) {
        case '(':
            next();
            if (LX.tk.sym == ')') {
                return (Type) { TYPE_UNIT, {} };
            } else {
                return (Type) { TYPE_NONE, .err=unexpected(lexer_tk2str(&LX.tk)) };
            }
    }
    return (Type) { TYPE_NONE, .err=expected("type") };
}

///////////////////////////////////////////////////////////////////////////////

Expr parser_expr (void) {
    switch (LX.tk.sym) {
        case '(':
            next();
            if (LX.tk.sym == ')') {
                return (Expr) { EXPR_UNIT, {} };
            } else {
                Expr ret = parser_expr();
                next();
                if (LX.tk.sym == ')') {
                    return ret;
                } else {
                    return (Expr) { EXPR_NONE, .err=expected("`)`") };
                }
            }
        case TK_VAR:
            return (Expr) { EXPR_VAR, .tk=LX.tk };
        case TK_DATA:
            return (Expr) { EXPR_CONS, .tk=LX.tk };
    }
    return (Expr) { EXPR_NONE, {} };
}

///////////////////////////////////////////////////////////////////////////////

Decl parser_decl (void) {
    if (LX.tk.sym != TK_VAR) {
        return (Decl) { DECL_NONE, .err=expected("declaration") };
    }
    Tk var = LX.tk;

    next();
    if (LX.tk.sym != TK_DECL) {
        return (Decl) { DECL_NONE, .err=expected("`::`") };
    }

    next();
    Type tp = parser_type();
    if (tp.sub == TYPE_NONE) {
        return (Decl) { DECL_NONE, .err=tp.err };
    }

    return (Decl) { DECL_SIG, .var=var, .type=tp };
}
