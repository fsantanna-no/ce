/*
 *
 * Prog  ::= { Data | Expr }
 *
 * Data  ::= data <Id> [`=` Type] [`:` { Cons }]
 * Cons  ::= <Id> `=` Type
 *
 * Type  ::= `(` `)` | <Id>
 *        |  Type `->` Type
 *        | `(` Type { `,` Type } `)`
 *        | `(` Type `)`
 *
 * Patt  ::=  `(` `)` | `_` | <id>
 *        |   `~` Expr
 *        |   <Id> [ `(` Patt `)` ]
 *        |   `(` Patt { `,` Patt } `)`
 *
 * Expr  ::= Expr' [ where `:` { Expr } ]
 * Expr' ::= `(` `)` | `...` | <id>             // EXPR_UNIT | EXPR_ARG | EXPR_VAR
 *        |  <Id> [`(` Expr `)`]                // EXPR_CONS
 *        | `{` <...> `}`                       // EXPR_RAW
 *        | `(` Expr { `,` Expr } `)`           // EXPR_TUPLE
 *        |  func `::` Type Expr                // EXPR_FUNC
 *        |  Expr `(` Expr `)`                  // EXPR_CALL
 *        | `(` Expr `)`
 *        |  set <id> `=` Expr                  // EXPR_SET
 *        |  `:` { Expr }                       // EXPR_SEQ
 *        |  case Expr `:`                      // EXPR_CASE
 *               { Patt [`::` Type] [`->`] Expr }
 *               [ `else` [`->`] Expr ]
 *        |  `if` Expr [`->`] Expr [`->`] Expr  // EXPR_IF
 *        |  `if` `:`                           // EXPR_IFS
 *               { Expr [`->`] Expr }
 *               [ `else` [`->`] Expr ]
 *        |  `loop` Expr                        // EXPR_LOOP
 *        |  `break`                            // EXPR_BREAK
 *        |  Expr `~` Expr                      // EXPR_MATCH
 *        |  let Patt `::` Type [`->`] Expr     // EXPR_LET
 *        |  (val | mut) Patt `::` Type         // EXPR_DECL
 *               [`=` Expr]
 *
 */

///////////////////////////////////////////////////////////////////////////////

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
    EXPR_RETURN,
    EXPR_TUPLE,
    EXPR_SEQ,
    EXPR_CALL,
    EXPR_BLOCK,
    EXPR_LET,
    EXPR_DECL,
    EXPR_IF,
    EXPR_IFS,
    EXPR_MATCH,
    EXPR_CASES,
    EXPR_LOOP,
    EXPR_BREAK,
    EXPR_PASS,
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
        struct Expr* Expr;  // PATT_EXPR
        struct {            // PATT_SET
            Tk  id;
            int size;       // -1 if not pool, 0 if unbounded, n if bounded
        } Set;
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

typedef struct Patt_Type {
    Patt patt;
    Type type;
} Patt_Type;

typedef struct Decl {
    Patt patt;
    Type type;
    struct Expr* init;
} Decl;

enum { ENV_PLAIN, ENV_HUB };
typedef struct Env {
    int sub;
    struct Env* prev;
    union {
        struct Env* Hub;
        struct {
            Tk   id;
            int  size;       // -1 if not pool, 0 if unbounded, n if bounded
            Type type;
        } Plain;
    };
} Env;

typedef struct {
    Decl decl;
    struct Expr* body;
} Let;

typedef struct {
    struct Expr* tst;
    struct Expr* ret;
} If;

typedef struct Expr {
    EXPR  sub;
    State_Tok tok;
    Env*  env;
    struct Expr* where;    // block that executes/declares before expression
    union {
        Tk Raw;
        Tk Unit;
        Tk Var;
        Tk Cons;
        Decl Decl;              // EXPR_DECL
        Let  Let;               // EXPR_LET
        struct Expr* New;       // EXPR_NEW
        struct Expr* Loop;      // EXPR_LOOP
        struct Expr* Break;     // EXPR_BREAK
        struct Expr* Return;    // EXPR_RETURN
        struct {        // EXPR_TUPLE
            int size;
            struct Expr* vec;
        } Tuple;
        struct {        // EXPR_SEQ
            int size;
            struct Expr* vec;
        } Seq;
        struct {        // EXPR_SET
            Patt patt;
            struct Expr* expr;
        } Set;
        struct {        // EXPR_CALL
            struct Expr* func;
            struct Expr* arg;
            struct Patt* out;       // l[]=f() -> f(l)
        } Call;
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
            int  size;
            Let* vec;
        } Cases;
        struct {        // EXPR_CASES
            int size;
            If* vec;
        } Ifs;
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
        Expr expr;
    };
} Glob;

typedef struct {
    int   size;
    Glob* vec;
} Prog;

///////////////////////////////////////////////////////////////////////////////

typedef struct {
    int size;
    void* vec;
} List;
typedef void* (*List_F) (Env** env);
int parser_list_line (Env** env, int global, List* ret, List_F f, size_t unit);

int is_rec (const char* v);
void init (FILE* out, FILE* inp);
FILE* stropen (const char* mode, size_t size, char* str);

Env* env_find (Env* cur, char* want);
void patt2patts (Patt* patts, int* patts_i, Patt patt);

int parser_type (Type* ret);
int parser_data (Data* ret);
int parser_patt (Env* env, Patt* ret, int is_match);
int parser_expr (Env** env, Expr* ret);
int parser_prog (Prog* prog);
