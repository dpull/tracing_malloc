#define _GNU_SOURCE
#include <dlfcn.h>
#include "tracking_malloc_stacktrace.h"
#include "tracking_malloc.h"
#include <execinfo.h>
#include <string.h>

struct stacktrace* stacktrace_create(int skip, int max_depth)
{
    int size = skip + max_depth;
    if (skip < 0 || max_depth <= 0 || size == 0)
        return NULL;
    
    void* buffer[size];
    int count = backtrace(buffer, size);
    count -= skip;
    if (count <= 0)
        return NULL;

    size_t stacktrace_size = sizeof(struct stacktrace) + sizeof(struct stacktrace_frame) * count;
    struct stacktrace* stacktrace = (struct stacktrace*)sys_malloc(stacktrace_size);
    if (!stacktrace)
        return NULL;

    memset(stacktrace, 0, stacktrace_size);

    stacktrace->count = count;
    for (int i = 0; i < count; ++i) {
        stacktrace->frames[i].address = buffer[i + skip];
    }
    return stacktrace;
}

void stacktrace_analysis(struct stacktrace* stacktrace, fn_cache_fname* cache_fname) 
{
    Dl_info dli;
    for (int i = 0; i < stacktrace->count; ++i) {
        struct stacktrace_frame* frame = stacktrace->frames + i;
        if (dladdr(frame->address, &dli) == 0)
            continue;

        frame->fbase = dli.dli_fbase;
        frame->fname = cache_fname ? cache_fname(dli.dli_fbase, dli.dli_fname) : dli.dli_fname;
    }
}

void stacktrace_destroy(struct stacktrace* stacktrace)
{
    sys_free(stacktrace);
}
