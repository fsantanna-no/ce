typedef enum {
    TYPE_UNIT,
    TYPE_DATA
} TYPE;

typedef enum {
    EXPR_UNIT,
    EXPR_VAR,
    EXPR_CONS,
    EXPR_SET,
    EXPR_FUNC,
    EXPR_SEQ,
    EXPR_CALL,
    EXPR_BLOCK
} EXPR;

typedef enum {
    GLOB_DATAS,
    GLOB_DECL,
    GLOB_EXPR
} GLOB;

///////////////////////////////////////////////////////////////////////////////

/*
 *
 * Cons  ::= IDDATA `=` Type
 * Data  ::= data IDDATA [`=` Type] [`:` { Cons }]
 *
 * Decl  ::= IDVAR `::` Type
 * Decls ::= { Decl }
 *
 * Expr  ::= `(` `)` | IDVAR | IDDATA
 *        |  set IDVAR `=` Expr
 *        |  func `::` Type Expr
 *        |  `:` { Expr }           // sequence
 *        |  Expr `(` Expr `)`      // call
 *        |  Expr `:` { Decl }      // block
 *        | `(` Expr `)`
 */

typedef struct {
    TYPE sub;
    Tk   tk;
} Type;

typedef struct {
    int  idx;
    Tk   tk;
    Type type;
} Cons;

typedef struct {
    Tk    tk;
    int   size;
    Cons* vec;
} Data;

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
} Seq;

typedef struct Expr {
    EXPR sub;
    union {
        Tk tk;          // EXPR_UNIT, EXPR_VAR, EXPR_CONS
        Seq seq;        // EXPR_SEQ
        struct {        // EXPR_SET
            Tk var;
            struct Expr* val;
        } Set;
        struct {        // EXPR_CALL
            struct Expr* func;
            struct Expr* arg;
        } Call;
        struct {        // EXPR_BLOCK
            struct Expr* ret;
            Decls* decls;
        } Block;
        struct {        // EXPR_FUNC
            Type type;
            struct Expr* body;
        } Func;
    };
} Expr;

typedef struct {
    int sub;
    union {
        Data datas;
        Decl decl;
        Expr expr;
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
int parser_data  (Data*  ret);
int parser_decls (Decls* ret);
int parser_expr  (Expr*  ret);
int parser_prog  (Prog* prog);
