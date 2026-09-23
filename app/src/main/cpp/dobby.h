#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int DobbyHook(void *address, void *fake_func, void **out_origin_func);

#ifdef __cplusplus
}
#endif
