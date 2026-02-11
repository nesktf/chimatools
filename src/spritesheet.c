#include "./internal.h"

#include <stdalign.h>
#include <string.h>

typedef struct sheet_image_node {
  struct sheet_image_node* next;
  const chima_image* image;
  chima_u32 frametime;
  chima_string name;
} sheet_image_node;

typedef struct sheet_anim_node {
  struct sheet_anim_node* next;
  const chima_image_anim* anim;
  chima_string name;
} sheet_anim_node;

typedef struct chima_sheet_data_ {
  chima_context chima;
  sheet_image_node* image_head;
  chima_size image_count;
  sheet_anim_node* anim_head;
  chima_size anim_count;
} chima_sheet_data_;

chima_result chima_create_sheet_data(chima_context chima,
                                     chima_sheet_data* data) {
  if (!chima) {
    return CHIMA_INVALID_VALUE;
  }

  chima_sheet_data_* sheet = CHIMA_MALLOC(sizeof(chima_sheet_data_));
  if (!sheet) {
    return CHIMA_ALLOC_FAILURE;
  }
  memset(sheet, 0, sizeof(*sheet));
  sheet->chima = chima;
  (*data) = sheet;

  return CHIMA_NO_ERROR;
}

static void format_indexed_image_name(chima_string* dst, const char* data, chima_size len,
                                      chima_size image_idx) {
  memset(dst->data, 0, CHIMA_STRING_MAX_SIZE);
  const chima_size suffix_size = 6;
  const size_t max_src_len = CHIMA_STRING_MAX_SIZE - suffix_size - 1; // Include NULL terminator
  const size_t src_len = (len < max_src_len) ? len : max_src_len;
  int wrt = snprintf(dst->data, CHIMA_STRING_MAX_SIZE, "%.*s.%05zu",
                     (int)src_len, data, image_idx);
  CHIMA_ASSERT(wrt >= 0);
  dst->len = (chima_size)wrt;
}

static chima_result do_sheet_add_images(chima_sheet_data data,
                                        const chima_image* images,
                                        const chima_u32* frametimes,
                                        chima_size image_count,
                                        const char* name, chima_size name_len) {
  if (!data || !images || !image_count) {
    return CHIMA_INVALID_VALUE;
  }
  CHIMA_ASSERT(data->chima);
  chima_context chima = data->chima;

  sheet_image_node* nodes = CHIMA_CALLOC(image_count, sizeof(sheet_image_node));
  if (!nodes) {
    return CHIMA_ALLOC_FAILURE;
  }
  memset(nodes, 0, image_count * sizeof(*nodes));

  name_len = (name_len < CHIMA_STRING_MAX_SIZE - 1) ? name_len : CHIMA_STRING_MAX_SIZE - 1;
  for (chima_size i = 0; i < image_count; ++i) {
    nodes[i].next = nodes + i + 1;
    nodes[i].image = images+i;
    nodes[i].frametime = frametimes ? frametimes[i] : 1;
    if (!name) {
      int wrt = sprintf(nodes[i].name.data, "chima_image.%05zu", data->image_count + i);
      CHIMA_ASSERT(wrt);
      nodes[i].name.len = (chima_size)wrt;
    } else if (image_count > 1){
      format_indexed_image_name(&nodes[i].name, name, name_len, i);
    } else {
      memcpy(nodes[i].name.data, name, name_len);
      nodes[i].name.len = name_len;
    }
  }
  nodes[image_count - 1].next = data->image_head;

  data->image_head = nodes;
  data->image_count += image_count;
  return CHIMA_NO_ERROR;
}

