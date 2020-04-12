typedef enum {
    TYPE_NONE,
    TYPE_RAW,
    TYPE_UNIT,
    TYPE_DATA,
    TYPE_FUNC,
    TYPE_TUPLE
} TYPE;

typedef enum {
    PATT_RAW,
    PATT_UNIT,
    PATT_ARG,
    PATT_ANY,
    PATT_CONS,
    PATT_SET,
    PATT_EXPR,
    PATT_TUPLE
} PATT;

typedef enum {
    EXPR_RAW,
    EXPR_ARG,
    EXPR_UNIT,
    EXPR_VAR,
    EXPR_CONS,
    EXPR_NEW,
    EXPR_SET,
    EXPR_FUNC,
    EXPR_TUPLE,
    EXPR_SEQ,
    EXPR_CALL,
    EXPR_BLOCK,
    EXPR_LET,
    EXPR_IF,
    EXPR_MATCH,
    EXPR_CASES,
    ////
    EXPR_TUPLE_IDX,
    EXPR_CONS_SUB
} EXPR;

typedef enum {
    GLOB_DATA,
    GLOB_DECL,
    GLOB_EXPR
} GLOB;

///////////////////////////////////////////////////////////////////////////////

/*
 *
 * Prog  ::= { Data | Decl | Expr }
 *
 * Data  ::= data <Id> [`=` Type] [`:` { Cons }]
 * Cons  ::= <Id> `=` Type
 *
 * Type  ::= `(` `)` | <Id>
 *        |  Type `->` Type
 *        | `(` Type { `,` Type } `)`
 *        | `(` Type `)`
 *
 * Decl  ::= (val | mut) Patt `::` Type [`=` Expr [Where]]
 * Where ::= where `:` { Decl }
 *
 * Patt  ::=  `(` `)` | `_` | <id>
 *        |   `~` Expr
 *        |   <Id> [ `(` Patt `)` ]
 *        |   `(` Patt { `,` Patt } `)`
 *
 * Expr  ::= `(` `)` | `...` | <id>             // EXPR_UNIT | EXPR_ARG | EXPR_VAR
 *        |  <Id> [`(` Expr `)`]                // EXPR_CONS
 *        | `{` <...> `}`                       // EXPR_RAW
 *        | `(` Expr { `,` Expr } `)`           // EXPR_TUPLE
 *        |  func `::` Type Expr [Where]        // EXPR_FUNC
 *        |  Expr `(` Expr `)`                  // EXPR_CALL
 *        | `(` Expr `)`
 *        |  set <id> `=` Expr                  // EXPR_SET
 *        |  `:` { Expr [Where] }               // EXPR_SEQ
 *        |  case Expr `:`                      // EXPR_CASE
 *               { Patt [`::` Type] [`->`] Expr [Where] }
 *        |  let Decl [`->`] Expr               // EXPR_LET
 *        |  `if` Expr [`->`] Expr [`->`] Expr  // EXPR_IF
 *        |  Expr `~` Expr                      // EXPR_MATCH
 */

struct Expr;

typedef struct Type {
    TYPE sub;
    union {
        Tk Raw;
        Tk Data;
        struct {        // TYPE_FUNC
            struct Type* inp;
            struct Type* out;
        } Func;
        struct {        // TYPE_TUPLE
            int size;
            struct Type* vec;
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
    int   size;     // size=0: recursive pre declaration
    Cons* vec;
} Data;

typedef struct Patt {
    PATT sub;
    union {
        Tk Raw;             // PATT_RAW
        Tk Set;             // PATT_SET
        struct Expr* Expr;  // PATT_SET
        struct {            // PATT_TUPLE
            int size;
            struct Patt* vec;
        } Tuple;
        struct {            // PATT_CONS
            Tk data;
            struct Patt* arg;
        } Cons;
    };
} Patt;

typedef struct {
    Patt patt;
    Type type;
    struct Expr* init;
} Decl;

typedef struct {
    int   size;
    Decl* vec;
} Decls;

typedef struct {
    Patt patt;
    Type type;
    struct Expr* expr;
} Case;

typedef struct Expr {
    EXPR  sub;
    Decls decls;
    union {
        Tk Raw;
        Tk Unit;
        Tk Var;
        Tk Cons;
        struct Expr* New;
        struct {        // EXPR_TUPLE
            int size;
            struct Expr* vec;
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
        struct {        // EXPR_LET
            Patt patt;
            Type type;
            struct Expr* init;
            struct Expr* body;
        } Let;
        struct {        // EXPR_IF
            struct Expr* tst;
            struct Expr* true;
            struct Expr* false;
        } Cond;
        struct {        // EXPR_MATCH
            struct Expr* expr;
            struct Patt* patt;
        } Match;
        struct {        // EXPR_CASES
            struct Expr* tst;
            int   size;
            Case* vec;
        } Cases;
        struct {        // EXPR_FUNC
            Type type;
            struct Expr* body;
        } Func;
        ////
        struct {
            struct Expr* tuple;
            int idx;
        } Tuple_Idx;
        struct {
            struct Expr* cons;
            const char* sub;
        } Cons_Sub;
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

int is_rec (const char* v);
void dump_expr (Expr e);
void init (FILE* out, FILE* inp);

int parser_type  (Type*  ret);
int parser_data  (Data*  ret);
int parser_decls (Decls* ret);
int parser_patt  (Patt*  ret);
int parser_where (Decls* ds);
int parser_expr  (Expr*  ret);
int parser_prog  (Prog*  prog);
