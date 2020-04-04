#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

///////////////////////////////////////////////////////////////////////////////

void parser_dump_expr (Expr e, int spc) {
    for (int i=0; i<spc; i++) printf(" ");
    switch (e.sub) {
        case EXPR_VAR:
            puts(e.tk.val.s);
            break;
        case EXPR_EXPRS:
            printf(": [%d]\n", e.exprs.size);
            for (int i=0; i<e.exprs.size; i++) {
                parser_dump_expr(e.exprs.vec[i], spc+4);
            }
            break;
        default:
            printf(">>> %d\n", e.sub);
            assert(0 && "TODO");
    }
}

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

int pr_accept (TK tk, int ok) {
    if (CUR.tk.sym==tk && ok) {
        pr_next();
        return 1;
    } else {
        return 0;
    }
}

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
    CUR = (Lexer) { buf,0,-1,0,0,{} };
    pr_next();
}

///////////////////////////////////////////////////////////////////////////////

Type parser_type (void) {
    if (pr_accept('(',1)) {
        if (pr_accept(')',1)) {
            return (Type) { TYPE_UNIT, {} };
        } else {
            return (Type) { TYPE_NONE, .err=unexpected(lexer_tk2str(&CUR.tk)) };
        }
    }
    return (Type) { TYPE_NONE, .err=expected("type") };
}

///////////////////////////////////////////////////////////////////////////////

Expr parser_expr_one (void) {
    // PARENS
    if (pr_accept('(',1)) {
        if (pr_accept(')',1)) {
            return (Expr) { EXPR_UNIT, {} };
        } else {
            Expr ret = parser_expr();
            if (pr_accept(')',1)) {
                return ret;
            } else {
                return (Expr) { EXPR_NONE, .err=expected("`)`") };
            }
        }

    // FUNC
    } else if (pr_accept(TK_FUNC,1)) {
        if (!pr_accept(TK_DECL,1)) {
            return (Expr) { EXPR_NONE, .err=expected("`::`") };
        }
        Type tp = parser_type();
        if (tp.sub == TYPE_NONE) {
            return (Expr) { EXPR_NONE, .err=tp.err };
        }
        Expr e = parser_expr();
        if (e.sub == EXPR_NONE) {
            return e;
        }
        Expr* pe = malloc(sizeof(*pe));
        assert(pe != NULL);
        *pe = e;
        return (Expr) { EXPR_FUNC, .Func={tp,pe} };

    // EXPRS
    } else if (pr_accept(':',1)) {
        CUR.ind++;

        if (!pr_accept(TK_LINE, CUR.tk.val.n==CUR.ind)) {
            return (Expr) { EXPR_NONE, .err=unexpected("indentation level") };
        }

        Expr* vec = NULL;
        int i = 0;
        while (1) {
            Expr e = parser_expr();
            if (e.sub == EXPR_NONE) {
                if (i == 0) {
                    return (Expr) { EXPR_NONE, .err=expected("expression") };
                } else {
                    break;
                }
            }
            vec = realloc(vec, (i+1)*sizeof(vec[0]));
            vec[i++] = e;
            if (pr_accept(TK_EOF,1) || pr_accept(TK_LINE, CUR.tk.val.n<CUR.ind)) {
                break;
            }
            if (!pr_accept(TK_LINE, CUR.tk.val.n==CUR.ind)) {
                return (Expr) { EXPR_NONE, .err=unexpected("indentation level") };
            }
        }

        CUR.ind--;
        return (Expr) { EXPR_EXPRS, .exprs={i,vec} };

    // VAR,DATA
    } else if (pr_accept(TK_VAR,1)) {
        return (Expr) { EXPR_VAR, .tk=OLD.tk };
    } else if (pr_accept(TK_DATA,1)) {
        return (Expr) { EXPR_CONS, .tk=OLD.tk };
    }
    return (Expr) { EXPR_NONE, {} };
}

Expr parser_expr (void) {
    Expr e1 = parser_expr_one();
    if (e1.sub == EXPR_NONE) {
        return e1;
    }

    Lexer BAK = CUR;
    Expr e2 = parser_expr_one();
    if (e2.sub == EXPR_NONE) {
        fseek(BAK.buf, BAK.off, SEEK_SET);
        pr_next();
        return e1;
    }

    Expr* pe1 = malloc(sizeof(*pe1));
    Expr* pe2 = malloc(sizeof(*pe2));
    assert(pe1!=NULL && pe2!=NULL);
    *pe1 = e1;
    *pe2 = e2;
    return (Expr) { EXPR_CALL, .Call={pe1,pe2} };
}

///////////////////////////////////////////////////////////////////////////////

Decl parser_decl (void) {
    if (!pr_accept(TK_VAR,1)) {
        return (Decl) { DECL_NONE, .err=expected("declaration") };
    }
    Tk var = OLD.tk;

    // DECL_SIG
    if (pr_accept(TK_DECL,1)) {
        Type tp = parser_type();
        if (tp.sub == TYPE_NONE) {
            return (Decl) { DECL_NONE, .err=tp.err };
        }
        return (Decl) { DECL_SIG, .var=var, .type=tp };

    // DECL_ATR
    } else if (pr_accept('=',1)) {
        Expr e = parser_expr();
        if (e.sub == EXPR_NONE) {
            return (Decl) { DECL_NONE, .err=e.err };
        }
        return (Decl) { DECL_ATR, .patt=var, .expr=e };
    }

    return (Decl) { DECL_NONE, .err=expected("`::`") };
}
