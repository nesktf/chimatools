#include "./internal.h"

#include <stdlib.h>
#include <string.h>

const char* chima_error_string(chima_result ret) {
  switch (ret) {
    case CHIMA_NO_ERROR: return "No error";
    case CHIMA_ALLOC_FAILURE: return "Allocation failure";
    case CHIMA_FILE_OPEN_FAILURE: return "Failed to open file";
    case CHIMA_FILE_WRITE_FAILURE: return "Failed to write file";
    case CHIMA_FILE_EOF: return "Reached end of file";
    case CHIMA_IMAGE_PARSE_FAILURE: return "Failed to parse image data";
    case CHIMA_INVALID_VALUE: return "Invalid parameter provided";
    case CHIMA_UNSUPPORTED_FORMAT : return "Unsupported format provided";
    case CHIMA_PACKING_FAILED: return "Rectangle packing failed";
    default: return "Unknown error";
  }
}

static void* chima_malloc(void* user, size_t size) {
  CHIMA_UNUSED(user);
  void* mem = malloc(size);
  // printf("- MALLOC: %p (%zu)\n", mem, size);
  return mem;
}

static void chima_free(void* user, void* ptr) {
  CHIMA_UNUSED(user);
  // printf("- FREE: %p\n", ptr);
  free(ptr);
}

static void* chima_realloc(void* user, void* ptr, size_t oldsz, size_t newsz) {
  CHIMA_UNUSED(user);
  CHIMA_UNUSED(oldsz);
  void* mem = realloc(ptr, newsz);
  // printf("- REALLOC: %p (%zu) -> %p (%zu)\n", ptr, oldsz, mem, newsz);
  return mem;
}

#define ATLAS_MAX_SIZE  16384
#define ATLAS_INIT_SIZE 512
#define ATLAS_GROW_FAC  2.0f

chima_result chima_create_context(chima_context* chima,
                                            const chima_alloc* alloc) {
  if (!chima) {
    return CHIMA_INVALID_VALUE;
  }

  chima_alloc alloc_funcs;
  if (alloc && alloc->malloc && alloc->realloc && alloc->free) {
    alloc_funcs = *alloc;
  } else {
    alloc_funcs.user = NULL;
    alloc_funcs.malloc = &chima_malloc;
    alloc_funcs.free = &chima_free;
    alloc_funcs.realloc = &chima_realloc;
  }
  CHIMA_ASSERT(alloc_funcs.malloc);
  CHIMA_ASSERT(alloc_funcs.free);
  CHIMA_ASSERT(alloc_funcs.realloc);

  chima_context ctx = alloc_funcs.malloc(alloc_funcs.user, sizeof(chima_context_));
  if (!ctx) {
    *chima = NULL;
    return CHIMA_ALLOC_FAILURE;
  }
  memset(ctx, 0, sizeof(*ctx));

  ctx->mem_user = alloc_funcs.user;
  ctx->mem_alloc = alloc_funcs.malloc;
  ctx->mem_realloc = alloc_funcs.realloc;
  ctx->atlas_grow_fac = ATLAS_GROW_FAC;
  ctx->atlas_initial = ATLAS_INIT_SIZE;

  (*chima) = ctx;
  return CHIMA_NO_ERROR;
}

void chima_destroy_context(chima_context chima) {
  if (!chima) {
    return;
  }
  void* user = chima->mem_user;
  PFN_chima_free mem_free = chima->mem_free;
  CHIMA_ASSERT(mem_free);
  memset(chima, 0, sizeof(*chima));
  mem_free(user, chima);
}
