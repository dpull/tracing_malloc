#include "tracking_malloc.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define likely(x)    __builtin_expect(!!(x), 1)
#define unlikely(x)  __builtin_expect(!!(x), 0)

static char* internal_malloc_buffer = NULL;
static char* internal_malloc_buffer_end = NULL;
static char* internal_malloc_buffer_pos = NULL;

static void* (*g_sys_malloc)(size_t size) = NULL;
static void(*g_sys_free)(void *ptr) = NULL;
static void* (*g_sys_calloc)(size_t nmemb, size_t size) = NULL;
static void* (*g_sys_realloc)(void *ptr, size_t size) = NULL;

static void* internal_malloc(size_t size)
{
    if (!internal_malloc_buffer) {
        internal_malloc_buffer = (char*)sbrk(INTERNAL_MALLOC_BUFFER_SIZE);
        internal_malloc_buffer_end = internal_malloc_buffer + INTERNAL_MALLOC_BUFFER_SIZE;
        internal_malloc_buffer_pos = internal_malloc_buffer;
    }

    size = (size + 7) / 8 * 8;

    char* ptr = internal_malloc_buffer_pos;
    char* ptr_end = ptr + size;

    if (ptr_end < internal_malloc_buffer_end) {
        internal_malloc_buffer_pos = ptr_end;
        return ptr;
    }

    abort();
    return NULL;
}

static int is_internal_malloc(void* ptr)
{
    return (char*)ptr >= internal_malloc_buffer && (char*)ptr < internal_malloc_buffer_end;
}

__attribute__((constructor))
static void init(void)
{
    g_sys_malloc = dlsym(RTLD_NEXT, "malloc");
    g_sys_free = dlsym(RTLD_NEXT, "free");
    g_sys_calloc = dlsym(RTLD_NEXT, "calloc");
    g_sys_realloc = dlsym(RTLD_NEXT, "realloc");

    if (!g_sys_malloc || !g_sys_calloc || !g_sys_realloc || !g_sys_free)
        abort();

    if (record_init())
        abort();
}

__attribute__((destructor))
static void uninit(void)
{
    record_uninit();
}

void* sys_malloc(size_t size)
{
    return g_sys_malloc(size);
}

void sys_free(void* ptr)
{
    g_sys_free(ptr);
}

void* malloc(size_t size)
{
    if (unlikely(!g_sys_malloc))
        return internal_malloc(size);

    void* ptr = g_sys_malloc(size);
    record_alloc(ptr, size);
    return ptr;
}

void free(void* ptr)
{
    if (unlikely(is_internal_malloc(ptr)))
        return;
    g_sys_free(ptr);
    record_free(ptr);
}

void* calloc(size_t nmemb, size_t size)
{
    if (unlikely(!g_sys_calloc)) {
        size *= nmemb;
        void* ptr = internal_malloc(size);
        memset(ptr, 0, size);
        return ptr;
    }

    void* ptr = g_sys_calloc(nmemb, size);
    record_alloc(ptr, nmemb * size);
    return ptr;
}

void *realloc(void* ptr, size_t size)
{
    if (unlikely(is_internal_malloc(ptr))) {
        void* new_ptr = internal_malloc(size);
        memcpy(new_ptr, ptr, size);
        return new_ptr;
    }

    void* new_ptr = g_sys_realloc(ptr, size);
    if (new_ptr) {
        record_free(ptr);
        record_alloc(new_ptr, size);
    }
    return new_ptr;
}
