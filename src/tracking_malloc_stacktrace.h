#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct stacktrace_frame {
    const void* address;
    const void* fbase;
    const char* fname;
};

struct stacktrace {
    int count;
    struct stacktrace_frame frames[];
};

typedef const char* (fn_cache_fname)(const void* fbase, const char* fname);

struct stacktrace* stacktrace_create(int skip, int max_depth);
void stacktrace_analysis(struct stacktrace* stacktrace, fn_cache_fname* cache_fname);
void stacktrace_destroy(struct stacktrace* stacktrace);

#ifdef __cplusplus
}
#endif
