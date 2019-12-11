#include "tracing_malloc.h"
#include "tracing_malloc_record.h"
#include "tracing_malloc_hashmap.h"
#include "tracing_malloc_stacktrace.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>

#define PARAMETER_ERROR     (-1)
#define ALLOC_FAILED        (-2)
#define BACKTRACE_FAILED    (-3)

struct record_data {
    struct hashmap* hashmap;
    pid_t pid;
};

static struct record_data g_record = {NULL, 0};
__thread int record_disable_flag = 0;

__attribute__((always_inline)) 
static inline int _record_init() 
{
    pid_t pid =  getpid();
    char file_name[FILENAME_MAX];

    if (g_record.hashmap) {
        assert(g_record.pid != 0 && g_record.pid != pid);
        hashmap_destory(g_record.hashmap);
        g_record.hashmap = NULL;
        g_record.pid = 0;
    }

    sprintf(file_name, "/tmp/%s.%d", "tracing.malloc", pid);
    struct hashmap* hashmap = hashmap_create(file_name, 1024 * 1024);

    if (!hashmap)
        return ALLOC_FAILED;

    g_record.hashmap = hashmap;
    g_record.pid = pid;
    return 0;
}

int record_init()
{
    record_disable_flag = 1;
    int ret = _record_init();
    record_disable_flag = 0;
    return ret;
}

static void _backup_proc_maps()
{
    pid_t pid = getpid();
    FILE* src = NULL;
    FILE* dst = NULL;
    char file_name_buffer[FILENAME_MAX];

    src = fopen("/proc/self/maps", "rb");
    sprintf(file_name_buffer, "/tmp/%s.%d.maps", "tracing.malloc", pid);
    dst = fopen(file_name_buffer, "wb");

    if (!src || !dst) 
        goto cleanup;

    while (1) {
        int bytes_read_len = fread(file_name_buffer, 1, sizeof(file_name_buffer), src);
        if (bytes_read_len <= 0)
            break;

        int bytes_write_len = fwrite(file_name_buffer, bytes_read_len, 1, dst);
        if (bytes_write_len != 1)
            break;
    }
    
cleanup:
    if (src)
        fclose(src);
    if (dst)
        fclose(dst);
}

static int _record_debug(struct hashmap_value* hashmap_value) 
{
    printf("ptr:%p, size:%lld\n", (void*)hashmap_value->pointer, hashmap_value->alloc_size);
    return 0;
}

__attribute__((always_inline)) 
static inline void _record_uninit() 
{
    /* 
    hashmap_traverse(g_record.hashmap, _record_debug); 
    */
    _backup_proc_maps();
    hashmap_destory(g_record.hashmap);
    g_record.hashmap = NULL;
    g_record.pid = 0;
}

void record_uninit()
{
    record_disable_flag = 1;
    _record_uninit();
    record_disable_flag = 0;
}

__attribute__((always_inline)) 
static inline int _record_alloc(int add_flag, void* ptr, size_t size) 
{
    struct hashmap_value* hashmap_value;
    void* buffer[sizeof(hashmap_value->address) / sizeof(hashmap_value->address[0]) + STACK_TRACE_SKIP];
    memset(buffer, 0, sizeof(buffer));
    int count = stack_backtrace(buffer, sizeof(buffer) / sizeof(buffer[0]), STACK_TRACE_SKIP); 
    if (unlikely(count < STACK_TRACE_SKIP))
        return BACKTRACE_FAILED;

    if (likely(add_flag))
        hashmap_value = hashmap_add(g_record.hashmap, (intptr_t)ptr);
    else
        hashmap_value = hashmap_get(g_record.hashmap, (intptr_t)ptr);

    if (unlikely(!hashmap_value))
        return ALLOC_FAILED;

    hashmap_value->alloc_time = (int64_t)time(NULL);
    hashmap_value->alloc_size = size;
    memcpy(hashmap_value->address, buffer + STACK_TRACE_SKIP, sizeof(hashmap_value->address));
    return 0;
}

int record_alloc(void* ptr, size_t size)
{
    if (unlikely(!ptr || record_disable_flag || !g_record.hashmap))
        return PARAMETER_ERROR;

    record_disable_flag = 1;
    int ret = _record_alloc(1, ptr, size);
    record_disable_flag = 0;
    return ret;
}

int record_free(void* ptr)
{
    if (unlikely(!ptr || record_disable_flag || !g_record.hashmap))
        return PARAMETER_ERROR;

    record_disable_flag = 1;
    int ret = hashmap_remove(g_record.hashmap, (intptr_t)ptr);
    record_disable_flag = 0;
    return ret;
}

int record_update(void* ptr, size_t new_size)
{
    if (unlikely(record_disable_flag || !g_record.hashmap))
        return PARAMETER_ERROR;

    record_disable_flag = 1;
    int ret = _record_alloc(0, ptr, new_size);
    record_disable_flag = 0;
    return ret;
}
