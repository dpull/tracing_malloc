#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INTERNAL_MALLOC_BUFFER_SIZE ((1 << 20) - 1)
#define STACK_TRACE_DEPTH (28)
#define STACK_TRACE_SKIP (2)
#define RECORD_MAX_COUNT (8 * 1024 * 1024)

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

void *sys_malloc(size_t size);
void sys_free(void *ptr);

#ifdef __cplusplus
}
#endif
