#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_rect_pack.h"

#include "chimatools.h"

#define CHIMA_UNUSED(x) (void)x
#define CHIMA_UNREACHABLE() __builtin_unreachable()

#define CHIMA_MALLOC(ctx, sz) ctx->alloc.malloc(ctx->alloc.user_data, sz)
#define CHIMA_FREE(ctx, ptr) ctx->alloc.free(ctx->alloc.user_data, ptr)

#define CHIMA_FORMAT_MAX_SIZE 7

#define CHIMA_STATIC_ASSERT(cond) _Static_assert(cond, #cond)

typedef struct chima_context_ {
  chima_alloc alloc;
} chima_context_;

static void* chima_malloc(void* user, size_t size) {
  CHIMA_UNUSED(user);
  void* mem = malloc(size);
  // printf("- MALLOC: %p (%lu)\n", mem, size);
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
  // printf("- REALLOC: %p (%lu) -> %p (%lu)\n", ptr, oldsz, mem, newsz);
  return mem;
}

chima_return chima_create_context(chima_context* ctx, const chima_alloc* alloc) {
  if (!ctx) {
    return CHIMA_INVALID_VALUE;
  }

  chima_alloc chima_funcs;
  if (alloc) {
    chima_funcs = *alloc;
  } else {
    chima_funcs.user_data = NULL;
    chima_funcs.malloc = &chima_malloc;
    chima_funcs.free = &chima_free;
    chima_funcs.realloc = &chima_realloc;
  }
  assert(chima_funcs.malloc);
  assert(chima_funcs.free);
  assert(chima_funcs.realloc);

  chima_context chima = chima_funcs.malloc(chima_funcs.user_data, sizeof(chima_context_));
  if (!chima) {
    *ctx = NULL;
    return CHIMA_ALLOC_FAILURE;
  }

  chima->alloc = chima_funcs;

  (*ctx) = chima;
  return CHIMA_NO_ERROR;
}

void chima_destroy_context(chima_context ctx) {
  if (!ctx) {
    return;
  }
  void* user = ctx->alloc.user_data;
  PFN_chima_free func = ctx->alloc.free;
  func(user, ctx);
}

const char* chima_error_string(chima_return ret) {
  switch (ret) {
    case CHIMA_NO_ERROR: return "No error";
    case CHIMA_ALLOC_FAILURE: return "Allocation failure";
    case CHIMA_FILE_OPEN_FAILURE: return "Unable to open file";
    case CHIMA_FILE_WRITE_FAILURE: return "Failed to write image";
    case CHIMA_INVALID_VALUE: return "Invalid value";
    case CHIMA_INVALID_FORMAT: return "Invalid image format";
    case CHIMA_IMAGE_PARSE_FAILURE: return "Failed to parse image";
    default: return "";
  }
}

// typedef enum chima_image_depth {
//   CHIMA_DEPTH_8U = 0,
//   CHIMA_DEPTH_16U,
//   CHIMA_DEPTH_32F,
//   CHIMA_DEPTH_COUNT,
//
//   _CHIMA_DEPTH_FORCE_32BIT = 0x7fffffff,
// } chima_image_depth;

chima_return chima_create_image(chima_context ctx, chima_image* image,
                                uint32_t w, uint32_t h, uint32_t ch, chima_color color)
{
  if (!ctx || !image || !w || !h) {
    return CHIMA_INVALID_VALUE;
  }
  if (!ch || ch > 4) {
    return CHIMA_INVALID_VALUE;
  }

  // switch (depth) {
  //   case CHIMA_DEPTH_8U: {
  //     size_t stride = channels*sizeof(uint8_t);
  //     size_t sz = w*h*stride;
  //     data = CHIMA_MALLOC(ctx, sz);
  //     if (!data){
  //       return CHIMA_ALLOC_FAILURE;
  //     }
  //     uint8_t cols[] = {
  //       (uint8_t)floorf(color.r*0xFF),
  //       (uint8_t)floorf(color.g*0xFF),
  //       (uint8_t)floorf(color.b*0xFF),
  //       (uint8_t)floorf(color.a*0xFF),
  //     };
  //     for (size_t i = 0; i < sz; i += stride) {
  //       memcpy(data+i, cols, stride);
  //     }
  //     break;
  //   }
  //   case CHIMA_DEPTH_16U: {
  //     size_t stride = channels*sizeof(uint16_t);
  //     size_t sz = w*h*stride;
  //     data = CHIMA_MALLOC(ctx, sz);
  //     if (!data){
  //       return CHIMA_ALLOC_FAILURE;
  //     }
  //     uint16_t cols[] = {
  //       (uint16_t)floorf(color.r*0xFFFF),
  //       (uint16_t)floorf(color.g*0xFFFF),
  //       (uint16_t)floorf(color.b*0xFFFF),
  //       (uint16_t)floorf(color.a*0xFFFF),
  //     };
  //     for (size_t i = 0; i < sz; i += stride) {
  //       memcpy(data+i, cols, stride);
  //     }
  //     break;
  //   }
  //   case CHIMA_DEPTH_32F: {
  //     size_t stride = channels*sizeof(uint16_t);
  //     size_t sz = w*h*stride;
  //     data = CHIMA_MALLOC(ctx, sz);
  //     if (!data){
  //       return CHIMA_ALLOC_FAILURE;
  //     }
  //     float cols[] = {color.r, color.g, color.b, color.a};
  //     for (size_t i = 0; i < sz; i += stride) {
  //       memcpy(data+i, cols, stride);
  //     }
  //     break;
  //   }
  //   default: CHIMA_UNREACHABLE();
  // }

  size_t sz = w*h*ch;
  void* data = CHIMA_MALLOC(ctx, sz);
  if (!data){
    return CHIMA_ALLOC_FAILURE;
  }
  uint8_t cols[] = {
    (uint8_t)floorf(color.r*0xFF),
    (uint8_t)floorf(color.g*0xFF),
    (uint8_t)floorf(color.b*0xFF),
    (uint8_t)floorf(color.a*0xFF),
  };
  for (size_t i = 0; i < sz; i += ch) {
    memcpy(data+i, cols, ch);
  }

  image->width = w;
  image->height = h;
  image->channels = ch;
  image->data = data;
  image->anim = NULL;
  // image->depth = depth;

  return CHIMA_NO_ERROR;
}

