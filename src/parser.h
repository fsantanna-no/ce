typedef enum {
    TYPE_UNIT,
    TYPE_DATA,
    TYPE_FUNC,
    TYPE_TUPLE
} TYPE;

typedef enum {
    PATT_UNIT,
    PATT_ARG,
    PATT_ANY,
    PATT_CONS,
    PATT_SET,
    PATT_EXP,
    PATT_TUP
} PATT;

typedef enum {
    EXPR_ARG,
    EXPR_UNIT,
    EXPR_VAR,
    EXPR_CONS,
    EXPR_SET,
    EXPR_FUNC,
    EXPR_TUPLE,
    EXPR_SEQ,
    EXPR_CALL,
    EXPR_BLOCK,
    EXPR_CASES
} EXPR;

typedef enum {
    GLOB_DATA,
    GLOB_DECL,
    GLOB_EXPR
} GLOB;

///////////////////////////////////////////////////////////////////////////////

/*
 *
 * Cons  ::= <Id> `=` Type
 * Data  ::= data <Id> [`=` Type] [`:` { Cons }]
 *
 * Type  ::= `(` `)` | <Id>
 *        |  Type `->` Type
 *        | `(` Type { `,` Type } `)`
 *        | `(` Type `)`
 *
 * Decl  ::= <id> `::` Type [`=` Expr]
 * Decls ::= { Decl }
 *
 * Patt  ::=  `(` `)` | `...` | `_`
 *        |   <Id> [ `(` Patt `)`
 *        |   `=` <id>
 *        |   `~` Expr
 *        |   `(` Patt { `,` Patt } `)`
 *
 * Expr  ::= `(` `)` | <id>
 *        |  <Id> [`(` Expr `)`]
 *        |  set <id> `=` Expr
 *        |  func `::` Type Expr
 *        |  case Expr `:` { Patt [`->`] Expr }
 *        |  `:` { Expr }           // sequence
 *        |  Expr `(` Expr `)`      // call
 *        |  Expr `:` { Decl }      // block
 *        | `(` Expr { `,` Expr } `)`
 *        | `(` Expr `)`
 */

struct Expr;

typedef struct Type {
    TYPE sub;
    union {
        Tk Data;
        struct {        // TYPE_FUNC
            struct Type* inp;
            struct Type* out;
        } Func;
        struct {        // TYPE_TUPLE
            int size;
            struct Expr* vec;
        } Tuple;
    };
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
    struct Expr* set;
} Decl;

typedef struct {
    int   size;
    Decl* vec;
} Decls;

typedef struct Patt {
    PATT sub;
    union {
        Tk Set;         // PATT_SET
        struct {        // PATT_CONS
            Tk data;
            struct Patt* arg;
        } Cons;
    };
} Patt;

typedef struct {
    Patt patt;
    struct Expr* expr;
} Case;

typedef struct Expr {
    EXPR sub;
    union {
        Tk Unit;
        Tk Var;
        Tk Cons;
        struct {        // EXPR_TUPLE
            int size;
            struct Type* vec;
        } Tuple;
        struct {        // EXPR_SEQ
            int size;
            struct Expr* vec;
        } Seq;
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
        struct {        // EXPR_CASES
            struct Expr* tst;
            int   size;
            Case* vec;
        } Cases;
        struct {        // EXPR_FUNC
            Type type;
            struct Expr* body;
        } Func;
    };
} Expr;

typedef struct {
    int sub;
    union {
        Data data;
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
