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

Env* env_find (Env* cur, char* want);
