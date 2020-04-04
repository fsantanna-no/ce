typedef enum {
    TYPE_UNIT,
    TYPE_DATA
} TYPE;

typedef enum {
    EXPR_ERR = 0,
    EXPR_UNIT,
    EXPR_VAR,
    EXPR_CONS,
    EXPR_SET,
    EXPR_CALL,
    EXPR_FUNC,
    EXPR_EXPRS
} EXPR;

typedef enum {
    BLOCK_ERR = 0,
    BLOCK_OK
} BLOCK;

///////////////////////////////////////////////////////////////////////////////

typedef struct {
    long off;
    char msg[256];
} Error;

typedef struct {
    TYPE sub;
    Tk   tk;
} Type;

typedef struct {
    Tk   var;
    Type type;
} Decl;

typedef struct {
    int   size;
    Decl* vec;
} Decls;

struct Expr;
typedef struct {
    int size;
    struct Expr* vec;
} Exprs;

typedef struct Expr {
    EXPR sub;
    union {
        Error err;      // EXPR_ERR
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

typedef struct {
    BLOCK sub;
    union {
        Error err;
        struct {
            Decls decls;
            Expr  expr;
        };
    };
} Block;

///////////////////////////////////////////////////////////////////////////////

void  dump_expr (Expr e, int spc);
void  init (FILE* out, FILE* inp);

int parser_type  (Type*  ret);
int parser_decls (Decls* ret);
Expr  parser_expr  (void);
Block parser_block (void);
