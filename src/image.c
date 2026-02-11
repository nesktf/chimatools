#include "./internal.h"
#include "chimatools.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_NO_FAILURE_STRINGS
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_BMP
#define STBI_ONLY_GIF
#include "./stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#include "./stb/stb_image_write.h"

#define STB_RECT_PACK_IMPLEMENTATION
#define STBRP_STATIC
#include "./stb/stb_rect_pack.h"

chima_u32 chima_set_atlas_initial(chima_context chima, chima_u32 initial) {
  CHIMA_ASSERT(chima != NULL && "Invalid chima context");
  CHIMA_ASSERT(initial > 0 && "Invalid atlas initial size");
  chima_u32 old = chima->atlas_initial;
  chima->atlas_initial = initial;
  return old;
}

chima_f32 chima_set_atlas_factor(chima_context chima, chima_f32 factor) {
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

chima_result chima_gen_blank_image(chima_context chima, chima_image* image, chima_u32 width,
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
  chima_size image_size = (chima_size)(width * height * channels);
  // TODO: Fill the bitmap with color in a more efficient way?
  switch (depth) {
    case CHIMA_DEPTH_8U: {
      chima_u8* data = CHIMA_CALLOC(image_size, sizeof(chima_u8));
      if (!data) {
        return CHIMA_ALLOC_FAILURE;
      }
      const chima_u8 colors[] = {
        (chima_u8)floorf(background_color.r * 0xFF),
        (chima_u8)floorf(background_color.g * 0xFF),
        (chima_u8)floorf(background_color.b * 0xFF),
        (chima_u8)floorf(background_color.a * 0xFF),
      };
      for (chima_size pixel = 0; pixel < image_size; pixel += channels) {
        memcpy(data + pixel, colors, channels);
      }
      image->data = data;
    } break;
    case CHIMA_DEPTH_16U: {
      chima_u16* data = CHIMA_CALLOC(image_size, sizeof(chima_u16));
      if (!data) {
        return CHIMA_ALLOC_FAILURE;
      }
      const chima_u16 colors[] = {
        (chima_u16)floorf(background_color.r * 0xFFFF),
        (chima_u16)floorf(background_color.g * 0xFFFF),
        (chima_u16)floorf(background_color.b * 0xFFFF),
        (chima_u16)floorf(background_color.a * 0xFFFF),
      };
      for (chima_size pixel = 0; pixel < image_size; pixel += channels) {
        memcpy(data + pixel, colors, channels);
      }
      image->data = data;
    } break;
    case CHIMA_DEPTH_32F: {
      chima_f32* data = CHIMA_CALLOC(image_size, sizeof(chima_f32));
      if (!data) {
        return CHIMA_ALLOC_FAILURE;
      }
      for (chima_size pixel = 0; pixel < image_size; pixel += channels) {
        memcpy(data + pixel, &background_color, channels);
      }
      image->data = data;
    } break;
    default:
      return CHIMA_INVALID_VALUE;
  }
  image->extent.width = width;
  image->extent.height = height;
  image->channels = channels;
  image->depth = depth;

  return CHIMA_NO_ERROR;
}

#define ATLAS_MAX_SIZE 16384

chima_result chima_gen_atlas_image(chima_context chima, chima_image* atlas, chima_rect* sprites,
                                   chima_u32 padding, chima_color background_color,
                                   const chima_image* images, chima_size image_count) {
  if (!chima || !atlas || !sprites) {
    return CHIMA_INVALID_VALUE;
  }
  if (!images || !image_count) {
    return CHIMA_INVALID_VALUE;
  }
  const chima_image_depth depth = images[0].depth;
  const chima_u32 channels = images[0].channels;
  chima_result ret = CHIMA_NO_ERROR;

  stbrp_rect* rects = CHIMA_CALLOC(image_count, sizeof(stbrp_rect));
  if (!rects) {
    return CHIMA_ALLOC_FAILURE;
  }

  memset(rects, 0, image_count * sizeof(stbrp_rect));
  for (size_t i = 0; i < image_count; ++i) {
    if (images[i].depth != depth || images[i].channels != channels) {
      ret = CHIMA_INVALID_VALUE;
      goto free_rects;
    }
    rects[i].w = images[i].extent.width + padding;
    rects[i].h = images[i].extent.height + padding;
  }

  const chima_size node_count = image_count;
  stbrp_node* nodes = CHIMA_CALLOC(node_count, sizeof(stbrp_node));
  if (!nodes) {
    ret = CHIMA_ALLOC_FAILURE;
    goto free_rects;
  }

  chima_u32 atlas_size = chima->atlas_initial;
  while (atlas_size <= ATLAS_MAX_SIZE) {
    stbrp_context stbrp;
    stbrp_init_target(&stbrp, atlas_size, atlas_size, nodes, node_count);
    if (stbrp_pack_rects(&stbrp, rects, image_count)) {
      goto copy_texels;
    }
    atlas_size = roundf(atlas_size * chima->atlas_grow_fac);
  }
  ret = CHIMA_PACKING_FAILED;
  goto free_nodes;

copy_texels:
  ret =
    chima_gen_blank_image(chima, atlas, atlas_size, atlas_size, channels, depth, background_color);
  if (ret) {
    goto free_nodes;
  }

  for (chima_size i = 0; i < image_count; ++i) {
    sprites[i].width = images[i].extent.width;
    sprites[i].height = images[i].extent.height;
    sprites[i].x = (chima_u32)rects[i].x;
    sprites[i].y = (chima_u32)rects[i].y;
    ret = chima_composite_image(atlas, images + i, sprites[i].x, sprites[i].y);
    if (ret) {
      break; // propagate error
    }
  }

free_nodes:
  CHIMA_FREE(nodes);
free_rects:
  CHIMA_FREE(rects);
  return ret;
}

chima_result chima_load_image_file(chima_context chima, chima_image* image, chima_image_depth d,
                                   FILE* f) {
  if (!chima || !f || !image) {
    return CHIMA_INVALID_VALUE;
  }

  stbi_user_alloc al;
  al.user = chima->mem_user;
  al.malloc = chima->mem_alloc;
  al.realloc = chima->mem_realloc;
  al.free = chima->mem_free;
  void* data = NULL;
  int w, h, comp;
  int flip_y = (chima->flags & CHIMA_CTX_FLAG_FLIP_Y);
  switch (d) {
    case CHIMA_DEPTH_8U: {
      data = stbi_load_from_file(&al, f, &w, &h, &comp, 0, flip_y);
    } break;
    case CHIMA_DEPTH_16U: {
      data = stbi_load_from_file_16(&al, f, &w, &h, &comp, 0, flip_y);
    } break;
    case CHIMA_DEPTH_32F: {
      data = stbi_loadf_from_file(&al, f, &w, &h, &comp, 0, flip_y);
    } break;
    default:
      return CHIMA_INVALID_VALUE;
  }
  if (!data) {
    return CHIMA_IMAGE_PARSE_FAILURE;
  }

  CHIMA_ASSERT(w > 0 && h > 0);
  CHIMA_ASSERT(comp > 0);

  memset(image, 0, sizeof(chima_image));
  image->extent.width = (chima_u32)w;
  image->extent.height = (chima_u32)h;
  image->channels = (chima_u32)comp;
  image->data = data;
  image->depth = d;

  return CHIMA_NO_ERROR;
}

chima_result chima_load_image(chima_context chima, chima_image* image, chima_image_depth depth,
                              const char* path) {
  if (!chima || !path || !image) {
    return CHIMA_INVALID_VALUE;
  }

  FILE* f = fopen(path, "rb");
  if (!f) {
    return CHIMA_FILE_OPEN_FAILURE;
  }
  chima_result ret = chima_load_image_file(chima, image, depth, f);
  fclose(f);
  return ret;
}

chima_result chima_load_image_mem(chima_context chima, chima_image* image, chima_image_depth depth,
                                  const chima_u8* buffer, size_t buffer_len) {
  if (!chima || !buffer || !buffer_len || !image) {
    return CHIMA_INVALID_VALUE;
  }

  stbi_user_alloc al;
  al.user = chima->mem_user;
  al.malloc = chima->mem_alloc;
  al.realloc = chima->mem_realloc;
  al.free = chima->mem_free;
  void* data = NULL;
  int w, h, comp;
  int flip_y = (chima->flags & CHIMA_CTX_FLAG_FLIP_Y);

  switch (depth) {
    case CHIMA_DEPTH_8U: {
      data = stbi_load_from_memory(&al, buffer, buffer_len, &w, &h, &comp, 0, flip_y);
    } break;
    case CHIMA_DEPTH_16U: {
      data = stbi_load_16_from_memory(&al, buffer, buffer_len, &w, &h, &comp, 0, flip_y);
    } break;
    case CHIMA_DEPTH_32F: {
      data = stbi_loadf_from_memory(&al, buffer, buffer_len, &w, &h, &comp, 0, flip_y);
    } break;
    default:
      return CHIMA_INVALID_VALUE;
  }
  if (!data) {
    return CHIMA_IMAGE_PARSE_FAILURE;
  }

  CHIMA_ASSERT(w > 0 && h > 0);
  CHIMA_ASSERT(comp > 0);

  memset(image, 0, sizeof(chima_image));
  image->channels = (chima_u32)comp;
  image->extent.height = (chima_u32)h;
  image->extent.width = (chima_u32)w;
  image->data = data;
  image->depth = depth;

  return CHIMA_NO_ERROR;
}

chima_size chima__write_atlas_file(chima_context chima, FILE* f, chima_u32 w, chima_u32 h,
                                   chima_u32 ch, chima_image_format format, const void* data) {
  chima_size wrt;
  int flip_y = (chima->flags & CHIMA_CTX_FLAG_FLIP_Y);
  switch (format) {
    case CHIMA_FILE_FORMAT_RAW: {
      wrt = fwrite(data, w*h*ch, 1, f);
    } break;
    case CHIMA_FILE_FORMAT_PNG: {
      chima_size stride = w*ch;
      stbiw_user_alloc al;
      al.user = chima->mem_user;
      al.malloc = chima->mem_alloc;
      al.realloc = chima->mem_realloc;
      al.free = chima->mem_free;
      wrt = stbi_write_png_file(&al, f, w, h, ch, data, stride, flip_y, 8, -1);
    } break;
    case CHIMA_FILE_FORMAT_BMP: {
      wrt = stbi_write_bmp_file(f, w, h, ch, data, flip_y);
    } break;
    case CHIMA_FILE_FORMAT_TGA: {
      wrt = stbi_write_tga_file(f, w, h, ch, data, flip_y, 1);
    } break;
    default:
      CHIMA_UNREACHABLE();
  }
  return wrt;
}

chima_result chima_write_image_file(chima_context chima, const chima_image* image,
                                    chima_image_format format, FILE* f) {
  if (!chima || !image || !f) {
    return CHIMA_INVALID_VALUE;
  }

  int flip_y = (chima->flags & CHIMA_CTX_FLAG_FLIP_Y);
  int wrt;
  switch (format) {
    case CHIMA_FILE_FORMAT_PNG: {
      chima_size stride= image->extent.width * image->channels;
      stbiw_user_alloc al;
      al.user = chima->mem_user;
      al.malloc = chima->mem_alloc;
      al.realloc = chima->mem_realloc;
      al.free = chima->mem_free;
      wrt = stbi_write_png_file(&al, f, image->extent.width, image->extent.height, image->channels,
                           image->data, stride, flip_y, 8, -1);
      break;
    }
    case CHIMA_FILE_FORMAT_BMP: {
      wrt = stbi_write_bmp_file(f, image->extent.width, image->extent.height, image->channels,
                           image->data, flip_y);
      break;
    }
    case CHIMA_FILE_FORMAT_TGA: {
      wrt = stbi_write_tga_file(f, image->extent.width, image->extent.height, image->channels,
                           image->data, flip_y, 1);
      break;
    }
    case CHIMA_FILE_FORMAT_RAW: {
      wrt = fwrite(image->data, image->extent.width*image->extent.height*image->channels, 1, f);
    } break;
    default:
      return CHIMA_INVALID_VALUE;
  }
  return wrt ? CHIMA_NO_ERROR : CHIMA_FILE_WRITE_FAILURE;
}

chima_result chima_write_image(chima_context chima, const chima_image* image,
                               chima_image_format format, const char* path) {
  if (!chima || !image || !path) {
    return CHIMA_INVALID_VALUE;
  }

  FILE* f = fopen(path, "wb");
  if (!f) {
    return CHIMA_FILE_OPEN_FAILURE;
  }
  chima_result ret = chima_write_image_file(chima, image, format, f);
  fclose(f);
  return ret;
}

chima_result chima_composite_image(chima_image* dst, const chima_image* src, chima_u32 xpos,
                                   chima_u32 ypos) {
  if (!dst || !src) {
    return CHIMA_INVALID_VALUE;
  }
  CHIMA_ASSERT(src->depth == CHIMA_DEPTH_8U && "TODO");
  CHIMA_ASSERT(dst->depth == CHIMA_DEPTH_8U && "TODO");

  const chima_size dst_w = dst->extent.width, dst_h = dst->extent.height, dst_ch = dst->channels;
  chima_u8* dst_row_start = dst->data + (ypos * dst_w + xpos) * dst_ch;
  chima_u8* dst_end = dst->data + dst_w * dst_h * dst_ch;

  const chima_size src_w = src->extent.width, src_h = src->extent.height, src_ch = src->channels;
  const chima_size src_row_pixels = xpos + src_w > dst_w ? dst_w - xpos : src_w;
  chima_size written_rows = 0;

  while (dst_row_start < dst_end && written_rows < src_h) {
    chima_u8* src_row_start = src->data + (written_rows * src_w * src_ch);
    for (chima_size pixel = 0; pixel < src_row_pixels; ++pixel) {
      chima_u8* dst_pixel = dst_row_start + (pixel * dst_ch);
      chima_u8* src_pixel = src_row_start + (pixel * src_ch);

      chima_f32 src_alpha = src_ch == 4 ? (chima_f32)(src_pixel[3]) / 256.f : 1.f;
      chima_f32 dst_alpha = dst_ch == 4 ? (chima_f32)(dst_pixel[3]) / 256.f : 1.f;
      chima_f32 comp_alpha = src_alpha + dst_alpha * (1.f - src_alpha);
      for (chima_u32 i = 0; i < dst_ch; ++i) {
        chima_f32 dst_blend =
          dst_pixel[i] * dst_alpha * (1.f - src_alpha) + src_pixel[i] * src_alpha;
        dst_pixel[i] = (chima_u8)roundf(dst_blend / comp_alpha);
      }
    }
    ++written_rows;
    dst_row_start += dst_w * dst_ch;
  }

  return CHIMA_NO_ERROR;
}

void chima_destroy_image(chima_context chima, chima_image* image) {
  if (!image) {
    return;
  }
  CHIMA_FREE(image->data); // stbi_image_free just calls alloc->free()
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

chima_result chima_load_image_anim_file(chima_context chima, chima_image_anim* anim, FILE* f) {
  if (!chima || !anim || !f) {
    return CHIMA_INVALID_VALUE;
  }

  stbi_user_alloc al;
  al.user = chima->mem_user;
  al.malloc = chima->mem_alloc;
  al.realloc = chima->mem_realloc;
  al.free = chima->mem_free;

  stbi__context stbi;
  stbi__start_file(&stbi, f);
  stbi.al = &al;

  stbi__result_info ri;
  memset(&ri, 0, sizeof(ri));
  ri.bits_per_channel = 8;
  ri.channel_order = STBI_ORDER_RGB;
  ri.num_channels = 0;

  if (!stbi__gif_test(&stbi)) {
    return CHIMA_UNSUPPORTED_FORMAT;
  }

  int comp = 0;

  stbi__gif gif;
  memset(&gif, 0, sizeof(gif));
  void* data = NULL;
  uint8_t* two_back = NULL;
  chima_result ret = CHIMA_NO_ERROR;

  // https://gist.github.com/urraka/685d9a6340b26b830d49
  gif_node head;
  memset(&head, 0, sizeof(head));
  chima_u32 image_count = 0;
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
      gif_node* node = CHIMA_MALLOC(sizeof(gif_node));
      if (!node) {
        ret = CHIMA_ALLOC_FAILURE;
        free_images = CHIMA_TRUE;
        goto free_nodes;
      }
      memset(node, 0, sizeof(gif_node));
      curr_node->next = node;
      curr_node = node;
    }

    chima_size image_sz = comp * gif.w * gif.h;
    curr_node->data = CHIMA_MALLOC(image_sz);
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
  CHIMA_FREE(gif.out);
  CHIMA_FREE(gif.history);
  CHIMA_FREE(gif.background);

  chima_image* images = CHIMA_CALLOC(image_count, sizeof(chima_image));
  if (!images) {
    ret = CHIMA_ALLOC_FAILURE;
    free_images = CHIMA_TRUE;
    goto free_nodes;
  }
  chima_u32* frametimes = CHIMA_CALLOC(image_count, sizeof(chima_u32));
  if (!frametimes) {
    ret = CHIMA_ALLOC_FAILURE;
    free_images = CHIMA_TRUE;
    CHIMA_FREE(images);
    goto free_nodes;
  }

  memset(images, 0, image_count * sizeof(chima_image));
  curr_node = &head;
  size_t i = 0;
  while (curr_node) {
    images[i].data = curr_node->data;
    images[i].extent.height = curr_node->height;
    images[i].extent.width = curr_node->width;
    images[i].channels = curr_node->channels;
    images[i].depth = CHIMA_DEPTH_8U;
    frametimes[i] = (chima_u32)curr_node->delay;
    curr_node = curr_node->next;
    ++i;
  }

  memset(anim, 0, sizeof(chima_image_anim));
  anim->images = images;
  anim->frametimes = frametimes;
  anim->image_count = image_count;

free_nodes:
  curr_node = head.next;
  while (curr_node) {
    gif_node* node = curr_node;
    curr_node = node->next;
    if (node->data && free_images) {
      CHIMA_FREE(node->data);
    }
    CHIMA_FREE(node);
  }
  if (free_images) {
    CHIMA_FREE(head.data);
  }

  return ret;
}

chima_result chima_load_image_anim(chima_context chima, chima_image_anim* anim, const char* path) {
  if (!chima || !path || !anim) {
    return CHIMA_INVALID_VALUE;
  }

  FILE* f = fopen(path, "rb");
  if (!f) {
    return CHIMA_FILE_OPEN_FAILURE;
  }
  chima_result ret = chima_load_image_anim_file(chima, anim, f);
  fclose(f);
  return ret;
}

void chima_destroy_image_anim(chima_context chima, chima_image_anim* anim) {
  if (!anim) {
    return;
  }
  for (size_t i = 0; i < anim->image_count; ++i) {
    CHIMA_FREE(anim->images[i].data);
  }
  CHIMA_FREE(anim->images);
  CHIMA_FREE(anim->frametimes);
  memset(anim, 0, sizeof(chima_image_anim));
}
