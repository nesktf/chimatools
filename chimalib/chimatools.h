#pragma once

#include "chima_common.h"
#define CHIMADEF extern
#define CHIMA_STRING_MAX_SIZE 1024
#define CHIMA_TRUE 1
#define CHIMA_FALSE 0

#ifdef __cplusplus
extern "C" {
#endif

struct chima_context_;
typedef struct chima_context_ *chima_context;

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
  CHIMA_IMAGE_PARSE_FAILURE,
  CHIMA_INVALID_VALUE,
  CHIMA_INVALID_FORMAT,
  CHIMA_RETURN_COUNT,

  _CHIMA_RETURN_FORCE_32BIT = 0x7fffffff,
} chima_return;

CHIMADEF chima_return chima_create_context(chima_context* ctx, const chima_alloc* alloc);

CHIMADEF void chima_destroy_context(chima_context ctx);

CHIMADEF const char* chima_error_string(chima_return ret);

typedef enum chima_image_format {
  CHIMA_FORMAT_RAW = 0,
  CHIMA_FORMAT_PNG,
  CHIMA_FORMAT_BMP,
  CHIMA_FORMAT_TGA,
  CHIMA_FORMAT_COUNT,

  _CHIMA_FORMAT_FORCE_32BIT = 0x7ffffff,
} chima_image_format;

typedef struct chima_anim chima_anim;
typedef struct chima_image {
  uint32_t width;
  uint32_t height;
  uint8_t channels;
  void* data;
  chima_anim* anim;
} chima_image;

CHIMADEF chima_return chima_create_image(chima_context chima, chima_image* image,
                                         uint32_t w, uint32_t h, uint32_t ch, chima_color color);

CHIMADEF chima_return chima_load_image(chima_context chima, const char* path,
                                       chima_image* image, chima_bool flip_y);

CHIMADEF chima_return chima_load_image_mem(chima_context chima,
                                           const uint8_t* buffer, size_t len,
                                           chima_image* image, chima_bool flip_y);

CHIMADEF chima_return chima_write_image(chima_context chima,
                                        const chima_image* image, const char* path,
                                        chima_image_format format);

CHIMADEF void chima_destroy_image(chima_context chima, chima_image* image);

CHIMADEF chima_return chima_composite_image(chima_image* dst, const chima_image* src,
                                            uint32_t y, uint32_t x);

typedef struct chima_anim {
  chima_image* images;
  size_t image_count;
  float fps;
} chima_anim;

CHIMADEF chima_return chima_load_anim(chima_context chima, const char* path,
                                      chima_anim* anim, chima_bool flip_y);

CHIMADEF chima_return chima_load_anim_mem(chima_context chima,
                                          const uint8_t* buffer, size_t len,
                                          chima_anim* anim, chima_bool flip_y);

CHIMADEF void chima_destroy_anim(chima_context chima, chima_anim* anim);

typedef struct chima_sprite {
  chima_string name;
  uint32_t width;
  uint32_t height;
  uint32_t x_off;
  uint32_t y_off;
  float uv_x_lin;
  float uv_x_con;
  float uv_y_lin;
  float uv_y_con;
} chima_sprite;

typedef struct chima_sprite_anim {
  chima_string name;
  size_t* sprite_indices;
  size_t sprite_count;
  float fps;
} chima_sprite_anim;

CHIMADEF chima_return chima_create_atlas_image(chima_context chima,
                                               chima_image* atlas, chima_sprite* sprites,
                                               const chima_image* images, size_t image_count);

CHIMADEF chima_return chima_write_spritesheet(chima_context chima,
                                              const char* path, const chima_image* atlas,
                                              const chima_sprite* sprites, size_t sprite_count,
                                              const chima_sprite_anim* anims, size_t anim_count);

#ifdef __cplusplus
}
#endif
