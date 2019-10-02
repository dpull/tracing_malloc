#include "tracking_malloc_stacktrace_default.h"
#include <execinfo.h>
#include <cxxabi.h>
#include <string.h>

#ifdef USE_STACKTRACE_COMPARISON
#include "tracking_malloc_stacktrace_comparison.h"
#endif

stacktrace_default::~stacktrace_default() 
{
    if (!frames)
        return;

    for (auto i = 0; i < size; ++i) {
        auto frame = frames + i;
        frame_uninit(frame);
    }
    sys_free(frames);
}

void stacktrace_default::collect() 
{
    size = backtrace(buffer, sizeof(buffer) / sizeof(buffer[0]));
}

void stacktrace_default::analysis()
{
    if (frames || size == 0)
        return;

    frames = (stacktrace_frame*)sys_malloc(sizeof(stacktrace_frame) * size);
    memset(frames, 0, sizeof(stacktrace_frame) * size);

    Dl_info dli;
    for (auto i = 0; i < size; ++i) {
        if (dladdr(buffer[i], &dli) == 0)
            continue;

        auto frame = frames + i;
        frame_init(frame, &dli);
    }
}

void stacktrace_default::output(FILE* stream, int start_level)
{
    fprintf(stream, "stacktrace_backtrace:%d\n", size - start_level);
    for (auto i = start_level; i < size; ++i) {
        auto frame = frames + i;
        fprintf(stream, "%p\t%s\t%p\t%s\t%p\n", buffer[i], frame->fname, frame->fbase, frame->sname, frame->saddr);
    }
}

void stacktrace_default::frame_init(stacktrace_frame* frame, Dl_info* dli) 
{
    if (dli->dli_fname)
        frame->fname = strdup(dli->dli_fname);
    frame->fbase = dli->dli_fbase;
    frame->saddr = dli->dli_saddr;

    if (!dli->dli_sname)
        return;

    int status;
    char* demangled = abi::__cxa_demangle(dli->dli_sname, nullptr, nullptr, &status);
    if (status == 0) {
        frame->sname = demangled;
        return;
    }
    if (demangled)
        free(demangled);
    frame->sname = strdup(dli->dli_sname);
}

void stacktrace_default::frame_uninit(stacktrace_frame* frame) 
{
    if (frame->fname) 
        free(frame->fname);
    if (frame->sname) 
        free(frame->sname);
}

stacktrace* stacktrace_create()
{
#ifdef USE_STACKTRACE_COMPARISON
    return sys_new<stacktrace_comparison>();
#else
    return sys_new<stacktrace_default>();
#endif
}

void stacktrace_destroy(stacktrace* st)
{
    sys_delete(st);
}
