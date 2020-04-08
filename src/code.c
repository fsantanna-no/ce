#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "lexer.h"
#include "parser.h"
#include "code.h"

void out (const char* v) {
    fputs(v, ALL.out);
}

void code_spc (int spc) {
    for (int i=0; i<spc; i++) {
        out(" ");
    }
}

void code_ret (tce_ret* ret) {
    while (ret != NULL) {
        out(ret->val);
        out(" = ");
        ret = ret->nxt;
    }
}

///////////////////////////////////////////////////////////////////////////////

void code_type (Type tp) {
    switch (tp.sub) {
        case TYPE_UNIT:
            out("int");
            break;
        case TYPE_DATA:
            out(tp.Data.val.s);
            break;
        case TYPE_TUPLE:
            //out("\nstruct Tuple_XXX {\n");
            //out("}\n\n");
            //out("Tuple_XXX");
            out("struct { ");
            for (int i=0; i<tp.Tuple.size; i++) {
                code_type(tp.Tuple.vec[i]);
                fprintf(ALL.out, " _%d; ", i);
            }
            out(" }");
            break;
        default:
//printf("%d\n", tp.sub);
            assert(0 && "TODO");
    }
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
            out("(v)");
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
            out("=v");
        }
        out(" })\n");
    }
    out("\n");

    out("typedef enum {\n");
        for (int i=0; i<data.size; i++) {
            char* v = data.vec[i].tk.val.s;
            out("    ");
            out(sup);
            out("_");
            out(v);
            if (i < data.size-1) {
                out(",");
            }
            out("\n");
        }
    out("} ");
    out(SUP);
    out(";\n");
    out("\n");
    out("typedef struct ");
    out(sup);
    out(" {\n");
    out("    ");
    out(SUP);
    out(" sub;\n");
    out("    union {\n");
        for (int i=0; i<data.size; i++) {
            Cons cons = data.vec[i];
            if (cons.type.sub != TYPE_UNIT) {
                assert(data.size==1 && "TODO");
                out("        ");
                code_type(cons.type);
                out(" _");
                out(cons.tk.val.s);
                out(";\n");
            }
        }
    out("    };\n");
    out("} ");
    out(sup);
    out(";\n\n");
}

///////////////////////////////////////////////////////////////////////////////

void code_case_tst (Expr tst, Patt p) {
    switch (p.sub) {
        case PATT_ANY:
        case PATT_SET:
            out("1");
            break;
        case PATT_UNIT:
            code_expr(0, tst, NULL);
            out(" == 1");
            break;
        case PATT_CONS:
            out("toint(");
            code_expr(0, tst, NULL);
            out(") == SUP_");
            out(p.Cons.data.val.s);
            if (p.Cons.arg != NULL) {
                out(" && ");
                code_case_tst (
                    (Expr) { EXPR_CONS_SUB, .Cons_Sub={&tst,p.Cons.data.val.s} },
                    *p.Cons.arg
                );
            }
            break;
        case PATT_TUPLE:
            for (int i=0; i<p.Tuple.size; i++) {
                if (i > 0) {
                    out(" && ");
                }
                code_case_tst (
                    (Expr) { EXPR_TUPLE_IDX, .Tuple_Idx={&tst,i} },
                    p.Tuple.vec[i]
                );
            }
            break;
        default:
            assert(0 && "TODO");
    }
}

void code_case_set (int spc, Expr tst, Patt p) {
    switch (p.sub) {
        case PATT_ANY:
        case PATT_UNIT:
            break;
        case PATT_SET:          // x = ce_tst
            code_spc(spc);
            out(p.Set.val.s);
            out(" = ");
            out("*(typeof(");
            out(p.Set.val.s);
            out(")*) &");
            code_expr(0, tst, NULL);
            out(";\n");
            break;
        case PATT_CONS:
            if (p.Cons.arg != NULL) {
                code_case_set (
                    spc,
                    (Expr) { EXPR_CONS_SUB, .Cons_Sub={&tst,p.Cons.data.val.s} },
                    *p.Cons.arg
                );
            }
            break;
        case PATT_TUPLE:
            for (int i=0; i<p.Tuple.size; i++) {
                code_case_set (
                    spc,
                    (Expr) { EXPR_TUPLE_IDX, .Tuple_Idx={&tst,i} },
                    p.Tuple.vec[i]
                );
            }
            break;
        default:
            assert(0 && "TODO");
    }
}

void code_case_vars (Tk* vars, int* vars_i, Patt patt) {
    switch (patt.sub) {
        case PATT_ANY:
        case PATT_UNIT:
            break;
        case PATT_SET:
            assert(*vars_i < 16);
            vars[(*vars_i)++] = patt.Set;
            break;
        case PATT_CONS:
            if (patt.Cons.arg != NULL) {
                code_case_vars(vars, vars_i, *patt.Cons.arg);
            }
            break;
        case PATT_TUPLE:
            for (int i=0; i<patt.Tuple.size; i++) {
                code_case_vars(vars, vars_i, patt.Tuple.vec[i]);
            }
            break;
        default:
            assert(0 && "TODO");
    }
}

