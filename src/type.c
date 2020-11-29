#include "all.h"

Data* data_get (const char* id) {
    for (int i=0; i<ALL.prog.size; i++) {
        Glob g = ALL.prog.vec[i];
        if (g.sub==GLOB_DATA && !strcmp(id,g.data.tk.val.s)) {
            return &ALL.prog.vec[i].data;
        }
    }
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////

Cons* cons_get (Data data, const char* id) {
    for (int i=0; i<data.size; i++) {
        if (!strcmp(id,data.vec[i].tk.val.s)) {
            return &data.vec[i];
        }
    }
    return NULL;
}

Data* cons_sup (const char* id, Cons* sub) {
    for (int i=0; i<ALL.prog.size; i++) {
        Glob g = ALL.prog.vec[i];
        if (g.sub == GLOB_DATA) {
            Cons* cons = cons_get(g.data, id);
            if (cons != NULL) {
                if (sub != NULL) {
                    *sub = *cons;
                }
                return &ALL.prog.vec[i].data;
            }
        }
    }
    return NULL;
}
