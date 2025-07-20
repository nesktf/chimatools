#include "chimatools.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_NO_FAILURE_STRINGS
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_BMP
#define STBI_ONLY_GIF
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#include "stb_image_write.h"

#define STB_RECT_PACK_IMPLEMENTATION
#define STBRP_STATIC
#include "stb_rect_pack.h"

#include <stdalign.h>

#define CHIMA_UNUSED(x) (void)x
#define CHIMA_UNREACHABLE() __builtin_unreachable()

#define CHIMA_MALLOC(ctx, sz) ctx->alloc.malloc(ctx->alloc.user_data, sz)
#define CHIMA_FREE(ctx, ptr) ctx->alloc.free(ctx->alloc.user_data, ptr)

#define CHIMA_FORMAT_MAX_SIZE 7
#define CHIMA_SHEET_MAJ 0
#define CHIMA_SHEET_MIN 1

#define CHIMA_STATIC_ASSERT(cond) _Static_assert(cond, #cond)
#define ATLAS_MAX_SIZE 16384
#define ATLAS_INIT_SIZE 512
#define ATLAS_GROW_FAC 2.f
#define ATLAS_NAME "atlas"

static const char CHIMA_MAGIC[] =
  { 0x89, 'C', 'H', 'I', 'M', 'A', 0x89, 'A', 'S', 'S', 'E', 'T'};