static void prepare_stbi_ri(stbi__result_info* ri) {
  memset(ri, 0, sizeof(*ri)); // make sure it's initialized if we add new fields
  ri->bits_per_channel = 8; // default is 8 so most paths don't have to be changed
  ri->channel_order = STBI_ORDER_RGB; // all current input & output are this, but this is here so we can add BGR order
  ri->num_channels = 0;
}

static chima_return load_texels(stbi__context* stbi, stbi__result_info* ri, void** result,
                                int* w, int* h, int* comp, int req_comp, int flip_y)
{
  if (stbi__png_test(stbi)) {
    *result = stbi__png_load(stbi, w, h, comp, req_comp, ri);
  } else if (stbi__bmp_test(stbi)) {
    *result = stbi__bmp_load(stbi, w, h, comp, req_comp, ri);
  } else if (stbi__jpeg_test(stbi)) {
    *result = stbi__jpeg_load(stbi, w, h, comp, req_comp, ri);
  } else {
    return CHIMA_INVALID_FORMAT;
  }

  if (!*result) {
    return CHIMA_IMAGE_PARSE_FAILURE;
  }
  // TODO: Handle image depth requests
  stbi__postprocess_8bit(stbi, ri, result, *w, *h, *comp, req_comp, flip_y);

  // switch (depth) {
  //   case CHIMA_DEPTH_8U: {
  //     printf("Processing 8bit\n");
  //     stbi__postprocess_8bit(&stbi, &ri, &result, w, h, comp, req_comp, flip_y);
  //     // if (result) {
  //     //   // need to 'unget' all the characters in the IO buffer
  //     //   fseek(f, - (int) (stbi.img_buffer_end - stbi.img_buffer), SEEK_CUR);
  //     // }
  //     break;
  //   }
  //   case CHIMA_DEPTH_16U: {
  //     stbi__postprocess_16bit(&stbi, &ri, &result, w, h, comp, req_comp, flip_y);
  //     // if (result) {
  //     //   // need to 'unget' all the characters in the IO buffer
  //     //   fseek(f, - (int) (stbi.img_buffer_end - stbi.img_buffer), SEEK_CUR);
  //     // }
  //     break;
  //   }
  //   case CHIMA_DEPTH_32F: {
  //     stbi__postprocess_8bit(&stbi, &ri, &result, w, h, comp, req_comp, flip_y);
  //     if (result) {
  //       result = stbi__ldr_to_hdr(&stbi, result, w, h, req_comp ? req_comp : comp);
  //     }
  //     break;
  //   }
  //   default: CHIMA_UNREACHABLE();
  // }

  return CHIMA_NO_ERROR;
}

