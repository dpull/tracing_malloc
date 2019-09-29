#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifdef USE_STACKTRACE_COMPARISON
    #define ENABLE_BACKTRACE
    #define ENABLE_BOOST_STACKTRACE
    #define ENABLE_LIBUNWIND
#else
    #define ENABLE_BACKTRACE
#endif

#define INTERNAL_MALLOC_BUFFER_SIZE  (1024*1024)
#define STACK_TRACE_DEPTH            (32)


void* sys_malloc(size_t size);
void sys_free(void* ptr);

int record_init();
int record_uninit();
int record_alloc(void* ptr, size_t size);
int record_free(void* ptr);

#ifdef __cplusplus
}
#endif
