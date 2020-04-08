#include <assert.h>
#include <stdio.h>

typedef void (*tce_show) (void* v);
static tce_show SHOW[256];

#define toint(x) (x.sub)
#define show_unit(x) printf("%d\n", x)
