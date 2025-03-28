#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int record_init();
void record_uninit();
int record_alloc(void* ptr, size_t size);
int record_free(void* ptr);
int record_update(void* ptr, size_t new_size);

#ifdef __cplusplus
}
#endif
