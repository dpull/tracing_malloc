#include "tracking_malloc.h"
#include "tracking_malloc_record.h"
#include "tracking_malloc_hashmap.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <execinfo.h>
#include <unwind.h>

#ifdef USE_LIBUNWIND

#define UNW_LOCAL_ONLY
#include <libunwind.h>
__attribute__((always_inline))
static inline int stacktrace_fast(void** buffer, int size, int skip) 
{
    return unw_backtrace(buffer, size);
}

#endif // USE_LIBUNWIND

struct libgcc_backtrace_data {
    int skip;
    void** pos;
    void** end;
};

static _Unwind_Reason_Code libgcc_backtrace_callback(struct _Unwind_Context* ctx, void* _data) 
{
    struct libgcc_backtrace_data* data = (struct libgcc_backtrace_data*)_data;
    if (data->skip > 0) {
        data->skip--;
        return _URC_NO_REASON;
    }

    void* ip = (void*)_Unwind_GetIP(ctx);
    *(data->pos++) = ip;
    return (ip && data->pos < data->end) ? _URC_NO_REASON : _URC_END_OF_STACK;
}

__attribute__((always_inline))
static inline int stacktrace_slow(void** buffer, int size, int skip) 
{
    struct libgcc_backtrace_data data;
    data.skip = skip;
    data.pos = buffer + skip;
    data.end = buffer + size;
    _Unwind_Backtrace(libgcc_backtrace_callback, &data);
    return data.pos - buffer;
}

int stack_backtrace(void** buffer, int size, int skip) 
{
#ifdef USE_LIBUNWIND
    return stacktrace_fast(buffer, size, skip);
#else
    return stacktrace_slow(buffer, size, skip);
#endif // USE_LIBUNWIND
}