static chima_result do_sheet_add_anim(chima_sheet_data data,
                                      const chima_image_anim* anim,
                                      const char* name, chima_size name_len) {
  if (!data || !anim) {
    return CHIMA_INVALID_VALUE;
  }
  chima_context chima = data->chima;
  CHIMA_ASSERT(chima);

  sheet_anim_node* node = CHIMA_MALLOC(sizeof(sheet_anim_node));
  if (!node) {
    return CHIMA_ALLOC_FAILURE;
  }
  memset(node, 0, sizeof(*node));

  node->anim = anim;
  node->next = data->anim_head;
  data->anim_head = node;
  if (!name) {
    int wrt = snprintf(node->name.data, CHIMA_STRING_MAX_SIZE, "chima_anim.%05zu",
                       data->anim_count);
    CHIMA_ASSERT(wrt);
    node->name.len = (chima_size)wrt;
  } else {
    name_len = (name_len < CHIMA_STRING_MAX_SIZE-1) ? name_len : CHIMA_STRING_MAX_SIZE-1;
    strncpy(node->name.data, name, name_len);
    node->name.len = name_len;
  }
  ++data->anim_count;

  return CHIMA_NO_ERROR;
}

chima_result chima_sheet_add_image(chima_sheet_data data,
                                   const chima_image* image, const char* name) {
  return do_sheet_add_images(data, image, NULL, 1, name,
                             name ? strlen(name) : 0);
}

chima_result chima_sheet_add_images(chima_sheet_data data,
                                    const chima_image* images,
                                    const chima_u32* frametimes,
                                    chima_size count, const char* basename) {
  return do_sheet_add_images(data, images, frametimes, count, basename,
                             basename ? strlen(basename) : 0);
}

chima_result chima_sheet_add_image_sv(chima_sheet_data data,
                                      const chima_image* image,
                                      chima_string_view name) {
  return do_sheet_add_images(data, image, NULL, 1, name.data,
                             name.data ? name.len : 0);
}

chima_result chima_sheet_add_images_sv(chima_sheet_data data,
                                       const chima_image* images,
                                       const chima_u32* frametimes,
                                       chima_size count,
                                       chima_string_view basename) {
  return do_sheet_add_images(data, images, frametimes, count, basename.data,
                             basename.data ? basename.len : 0);
}

chima_result chima_sheet_add_image_anim(chima_sheet_data data,
                                        const chima_image_anim* anim,
                                        const char* basename) {
  return do_sheet_add_anim(data, anim, basename,
                           basename ? strlen(basename) : 0);
}

chima_result chima_sheet_add_image_anim_sv(chima_sheet_data data,
                                           const chima_image_anim* anim,
                                           chima_string_view basename) {
  return do_sheet_add_anim(data, anim, basename.data,
                           basename.data ? basename.len : 0);
}

void chima_destroy_sheet_data(chima_sheet_data data) {
  if (!data) {
    return;
  }

  chima_context chima = data->chima;
  CHIMA_ASSERT(chima);
  sheet_image_node* image_head = data->image_head;
  while (image_head) {
    sheet_image_node* next = image_head->next;
    memset(image_head, 0, sizeof(*image_head));
    CHIMA_FREE(image_head);
    image_head = next;
  }
  sheet_anim_node* anim_head = data->anim_head;
  while (anim_head) {
    sheet_anim_node* next = anim_head->next;
    memset(anim_head, 0, sizeof(*anim_head));
    CHIMA_FREE(anim_head);
    anim_head = next;
  }
  memset(data, 0, sizeof(*data));
  CHIMA_FREE(data);
}

