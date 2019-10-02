
#include "stacktrace_libunwind.h"
#include <cxxabi.h>
#include <stdlib.h>
#include <string.h>
#define UNW_LOCAL_ONLY
#include <libunwind.h>

struct stacktrace_libunwind_frame {
    unw_word_t reg_ip;
    unw_word_t offset;
    char proc_name[1024];
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

        int status = unw_get_proc_name(&cursor, frame->proc_name, sizeof(frame->proc_name), &frame->offset);
        if (status == 0) {
            char* demangled = abi::__cxa_demangle(frame->proc_name, nullptr, nullptr, &status);
            if (status == 0)
                strncpy(frame->proc_name, demangled, sizeof(frame->proc_name));
            if (demangled)
                free(demangled);
        }
        stacktrace_len++;
    }
}

void stacktrace_libunwind::analysis()
{

}

void stacktrace_libunwind::output(FILE* stream)
{
    fprintf(stream, "stacktrace_libunwind:%d\n", stacktrace_len);
    for (auto i = 0; i < stacktrace_len; ++i) {
        auto frame = stacktrace + i;
        if (frame->proc_name[0])
            fprintf(stream, "%d# %s\n", i, frame->proc_name);
        else
            fprintf(stream, "%d# 0x%lx\n", i, frame->reg_ip);
    }
}
