#pragma once

#include "chima_common.h"
#define CHIMADEF extern
#define CHIMA_STRING_MAX_SIZE 1024
#define CHIMA_FORMAT_MAX_SIZE 7
#define CHIMA_TRUE 1
#define CHIMA_FALSE 0

#ifdef __cplusplus
extern "C" {
#endif

struct chima_context_;
typedef struct chima_context_ *chima_context;

typedef enum chima_return {
  CHIMA_NO_ERROR = 0,
  CHIMA_ALLOC_FAILURE,
  CHIMA_FILE_OPEN_FAILURE,
  CHIMA_IMAGE_PARSE_FAILURE,
  CHIMA_INVALID_VALUE,
  CHIMA_INVALID_FORMAT,
  CHIMA_RETURN_COUNT,

  _CHIMA_RETURN_FORCE_32BIT = 0x7fffffff,
} chima_return;

CHIMADEF uint32_t chima_create_context(chima_context* ctx, const chima_alloc* alloc);
CHIMADEF void chima_destroy_context(chima_context ctx);
CHIMADEF const char* chima_error_string(chima_return ret);

typedef struct chima_string {
  uint32_t length;
  char data[CHIMA_STRING_MAX_SIZE];
} chima_string;

typedef uint32_t chima_bool;

typedef enum chima_image_depth {
  CHIMA_DEPTH_8U = 0,
  CHIMA_DEPTH_16U,
  CHIMA_DEPTH_32F,
  CHIMA_DEPTH_COUNT,

  _CHIMA_DEPTH_FORCE_32BIT = 0x7fffffff,
} chima_image_depth;

typedef struct chima_image {
  uint32_t width;
  uint32_t height;
  uint8_t channels;
  uint8_t depth;
  // char format[CHIMA_FORMAT_MAX_SIZE];
  void* data;
} chima_image;

typedef struct chima_color {
  float r, g, b, a;
} chima_color;

chima_return chima_create_image(chima_context ctx, chima_image* image, uint32_t w, uint32_t h,
                                uint32_t channels, chima_image_depth depth, chima_color color);
chima_bool chima_composite_image(chima_image* dst, const chima_image* src, uint32_t w, uint32_t h);

typedef struct chima_anim {
  chima_image* images;
  size_t image_count;
} chima_anim;

typedef enum chima_asset_type {
  CHIMA_ASSET_INVALID = 0,
  CHIMA_ASSET_SPRITESHEET,
  CHIMA_ASSET_MODEL3D,
  CHIMA_ASSET_COUNT,

  _CHIMA_ASSET_FORCE_32BIT = 0x7ffffff,
} chima_asset_type;

typedef struct chima_file_header {
  uint8_t magic[12];
  uint16_t file_enum;
  uint8_t ver_maj;
  uint8_t ver_min;
} chima_file_header;
CHIMA_STATIC_ASSERT(sizeof(chima_file_header) == 16);

typedef struct chima_file_sprite_meta {
  uint32_t sprite_count;
  uint32_t anim_count;
  uint32_t image_width;
  uint32_t image_height;
  uint32_t image_offset;
  uint8_t image_channels;
  char image_format[7]; // Up to 6 chars + null terminator
} chima_file_sprite_meta;
CHIMA_STATIC_ASSERT(sizeof(chima_file_sprite_meta) == 28);

typedef struct chima_file_sprite {
  uint32_t width;
  uint32_t height;
  uint32_t x_off;
  uint32_t y_off;
  float uv_x_lin;
  float uv_x_con;
  float uv_y_lin;
  float uv_y_con;
  uint32_t name_offset;
  uint32_t name_size;
} chima_file_sprite;
CHIMA_STATIC_ASSERT(sizeof(chima_file_sprite) == 40);

typedef struct chima_file_anim {
  uint32_t sprite_idx;
  uint32_t sprite_count;
  uint32_t name_offset;
  uint32_t name_size;
  float fps;
} chima_file_anim;
CHIMA_STATIC_ASSERT(sizeof(chima_file_anim) == 20);

#ifdef __cplusplus
}
#endif
