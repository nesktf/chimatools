#ifndef CHIMATOOLS_CORE_H
#define CHIMATOOLS_CORE_H

#include "../include/chimatools/chimatools.h"
#include <assert.h>

#define CHIMA_UNUSED(x)     (void)x
#define CHIMA_UNREACHABLE() __builtin_unreachable()

#define CHIMA_ARRAY_SIZE(arr_) (sizeof(arr_) / sizeof(arr_[0]))

#define CHIMA_STATIC_ASSERT(cond_) _Static_assert(cond_, #cond_)
#define CHIMA_ASSERT(cond_)        assert(cond_)

#define CHIMA_SET_FLAG(cond_, flags_, flag_) \
  cond_ ? (flags_ | flag_) : (flags_ & ~flag_)

#define CHIMA_CLAMP(val_, min_, max_) \
  val_ > max_ ? max_ : (val_ < min_ ? min_ : val_)

static const char CHIMA_MAGIC[] = {0x89, 'C', 'H', 'I', 'M', 'A',
                                   0x89, 'A', 'S', 'S', 'E', 'T'};

// TODO: Add a scratch arena?
typedef struct chima_context_ {
  void* mem_user;
  PFN_chima_malloc mem_alloc;
  PFN_chima_realloc mem_realloc;
  PFN_chima_free mem_free;
  chima_bitfield flags;
  chima_f32 atlas_grow_fac;
  chima_u32 atlas_initial;
  chima_u32 image_count;
  chima_u32 anim_count;
} chima_context_;

static inline chima_alloc alloc_from_chima(chima_context chima) {
  return (chima_alloc){.user = chima->mem_user,
                       .malloc = chima->mem_alloc,
                       .realloc = chima->mem_realloc,
                       .free = chima->mem_free};
}

typedef enum chima_ctx_flags {
  CHIMA_CTX_FLAG_NONE = 0x0000,
  CHIMA_CTX_FLAG_FLIP_Y = 0x0001,

  _CHIMA_CTX_FLAG_FORCE_32BIT = 0x7FFFFFFF,
} chima_ctx_flags;

#define CHIMA_MALLOC(size_) chima->mem_alloc(chima->mem_user, size_)

#define CHIMA_REALLOC(ptr_, oldsz_, newsz_) \
  chima->mem_realloc(chima->mem_user, ptr_, oldsz_, newsz_)

#define CHIMA_FREE(ptr_) chima->mem_free(chima->mem_user, ptr_)

#define CHIMA_CALLOC(n_, size_) chima->mem_alloc(chima->mem_user, n_* size_)

#endif
