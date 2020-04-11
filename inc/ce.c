#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define show_Unit(x) (assert(x==1), puts("()"))
#define matches(a,b) ((a) == (b) ? 1 : 0)
