#pragma once
#include "stacktrace.h"

class stacktrace_libunwind : public stacktrace {
public:
    stacktrace_libunwind();
    ~stacktrace_libunwind() override;
    void collect() override;
    void analysis() override;
    void output(FILE* stream) override;

private:
    struct stacktrace_libunwind_frame* stacktrace;
    int stacktrace_len;
};