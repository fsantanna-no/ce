#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

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

#include "type.h"

enum { ENV_PLAIN, ENV_HUB };
typedef struct Env {
    int sub;
    struct Env* prev;
    union {
        struct Env* Hub;
        struct {
            Tk   id;
            Type type;
        } Plain;
    };
} Env;

#include "parser.h"
#include "dump.h"
#include "code.h"

void patt2patts (Patt* patts, int* patts_i, Patt patt);
void env_add (Env** old, Patt patt, Type type);
Type* env_get (Env* cur, char* want);
Type* env_expr (Expr expr);
