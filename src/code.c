#include "all.h"

void out (const char* v) {
    fputs(v, ALL.out[OGLOB]);
}

void outl (State_Tok tok) {
    //fprintf(ALL.out[OGLOB], "#line %ld\n", tok.lin);
}

void code_ret (tce_ret* ret) {
    while (ret != NULL) {
        if (ret->env.type.sub!=TYPE_DATA || ret->env.type.Data.size==-1) {
            out(ret->env.id.val.s);
        } else {
            out("(");
            out(ret->env.id.val.s);
            out("->root");
            out(")");
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
            int isrec = (datas_data(tp.Data.tk.val.s) == DATA_REC);
            if (isrec) strcat(out2, "struct ");
            strcat(out2, tp.Data.tk.val.s);
            if (isrec) strcat(out2, "*");
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
    DATA kind = datas_data(sup);

    // only pre declaration
    if (data.size == 0) {
        fprintf(ALL.out[OGLOB], "struct %s;\n", sup);
        return;
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
        if (cons.type.sub != TYPE_UNIT) {
            out("(...)");
        }
        if (kind==DATA_REC && cons.type.sub==TYPE_UNIT) {
            out(" NULL\n");
        } else {
            out(" ((");
            out(sup);
            out(") { ");
            out(sup);
            out("_");
            out(sub);
            if (cons.type.sub != TYPE_UNIT) {
                out(", ._");
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

    if (kind==DATA_PLAIN || (kind==DATA_REC && data.size>2)) {
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

    fprintf(ALL.out[OGLOB],
        "void show_%s (%s%s v) {\n",
        sup, sup, (kind==DATA_REC ? "*" : "")
    );
    int has_switch = 0;
    for (int i=0; i<data.size; i++) {
        char* v = data.vec[i].tk.val.s;
        if (kind==DATA_REC && i==0) {
            fprintf(ALL.out[OGLOB],
                "if (v == NULL) {\n"
                "    puts(\"%s\");\n"
                "    return;\n"
                "}\n",
                v
            );
        }
        if (kind == DATA_SINGLE) {
            // no switch
            fprintf(ALL.out[OGLOB], "puts(\"%s\");\n", v);
        } else if (kind == DATA_REC && data.size<=2) {
            // no switch
            if (i > 0) {
                fprintf(ALL.out[OGLOB], "puts(\"%s\");\n", v);
            }
        } else {
            if (kind == DATA_PLAIN) {
                has_switch = 1;
                if (i == 0) {
                    fprintf(ALL.out[OGLOB], "switch (v.sub) {\n");
                }
            } else if (kind == DATA_REC) {
                has_switch = 1;
                if (i == 1) {
                    fprintf(ALL.out[OGLOB], "switch (v->sub) {\n");
                }
            }
            fprintf(ALL.out[OGLOB],
                "case %s_%s:\n"
                "    puts(\"%s\");\n"
                "    break;\n",
                sup, v, v
            );
        }
    }
    if (has_switch) {
        fprintf(ALL.out[OGLOB],
            "default:\n"
            "    assert(0 && \"bug found\");\n"
            "}\n"
        );
    }
    out("}\n\n");
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
        case PATT_CONS: {
            CONS cons = datas_cons(p.Cons.data.val.s, NULL);
            switch (cons) {
                case CONS_SINGLE:
                    out("1"); // always succeeds
                    break;
                case CONS_NULL:
                    code_expr(tst, NULL);
                    out(" == NULL");
                    break;
                case CONS_CASE1:
                    out("1"); // always succeeds (after NULL check)
                    break;
                default:
                    out("(");
//dump_expr(tst);
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
        case PATT_EXPR:
            break;
        case PATT_SET: {        // x = ce_tst
            Type* type = env_get(env, p.Set.val.s, NULL);
            assert(type != NULL);
            int isrec = (type->sub==TYPE_DATA && datas_data(type->Data.tk.val.s)==DATA_REC);
            if (isrec && (e.sub==EXPR_CALL && e.Call.func->sub!=EXPR_CONS)) {
                //  l[] = f(...)
                // becomes
                //  f(l,...)
                e.Call.out = &p;
                code_expr(e, NULL);
            } else {
                Env_Plain env = { p.Set, *type };
                tce_ret r = { env, NULL };
                code_expr(e, &r);
            }
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
            out(";\nPool*"); // all pools must be used as pointers b/c of fun args w/ write access
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
        int isrec = (d.type.Func.out->sub == TYPE_DATA &&
                     datas_data(d.type.Func.out->Data.tk.val.s) == DATA_REC);
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
        if (isrec) {
            out("void");
        } else {
            code_type(*d.type.Func.out);
        }
            out(" ");
        out(d.patt.Set.val.s);
        out(" (");
        if (isrec) {
            out("Pool* ce_ret,");
        }
        out(out2);
        out(" ce_arg) {\n");
            if (d.type.Func.out->sub != TYPE_UNIT) {
                if (!isrec) {
                    code_type(*d.type.Func.out);
                    out(" ce_ret;\n");
                }
                Env_Plain env = { {TK_IDVAR,{.s="ce_ret"}}, *d.type.Func.out };
                tce_ret r = { env, NULL };
                code_expr(*d.init, &r);
                out(";\n");
                if (!isrec) {
                    out("return ce_ret;\n");
                }
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
        case EXPR_ARG:
            code_ret(ret);
            out("ce_arg");
            break;
        case EXPR_UNIT:
            code_ret(ret);
            out("1");
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
        case EXPR_CONS:
            code_ret(ret);
            out(e.Cons.val.s);
            break;
        case EXPR_NEW: {
            char* cons;
            if (e.New->sub == EXPR_CONS) {
                cons = e.New->Cons.val.s;
            } else if (e.New->sub==EXPR_CALL && e.New->Call.func->sub==EXPR_CONS) {
                cons = e.New->Call.func->Cons.val.s;
            } else {
                assert(0 && "bug found");
            }
            //printf(">>> %s %d\n", e.New->Cons.val.s, datas_cons(e.New->Cons.val.s));
            if (datas_cons(cons,NULL) == CONS_NULL) {
                code_ret(ret);
                out("NULL");
            } else {
                code_ret(ret);
                out("({");
// TODO
            //out(ret->patt->Set.val.s);
            //out("->cur++; ");
                out("typeof(");
                code_expr(*e.New, NULL);
                out(")* ptr = malloc(sizeof(");
                code_expr(*e.New, NULL);
                out(")) ; *ptr=");
                code_expr(*e.New, NULL);
                out("; ptr; })");
            }
            break;
        }
        case EXPR_SET:
            code_patt_set(e.Set.expr->env, e.Set.patt, *e.Set.expr);
            out(";\n");
            break;
        case EXPR_CALL:
            code_ret(ret);
            code_expr(*e.Call.func, NULL);
            out("(");
            if (e.Call.out != NULL) {
                out(e.Call.out->Set.val.s);
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
            } else if (e.Call.func->sub == EXPR_CONS) {
                code_expr(*e.Call.arg, NULL);
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
            out("return ");
            code_expr(*e.Return, NULL);
            out(";\n");
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
//dump_expr(*e.Cons_Sub.cons);
//printf(">>> %d\n", type.sub);
            assert(type.sub == TYPE_DATA);
            out("(");
            code_expr(*e.Cons_Sub.cons, NULL);
            out(")");
            out(datas_data(type.Data.tk.val.s) == DATA_REC ? "->" : ".");
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

void code_prog (Prog prog) {
    for (int i=0; i<prog.size; i++) {
        Glob g = prog.vec[i];
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

void code (Prog prog) {
    out (
        "#include \"inc/ce.c\"\n"
        "int main (void) {\n"
        "\n"
    );
    code_prog(prog);
    fprintf(ALL.out[OGLOB], "\n");
    out("}\n");
}
