#include "./internal.h"

#include "./stb/stb_image.h"
#include "./stb/stb_image_write.h"

chima_u32 chima_set_atlas_initial(chima_context chima,
                                            chima_u32 initial) {
  CHIMA_ASSERT(chima != NULL && "Invalid chima context");
  CHIMA_ASSERT(initial > 0 && "Invalid atlas initial size");
  chima_u32 old = chima->atlas_initial;
  chima->atlas_initial = initial;
  return old;
}

chima_f32 chima_set_atlas_factor(chima_context chima,
                                           chima_f32 factor) {
  CHIMA_ASSERT(chima != NULL && "Invalid chima context");
  CHIMA_ASSERT(factor >= 1.0f && "Invalid atlas grow factor");
  chima_f32 old = chima->atlas_grow_fac;
  chima->atlas_grow_fac = factor;
  return old;
}

chima_bool chima_set_flip_y(chima_context chima, chima_bool flip_y) {
  CHIMA_ASSERT(chima != NULL && "Invalid chima context");
  chima_bool old = chima->flags & CHIMA_CTX_FLAG_FLIP_Y;
  chima->flags = CHIMA_SET_FLAG(flip_y, chima->flags, CHIMA_CTX_FLAG_FLIP_Y);
  return old;
}

/*
static void rename_image(const char* name, chima_image* image, size_t fmt_count) {
  if (name) {
    image->name.length = strlen(name);
    memcpy(image->name.data, name, image->name.length+1); // copy null terminator
  } else {
    int len = snprintf(image->name.data, CHIMA_STRING_MAX_SIZE,
                       "chima_image%lu", fmt_count);
    CHIMA_ASSERT(len > 0);
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
    CHIMA_ASSERT(len > 0);
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
    CHIMA_ASSERT(len > 0);
    image->name.length = (size_t)len;
  }
}
*/

chima_result chima_create_image(chima_context chima, chima_image* image, chima_u32 width,
                                chima_u32 height, chima_u32 channels, chima_image_depth depth,
                                chima_color background_color) {
  if (!chima || !image || !width || !height) {
    return CHIMA_INVALID_VALUE;
  }

  channels = CHIMA_CLAMP(channels, 1, 4);
  background_color.r = CHIMA_CLAMP(background_color.r, 0.f, 1.f);
  background_color.g = CHIMA_CLAMP(background_color.g, 0.f, 1.f);
  background_color.b = CHIMA_CLAMP(background_color.b, 0.f, 1.f);
  background_color.a = CHIMA_CLAMP(background_color.a, 0.f, 1.f);

  memset(image, 0, sizeof(*image));
  chima_size image_size = (chima_size)(width*height*channels);
  // TODO: Fill the bitmap with color in a more efficient way?
  switch (depth) {
    case CHIMA_DEPTH_8U: {
      chima_u8* data = CHIMA_CALLOC(image_size, sizeof(chima_u8));
      if (!data) {
        return CHIMA_ALLOC_FAILURE;
      }
      const chima_u8 colors[] = {
        (chima_u8)floorf(background_color.r*0xFF),
        (chima_u8)floorf(background_color.g*0xFF),
        (chima_u8)floorf(background_color.b*0xFF),
        (chima_u8)floorf(background_color.a*0xFF),
      };
      for (chima_size pixel = 0; pixel < image_size; pixel += channels) {
        memcpy(data+pixel, colors, channels);
      }
      image->data = data;
    } break;
    case CHIMA_DEPTH_16U: {
      chima_u16* data = CHIMA_CALLOC(image_size, sizeof(chima_u16));
      if (!data) {
        return CHIMA_ALLOC_FAILURE;
      }
      const chima_u16 colors[] = {
        (chima_u16)floorf(background_color.r*0xFFFF),
        (chima_u16)floorf(background_color.g*0xFFFF),
        (chima_u16)floorf(background_color.b*0xFFFF),
        (chima_u16)floorf(background_color.a*0xFFFF),
      };
      for (chima_size pixel = 0; pixel < image_size; pixel += channels) {
        memcpy(data+pixel, colors, channels);
      }
      image->data = data;
    } break;
    case CHIMA_DEPTH_32F: {
      chima_f32* data = CHIMA_CALLOC(image_size, sizeof(chima_f32));
      if (!data) {
        return CHIMA_ALLOC_FAILURE;
      }
      for (chima_size pixel = 0; pixel < image_size; pixel += channels) {
        memcpy(data+pixel, &background_color, channels);
      }
      image->data = data;
    } break;
    default:
      return CHIMA_INVALID_VALUE;
  }
  image->extent.width= width;
  image->extent.height = height;
  image->channels = channels;
  image->depth = depth;

  return CHIMA_NO_ERROR;
}

