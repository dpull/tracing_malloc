#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int stack_backtrace(void **buffer, int size, int skip);

#ifdef __cplusplus
}
#endif
