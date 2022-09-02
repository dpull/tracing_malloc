#include "tracing_malloc.h"
#include "tracing_malloc_record.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>

static char *internal_malloc_buffer = NULL;
static char *internal_malloc_buffer_end = NULL;
static char *internal_malloc_buffer_pos = NULL;

static void *(*g_sys_malloc)(size_t size) = NULL;
static void (*g_sys_free)(void *ptr) = NULL;
static void *(*g_sys_calloc)(size_t nmemb, size_t size) = NULL;
static void *(*g_sys_realloc)(void *ptr, size_t size) = NULL;
static int (*g_sys_posix_memalign)(void **memptr, size_t alignment,
				   size_t size) = NULL;
static void *(*g_sys_aligned_alloc)(size_t alignment, size_t size) = NULL;

/* Round "value" up to next "alignment" boundary. Requires that "alignment" be a power of two. */
static inline intptr_t round_up(intptr_t value, intptr_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

static void *internal_malloc(size_t alignment, size_t size)
{
	if (!internal_malloc_buffer) {
		internal_malloc_buffer =
			(char *)sbrk(INTERNAL_MALLOC_BUFFER_SIZE);
		internal_malloc_buffer_end =
			internal_malloc_buffer + INTERNAL_MALLOC_BUFFER_SIZE;
		internal_malloc_buffer_pos = internal_malloc_buffer;
	}

	assert((alignment & (alignment - 1)) == 0);
	size_t extra_bytes = alignment - 1;
	if (size + extra_bytes < size)
		return NULL; /* Overflow */
	size += extra_bytes;

	char *ptr = internal_malloc_buffer_pos;
	char *ptr_end = ptr + size;
	if (ptr_end < internal_malloc_buffer_end) {
		internal_malloc_buffer_pos = ptr_end;
		return (void *)round_up((intptr_t)ptr, alignment);
	}

	abort();
	return NULL;
}

static int is_internal_malloc(void *ptr)
{
	return (char *)ptr >= internal_malloc_buffer &&
	       (char *)ptr < internal_malloc_buffer_end;
}

static inline void *get_address(const char *symbol)
{
	void *address = dlsym(RTLD_NEXT, symbol);
	if (!address)
		abort();
	return address;
}

static void atfork_child(void)
{
	/* todo copy files.*/
	record_init();
}

__attribute__((constructor)) static void init(void)
{
	g_sys_malloc = get_address("malloc");
	g_sys_free = get_address("free");
	g_sys_calloc = get_address("calloc");
	g_sys_realloc = get_address("realloc");
	g_sys_posix_memalign = get_address("posix_memalign");
	g_sys_aligned_alloc = get_address("aligned_alloc");

	pthread_atfork(NULL, NULL, atfork_child);

	if (record_init())
		abort();
}

__attribute__((destructor)) static void uninit(void)
{
	record_uninit();
}

void *sys_malloc(size_t size)
{
	return g_sys_malloc(size);
}

void sys_free(void *ptr)
{
	g_sys_free(ptr);
}

__attribute__((visibility("default"))) void *malloc(size_t size)
{
	if (unlikely(!g_sys_malloc))
		return internal_malloc(1, size);

	void *ptr = g_sys_malloc(size);
	record_alloc(ptr, size);
	return ptr;
}

__attribute__((visibility("default"))) void free(void *ptr)
{
    if (unlikely(!ptr))
		return;
	if (unlikely(is_internal_malloc(ptr)))
		return;
	g_sys_free(ptr);
	record_free(ptr);
}

__attribute__((visibility("default"))) void *calloc(size_t nmemb, size_t size)
{
	if (unlikely(!g_sys_calloc)) {
		size *= nmemb;
		void *ptr = internal_malloc(1, size);
		memset(ptr, 0, size);
		return ptr;
	}

	void *ptr = g_sys_calloc(nmemb, size);
	record_alloc(ptr, nmemb * size);
	return ptr;
}

__attribute__((visibility("default"))) void *realloc(void *ptr, size_t size)
{
	if (unlikely(is_internal_malloc(ptr))) {
		if (size == 0)
			return NULL;

		void *new_ptr = internal_malloc(1, size);
		memcpy(new_ptr, ptr, size);
		return new_ptr;
	}

	void *new_ptr = g_sys_realloc(ptr, size);
	if (unlikely(size == 0)) {
		record_free(ptr);
		assert(new_ptr == NULL);
		return new_ptr;
	}

	if (unlikely(new_ptr == NULL))
		return new_ptr;

	if (likely(new_ptr != ptr)) {
		record_free(ptr);
		record_alloc(new_ptr, size);
	} else {
		record_update(ptr, size);
	}
	return new_ptr;
}

__attribute__((visibility("default"))) int
posix_memalign(void **memptr, size_t alignment, size_t size)
{
	if (unlikely(!g_sys_posix_memalign)) {
		void *ptr = NULL;
		if (size != 0)
			ptr = internal_malloc(alignment, size);
		*memptr = ptr;
		return 0;
	}

	int ret = g_sys_posix_memalign(memptr, alignment, size);
	if (likely(ret == 0))
		record_alloc(*memptr, size);
	return ret;
}

__attribute__((visibility("default"))) void *aligned_alloc(size_t alignment,
							   size_t size)
{
	if (unlikely(!g_sys_aligned_alloc))
		return internal_malloc(alignment, size);

	void *ptr = g_sys_aligned_alloc(alignment, size);
	record_alloc(ptr, size);
	return ptr;
}
