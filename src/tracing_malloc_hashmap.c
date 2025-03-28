#include "tracing_malloc_hashmap.h"
#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define HASHMAP_NULL (-1)
#define HASHMAP_SENTINEL (-2)

struct hashmap {
    struct hashmap_value* hashmap_value;
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

static void check_system_supports(struct hashmap_value* pointer)
{
    _Static_assert(sizeof(int64_t) == sizeof(void*), "sizeof(int64_t) == sizeof(void*)");

    int pagesize = getpagesize();
    if ((pagesize % sizeof(pointer[0])) != 0)
        abort();
    if (((intptr_t)pointer % pagesize) != 0)
        abort();
}

struct hashmap* hashmap_create(const char* file, size_t value_max_count)
{
    size_t length = value_max_count * sizeof(struct hashmap_value);
    if (length == 0)
        return NULL;

    struct hashmap_value* hashmap_value = (struct hashmap_value*)filemapping_create_readwrite(file, length);
    if (!hashmap_value)
        return NULL;

    check_system_supports(hashmap_value);
    for (size_t i = 0; i < value_max_count; i++) {
        hashmap_value[i].pointer = HASHMAP_NULL;
    }

    struct hashmap* hashmap = (struct hashmap*)sys_malloc(sizeof(*hashmap));
    hashmap->hashmap_value = hashmap_value;
    hashmap->max_count = value_max_count;
    hashmap->length = length;
    if (pthread_mutex_init(&hashmap->mutex, NULL) != 0) {
        munmap(hashmap->hashmap_value, hashmap->length);
        sys_free(hashmap);
        return NULL;
    }
    return hashmap;
}

int hashmap_destory(struct hashmap* hashmap)
{
    pthread_mutex_destroy(&hashmap->mutex);
    munmap(hashmap->hashmap_value, hashmap->length);
    sys_free(hashmap);
    return 0;
}

struct hashmap_value* hashmap_add(struct hashmap* hashmap, intptr_t pointer)
{
    size_t max_count = hashmap->max_count;
    int index = pointer % max_count;

    pthread_mutex_lock(&hashmap->mutex);

    for (size_t i = 0; i < hashmap->max_count; i++) {
        struct hashmap_value* hashmap_value = hashmap->hashmap_value + index;
        if (hashmap_value->pointer == HASHMAP_NULL || hashmap_value->pointer == HASHMAP_SENTINEL) {
            hashmap_value->pointer = pointer;

            pthread_mutex_unlock(&hashmap->mutex);
            return hashmap_value;
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
    for (size_t i = 0; i < hashmap->max_count; i++) {
        struct hashmap_value* hashmap_value = hashmap->hashmap_value + index;
        if (hashmap_value->pointer == pointer)
            return hashmap_value;

        if (hashmap_value->pointer == HASHMAP_NULL)
            break;

        index = (index + 1) % max_count;
    }
    return NULL;
}

int hashmap_remove(struct hashmap* hashmap, intptr_t pointer)
{
    pthread_mutex_lock(&hashmap->mutex);

    struct hashmap_value* hashmap_value = hashmap_get(hashmap, pointer);
    if (hashmap_value) {
        hashmap_value->pointer = HASHMAP_SENTINEL;
    }

    pthread_mutex_unlock(&hashmap->mutex);
    return hashmap_value ? 0 : -1;
}

void hashmap_traverse(struct hashmap* hashmap, hashmap_callback* callback)
{
    for (size_t i = 0; i < hashmap->max_count; i++) {
        struct hashmap_value* hashmap_value = hashmap->hashmap_value + i;
        if (hashmap_value->pointer == HASHMAP_NULL || hashmap_value->pointer == HASHMAP_SENTINEL)
            continue;
        if (callback(hashmap_value))
            break;
    }
}
