#pragma once
#include "tracking_malloc_stacktrace.h"
#include "tracking_malloc.hpp"
#include <dlfcn.h>

class stacktrace_default : public stacktrace {
    struct stacktrace_frame{
        char* fname;
        void* fbase;
        char* sname;
        void* saddr;
    };

public:
    ~stacktrace_default() override;
    void collect() override;
    void analysis() override;
    void output(FILE* stream, int start_level) override;

private:
    void frame_init(stacktrace_frame* frame, Dl_info* dli) ;
    void frame_uninit(stacktrace_frame* frame);

private:
    int size = 0;
    void* buffer[STACK_TRACE_DEPTH];
    stacktrace_frame* frames = nullptr;
};
