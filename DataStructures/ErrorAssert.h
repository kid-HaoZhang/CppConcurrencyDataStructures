#ifdef __GNUC__
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define unlikely(x) !!(x)
#endif


#define alloc_assert(x) \
    if (unlikely (!x)) {\
        fprintf (stderr, "FATAL ERROR: OUT OF MEMORY (%s:%d)\n",\
            __FILE__, __LINE__);\
    }