chima_result chima_gen_spritesheet(chima_context chima, chima_spritesheet* sheet,
                                             chima_sheet_data data, chima_u32 padding,
                                             chima_color background_color) {
  if (!chima || !sheet || !data) {
    return CHIMA_INVALID_VALUE;
  }

  chima_size total_images = data->image_count;
  {
    // Count the animation images to allocate the temporary rectangles and images
    sheet_anim_node* node = data->anim_head;
    while (node) {
      total_images += node->anim->image_count;
      node = node->next;
    }
  }
  if (!total_images) {
    return CHIMA_INVALID_VALUE;
  }

  chima_result ret = CHIMA_NO_ERROR;
  chima_image* images = CHIMA_CALLOC(total_images, sizeof(chima_image));
  if (!images) {
    return CHIMA_ALLOC_FAILURE;
  }
  chima_rect* rects = CHIMA_CALLOC(total_images, sizeof(chima_rect));
  if (!rects) {
    ret = CHIMA_ALLOC_FAILURE;
    goto free_sheet_temp_images;
  }

  chima_sprite* sprites = CHIMA_CALLOC(total_images, sizeof(chima_sprite));
  if (!sprites) {
    ret = CHIMA_ALLOC_FAILURE;
    goto sheet_exit;
  }
  chima_sprite_anim* anims = CHIMA_CALLOC(data->anim_count, sizeof(chima_sprite_anim));
  if (!anims) {
    ret = CHIMA_ALLOC_FAILURE;
    goto free_sheet_sprites;
  }
  memset(sprites, 0, total_images*sizeof(sprites[0]));
  memset(anims, 0, data->anim_count*sizeof(anims[0]));

  // First pass, we copy all the images in a temporary buffer.
  {
    // We also copy the frametime and name for each image.
    chima_size image_idx = 0;
    sheet_image_node* image_node = data->image_head;
    while (image_node) {
      memcpy(images+image_idx, image_node->image, sizeof(images[0]));
      // Assume the name is already in a copyable format
      memcpy(sprites[image_idx].name.data, image_node->name.data, image_node->name.len);
      sprites[image_idx].name.len = image_node->name.len;
      sprites[image_idx].frametime = image_node->frametime;
      ++image_idx;
      image_node = image_node->next;
    }

    // For animations, we copy the animation name, generate a name for each
    // animation image, and set the appropiate indices
    chima_size anim_idx = 0;
    sheet_anim_node* anim_node = data->anim_head;
    const chima_size anim_image_count = anim_node->anim->image_count;
    while (anim_node) {
      memcpy(images+image_idx, anim_node->anim->images, anim_image_count*sizeof(images[0]));
      for (chima_size i = 0; i < anim_image_count; ++i) {
        // For animations, we generate a name using the current image index
        format_indexed_image_name(&sprites[image_idx+i].name, anim_node->name.data,
                                  anim_node->name.len, i);
        sprites[image_idx+i].frametime = anim_node->anim->frametimes[i];
      }
      memcpy(anims[anim_idx].name.data, anim_node->name.data, anim_node->name.len);
      anims[anim_idx].name.len = anim_node->name.len;
      anims[anim_idx].sprite_start = image_idx;
      anims[anim_idx].sprite_count = anim_image_count;
      image_idx += anim_image_count; 
      ++anim_idx;
      anim_node = anim_node->next;
    }
  }

  memset(sheet, 0, sizeof(*sheet));
  // Once we have our images copied to a buffer, we generate an atlas
  ret = chima_gen_atlas_image(chima, &sheet->atlas, rects, padding, background_color, images,
                              total_images);
  if (ret) {
    goto free_sheet_anims;
  }

  // Now we copy all the rectangles
  for (chima_size i = 0; i < total_images; ++i) {
    memcpy(&sprites[i].rect, rects+i, sizeof(rects[0]));
  }

  sheet->sprite_count = total_images;
  sheet->anim_count = data->anim_count;
  sheet->sprites = sprites;
  sheet->anims = anims;
  goto sheet_exit;

free_sheet_anims:
  CHIMA_FREE(anims);
free_sheet_sprites:
  CHIMA_FREE(sprites);
sheet_exit:
  CHIMA_FREE(rects);
free_sheet_temp_images:
  CHIMA_FREE(images);
  return ret;
}

#define CHIMA_FORMAT_MAX_SIZE 7
#define CHIMA_SHEET_MAJ       1
#define CHIMA_SHEET_MIN       0

