#include "all.h"

void patt2patts (Patt* patts, int* patts_i, Patt patt) {
    switch (patt.sub) {
        case PATT_RAW:
        case PATT_ANY:
        case PATT_UNIT:
        case PATT_EXPR:
            break;
        case PATT_SET:
            assert(*patts_i < 16);
//puts(patt.Set.val.s);
            patts[(*patts_i)++] = patt;
            break;
        case PATT_CONS:
            if (patt.Cons.arg != NULL) {
                patt2patts(patts, patts_i, *patt.Cons.arg);
            }
            break;
        case PATT_TUPLE:
            for (int i=0; i<patt.Tuple.size; i++) {
                patt2patts(patts, patts_i, patt.Tuple.vec[i]);
            }
            break;
        default:
            assert(0 && "TODO");
    }
}

Type* env_get (Env* cur, char* want, Env* stop) {
//printf("want %s [%p] // have ", want, cur);
    if (cur==NULL || cur==stop) {
        //puts("null");
        return NULL;
    }
    switch (cur->sub) {
        case ENV_HUB: {
            Type* x = env_get(cur->Hub, want, cur->prev);
            if (x != NULL) {
                return x;
            }
            break;
        }
        case ENV_PLAIN:
//puts(cur->Plain.id.val.s);
            if (!strcmp(cur->Plain.id.val.s, want)) {
                return &cur->Plain.type;
            }
    }
    return env_get(cur->prev, want, stop);
}

void env_add (Env** old, Patt patt, Type type) {
    Patt patts[16];
    int patts_i = 0;
    patt2patts(patts, &patts_i, patt);
    if (patts_i > 1) {
        assert(type.sub == TYPE_TUPLE);
        for (int i=0; i<patts_i; i++) {
            env_add(old, patts[i], type.Tuple.vec[i]);
        }
        return;
    }

    assert(patts_i == 1);
    assert(patts[0].sub == PATT_SET);

    Env* new = malloc(sizeof(Env));
    *new = (Env) { ENV_PLAIN, *old, .Plain={patts[0].Set, type} };
    *old = new;
//printf("add %s\n", patts[0].Set.val.s);
}

///////////////////////////////////////////////////////////////////////////////

Type env_expr (Expr expr) {
    switch (expr.sub) {
        case EXPR_UNIT:
            return (Type) { TYPE_UNIT };
        case EXPR_ARG:
            return *env_get(expr.env, "ce_inp", NULL);
        case EXPR_VAR:
            return *env_get(expr.env, expr.Var.val.s, NULL);
        case EXPR_CONS:
            return (Type) { TYPE_DATA, .Data={expr.Cons,-1} };
        case EXPR_CALL: {
            Type type = env_expr(*expr.Call.func);
            if (type.sub == TYPE_FUNC) {
                return *type.Func.out;
            }
        }
        default:
            printf(">>>>>> ENV_EXPR %d <<<<<<\n", expr.sub);
            return (Type) { TYPE_NONE };
            //assert(0 && "TODO");
    }
}
