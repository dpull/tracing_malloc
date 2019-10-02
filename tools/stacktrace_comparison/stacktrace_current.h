#pragma once
#include "stacktrace.h"

class stacktrace_current : public stacktrace {
public:
    stacktrace_current();
    ~stacktrace_current() override;
    void collect() override;
    void analysis() override;
    void output(FILE* stream) override;

private:
    struct stacktrace_current_frame* stacktrace;
    int stacktrace_len;
};
