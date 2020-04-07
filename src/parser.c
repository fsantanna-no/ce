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

int parser_list_comma (List* ret, List_F f, size_t unit) {
    if (!pr_accept(',',1)) {
        return err_expected("`,`");
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
        if (!pr_accept(',',1)) {
            break;
        }
    }

    ret->size = i;
    ret->vec  = vec;
    return 1;
}

int parser_list_line (List* ret, List_F f, size_t unit) {
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

int parser_type_ (void** type) {
    static Type tp_;
    Type tp;
    if (!parser_type(&tp)) {
        return 0;
    }
    tp_ = tp;
    *type = &tp_;
    return 1;
}

int parser_type (Type* ret) {
    // TYPE_UNIT
    if (pr_accept('(',1)) {
        if (pr_accept(')',1)) {
            *ret = (Type) { TYPE_UNIT, {} };
        } else {
            if (!parser_type(ret)) {
                return 0;
            }
    // TYPE_TUPLE
            if (pr_check(',',1)) {
                List lst = { 0, NULL };
                if (!parser_list_comma(&lst, parser_type_, sizeof(Type))) {
                    return 0;
                }
                *ret = (Type) { TYPE_TUPLE, .Tuple={lst.size,lst.vec} };
            }
    // TYPE_PARENS
            if (!pr_accept(')',1)) {
                return err_expected("`)`");
            }
        }
    // TYPE_DATA
    } else if (pr_accept(TK_IDDATA,1)) {
        *ret = (Type) { TYPE_DATA, .Data=PRV.tk };
    } else {
        return err_expected("type");
    }

    // TYPE_FUNC
    if (pr_accept(TK_ARROW,1)) {
        Type tp;
        if (parser_type(&tp)) {
            Type* inp = malloc(sizeof(*inp));
            Type* out = malloc(sizeof(*out));
            assert(inp != NULL);
            assert(out != NULL);
            *inp = *ret;
            *out = tp;
            *ret = (Type) { TYPE_FUNC, .Func={inp,out} };
        }
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int parser_patt_ (void** patt) {
    static Patt pt_;
    Patt pt;
    if (!parser_patt(&pt)) {
        return 0;
    }
    pt_ = pt;
    *patt = &pt_;
    return 1;
}

int parser_patt (Patt* ret) {
    // PATT_UNIT
    if (pr_accept('(',1)) {
        if (pr_accept(')',1)) {
            *ret = (Patt) { PATT_UNIT, {} };
        } else {
            if (!parser_patt(ret)) {
                return 0;
            }
    // PATT_TUPLE
            if (pr_check(',',1)) {
                List lst = { 0, NULL };
                if (!parser_list_comma(&lst, parser_patt_, sizeof(Patt))) {
                    return 0;
                }
                *ret = (Patt) { PATT_TUPLE, .Tuple={lst.size,lst.vec} };
            }
    // PATT_PARENS
            if (!pr_accept(')',1)) {
                return err_expected("`)`");
            }
        }

    // PATT_ANY
    } else if (pr_accept('_',1)) {
        *ret = (Patt) { PATT_ANY };

    // PATT_CONS
    } else if (pr_accept(TK_IDDATA,1)) {
        *ret = (Patt) { PATT_CONS, .Cons={PRV.tk,NULL} };
        if (pr_check('(',1)) {
    // PATT_CONS(...)
            Patt arg;
            if (!parser_patt(&arg)) {
                return 0;
            }
            Patt* parg = malloc(sizeof(arg));
            assert(parg != NULL);
            *parg = arg;
            *ret = (Patt) { PATT_CONS, .Cons={ret->Cons.data,parg} };
        }
    // PATT_SET
    } else if (pr_accept('=',1)) {
        if (!pr_accept(TK_IDVAR,1)) {
            return err_expected("variable identifier");
        }
        *ret = (Patt) { PATT_SET, .Set=PRV.tk };
    }
    return 1;
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
    int lst_ok = pr_check(':', 1);
    if (lst_ok) {
        if (!parser_list_line(&lst, &parser_cons, sizeof(Cons))) {
            return 0;
        }
    }

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
            strcpy(dt.tk.val.s, id.val.s);
            ret->vec[0] = dt;
        }
    }

    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int parser_decl (Decl* decl) {
    if (!pr_accept(TK_VAR,1)) {
        return err_expected("`var`");
    }

    if (!pr_accept(TK_IDVAR,1)) {
        return err_expected("variable identifier");
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
        if (!parser_where(&set.decls)) {
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
    if (!parser_list_line(&lst, &parser_decl_, sizeof(Decl))) {
        return 0;
    }
    *ret = (Decls) { lst.size, lst.vec };
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int parser_expr_ (void** expr) {
    static Expr e_;
    Expr e;
    if (!parser_expr(&e)) {
        return 0;
    }
    e_ = e;
    *expr = &e_;
    return 1;
}

int parser_expr__ (void** expr) {
    return parser_expr_(expr) &&
           parser_where(&((*((Expr**)expr))->decls));
}

int parser_where (Decls** ds) {
    if (!pr_accept(TK_WHERE,1)) {
        *ds = NULL;
        return 1;
    }
    *ds = malloc(sizeof(*ds));
    return parser_decls(*ds);
}

int parser_case (void** casi) {
    static Case c;

    // patt
    if (pr_accept(TK_ELSE,1)) {
        c.patt = (Patt) { PATT_ANY, {} };
    } else if (!parser_patt(&c.patt)) {
        return 0;
    }

    // ->
    pr_accept(TK_ARROW,1);       // optional

    // expr
    Expr e;
    if (!parser_expr(&e)) {
        return 0;
    }
    if (!parser_where(&e.decls)) {
        return 0;
    }

    Expr* pe = malloc(sizeof(*pe));
    assert(pe != NULL);
    *pe = e;
    c.expr = pe;

    *casi = &c;
    return 1;
}

int parser_expr_one (Expr* ret) {
    // EXPR_UNIT
    if (pr_accept('(',1)) {
        if (pr_accept(')',1)) {
            *ret = (Expr) { EXPR_UNIT, NULL, {} };
        } else {
            if (!parser_expr(ret)) {
                return 0;
            }
    // EXPR_TUPLE
            if (pr_check(',',1)) {
                List lst = { 0, NULL };
                if (!parser_list_comma(&lst, parser_expr_, sizeof(Expr))) {
                    return 0;
                }
                *ret = (Expr) { EXPR_TUPLE, NULL, .Tuple={lst.size,lst.vec} };
            }
    // EXPR_PARENS
            if (!pr_accept(')',1)) {
                return err_expected("`)`");
            }
        }

    // EXPR_ARG
    } else if (pr_accept(TK_ARG,1)) {
        *ret = (Expr) { EXPR_ARG, NULL, {} };

    // EXPR_VAR
    } else if (pr_accept(TK_IDVAR,1)) {
        *ret = (Expr) { EXPR_VAR, NULL, .Var=PRV.tk };

    // EXPR_CONS
    } else if (pr_accept(TK_IDDATA,1)) {
        *ret = (Expr) { EXPR_CONS, NULL, .Cons=PRV.tk };

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
        *ret = (Expr) { EXPR_SET, NULL, .Set={var,pe} };

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
        *ret = (Expr) { EXPR_FUNC, NULL, .Func={tp,pe} };
        if (!parser_where(&ret->decls)) {
            return 0;
        }

    // EXPR_SEQ
    } else if (pr_check(':',1)) {
        List lst;
        if (!parser_list_line(&lst, &parser_expr__, sizeof(Expr))) {
            return 0;
        }
        *ret = (Expr) { EXPR_SEQ, NULL, .Seq={lst.size,lst.vec} };

    // EXPR_CASES
    } else if (pr_accept(TK_CASE,1)) {
        Expr e;
        if (!parser_expr(&e)) {
            return 0;
        }

        List lst;
        if (!parser_list_line(&lst, &parser_case, sizeof(Case))) {
            return 0;
        }

        Expr* pe = malloc(sizeof(*pe));
        assert(pe != NULL);
        *pe = e;

        *ret = (Expr) { EXPR_CASES, NULL, .Cases={pe,lst.size,lst.vec} };
    }

    return 1;
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
        *ret = (Expr) { EXPR_CALL, NULL, .Call={pe1,pe2} };
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

    if (pr_check(TK_DATA,1)) {
        if (!parser_data(&g.data)) {
            return 0;
        }
        g.sub = GLOB_DATA;
        *glob = &g;
        return 1;
    }

    if (pr_check(TK_VAR,1)) {
        if (!parser_decl(&g.decl)) {
            return 0;
        }
        g.sub = GLOB_DECL;
        *glob = &g;
        return 1;
    }

    if (!parser_expr(&g.expr)) {
        return 0;
    }
    g.sub = GLOB_EXPR;
    *glob = &g;
    return 1;
}

int parser_prog (Prog* ret) {
    List lst;
    if (!parser_list_line(&lst, &parser_glob, sizeof(Glob))) {
        return 0;
    }
    *ret = (Prog) { lst.size, lst.vec };

    if (!pr_accept(TK_EOF,1)) {
        return err_expected("end of file");
    }
    return 1;
}
