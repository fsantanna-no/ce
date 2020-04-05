typedef enum {
    TYPE_UNIT,
    TYPE_DATA
} TYPE;

typedef enum {
    EXPR_UNIT,
    EXPR_VAR,
    EXPR_CONS,
    EXPR_SET,
    EXPR_CALL,
    EXPR_FUNC,
    EXPR_EXPRS
} EXPR;

typedef enum {
    GLOB_DATAS,
    GLOB_DECL,
    GLOB_EXPR
} GLOB;

///////////////////////////////////////////////////////////////////////////////

typedef struct {
    TYPE sub;
    Tk   tk;
} Type;

typedef struct {
    int  idx;
    Tk   tk;
    Type type;
} Data;

typedef struct {
    Tk    tk;
    int   size;
    Data* vec;
} Datas;

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
    Decls decls;
    Expr  expr;
} Block;

typedef struct {
    int sub;
    union {
        Datas datas;
        Decl  decl;
        Expr  expr;
    };
} Glob;

typedef struct {
    int   size;
    Glob* vec;
} Prog;

///////////////////////////////////////////////////////////////////////////////

void dump_expr (Expr e, int spc);
void init (FILE* out, FILE* inp);

int parser_type  (Type*  ret);
int parser_datas (Datas* ret);
int parser_decls (Decls* ret);
int parser_expr  (Expr*  ret);
int parser_block (Block* ret);
int parser_prog  (Prog* prog);
