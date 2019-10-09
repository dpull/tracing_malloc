#pragma once
#include "tracking_malloc.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct hashmap_value {
    int64_t pointer;
    int64_t alloc_time;
    int64_t alloc_size;
    int64_t reserve;    
    int64_t address[STACK_TRACE_DEPTH];
};

struct hashmap* hashmap_create(const char* file, size_t value_max_count);
int hashmap_destory(struct hashmap* hashmap);

struct hashmap_value* hashmap_add(struct hashmap* hashmap, intptr_t pointer);
struct hashmap_value* hashmap_get(struct hashmap* hashmap, intptr_t pointer);
int hashmap_remove(struct hashmap* hashmap, intptr_t pointer);

#ifdef __cplusplus
}
#endif
