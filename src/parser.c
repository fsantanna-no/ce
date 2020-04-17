#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

///////////////////////////////////////////////////////////////////////////////

void dump_expr_ (Expr e, int spc) {
    for (int i=0; i<spc; i++) printf(" ");
    switch (e.sub) {
        case EXPR_RAW:
            printf("{ %s }", e.Raw.val.s);
            break;
        case EXPR_UNIT:
            fputs("()", stdout);
            break;
        case EXPR_VAR:
            fputs(e.Var.val.s, stdout);
            break;
        case EXPR_CONS:
            fputs(e.Var.val.s, stdout);
            break;
        case EXPR_SET:
            puts("set");
            //fputs(e.Set.var.val.s, stdout);
            //fputs(" = ", stdout);
            //dump_expr_(*e.Set.val, 0);
            break;
        case EXPR_NEW:
            fputs("new (...)", stdout);
            break;
        case EXPR_FUNC:
            fputs("func (...)", stdout);
            break;
        case EXPR_RETURN:
            fputs("return ", stdout);
            dump_expr_(*e.Return, 0);
            break;
        case EXPR_BREAK:
            fputs("break ", stdout);
            if (e.Break) {
                dump_expr_(*e.Break, 0);
            }
            break;
        case EXPR_LOOP: {
            puts("loop:");
            dump_expr_(*e.Loop, spc+4);
            break;
        }
        case EXPR_MATCH:
            dump_expr_(*e.Match.expr, 0);
            fputs(" ~ ???", stdout);
            break;
        case EXPR_TUPLE:
            fputs("<", stdout);
            for (int i=0; i<e.Tuple.size; i++) {
                if (i>0) fputs(", ", stdout);
                dump_expr_(e.Tuple.vec[i], 0);
            }
            fputs(">", stdout);
            break;
        case EXPR_CALL:
            dump_expr_(*e.Call.func, 0);
            fputs("(", stdout);
            dump_expr_(*e.Call.arg, 0);
            fputs(")", stdout);
            break;
        case EXPR_SEQ:
            printf(": [%d]\n", e.Seq.size);
            for (int i=0; i<e.Seq.size; i++) {
                dump_expr_(e.Seq.vec[i], spc+4);
                puts("");
            }
            break;
        case EXPR_LET:
            fputs("let (...)", stdout);
            break;
        case EXPR_IF:
            fputs("if (...)", stdout);
            break;
        case EXPR_CASES:
            fputs("match (...)", stdout);
            break;
        default:
            printf(">>> %d\n", e.sub);
            assert(0 && "TODO");
    }
}

void dump_expr (Expr e) {
    dump_expr_(e, 0);
    puts("");
}

int is_rec (const char* v) {
    for (int i=0; i<ALL.data_recs.size; i++) {
        if (!strcmp(ALL.data_recs.buf[i].val.s, v)) {
            return 1;
        }
    }
    return 0;
}

Expr* expr_new (Decl** env) {
    Expr e;
    if (!parser_expr(env,&e)) {
        return NULL;
    }
    Expr* pe = malloc(sizeof(Expr));
    assert(pe != NULL);
    *pe = e;
    return pe;
}

///////////////////////////////////////////////////////////////////////////////

void pr_read (State_Tok* tok) {
    tok->off = ftell(ALL.inp);
    tok->tk  = lexer();

    // skip comment
    if (tok->tk.sym == TK_COMMENT) {
        tok->off = ftell(ALL.inp);
        tok->tk  = lexer();
    }
}

void pr_lincol (void) {
    if (TOK1.tk.sym == '\n') {
        TOK2.lin = TOK1.lin + 1;
        TOK2.col = (TOK2.off - TOK1.off);
    } else {
        TOK2.col = TOK1.col + (TOK2.off - TOK1.off);
    }
}

void pr_init () {
    TOK0 = (State_Tok) { -1,0,0,{} };
    TOK1 = (State_Tok) { -1,1,1,{} };
    TOK2 = (State_Tok) { -1,1,1,{} };
    pr_read(&TOK1);
    pr_read(&TOK2);
    pr_lincol();
}

void pr_next () {
    TOK0 = TOK1;
    TOK1 = TOK2;
    pr_read(&TOK2);
    pr_lincol();
}

int pr_accept1 (TK tk) {
    if (TOK1.tk.sym==tk) {
        pr_next();
        return 1;
    } else {
        return 0;
    }
}

