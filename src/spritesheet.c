#include "./internal.h"

#include "./stb/stb_image_write.h"

#include <string.h>
#include <stdalign.h>

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

chima_result chima_create_sheet_data(chima_context chima, chima_sheet_data* data) {
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

static chima_result do_sheet_add_images(chima_sheet_data data, const chima_image* images,
                                        const chima_u32* frametimes, chima_size image_count,
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
  memset(nodes, 0, image_count*sizeof(*nodes));

  name_len = name_len > CHIMA_STRING_MAX_SIZE-1 ? CHIMA_STRING_MAX_SIZE-1 : name_len;
  for (chima_size i = 0; i < image_count; ++i) {
    nodes[i].next = nodes+i+1;
    nodes[i].image = &images[i];
    nodes[i].frametime = frametimes ? frametimes[i] : 1;
    if (!name) {
      int len = sprintf(nodes[i].name.data, "chima_image.%05zu", data->image_count+i);
      nodes[i].name.len = len; // sprintf returns the length without null terminator
    } else {
      strncpy(nodes[i].name.data, name, name_len);
      nodes[i].name.data[name_len] = '\0';
    }
  }
  nodes[image_count-1].next = data->image_head;
  if (image_count == 1) {
  }

  data->image_head = nodes;
  data->image_count += image_count;
  return CHIMA_NO_ERROR;
}

static chima_result do_sheet_add_anim(chima_sheet_data data, const chima_image_anim* anim,
                                      const char* name, chima_size name_len) {
  if (!data || !anim) {
    return CHIMA_INVALID_VALUE;
  }

  return CHIMA_NO_ERROR;
}

chima_result chima_sheet_add_image(chima_sheet_data data,
                                             const chima_image* image,
                                             const char* name) {
  return do_sheet_add_images(data, image, NULL, 1, name, name ? strlen(name) : 0);
}

chima_result chima_sheet_add_images(chima_sheet_data data, const chima_image* images,
                                    const chima_u32* frametimes, chima_size count,
                                    const char* basename) {
  return do_sheet_add_images(data, images, frametimes, count, basename,
                             basename ? strlen(basename) : 0);
}

chima_result chima_sheet_add_image_sv(chima_sheet_data data, const chima_image* image,
                                      chima_string_view name) {
  return do_sheet_add_images(data, image, NULL, 1, name.data, name.data ? name.len : 0);
}

chima_result chima_sheet_add_images_sv(chima_sheet_data data, const chima_image* images,
                                       const chima_u32* frametimes, chima_size count,
                                       chima_string_view basename) {
  return do_sheet_add_images(data, images, frametimes, count, basename.data,
                             basename.data ? basename.len : 0);
}

chima_result chima_sheet_add_image_anim(chima_sheet_data data, const chima_image_anim* anim,
                                        const char* basename) {
  return do_sheet_add_anim(data, anim, basename, basename ? strlen(basename) : 0);
}

chima_result chima_sheet_add_image_anim_sv(chima_sheet_data data, const chima_image_anim* anim,
                                           chima_string_view basename) {
  return do_sheet_add_anim(data, anim, basename.data, basename.data ? basename.len : 0);
}

chima_result chima_create_spritesheet(chima_context chima, chima_spritesheet* sheet,
                                      chima_sheet_data data, chima_u32 padding,
                                      chima_color background_color) {
  if (!chima || !sheet || !data) {
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

#define CHIMA_FORMAT_MAX_SIZE 7
#define CHIMA_SHEET_MAJ 0
#define CHIMA_SHEET_MIN 1

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
  char image_format[7]; // Up to 6 chars + null terminator
} chima_sprite_file_header;

typedef struct chima_file_sprite {
  chima_u32 width;
  chima_u32 height;
  chima_u32 x_off;
  chima_u32 y_off;
  float uv_x_lin;
  float uv_x_con;
  float uv_y_lin;
  float uv_y_con;
  chima_u32 name_offset;
  chima_u32 name_size;
} chima_file_sprite;

typedef struct chima_file_anim {
  chima_u32 sprite_idx;
  chima_u32 sprite_count;
  chima_u32 name_offset;
  chima_u32 name_size;
  float fps;
} chima_file_anim;

chima_result chima_load_spritesheet(chima_context chima,
                                              chima_spritesheet* sheet,
                                              const char* path) {
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
  CHIMA_ASSERT(read == read_len);
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
                                           (chima_u8*)image_data, read_len);
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

chima_result chima_load_spritesheet_mem(chima_context chima,
                                                  chima_spritesheet* sheet,
                                                  const chima_u8* buffer,
                                                  chima_size buffer_len) {

}

chima_result chima_write_spritesheet(const chima_spritesheet* sheet,
                                               const char* path,
                                               chima_image_format format) {
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
  header.sprite_count = (chima_u32)sprite_count;
  header.anim_count = (chima_u32)anim_count;
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

  chima_u32 w = sheet->atlas.width;
  chima_u32 h = sheet->atlas.height;
  chima_u32 ch = sheet->atlas.channels;
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

void chima_destroy_spritesheet(chima_context chima,
                                         chima_spritesheet* sheet) {
  if (!sheet) {
    return;
  }
  chima_context chima = sheet->ctx;
  CHIMA_FREE(chima, sheet->anims);
  CHIMA_FREE(chima, sheet->sprites);
  chima_destroy_image(&sheet->atlas);
  memset(sheet, 0, sizeof(chima_spritesheet));
}
