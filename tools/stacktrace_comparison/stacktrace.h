#pragma once
#include <stdio.h>

#define STACK_TRACE_DEPTH (64)

struct stacktrace {
    virtual ~stacktrace() {}
    virtual void collect() = 0;
    virtual void analysis() = 0;
    virtual void output(FILE* stream) = 0;
};
