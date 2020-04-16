#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Pool {
    void* root;         // pointer to actual value
    int n;              // number of allocated items
    struct Pool* nxt;   // next pool in the scope
} Pool;

#define show_Unit(x) (assert(x==1), puts("()"))
#define matches(a,b) ((a) == (b) ? 1 : 0)
