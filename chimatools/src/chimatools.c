#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_rect_pack.h"

#include "chimatools.h"

#define CHIMA_UNUSED(x) (void)x
#define CHIMA_UNREACHABLE() __builtin_unreachable()

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

uint32_t chima_create_context(chima_context* ctx, const chima_alloc* alloc) {
  chima_alloc chima_funcs;
  if (alloc) {
    chima_funcs.user_data = alloc->user_data;
    chima_funcs.malloc = alloc->malloc;
    chima_funcs.free = alloc->free;
    chima_funcs.realloc = alloc->realloc;
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
    case CHIMA_INVALID_VALUE: return "Invalid value";
    case CHIMA_INVALID_FORMAT: return "Invalid image format";
    case CHIMA_IMAGE_PARSE_FAILURE: return "Failed to parse image";
    default: return "";
  }
}

static void prepare_stbi(chima_context ctx, FILE* f,
                         stbi__context* stbi, stbi__result_info* ri)
{
  stbi__start_file(stbi,f);
  stbi->a = ctx->alloc;

  int w, h, comp;
  int req_comp = 0;
  int flip_y = 0;

  memset(ri, 0, sizeof(*ri)); // make sure it's initialized if we add new fields
  ri->bits_per_channel = 8; // default is 8 so most paths don't have to be changed
  ri->channel_order = STBI_ORDER_RGB; // all current input & output are this, but this is here so we can add BGR order
  ri->num_channels = 0;
}

chima_return chima_load_image(chima_context ctx, const char* path,
                              chima_image* image, chima_image_depth depth, const char* name)
{
  if (!ctx || !path || !image || depth >= CHIMA_DEPTH_COUNT) {
    return CHIMA_INVALID_VALUE;
  }

  FILE* f = fopen(path, "rb");
  if (!f) {
    return CHIMA_FILE_OPEN_FAILURE;
  }

  printf("Starting load\n");
  stbi__context stbi;
  stbi__result_info ri;
  prepare_stbi(ctx, f, &stbi, &ri);
  int w, h, comp;
  int req_comp = 0;
  int flip_y = 0;

  void* result;
  printf("Loading texels\n");
  if (stbi__png_test(&stbi)) {
    result = stbi__png_load(&stbi, &w, &h, &comp, req_comp, &ri);
  } else if (stbi__bmp_test(&stbi)) {
    result = stbi__bmp_load(&stbi, &w, &h, &comp, req_comp, &ri);
  } else if (stbi__jpeg_test(&stbi)) {
    result = stbi__jpeg_load(&stbi, &w, &h, &comp, req_comp, &ri);
  } else {
    fclose(f);
    return CHIMA_INVALID_FORMAT;
  }

  if (!result) {
    fclose(f);
    return CHIMA_IMAGE_PARSE_FAILURE;
  }

  switch (depth) {
    case CHIMA_DEPTH_8U: {
      printf("Processing 8bit\n");
      stbi__postprocess_8bit(&stbi, &ri, &result, w, h, comp, req_comp, flip_y);
      // if (result) {
      //   // need to 'unget' all the characters in the IO buffer
      //   fseek(f, - (int) (stbi.img_buffer_end - stbi.img_buffer), SEEK_CUR);
      // }
      break;
    }
    case CHIMA_DEPTH_16U: {
      stbi__postprocess_16bit(&stbi, &ri, &result, w, h, comp, req_comp, flip_y);
      // if (result) {
      //   // need to 'unget' all the characters in the IO buffer
      //   fseek(f, - (int) (stbi.img_buffer_end - stbi.img_buffer), SEEK_CUR);
      // }
      break;
    }
    case CHIMA_DEPTH_32F: {
      stbi__postprocess_8bit(&stbi, &ri, &result, w, h, comp, req_comp, flip_y);
      if (result) {
        result = stbi__ldr_to_hdr(&stbi, result, w, h, req_comp ? req_comp : comp);
      }
      break;
    }
    default: CHIMA_UNREACHABLE();
  }
  fclose(f);

  printf("Seting vars\n");
  assert(result);
  image->channels = (uint8_t)comp;
  image->data = result;
  image->height = (uint32_t)h;
  image->width = (uint32_t)w;
  image->depth = (uint8_t)depth;
  memset(image->format, 0, CHIMA_FORMAT_MAX_SIZE);
  strcpy(image->format, "RAW");
  size_t name_len = strlen(name);
  name_len = name_len > CHIMA_STRING_MAX_SIZE ? CHIMA_STRING_MAX_SIZE : name_len;
  strncpy(image->name.data, name, name_len);
  image->name.length = name_len;

  return CHIMA_NO_ERROR;
}

void chima_free_image(chima_context ctx, chima_image* image) {
  const chima_alloc* a = &ctx->alloc;
  a->free(a->user_data, image->data);
}

typedef struct gif_result {
  void* data;
  int delay;
  struct gif_result* next;
} gif_result;

chima_return chima_load_gif(chima_context ctx, const char* path,
                            chima_anim* anim, chima_image_depth depth)
{
  if (!ctx || !path || !anim || depth >= CHIMA_DEPTH_COUNT) {
    return CHIMA_INVALID_VALUE;
  }

  FILE* f = fopen(path, "rb");
  if (!f) {
    return CHIMA_FILE_OPEN_FAILURE;
  }

  stbi__context stbi;
  stbi__result_info ri;
  prepare_stbi(ctx, f, &stbi, &ri);
  int comp;
  int req_comp = 0;
  int flip_y = 0;

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
    gr = ctx->alloc.malloc(ctx->alloc.user_data, sizeof(gif_result));
    assert(gr);
    memset(gr, 0, sizeof(gif_result));

    char buff[1024];
    snprintf(buff, 1024, "res/thing%lu.png", frames);
    stbi_write_png(buff, gif.w, gif.h, comp, data, 0);

    ++frames;
  }
  ctx->alloc.free(ctx->alloc.user_data, gif.out);
  ctx->alloc.free(ctx->alloc.user_data, gif.history);
  ctx->alloc.free(ctx->alloc.user_data, gif.background);

  if (gr != &head) {
    ctx->alloc.free(ctx->alloc.user_data, gr);
  }
  assert(0);

  if (req_comp && req_comp != 4) {

  }

  return CHIMA_NO_ERROR;
}

int main() {
  chima_context chima;
  chima_create_context(&chima, NULL);

  chima_anim anim;
  chima_load_gif(chima, "res/mariass.gif", &anim, CHIMA_DEPTH_8U);

  chima_image image;
  uint32_t ret = chima_load_image(chima, "res/test.png", &image, CHIMA_DEPTH_8U, "test");
  assert(ret == CHIMA_NO_ERROR);
  size_t st = image.width*image.channels*sizeof(uint8_t);
  stbi_write_png("res/copy_test.png", image.width, image.height, image.channels, image.data, st);

  chima_free_image(chima, &image);
  chima_destroy_context(chima);
}

