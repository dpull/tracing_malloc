#include "tracking_malloc_hashmap.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define HASHMAP_SENTINEL (-1)

struct hashmap {
    struct hashmap_value* data;
    size_t max_count;
    size_t length;
    pthread_mutex_t mutex;
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

    struct hashmap* hashmap = (struct hashmap*)sys_malloc(sizeof(*hashmap));
    hashmap->data = data;
    hashmap->max_count = value_max_count;
    hashmap->length = length;
    if (pthread_mutex_init(&hashmap->mutex, NULL) != 0) {
        munmap(hashmap->data, hashmap->length);
        sys_free(hashmap);
        return NULL;
    }
    return hashmap;
}

int hashmap_destory(struct hashmap* hashmap)
{
    pthread_mutex_destroy(&hashmap->mutex);
    munmap(hashmap->data, hashmap->length);
    sys_free(hashmap);
    return 0;
}

struct hashmap_value* hashmap_add(struct hashmap* hashmap, intptr_t pointer)
{
    size_t max_count = hashmap->max_count;
    int index = pointer % max_count;

    pthread_mutex_lock(&hashmap->mutex);

    for(size_t i = 0; i < hashmap->max_count; i++) {
        struct hashmap_value* data = hashmap->data + index;
        if (data->pointer == 0 || data->pointer == HASHMAP_SENTINEL) {
            data->pointer = pointer;

            pthread_mutex_unlock(&hashmap->mutex);
            return data;
        }
        index = (index + 1) % max_count;
    }

    pthread_mutex_unlock(&hashmap->mutex);
    return NULL;
}

struct hashmap_value* hashmap_get(struct hashmap* hashmap, intptr_t pointer)
{
    size_t max_count = hashmap->max_count;
    int index = pointer % max_count;
    for(size_t i = 0; i < hashmap->max_count; i++) {
        struct hashmap_value* data = hashmap->data + index;
        if (data->pointer == pointer) 
            return data;

        if (!data->pointer) 
            break;

        index = (index + 1) % max_count;
    }
    return NULL;
}

int hashmap_remove(struct hashmap* hashmap, intptr_t pointer) 
{
    pthread_mutex_lock(&hashmap->mutex);

    struct hashmap_value* data = hashmap_get(hashmap, pointer);
    if (data)
        data->pointer = HASHMAP_SENTINEL;

    pthread_mutex_unlock(&hashmap->mutex);
    return data ? 0 : -1;
}