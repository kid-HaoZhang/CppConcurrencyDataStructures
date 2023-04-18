#define alloc_assert(x) \
    if (unlikely (!x)) {\
        fprintf (stderr, "FATAL ERROR: OUT OF MEMORY (%s:%d)\n",\
            __FILE__, __LINE__);\
    }