#pragma once
#include <stdio.h>

struct stacktrace {
    virtual ~stacktrace() {};
    virtual void collect() = 0;
    virtual void analysis() = 0;
    virtual void output(FILE* stream, int start_level) = 0;
};

stacktrace* stacktrace_create();
void stacktrace_destroy(stacktrace* st);