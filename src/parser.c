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
            CUR.col = (off - OLD.off);
        } else {
            CUR.col = OLD.col + (off - OLD.off);
        }
    }
    CUR.tk  = tk;
    CUR.off = off;
    printf("CUR: ln=%ld cl=%ld off=%ld tk=%s\n", CUR.lin, CUR.col, CUR.off, lexer_tk2str(&CUR.tk));
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

typedef union {
    Error err;
    struct {
        int size;
        void* vec;
    };
} List;

typedef union {
    Error err;
    void* val;
} List_Item;

typedef int (*List_F) (List_Item*);

int parser_list (List* ret, List_F f, size_t unit) {
    CUR.ind++;

    if (!pr_accept(TK_LINE, CUR.tk.val.n==CUR.ind)) {
        ret->err = unexpected("indentation level");
        return 0;
    }

    void* vec = NULL;
    int i = 0;
    while (1) {
        List_Item item;
        if (!f(&item)) {
            ret->err = item.err;
            return 0;
        }
        vec = realloc(vec, (i+1)*unit);
        memcpy(vec+i*unit, item.val, unit);
        i++;
        if (pr_accept(TK_EOF,1) || pr_accept(TK_LINE, CUR.tk.val.n<CUR.ind)) {
            break;
        }
        if (!pr_accept(TK_LINE, CUR.tk.val.n==CUR.ind)) {
            if (pr_accept(TK_LINE, CUR.tk.val.n>CUR.ind)) {
                ret->err = unexpected("indentation level");
                return 0;
            } else {
                ret->err = expected("new line");
                return 0;
            }
        }
    }

    CUR.ind--;
    ret->size = i;
    ret->vec  = vec;
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int parser_expr_ (List_Item* item) {
    static Expr e;
    e = parser_expr();
    if (e.sub == EXPR_NONE) {
        item->err = e.err;
        return 0;
    } else {
        item->val = &e;
        return 1;
    }
}

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

    // EXPR_SET
    } else if (pr_accept(TK_SET,1)) {
        if (!pr_accept(TK_VAR,1)) {
            return (Expr) { EXPR_NONE, .err=expected("variable") };
        }
        Tk var = OLD.tk;
        if (!pr_accept('=',1)) {
            return (Expr) { EXPR_NONE, .err=expected("`=`") };
        }
        Expr e = parser_expr();
        if (e.sub == EXPR_NONE) {
            return (Expr) { EXPR_NONE, .err=e.err };
        }
        Expr* pe = malloc(sizeof(*pe));
        assert(pe != NULL);
        *pe = e;
        return (Expr) { EXPR_SET, .Set={var,pe} };

    // EXPR_FUNC
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

    // EXPR_EXPRS
    } else if (pr_accept(':',1)) {
        List lst;
        int ok = parser_list(&lst, &parser_expr_, sizeof(Expr));
        if (ok) {
            return (Expr) { EXPR_EXPRS, .exprs={lst.size,lst.vec} };
        } else {
            return (Expr) { EXPR_NONE,  .err=lst.err };
        }

    // EXPR_VAR,EXPR_DATA
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
        CUR = BAK;
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

#if 0
void* parser_decl (void) {
    static Decl decl;

    if (!pr_accept(TK_VAR,1)) {
        return (Decls) { DECLS_NONE, .err=expected("declaration") };
    }
    Tk var = OLD.tk;

    // DECL_SIG
    if (pr_accept(TK_DECL,1)) {
        Type tp = parser_type();
        if (tp.sub == TYPE_NONE) {
            return (Decls) { DECLS_NONE, .err=tp.err };
        }
        //return (Decls) { DECL_SIG, .var=var, .type=tp };

    }

    return (Decls) { DECLS_NONE, .err=expected("`::`") };
}
#endif

Decls parser_decls (void) {
    if (!pr_accept(TK_VAR,1)) {
        return (Decls) { DECLS_NONE, .err=expected("declaration") };
    }
    Tk var = OLD.tk;

    // DECL_SIG
    if (pr_accept(TK_DECL,1)) {
        Type tp = parser_type();
        if (tp.sub == TYPE_NONE) {
            return (Decls) { DECLS_NONE, .err=tp.err };
        }
        //return (Decls) { DECL_SIG, .var=var, .type=tp };

    }

    return (Decls) { DECLS_NONE, .err=expected("`::`") };
}
