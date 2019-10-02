#pragma once
#include "stacktrace.h"

class stacktrace_backtrace : public stacktrace {
public:
    ~stacktrace_backtrace() override;
    void collect() override;
    void analysis() override;
    void output(FILE* stream) override;

private:
    void* buffer[STACK_TRACE_DEPTH];
    int size = 0;
    char** strings = nullptr;
};
