#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "lexer.h"
#include "parser.h"
#include "code.h"

void out (const char* v) {
    fputs(v, ALL.out[OGLOB]);
}

void code_ret (tce_ret* ret) {
    while (ret != NULL) {
        assert (ret->patt->sub == PATT_SET);
        out(ret->patt->Set.val.s);
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
            return tp->Data.val.s;
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
            int is = is_rec(tp.Data.val.s);
            if (is) strcat(out2, "struct ");
            strcat(out2, tp.Data.val.s);
            if (is) strcat(out2, "*");
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
    fprintf(ALL.out[OGLOB],
        "typedef struct %s {\n"
        "    %s sub;\n"
        "    union {\n"
        "%s"
        "    };\n"
        "} %s;\n\n",
        sup, SUP, out2, sup
    );

    int is = is_rec(sup);
    fprintf(ALL.out[OGLOB],
        "void show_%s (%s%s v) {\n"
        "    switch (v%ssub) {\n",
        sup, sup, (is ? "*" : ""), (is ? "->" : ".")
    );
    for (int i=0; i<data.size; i++) {
        char* v = data.vec[i].tk.val.s;
        fprintf(ALL.out[OGLOB],
            "        case %s_%s:\n"
            "            puts(\"%s\");\n"
            "            break;\n",
            sup, v, v
        );
    }
    fprintf(ALL.out[OGLOB],
        "        default:\n"
        "            assert(0 && \"bug found\");\n"
        "    }\n"
        "}\n\n"
    );
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
        case PATT_CONS:
            out("(");
            code_expr(tst, NULL);
            out(").sub == SUP_");
            out(p.Cons.data.val.s);
            if (p.Cons.arg != NULL) {
                out(" && ");
                code_patt_match (
                    *p.Cons.arg,
                    (Expr) { EXPR_CONS_SUB, .Cons_Sub={&tst,p.Cons.data.val.s} }
                );
            }
            break;
        case PATT_TUPLE:
            for (int i=0; i<p.Tuple.size; i++) {
                if (i > 0) {
                    out(" && ");
                }
                code_patt_match (
                    p.Tuple.vec[i],
                    (Expr) { EXPR_TUPLE_IDX, .Tuple_Idx={&tst,i} }
                );
            }
            break;
        default:
            assert(0 && "TODO");
    }
}

void code_patt_set (Patt p, Expr tst) {
    switch (p.sub) {
        case PATT_RAW:
        case PATT_ANY:
        case PATT_UNIT:
        case PATT_EXPR:
            break;
        case PATT_SET: {        // x = ce_tst
            tce_ret r = { &p, NULL };
            code_expr(tst, &r);
            out(";\n");
            break;
        }
        case PATT_CONS:
            if (p.Cons.arg != NULL) {
                code_patt_set (
                    *p.Cons.arg,
                    (Expr) { EXPR_CONS_SUB, .Cons_Sub={&tst,p.Cons.data.val.s} }
                );
            }
            break;
        case PATT_TUPLE:
            for (int i=0; i<p.Tuple.size; i++) {
                code_patt_set (
                    p.Tuple.vec[i],
                    (Expr) { EXPR_TUPLE_IDX, .Tuple_Idx={&tst,i} }
                );
            }
            break;
        default:
            assert(0 && "TODO");
    }
}

