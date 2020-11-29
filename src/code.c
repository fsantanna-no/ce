#include "all.h"

void out (const char* v) {
    fputs(v, ALL.out[OGLOB]);
}

void outl (State_Tok tok) {
    fprintf(ALL.out[OGLOB], "#line %ld\n", tok.lin);
}

void code_ret (tce_ret* ret) {
    while (ret != NULL) {
        if (ret->env.type.sub==TYPE_DATA && ret->env.type.Data.size!=-1) {
//assert(0);
            out("(");
            out(ret->env.id.val.s);
            out("->root");
            out(")");
        } else {
            out(ret->env.id.val.s);
        }
        out(" = ");
        ret = ret->nxt;
    }
}

///////////////////////////////////////////////////////////////////////////////

char* type2str (Type* tp) {
    switch (tp->sub) {
        case TYPE_RAW: {
            static Type _tp_;
            // char* -> char_
            if (tp->Raw.val.s[strlen(tp->Raw.val.s)-1] == '*') {
                _tp_ = *tp;
                _tp_.Raw.val.s[strlen(_tp_.Raw.val.s)-1] = '_';
                return _tp_.Raw.val.s;
            } else {
                return tp->Raw.val.s;
            }
        }
        case TYPE_UNIT:
            return "unit";
        case TYPE_DATA: {
            return tp->Data.tk.val.s;
        }
        case TYPE_TUPLE: {
            static char _ret_[256];
            char ret[256] = "Tuple";
            for (int i=0; i<tp->Tuple.size; i++) {
                // TODO: asserts
                strcat(ret, "__");
                strcat(ret, type2str(&tp->Tuple.vec[i]));
            }
            strcpy(_ret_, ret);
            return _ret_;
        }
        default:
//printf("%d\n", tp.sub);
            assert(0 && "TODO");
    }
}

void code_type_ (char* out1, char* out2, Type tp) {
    switch (tp.sub) {
        case TYPE_RAW:
            strcat(out2, tp.Raw.val.s);
            break;
        case TYPE_UNIT:
            strcat(out2, "int");
            break;
        case TYPE_DATA: {
            Data* data = data_get(tp.Data.tk.val.s);
            assert(data != NULL);
            if (data->isrec) strcat(out2, "struct ");
            strcat(out2, tp.Data.tk.val.s);
            if (data->isrec) strcat(out2, "*");
            break;
        }
        case TYPE_TUPLE: {
            char* _str_ = type2str(&tp);
            char str[256];
            strcpy(str, _str_);
            sprintf(&out1[strlen(out1)],
                "#ifndef DEF__%s\n"
                "#define DEF__%s\n"
                "typedef struct { ",
                str, str
            );
            for (int i=0; i<tp.Tuple.size; i++) {
                char out1_[4096] = "";
                char out2_[4096] = "";    // TODO: asserts
                code_type_(out1_, out2_, tp.Tuple.vec[i]);
                strcat(out1_, out1);
                strcpy(out1, out1_);
                sprintf(&out1[strlen(out1)], "%s _%d; ", out2_, i);
            }
            sprintf(&out1[strlen(out1)],
                "} %s;\n"
                "#endif\n",
                str
            );
            strcat(out2, str);
            break;
        }
        default:
//printf("%d\n", tp.sub);
            assert(0 && "TODO");
    }
}

void code_type (Type tp) {
    char out1[4096] = "";
    char out2[4096] = "";    // TODO: asserts
    code_type_(out1, out2, tp);
    out(out1);
    out(out2);
}

///////////////////////////////////////////////////////////////////////////////

char* strupper (const char* src) {
    static char dst[256];
    assert(strlen(src) < sizeof(dst));
    for (int i=0; i<strlen(src); i++) {
        dst[i] = toupper(src[i]);
    }
    dst[strlen(src)] = '\0';
    return dst;
}

