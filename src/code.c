#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "lexer.h"
#include "parser.h"
#include "code.h"

void code_spc (int spc) {
    for (int i=0; i<spc; i++) {
        fputs(" ", ALL.out);
    }
}

void code_ret (tce_ret* ret) {
    while (ret != NULL) {
        fputs(ret->val, ALL.out);
        fputs(" = ", ALL.out);
        ret = ret->nxt;
    }
}

///////////////////////////////////////////////////////////////////////////////

void code_type (Type tp) {
    switch (tp.sub) {
        case TYPE_UNIT:
            fputs("int /* () */", ALL.out);
            break;
        case TYPE_DATA:
            fputs(tp.Data.val.s, ALL.out);
            break;
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

    // TODO: asserts || strncat
    char subs[1024] = "";
    for (int i=0; i<data.size; i++) {
        char* v = data.vec[i].tk.val.s;
        strcat(subs, "    ");
        strcat(subs, ID);
        strcat(subs, "_");
        strcat(subs, strupper(v));  // TODO: assert strupper
        if (i < data.size-1) {
            strcat(subs, ",");
        }
        strcat(subs, "\n");
    }


    char conss[1024] = "";
    for (int i=0; i<data.size; i++) {
        Cons v = data.vec[i];
        if (v.type.sub != TYPE_UNIT) {
            assert(0 && "TODO");
        }
    }

    fprintf (ALL.out,
        "typedef enum {\n"
        "%s"
        "} %s;\n"
        "\n"
        "typedef struct %s {\n"
        "    %s sub;\n"
        "    union {\n"
        "%s"
        "    };\n"
        "} %s;\n\n",
        subs,
        ID,
        id,
        ID,
        conss,
        id
    );
}

///////////////////////////////////////////////////////////////////////////////

void code_patt (Patt p) {
    switch (p.sub) {
        case PATT_ANY:
            fputs("ce_tst", ALL.out);
            break;
        case PATT_UNIT:
            fputs("0", ALL.out);
            break;
        default:
            assert(0 && "TODO");
    }
}

void code_case (int spc, Case e, tce_ret* ret) {
    fputs("if (ce_tst == ", ALL.out);
    code_patt(e.patt);
    fputs(") {\n", ALL.out);
    code_spc(spc+4);
    code_expr(spc+4, *e.expr, ret);
    fputs(";", ALL.out);
    fputs("\n} else ", ALL.out);
}

void code_expr (int spc, Expr e, tce_ret* ret) {
    switch (e.sub) {
        case EXPR_UNIT:
            code_ret(ret);
            fputs("0", ALL.out);
            break;
        case EXPR_VAR:
            code_ret(ret);
            fputs(e.Var.val.s, ALL.out);
            break;
        case EXPR_CONS: {
            char tmp[256];
            strcpy(tmp, e.Cons.val.s);
            code_ret(ret);
            fprintf(ALL.out, "(%s) { %s }", strtok(tmp,"_"), strupper(e.Cons.val.s));
            break;
        }
        case EXPR_SET: {
            tce_ret r = { e.Set.var.val.s, ret };
            code_expr(spc, *e.Set.val, &r);
            break;
        }
        case EXPR_CALL:
            code_expr(spc, *e.Call.func, ret);
            fputs("(", ALL.out);
            code_expr(spc, *e.Call.arg, ret);
            fputs(")", ALL.out);
            break;
        case EXPR_BLOCK:
            code_decls(spc, *e.Block.decls);
            code_spc(spc);
            code_expr (spc, *e.Block.ret, ret);
            break;
        case EXPR_CASES:
            // typeof(tst) ce_tst = tst;
            fputs("typeof(", ALL.out);
            code_expr(spc, *e.Cases.tst, NULL);
            fputs(") ce_tst = ", ALL.out);
            code_expr(spc, *e.Cases.tst, NULL);
            fputs(";\n", ALL.out);

            code_spc(spc);
            for (int i=0; i<e.Cases.size; i++) {
                code_case(spc, e.Cases.vec[i], ret);
            }
            fputs("{\n", ALL.out);
            code_spc(spc+4);
            fputs("assert(0 && \"case not matched\");\n", ALL.out);
            code_spc(spc);
            fputs("}", ALL.out);
            break;
        default:
            printf("%d\n", e.sub);
            assert(0 && "TODO");
    }
}

///////////////////////////////////////////////////////////////////////////////

void code_decl (int spc, Decl d) {
    code_type(d.type);
    fputs(" ", ALL.out);
    fputs(d.var.val.s, ALL.out);
    if (d.set != NULL) {
        fputs(" = ", ALL.out);
        code_expr(spc, *d.set, NULL);
    }
    fputs(";\n", ALL.out);
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
                fputs(";\n", ALL.out);
                break;
        }
    }
}

void code (Prog prog) {
    fputs (
        "#include \"ce.c\"\n"
        "int main (void) {\n"
        "\n",
        ALL.out
    );
    code_prog(0, prog);
    fprintf(ALL.out, "\n");
    fputs("}\n", ALL.out);
}

void compile (const char* inp) {
    FILE* f = popen("gcc -xc -", "w");
    assert(f != NULL);
    fputs(inp, f);
    fclose(f);
}
