#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

///////////////////////////////////////////////////////////////////////////////

void dump_expr (Expr e, int spc) {
    for (int i=0; i<spc; i++) printf(" ");
    switch (e.sub) {
        case EXPR_VAR:
            puts(e.Var.val.s);
            break;
        case EXPR_SEQ:
            printf(": [%d]\n", e.Seq.size);
            for (int i=0; i<e.Seq.size; i++) {
                dump_expr(e.Seq.vec[i], spc+4);
            }
            break;
        default:
            printf(">>> %d\n", e.sub);
            assert(0 && "TODO");
    }
}

///////////////////////////////////////////////////////////////////////////////

void pr_next () {
    PRV = NXT;

    long off = ftell(ALL.inp);
    Tk   tk  = lexer();

    if (PRV.off == -1) {
        NXT.lin = 1;
        NXT.col = 1;
    } else {
        if (PRV.tk.sym == TK_LINE) {
            NXT.lin = PRV.lin + 1;
            NXT.col = (off - PRV.off);
        } else {
            NXT.col = PRV.col + (off - PRV.off);
        }
    }
    NXT.tk  = tk;
    NXT.off = off;
    //printf("NXT: ln=%ld cl=%ld off=%ld tk=%s\n", NXT.lin, NXT.col, NXT.off, lexer_tk2str(&NXT.tk));
}

int pr_accept (TK tk, int ok) {
    if (NXT.tk.sym==tk && ok) {
        pr_next();
        return 1;
    } else {
        return 0;
    }
}

int pr_check (TK tk, int ok) {
    return (NXT.tk.sym==tk && ok);
}

///////////////////////////////////////////////////////////////////////////////

int err_expected (const char* v) {
    sprintf(ALL.err, "(ln %ld, col %ld): expected %s : have %s",
        NXT.lin, NXT.col, v, lexer_tk2str(&NXT.tk));
    return 0;
}

