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
            out("int /* () */");
            break;
        case TYPE_DATA:
            out(tp.Data.val.s);
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
    char* id = data.tk.val.s;
    char ID[256];
    assert(strlen(id) < sizeof(ID));
    strcpy(ID, strupper(id));

    for (int i=0; i<data.size; i++) {
        char* v = data.vec[i].tk.val.s;
        // #define SUP_False Bool_False
        out("#define SUP_");
        out(v);
        out(" ");
        out(id);
        out("_");
        out(v);
        out("\n");

        // #define False() ((Bool) { Bool_False })
        out("#define ");
        out(v);
        out("() ((");
        out(id);
        out(") { ");
        out(id);
        out("_");
        out(v);
        out(" })\n");
    }
    out("\n");

    out("typedef enum {\n");
        for (int i=0; i<data.size; i++) {
            char* v = data.vec[i].tk.val.s;
            out("    ");
            out(id);
            out("_");
            out(v);
            if (i < data.size-1) {
                out(",");
            }
            out("\n");
        }
    out("} ");
    out(ID);
    out(";\n");
    out("\n");
    out("typedef struct ");
    out(id);
    out(" {\n");
    out("    ");
    out(ID);
    out(" sub;\n");
    out("    union {\n");
        for (int i=0; i<data.size; i++) {
            Cons v = data.vec[i];
            if (v.type.sub != TYPE_UNIT) {
                assert(0 && "TODO");
            }
        }
    out("    };\n");
    out("} ");
    out(id);
    out(";\n\n");
}

///////////////////////////////////////////////////////////////////////////////

void code_tst (Patt p) {
    switch (p.sub) {
        case PATT_ANY:
            out("1");
            break;
        case PATT_UNIT:
            out("ce_tst == 0");
            break;
        case PATT_CONS:
            out("toint(ce_tst) == SUP_");
            out(p.Cons.val.s);
            break;
        default:
            assert(0 && "TODO");
    }
}

void code_case (int spc, Case e, tce_ret* ret) {
    out("if (");
    code_tst(e.patt);
    out(") {\n");
    code_spc(spc+4);
    code_expr(spc+4, *e.expr, ret);
    out(";");
    out("\n");
    code_spc(spc);
    out("} else ");
}

void code_expr (int spc, Expr e, tce_ret* ret) {
    switch (e.sub) {
        case EXPR_ARG:
            code_ret(ret);
            out("ce_arg");
            break;
        case EXPR_UNIT:
            code_ret(ret);
            out("0");
            break;
        case EXPR_VAR:
            code_ret(ret);
            out(e.Var.val.s);
            break;
        case EXPR_CONS: {
            code_ret(ret);
            out(e.Cons.val.s);
            out("()");
            break;
        }
        case EXPR_SET: {
            tce_ret r = { e.Set.var.val.s, ret };
            code_expr(spc, *e.Set.val, &r);
            break;
        }
        case EXPR_CALL:
            code_expr(spc, *e.Call.func, ret);
            out("(");
            code_expr(spc, *e.Call.arg, ret);
            out(")");
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
        case EXPR_BLOCK:
            code_decls(spc, *e.Block.decls);
            code_spc(spc);
            code_expr (spc, *e.Block.ret, ret);
            break;
        case EXPR_CASES:
            // typeof(tst) ce_tst = tst;
            out("typeof(");
            code_expr(spc, *e.Cases.tst, NULL);
            out(") ce_tst = ");
            code_expr(spc, *e.Cases.tst, NULL);
            out(";\n");

            code_spc(spc);
            for (int i=0; i<e.Cases.size; i++) {
                code_case(spc, e.Cases.vec[i], ret);
            }
            out("{\n");
            code_spc(spc+4);
            out("assert(0 && \"case not matched\");\n");
            code_spc(spc);
            out("}\n");
            break;
        default:
//printf("%d\n", e.sub);
            assert(0 && "TODO");
    }
}

///////////////////////////////////////////////////////////////////////////////

void code_decl (int spc, Decl d) {
    code_type(d.type);
    out(" ");
    out(d.var.val.s);
    if (d.set != NULL) {
        out(" = ");
        code_expr(spc, *d.set, NULL);
    }
    out(";\n");
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