// TODO: Add a scratch arena
typedef struct chima_context_ {
  chima_alloc alloc;
  size_t image_count;
  size_t anim_count;
  float atlas_grow_fac;
  chima_bool flip_y;
  uint32_t atlas_initial;
  chima_color atlas_color;
  chima_image_format atlas_format;
  chima_string atlas_name;
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

chima_return chima_create_context(chima_context* chima, const chima_alloc* alloc) {
  if (!chima) {
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

  chima_context ctx = chima_funcs.malloc(chima_funcs.user_data, sizeof(chima_context_));
  if (!ctx) {
    *chima = NULL;
    return CHIMA_ALLOC_FAILURE;
  }
  memset(ctx, 0, sizeof(chima_context_));
  ctx->image_count = 0;
  ctx->anim_count = 0;
  ctx->alloc = chima_funcs;
  ctx->atlas_grow_fac = ATLAS_GROW_FAC;
  ctx->flip_y = CHIMA_FALSE;
  ctx->atlas_color.r = 1.f;
  ctx->atlas_color.g = 1.f;
  ctx->atlas_color.b = 1.f;
  ctx->atlas_color.a = 0.f;
  ctx->atlas_format = CHIMA_FORMAT_PNG;
  ctx->atlas_initial = ATLAS_INIT_SIZE;

  ctx->atlas_name.length = sizeof(ATLAS_NAME);
  strncpy(ctx->atlas_name.data, ATLAS_NAME, sizeof(ATLAS_NAME));

  (*chima) = ctx;
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
    case CHIMA_PACKING_FAILED: return "Sprite packing failed";
    case CHIMA_FILE_EOF: return "Early end of file";
    default: return "";
  }
}

float chima_set_atlas_factor(chima_context chima, float fac) {
  if (fac <= 1.f) {
    return 0.f;
  }
  float r = chima->atlas_grow_fac;
  chima->atlas_grow_fac = fac;
  return r;
}

chima_bool chima_set_y_flip(chima_context chima, chima_bool flip_y) {
  chima_bool r = chima->flip_y;
  chima->flip_y = flip_y;
  return r;
}

chima_color chima_set_sheet_color(chima_context chima, chima_color color) {
  chima_color r = chima->atlas_color;
  chima->atlas_color = color;
  return r;
}

chima_image_format chima_set_sheet_format(chima_context chima, chima_image_format format) {
  chima_image_format r = chima->atlas_format;
  if (format < CHIMA_FORMAT_COUNT) {
    chima->atlas_format = format;
  }
  return r;
}

chima_return chima_set_sheet_name(chima_context chima, const char* name) {
  int len = strlen(name);
  if (len > CHIMA_STRING_MAX_SIZE){
    return CHIMA_INVALID_VALUE;
  }
  chima->atlas_name.length = len;
  strncpy(chima->atlas_name.data, name, len);
  return CHIMA_NO_ERROR;
}

uint32_t chima_set_sheet_initial(chima_context chima, uint32_t initial_sz) {
  uint32_t ret = chima->atlas_initial;
  if (initial_sz) {
    chima->atlas_initial = initial_sz;
  }
  return ret;
}

static void rename_image(const char* name, chima_image* image, size_t fmt_count) {
  if (name) {
    image->name.length = strlen(name);
    memcpy(image->name.data, name, image->name.length+1); // copy null terminator
  } else {
    int len = snprintf(image->name.data, CHIMA_STRING_MAX_SIZE,
                       "chima_image%lu", fmt_count);
    assert(len > 0);
    image->name.length = (size_t)len-1; // No null terminator
  }
}

static void rename_anim(const char* name, chima_anim* anim, size_t fmt_count) {
  char name_fmt_buff[1024] = {0};
  if (name) {
    anim->name.length = strlen(name);
    memcpy(anim->name.data, name, anim->name.length+1); // copy null terminator
  } else {
    int len = snprintf(anim->name.data, CHIMA_STRING_MAX_SIZE,
                       "chima_anim%lu", fmt_count);
    assert(len > 0);
    anim->name.length = (size_t)len-1; // No null terminator
  }

  const char fmt[] = ".%lu";
  memcpy(name_fmt_buff, anim->name.data, anim->name.length);
  memcpy(name_fmt_buff+anim->name.length, fmt, sizeof(fmt));
  // Names each animation like "<anim_name>.<image_idx>"
  for (size_t i = 0; i < anim->image_count; ++i) {
    chima_image* image = anim->images+i;
    int len = snprintf(image->name.data, CHIMA_STRING_MAX_SIZE,
                       name_fmt_buff, i);
    assert(len > 0);
    image->name.length = (size_t)len;
  }
}

chima_return chima_create_image(chima_context chima, chima_image* image,
                                uint32_t w, uint32_t h, uint32_t ch,
                                chima_color color, const char* name)
{
  if (!chima || !image || !w || !h) {
    return CHIMA_INVALID_VALUE;
  }
  if (!ch || ch > 4) {
    return CHIMA_INVALID_VALUE;
  }

  size_t sz = w*h*ch;
  void* data = CHIMA_MALLOC(chima, sz);
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

  memset(image, 0, sizeof(chima_image));
  image->width = w;
  image->height = h;
  image->channels = ch;
  image->data = data;
  image->anim = NULL;
  image->depth = CHIMA_DEPTH_8U;
  rename_image(name, image, chima->image_count);
  image->ctx = chima;
  ++chima->image_count;

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

  return CHIMA_NO_ERROR;
}

chima_return chima_load_image(chima_context chima, chima_image* image,
                              const char* name, const char* path)
{
  if (!chima || !path || !image) {
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

  int w, h, comp;
  int req_comp = 0;
  void* result = NULL;

  chima_return ret = load_texels(&stbi, &ri, &result, &w, &h, &comp, req_comp, chima->flip_y);
  fclose(f);
  if (ret != CHIMA_NO_ERROR) {
    return ret;
  }

  assert(w > 0 && h > 0);
  assert(comp > 0);
  assert(result);

  memset(image, 0, sizeof(chima_image));
  image->width = (uint32_t)w;
  image->height = (uint32_t)h;
  image->channels = (uint32_t)comp;
  image->data = result;
  image->anim = NULL;
  image->depth = CHIMA_DEPTH_8U;
  rename_image(name, image, chima->image_count);
  image->ctx = chima;
  ++chima->image_count;

  return CHIMA_NO_ERROR;
}

chima_return chima_load_image_mem(chima_context chima,
                                  chima_image* image, const char* name,
                                  const uint8_t* buff, size_t len)
{
  if (!chima || !buff || !len || !image) {
    return CHIMA_INVALID_VALUE;
  }

  stbi__context stbi;
  stbi__start_mem(&stbi, buff, len);
  stbi.a = chima->alloc;

  stbi__result_info ri;
  prepare_stbi_ri(&ri);

  int w, h, comp;
  int req_comp = 0;
  void* result = NULL;

  chima_return ret = load_texels(&stbi, &ri, &result, &w, &h, &comp, req_comp, chima->flip_y);
  if (ret != CHIMA_NO_ERROR) {
    return ret;
  }

  assert(result);
  image->channels = (uint8_t)comp;
  image->height = (uint32_t)h;
  image->width = (uint32_t)w;
  image->data = result;
  image->anim = NULL;
  rename_image(name, image, chima->image_count);
  image->ctx = chima;
  ++chima->image_count;

  return CHIMA_NO_ERROR;
}

chima_return chima_write_image(const chima_image* image, const char* path,
                               chima_image_format format)
{
  size_t st = image->width*image->channels;
  int wrt;
  switch (format) {
    case CHIMA_FORMAT_PNG: {
      wrt = stbi_write_png(&image->ctx->alloc,
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


void chima_destroy_image(chima_image* image) {
  if (!image) {
    return;
  }
  CHIMA_FREE(image->ctx, image->data);
  memset(image, 0, sizeof(chima_image));
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

typedef struct gif_node {
  uint32_t width;
  uint32_t height;
  uint32_t channels;
  int delay;
  void* data;
  struct gif_node* next;
} gif_node;

chima_return chima_load_anim(chima_context chima,
                             chima_anim* anim, const char* name, const char* path)
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

  int comp = 0;

  if (!stbi__gif_test(&stbi)) {
    fclose(f);
    return CHIMA_INVALID_FORMAT;
  }

  stbi__gif gif;
  memset(&gif, 0, sizeof(gif));
  void* data = NULL;
  uint8_t* two_back = NULL;
  chima_return ret = CHIMA_NO_ERROR;

  // https://gist.github.com/urraka/685d9a6340b26b830d49
  gif_node head;
  memset(&head, 0, sizeof(head));
  uint32_t image_count = 0;
  gif_node* curr_node = NULL;
  chima_bool free_images = CHIMA_FALSE;
  while ((data = stbi__gif_load_next(&stbi, &gif, &comp, 0, two_back)) != NULL) {
    assert(comp);
    if (data == &stbi) {
      data = NULL;
      break;
    }

    if (!curr_node) {
      curr_node = &head; // First image
    } else {
      gif_node* node = CHIMA_MALLOC(chima, sizeof(gif_node));
      if (!node) {
        ret = CHIMA_ALLOC_FAILURE;
        free_images = CHIMA_TRUE;
        goto free_nodes;
      }
      memset(node, 0, sizeof(gif_node));
      curr_node->next = node;
      curr_node = node;
    }

    size_t image_sz = comp*gif.w*gif.h;
    curr_node->data = CHIMA_MALLOC(chima, image_sz);
    if (!curr_node->data) {
      ret = CHIMA_ALLOC_FAILURE;
      free_images = CHIMA_TRUE;
      goto free_nodes;
    }
    memcpy(curr_node->data, data, image_sz);
    curr_node->width = gif.w;
    curr_node->height = gif.h;
    curr_node->channels = comp;
    curr_node->delay = gif.delay;

    ++image_count;
  }
  CHIMA_FREE(chima, gif.out);
  CHIMA_FREE(chima, gif.history);
  CHIMA_FREE(chima, gif.background);

  chima_image* images = CHIMA_MALLOC(chima, image_count*sizeof(chima_image));
  if (!images) {
    ret = CHIMA_ALLOC_FAILURE;
    free_images = CHIMA_TRUE;
    goto free_nodes;
  }
  memset(images, 0, image_count*sizeof(chima_image));
  curr_node = &head;
  size_t i = 0;
  while (curr_node) {
    images[i].data = curr_node->data;
    images[i].height = curr_node->height;
    images[i].width = curr_node->width;
    images[i].channels = curr_node->channels;
    images[i].anim = anim;
    images[i].depth = CHIMA_DEPTH_8U;
    curr_node = curr_node->next;
    ++i;
  }

  memset(anim, 0, sizeof(chima_anim));
  anim->images = images;
  anim->image_count = image_count;
  anim->fps = 1000.f/(float)head.delay;
  rename_anim(name, anim, chima->anim_count);
  anim->ctx = chima;
  ++chima->anim_count;
  chima->image_count += image_count;

free_nodes:
  curr_node = head.next;
  while (curr_node) {
    gif_node* node = curr_node;
    curr_node = node->next;
    if (node->data && free_images) {
      CHIMA_FREE(chima, node->data);
    }
    CHIMA_FREE(chima, node);
  }
  if (free_images) {
    CHIMA_FREE(chima, head.data);
  }

  return ret;
}

void chima_destroy_anim(chima_anim* anim) {
  if (!anim) {
    return;
  }
  for (size_t i = 0; i < anim->image_count; ++i) {
    CHIMA_FREE(anim->ctx, anim->images[i].data);
  }
  CHIMA_FREE(anim->ctx, anim->images);
  memset(anim, 0, sizeof(chima_anim));
}

static size_t align_fw_adjust(void* ptr, size_t align) {
  uintptr_t iptr = (uintptr_t)ptr;
  // return ((iptr - 1u + align) & -align) - iptr;
  return align - (iptr & (align - 1u));
}

chima_return chima_create_atlas_image(chima_context chima, chima_image* atlas,
                                      chima_sprite* sprites, uint32_t pad,
                                      const chima_image** images, size_t image_count)
{
  // Evil hack: use the sprite array as stbrp_node and stbrp_rect storage
  // Remove this when the scratch arena is implemented (?)
  uint8_t* rect_start =
    (uint8_t*)(sprites+image_count)-(image_count+1)*sizeof(stbrp_rect); // one for alignment
  stbrp_rect* rects = (stbrp_rect*)(rect_start+align_fw_adjust(rects, alignof(stbrp_rect)));

  stbrp_node* nodes =
    (stbrp_node*)((uint8_t*)sprites+align_fw_adjust(sprites, alignof(stbrp_node)));
  size_t node_count = ((uintptr_t)rects - (uintptr_t)nodes)/sizeof(stbrp_node);

  memset(rects, 0, image_count*sizeof(stbrp_rect));
  for (size_t i = 0; i < image_count; ++i) {
    rects[i].w = images[i]->width+pad;
    rects[i].h = images[i]->height+pad;
  }

  uint32_t atlas_size = chima->atlas_initial;
  while (atlas_size <= ATLAS_MAX_SIZE) {

    stbrp_context stbrp;
    stbrp_init_target(&stbrp, atlas_size, atlas_size, nodes, node_count);
    if (stbrp_pack_rects(&stbrp, rects, image_count)) {
      goto copy_texels;
    }
    atlas_size = roundf(atlas_size*chima->atlas_grow_fac);
  }
  return CHIMA_PACKING_FAILED;

copy_texels:
  ;
  chima_return ret = chima_create_image(chima, atlas, atlas_size, atlas_size, 4,
                                        chima->atlas_color, chima->atlas_name.data);
  if (ret != CHIMA_NO_ERROR) {
    return ret;
  }
  for (size_t i = 0; i < image_count; ++i) {
    // Writting directly to the chima_sprites **should** be fine. Will need some tests
    // Copy in case we overlap with the stbrp_rect data in the last chima_sprite
    uint32_t x = rects[i].x;
    uint32_t y = rects[i].y;
    sprites[i].x_off = x;
    sprites[i].y_off = y;
    sprites[i].width = images[i]->width;
    sprites[i].height = images[i]->height;
    memset(sprites[i].name.data, 0, CHIMA_STRING_MAX_SIZE);
    memcpy(sprites[i].name.data, images[i]->name.data, images[i]->name.length);
    sprites[i].name.length = images[i]->name.length;
    chima_composite_image(atlas, images[i], x, y);
  }

  return CHIMA_NO_ERROR;
}

chima_return chima_create_spritesheet(chima_context chima,
                                      chima_spritesheet* sheet, uint32_t pad,
                                      const chima_image* images, size_t image_count,
                                      const chima_anim* anims, size_t anim_count)
{
  if (!chima || !sheet) {
    return CHIMA_INVALID_VALUE;
  }
  if (!images && !anims) {
    return CHIMA_INVALID_VALUE;
  }
  if (images && !image_count) {
    return CHIMA_INVALID_VALUE;
  }
  if (anims && !anim_count) {
    return CHIMA_INVALID_VALUE;
  }

  size_t total_images = image_count;
  for (size_t i = 0; i < anim_count; ++i) {
    size_t count = anims[i].image_count;
    total_images += count;
  }

  const chima_image** tmp_images = CHIMA_MALLOC(chima, total_images*sizeof(const chima_image*));
  if (!tmp_images) {
    return CHIMA_ALLOC_FAILURE;
  }
  memset(tmp_images, 0, total_images*sizeof(const chima_image*));

  size_t image_off = 0;
  for (; image_off < image_count; ++image_off){
    tmp_images[image_off] = images+image_off;
  }
  for (size_t i = 0; i < anim_count; ++i) {
    size_t count = anims[i].image_count;
    for (size_t j = 0; j < count; ++j, ++image_off) {
      tmp_images[image_off] = anims[i].images+j;
    }
  }

  chima_sprite_anim* sprite_anims = CHIMA_MALLOC(chima, anim_count*sizeof(chima_sprite_anim));
  if (!sprite_anims) {
    CHIMA_FREE(chima, tmp_images);
    return CHIMA_ALLOC_FAILURE;
  }
  image_off = image_count;
  memset(sprite_anims, 0, anim_count*sizeof(chima_sprite_anim));
  for (size_t i = 0; i < anim_count; ++i) {
    size_t count = anims[i].image_count;
    sprite_anims[i].sprite_idx = image_off;
    image_off += count;
    sprite_anims[i].sprite_count = count;
    sprite_anims[i].fps = anims[i].fps;
    memcpy(sprite_anims[i].name.data, anims[i].name.data, anims[i].name.length);
    sprite_anims[i].name.length = anims[i].name.length;
  }

  chima_sprite* sprites = CHIMA_MALLOC(chima, total_images*sizeof(chima_sprite));
  if (!sprites) {
    CHIMA_FREE(chima, tmp_images);
    CHIMA_FREE(chima, sprite_anims);
    return CHIMA_ALLOC_FAILURE;
  }

  memset(sheet, 0, sizeof(chima_spritesheet));
  chima_return ret = chima_create_atlas_image(chima, &sheet->atlas, sprites, pad,
                                              tmp_images, total_images);
  if (ret != CHIMA_NO_ERROR) {
    CHIMA_FREE(chima, tmp_images);
    CHIMA_FREE(chima, sprite_anims);
    CHIMA_FREE(chima, sprites);
    return ret;
  }

  sheet->asset_type = CHIMA_ASSET_SPRITESHEET;
  sheet->sprite_count = total_images;
  sheet->anim_count = anim_count;
  sheet->anims = sprite_anims;
  sheet->sprites = sprites;
  sheet->ctx = chima;

  CHIMA_FREE(chima, tmp_images);

  return CHIMA_NO_ERROR;
}

void chima_destroy_spritesheet(chima_spritesheet* sheet) {
  if (!sheet) {
    return;
  }
  chima_context chima = sheet->ctx;
  CHIMA_FREE(chima, sheet->anims);
  CHIMA_FREE(chima, sheet->sprites);
  chima_destroy_image(&sheet->atlas);
  memset(sheet, 0, sizeof(chima_spritesheet));
}

typedef struct chima_sprite_file_header {
  uint8_t magic[sizeof(CHIMA_MAGIC)];
  uint16_t file_enum;
  uint8_t ver_maj;
  uint8_t ver_min;
  uint32_t sprite_count;
  uint32_t anim_count;
  uint32_t name_size;
  uint32_t name_offset;
  uint32_t image_width;
  uint32_t image_height;
  uint32_t image_offset;
  uint8_t image_channels;
  char image_format[7]; // Up to 6 chars + null terminator
} chima_sprite_file_header;

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

typedef struct chima_file_anim {
  uint32_t sprite_idx;
  uint32_t sprite_count;
  uint32_t name_offset;
  uint32_t name_size;
  float fps;
} chima_file_anim;

chima_return chima_load_spritesheet(chima_context chima,
                                    chima_spritesheet* sheet, const char* path)
{
  if (!path){
    return CHIMA_INVALID_VALUE;
  }

  FILE* f = fopen(path, "rb");
  if (!f) {
    return CHIMA_FILE_OPEN_FAILURE;
  }

  fseek(f, 0, SEEK_END);
  size_t file_sz = ftell(f);
  rewind(f);

  chima_sprite_file_header header;
  memset(&header, 0, sizeof(header));
  int read = fread(&header, sizeof(header), 1, f);
  if (read < 1) {
    return CHIMA_INVALID_FORMAT;
  }
  if (strncmp((char*)header.magic, CHIMA_MAGIC, sizeof(CHIMA_MAGIC))) {
    return CHIMA_INVALID_FORMAT;
  }
  if (header.file_enum != CHIMA_ASSET_SPRITESHEET){
    return CHIMA_INVALID_FORMAT;
  }
  if (header.ver_maj != CHIMA_SHEET_MAJ) {
    return CHIMA_INVALID_FORMAT;
  }
  if (header.ver_min != CHIMA_SHEET_MIN) {
    return CHIMA_INVALID_FORMAT;
  }

  // Assume everything else is fine...
  size_t read_offset = sizeof(header);
  size_t read_len = header.sprite_count*sizeof(chima_file_sprite);
  chima_file_sprite* fsprites = CHIMA_MALLOC(chima, read_len);
  if (!fsprites) {
    fclose(f);
    return CHIMA_ALLOC_FAILURE;
  }
  read = fread(fsprites, sizeof(fsprites[0]), header.sprite_count, f);
  if (read != header.sprite_count) {
    CHIMA_FREE(chima, fsprites);
    fclose(f);
    return CHIMA_FILE_EOF;
  }
  read_offset += read_len;

  read_len = header.anim_count*sizeof(chima_file_anim);
  chima_file_anim* fanims = CHIMA_MALLOC(chima, read_len);
  if (!fanims) {
    CHIMA_FREE(chima, fsprites);
    fclose(f);
    return CHIMA_ALLOC_FAILURE;
  }
  read = fread(fanims, sizeof(fanims[0]), header.anim_count, f);
  if (read != header.anim_count) {
    CHIMA_FREE(chima, fanims);
    CHIMA_FREE(chima, fsprites);
    fclose(f);
    return CHIMA_FILE_EOF;
  }
  read_offset += read_len;

  read_len = header.name_size;
  char* fnames = CHIMA_MALLOC(chima, read_len);
  if (!fnames){
    CHIMA_FREE(chima, fanims);
    CHIMA_FREE(chima, fsprites);
    fclose(f);
    return CHIMA_ALLOC_FAILURE;
  }
  read = fread(fnames, sizeof(fnames[0]), header.name_size, f);
  if (read != read_len) {
    CHIMA_FREE(chima, fnames);
    CHIMA_FREE(chima, fanims);
    CHIMA_FREE(chima, fsprites);
    fclose(f);
    return CHIMA_FILE_EOF;
  }
  read_offset += read_len;

  read_len = file_sz - read_offset;
  void* image_data = CHIMA_MALLOC(chima, read_len);
  if (!image_data) {
    CHIMA_FREE(chima, fnames);
    CHIMA_FREE(chima, fanims);
    CHIMA_FREE(chima, fsprites);
    fclose(f);
    return CHIMA_ALLOC_FAILURE;
  }
  read = fread(image_data, 1, file_sz, f);
  assert(read == read_len);
  fclose(f);

  chima_sprite* sprites = CHIMA_MALLOC(chima, header.sprite_count*sizeof(chima_sprite));
  if (!sprites) {
    CHIMA_FREE(chima, fnames);
    CHIMA_FREE(chima, fanims);
    CHIMA_FREE(chima, fsprites);
    return CHIMA_ALLOC_FAILURE;
  }
  memset(sprites, 0, header.sprite_count*sizeof(sprites[0]));
  for (size_t i = 0; i < header.sprite_count; ++i) {
    const chima_file_sprite* s = &fsprites[i];
    memcpy(sprites[i].name.data, fnames+s->name_offset, s->name_size);
    sprites[i].name.length = s->name_size;
    sprites[i].height = s->height;
    sprites[i].width = s->width;
    sprites[i].y_off = s->y_off;
    sprites[i].x_off = s->x_off;
    sprites[i].uv_x_lin = s->uv_x_con;
    sprites[i].uv_x_con = s->uv_x_lin;
    sprites[i].uv_y_con = s->uv_y_con;
    sprites[i].uv_y_lin = s->uv_y_lin;
  }

  chima_sprite_anim* anims = CHIMA_MALLOC(chima, header.anim_count*sizeof(chima_sprite_anim));
  if (!anims) {
    CHIMA_FREE(chima, sprites);
    CHIMA_FREE(chima, fnames);
    CHIMA_FREE(chima, fanims);
    CHIMA_FREE(chima, fsprites);
    return CHIMA_ALLOC_FAILURE;
  }
  memset(anims, 0, header.anim_count*sizeof(anims[0]));
  for (size_t i = 0; i < header.anim_count; ++i){
    const chima_file_anim* a = &fanims[i];
    memcpy(anims[i].name.data, fnames+a->name_offset, a->name_size);
    anims[i].name.length = a->name_size;
    anims[i].fps = a->fps;
    anims[i].sprite_idx = a->sprite_idx;
    anims[i].sprite_count = a->sprite_count;
  }

  memset(sheet, 0, sizeof(*sheet));
  sheet->sprite_count = header.sprite_count;
  sheet->sprites = sprites;
  sheet->anim_count = header.anim_count;
  sheet->anims = anims;
  sheet->asset_type = CHIMA_ASSET_SPRITESHEET;
  if (strncmp(header.image_format, "RAW", 4) == 0) {
    sheet->atlas.ctx = chima;
    sheet->atlas.data = image_data;
    sheet->atlas.width = header.image_width;
    sheet->atlas.height = header.image_height;
    sheet->atlas.channels = header.image_channels;
    sheet->atlas.depth = CHIMA_DEPTH_8U;
    sheet->atlas.anim = NULL;
    memcpy(sheet->atlas.name.data, ATLAS_NAME, sizeof(ATLAS_NAME));
    sheet->atlas.name.length = sizeof(ATLAS_NAME);
  } else {
    chima_return ret = chima_load_image_mem(chima, &sheet->atlas, chima->atlas_name.data,
                                           (uint8_t*)image_data, read_len);
    CHIMA_FREE(chima, image_data);
    if (ret != CHIMA_NO_ERROR) {
      CHIMA_FREE(chima, sprites);
      CHIMA_FREE(chima, fnames);
      CHIMA_FREE(chima, fanims);
      CHIMA_FREE(chima, fsprites);
      return ret;
    }
  }
  sheet->ctx = chima;

  CHIMA_FREE(chima, fsprites);
  CHIMA_FREE(chima, fanims);
  CHIMA_FREE(chima, fnames);

  return CHIMA_NO_ERROR;
}

chima_return chima_write_spritesheet(const chima_spritesheet* sheet,
                                     const char* path)
{
  // TODO: Support more depths
  if (sheet->atlas.depth != CHIMA_DEPTH_8U) {
    return CHIMA_INVALID_FORMAT;
  }
  if (!sheet || !path) {
    return CHIMA_INVALID_VALUE;
  }

  chima_context chima = sheet->ctx;
  size_t name_size = 0;
  size_t sprite_count = sheet->sprite_count;
  size_t anim_count = sheet->anim_count;
  for (size_t i = 0; i < sprite_count; ++i) {
    name_size += sheet->sprites[i].name.length;
  }
  for (size_t i = 0; i < anim_count; ++i){
    name_size += sheet->anims[i].name.length;
  }

  chima_sprite_file_header header;
  memset(&header, 0, sizeof(chima_sprite_file_header));
  memcpy(header.magic, CHIMA_MAGIC, sizeof(CHIMA_MAGIC));
  header.file_enum = CHIMA_ASSET_SPRITESHEET;
  header.ver_maj = CHIMA_SHEET_MAJ;
  header.ver_min = CHIMA_SHEET_MIN;
  header.sprite_count = (uint32_t)sprite_count;
  header.anim_count = (uint32_t)anim_count;
  header.image_width = sheet->atlas.width;
  header.image_height = sheet->atlas.height;
  header.image_channels = sheet->atlas.channels;
  switch (chima->atlas_format) {
    case CHIMA_FORMAT_RAW: {
      const char format[] = "RAW";
      memcpy(header.image_format, format, sizeof(format));
      break;
    }
    case CHIMA_FORMAT_PNG: {
      const char format[] = "PNG";
      memcpy(header.image_format, format, sizeof(format));
      break;
    }
    case CHIMA_FORMAT_BMP: {
      const char format[] = "BMP";
      memcpy(header.image_format, format, sizeof(format));
      break;
    }
    case CHIMA_FORMAT_TGA: {
      const char format[] = "TGA";
      memcpy(header.image_format, format, sizeof(format));
      break;  
    }
    default: CHIMA_UNREACHABLE();
  }

  size_t name_offset =
    sizeof(chima_sprite_file_header) +
    sprite_count*sizeof(chima_file_sprite) +
    anim_count*sizeof(chima_file_anim);

  chima_file_sprite* sprites = CHIMA_MALLOC(chima, sprite_count*sizeof(chima_file_sprite));
  if (!sprites) {
    return CHIMA_ALLOC_FAILURE;
  }
  memset(sprites, 0, sprite_count*sizeof(chima_file_sprite));
  size_t name_pos = 0;
  for (size_t i = 0; i < sprite_count; ++i) {
    const chima_sprite* s = &sheet->sprites[i];
    sprites[i].height = s->height;
    sprites[i].width = s->width;
    sprites[i].x_off = s->x_off;
    sprites[i].y_off = s->y_off;
    sprites[i].uv_x_lin = s->uv_x_lin;
    sprites[i].uv_x_con = s->uv_x_con;
    sprites[i].uv_y_lin = s->uv_y_lin;
    sprites[i].uv_y_con = s->uv_y_con;
    sprites[i].name_offset = name_pos;
    sprites[i].name_size = s->name.length;
    name_pos += s->name.length;
  }

  chima_file_anim* anims = CHIMA_MALLOC(chima, anim_count*sizeof(chima_file_anim));
  if (!anims) {
    CHIMA_FREE(chima, sprites);
    return CHIMA_ALLOC_FAILURE;
  }
  memset(anims, 0, anim_count*sizeof(chima_file_anim));
  for (size_t i = 0; i < anim_count; ++i) {
    const chima_sprite_anim* a = &sheet->anims[i];
    anims[i].sprite_idx = a->sprite_idx;
    anims[i].sprite_count = a->sprite_count;
    anims[i].fps = a->fps;
    anims[i].name_offset = name_pos;
    anims[i].name_size = a->name.length;
    name_pos += a->name.length;
  }

  char* name_data = CHIMA_MALLOC(chima, name_size);
  if (!name_data) {
    CHIMA_FREE(chima, sprites);
    CHIMA_FREE(chima, anims);
    return CHIMA_ALLOC_FAILURE;
  }
  memset(name_data, 0, name_size);
  {
    size_t name_off = 0;
    for (size_t i = 0; i < sheet->sprite_count; ++i) {
      chima_string* name = &sheet->sprites[i].name;
      memcpy(name_data+name_off, name->data, name->length);
      name_off += name->length;
    }
    for (size_t i = 0; i < sheet->anim_count; ++i) {
      chima_string* name = &sheet->anims[i].name;
      memcpy(name_data+name_off, name->data, name->length);
      name_off += name->length;
    }
  }
  header.name_size = name_size;
  header.name_offset = name_offset;
  header.image_offset = name_offset+name_size;

  FILE* f = fopen(path, "wb");
  if (!f) {
    CHIMA_FREE(chima, name_data);
    CHIMA_FREE(chima, sprites);
    CHIMA_FREE(chima, anims);
    return CHIMA_FILE_OPEN_FAILURE;
  }
  fwrite(&header, sizeof(header), 1, f);
  fwrite(sprites, sizeof(sprites[0]), sprite_count, f);
  fwrite(anims, sizeof(anims[0]), anim_count, f);
  fwrite(name_data, sizeof(name_data[0]), name_size, f);

  uint32_t w = sheet->atlas.width;
  uint32_t h = sheet->atlas.height;
  uint32_t ch = sheet->atlas.channels;
  void* data = sheet->atlas.data;
  switch (chima->atlas_format) {
    case CHIMA_FORMAT_RAW: {
      fwrite(data, w*h*ch, 1, f);
      break;
    }
    case CHIMA_FORMAT_PNG: {
      stbi_write_png_file(&chima->alloc, f, w, h, ch, data, w*ch);
      break;
    }
    case CHIMA_FORMAT_BMP: {
      stbi_write_bmp_file(f, w, h, ch, data);
      break;
    }
    case CHIMA_FORMAT_TGA: {
      stbi_write_tga_file(f, w, h, ch, data);
      break;
    }
    default: CHIMA_UNREACHABLE();
  }
  fclose(f);

  CHIMA_FREE(chima, name_data);
  CHIMA_FREE(chima, anims);
  CHIMA_FREE(chima, sprites);
  return CHIMA_NO_ERROR;
}