void code_data (Data data) {
    char* sup = data.tk.val.s;
    char SUP[256];
    assert(strlen(sup) < sizeof(SUP));
    strcpy(SUP, strupper(sup));

    if (data.isrec) {
        fprintf(ALL.out[OGLOB], "struct %s;\n", sup);
    }

    for (int i=0; i<data.size; i++) {
        Cons cons = data.vec[i];
        char* sub = cons.tk.val.s;
        // #define SUP_False Bool_False
        out("#define SUP_");
        out(sub);
        out(" ");
        out(sup);
        out("_");
        out(sub);
        out("\n");

        // #define False ((Bool) { Bool_False })
        // #define Sub(v) ((Sup) { Sup_Sub, ._Sub=v })
        out("#define ");
        out(sub);
        if (cons.type.sub == TYPE_UNIT) {
            out("(x)");
        } else {
            out("(...)");
        }
        if (data.isrec && cons.type.sub==TYPE_UNIT) {
            out(" NULL\n");
        } else {
            int ismult = (data.size>=3 || (!data.isrec && data.size>=2));
            out(" ((");
            out(sup);
            out(") { ");
            if (ismult) {
                out(sup);
                out("_");
                out(sub);
            }
            if (cons.type.sub != TYPE_UNIT) {
                if (ismult) {
                    out(", ");
                }
                out("._");
                out(sub);
                out("=__VA_ARGS__");
            }
            out(" })\n");
        }
    }
    out("\n");

    out("typedef enum {\n");
        for (int i=0; i<data.size; i++) {
            char* v = data.vec[i].tk.val.s;
            out("    ");
            out(sup);       // Bool
            out("_");
            out(v);         // False
            if (i < data.size-1) {
                out(",");
            }
            out("\n");
        }
    out("} ");
    out(SUP);
    out(";\n\n");

    char out1[4096] = "";    // TODO: asserts
    char out2[4096] = "";
    for (int i=0; i<data.size; i++) {
        Cons cons = data.vec[i];
        if (cons.type.sub != TYPE_UNIT) {
            strcat(&out2[strlen(out2)], "        ");
            code_type_(&out1[strlen(out1)], &out2[strlen(out2)], cons.type);
            strcat(&out2[strlen(out2)], " _");
            strcat(&out2[strlen(out2)], cons.tk.val.s);
            strcat(&out2[strlen(out2)], ";\n");
        }
    }

    out(out1);

    int isplain  = !data.isrec && data.size>=2;
    if (isplain || (data.isrec && data.size>2)) {
        fprintf(ALL.out[OGLOB],
            "typedef struct %s {\n"
            "    %s sub;\n"
            "    union {\n"
            "%s"
            "    };\n"
            "} %s;\n\n",
            sup, SUP, out2, sup
        );
    } else {
        fprintf(ALL.out[OGLOB],
            "typedef struct %s {\n"
            "%s"
            "} %s;\n\n",
            sup, out2, sup
        );
    }

    // SHOW
    {
        fprintf(ALL.out[OGLOB],
            "void _show_%s (%s%s v) {\n",
            sup, sup, (data.isrec ? "*" : "")
        );
        if (data.isrec) {
            fprintf(ALL.out[OGLOB],
                "if (v == NULL) {\n"
                "    printf(\"$\");\n"
                "    return;\n"
                "}\n"
            );
        }
        for (int i=0; i<data.size; i++) {
            Cons cons = data.vec[i];
            char* v = cons.tk.val.s;
            if (data.size == 1) {
                fprintf(ALL.out[OGLOB], "printf(\"%s\");\n", v);
            } else {
                if (data.size>=2 && i==0) {
                    if (data.isrec) {
                        fprintf(ALL.out[OGLOB], "switch (v->sub) {\n");
                    } else {
                        fprintf(ALL.out[OGLOB], "switch (v.sub) {\n");
                    }
                }
                fprintf(ALL.out[OGLOB],
                    "case %s_%s:\n"
                    "    printf(\"%s\");\n",
                    sup, v, v
                );
            }

            if (cons.type.sub != TYPE_UNIT) {
                out("putchar('(');\n");
                void aux (Type type, char* arg, int first) {
                    switch (type.sub) {
                        case TYPE_TUPLE:
                            if (!first) {
                                out("putchar('(');\n");
                            }
                            for (int i=0; i<type.Tuple.size; i++) {
                                if (i > 0) {
                                    out("printf(\",\");\n");
                                }
                                char arg_[256];
                                sprintf(arg_, "%s._%d", arg, i);
                                aux(type.Tuple.vec[i], arg_, 0);
                            }
                            if (!first) {
                                out("putchar(')');\n");
                            }
                            break;
                        case TYPE_DATA:
                            fprintf(ALL.out[OGLOB], "_show_%s(%s);\n", type.Data.tk.val.s, arg);
                            break;
                        default:
                            out("printf(\"%s\", \"???\");\n");
                    }
                }
                char arg_[256];
                sprintf(arg_, "v%s_%s", (data.isrec?"->":"."), v);
                aux(cons.type, arg_, 1);
                out("putchar(')');\n");
            }
            if (data.size >= 2) {
                out("    break;\n");
            }
        }
        if (data.size >= 2) {
            fprintf(ALL.out[OGLOB],
                "default:\n"
                "    assert(0 && \"bug found\");\n"
                "}\n"
            );
        }
        out("}\n");
        fprintf(ALL.out[OGLOB],
            "void show_%s (%s%s v) { _show_%s(v); puts(\"\"); }\n\n",
            sup, sup, (data.isrec ? "*" : ""), sup);
    }
}

