#include "../all.h"

int all (const char* xp, char* src) {
    static char out[65000];
    all_init (
        stropen("w", sizeof(out), out),
        stropen("r", 0, src)
    );
    if (!parser_prog()) {
        puts(ALL.err);
        return !strcmp(ALL.err, xp);
    }
    code();
    fclose(ALL.out[OGLOB]);
#if 0
puts(">>>");
puts(out);
puts("<<<");
#endif
    remove("a.out");

    // compile
    {
        FILE* f = popen("gcc -xc -", "w");
        assert(f != NULL);
        fputs(out, f);
        fclose(f);
    }

    // execute
    {
        FILE* f = popen("./a.out", "r");
        assert(f != NULL);
        char* cur = out;
        int n = sizeof(out) - 1;
        while (1) {
            char* ret = fgets(cur,n,f);
            if (ret == NULL) {
                break;
            }
            n -= strlen(ret);
            cur += strlen(ret);
        }
    }
#if 0
puts(">>>");
puts(out);
puts("---");
puts(xp);
puts("<<<");
#endif
    return !strcmp(out,xp);
}

void t_all (void) {
    assert(all(
        "1\n",
        "mut a :: ()\nset a = ()\n{ printf(\"%d\\n\",a) }\n"
    ));
}

int main (void) {
    t_all();
    puts("OK");
}
