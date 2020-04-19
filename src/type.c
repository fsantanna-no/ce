#include <assert.h>

#include "lexer.h"
#include "parser.h"

Type type_expr (Expr expr) {
    switch (expr.sub) {
        case EXPR_VAR: {
            Env* env = env_find(expr.env, expr.Var.val.s);
            return (Type) { TYPE_NONE };
            break;
        }
        default:
            printf(">>> %d\n", expr.sub);
            assert(0 && "TODO");
    }
}