/*
static size_t align_fw_adjust(void* ptr, size_t align) {
  uintptr_t iptr = (uintptr_t)ptr;
  // return ((iptr - 1u + align) & -align) - iptr;
  return align - (iptr & (align - 1u));
}
*/

chima_result chima_create_atlas_image(chima_context chima, chima_image* atlas, chima_rect* sprites,
                                      chima_u32 padding, chima_color background_color,
                                      const chima_image* images, chima_size image_count) {
  if (!chima || !atlas || !sprites) {
    return CHIMA_INVALID_VALUE;
  }
  if (!images || !image_count) {
    return CHIMA_INVALID_VALUE;
  }

  chima_result res;
  chima_packed_rect* rects = CHIMA_CALLOC(image_count, sizeof(chima_packed_rect));
  if (!rects) {
    return CHIMA_ALLOC_FAILURE;
  }
  memset(rects, 0, image_count*sizeof(*rects));
  const chima_u32 channels = images[0].channels;
  const chima_image_depth depth = images[0].depth;
  for (chima_size i = 0; i < image_count; ++i) {
    if (images[i].channels != channels || images[i].depth != depth) {
      res = CHIMA_INVALID_VALUE;
      goto free_rects;
    }
    rects[i].id = (chima_u32)i;
    rects[i].rect.width = images[i].extent.width+padding;
    rects[i].rect.height = images[i].extent.height+padding;
  }

  const chima_u32 atlas_size = chima->atlas_initial;
  res = chima_pack_rects(chima, atlas_size, atlas_size, rects, image_count);
  if (!res) {
    goto free_rects;
  }

  res = chima_create_image(chima, atlas, atlas_size, atlas_size, channels, depth,
                           background_color);
  if (!res) {
    goto free_rects;
  }

  memset(sprites, 0, image_count*sizeof(*sprites));
  for (chima_size i = 0; i < image_count; ++i) {
    CHIMA_ASSERT(rects[i].was_packed == CHIMA_TRUE);
    sprites[i] = rects[i].rect;
    res = chima_composite_image(atlas, images+i, sprites[i].x, sprites[i].y);
  }

free_rects:
  memset(rects, 0, image_count*sizeof(rects));
  CHIMA_FREE(rects);
  return res;
}
/*
  // Evil hack: use the sprite array as stbrp_node and stbrp_rect storage
  // Remove this when the scratch arena is implemented (?)
  //
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

  chima_bool flip_x = (chima->ctx_flags & CHIMA_FLAG_UV_X_FLIP);
  chima_bool flip_y = (chima->ctx_flags & CHIMA_FLAG_UV_Y_FLIP);

  for (size_t i = 0; i < image_count; ++i) {
    // Writting directly to the chima_sprites **should** be fine. Will need some tests
    // Copy in case we overlap with the stbrp_rect data in the last chima_sprite
    uint32_t x = rects[i].x;
    uint32_t y = rects[i].y;

    uint32_t w = images[i]->width;
    uint32_t h = images[i]->height;

    sprites[i].x_off = x;
    sprites[i].y_off = y;
    sprites[i].width = w;
    sprites[i].height = h;
    memset(sprites[i].name.data, 0, CHIMA_STRING_MAX_SIZE);
    memcpy(sprites[i].name.data, images[i]->name.data, images[i]->name.length);
    sprites[i].name.length = images[i]->name.length;

    chima_composite_image(atlas, images[i], x, y);
  }

  return CHIMA_NO_ERROR;
*/


    /*
     * We consider the top left as the (0,0) coordinate
     * for the atlas image, the sprite offsets and the sprite UVs.
     *
     * (0,0);(0,0)                                        (sz,0);(1,0)
     * +-------------------------------------------------------------+
     * |  (x,y);(x/sz,y/sz)                 (x+w,y);((x+w)/sz,y/sz)  |
     * |                      +----------+                           |
     * |                      |          |                           |
     * |                      |          |                           |
     * |                      +----------+                           |
     * |  (x,y+h);(x/sz,(y+h)/sz)     (x+w,y+h);((x+w)/sz,(y+h)/sz)  |
     * +-------------------------------------------------------------+
     * (0,sz);(0,1)                                      (sz,sz);(1,1)
    float uv_scale_x = (flip_x*-1.f + !flip_x*1.f)*(float)w/(float)atlas_size;
    float uv_off_x = (float)x/(float)atlas_size;
    sprites[i].uv_x_lin = uv_scale_x;
    sprites[i].uv_x_con = flip_x*(1.f-uv_off_x) + !flip_x*uv_off_x;

    float uv_scale_y = (flip_y*-1.f + !flip_y*1.f)*(float)h/(float)atlas_size;
    float uv_off_y = (float)y/(float)atlas_size;
    sprites[i].uv_y_lin = uv_scale_y;
    sprites[i].uv_y_con = flip_y*(1.f-uv_off_y) + !flip_y*uv_off_y;
    */

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