chima_return chima_load_image(chima_context ctx, const char* path,
                              chima_image* image, chima_bool flip_y)
{
  if (!ctx || !path || !image) {
    return CHIMA_INVALID_VALUE;
  }

  FILE* f = fopen(path, "rb");
  if (!f) {
    return CHIMA_FILE_OPEN_FAILURE;
  }

  stbi__context stbi;
  stbi__start_file(&stbi, f);
  stbi.a = ctx->alloc;

  stbi__result_info ri;
  prepare_stbi_ri(&ri);

  int w, h, comp;
  int req_comp = 0;
  void* result = NULL;

  uint32_t ret = load_texels(&stbi, &ri, &result, &w, &h, &comp, req_comp, flip_y);
  fclose(f);
  if (ret) {
    return ret;
  }

  assert(result);
  image->channels = (uint8_t)comp;
  image->height = (uint32_t)h;
  image->width = (uint32_t)w;
  image->data = result;
  image->anim = NULL;

  // image->depth = (uint8_t)depth;
  // memset(image->format, 0, CHIMA_FORMAT_MAX_SIZE);
  // strcpy(image->format, "RAW");
  // size_t name_len = strlen(name);
  // name_len = name_len > CHIMA_STRING_MAX_SIZE ? CHIMA_STRING_MAX_SIZE : name_len;
  // strncpy(image->name.data, name, name_len);
  // image->name.length = name_len;

  return CHIMA_NO_ERROR;
}

chima_return chima_load_image_mem(chima_context ctx, const uint8_t* buff, size_t len,
                                  chima_image* image, chima_bool flip_y)
{
  if (!ctx || !buff || !len || !image) {
    return CHIMA_INVALID_VALUE;
  }

  stbi__context stbi;
  stbi__start_mem(&stbi, buff, len);
  stbi.a = ctx->alloc;

  stbi__result_info ri;
  prepare_stbi_ri(&ri);

  int w, h, comp;
  int req_comp = 0;
  void* result = NULL;

  uint32_t ret = load_texels(&stbi, &ri, &result, &w, &h, &comp, req_comp, flip_y);
  if (ret) {
    return ret;
  }

  assert(result);
  image->channels = (uint8_t)comp;
  image->height = (uint32_t)h;
  image->width = (uint32_t)w;
  image->data = result;
  image->anim = NULL;

  return CHIMA_NO_ERROR;
}

chima_return chima_write_image(chima_context chima, const chima_image* image, const char* path,
                               chima_image_format format)
{
  size_t st = image->width*image->channels;
  int wrt;
  switch (format) {
    case CHIMA_FORMAT_PNG: {
      wrt = stbi_write_png(&chima->alloc,
                           path, image->width, image->height, image->channels, image->data, st);
      break;
    }
    case CHIMA_FORMAT_BMP: {
      wrt = stbi_write_bmp(path, image->width, image->height, image->channels, image->data);
      break;
    }
    case CHIMA_FORMAT_TGA: {
      wrt = stbi_write_tga(path, image->width, image->height, image->channels, image->data);
      break;
    }
    default: return CHIMA_INVALID_FORMAT;
  }

  if (!wrt) {
    return CHIMA_FILE_WRITE_FAILURE;
  }
  return CHIMA_NO_ERROR;
}


void chima_destroy_image(chima_context ctx, chima_image* image) {
  if (!ctx || !image) {
    return;
  }
  CHIMA_FREE(ctx, image->data);
}

chima_return chima_composite_image(chima_image* dst, const chima_image* src,
                                   uint32_t x, uint32_t y)
{
  if (!dst || !src) {
    return CHIMA_INVALID_VALUE;
  }

  const size_t dst_w = dst->width, dst_h = dst->height, dst_ch = dst->channels;
  uint8_t* dst_row_start = dst->data + (y*dst_w + x)*dst_ch;
  uint8_t* dst_end = dst->data + dst_w*dst_h*dst_ch;

  const size_t src_w = src->width, src_h = src->height, src_ch = src->channels;
  const size_t src_row_pixels = x+src_w > dst_w ? dst_w - x : src_w;
  size_t written_rows = 0;

  while (dst_row_start < dst_end && written_rows < src_h) {
    uint8_t* src_row_start = src->data + (written_rows*src_w*src_ch);
    for (size_t pixel = 0; pixel < src_row_pixels; ++pixel) {
      uint8_t* dst_pixel = dst_row_start+(pixel*dst_ch);
      uint8_t* src_pixel = src_row_start+(pixel*src_ch);

      float src_alpha = src_ch == 4 ? (float)(src_pixel[3])/256.f : 1.f;
      float dst_alpha = dst_ch == 4 ? (float)(dst_pixel[3])/256.f : 1.f;
      float comp_alpha = src_alpha + dst_alpha*(1.f-src_alpha);
      for (uint32_t i = 0; i < dst_ch; ++i) {
        float dst_blend = dst_pixel[i]*dst_alpha*(1.f-src_alpha) + src_pixel[i]*src_alpha;
        dst_pixel[i] = (uint8_t)roundf(dst_blend/comp_alpha);
      }
    }
    ++written_rows;
    dst_row_start += dst_w*dst_ch;
  }
 
  return CHIMA_NO_ERROR;
}

typedef struct gif_result {
  void* data;
  int delay;
  struct gif_result* next;
} gif_result;

