#include "all.h"

void dump_env (Env* cur, Env* stop) {
    static int N = 0;
    if (cur==NULL || cur==stop) {
        return;
    }
    for (int i=0; i<N; i++) printf(" ");
    switch (cur->sub) {
        case ENV_HUB:
            printf("hub [%p->%p]:\n", cur, cur->prev);
            N += 2;
            dump_env(cur->Hub, cur->prev);
            N -= 2;
            break;
        case ENV_PLAIN:
            printf("id %s [%p->%p]\n", cur->Plain.id.val.s, cur, cur->prev);
            break;
    }
    return dump_env(cur->prev, stop);
}

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
        case EXPR_DECL:
            fputs("decl (...)", stdout);
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
        case EXPR_CONS_SUB:
            dump_expr_(*e.Cons_Sub.cons, 0);
            fputs("()", stdout);
            break;
        case EXPR_TUPLE_IDX:
            fputs("idx (...)", stdout);
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


