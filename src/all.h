#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "lexer.h"
#include "type.h"
#include "env.h"
#include "parser.h"
#include "dump.h"
#include "code.h"

void patt2patts (Patt* patts, int* patts_i, Patt patt);
void env_add (Env** old, Patt patt, Type type);
Type type_expr (Expr expr);