///////////////////////////////////////////////////////////////////////////////

void code_patt_match (Patt p, Expr tst) {
    switch (p.sub) {
        case PATT_RAW:
            code_expr(tst, NULL);
            out(" == ");
            out(p.Raw.val.s);
            break;
        case PATT_ANY:
        case PATT_SET:
            out("1");
            break;
        case PATT_EXPR:
            code_expr(tst, NULL);
            out(" == ");
            code_expr(*p.Expr, NULL);
            break;
        case PATT_UNIT:
            code_expr(tst, NULL);
            out(" == 1");
            break;
        case PATT_NIL:
            code_expr(tst, NULL);
            out(" == NULL");
            break;
        case PATT_CONS: {
            Cons cons;
            Data* data = cons_sup(p.Cons.data.val.s, &cons);
            assert(data != NULL);
            if (data->size == 1) {                  // SINGLE
                if (data->isrec) {                  // CASE1
                    code_expr(tst, NULL);
                    out(" != NULL");
                } else {
                    out("1"); // always succeeds
                }
            } else {
                out("(");
                code_expr(tst, NULL);
                out(").sub == SUP_");
                out(p.Cons.data.val.s);
                break;
            }
            if (p.Cons.arg != NULL) {
                out(" && ");
                code_patt_match (
                    *p.Cons.arg,
                    (Expr) { EXPR_CONS_SUB, {}, tst.env, NULL, .Cons_Sub={&tst,p.Cons.data.val.s} }
                );
            }
            break;
        }
        case PATT_TUPLE:
            for (int i=0; i<p.Tuple.size; i++) {
                if (i > 0) {
                    out(" && ");
                }
                code_patt_match (
                    p.Tuple.vec[i],
                    (Expr) { EXPR_TUPLE_IDX, {}, tst.env, NULL, .Tuple_Idx={&tst,i} }
                );
            }
            break;
        default:
            assert(0 && "TODO");
    }
}