chima_return chima_load_anim(chima_context chima, const char* path,
                             chima_anim* anim, chima_bool flip_y)
{
  if (!chima || !path || !anim) {
    return CHIMA_INVALID_VALUE;
  }

  FILE* f = fopen(path, "rb");
  if (!f) {
    return CHIMA_FILE_OPEN_FAILURE;
  }

  stbi__context stbi;
  stbi__start_file(&stbi, f);
  stbi.a = chima->alloc;

  stbi__result_info ri;
  prepare_stbi_ri(&ri);

  int comp;
  int req_comp = 0;

  if (!stbi__gif_test(&stbi)) {
    fclose(f);
    return CHIMA_INVALID_FORMAT;
  }

  stbi__gif gif;
  memset(&gif, 0, sizeof(gif));
  void* data = NULL;
  uint8_t* two_back = NULL;

  gif_result head;
  memset(&head, 0, sizeof(head));

  gif_result* prev = NULL;
  gif_result* gr = &head;
  size_t frames = 0;

  // https://gist.github.com/urraka/685d9a6340b26b830d49
  while ((data = stbi__gif_load_next(&stbi, &gif, &comp, req_comp, two_back)) != NULL) {
    if (data == &stbi) {
      data = NULL;
      break;
    }
    printf("- %lu (%p) -> %dx%d %d,%d %d\n",
           frames, gif.out, gif.w, gif.h, gif.cur_x, gif.cur_y, gif.delay);
    printf("- data %p\n", data);

    gr->data = data;
    gr->delay = gif.delay;
    prev = gr;
    gr = CHIMA_MALLOC(chima, sizeof(gif_result));
    assert(gr);
    memset(gr, 0, sizeof(gif_result));

    char buff[1024];
    snprintf(buff, 1024, "res/thing%lu.png", frames);
    stbi_write_png(&chima->alloc, buff, gif.w, gif.h, comp, data, 0);

    ++frames;
  }
  CHIMA_FREE(chima, gif.out);
  CHIMA_FREE(chima, gif.history);
  CHIMA_FREE(chima, gif.background);

  if (gr != &head) {
    CHIMA_FREE(chima, gr);
  }
  assert(0);

  // if (req_comp && req_comp != 4) {
  //
  // }

  return CHIMA_NO_ERROR;
}

void chima_destroy_anim(chima_context chima, chima_anim* anim) {
  if (!chima || !anim) {
    return;
  }
  for (size_t i = 0; i < anim->image_count; ++i) {
    chima_destroy_image(chima, anim->images+i);
  }
}

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

int main() {
  chima_context chima;
  chima_create_context(&chima, NULL);

  chima_color color;
  color.r = 1.f;
  color.g = 1.f;
  color.b = 1.f;
  color.a = 1.f;
  chima_image dst;
  uint32_t ret = chima_create_image(chima, &dst, 1024, 1024, 4, color);
  printf("%s\n", chima_error_string(ret));
  assert(ret == CHIMA_NO_ERROR);

  chima_anim cino;
  ret = chima_load_anim(chima, "res/cino.gif", &cino, 0);
  printf("%s\n", chima_error_string(ret));
  assert(ret == CHIMA_NO_ERROR);

  chima_image src;
  color.g = 0.f;
  color.b = 0.f;
  color.a = .5f;
  ret = chima_create_image(chima, &src, 512, 512, 4, color);
  printf("%s\n", chima_error_string(ret));
  assert(ret == CHIMA_NO_ERROR);

  chima_image chimata;
  ret = chima_load_image(chima, "res/chimata.png", &chimata, 0);
  printf("%s\n", chima_error_string(ret));
  assert(ret == CHIMA_NO_ERROR);

  chima_image marisa;
  ret = chima_load_image(chima, "res/marisa_emacs.png", &marisa, 0);
  printf("%s\n", chima_error_string(ret));
  assert(ret == CHIMA_NO_ERROR);

  ret = chima_composite_image(&dst, &src, 0, 0);
  printf("%s\n", chima_error_string(ret));
  assert(ret == CHIMA_NO_ERROR);

  ret = chima_composite_image(&dst, &chimata, 0, 0);
  printf("%s\n", chima_error_string(ret));
  assert(ret == CHIMA_NO_ERROR);

  ret = chima_composite_image(&dst, &marisa, chimata.width/2, 0);
  printf("%s\n", chima_error_string(ret));
  assert(ret == CHIMA_NO_ERROR);

  chima_write_image(chima, &dst, "res/chima_test.png", CHIMA_FORMAT_PNG);

  chima_destroy_anim(chima, &cino);
  chima_destroy_image(chima, &marisa);
  chima_destroy_image(chima, &chimata);
  chima_destroy_image(chima, &dst);
  chima_destroy_image(chima, &src);
  chima_destroy_context(chima);
}

