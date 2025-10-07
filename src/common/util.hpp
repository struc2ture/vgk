#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// ======================== LOGGING ========================

#define fatal(FMT, ...) do { \
    fprintf(stderr, "[FATAL: %s:%d:%s]: " FMT "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
    abort(); \
} while (0)

#define warning(FMT, ...) do { \
    printf("[WARNING: %s:%d:%s]: " FMT "\n",  __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
} while (0)

#define trace(FMT, ...) do { \
    printf("[TRACE: %s:%d:%s]: " FMT "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
} while (0)

// ======================== ASSERTS ========================

#undef assert
#define assert(cond) do { \
    if (!(cond)) { \
        abort(); \
    } \
} while (0)

#define bassert(EXPR) do { \
    if (!(EXPR)) { \
        bp(); \
        warning("bassert failed: %s", #EXPR); \
    } \
} while (0)

#define bassertf(EXPR, FMT, ...) do { \
    if (!(EXPR)) { \
        bp(); \
        warning("bassertf failed: %s | " FMT, #EXPR, ##__VA_ARGS__); \
    } \
} while (0)

// ======================== UTIL MACROS ========================

#define noop() do {} while (0)
#define bp() __builtin_debugtrap()
#define array_count(ARRAY) (sizeof(ARRAY)/sizeof(ARRAY[0]))

// ======================== DYNAMIC LIST ========================

#define list_define_type(NAME, TYPE) \
    typedef struct NAME { \
        TYPE *data; \
        size_t size; \
        size_t cap; \
    } NAME

#define list_init(LIST, CAP) \
    do { \
        (LIST)->cap = (CAP); \
        (LIST)->data = xrealloc((LIST)->data, (LIST)->cap * sizeof(*(LIST)->data)); \
    } while(0)

#define list_grow(LIST) \
    do { \
        if ((LIST)->data == NULL) \
        { \
            list_init(LIST, 64); \
        } \
        if ((LIST)->size >= (LIST)->cap) \
        { \
            (LIST)->cap *= 2; \
            (LIST)->data = xrealloc((LIST)->data, (LIST)->cap * sizeof(*(LIST)->data)); \
        } \
    } while (0)

#define list_append(LIST, ITEM) \
    do { \
        list_grow((LIST)); \
        (LIST)->data[(LIST)->size++] = (ITEM); \
    } while (0)

#define list_clear(LIST) \
    do { \
        (LIST)->size = 0; \
    } while (0)

#define list_free(LIST) \
    do { \
        free((LIST)->data); \
        (LIST)->data = NULL; \
        (LIST)->size = 0; \
        (LIST)->cap = 0; \
    } while (0)

#define list_iterate(LIST, INDEX, IT) \
    for (size_t INDEX = 0; INDEX < (LIST)->size && ((IT) = &(LIST)->data[INDEX], 1); ++INDEX)

#define list_erase(LIST, INDEX) \
    do { \
        if ((INDEX) < (LIST)->size) { \
            for (size_t ___i = (INDEX); ___i + 1 < (LIST)->size; ++___i) \
                (LIST)->data[___i] = (LIST)->data[___i + 1]; \
            (LIST)->size--; \
        } \
    } while (0)

// ======================== STDLIB HEAP ========================

static void *xmalloc(size_t size)
{
    void *ptr = malloc(size);
    if (!ptr) fatal("malloc failed for %zu", size);
    return ptr;
}

static void *xcalloc(size_t size)
{
    void *ptr = calloc(1, size);
    if (!ptr) fatal("calloc failed for %zu", size);
    return ptr;
}

static void *xrealloc(void *data, size_t new_size)
{
    void *new_data = realloc(data, new_size);
    if (!new_data) fatal("realloc failed for %zu", new_size);
    return new_data;
}

// ======================== STDLIB STRING ========================

static void *xstrdup(const char *str)
{
    char *new_str = strdup(str);
    if (!new_str) fatal("strdup failed for %s", str);
    return new_str;
}

static char *strf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    size_t size = vsnprintf(NULL, 0, fmt, args) + 1;
    va_end(args);
    char *str = (char *)xmalloc(size);
    va_start(args, fmt);
    vsnprintf(str, size, fmt, args);
    va_end(args);
    return str;
}

// ======================== PLATFORM MACROS ========================

#if defined(_WIN32)
    #define OS_WINDOWS 1
#elif defined(__APPLE__)
    #define OS_MAC 1
#elif defined(__linux__)
    #define OS_LINUX 1
#else
    #error Unknown OS
#endif

// ======================== SIZE MACROS ========================

#define kilobytes(SIZE) (1024ull * (SIZE))
#define megabytes(SIZE) (1024ull * kilobytes(SIZE))
#define gigabytes(SIZE) (1024ull * megabytes(SIZE))
