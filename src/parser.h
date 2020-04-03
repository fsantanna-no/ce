typedef enum {
    TYPE_NONE = 0,
    TYPE_UNIT
} TYPE;

typedef enum {
    EXPR_NONE = 0,
    EXPR_UNIT,
    EXPR_VAR,
    EXPR_CONS,
    EXPR_CALL,
    EXPR_FUNC
} EXPR;

typedef enum {
    DECL_NONE = 0,
    DECL_SIG,
    DECL_ATR
} DECL;

///////////////////////////////////////////////////////////////////////////////

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

typedef struct Expr {
    EXPR sub;
    union {
        Error err;      // EXPR_NONE
        Tk    tk;       // EXPR_UNIT, EXPR_VAR, EXPR_CONS
        struct {        // EXPR_CALL
            struct Expr* func;
            struct Expr* expr;
        } Call;
        struct {        // EXPR_FUNC
            Type type;
            struct Expr* expr;
        } Func;
    };
} Expr;

typedef struct {
    DECL sub;
    union {
        Error err;
        struct {        // DECL_SIG
            Tk   var;
            Type type;
        };
        struct {        // DECL_SIG
            Tk   patt;
            Expr expr;
        };
    };
} Decl;

///////////////////////////////////////////////////////////////////////////////

void parser_init (FILE* buf);
Type parser_type (void);
Expr parser_expr (void);
Decl parser_decl (void);
