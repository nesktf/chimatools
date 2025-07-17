#pragma once

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void* (*PFN_chima_malloc)(void* user, size_t size);
typedef void* (*PFN_chima_realloc)(void* user, void* ptr, size_t oldsz, size_t newsz);
typedef void (*PFN_chima_free)(void* user, void* ptr);

typedef struct chima_alloc {
  void* user_data;
  PFN_chima_malloc malloc;
  PFN_chima_realloc realloc;
  PFN_chima_free free;
} chima_alloc;


#ifdef __cplusplus
}
#endif
