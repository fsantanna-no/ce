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

int is_rec (const char* v) {
    for (int i=0; i<ALL.data_recs.size; i++) {
        if (!strcmp(ALL.data_recs.buf[i].val.s, v)) {
            return 1;
        }
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

void pr_next () {
    PRV = NXT;

    long off = ftell(ALL.inp);
    Tk   tk  = lexer();

    // skip comments
    while (tk.sym == TK_COMMENT) {
        tk = lexer();
    }

    int lns = 0;
    while (PRV.tk.sym==TK_LINE && tk.sym==TK_LINE && tk.val.n==0) {
        tk = lexer();
        lns++;  // skip empty line
    }

    if (PRV.off == -1) {
        NXT.lin = lns + 1;
        NXT.col = 1;
    } else {
        if (PRV.tk.sym == TK_LINE) {
            NXT.lin = PRV.lin + lns + 1;
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
    ALL = (State_All) { out,inp,{},0,{0,{}} };
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

typedef void* (*List_F) (void);

int parser_list_comma (List* ret, void* fst, List_F f, size_t unit) {
    if (!pr_accept(',',1)) {
        return err_expected("`,`");
    }

    void* vec = malloc(unit);
    memcpy(vec, fst, unit);
    int i = 1;
    while (1) {
        void* item = f();
        if (item == NULL) {
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

int parser_list_line (int doind, List* ret, List_F f, size_t unit) {
    if (doind) {
        if (!pr_accept(':',1)) {
            return err_expected("`:`");
        }
        ALL.ind++;

        if (!pr_accept(TK_LINE, NXT.tk.val.n==ALL.ind)) {
            return err_unexpected("indentation level");
        }
    }

    void* vec = NULL;
    int i = 0;
    while (1) {
        void* item = f();;
        if (item == NULL) {
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

    if (doind) {
        ALL.ind--;
    }
    ret->size = i;
    ret->vec  = vec;
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

void* parser_type_ (void) {
    static Type tp_;
    Type tp;
    if (!parser_type(&tp)) {
        return NULL;
    }
    tp_ = tp;
    return &tp_;
}

int parser_type (Type* ret) {
    // EXPR_RAW
    if (pr_accept(TK_RAW,1)) {
        *ret = (Type) { TYPE_RAW, .Raw=PRV.tk };

    // TYPE_UNIT
    } else if (pr_accept('(',1)) {
        if (pr_accept(')',1)) {
            *ret = (Type) { TYPE_UNIT, {} };
        } else {
            if (!parser_type(ret)) {
                return 0;
            }
    // TYPE_TUPLE
            if (pr_check(',',1)) {
                List lst = { 0, NULL };
                if (!parser_list_comma(&lst, ret, parser_type_, sizeof(Type))) {
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

void* parser_patt_ (void) {
    static Patt pt_;
    Patt pt;
    if (!parser_patt(&pt)) {
        return NULL;
    }
    pt_ = pt;
    return &pt_;
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
                if (!parser_list_comma(&lst, ret, parser_patt_, sizeof(Patt))) {
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

void* parser_cons_ (void) {
    static Cons c_;
    Cons c;

    if (!pr_accept(TK_IDDATA,1)) {
        err_expected("data identifier");
        return NULL;
    }
    c.tk = PRV.tk;

    if (!parser_type(&c.type)) {
        return NULL;
    }

    c_ = c;
    return &c_;
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
    int tp_ok = parser_type(&tp);

    List lst = { 0, NULL };
    int lst_ok = pr_check(':', 1);
    if (lst_ok) {
        if (!parser_list_line(1, &lst, &parser_cons_, sizeof(Cons))) {
            return 0;
        }
    }

    if (!tp_ok && !lst_ok) {
        // recursive pre declaration
        assert(ALL.data_recs.size < sizeof(ALL.data_recs.buf));
        ALL.data_recs.buf[ALL.data_recs.size++] = id;
        *ret = (Data) { id, 0, NULL };
        return 1;
    }

    *ret = (Data) { id, lst.size, lst.vec };
    for (int i=0; i<ret->size; i++) {
        ret->vec[i].idx = i;
    }

    // mark each Cons as is_rec as well
    if (is_rec(id.val.s)) {
        for (int i=0; i<ret->size; i++) {
            ALL.data_recs.buf[ALL.data_recs.size++] = ret->vec[i].tk;
        }
    }

    // realloc TP more spaces inside each CONS
    if (tp_ok) {
        if (lst_ok) {
            assert(0 && "TODO");
        } else {
            ret->size = 1;
            ret->vec  = malloc(sizeof(Cons));

            Cons c = (Cons) { 0, {}, tp };
            strcpy(c.tk.val.s, id.val.s);
            ret->vec[0] = c;
        }
    }

    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int parser_decl (Decl* decl) {
    if (!pr_accept(TK_MUT,1) && !pr_accept(TK_VAL,1)) {
        return err_expected("`mut` or `val`");
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

void* parser_decl_ (void) {
    static Decl d_;
    Decl d;
    if (!parser_decl(&d)) {
        return NULL;
    }
    d_ = d;
    return &d_;
}

int parser_decls (Decls* ret) {
    List lst;
    if (!parser_list_line(1, &lst, &parser_decl_, sizeof(Decl))) {
        return 0;
    }
    *ret = (Decls) { lst.size, lst.vec };
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

void* parser_expr_ (void) {
    static Expr e_;
    Expr e;
    if (!parser_expr(&e)) {
        return NULL;
    }
    e_ = e;
    return &e_;
}

void* parser_expr__ (void) {
    static Expr e_;
    Expr* pe = parser_expr_();
    if (pe == NULL) {
        return NULL;
    }
    Expr e = *pe;
    if (!parser_where(&e.decls)) {
        return 0;
    }
    e_ = e;
    return &e_;
}

int parser_where (Decls** ds) {
    if (!pr_accept(TK_WHERE,1)) {
        *ds = NULL;
        return 1;
    }
    *ds = malloc(sizeof(*(*ds)));
    return parser_decls(*ds);
}

void* parser_case_ (void) {
    static Case c_;
    Case c;

    // patt
    if (pr_accept(TK_ELSE,1)) {
        c.patt = (Patt) { PATT_ANY, {} };
    } else if (!parser_patt(&c.patt)) {
        return NULL;
    }

    // decls
    if (pr_accept(TK_DECL,1)) {
        Type tp;
        if (!parser_type(&tp)) {
            return 0;
        }
        c.type = malloc(sizeof(tp));
        *c.type = tp;
    } else {
        c.type = NULL;
    }

    // ->
    pr_accept(TK_ARROW,1);       // optional

    // expr
    Expr e;
    if (!parser_expr(&e)) {
        return NULL;
    }
    if (!parser_where(&e.decls)) {
        return NULL;
    }

    Expr* pe = malloc(sizeof(*pe));
    assert(pe != NULL);
    *pe = e;
    c.expr = pe;

    c_ = c;
    return &c_;
}

int parser_expr_one (Expr* ret) {
    int is_line = (PRV.tk.sym==TK_LINE);

    // EXPR_RAW
    if (pr_accept(TK_RAW,1)) {
        *ret = (Expr) { EXPR_RAW, NULL, .Raw=PRV.tk };

    // EXPR_UNIT
    } else if (pr_accept('(',1)) {
        if (pr_accept(')',1)) {
            *ret = (Expr) { EXPR_UNIT, NULL, {} };
        } else {
            if (!parser_expr(ret)) {
                return 0;
            }
    // EXPR_TUPLE
            if (pr_check(',',1)) {
                List lst = { 0, NULL };
                if (!parser_list_comma(&lst, ret, parser_expr_, sizeof(Expr))) {
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

    // EXPR_NEW
    } else if (pr_accept(TK_NEW,1)) {
        Expr e;
        if (!parser_expr(&e)) {
            return 0;
        }
        Expr* pe = malloc(sizeof(*pe));
        assert(pe != NULL);
        *pe = e;
        *ret = (Expr) { EXPR_NEW, NULL, .New=pe };

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
        if (!parser_list_line(1, &lst, &parser_expr__, sizeof(Expr))) {
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
        if (!parser_list_line(1, &lst, &parser_case_, sizeof(Case))) {
            return 0;
        }

        Expr* pe = malloc(sizeof(*pe));
        assert(pe != NULL);
        *pe = e;

        *ret = (Expr) { EXPR_CASES, NULL, .Cases={pe,lst.size,lst.vec} };

    // EXPR_CALL
    } else if (pr_accept(TK_CALL,1)) {
        if (!is_line) {
            return err_unexpected("`call`");    // use `call` only starting line
        }
        Expr func, arg;
        if (!parser_expr_one(&func) || !parser_expr(&arg)) {
            return 0;
        }
        Expr* p1 = malloc(sizeof(*p1));
        Expr* p2 = malloc(sizeof(*p2));
        assert(p1!=NULL && p2!=NULL);
        *p1 = func;
        *p2 = arg;
        *ret = (Expr) { EXPR_CALL, NULL, .Call={p1,p2} };
    } else {
        return err_expected("expression");
    }

    return 1;
}

int parser_expr (Expr* ret) {
    int is_line = (PRV.tk.sym==TK_LINE);

    Expr e;
    if (!parser_expr_one(&e)) {
        return 0;
    }
    *ret = e;

    if (pr_check('(',1) && is_line && e.sub!=EXPR_CALL) {
        sprintf(ALL.err,
            "(ln %ld, col %ld): expected `call` at the beginning of line",
            NXT.lin, NXT.col
        );
        return 0;
    }

    // CALLS
    while (1) {
        if (!pr_check('(',1)) {
            break;
        }
        Expr arg;
        if (!parser_expr(&arg)) {
            return 0;
        }

        Expr* pe1 = malloc(sizeof(*pe1));
        Expr* pe2 = malloc(sizeof(*pe2));
        assert(pe1!=NULL && pe2!=NULL);
        *pe1 = *ret;
        *pe2 = arg;
        *ret = (Expr) { EXPR_CALL, NULL, .Call={pe1,pe2} };
    }

    return 1;
}

///////////////////////////////////////////////////////////////////////////////

// Glob ::= Data | Decl | Expr
// Prog ::= { Glob }

void* parser_glob_ (void) {
    static Glob g_;
    Glob g;

    if (pr_check(TK_DATA,1)) {
        if (!parser_data(&g.data)) {
            return NULL;
        }
        g.sub = GLOB_DATA;
        g_ = g;
        return &g_;
    }

    if (pr_check(TK_MUT,1) || pr_check(TK_VAL,1)) {
        if (!parser_decl(&g.decl)) {
            return NULL;
        }
        g.sub = GLOB_DECL;
        g_ = g;
        return &g_;
    }

    Expr* e = parser_expr__();  // other parser_expr* variations do not parse where
    if (e == NULL) {
        return NULL;
    }
    g.expr = *e;
    g.sub = GLOB_EXPR;
    g_ = g;
    return &g_;
}

int parser_prog (Prog* ret) {
    List lst;
    if (!parser_list_line(0, &lst, &parser_glob_, sizeof(Glob))) {
        return 0;
    }
    *ret = (Prog) { lst.size, lst.vec };

    if (!pr_accept(TK_EOF,1)) {
        return err_expected("end of file");
    }
    return 1;
}