typedef struct chima_sprite_file_header {
  chima_u8 magic[sizeof(CHIMA_MAGIC)];
  chima_u16 file_enum;
  chima_u8 ver_maj;
  chima_u8 ver_min;
  chima_u32 sprite_count;
  chima_u32 anim_count;
  chima_u32 name_size;
  chima_u32 name_offset;
  chima_u32 image_width;
  chima_u32 image_height;
  chima_u32 image_offset;
  chima_u8 image_channels;
  chima_u8 image_depth;
  char image_format[7]; // Up to 6 chars + null terminator
} chima_sprite_file_header;

typedef struct chima_file_sprite {
  chima_u32 width;
  chima_u32 height;
  chima_u32 x_off;
  chima_u32 y_off;
  chima_u32 frametime;
  chima_u32 name_offset;
  chima_u32 name_size;
} chima_file_sprite;

typedef struct chima_file_anim {
  chima_u32 sprite_idx;
  chima_u32 sprite_count;
  chima_u32 name_offset;
  chima_u32 name_size;
} chima_file_anim;


chima_result chima_load_spritesheet_file(chima_context chima, chima_spritesheet* sheet,
                                                   FILE* f) {
  if (!chima || !sheet || !f) {
    return CHIMA_INVALID_VALUE;
  }

  fseek(f, 0, SEEK_END);
  size_t file_sz = ftell(f);
  rewind(f);

  chima_sprite_file_header header;
  memset(&header, 0, sizeof(header));
  int read = fread(&header, sizeof(header), 1, f);
  if (read < 1) {
    return CHIMA_INVALID_FILE_FORMAT;
  }
  if (strncmp((char*)header.magic, CHIMA_MAGIC, sizeof(CHIMA_MAGIC))) {
    return CHIMA_INVALID_FILE_FORMAT;
  }
  if (header.file_enum != ASSET_TYPE_SPRITESHEET) {
    return CHIMA_INVALID_FILE_FORMAT;
  }
  if (header.ver_maj != CHIMA_SHEET_MAJ) {
    return CHIMA_INVALID_FILE_FORMAT;
  }
  if (header.ver_min != CHIMA_SHEET_MIN) {
    return CHIMA_INVALID_FILE_FORMAT;
  }

  // Assume everything else is fine...
  size_t read_offset = sizeof(header);
  size_t read_len = header.sprite_count * sizeof(chima_file_sprite);
  chima_file_sprite* fsprites = CHIMA_MALLOC(read_len);
  if (!fsprites) {
    fclose(f);
    return CHIMA_ALLOC_FAILURE;
  }
  read = fread(fsprites, sizeof(fsprites[0]), header.sprite_count, f);
  if ((chima_u32)read != header.sprite_count) {
    CHIMA_FREE(fsprites);
    fclose(f);
    return CHIMA_FILE_EOF;
  }
  read_offset += read_len;

  read_len = header.anim_count * sizeof(chima_file_anim);
  chima_file_anim* fanims = CHIMA_MALLOC(read_len);
  if (!fanims) {
    CHIMA_FREE(fsprites);
    fclose(f);
    return CHIMA_ALLOC_FAILURE;
  }
  read = fread(fanims, sizeof(fanims[0]), header.anim_count, f);
  if ((chima_u32)read != header.anim_count) {
    CHIMA_FREE(fanims);
    CHIMA_FREE(fsprites);
    fclose(f);
    return CHIMA_FILE_EOF;
  }
  read_offset += read_len;

  read_len = header.name_size;
  char* fnames = CHIMA_MALLOC(read_len);
  if (!fnames) {
    CHIMA_FREE(fanims);
    CHIMA_FREE(fsprites);
    fclose(f);
    return CHIMA_ALLOC_FAILURE;
  }
  read = fread(fnames, sizeof(fnames[0]), header.name_size, f);
  if ((chima_u32)read != read_len) {
    CHIMA_FREE(fnames);
    CHIMA_FREE(fanims);
    CHIMA_FREE(fsprites);
    fclose(f);
    return CHIMA_FILE_EOF;
  }
  read_offset += read_len;

  read_len = file_sz - read_offset;
  void* image_data = CHIMA_MALLOC(read_len);
  if (!image_data) {
    CHIMA_FREE(fnames);
    CHIMA_FREE(fanims);
    CHIMA_FREE(fsprites);
    fclose(f);
    return CHIMA_ALLOC_FAILURE;
  }
  read = fread(image_data, 1, file_sz, f);
  CHIMA_ASSERT((chima_u32)read == read_len);
  fclose(f);

  chima_sprite* sprites =
    CHIMA_MALLOC(header.sprite_count * sizeof(chima_sprite));
  if (!sprites) {
    CHIMA_FREE(fnames);
    CHIMA_FREE(fanims);
    CHIMA_FREE(fsprites);
    return CHIMA_ALLOC_FAILURE;
  }
  memset(sprites, 0, header.sprite_count * sizeof(sprites[0]));
  for (size_t i = 0; i < header.sprite_count; ++i) {
    const chima_file_sprite* s = &fsprites[i];
    memcpy(sprites[i].name.data, fnames + s->name_offset, s->name_size);
    sprites[i].name.len = s->name_size;
    sprites[i].rect.height = s->height;
    sprites[i].rect.width = s->width;
    sprites[i].rect.y = s->y_off;
    sprites[i].rect.x = s->x_off;
    sprites[i].frametime = s->frametime;
  }

  chima_sprite_anim* anims =
    CHIMA_MALLOC(header.anim_count * sizeof(chima_sprite_anim));
  if (!anims) {
    CHIMA_FREE(sprites);
    CHIMA_FREE(fnames);
    CHIMA_FREE(fanims);
    CHIMA_FREE(fsprites);
    return CHIMA_ALLOC_FAILURE;
  }
  memset(anims, 0, header.anim_count * sizeof(anims[0]));
  for (size_t i = 0; i < header.anim_count; ++i) {
    const chima_file_anim* a = &fanims[i];
    memcpy(anims[i].name.data, fnames + a->name_offset, a->name_size);
    anims[i].name.len = a->name_size;
    anims[i].sprite_start = a->sprite_idx;
    anims[i].sprite_count = a->sprite_count;
  }

  memset(sheet, 0, sizeof(*sheet));
  sheet->sprite_count = header.sprite_count;
  sheet->sprites = sprites;
  sheet->anim_count = header.anim_count;
  sheet->anims = anims;
  chima_image_depth depth = (chima_image_depth)header.image_depth;
  if (strncmp(header.image_format, "RAW", 4) == 0) {
    sheet->atlas.data = image_data;
    sheet->atlas.extent.width = header.image_width;
    sheet->atlas.extent.height = header.image_height;
    sheet->atlas.channels = header.image_channels;
    sheet->atlas.depth = depth;
  } else {
    chima_result ret =
      chima_load_image_mem(chima, &sheet->atlas, depth, (chima_u8*)image_data, read_len);
    CHIMA_FREE(image_data);
    if (ret != CHIMA_NO_ERROR) {
      CHIMA_FREE(sprites);
      CHIMA_FREE(fnames);
      CHIMA_FREE(fanims);
      CHIMA_FREE(fsprites);
      return ret;
    }
  }

  CHIMA_FREE(fsprites);
  CHIMA_FREE(fanims);
  CHIMA_FREE(fnames);

  return CHIMA_NO_ERROR;
}

