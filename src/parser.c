#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

///////////////////////////////////////////////////////////////////////////////

void pr_next () {
    OLD = CUR;

    long off = ftell(CUR.buf);
    Tk   tk  = lexer();

    if (OLD.off == -1) {
        CUR.lin = 1;
        CUR.col = 1;
    } else {
        if (OLD.tk.sym == TK_LINE) {
            CUR.lin = OLD.lin + 1;
            CUR.col = 1;
        } else {
            CUR.col = OLD.col + (off - OLD.off);
        }
    }
    CUR.tk  = tk;
    CUR.off = off;
    //printf("CUR: ln=%ld cl=%ld off=%ld tk=%s\n", CUR.lin, CUR.col, CUR.off, lexer_tk2str(&CUR.tk));
}

int pr_accept (TK tk) {
    if (CUR.tk.sym == tk) {
        pr_next();
        return 1;
    } else {
        return 0;
    }
}

#if 0
void back () {
    fseek(CUR.buf, CUR.off, SEEK_SET);
}
#endif

///////////////////////////////////////////////////////////////////////////////

Error expected (const char* v) {
    Error ret;
    ret.off = CUR.off;
    //sprintf(ret.msg, "(ln %ld, col %ld): expected %s : have %s",
        //CUR.lin, CUR.col+lexer_tk2len(&CUR.tk), v, lexer_tk2str(&CUR.tk));
    sprintf(ret.msg, "(ln %ld, col %ld): expected %s : have %s",
        CUR.lin, CUR.col, v, lexer_tk2str(&CUR.tk));
    return ret;
}

Error unexpected (const char* v) {
    Error ret;
    ret.off = CUR.off;
    sprintf(ret.msg, "(ln %ld, col %ld): unexpected %s", CUR.lin, CUR.col, v);
    return ret;
}

///////////////////////////////////////////////////////////////////////////////

void parser_init (FILE* buf) {
    CUR = (Lexer) { buf,-1,0,0,{} };
    pr_next();
}

///////////////////////////////////////////////////////////////////////////////

Type parser_type (void) {
    if (pr_accept('(')) {
        if (pr_accept(')')) {
            return (Type) { TYPE_UNIT, {} };
        } else {
            return (Type) { TYPE_NONE, .err=unexpected(lexer_tk2str(&CUR.tk)) };
        }
    }
    return (Type) { TYPE_NONE, .err=expected("type") };
}

///////////////////////////////////////////////////////////////////////////////

Expr parser_expr (void) {
    if (pr_accept('(')) {
        if (pr_accept(')')) {
            return (Expr) { EXPR_UNIT, {} };
        } else {
            Expr ret = parser_expr();
            if (pr_accept(')')) {
                return ret;
            } else {
                return (Expr) { EXPR_NONE, .err=expected("`)`") };
            }
        }
    } else if (pr_accept(TK_VAR)) {
        return (Expr) { EXPR_VAR, .tk=OLD.tk };
    } else if (pr_accept(TK_DATA)) {
        return (Expr) { EXPR_CONS, .tk=OLD.tk };
    }
    return (Expr) { EXPR_NONE, {} };
}

///////////////////////////////////////////////////////////////////////////////

Decl parser_decl (void) {
    if (!pr_accept(TK_VAR)) {
        return (Decl) { DECL_NONE, .err=expected("declaration") };
    }
    Tk var = OLD.tk;

    if (!pr_accept(TK_DECL)) {
        return (Decl) { DECL_NONE, .err=expected("`::`") };
    }

    Type tp = parser_type();
    if (tp.sub == TYPE_NONE) {
        return (Decl) { DECL_NONE, .err=tp.err };
    }

    return (Decl) { DECL_SIG, .var=var, .type=tp };
}
