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
    TK   var;
    TYPE type;
} Decl;

TYPE parser_type ();
EXPR parser_expr ();
Decl parser_decl ();