int err_unexpected (const char* v) {
    sprintf(ALL.err, "(ln %ld, col %ld): unexpected %s", NXT.lin, NXT.col, v);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

void init (FILE* out, FILE* inp) {
    ALL = (State_All) { out,inp,{},0 };
    NXT = (State_Tok) { -1,0,0,{} };
    if (inp != NULL) {
        pr_next();
    }
}

///////////////////////////////////////////////////////////////////////////////

typedef struct {
    int size;
    void* vec;
} List;

typedef int (*List_F) (void**);

int parser_list (List* ret, List_F f, size_t unit) {
    if (!pr_accept(':',1)) {
        return err_expected("`:`");
    }

    ALL.ind++;

    if (!pr_accept(TK_LINE, NXT.tk.val.n==ALL.ind)) {
        return err_unexpected("indentation level");
    }

    void* vec = NULL;
    int i = 0;
    while (1) {
        void* item;
        if (!f(&item)) {
            return 0;
        }
        vec = realloc(vec, (i+1)*unit);
        memcpy(vec+i*unit, item, unit);
        i++;
        if (pr_check(TK_EOF,1) || pr_check(TK_LINE, NXT.tk.val.n<ALL.ind)) {
            //pr_accept(TK_LINE, NXT.tk.val.n==ALL.ind-1);
            break;
        }
        if (!pr_accept(TK_LINE, NXT.tk.val.n==ALL.ind)) {
            if (pr_accept(TK_LINE, NXT.tk.val.n>ALL.ind)) {
                return err_unexpected("indentation level");
            } else {
                return err_expected("new line");
            }
        }
    }

    ALL.ind--;
    ret->size = i;
    ret->vec  = vec;
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int parser_type (Type* ret) {
    if (pr_accept('(',1)) {
        if (pr_accept(')',1)) {
            *ret = (Type) { TYPE_UNIT, {} };
            return 1;
        } else {
            return err_unexpected(lexer_tk2str(&NXT.tk));
        }
    } else if (pr_accept(TK_IDDATA,1)) {
        *ret = (Type) { TYPE_UNIT, PRV.tk };
        return 1;
    }
    return err_expected("type");
}

///////////////////////////////////////////////////////////////////////////////

int parser_patt (Patt* ret) {
    if (pr_accept('(',1)) {
        if (pr_accept(')',1)) {
            *ret = (Patt) { PATT_UNIT, {} };
            return 1;
        }
    } else if (pr_accept(TK_IDDATA,1)) {
        *ret = (Patt) { PATT_CONS, .cons=PRV.tk };
        return 1;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

int parser_cons (void** cons) {
    static Cons d;

    if (!pr_accept(TK_IDDATA,1)) {
        return err_expected("data identifier");
    }
    d.tk = PRV.tk;

    if (!pr_accept('=',1)) {
        return err_expected("`=`");
    }

    if (!parser_type(&d.type)) {
        return 0;
    }

    *cons = &d;
    return 1;
}

int parser_data (Data* ret) {
    if (!pr_accept(TK_DATA,1)) {
        return err_expected("`data`");
    }
    if (!pr_accept(TK_IDDATA,1)) {
        return err_expected("data identifier");
    }
    Tk id = PRV.tk;

    Type tp;
    int tp_ok = pr_accept('=', 1);
    if (tp_ok) {
        parser_type(&tp);
    }

    List lst = { 0, NULL };
    int lst_ok = parser_list(&lst, &parser_cons, sizeof(Cons));

    if (!tp_ok && !lst_ok) {
        return err_expected("`=` or `:`");
    }

    *ret = (Data) { id, lst.size, lst.vec };
    for (int i=0; i<ret->size; i++) {
        ret->vec[i].idx = i;
    }

    // realloc TP more spaces inside each CONS
    if (tp_ok) {
        if (lst_ok) {
            assert(0 && "TODO");
        } else {
            ret->size = 1;
            ret->vec  = malloc(sizeof(Cons));

            Cons dt = (Cons) { 0, {}, tp };
            ret->vec[0] = dt;
        }
    }

    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int parser_decl (Decl* decl) {
    if (!pr_accept(TK_IDVAR,1)) {
        return err_expected("declaration");
    }
    decl->var = PRV.tk;

    if (!pr_accept(TK_DECL,1)) {
        return err_expected("`::`");
    }

    if (!parser_type(&decl->type)) {
        return 0;
    }

    if (pr_accept('=',1)) {
        Expr set;
        if (!parser_expr(&set)) {
            return 0;
        }
        decl->set = malloc(sizeof(*decl->set));
        assert(decl->set != NULL);
        *decl->set = set;
    } else {
        decl->set = NULL;
    }

    return 1;
}

int parser_decl_ (void** decl) {
    static Decl d;
    int ret = parser_decl(&d);
    *decl = &d;
    return ret;
}

int parser_decls (Decls* ret) {
    List lst;
    if (!parser_list(&lst, &parser_decl_, sizeof(Decl))) {
        return 0;
    }
    *ret = (Decls) { lst.size, lst.vec };
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int parser_expr_ (void** expr) {
    static Expr e;
    if (!parser_expr(&e)) {
        return 0;
    }
    *expr = &e;
    return 1;
}

int parser_case (void** casi) {
    static Case c;
    if (pr_accept(TK_ELSE,1)) {
        c.patt = (Patt) { PATT_ANY, {} };
    } else if (!parser_patt(&c.patt)) {
        return 0;
    }
    Expr e;
    if (!parser_expr(&e)) {
        return 0;
    }

    Expr* pe = malloc(sizeof(*pe));
    assert(pe != NULL);
    *pe = e;
    c.expr = &e;

    *casi = &c;
    return 1;
}

int parser_expr_one (Expr* ret) {
    // PARENS
    if (pr_accept('(',1)) {
        if (pr_accept(')',1)) {
            *ret = (Expr) { EXPR_UNIT, {} };
            return 1;
        } else {
            if (!parser_expr(ret)) {
                return 0;
            }
            if (!pr_accept(')',1)) {
                return err_expected("`)`");
            }
            return 1;
        }

    // EXPR_SET
    } else if (pr_accept(TK_SET,1)) {
        if (!pr_accept(TK_IDVAR,1)) {
            return err_expected("variable");
        }
        Tk var = PRV.tk;
        if (!pr_accept('=',1)) {
            return err_expected("`=`");
        }
        Expr e;
        if (!parser_expr(&e)) {
            return 0;
        }
        Expr* pe = malloc(sizeof(*pe));
        assert(pe != NULL);
        *pe = e;
        *ret = (Expr) { EXPR_SET, .Set={var,pe} };
        return 1;

    // EXPR_FUNC
    } else if (pr_accept(TK_FUNC,1)) {
        if (!pr_accept(TK_DECL,1)) {
            return err_expected("`::`");
        }
        Type tp;
        if (!parser_type(&tp)) {
            return 0;
        }
        Expr e;
        if (!parser_expr(&e)) {
            return 0;
        }
        Expr* pe = malloc(sizeof(*pe));
        assert(pe != NULL);
        *pe = e;
        *ret = (Expr) { EXPR_FUNC, .Func={tp,pe} };
        return 1;

    // EXPR_SEQ
    } else if (pr_check(':',1)) {
        List lst;
        if (!parser_list(&lst, &parser_expr_, sizeof(Expr))) {
            return 0;
        }
        *ret = (Expr) { EXPR_SEQ, .Seq={lst.size,lst.vec} };
        return 1;

    // EXPR_CASES
    } else if (pr_accept(TK_CASE,1)) {
        Expr e;
        if (!parser_expr(&e)) {
            return 0;
        }

        List lst;
        if (!parser_list(&lst, &parser_case, sizeof(Case))) {
            return 0;
        }

        Expr* pe = malloc(sizeof(*pe));
        assert(pe != NULL);
        *pe = e;

        *ret = (Expr) { EXPR_CASES, .Cases={pe,lst.size,lst.vec} };
        return 1;

    // EXPR_VAR,EXPR_CONS
    } else if (pr_accept(TK_IDVAR,1)) {
        *ret = (Expr) { EXPR_VAR, .Var=PRV.tk };
        return 1;
    } else if (pr_accept(TK_IDDATA,1)) {
        *ret = (Expr) { EXPR_CONS, .Cons=PRV.tk };
        return 1;
    }

    return err_expected("expression");
}

int parser_expr (Expr* ret) {
    Expr e;
    if (!parser_expr_one(&e)) {
        return 0;
    }

    // CALL
    if (pr_check('(',1)) {
        Expr arg;
        if (!parser_expr(&arg)) {
            return 0;
        }

        Expr* pe1 = malloc(sizeof(*pe1));
        Expr* pe2 = malloc(sizeof(*pe2));
        assert(pe1!=NULL && pe2!=NULL);
        *pe1 = e;
        *pe2 = arg;
        *ret = (Expr) { EXPR_CALL, .Call={pe1,pe2} };
        return 1;
    }

    // BLOCK
    if (pr_accept(TK_WHERE,1)) {
        Decls ds;
        if (!parser_decls(&ds)) {
            return 0;
        }

        Expr*  pe = malloc(sizeof(*pe));
        Decls* pd = malloc(sizeof(*pd));
        assert(pe!=NULL && pd!=NULL);
        *pe = e;
        *pd = ds;
        *ret = (Expr) { EXPR_BLOCK, .Block={pe,pd} };
        return 1;
    }

    // SINGLE
    *ret = e;
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

// Glob ::= Data | Decl | Expr
// Prog ::= { Glob }

int parser_glob (void** glob) {
    static Glob g;

    if (parser_data(&g.data)) {
        g.sub = GLOB_DATA;
        *glob = &g;
        return 1;
    }

    if (parser_decl(&g.decl)) {
        g.sub = GLOB_DECL;
        *glob = &g;
        return 1;
    }

    if (parser_expr(&g.expr)) {
        g.sub = GLOB_EXPR;
        *glob = &g;
        return 1;
    }

    return err_expected("global statement");
}

int parser_prog (Prog* ret) {
    List lst;
    if (!parser_list(&lst, &parser_glob, sizeof(Glob))) {
        return 0;
    }
    *ret = (Prog) { lst.size, lst.vec };

    if (!pr_accept(TK_EOF,1)) {
        return err_expected("end of file");
    }
    return 1;
}
