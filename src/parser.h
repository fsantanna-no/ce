typedef enum {
    TYPE_NONE = 0,
    TYPE_UNIT
} TYPE;

typedef enum {
    EXPR_NONE = 0,
    EXPR_UNIT,
    EXPR_VAR,
    EXPR_CONS,
    EXPR_SET,
    EXPR_CALL,
    EXPR_FUNC,
    EXPR_EXPRS
} EXPR;

typedef enum {
    DECLS_NONE = 0,
    DECLS_SOME
} DECLS;

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

typedef struct {
    Tk   var;
    Type type;
} Decl;

typedef struct {
    DECLS sub;
    union {
        Error err;
        struct {
            int size;
            Decl* vec;
        };
    };
} Decls;

struct Expr;
typedef struct {
    int size;
    struct Expr* vec;
} Exprs;

typedef struct Expr {
    EXPR sub;
    union {
        Error err;      // EXPR_NONE
        Tk tk;          // EXPR_UNIT, EXPR_VAR, EXPR_CONS
        Exprs exprs;    // EXPR_EXPRS
        struct {        // EXPR_SET
            Tk var;
            struct Expr* expr;
        } Set;
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

///////////////////////////////////////////////////////////////////////////////

void  parser_init      (FILE* buf);
Type  parser_type      (void);
Expr  parser_expr      (void);
Decls parser_decls     (void);
void  parser_dump_expr (Expr e, int spc);
