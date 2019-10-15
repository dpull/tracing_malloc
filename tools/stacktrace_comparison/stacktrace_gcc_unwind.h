#pragma once
#include "stacktrace.h"

class stacktrace_gcc_unwind : public stacktrace {
public:
    void collect() override;
    void output(FILE* stream) override;

private:
    void* buffer[STACK_TRACE_DEPTH];
    int size = 0;
};
