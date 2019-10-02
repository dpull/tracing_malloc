#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INTERNAL_MALLOC_BUFFER_SIZE  (1024*1024)
#define STACK_TRACE_DEPTH            (32)
#define STACK_TRACE_SKIP             (3)
#define STACK_RECORD_INTERVAL_SEC    (120)  

void* sys_malloc(size_t size);
void sys_free(void* ptr);

int record_init();
int record_uninit();
int record_alloc(void* ptr, size_t size);
int record_free(void* ptr);

#ifdef __cplusplus
}
#endif