chima_result chima_load_image(chima_context chima, chima_image* image,
                                        chima_image_depth depth,
                                        const char* path) {
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

  int flip_y = (chima->ctx_flags & CHIMA_FLAG_Y_FLIP);
  chima_return ret = load_texels(&stbi, &ri, &result, &w, &h, &comp, req_comp, flip_y);
  fclose(f);
  if (ret != CHIMA_NO_ERROR) {
    return ret;
  }

  CHIMA_ASSERT(w > 0 && h > 0);
  CHIMA_ASSERT(comp > 0);
  CHIMA_ASSERT(result);

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

chima_result chima_load_image_mem(chima_context chima,
                                            chima_image* image,
                                            chima_image_depth depth,
                                            const chima_u8* buffer,
                                            size_t buffer_len) {
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

  int flip_y = (chima->ctx_flags & CHIMA_FLAG_Y_FLIP);
  chima_return ret = load_texels(&stbi, &ri, &result, &w, &h, &comp, req_comp, flip_y);
  if (ret != CHIMA_NO_ERROR) {
    return ret;
  }

  CHIMA_ASSERT(result);
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

chima_result chima_write_image(const chima_image* image,
                                         const char* path,
                                         chima_image_format format) {
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

chima_result chima_composite_image(chima_image* dst,
                                             const chima_image* src,
                                             chima_u32 xpos, chima_u32 ypos) {
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

void chima_destroy_image(chima_context chima, chima_image* image) {
  if (!image) {
    return;
  }
  CHIMA_FREE(image->ctx, image->data);
  memset(image, 0, sizeof(chima_image));
}

typedef struct gif_node {
  uint32_t width;
  uint32_t height;
  uint32_t channels;
  int delay;
  void* data;
  struct gif_node* next;
} gif_node;

chima_result chima_load_image_anim(chima_context chima,
                                             chima_image_anim* anim,
                                             const char* path) {
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
    CHIMA_ASSERT(comp);
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

void chima_destroy_anim(chima_context chima, chima_image_anim* anim) {
  if (!anim) {
    return;
  }
  for (size_t i = 0; i < anim->image_count; ++i) {
    CHIMA_FREE(anim->ctx, anim->images[i].data);
  }
  CHIMA_FREE(anim->ctx, anim->images);
  memset(anim, 0, sizeof(chima_anim));
}
