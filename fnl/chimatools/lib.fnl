(local ffi (require :ffi))

(ffi.cdef "
  struct chima_context_;
  typedef struct chima_context_ *chima_context;

  typedef void* (*PFN_chima_malloc)(void* user, size_t size);
  typedef void* (*PFN_chima_realloc)(void* user, void* ptr, size_t oldsz, size_t newsz);
  typedef void (*PFN_chima_free)(void* user, void* ptr);

  typedef uint8_t chima_u8;
  typedef uint16_t chima_u16;
  typedef uint32_t chima_u32;
  typedef float chima_f32;
  typedef size_t chima_size;
  typedef uint32_t chima_bool;
  typedef chima_u32 chima_bitfield;

  typedef struct chima_alloc {
    void* user;
    PFN_chima_malloc malloc;
    PFN_chima_realloc realloc;
    PFN_chima_free free;
  } chima_alloc;

  static const chima_size CHIMA_STRING_MAX_SIZE = 256;
  typedef struct chima_string {
    char data[CHIMA_STRING_MAX_SIZE];
    chima_size len;
  } chima_string;

  typedef struct chima_color {
    chima_f32 r, g, b, a;
  } chima_color;

  typedef struct chima_extent2d {
    chima_u32 width, height;
  } chima_extent2d;

  typedef struct chima_rect {
    chima_u32 x, y;
    chima_u32 width, height;
  } chima_rect;

  typedef enum chima_result {
    CHIMA_NO_ERROR = 0,
    CHIMA_ALLOC_FAILURE,
    CHIMA_FILE_OPEN_FAILURE,
    CHIMA_FILE_WRITE_FAILURE,
    CHIMA_FILE_EOF,
    CHIMA_INVALID_FILE_FORMAT,
    CHIMA_IMAGE_PARSE_FAILURE,
    CHIMA_INVALID_VALUE,
    CHIMA_UNSUPPORTED_FORMAT,
    CHIMA_PACKING_FAILED,

    _CHIMA_RESULT_COUNT,
    _CHIMA_RETURN_FORCE_32BIT = 0x7fffffff,
  } chima_result;

  const char* chima_error_string(chima_result ret);
  chima_result chima_create_context(chima_context* chima, const chima_alloc* alloc);
  chima_u32 chima_set_atlas_initial(chima_context chima, chima_u32 initial);
  chima_f32 chima_set_atlas_factor(chima_context chima, chima_f32 factor);
  chima_bool chima_set_flip_y(chima_context chima, chima_bool flip_y);
  void chima_destroy_context(chima_context chima);

  typedef enum chima_image_format {
    CHIMA_FILE_FORMAT_RAW = 0,
    CHIMA_FILE_FORMAT_PNG,
    CHIMA_FILE_FORMAT_BMP,
    CHIMA_FILE_FORMAT_TGA,

    _CHIMA_FILE_FORMAT_COUNT,
    _CHIMA_FORMAT_FORCE_32BIT = 0x7ffffff,
  } chima_image_format;

  typedef enum chima_image_depth {
    CHIMA_DEPTH_8U = 0,
    CHIMA_DEPTH_16U,
    CHIMA_DEPTH_32F,

    _CHIMA_DEPTH_COUNT,
    _CHIMA_DEPTH_FORCE_32BIT = 0x7FFFFFFF,
  } chima_image_depth;

  chima_result chima_gen_blank_image(chima_context chima, chima_image* image,
                                     chima_u32 width, chima_u32 height, chima_u32 channels,
                                     chima_image_depth depth, chima_color background_color);

  chima_result chima_gen_atlas_image(chima_context chima, chima_image* atlas,
                                     chima_rect* sprites, chima_u32 padding,
                                     chima_color background_color,
                                     const chima_image* images, chima_size image_count);

  chima_result chima_load_image(chima_context chima, chima_image* image,
                                chima_image_depth depth, const char* path);

  chima_result chima_write_image(chima_context chima, const chima_image* image,
                                 chima_image_format format, const char* path);

  chima_result chima_composite_image(chima_image* dst, const chima_image* src,
                                     chima_u32 xpos, chima_u32 ypos);

  void chima_destroy_image(chima_context chima, chima_image* image);

  typedef struct chima_image_anim {
    chima_image* images;
    chima_u32* frametimes;
    chima_size image_count;
  } chima_image_anim;

  chima_result chima_load_image_anim(chima_context chima, chima_image_anim* anim,
                                     const char* path);

  void chima_destroy_image_anim(chima_context chima, chima_image_anim* anim);

  struct chima_sheet_data_;
  typedef struct chima_sheet_data_* chima_sheet_data;

  chima_result chima_create_sheet_data(chima_context chima, chima_sheet_data* data);

  chima_result chima_sheet_add_image(chima_sheet_data data, const chima_image* image,
                                     const char* name);

  chima_result chima_sheet_add_image_sv(chima_sheet_data data, const chima_image* image,
                                        chima_string_view name);

  chima_result chima_sheet_add_images(chima_sheet_data data, const chima_image* images,
                                      const chima_u32* frametimes, chima_size count,
                                      const char* basename);

  chima_result chima_sheet_add_images_sv(chima_sheet_data data, const chima_image* images,
                                         const chima_u32* frametimes, chima_size count,
                                         chima_string_view basename);

  chima_result chima_sheet_add_image_anim(chima_sheet_data data,
                                          const chima_image_anim* anim,
                                          const char* basename);

  chima_result chima_sheet_add_image_anim_sv(chima_sheet_data data,
                                             const chima_image_anim* anim,
                                             chima_string_view basename);

  void chima_destroy_sheet_data(chima_sheet_data data);

  typedef struct chima_sprite {
    chima_string name;
    chima_rect rect;
    chima_u32 frametime;
  } chima_sprite;

  typedef struct chima_sprite_anim {
    chima_string name;
    chima_size sprite_start;
    chima_size sprite_count;
  } chima_sprite_anim;

  typedef struct chima_spritesheet {
    chima_image atlas;
    chima_sprite* sprites;
    chima_size sprite_count;
    chima_sprite_anim* anims;
    chima_size anim_count;
  } chima_spritesheet;

  chima_result chima_gen_spritesheet(chima_context chima, chima_spritesheet* sheet,
                                     chima_sheet_data data, chima_u32 padding,
                                     chima_color background_color);

  chima_result chima_load_spritesheet(chima_context chima, chima_spritesheet* sheet,
                                      const char* path);

  chima_result chima_write_spritesheet(chima_context chima, const chima_spritesheet* sheet,
                                       const char* path, chima_image_format format);

  void chima_destroy_spritesheet(chima_context chima, chima_spritesheet* sheet);

  typedef struct chima_uv_transf {
    chima_f32 x_lin, x_con;
    chima_f32 y_lin, y_con;
  } chima_uv_transf;

  typedef enum chima_uv_flags {
    CHIMA_UV_FLAG_NONE = 0x0000,
    CHIMA_UV_FLAG_FLIP_Y = 0x0001,
    CHIMA_UV_FLAG_FLIP_X = 0x0002,

    _CHIMA_UV_FLAG_FORCE_32BIT = 0x7FFFFFFF
  } chima_uv_flags;

  chima_uv_transf chima_calc_uv_transform(chima_u32 image_width, chima_u32 image_height,
                                          chima_rect rect, chima_bitfield flags);
")

(local lib (ffi.load :chimatools))

(fn check-err [ret]
  (if (not= ret 0)
      (values (ffi.string (lib.chima_error_string ret)) ret)
      nil))

(local color-mt {})
(set color-mt.__index color-mt)
(local color-ctype (ffi.metatype :chima_color color-mt))
(local color {:_ctype color-ctype
              :new (fn [r g b a]
                     (let [col (ffi.new color-ctype)]
                       (set (. col :r) (or r 0))
                       (set (. col :g) (or g 0))
                       (set (. col :b) (or b 0))
                       (set (. col :a) (or a 0))
                       col))})

(local rect-mt {})
(set rect-mt.__index rect-mt)
(local rect-ctype (ffi.metatype :chima_rect rect-mt))
(local rect {:_ctype rect-ctype
             :new (fn [x y width height]
                    (let [rec (ffi.new rect-ctype)]
                      (set (. rec :x) (or x 0))
                      (set (. rec :y) (or y 0))
                      (set (. rec :width) (or width 0))
                      (set (. rec :height) (or height 0))
                      rec))})

{: lib : check-err : color : rect}
