#include "all.h"

void parser_init (void) {
    TOK0 = (State_Tok) { -1,0,0,{} };
    TOK1 = (State_Tok) { -1,1,1,{} };
    TOK2 = (State_Tok) { -1,1,1,{} };
    ll_read(&TOK1);
    ll_read(&TOK2);
    ll_lincol();
}

Expr* expr_new (Env** env) {
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

int parser_list_comma (Env** env, List* ret, void* fst, List_F f, size_t unit) {
    if (!ll_accept1(',')) {
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
        if (!ll_accept1(',')) {
            break;
        }
    }

    ret->size = i;
    ret->vec  = vec;
    return 1;
}

int parser_list_line (Env** env, int global, List* ret, List_F f, size_t unit) {
    if (global) {
        if (!ll_accept1(':')) {
            return err_expected("`:`");
        }
        if (!ll_check1('\n')) {
            return err_expected("new line");
        }
        ALL.ind += 4;
    }

    void* vec = NULL;
    int i = 0;
    while (1) {
        while (ll_accept1('\n'));    // skip line + empty lines

        if (ll_accept1(EOF)) {
            break;  // unnest
        } else if (TOK1.lin==1 && TOK1.col==1) {
            // ok
        } else if (ll_check0('\n') && TOK0.tk.val.n==ALL.ind) {
            // ok
        } else if (i>0 && ll_check0('\n') && TOK0.tk.val.n<ALL.ind) {
            break;  // unnest
        } else if (ll_check0('\n')) {
            char s[256];
            ll_accept1('\n');
            sprintf(s, "indentation of %d spaces", ALL.ind);
            return err_expected(s);
        } else {
            sprintf(ALL.err, "(ln %ld, col %ld): expected new line : have %s",
                TOK1.lin, TOK1.col, lexer_tk2str(&TOK1.tk));
            return 0;
        }

        // COMMENT
        if (ll_accept1(TK_COMMENT)) {
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

void* parser_type_ (Env** env) {
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
    if (ll_accept1(TK_RAW)) {
        *ret = (Type) { TYPE_RAW, .Raw=TOK0.tk };

    // TYPE_UNIT
    } else if (ll_accept1('(')) {
        if (ll_accept1(')')) {
            *ret = (Type) { TYPE_UNIT, {} };
        } else {
            if (!parser_type(ret)) {
                return 0;
            }

    // TYPE_FUNC
            if (ll_accept1(TK_ARROW)) {
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
            if (ll_check1(',')) {
                List lst = { 0, NULL };
                if (!parser_list_comma(NULL, &lst, ret, parser_type_, sizeof(Type))) {
                    return 0;
                }
                *ret = (Type) { TYPE_TUPLE, .Tuple={lst.size,lst.vec} };
            }
    // TYPE_PARENS
            if (!ll_accept1(')')) {
                return err_expected("`)`");
            }
        }
    // TYPE_DATA
    } else if (ll_accept1(TK_IDDATA)) {
        int size = -1;
        Tk tk0 = TOK0.tk;
        if (ll_accept1('[')) {
            if (!ll_accept1(']')) {
                return err_expected("`]`");
            }
            size = 0;
        }
        *ret = (Type) { TYPE_DATA, .Data={tk0,size} };
    } else {
        return err_expected("type");
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

void* parser_patt_ (Env** env) {
    static Patt pt_;
    Patt pt;
    if (!parser_patt(*env,&pt,0)) {
        return NULL;
    }
    pt_ = pt;
    return &pt_;
}

int parser_patt (Env* env, Patt* ret, int is_match) {
    // PATT_RAW
    if (ll_accept1(TK_RAW)) {
        *ret = (Patt) { PATT_RAW, .Raw=TOK0.tk };
    // PATT_UNIT
    } else if (ll_accept1('(')) {
        if (ll_accept1(')')) {
            *ret = (Patt) { PATT_UNIT, {} };
        } else {
            if (!parser_patt(env,ret,is_match)) {
                return 0;
            }
    // PATT_TUPLE
            if (ll_check1(',')) {
                List lst = { 0, NULL };
                if (!parser_list_comma(&env, &lst, ret, parser_patt_, sizeof(Patt))) {
                    return 0;
                }
                *ret = (Patt) { PATT_TUPLE, .Tuple={lst.size,lst.vec} };
            }
    // PATT_PARENS
            if (!ll_accept1(')')) {
                return err_expected("`)`");
            }
        }
    // PATT_ANY
    } else if (ll_accept1('_')) {
        *ret = (Patt) { PATT_ANY };
    // PATT_CONS
    } else if (ll_accept1(TK_IDDATA)) {
        *ret = (Patt) { PATT_CONS, .Cons={TOK0.tk,NULL} };
        if (ll_check1('(')) {
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
    } else if (ll_accept1(TK_IDVAR)) {
        if (is_match) {
            Expr e = (Expr) { EXPR_VAR, {}, env, NULL, .Var=TOK0.tk };
            Expr* pe = malloc(sizeof(Expr));
            assert(pe != NULL);
            *pe = e;
            *ret = (Patt) { PATT_EXPR, .Expr=pe };
        } else {
            *ret = (Patt) { PATT_SET, .Set=TOK0.tk };
        }
    } else if (ll_accept1('~')) {
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

void* parser_cons_ (Env** env) {
    static Cons c_;
    Cons c;

    if (!ll_accept1(TK_IDDATA)) {
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
    if (!ll_accept1(TK_DATA)) {
        return err_expected("`data`");
    }
    if (!ll_accept1(TK_IDDATA)) {
        return err_expected("data identifier");
    }
    Tk id = TOK0.tk;

    Type tp;
    int tp_ok = parser_type(&tp);

    List lst = { 0, NULL };
    int lst_ok = ll_check1(':');
    if (lst_ok) {
        if (!parser_list_line(NULL, 1, &lst, &parser_cons_, sizeof(Cons))) {
            return 0;
        }
    }

    if (!tp_ok && !lst_ok) {
        // recursive pre declaration
        assert(ALL.data.datas.size < sizeof(ALL.data.datas.buf));
        ALL.data.datas.buf[ALL.data.datas.size].kind = DATA_REC;
        strcpy(ALL.data.datas.buf[ALL.data.datas.size].id, id.val.s);
        ALL.data.datas.size++;
        *ret = (Data) { id, 0, NULL };
        return 1;
    }

    *ret = (Data) { id, lst.size, lst.vec };
    for (int i=0; i<ret->size; i++) {
        ret->vec[i].idx = i;
    }

    DATA kind = datas_data(id.val.s);

    // new type is either DATA_SINGLE or DATA_PLAIN
    if (kind == DATA_ERROR) {
        ALL.data.datas.buf[ALL.data.datas.size].kind = (ret->size < 2 ? DATA_SINGLE : DATA_PLAIN);
        strcpy(ALL.data.datas.buf[ALL.data.datas.size].id, id.val.s);
        ALL.data.datas.size++;
    }

    // set kinds of CONS
    for (int i=0; i<ret->size; i++) {
        assert(ret->size >= 2);
        if (kind == DATA_ERROR) {
            ALL.data.conss.buf[ALL.data.conss.size].kind = CONS_CASE;
        } else {
            ALL.data.conss.buf[ALL.data.conss.size].kind = (i==0 ? CONS_NULL : CONS_CASE);
        }
        strcpy(ALL.data.conss.buf[ALL.data.conss.size].id, ret->vec[i].tk.val.s);
        ALL.data.conss.size++;
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

            ALL.data.conss.buf[ALL.data.conss.size].kind = CONS_SINGLE;
            strcpy(ALL.data.conss.buf[ALL.data.conss.size].id, id.val.s);
            ALL.data.conss.size++;
        }
    }

    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int parser_decl (Env** env, Decl* decl) {
    int is_func = ll_check0(TK_FUNC);
    if (!parser_patt(*env,&decl->patt,0)) {
        return 0;
    }

    if (!ll_accept1(TK_DECL)) {
        return err_expected("`::`");
    }

    if (!parser_type(&decl->type)) {
        return 0;
    }

//Env* old = *env;
    env_add(env, decl->patt, decl->type);

//char* s = (*env==NULL) ? "null" : (*env)->patt.Set.val.s;
//int   d = (*env==NULL) ?     -1 : (*env)->patt.sub;
//printf("[%p<-%p] %s\n", old, *env, (*env)->id.val.s);
printf("decl %s\n", (*env)->Plain.id.val.s);

    if (is_func || (!is_func && ll_accept1('='))) {
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

    return 1;
}

///////////////////////////////////////////////////////////////////////////////

void* parser_expr_ (Env** env) {
    static Expr e_;
    Expr e;
    if (!parser_expr(env,&e)) {
        return NULL;
    }
    e_ = e;
    return &e_;
}

void* parser_expr__ (Env** env) {
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

void* parser_case_ (Env** env) {
    static Let let_;
    Let let;

    // patt
    if (ll_accept1(TK_ELSE)) {
        let.decl.patt = (Patt) { PATT_ANY, {} };
    } else if (!parser_patt(*env,&let.decl.patt,0)) {
        return NULL;
    }

    // decls
    if (ll_accept1(TK_DECL)) {
        if (!parser_type(&let.decl.type)) {
            return NULL;
        }
    } else {
        let.decl.type.sub = TYPE_NONE;
    }

    // ->
    ll_accept1(TK_ARROW);       // optional

    // affect `env` before `body`
    if (let.decl.type.sub != TYPE_NONE) {
        env_add(env, let.decl.patt, let.decl.type);
printf("decl2 %s\n", (*env)->Plain.id.val.s);
    }

    // expr
    Expr* pe = expr_new(env);
puts("=======");
dump_env(*env,NULL);
    if (pe == NULL) {
        return NULL;
    }

    let.body = pe;
    let_ = let;
    return &let_;
}

void* parser_if_ (Env** env) {
    static If c_;
    If c;

    if (ll_accept1(TK_ELSE)) {
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
    ll_accept1(TK_ARROW);       // optional

    // expr
    c.ret = expr_new(env);
    if (c.ret == NULL) {
        return NULL;
    }

    c_ = c;
    return &c_;
}

int parser_expr_one (Env** env, Expr* ret) {
    // EXPR_RAW
    if (ll_accept1(TK_RAW)) {
        *ret = (Expr) { EXPR_RAW, {}, *env, NULL, .Raw=TOK0.tk };

    // EXPR_UNIT
    } else if (ll_accept1('(')) {
        if (ll_accept1(')')) {
            *ret = (Expr) { EXPR_UNIT, {}, *env, NULL, {} };
        } else {
            if (!parser_expr(env,ret)) {
                return 0;
            }
    // EXPR_TUPLE
            if (ll_check1(',')) {
                List lst = { 0, NULL };
                if (!parser_list_comma(env, &lst, ret, parser_expr_, sizeof(Expr))) {
                    return 0;
                }
                *ret = (Expr) { EXPR_TUPLE, {}, *env, NULL, .Tuple={lst.size,lst.vec} };
            }
    // EXPR_PARENS
            if (!ll_accept1(')')) {
                return err_expected("`)`");
            }
        }

    // EXPR_ARG
    } else if (ll_accept1(TK_ARG)) {
        *ret = (Expr) { EXPR_ARG, {}, *env, NULL, {} };

    // EXPR_VAR
    } else if (ll_accept1(TK_IDVAR)) {
        *ret = (Expr) { EXPR_VAR, {}, *env, NULL, .Var=TOK0.tk };

    // EXPR_CONS
    } else if (ll_accept1(TK_IDDATA)) {
        *ret = (Expr) { EXPR_CONS, {}, *env, NULL, .Cons=TOK0.tk };

    // EXPR_NEW
    } else if (ll_accept1(TK_NEW)) {
        Expr* pe = expr_new(env);
        if (pe == NULL) {
            return 0;
        }
        *ret = (Expr) { EXPR_NEW, {}, *env, NULL, .New=pe };

    // EXPR_SET
    } else if (ll_accept1(TK_SET)) {
        Patt p;
        if (!parser_patt(*env,&p,0)) {
            return 0;
        }
        if (!ll_accept1('=')) {
            return err_expected("`=`");
        }
        Expr* e = expr_new(env);
        if (e == NULL) {
            return 0;
        }
        *ret = (Expr) { EXPR_SET, {}, *env, NULL, .Set={p,e} };

    // EXPR_DECL
    } else if (ll_accept1(TK_MUT) || ll_accept1(TK_VAL)) {
        Decl d;
        if (!parser_decl(env,&d)) {
            return 0;
        }
        *ret = (Expr) { EXPR_DECL, {}, *env, NULL, .Decl=d };

    // EXPR_FUNC // EXPR_DECL_FUNC
    } else if (ll_accept1(TK_FUNC)) {
        Decl d;
        if (parser_decl(env,&d)) {
            *ret = (Expr) { EXPR_DECL, {}, *env, NULL, .Decl=d };
        } else if (ll_accept1(TK_DECL)) {
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
    } else if (ll_accept1(TK_PASS)) {
        *ret = (Expr) { EXPR_PASS, {}, *env, NULL };

    // EXPR_RETURN
    } else if (ll_accept1(TK_RETURN)) {
        Expr* e = expr_new(env);
        if (e == NULL) {
            return 0;
        }
        *ret = (Expr) { EXPR_RETURN, {}, *env, NULL, .Return=e };

    // EXPR_SEQ
    } else if (ll_check1(':') || ll_check1(TK_SEQ1)) {
        int nest = ll_check1(':');
        TOK1.tk.sym = ':';
        Env* tmp = *env;
        List lst;
        if (!parser_list_line(env, 1, &lst, &parser_expr__, sizeof(Expr))) {
            if (nest) {
                *env = tmp;
            }
            return 0;
        }
        if (nest) {
            *env = tmp;
        }
        *ret = (Expr) { EXPR_SEQ, {}, *env, NULL, .Seq={lst.size,lst.vec} };

    // EXPR_LET
    } else if (ll_accept1(TK_LET)) {
        Decl d;
        if (!parser_decl(env,&d)) {
            return 0;
        }
        if (d.init == NULL) {
            return err_expected("`=`");
        }

        ll_accept1(TK_ARROW);  // optional `->`
        Expr* pe = expr_new(env);
        if (pe == NULL) {
            return 0;
        }

        *ret = (Expr) { EXPR_LET, {}, *env, NULL, .Let={d,pe} };

    } else if (ll_accept1(TK_IF)) {
    // EXPR_IFS
        if (ll_check1(':')) {
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

            ll_accept1(TK_ARROW);  // optional `->`

            Expr* t = expr_new(env);
            if (tst == NULL) {
                return 0;
            }

            ll_accept1(TK_ARROW);  // optional `->`

            Expr* f = expr_new(env);
            if (tst == NULL) {
                return 0;
            }

            *ret = (Expr) { EXPR_IF, {}, *env, NULL, .Cond={tst,t,f} };
        }

    // EXPR_CASES
    } else if (ll_accept1(TK_CASE)) {
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
    } else if (ll_accept1(TK_BREAK)) {
        Expr* e = expr_new(env);
        if (e == NULL) {
            return 0;
        }
        *ret = (Expr) { EXPR_BREAK, {}, *env, NULL, .Break=e };

    // EXPR_LOOP
    } else if (ll_accept1(TK_LOOP)) {
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

int parser_expr (Env** env, Expr* ret) {
    int is_first = TOK0.off==-1 || ll_check0('\n') || ll_check0(TK_ARROW);

    Env* prv = *env;

    Env* hub = malloc(sizeof(Env));
    *hub = (Env) { ENV_HUB, *env, .Hub=prv };
puts("create");
    *env = hub;

    Expr e;
    if (!parser_expr_one(env, &e)) {
        return 0;
    }
    *ret = e;

    // cannot separate exprs with \n
    if (ll_check0('\n')) {
        goto _WHERE_;
    }

    // EXPR_CALL
    if (ll_check1('(')) {
        Expr arg;
        if (!parser_expr_one(env, &arg)) {
            return 0;
        }
        Expr* parg = malloc(sizeof(Expr));
        Expr* func = malloc(sizeof(Expr));
        assert(parg!=NULL && func!=NULL);
        *parg = arg;
        *func = *ret;
        *ret  = (Expr) { EXPR_CALL, {}, *env, NULL, .Call={func,parg,NULL} };
    }

    // EXPR_MATCH
    if (ll_accept1('~')) {
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
    if (ll_accept1(TK_IF)) {
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
        (ll_check0('\n') && TOK0.tk.val.n==ALL.ind && ll_accept1(TK_WHERE))
    ||
        (!ll_check0('\n') && ll_accept1(TK_WHERE))
    ) {
        if (ll_check1(':')) {
            TOK1.tk.sym = TK_SEQ1;
        }
        ret->where = expr_new(&hub->Hub);
        return (ret->where != NULL);
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

// Glob ::= Data | Decl | Expr
// Prog ::= { Glob }

void* parser_glob_ (Env** env) {
    static Glob g_;
    Glob g;

    if (ll_check1(TK_DATA)) {
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
    Env* env = NULL;
    List lst;
    if (!parser_list_line(&env, 0, &lst, &parser_glob_, sizeof(Glob))) {
        return 0;
    }
    *ret = (Prog) { lst.size, lst.vec };

    if (!ll_accept1(EOF)) {
        return err_expected("end of file");
    }
    return 1;
}
