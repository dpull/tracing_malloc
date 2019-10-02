#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include "stacktrace_current.h"
#include <execinfo.h>
#include <stdlib.h>
#include <cxxabi.h>

struct stacktrace_current_frame {
    void* address = nullptr;
    const char* fname = nullptr;
    void* fbase = nullptr;
    const char* sname = nullptr;
    void* saddr = nullptr;
    const char* demangled = nullptr;

    ~stacktrace_current_frame() {
        if (demangled)
            free((void*)demangled);
    }
};

stacktrace_current::stacktrace_current()
{
    stacktrace = new stacktrace_current_frame[STACK_TRACE_DEPTH];
    stacktrace_len = 0;
}

stacktrace_current::~stacktrace_current()
{
    if (stacktrace)
        delete[] stacktrace;
}

void stacktrace_current::collect() 
{
    void* buffer[STACK_TRACE_DEPTH];
    stacktrace_len = backtrace(buffer, sizeof(buffer) / sizeof(buffer[0]));

    for (auto i = 0; i < stacktrace_len; ++i) {
        stacktrace[i].address = buffer[i];
    }
}

void stacktrace_current::analysis() 
{
    int status;
    Dl_info dli;
    for (auto i = 0; i < stacktrace_len; ++i) {
        auto* frame = stacktrace + i;

        if (dladdr(frame->address, &dli) == 0)
            continue;

        frame->fname = dli.dli_fname;
        frame->fbase = dli.dli_fbase;
        frame->sname = dli.dli_sname;
        frame->saddr = dli.dli_saddr;

        if (dli.dli_sname) 
            frame->demangled = abi::__cxa_demangle(dli.dli_sname, nullptr, nullptr, &status);
    }
}

void stacktrace_current::output(FILE* stream)
{
    fprintf(stream, "stacktrace_current:%d\n", stacktrace_len);
    for (auto i = 0; i < stacktrace_len; ++i) {
        auto frame = stacktrace + i;
        if (frame->sname)
            fprintf(stream, "%d# %s in %s\n", i, frame->demangled ? frame->demangled : frame->sname, frame->fname);
        else
            fprintf(stream, "%d# %p in %s\n", i, frame->address, frame->fname);
    }
}
