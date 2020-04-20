#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

FILE* stropen  (const char* mode, size_t size, char* str);

#include "lexer.h"

enum { OGLOB, ODECL, OEXPR };
typedef struct {
    FILE* inp;
    FILE* out[3];
    char  err[256];
    int   ind;
    struct {
        int size;
        Tk  buf[256];
    } data_recs;
} State_All;

typedef struct {
    long  off;   // position before token (to fallback)
    long  lin;   // line before token
    long  col;   // column before token
    Tk    tk;
} State_Tok;

extern State_All ALL;
extern State_Tok TOK0, TOK1, TOK2;

#include "ll.h"

#include "type.h"

typedef struct Env_Plain {
    Tk   id;
    Type type;
} Env_Plain;

enum { ENV_PLAIN, ENV_HUB };
typedef struct Env {
    int sub;
    struct Env* prev;
    union {
        struct Env* Hub;
        Env_Plain Plain;
    };
} Env;

#include "parser.h"
#include "dump.h"

typedef struct tce_ret {
    Env_Plain env;
    struct tce_ret* nxt;
} tce_ret;

#include "code.h"

int   all_rec  (const char* v);
void  all_init (FILE* out, FILE* inp);

void patt2patts (Patt* patts, int* patts_i, Patt patt);

void  env_add  (Env** old, Patt patt, Type type);
Type* env_get  (Env* cur, char* want, Env* stop);
Type* env_expr (Expr expr);