#if 0
int pr_accept2 (TK tk, int ok) {
    if (TOK2.tk.sym==tk && ok) {
        pr_next();
        pr_next();
        return 1;
    } else {
        return 0;
    }
}
#endif

int pr_check0 (TK tk) {
    return (TOK0.tk.sym == tk);
}

int pr_check1 (TK tk) {
    return (TOK1.tk.sym == tk);
}

#if 0
int pr_check2 (TK tk) {
    return (TOK2.tk.sym == tk);
}
#endif

///////////////////////////////////////////////////////////////////////////////

int err_expected (const char* v) {
    sprintf(ALL.err, "(ln %ld, col %ld): expected %s : have %s",
        TOK1.lin, TOK1.col, v, lexer_tk2str(&TOK1.tk));
    return 0;
}

int err_unexpected (const char* v) {
    sprintf(ALL.err, "(ln %ld, col %ld): unexpected %s", TOK1.lin, TOK1.col, v);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

FILE* stropen (const char* mode, size_t size, char* str) {
    size = (size != 0) ? size : strlen(str);
    return fmemopen(str, size, mode);
}

void init (FILE* out, FILE* inp) {
    static char buf1[65000];
    static char buf2[65000];
    FILE* out1 = stropen("w", sizeof(buf1), buf1);
    FILE* out2 = stropen("w", sizeof(buf2), buf2);
    ALL = (State_All) { inp,{out,out1,out2},{},0,{0,{}} };
    if (inp != NULL) {
        pr_init();
    }
}

///////////////////////////////////////////////////////////////////////////////

int parser_list_comma (Decl** env, List* ret, void* fst, List_F f, size_t unit) {
    if (!pr_accept1(',')) {
        return err_expected("`,`");
    }

    void* vec = malloc(unit);
    memcpy(vec, fst, unit);
    int i = 1;
    while (1) {
        void* item = f(env);
        if (item == NULL) {
            return 0;
        }
        vec = realloc(vec, (i+1)*unit);
        memcpy(vec+i*unit, item, unit);
        i++;
        if (!pr_accept1(',')) {
            break;
        }
    }

    ret->size = i;
    ret->vec  = vec;
    return 1;
}

int parser_list_line (Decl** env, int global, List* ret, List_F f, size_t unit) {
    if (global) {
        if (!pr_accept1(':')) {
            return err_expected("`:`");
        }
        if (!pr_check1('\n')) {
            return err_expected("new line");
        }
        ALL.ind += 4;
    }

    void* vec = NULL;
    int i = 0;
    while (1) {
        while (pr_accept1('\n'));    // skip line + empty lines

        if (pr_accept1(EOF)) {
            break;  // unnest
        } else if (TOK1.lin==1 && TOK1.col==1) {
            // ok
        } else if (pr_check0('\n') && TOK0.tk.val.n==ALL.ind) {
            // ok
        } else if (i>0 && pr_check0('\n') && TOK0.tk.val.n<ALL.ind) {
            break;  // unnest
        } else if (pr_check0('\n')) {
            char s[256];
            pr_accept1('\n');
            sprintf(s, "indentation of %d spaces", ALL.ind);
            return err_expected(s);
        } else {
            sprintf(ALL.err, "(ln %ld, col %ld): expected new line : have %s",
                TOK1.lin, TOK1.col, lexer_tk2str(&TOK1.tk));
            return 0;
        }

        // COMMENT
        if (pr_accept1(TK_COMMENT)) {
            continue;
        }

        // ITEM
        void* item = f(env);;
        if (item == NULL) {
            return 0;
        }
        vec = realloc(vec, (i+1)*unit);
        memcpy(vec+i*unit, item, unit);
        i++;
    }

    if (global) {
        ALL.ind -= 4;
    }
    ret->size = i;
    ret->vec  = vec;
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

void* parser_type_ (Decl** env) {
    static Type tp_;
    Type tp;
    if (!parser_type(&tp)) {
        return NULL;
    }
    tp_ = tp;
    return &tp_;
}

int parser_type (Type* ret) {
    // TYPE_RAW
    if (pr_accept1(TK_RAW)) {
        *ret = (Type) { TYPE_RAW, .Raw=TOK0.tk };

    // TYPE_UNIT
    } else if (pr_accept1('(')) {
        if (pr_accept1(')')) {
            *ret = (Type) { TYPE_UNIT, {} };
        } else {
            if (!parser_type(ret)) {
                return 0;
            }

    // TYPE_FUNC
            if (pr_accept1(TK_ARROW)) {
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

    // TYPE_TUPLE
            if (pr_check1(',')) {
                List lst = { 0, NULL };
                if (!parser_list_comma(NULL, &lst, ret, parser_type_, sizeof(Type))) {
                    return 0;
                }
                *ret = (Type) { TYPE_TUPLE, .Tuple={lst.size,lst.vec} };
            }
    // TYPE_PARENS
            if (!pr_accept1(')')) {
                return err_expected("`)`");
            }
        }
    // TYPE_DATA
    } else if (pr_accept1(TK_IDDATA)) {
        *ret = (Type) { TYPE_DATA, .Data=TOK0.tk };
    } else {
        return err_expected("type");
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

void* parser_patt_ (Decl** env) {
    static Patt pt_;
    Patt pt;
    if (!parser_patt(*env,&pt,0)) {
        return NULL;
    }
    pt_ = pt;
    return &pt_;
}

int parser_patt (Decl* env, Patt* ret, int is_match) {
    // PATT_RAW
    if (pr_accept1(TK_RAW)) {
        *ret = (Patt) { PATT_RAW, .Raw=TOK0.tk };
    // PATT_UNIT
    } else if (pr_accept1('(')) {
        if (pr_accept1(')')) {
            *ret = (Patt) { PATT_UNIT, {} };
        } else {
            if (!parser_patt(env,ret,is_match)) {
                return 0;
            }
    // PATT_TUPLE
            if (pr_check1(',')) {
                List lst = { 0, NULL };
                if (!parser_list_comma(&env, &lst, ret, parser_patt_, sizeof(Patt))) {
                    return 0;
                }
                *ret = (Patt) { PATT_TUPLE, .Tuple={lst.size,lst.vec} };
            }
    // PATT_PARENS
            if (!pr_accept1(')')) {
                return err_expected("`)`");
            }
        }
    // PATT_ANY
    } else if (pr_accept1('_')) {
        *ret = (Patt) { PATT_ANY };
    // PATT_CONS
    } else if (pr_accept1(TK_IDDATA)) {
        *ret = (Patt) { PATT_CONS, .Cons={TOK0.tk,NULL} };
        if (pr_check1('(')) {
    // PATT_CONS(...)
            Patt arg;
            if (!parser_patt(env,&arg,is_match)) {
                return 0;
            }
            Patt* parg = malloc(sizeof(arg));
            assert(parg != NULL);
            *parg = arg;
            *ret = (Patt) { PATT_CONS, .Cons={ret->Cons.data,parg} };
        }
    // PATT_SET
    } else if (pr_accept1(TK_IDVAR)) {
        int size = -1;
        if (pr_accept1('[')) {
            if (!pr_accept1(']')) {
                return err_expected("`]`");
            }
            size = 0;
        }

        if (is_match) {
            Expr e = (Expr) { EXPR_VAR, {}, env, NULL, .Var=TOK0.tk };
            Expr* pe = malloc(sizeof(Expr));
            assert(pe != NULL);
            *pe = e;
            *ret = (Patt) { PATT_EXPR, .Expr=pe };
        } else {
            *ret = (Patt) { PATT_SET, .Set={TOK0.tk,size} };
        }
    } else if (pr_accept1('~')) {
        Expr* pe = expr_new(&env);
        if (pe == NULL) {
            return 0;
        }
        *ret = (Patt) { PATT_EXPR, .Expr=pe };
    } else {
        return err_expected("variable identifier");
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

void* parser_cons_ (Decl** env) {
    static Cons c_;
    Cons c;

    if (!pr_accept1(TK_IDDATA)) {
        err_expected("data identifier");
        return NULL;
    }
    c.tk = TOK0.tk;

    if (!parser_type(&c.type)) {
        c.type = (Type) { TYPE_UNIT, {} };
    }

    c_ = c;
    return &c_;
}

int parser_data (Data* ret) {
    if (!pr_accept1(TK_DATA)) {
        return err_expected("`data`");
    }
    if (!pr_accept1(TK_IDDATA)) {
        return err_expected("data identifier");
    }
    Tk id = TOK0.tk;

    Type tp;
    int tp_ok = parser_type(&tp);

    List lst = { 0, NULL };
    int lst_ok = pr_check1(':');
    if (lst_ok) {
        if (!parser_list_line(NULL, 1, &lst, &parser_cons_, sizeof(Cons))) {
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

int parser_decl (Decl** env, Decl* decl) {
    int is_func = pr_check0(TK_FUNC);
    if (!parser_patt(*env,&decl->patt,0)) {
        return 0;
    }

    if (!pr_accept1(TK_DECL)) {
        return err_expected("`::`");
    }

    if (!parser_type(&decl->type)) {
        return 0;
    }

    if (is_func || (!is_func && pr_accept1('='))) {
        Expr init;
        if (!parser_expr(env,&init)) {
            return is_func;
        }
        decl->init = malloc(sizeof(*decl->init));
        assert(decl->init != NULL);
        *decl->init = init;
    } else {
        decl->init = NULL;
    }

    decl->prev = (struct Decl*) *env;
    *env = decl;
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

void* parser_expr_ (Decl** env) {
    static Expr e_;
    Expr e;
    if (!parser_expr(env,&e)) {
        return NULL;
    }
    e_ = e;
    return &e_;
}

void* parser_expr__ (Decl** env) {
    static Expr e_;
    State_Tok tok = TOK1;
    Expr* pe = parser_expr_(env);
    if (pe == NULL) {
        return NULL;
    }
    pe->tok = tok;
    Expr e = *pe;
    e_ = e;
    return &e_;
}

void* parser_case_ (Decl** env) {
    static Let let_;
    Let let;

    // patt
    if (pr_accept1(TK_ELSE)) {
        let.decl.patt = (Patt) { PATT_ANY, {} };
    } else if (!parser_patt(*env,&let.decl.patt,0)) {
        return NULL;
    }

    // decls
    if (pr_accept1(TK_DECL)) {
        if (!parser_type(&let.decl.type)) {
            return 0;
        }
    } else {
        let.decl.type.sub = TYPE_NONE;
    }

    // ->
    pr_accept1(TK_ARROW);       // optional

    // expr
    Expr* pe = expr_new(env);
    if (pe == NULL) {
        return NULL;
    }

    let.body = pe;
    let_ = let;
    return &let_;
}

void* parser_if_ (Decl** env) {
    static If c_;
    If c;

    if (pr_accept1(TK_ELSE)) {
        c.tst = malloc(sizeof(Expr));
        assert(c.tst != NULL);
        *c.tst = (Expr) { EXPR_RAW, {}, *env, NULL, .Raw={TK_RAW,{.s="1"}} };
    } else {
        c.tst = expr_new(env);
        if (c.tst == NULL) {
            return NULL;
        }
    }

    // ->
    pr_accept1(TK_ARROW);       // optional

    // expr
    c.ret = expr_new(env);
    if (c.ret == NULL) {
        return NULL;
    }

    c_ = c;
    return &c_;
}

int parser_expr_one (Decl** env, Expr* ret) {
    // EXPR_RAW
    if (pr_accept1(TK_RAW)) {
        *ret = (Expr) { EXPR_RAW, {}, *env, NULL, .Raw=TOK0.tk };

    // EXPR_UNIT
    } else if (pr_accept1('(')) {
        if (pr_accept1(')')) {
            *ret = (Expr) { EXPR_UNIT, {}, *env, NULL, {} };
        } else {
            if (!parser_expr(env,ret)) {
                return 0;
            }
    // EXPR_TUPLE
            if (pr_check1(',')) {
                List lst = { 0, NULL };
                if (!parser_list_comma(env, &lst, ret, parser_expr_, sizeof(Expr))) {
                    return 0;
                }
                *ret = (Expr) { EXPR_TUPLE, {}, *env, NULL, .Tuple={lst.size,lst.vec} };
            }
    // EXPR_PARENS
            if (!pr_accept1(')')) {
                return err_expected("`)`");
            }
        }

    // EXPR_ARG
    } else if (pr_accept1(TK_ARG)) {
        *ret = (Expr) { EXPR_ARG, {}, *env, NULL, {} };

    // EXPR_VAR
    } else if (pr_accept1(TK_IDVAR)) {
        *ret = (Expr) { EXPR_VAR, {}, *env, NULL, .Var=TOK0.tk };

    // EXPR_CONS
    } else if (pr_accept1(TK_IDDATA)) {
        *ret = (Expr) { EXPR_CONS, {}, *env, NULL, .Cons=TOK0.tk };

    // EXPR_NEW
    } else if (pr_accept1(TK_NEW)) {
        Expr* pe = expr_new(env);
        if (pe == NULL) {
            return 0;
        }
        *ret = (Expr) { EXPR_NEW, {}, *env, NULL, .New=pe };

    // EXPR_SET
    } else if (pr_accept1(TK_SET)) {
        Patt p;
        if (!parser_patt(*env,&p,0)) {
            return 0;
        }
        if (!pr_accept1('=')) {
            return err_expected("`=`");
        }
        Expr* e = expr_new(env);
        if (e == NULL) {
            return 0;
        }
        *ret = (Expr) { EXPR_SET, {}, *env, NULL, .Set={p,e} };

    // EXPR_DECL
    } else if (pr_accept1(TK_MUT) || pr_accept1(TK_VAL)) {
        Decl d;
        if (!parser_decl(env,&d)) {
            return 0;
        }
        *ret = (Expr) { EXPR_DECL, {}, *env, NULL, .Decl=d };

    // EXPR_FUNC // EXPR_DECL_FUNC
    } else if (pr_accept1(TK_FUNC)) {
        Decl d;
        if (parser_decl(env,&d)) {
            *ret = (Expr) { EXPR_DECL, {}, *env, NULL, .Decl=d };
        } else if (pr_accept1(TK_DECL)) {
            Type tp;
            if (!parser_type(&tp)) {
                return 0;
            }
            Expr* pe = expr_new(env);
            if (pe == NULL) {
                return 0;
            }
            *ret = (Expr) { EXPR_FUNC, {}, *env, NULL, .Func={tp,pe} };
        } else {
            return err_expected("`::` or variable identifier");
        }

    // EXPR_PASS
    } else if (pr_accept1(TK_PASS)) {
        *ret = (Expr) { EXPR_PASS, {}, *env, NULL };

    // EXPR_RETURN
    } else if (pr_accept1(TK_RETURN)) {
        Expr* e = expr_new(env);
        if (e == NULL) {
            return 0;
        }
        *ret = (Expr) { EXPR_RETURN, {}, *env, NULL, .Return=e };

    // EXPR_SEQ
    } else if (pr_check1(':')) {
        List lst;
        if (!parser_list_line(env, 1, &lst, &parser_expr__, sizeof(Expr))) {
            return 0;
        }
        *ret = (Expr) { EXPR_SEQ, {}, *env, NULL, .Seq={lst.size,lst.vec} };

    // EXPR_LET
    } else if (pr_accept1(TK_LET)) {
        Decl d;
        if (!parser_decl(env,&d)) {
            return 0;
        }
        if (d.init == NULL) {
            return err_expected("`=`");
        }

        pr_accept1(TK_ARROW);  // optional `->`
        Expr* pe = expr_new(env);
        if (pe == NULL) {
            return 0;
        }

        *ret = (Expr) { EXPR_LET, {}, *env, NULL, .Let={d,pe} };

    } else if (pr_accept1(TK_IF)) {
    // EXPR_IFS
        if (pr_check1(':')) {
            List lst;
            if (!parser_list_line(env, 1, &lst, &parser_if_, sizeof(If))) {
                return 0;
            }
            *ret = (Expr) { EXPR_IFS, {}, *env, NULL, .Ifs={lst.size,lst.vec} };
    // EXPR_IF
        } else {
            Expr* tst = expr_new(env);
            if (tst == NULL) {
                return 0;
            }

            pr_accept1(TK_ARROW);  // optional `->`

            Expr* t = expr_new(env);
            if (tst == NULL) {
                return 0;
            }

            pr_accept1(TK_ARROW);  // optional `->`

            Expr* f = expr_new(env);
            if (tst == NULL) {
                return 0;
            }

            *ret = (Expr) { EXPR_IF, {}, *env, NULL, .Cond={tst,t,f} };
        }

    // EXPR_CASES
    } else if (pr_accept1(TK_CASE)) {
        Expr* pe = expr_new(env);
        if (pe == NULL) {
            return 0;
        }
        List lst;
        if (!parser_list_line(env, 1, &lst, &parser_case_, sizeof(Let))) {
            return 0;
        }
        *ret = (Expr) { EXPR_CASES, {}, *env, NULL, .Cases={pe,lst.size,lst.vec} };

    // EXPR_BREAK
    } else if (pr_accept1(TK_BREAK)) {
        Expr* e = expr_new(env);
        if (e == NULL) {
            return 0;
        }
        *ret = (Expr) { EXPR_BREAK, {}, *env, NULL, .Break=e };

    // EXPR_LOOP
    } else if (pr_accept1(TK_LOOP)) {
        Expr* pe = expr_new(env);
        if (pe == NULL) {
            return 0;
        }
        *ret = (Expr) { EXPR_LOOP, {}, *env, NULL, .Loop=pe };

    } else {
        return err_expected("expression");
    }

    assert(ret->where == NULL);

    return 1;
}

int parser_expr (Decl** env, Expr* ret) {
    int is_first = TOK0.off==-1 || pr_check0('\n') || pr_check0(TK_ARROW);
//printf(">>> (%d/%d/%d ==> %d/%d)\n", TOK0.tk.sym, TOK1.tk.sym, TOK2.tk.sym, ALL.ind, TOK0.tk.val.n);

    Expr e;
    if (!parser_expr_one(env, &e)) {
        return 0;
    }
    *ret = e;

    // cannot separate exprs with \n
    if (pr_check0('\n')) {
        goto _WHERE_;
    }

    // EXPR_CALL
    if (pr_check1('(')) {
        Expr arg;
        if (!parser_expr_one(env, &arg)) {
            return 0;
        }
        Expr* parg = malloc(sizeof(Expr));
        Expr* func = malloc(sizeof(Expr));
        assert(parg!=NULL && func!=NULL);
        *parg = arg;
        *func = *ret;
        *ret  = (Expr) { EXPR_CALL, {}, *env, NULL, .Call={func,parg} };
    }

    // EXPR_MATCH
    if (pr_accept1('~')) {
        Patt patt;
        if (!parser_patt(*env,&patt,1)) {
            return 0;
        }
        Expr* pe = malloc(sizeof(Expr));
        Patt* pp = malloc(sizeof(Patt));
        assert(pe!=NULL && pp!=NULL);
        *pe = *ret;
        *pp = patt;
        *ret = (Expr) { EXPR_MATCH, {}, *env, NULL, .Match={pe,pp} };
    }

    if (!is_first) {
        return 1;
    }

    // EXPR_IF
    if (pr_accept1(TK_IF)) {
        Expr* tst = expr_new(env);
        if (tst == NULL) {
            return 0;
        }
        If*   if_ = malloc(sizeof(If));
        Expr* xxx = malloc(sizeof(Expr));
        assert(if_!=NULL && xxx!=NULL);
        if_->tst = tst;
        if_->ret = xxx;
        *xxx = *ret;
        *ret = (Expr) { EXPR_IFS, {}, *env, NULL, .Ifs={1,if_} };
    }

_WHERE_:
    assert(ret->where == NULL);
    if (
        (pr_check0('\n') && TOK0.tk.val.n==ALL.ind && pr_accept1(TK_WHERE))
    ||
        (!pr_check0('\n') && pr_accept1(TK_WHERE))
    ) {
        ret->where = expr_new(env);
        return (ret->where != NULL);
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

// Glob ::= Data | Decl | Expr
// Prog ::= { Glob }

void* parser_glob_ (Decl** env) {
    static Glob g_;
    Glob g;

    if (pr_check1(TK_DATA)) {
        if (!parser_data(&g.data)) {
            return NULL;
        }
        g.sub = GLOB_DATA;
        g_ = g;
        return &g_;
    }

    Expr* e = parser_expr__(env);  // other parser_expr* variations do not parse where
    if (e == NULL) {
        return NULL;
    }
    g.expr = *e;
    g.sub = GLOB_EXPR;
    g_ = g;
    return &g_;
}

int parser_prog (Prog* ret) {
    Decl* env = NULL;
    List lst;
    if (!parser_list_line(&env, 0, &lst, &parser_glob_, sizeof(Glob))) {
        return 0;
    }
    *ret = (Prog) { lst.size, lst.vec };

    if (!pr_accept1(EOF)) {
        return err_expected("end of file");
    }
    return 1;
}
