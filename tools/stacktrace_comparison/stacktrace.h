#pragma once
#include <stdio.h>

#define STACK_TRACE_DEPTH (64)

struct stacktrace {
    virtual ~stacktrace() {}
    virtual void collect() {}
    virtual void analysis() {}
    virtual void output(FILE* stream) {}
};
