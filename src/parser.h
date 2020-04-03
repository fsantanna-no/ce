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

typedef enum {
    DECL_NONE = 0,
    DECL_SIG
} DECL;

typedef struct {
    long off;
    char msg[256];
} Error;

typedef struct {
    TYPE sub;
    union {
        Error err;      // TYPE_NONE
        Tk    tk;       // TYPE_DATA
    };
} Type;

typedef struct {
    EXPR sub;
    union {
        Error err;      // EXPR_NONE
        Tk    tk;       // EXPR_UNIT, EXPR_VAR, EXPR_CONS
    };
} Expr;

typedef struct {
    DECL sub;
    union {
        Error err;
        struct {
            Tk   var;
            Type type;
        };
    };
} Decl;

Type parser_type ();
Expr parser_expr ();
Decl parser_decl ();