void code_case (int spc, Expr tst, Case c, tce_ret* ret) {
    out("if (");
    code_case_tst(tst, c.patt);
    out(") {\n");
    {
        Tk vars[16];
        int vars_i = 0;
        code_case_vars(vars, &vars_i, c.patt);
        if (vars_i == 1) {
            code_spc(spc+4);
            code_type(*c.type);
            out(" ");
            out(vars[0].val.s);
            out(";\n");
        } else {
            for (int i=0; i<vars_i; i++) {
                code_spc(spc+4);
                code_type(c.type->Tuple.vec[i]);
                out(" ");
                out(vars[i].val.s);
                out(";\n");
            }
        }
    }
    code_case_set(spc+4, tst, c.patt);
    code_spc(spc+4);
    code_expr(spc+4, *c.expr, ret);
    out(";");
    out("\n");
    code_spc(spc);
    out("} else ");
}

///////////////////////////////////////////////////////////////////////////////

void code_expr (int spc, Expr e, tce_ret* ret) {
    if (e.decls != NULL) {
        code_spc(spc-4);
        out("{\n");
        code_decls(spc, *e.decls);
    }
    switch (e.sub) {
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
            if (ret != NULL) {
                out("*(typeof(");
                out(ret->val);
                out(")*) &");
            }
            out(e.Var.val.s);
            break;
        case EXPR_CONS: {
            code_ret(ret);
            out(e.Cons.val.s);
            break;
        }
        case EXPR_SET: {
            tce_ret r = { e.Set.var.val.s, ret };
            code_expr(spc, *e.Set.val, &r);
            break;
        }
        case EXPR_CALL:
            code_ret(ret);
            code_expr(spc, *e.Call.func, NULL);
            out("(");
            code_expr(spc, *e.Call.arg, NULL);
            out(")");
            break;
        case EXPR_TUPLE:
            code_ret(ret);
            if (ret != NULL) {
                out("(typeof(");
                out(ret->val);
                out(")) ");
            }
            out("{ ");
            for (int i=0; i<e.Tuple.size; i++) {
                //fprintf (ALL.out, "%c _%d=", ((i==0) ? ' ' : ','), i);
                if (i != 0) {
                    out(",");
                }
                code_expr(spc+4, e.Tuple.vec[i], NULL);
            }
            out(" }");
            break;
        case EXPR_FUNC:
            assert(ret!=NULL && ret->nxt==NULL);    // set f = func (only supported)
            out("\n");
            code_type(*e.Func.type.Func.out);
                out(" ");
                out(ret->val);
                out(" (");
                code_type(*e.Func.type.Func.inp);
            out(" ce_arg) {\n");
                code_spc(spc+4);
                code_type(*e.Func.type.Func.out);
                out(" ce_ret;\n");
                tce_ret r = { "ce_ret", NULL };
                code_expr(spc+4, *e.Func.body, &r);
                code_spc(spc+4);
                out("return ce_ret;\n");
            out("}\n\n");
            break;
        case EXPR_SEQ:
            for (int i=0; i<e.Seq.size; i++) {
                code_spc(spc);
                code_expr(spc, e.Seq.vec[i], (i==e.Seq.size-1) ? ret : NULL);
            }
            break;
        case EXPR_CASES:
            code_spc(spc);
            for (int i=0; i<e.Cases.size; i++) {
                code_case(spc, *e.Cases.tst, e.Cases.vec[i], ret);
            }
            out("{\n");
            code_spc(spc+4);
            out("assert(0 && \"case not matched\");\n");
            code_spc(spc);
            out("}\n");
            break;
        case EXPR_TUPLE_IDX:
            code_expr(spc, *e.Tuple_Idx.tuple, ret);
            fprintf(ALL.out, "._%d", e.Tuple_Idx.idx);
            break;
        case EXPR_CONS_SUB:
            code_expr(spc, *e.Cons_Sub.cons, ret);
            out("._");
            out(e.Cons_Sub.sub);
            break;
        default:
//printf("%d\n", e.sub);
            assert(0 && "TODO");
    }
    if (e.decls != NULL) {
        out(";\n");
        code_spc(spc-4);
        out("}\n");
    }
}

///////////////////////////////////////////////////////////////////////////////

void code_decl (int spc, Decl d) {
    code_type(d.type);
    out(" ");
    out(d.var.val.s);
    out(";\n");
    if (d.set != NULL) {
        tce_ret r = { d.var.val.s, NULL };
        code_expr(spc, *d.set, &r);
        code_spc(spc);
        out(";\n");
    }
}

void code_decls (int spc, Decls ds) {
    for (int i=0; i<ds.size; i++) {
        code_spc(spc);
        code_decl(spc, ds.vec[i]);
    }
}

///////////////////////////////////////////////////////////////////////////////

void code_prog (int spc, Prog prog) {
    for (int i=0; i<prog.size; i++) {
        Glob g = prog.vec[i];
        switch (g.sub) {
            case GLOB_DATA:
                code_data(g.data);
                break;
            case GLOB_DECL:
                code_decl(spc, g.decl);
                break;
            case GLOB_EXPR:
                code_expr(spc, g.expr, NULL);
                out(";\n");
                break;
        }
    }
}

void code (Prog prog) {
    out (
        "#include \"ce.c\"\n"
        "int main (void) {\n"
        "\n"
    );
    code_prog(0, prog);
    fprintf(ALL.out, "\n");
    out("}\n");
}

void compile (const char* inp) {
    FILE* f = popen("gcc -xc -", "w");
    assert(f != NULL);
    fputs(inp, f);
    fclose(f);
}
