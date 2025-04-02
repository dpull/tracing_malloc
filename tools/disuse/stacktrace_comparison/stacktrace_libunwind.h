#pragma once
#include "stacktrace.h"

class stacktrace_libunwind : public stacktrace {
#ifdef USE_LIBUNWIND
public:
    void collect() override;
    void output(FILE* stream) override;

private:
    void* buffer[STACK_TRACE_DEPTH];
    int size = 0;
#endif // USE_LIBUNWIND
};