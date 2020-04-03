typedef enum {
    TYPE_NONE = 0,
    TYPE_UNIT
} TYPE;

typedef enum {
    EXPR_NONE = 0,
    EXPR_UNIT,
    EXPR_VAR,
    EXPR_CONS
} EXPR;

typedef struct {
    long off;
    char msg[256];
} Error;

typedef struct {
    EXPR  sub;
    union {
        Error err;      // EXPR_NONE
        Tk    tk;       // EXPR_UNIT, EXPR_VAR, EXPR_CONS
    };
} Expr;

typedef struct {
    Tk   var;
    TYPE type;
} Decl;

TYPE parser_type ();
Expr parser_expr ();
Decl parser_decl ();
