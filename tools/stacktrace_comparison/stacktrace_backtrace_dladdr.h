#pragma once
#include "stacktrace.h"

class stacktrace_backtrace_dladdr : public stacktrace {
public:
    stacktrace_backtrace_dladdr();
    ~stacktrace_backtrace_dladdr() override;
    void collect() override;
    void analysis() override;
    void output(FILE* stream) override;

private:
    struct stacktrace_backtrace_dladdr_frame* stacktrace;
    int stacktrace_len;
};