void code_patt_set (Env* env, Patt p, Expr e) {
    switch (p.sub) {
        case PATT_RAW:
        case PATT_ANY:
        case PATT_UNIT:
        case PATT_NIL:
        case PATT_EXPR:
            break;
        case PATT_SET: {        // x = ce_tst
            Type* type = env_get(env, p.Set.val.s, NULL);
            assert(type != NULL);
            Env_Plain env = { p.Set, *type };
            tce_ret r = { env, NULL };
            code_expr(e, &r);
            out(";\n");
            break;
        }
        case PATT_CONS:
            if (p.Cons.arg != NULL) {
                code_patt_set (
                    env,
                    *p.Cons.arg,
                    (Expr) { EXPR_CONS_SUB, {}, e.env, NULL, .Cons_Sub={&e,p.Cons.data.val.s} }
                );
            }
            break;
        case PATT_TUPLE:
            for (int i=0; i<p.Tuple.size; i++) {
                code_patt_set (
                    env,
                    p.Tuple.vec[i],
                    (Expr) { EXPR_TUPLE_IDX, {}, e.env, NULL, .Tuple_Idx={&e,i} }
                );
            }
            break;
        default:
            assert(0 && "TODO");
    }
}

void code_patt_decls (Decl decl) {
    Patt patts[16];
    int patts_i = 0;
    patt2patts(patts, &patts_i, decl.patt);
    if (patts_i == 1) {
        int size = (decl.type.sub == TYPE_DATA ? decl.type.Data.size : -1);
        if (size == -1) {
            code_type(decl.type);
        } else {
            out("Pool _");
            out(patts[0].Set.val.s);
            out(" = { NULL, 0, 0, NULL };\n");
            out("Pool*"); // all pools must be used as pointers b/c of fun args w/ write access
        }
        out(" ");
        out(patts[0].Set.val.s);
        if (size != -1) {
            out(" = &_");
            out(patts[0].Set.val.s);
        }
        out(";\n");
    } else if (patts_i > 1) {
        assert(decl.type.Tuple.size == patts_i);
        for (int i=0; i<patts_i; i++) {
            code_type(decl.type.Tuple.vec[i]);
            out(" ");
            out(patts[i].Set.val.s);
            out(";\n");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

void code_decl (Decl d, tce_ret* ret) {
    if (d.type.sub == TYPE_FUNC) {
        int isrec = 0; {
            if (d.type.Func.out->sub == TYPE_DATA) {
                Data* data = data_get(d.type.Func.out->Data.tk.val.s);
                assert(data != NULL);
                isrec = data->isrec;
            }
        }
        assert(d.init != NULL);
        assert(d.patt.sub == PATT_SET);
        out("\n");
        char out1[4096] = "";
        char out2[4096] = "";    // TODO: asserts
        code_type_(out1, out2, *d.type.Func.inp);
        out(out1);
        out("#define TYPE_");
            out(d.patt.Set.val.s);
            out(" ");
            out(out2);
            out("\n");
        code_type(*d.type.Func.out);
            out(" ");
        out(d.patt.Set.val.s);
        out(" (");
        if (isrec) {
            out("Pool* ce_pool,");
        }
        out(out2);
        out(" ce_inp) {\n");
            if (d.type.Func.out->sub != TYPE_UNIT) {
                code_type(*d.type.Func.out);
                out(" ce_out;\n");
            }

            Env_Plain env = { {TK_IDVAR,{.s="ce_out"}}, *d.type.Func.out };
            if (env.type.sub == TYPE_DATA) {
                env.type.Data.size = -1;  // ce_out is not a Pool
            }

            tce_ret r = { env, NULL };
            code_expr(*d.init, (d.type.Func.out->sub==TYPE_UNIT ? NULL : &r));
            out(";\n");
            if (d.type.Func.out->sub != TYPE_UNIT) {
                out("return ce_out;\n");
            }
        out("}\n\n");
    } else {
        if (d.init == NULL) {
            code_patt_decls(d);
        } else {
            code_patt_decls(d);
            code_patt_set(d.init->env, d.patt, *d.init);
            out(";\n");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

void code_expr_new (Expr e, tce_ret* ret) {
    assert(ret != NULL);

    Type type = env_expr(*e.New);
    assert(type.sub == TYPE_DATA);
    char* id = type.Data.tk.val.s;
    Data* data; {
        data = data_get(id);
        if (data == NULL) {
            data = cons_sup(id,NULL);
        }
        assert(data != NULL);
    }
    char* sup = data->tk.val.s;

    //void aux1 (Expr e);
    //void aux2 (Expr e, Cons cons);

    // substitutes `v` in
    //      v
    // where v is a recursive value, so either
    // - a call to build it
    // - a cons to build it
    void aux1 (Expr e) {
        static int I = 0;
        int i = ++I;

        // substitutes `v` in
        //      Cons v
        // where v is a recursive value
        void aux2 (Expr e, Cons cons) {
            switch (cons.type.sub) {
                case TYPE_DATA:
                    if (!strcmp(sup,cons.type.Data.tk.val.s)) {
                        aux1(e);
                        return;
                    }
                    break;
                case TYPE_TUPLE:
                    assert(e.sub == EXPR_TUPLE);
                    out("{");
                    for (int i=0; i<cons.type.Tuple.size; i++) {
                        if (i > 0) {
                            out(", ");
                        }
                        if (cons.type.Tuple.vec[i].sub==TYPE_DATA &&
                            !strcmp(sup,cons.type.Tuple.vec[i].Data.tk.val.s)) {
                            aux1(e.Tuple.vec[i]);
                        } else {
                            code_expr(e.Tuple.vec[i], NULL);
                        }
                    }
                    out("}");
                    break;
                default:
                    code_expr(e, NULL);
            }
        }

        switch (e.sub) {
            //  l[] = new f(...)
            // becomes
            //  f(l,...)
            case EXPR_CALL: {
                Tk tk = ret->env.id;
                if (!strcmp(tk.val.s,"ce_out")) {
                    tk = (Tk) { TK_IDVAR,{.s="ce_pool"} };
                }
                e.Call.pool = &tk;
                code_expr(e, NULL);
                break;
            }

            // new Cons(...)
            case EXPR_CONS: {
                char* sub = e.Cons.id.val.s;
                Cons* cons = cons_get(*data,sub);
                assert(cons != NULL);

                //  new Cons(...)
                // becomes
                //  { List* ptr=malloc(sizeof(List)); *ptr=XXX(); ptr; }
                fprintf(ALL.out[OGLOB],
                    "({\n"
                    "//ce_pool->cur++;\n"
                    "%s* ptr_%d = malloc(sizeof(%s));\n"
                    "*ptr_%d = (%s) %s(",
                    sup, i, sup,
                    i, sup, sub
                );

                // XXX
                aux2(*e.Cons.arg, *cons);

                fprintf(ALL.out[OGLOB],
                    ");\n"
                    "ptr_%d;\n})",
                    i
                );
                break;
            }
            default:
                code_expr(e, NULL);
                //assert(0 && "bug found");
        }
    }
    code_ret(ret);
    aux1(*e.New);
}

void code_expr (Expr e, tce_ret* ret) {
    //Env* env = env_expr(e);

    if (e.where != NULL) {
        out("{\n");
        code_expr(*e.where, NULL);
    }
    switch (e.sub) {
        case EXPR_RAW:
            code_ret(ret);
            out(e.Raw.val.s);
            break;
        case EXPR_UNIT:
            code_ret(ret);
            out("1");
            break;
        case EXPR_NIL:
            code_ret(ret);
            out("NULL");
            break;
        case EXPR_ARG:
            code_ret(ret);
            out("ce_inp");
            break;
        case EXPR_VAR:
            code_ret(ret);
            Type* type = env_get(e.env, e.Var.val.s, NULL);
            assert(type != NULL);
            if (type==NULL || type->sub!=TYPE_DATA || type->Data.size==-1) {
                out(e.Var.val.s);
            } else {
                out("((");
                out(type->Data.tk.val.s);
                out("*)");
                out(e.Var.val.s);
                out("->root");
                out(")");
            }
            break;
        case EXPR_NEW:
            code_expr_new(e, ret);
            break;
        case EXPR_SET:
            code_patt_set(e.Set.expr->env, e.Set.patt, *e.Set.expr);
            out(";\n");
            break;
        case EXPR_CONS:
            code_ret(ret);
            out(e.Cons.id.val.s);
            out("(");
            code_expr(*e.Cons.arg, NULL);
            out(")");
            break;
        case EXPR_CALL:
            code_ret(ret);
            code_expr(*e.Call.func, NULL);
            out("(");
            if (e.Call.pool != NULL) {
                out(e.Call.pool->val.s);
                out(", ");
            }
            if (e.Call.func->sub == EXPR_RAW) {
                if (e.Call.arg->sub == EXPR_TUPLE) {
                    // {f}(a,b,c) -> f(a,b,c)
                    for (int i=0; i<e.Call.arg->Tuple.size; i++) {
                        if (i > 0) {
                            out(", ");
                        }
                        code_expr(e.Call.arg->Tuple.vec[i], NULL);
                    }
                } else {
                    // {f}(a) -> f(a)
                    code_expr(*e.Call.arg, NULL);
                }
            } else {
                assert(e.Call.func->sub == EXPR_VAR);
                out("(typeof(TYPE_");
                out(e.Call.func->Var.val.s);
                out("))");
                code_expr(*e.Call.arg, NULL);
            }
            out(")");
            break;
        case EXPR_TUPLE:
            code_ret(ret);
            if (ret != NULL) {
                out("(typeof(");
                out(ret->env.id.val.s);
                out(")) ");
            }
            out("{ ");
            for (int i=0; i<e.Tuple.size; i++) {
                //fprintf (ALL.out[OGLOB], "%c _%d=", ((i==0) ? ' ' : ','), i);
                if (i != 0) {
                    out(",");
                }
                code_expr(e.Tuple.vec[i], NULL);
            }
            out(" }");
            break;
        case EXPR_SEQ:
            for (int i=0; i<e.Seq.size; i++) {
                outl(e.Seq.vec[i].tok);
                code_expr(e.Seq.vec[i], (i==e.Seq.size-1) ? ret : NULL);
                out(";\n");
            }
            break;
        case EXPR_LET: {    // patt,type,init,body
            out("{\n");
            code_patt_decls(e.Let.decl);
            code_patt_set(e.Let.decl.init->env, e.Let.decl.patt, *e.Let.decl.init);
            out(";\n");
            code_expr(*e.Let.body, ret);
            out(";\n");
            out("}\n");
            break;
        }
        case EXPR_DECL:
            code_decl(e.Decl, ret);
            break;
        case EXPR_IF: {   // tst,true,false
            code_ret(ret);
            out("(");
            code_expr(*e.Cond.tst,   NULL);
            out(" ? ");
            code_expr(*e.Cond.true,  NULL);
            out(" : ");
            code_expr(*e.Cond.false, NULL);
            out(")");
            break;
        }
        case EXPR_LOOP: {
            out("while (1) {\n");
            code_expr(*e.Loop, ret);
            out("}\n");
            break;
        }
        case EXPR_BREAK: {
            if (e.Break != NULL) {
                code_expr(*e.Break, ret);
                out(";\n");
            }
            out("break;\n");
            break;
        }
        case EXPR_PASS: {
            out("/* pass */\n");
            break;
        }
        case EXPR_RETURN: {
            Env_Plain env = { {TK_IDVAR,{.s="ce_out"}}, *env_get(e.env,"ce_out",NULL) };
            if (env.type.sub == TYPE_DATA) {
                env.type.Data.size = -1;     // returns pointer, not pool[]
            }
            tce_ret r = { env, NULL };
            code_expr(*e.Return, &r);
            out(";\n");
            out("return ce_out;\n");
            break;
        }
        case EXPR_MATCH:
            code_ret(ret);
            code_patt_match(*e.Match.patt, *e.Match.expr);
            break;
        case EXPR_CASES: {  // tst,size,vec
            Expr tst = *e.Cases.tst;
            out("{\n");
#if 1
            Type type = env_expr(*e.Cases.tst);
            Tk tk = { TK_IDVAR,{.s="ce_tst"} };
            Env env = { ENV_PLAIN, e.env, .Plain={tk,type} };
            if (tst.sub == EXPR_RAW) {   // prevents multiple evaluation of tst
                tst = (Expr) { EXPR_VAR, {}, &env, NULL, {.Var=tk} };

                if (type.sub != TYPE_NONE) {
                    code_type(type);
                } else {
                    out("typeof(");
                    code_expr(*e.Cases.tst, NULL);
                    out(")");
                }
                out(" ce_tst = ");
                code_expr(*e.Cases.tst, NULL);
                out(";\n");
            }
#endif

            for (int i=0; i<e.Cases.size; i++) {
                Let let = e.Cases.vec[i];
                out("if (");
                code_patt_match(let.decl.patt, tst);
                out(") {\n");
                code_patt_decls(let.decl);
                code_patt_set(let.body->env, let.decl.patt, tst);
                out(";\n");     // maybe let.body->env->prev ?
                code_expr(*let.body, ret);
                out(";\n");
                out("} else ");
            }
            out("{\n");
            //out("assert(0 && \"match failed\");\n");
            out("}\n");
            out("}\n");
            break;
        }
        case EXPR_IFS:    // tst,size,vec
            for (int i=0; i<e.Ifs.size; i++) {
                out("if (");
                code_expr(*e.Ifs.vec[i].tst, NULL);
                out(") {\n");
                code_expr(*e.Ifs.vec[i].ret, ret);
                out(";\n} else \n");
            }
            out("{\n");
            //out("assert(0 && \"if failed\");\n");
            out("}\n");
            break;
        case EXPR_TUPLE_IDX:
            if (e.Tuple_Idx.tuple->sub == EXPR_TUPLE) {
                code_expr(e.Tuple_Idx.tuple->Tuple.vec[e.Tuple_Idx.idx], ret);
            } else {
                code_expr(*e.Tuple_Idx.tuple, ret);
                fprintf(ALL.out[OGLOB], "._%d", e.Tuple_Idx.idx);
            }
            break;
        case EXPR_CONS_SUB: {
            code_ret(ret);
            Type type = env_expr(*e.Cons_Sub.cons);
            Data* data = data_get(type.Data.tk.val.s);
            //assert(type.sub == TYPE_DATA);
            out("(");
            code_expr(*e.Cons_Sub.cons, NULL);
            out(")");
            out((data!=NULL && data->isrec) ? "->" : ".");
            out("_");
            out(e.Cons_Sub.sub);
            break;
        }
        default:
//printf("%d\n", e.sub);
            assert(0 && "TODO");
    }
    if (e.where != NULL) {
        out(";\n");
        out("}\n");
    }
}

///////////////////////////////////////////////////////////////////////////////

void code_prog (void) {
    for (int i=0; i<ALL.prog.size; i++) {
        Glob g = ALL.prog.vec[i];
        switch (g.sub) {
            case GLOB_DATA:
                code_data(g.data);
                break;
            case GLOB_EXPR:
                outl(g.expr.tok);
                code_expr(g.expr, NULL);
                out(";\n");
                break;
        }
    }
}

void code (void) {
    out (
        "#include \"inc/ce.c\"\n"
        "int main (void) {\n"
        "\n"
    );
    code_prog();
    fprintf(ALL.out[OGLOB], "\n");
    out("}\n");
}
