#include "tracking_malloc_hashmap.h"
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <threads.h>

// https://en.cppreference.com/w/c/thread

#define SENTINEL (-1)

struct hashmap {
    struct hashmap_value* data;
    size_t max_count;
    size_t length;
    mtx_t mutex;
};

static void* filemapping_create_readwrite(const char* file, size_t length)
{
	int fd = open(file, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if (fd == -1)
		return NULL;

    if (ftruncate(fd, length) != 0) {
        close(fd);
        return NULL;
    }

	void* data = mmap(0, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
    close(fd);
	return data != MAP_FAILED ? data : NULL;
}

struct hashmap* hashmap_create(const char* file, size_t value_max_count) 
{
    size_t length = value_max_count * sizeof(struct hashmap_value);
    if (length == 0)
        return NULL;
    
    struct hashmap_value* data = (struct hashmap_value*)filemapping_create_readwrite(file, length);
    if (!data)
        return NULL;

    memset(data, 0, length);

    struct hashmap* hashmap = (struct hashmap*)malloc(sizeof(*hashmap));
    hashmap->data = data;
    hashmap->max_count = value_max_count;
    hashmap->length = length;

    if (mtx_init(&hashmap->mutex, mtx_plain) != thrd_success) {
        munmap(hashmap->data, hashmap->length);
        free(hashmap);
        return NULL;
    }
    return hashmap;
}

int hashmap_destory(struct hashmap* hashmap)
{
    mtx_destroy(&hashmap->mutex);
    munmap(hashmap->data, hashmap->length);
    free(hashmap);
}

struct hashmap_value* hashmap_add(struct hashmap* hashmap, intptr_t pointer)
{
    size_t max_count = hashmap->max_count;
    int index = pointer % max_count;

    mtx_lock(&hashmap->mutex)
    for(size_t i = 0; i < hashmap->max_count; i++) {
        struct hashmap_value* data = hashmap->data + index;
        if (data->pointer == NULL || data->pointer == SENTINEL) {
            data->pointer = pointer;

            mtx_unlock(&hashmap->mutex)
            return data;
        }
        index = (index + 1) % max_count;
    }

    mtx_unlock(&hashmap->mutex)
    return NULL;
}

struct hashmap_value* hashmap_get(struct hashmap* hashmap, intptr_t pointer)
{
    size_t max_count = hashmap->max_count;
    int index = pointer % max_count;
    for(size_t i = 0; i < hashmap->max_count; i++) {
        struct hashmap_value* data = hashmap->data + index;
        if (data->pointer == pointer) {
            return data;
        }
        if (!data->pointer) 
            break;
        index = (index + 1) % max_count;
    }
    return NULL;
}

int hashmap_remove(struct hashmap* hashmap, intptr_t pointer) 
{
    mtx_lock(&hashmap->mutex)
    struct hashmap_value* data = hashmap_get(hashmap, pointer);
    if (data)
        data->pointer = SENTINEL;
    mtx_unlock(&hashmap->mutex)
    return data ? 0 : -1;
}