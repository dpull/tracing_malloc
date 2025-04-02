#include "stacktrace_libunwind.h"
#ifdef USE_LIBUNWIND

#define UNW_LOCAL_ONLY
#include <libunwind.h>

void stacktrace_libunwind::collect() { size = unw_backtrace(buffer, sizeof(buffer) / sizeof(buffer[0])); }

void stacktrace_libunwind::output(FILE* stream)
{
    fprintf(stream, "stacktrace_libunwind:%d\n", size);
    for (auto i = 0; i < size; ++i) {
        fprintf(stream, "%p\n", buffer[i]);
    }
}

#endif // USE_LIBUNWIND
