#include "stacktrace_backtrace.h"
#include <execinfo.h>
#include <stdlib.h>

stacktrace_backtrace::~stacktrace_backtrace()
{
    if (strings)
        free(strings);
}

void stacktrace_backtrace::collect() { size = backtrace(buffer, sizeof(buffer) / sizeof(buffer[0])); }

void stacktrace_backtrace::analysis()
{
    if (!strings)
        strings = backtrace_symbols(buffer, size);
}

void stacktrace_backtrace::output(FILE* stream)
{
    fprintf(stream, "stacktrace_backtrace:%d\n", size);
    for (auto i = 0; i < size; ++i) {
        fprintf(stream, "%s\n", strings[i]);
    }
}