chima_result chima_load_spritesheet_mem(chima_context chima,
                                        chima_spritesheet* sheet,
                                        const chima_u8* buffer,
                                        chima_size buffer_len) {
  CHIMA_UNUSED(chima);
  CHIMA_UNUSED(sheet);
  CHIMA_UNUSED(buffer);
  CHIMA_UNUSED(buffer_len);
  CHIMA_TODO();
}

chima_result chima_load_spritesheet(chima_context chima,
                                    chima_spritesheet* sheet,
                                    const char* path) {
  if (!path) {
    return CHIMA_INVALID_VALUE;
  }

  FILE* f = fopen(path, "rb");
  if (!f) {
    return CHIMA_FILE_OPEN_FAILURE;
  }
  chima_result ret = chima_load_spritesheet_file(chima, sheet, f);
  fclose(f);
  return ret;
}

chima_result chima_write_spritesheet(chima_context chima, const chima_spritesheet* sheet,
                                     const char* path,
                                     chima_image_format format) {
  if (sheet->atlas.depth != CHIMA_DEPTH_8U) {
    format = CHIMA_FILE_FORMAT_RAW; // for now, we only support writting non u8 depths as RAW bytes
  }
  if (!sheet || !path) {
    return CHIMA_INVALID_VALUE;
  }

  size_t name_size = 0;
  size_t sprite_count = sheet->sprite_count;
  size_t anim_count = sheet->anim_count;
  for (size_t i = 0; i < sprite_count; ++i) {
    name_size += sheet->sprites[i].name.len;
  }
  for (size_t i = 0; i < anim_count; ++i) {
    name_size += sheet->anims[i].name.len;
  }

  chima_sprite_file_header header;
  memset(&header, 0, sizeof(chima_sprite_file_header));
  memcpy(header.magic, CHIMA_MAGIC, sizeof(CHIMA_MAGIC));
  header.file_enum = ASSET_TYPE_SPRITESHEET;
  header.ver_maj = CHIMA_SHEET_MAJ;
  header.ver_min = CHIMA_SHEET_MIN;
  header.sprite_count = (chima_u32)sprite_count;
  header.anim_count = (chima_u32)anim_count;
  header.image_width = sheet->atlas.extent.width;
  header.image_height = sheet->atlas.extent.height;
  header.image_channels = sheet->atlas.channels;
  switch (format) {
    case CHIMA_FILE_FORMAT_RAW: {
      const char format[] = "RAW";
      memcpy(header.image_format, format, sizeof(format));
      break;
    }
    case CHIMA_FILE_FORMAT_PNG: {
      const char format[] = "PNG";
      memcpy(header.image_format, format, sizeof(format));
      break;
    }
    case CHIMA_FILE_FORMAT_BMP: {
      const char format[] = "BMP";
      memcpy(header.image_format, format, sizeof(format));
      break;
    }
    case CHIMA_FILE_FORMAT_TGA: {
      const char format[] = "TGA";
      memcpy(header.image_format, format, sizeof(format));
      break;
    }
    default:
      CHIMA_UNREACHABLE();
  }

  size_t name_offset = sizeof(chima_sprite_file_header) +
                       sprite_count * sizeof(chima_file_sprite) +
                       anim_count * sizeof(chima_file_anim);

  chima_file_sprite* sprites =
    CHIMA_MALLOC(sprite_count * sizeof(chima_file_sprite));
  if (!sprites) {
    return CHIMA_ALLOC_FAILURE;
  }
  memset(sprites, 0, sprite_count * sizeof(chima_file_sprite));
  size_t name_pos = 0;
  for (size_t i = 0; i < sprite_count; ++i) {
    const chima_sprite* s = &sheet->sprites[i];
    sprites[i].height = s->rect.height;
    sprites[i].width = s->rect.width;
    sprites[i].x_off = s->rect.x;
    sprites[i].y_off = s->rect.y;
    sprites[i].name_offset = name_pos;
    sprites[i].name_size = s->name.len;
    sprites[i].frametime = s->frametime;
    name_pos += s->name.len;
  }

  chima_file_anim* anims =
    CHIMA_MALLOC(anim_count * sizeof(chima_file_anim));
  if (!anims) {
    CHIMA_FREE(sprites);
    return CHIMA_ALLOC_FAILURE;
  }
  memset(anims, 0, anim_count * sizeof(chima_file_anim));
  for (size_t i = 0; i < anim_count; ++i) {
    const chima_sprite_anim* a = &sheet->anims[i];
    anims[i].sprite_idx = a->sprite_start;
    anims[i].sprite_count = a->sprite_count;
    anims[i].name_offset = name_pos;
    anims[i].name_size = a->name.len;
    name_pos += a->name.len;
  }

  char* name_data = CHIMA_MALLOC(name_size);
  if (!name_data) {
    CHIMA_FREE(sprites);
    CHIMA_FREE(anims);
    return CHIMA_ALLOC_FAILURE;
  }
  memset(name_data, 0, name_size);
  {
    size_t name_off = 0;
    for (size_t i = 0; i < sheet->sprite_count; ++i) {
      chima_string* name = &sheet->sprites[i].name;
      memcpy(name_data + name_off, name->data, name->len);
      name_off += name->len;
    }
    for (size_t i = 0; i < sheet->anim_count; ++i) {
      chima_string* name = &sheet->anims[i].name;
      memcpy(name_data + name_off, name->data, name->len);
      name_off += name->len;
    }
  }
  header.name_size = name_size;
  header.name_offset = name_offset;
  header.image_offset = name_offset + name_size;

  FILE* f = fopen(path, "wb");
  if (!f) {
    CHIMA_FREE(name_data);
    CHIMA_FREE(sprites);
    CHIMA_FREE(anims);
    return CHIMA_FILE_OPEN_FAILURE;
  }
  fwrite(&header, sizeof(header), 1, f);
  fwrite(sprites, sizeof(sprites[0]), sprite_count, f);
  fwrite(anims, sizeof(anims[0]), anim_count, f);
  fwrite(name_data, sizeof(name_data[0]), name_size, f);

  chima_u32 w = sheet->atlas.extent.width;
  chima_u32 h = sheet->atlas.extent.height;
  chima_u32 ch = sheet->atlas.channels;
  void* data = sheet->atlas.data;
  chima__write_atlas_file(chima, f, w, h, ch, format, data);
  fclose(f);

  CHIMA_FREE(name_data);
  CHIMA_FREE(anims);
  CHIMA_FREE(sprites);
  return CHIMA_NO_ERROR;
}

