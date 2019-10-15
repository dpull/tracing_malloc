
#include "stacktrace_libunwind.h"
#include <cxxabi.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_LIBUNWIND

#define UNW_LOCAL_ONLY
#include <libunwind.h>

struct stacktrace_libunwind_frame {
    unw_word_t reg_ip;
};

stacktrace_libunwind::stacktrace_libunwind()
{
    stacktrace = new stacktrace_libunwind_frame[STACK_TRACE_DEPTH];
    stacktrace_len = 0;
}

stacktrace_libunwind::~stacktrace_libunwind()
{
    if (stacktrace)
        delete[] stacktrace;
}

void stacktrace_libunwind::collect()
{
    unw_cursor_t cursor;
    unw_context_t uc;
    unw_getcontext(&uc);
    unw_init_local(&cursor, &uc);

    for (auto i = 0; i < STACK_TRACE_DEPTH; ++i) {
        if (unw_step(&cursor) <= 0)
            break;

        auto* frame = stacktrace + i;
        unw_get_reg(&cursor, UNW_REG_IP, &frame->reg_ip);
        stacktrace_len++;
    }
}

void stacktrace_libunwind::output(FILE* stream)
{
    fprintf(stream, "stacktrace_libunwind:%d\n", stacktrace_len);
    for (auto i = 0; i < stacktrace_len; ++i) {
        auto frame = stacktrace + i;
        fprintf(stream, "%d# 0x%lx\n", i, frame->reg_ip);
    }
}

#endif // USE_LIBUNWIND
