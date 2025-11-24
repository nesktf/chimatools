#pragma once

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
#ifndef CHIMA_ASSERT
#include <cassert>
#define CHIMA_ASSERT assert
#endif
#else
#include <stddef.h>
#include <stdint.h>
#ifndef CHIMA_ASSERT
#include <assert.h>
#define CHIMA_ASSERT assert
#endif
#endif

#define CHIMADEF              extern
#define CHIMA_STRING_MAX_SIZE 256
#define CHIMA_TRUE            1
#define CHIMA_FALSE           0
#define CHIMA_ARR_SZ(arr)     sizeof(arr) / sizeof(arr[0])

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

struct chima_context_;
typedef struct chima_context_* chima_context;

typedef uint32_t chima_bool;

typedef struct chima_string {
  uint32_t length;
  char data[CHIMA_STRING_MAX_SIZE];
} chima_string;

typedef struct chima_color {
  float r, g, b, a;
} chima_color;

typedef enum chima_return {
  CHIMA_NO_ERROR = 0,
  CHIMA_ALLOC_FAILURE,
  CHIMA_FILE_OPEN_FAILURE,
  CHIMA_FILE_WRITE_FAILURE,
  CHIMA_FILE_EOF,
  CHIMA_IMAGE_PARSE_FAILURE,
  CHIMA_INVALID_VALUE,
  CHIMA_INVALID_FORMAT,
  CHIMA_PACKING_FAILED,
  CHIMA_RETURN_COUNT,

  _CHIMA_RETURN_FORCE_32BIT = 0x7fffffff,
} chima_return;

typedef enum chima_image_format {
  CHIMA_FORMAT_RAW = 0,
  CHIMA_FORMAT_PNG,
  CHIMA_FORMAT_BMP,
  CHIMA_FORMAT_TGA,
  CHIMA_FORMAT_COUNT,

  _CHIMA_FORMAT_FORCE_32BIT = 0x7ffffff,
} chima_image_format;

CHIMADEF chima_return chima_create_context(chima_context* ctx, const chima_alloc* alloc);

CHIMADEF void chima_destroy_context(chima_context ctx);

CHIMADEF const char* chima_error_string(chima_return ret);

CHIMADEF float chima_set_atlas_factor(chima_context chima, float fac);

CHIMADEF chima_bool chima_set_image_y_flip(chima_context chima, chima_bool flip_y);

CHIMADEF chima_bool chima_set_uv_y_flip(chima_context chima, chima_bool flip_y);

CHIMADEF chima_bool chima_set_uv_x_flip(chima_context chima, chima_bool flip_x);

CHIMADEF chima_color chima_set_sheet_color(chima_context chima, chima_color color);

CHIMADEF chima_image_format chima_set_sheet_format(chima_context chima, chima_image_format format);

CHIMADEF chima_return chima_set_sheet_name(chima_context chima, const char* name);

CHIMADEF uint32_t chima_set_sheet_initial(chima_context chima, uint32_t initial_sz);

typedef enum chima_image_depth {
  CHIMA_DEPTH_8U = 0,
  CHIMA_DEPTH_16U,
  CHIMA_DEPTH_32F,
  CHIMA_DEPTH_COUNT,

  _CHIMA_DEPTH_FORCE_32BIT = 0x7fffffff,
} chima_image_depth;

typedef struct chima_anim chima_anim;

typedef struct chima_image {
  chima_context ctx;
  chima_string name;
  uint32_t width;
  uint32_t height;
  uint32_t channels;
  chima_image_depth depth;
  void* data;
  chima_anim* anim;
} chima_image;

CHIMADEF chima_return chima_create_image(chima_context chima, chima_image* image, uint32_t w,
                                         uint32_t h, uint32_t ch, chima_color color,
                                         const char* name);

CHIMADEF chima_return chima_load_image(chima_context chima, chima_image* image, const char* name,
                                       const char* path);

CHIMADEF chima_return chima_load_image_mem(chima_context chima, chima_image* image,
                                           const char* name, const uint8_t* buffer, size_t len);

CHIMADEF chima_return chima_write_image(const chima_image* image, const char* path,
                                        chima_image_format format);

CHIMADEF void chima_destroy_image(chima_image* image);

CHIMADEF chima_return chima_composite_image(chima_image* dst, const chima_image* src, uint32_t y,
                                            uint32_t x);

typedef struct chima_anim {
  chima_context ctx;
  chima_string name;
  chima_image* images;
  size_t image_count;
  float fps;
} chima_anim;

CHIMADEF chima_return chima_load_anim(chima_context chima, chima_anim* anim, const char* name,
                                      const char* path);

CHIMADEF void chima_destroy_anim(chima_anim* anim);

typedef enum chima_asset_type {
  CHIMA_ASSET_INVALID = 0,
  CHIMA_ASSET_SPRITESHEET,
  CHIMA_ASSET_MODEL3D,
  CHIMA_ASSET_COUNT,

  _CHIMA_ASSET_FORCE_32BIT = 0x7ffffff,
} chima_asset_type;

typedef struct chima_sprite {
  uint32_t width;
  uint32_t height;
  uint32_t x_off;
  uint32_t y_off;
  float uv_x_lin;
  float uv_x_con;
  float uv_y_lin;
  float uv_y_con;
  chima_string name;
} chima_sprite;

typedef struct chima_sprite_anim {
  uint32_t sprite_idx;
  uint32_t sprite_count;
  float fps;
  chima_string name;
} chima_sprite_anim;

typedef struct chima_spritesheet {
  chima_asset_type asset_type;
  chima_context ctx;
  chima_image atlas;
  chima_sprite* sprites;
  uint32_t sprite_count;
  chima_sprite_anim* anims;
  uint32_t anim_count;
} chima_spritesheet;

CHIMADEF chima_return chima_create_atlas_image(chima_context chima, chima_image* atlas,
                                               chima_sprite* sprites, uint32_t pad,
                                               const chima_image** images, size_t image_count);

CHIMADEF chima_return chima_create_spritesheet(chima_context chima, chima_spritesheet* sheet,
                                               uint32_t pad, const chima_image* images,
                                               size_t image_count, const chima_anim* anims,
                                               size_t anim_count);

CHIMADEF chima_return chima_load_spritesheet(chima_context chima, chima_spritesheet* sheet,
                                             const char* path);

CHIMADEF chima_return chima_write_spritesheet(const chima_spritesheet* sheet, const char* path);

CHIMADEF void chima_destroy_spritesheet(chima_spritesheet* sheet);

#ifdef __cplusplus
}
#endif