void chima_destroy_spritesheet(chima_context chima, chima_spritesheet* sheet) {
  if (!sheet) {
    return;
  }
  CHIMA_FREE(sheet->anims);
  CHIMA_FREE(sheet->sprites);
  chima_destroy_image(chima, &sheet->atlas);
  memset(sheet, 0, sizeof(chima_spritesheet));
}

chima_uv_transf chima_calc_uv_transform(chima_u32 image_width,
                                        chima_u32 image_height, chima_rect rect,
                                        chima_bitfield flags) {

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
  */
  chima_uv_transf out;
  const chima_bool flip_x = (flags & CHIMA_UV_FLAG_FLIP_X);
  const chima_bool flip_y = (flags & CHIMA_UV_FLAG_FLIP_Y);
  const chima_f32 uv_scale_x =
    (flip_x*-1.f + !flip_x*1.f)*(chima_f32)rect.width/(chima_f32)image_width;
  const chima_f32 uv_off_x = (chima_f32)rect.x/(chima_f32)image_width;
  out.x_lin = uv_scale_x;
  out.x_con = flip_x*(1.f-uv_off_x) + !flip_x*uv_off_x;

  const chima_f32 uv_scale_y = 
    (flip_y*-1.f + !flip_y*1.f)*(chima_f32)rect.height/(chima_f32)image_height;
  const chima_f32 uv_off_y = (chima_f32)rect.y/(chima_f32)image_height;
  out.y_lin = uv_scale_y;
  out.y_con = flip_y*(1.f-uv_off_y) + !flip_y*uv_off_y;
  return out;
}
