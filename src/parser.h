typedef enum EXPR {
    EXPR_NONE = 0,
    EXPR_UNIT,
    EXPR_VAR,
    EXPR_CONS
} EXPR;

EXPR parser_expr ();
