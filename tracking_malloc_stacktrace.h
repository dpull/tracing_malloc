#pragma once
#include <stdio.h>

#define USE_BACKTRACE
#define USE_BOOST_STACKTRACE
#define USE_LIBUNWIND

struct stacktrace {
    virtual ~stacktrace();
    virtual void collect();
    virtual void analysis();
    virtual void output(FILE* stream, int start_level);
};

stacktrace* stacktrace_create();
void stacktrace_destroy(stacktrace* st);