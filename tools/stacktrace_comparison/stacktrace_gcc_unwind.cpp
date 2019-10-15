#include "stacktrace_gcc_unwind.h"
#include <execinfo.h>
#include <stdlib.h>
#include <string.h>
#include <unwind.h>

struct libgcc_backtrace_data {
    void* array[STACK_TRACE_DEPTH];
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

void stacktrace_gcc_unwind::collect() 
{
    struct libgcc_backtrace_data data;
    memset(&data.array, 0, sizeof(data.array));
    data.skip = 0;
    data.pos = data.array;
    data.end = data.array + sizeof(data.array) / sizeof(data.array[0]);

    _Unwind_Backtrace(libgcc_backtrace_callback, &data);
}

void stacktrace_gcc_unwind::output(FILE* stream)
{
    fprintf(stream, "stacktrace_gcc_unwind:%d\n", size);
    for (auto i = 0; i < size; ++i) {
        fprintf(stream, "%p\n", buffer[i]);
    }
}