void code_patt_decls (Patt patt, Type type) {
    void aux (Tk* vars, int* vars_i, Patt patt) {
        switch (patt.sub) {
            case PATT_RAW:
            case PATT_ANY:
            case PATT_UNIT:
            case PATT_EXPR:
                break;
            case PATT_SET:
                assert(*vars_i < 16);
                vars[(*vars_i)++] = patt.Set;
                break;
            case PATT_CONS:
                if (patt.Cons.arg != NULL) {
                    aux(vars, vars_i, *patt.Cons.arg);
                }
                break;
            case PATT_TUPLE:
                for (int i=0; i<patt.Tuple.size; i++) {
                    aux(vars, vars_i, patt.Tuple.vec[i]);
                }
                break;
            default:
                assert(0 && "TODO");
        }
    }
    Tk vars[16];
    int vars_i = 0;
    aux(vars, &vars_i, patt);
    if (vars_i == 1) {
        code_type(type);
        out(" ");
        out(vars[0].val.s);
        out(";\n");
    } else if (vars_i > 0) {
        assert(type.Tuple.size == vars_i);
        for (int i=0; i<vars_i; i++) {
            code_type(type.Tuple.vec[i]);
            out(" ");
            out(vars[i].val.s);
            out(";\n");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

void code_decl (Decl d, tce_ret* ret) {
    if (d.type.sub == TYPE_FUNC) {
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
            out(out2);
        out(" ce_arg) {\n");
            if (d.type.Func.out->sub == TYPE_UNIT) {
                code_expr(*d.init, NULL);
                out(";\n");
            } else {
                code_type(*d.type.Func.out);
                out(" ce_ret;\n");
                Patt pt = (Patt){PATT_SET,.Set={TK_IDVAR,{.s="ce_ret"}}};
                tce_ret r = { &pt, NULL };
                code_expr(*d.init, &r);
                out(";\n");
                out("return ce_ret;\n");
            }
        out("}\n\n");
    } else {
        if (d.init == NULL) {
            code_patt_decls(d.patt, d.type);
        } else {
            code_patt_decls(d.patt, d.type);
            code_patt_set(d.patt, *d.init);
            out(";\n");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

void code_expr (Expr e, tce_ret* ret) {
    if (e.nested != NULL) {
        out("{\n");
        code_expr(*e.nested, NULL);
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
            out(e.Var.val.s);
            break;
        case EXPR_CONS:
            code_ret(ret);
            out(e.Cons.val.s);
            break;
        case EXPR_NEW:
            code_ret(ret);
            out("({");
            out("typeof(");
            code_expr(*e.New, NULL);
            out(")* ptr = malloc(sizeof(");
            code_expr(*e.New, NULL);
            out(")) ; *ptr=");
            code_expr(*e.New, NULL);
            out("; ptr; })");
            break;
        case EXPR_SET: {
            Patt pt = (Patt) { PATT_SET, .Set=e.Set.var };
            tce_ret r = { &pt, ret };
            code_expr(*e.Set.val, &r);
            break;
        }
        case EXPR_CALL:
            code_ret(ret);
            code_expr(*e.Call.func, NULL);
            out("(");
            if (e.Call.func->sub == EXPR_RAW) {
                if (e.Call.arg->sub == EXPR_TUPLE) {
                    for (int i=0; i<e.Call.arg->Tuple.size; i++) {
                        if (i > 0) {
                            out(", ");
                        }
                        code_expr(e.Call.arg->Tuple.vec[i], NULL);
                    }
                } else {
                    code_expr(*e.Call.arg, NULL);
                }
            } else if (e.Call.func->sub == EXPR_CONS) {
                code_expr(*e.Call.arg, NULL);
            } else {
                out("(typeof(TYPE_");
                code_expr(*e.Call.func, NULL);
                out("))");
                code_expr(*e.Call.arg, NULL);
            }
            out(")");
            break;
        case EXPR_TUPLE:
            code_ret(ret);
            if (ret != NULL) {
                out("(typeof(");
                out(ret->patt->Set.val.s);
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
                code_expr(e.Seq.vec[i], (i==e.Seq.size-1) ? ret : NULL);
                out(";\n");
            }
            break;
        case EXPR_LET: {    // patt,type,init,body
            out("{\n");
            code_patt_decls(e.Let.patt, e.Let.type);
            code_patt_set(e.Let.patt, *e.Let.init);
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
            if (tst.sub != EXPR_TUPLE) {   // prevents multiple evaluation of tst
                out("typeof(");
                code_expr(*e.Cases.tst, NULL);
                out(") ce_tst = ");
                code_expr(*e.Cases.tst, NULL);
                out(";\n");
                tst = (Expr) { EXPR_VAR, NULL, {.Var={TK_IDVAR,{.s="ce_tst"}}} };
            }

            for (int i=0; i<e.Cases.size; i++) {
                Expr tst_ = tst;
                Case c = e.Cases.vec[i];
                Expr star = (Expr) { EXPR_RAW, NULL, .Raw={TK_RAW,{.s="*"}} };
                Expr old  = tst_;
                if (c.patt.sub==PATT_CONS && is_rec(c.patt.Cons.data.val.s)) {
                    tst_ = (Expr) { EXPR_CALL, NULL, .Call={&star,&old} };
                }
                out("if (");
                code_patt_match(c.patt, tst_);
                out(") {\n");
                code_patt_decls(c.patt, c.type);
                code_patt_set(c.patt, tst_);
                out(";\n");
                code_expr(*c.expr, ret);
                out(";\n");
                out("} else ");
            }
            out("{\n");
            out("assert(0 && \"match failed\");\n");
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
            out("assert(0 && \"if failed\");\n");
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
        case EXPR_CONS_SUB:
            code_ret(ret);
            out("(");
            code_expr(*e.Cons_Sub.cons, NULL);
            out(")._");
            out(e.Cons_Sub.sub);
            break;
        default:
//printf("%d\n", e.sub);
            assert(0 && "TODO");
    }
    if (e.nested != NULL) {
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
