#ifndef CHIMATOOLS_H_
#define CHIMATOOLS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*! @file chimatools.h
 *  @brief Header for the chimatools API:
 *
 *  Header for the chimatools API. Defines all chimatools
 *  types and declares all its functions.
 */
/*! @defgroup core Common reference.
 *  @brief Functions and types used by all groups.
 *
 *  Functions and types used by all groups.
 */
/*! @defgroup image Image manipulation reference
 *  @brief Functions and types related to image manipulation.
 *
 *  Functions and types related to image manipulation. Includes creation,
 *  loading and other similar operations with image related objects.
 */
/*! @defgroup util Utillities reference
 *  @brief Utillity functions and types.
 *
 *  Utillity functions and types. Includes any other function and type
 *  that does not belong to other groups.
 */

#if !defined(_WIN32) && (defined(__WIN32__) || defined(WIN32) || defined(__MINGW32__))
#define _WIN32
#endif

#if defined(_WIN32) && defined(CHIMA_SHARED_BUILD_)
#define CHIMA_API __declspec(dllexport)
#elif defined(_WIN32) && defined(CHIMA_SHARED_BUILD_)
#define CHIMA_API __declspec(dllimport)
#elif defined(__GNUC__) && defined(CHIMA_SHARED_BUILD_)
#define CHIMA_API __attribute__((visibility("default")))
#else
#define CHIMA_API
#endif

/*! @name chimatools C declarations and definitions
 * @{ */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/*! @brief chimatools major version.
 *
 *  Modified when API breaking changes are introduced.
 */
#define CHIMA_VER_MAJ 1
/*! @brief chimatools minor version
 *
 *  Modified when non-API breaking changes are introduced.
 */
#define CHIMA_VER_MIN 0
/*! @brief chimatools revision version
 *
 *  Modified for patches that don't break the API.
 */
#define CHIMA_VER_REV 0

/*! @brief One.
 *
 *  @ingroup core
 */
#define CHIMA_TRUE  1
/*! @brief Zero.
 *
 *  @ingroup core
 */
#define CHIMA_FALSE 0

/*! @brief Alias for unsigned bytes.
 *
 *  @ingroup core
 */
typedef uint8_t chima_u8;
/*! @brief Alias for unsigned 2 byte integers.
 *
 *  @ingroup core
 */
typedef uint16_t chima_u16;
/*! @brief Alias for unsigned 4 byte integers.
 *
 *  @ingroup core
 */
typedef uint32_t chima_u32;
/*! @brief Alias for 4 byte floats.
 *
 *  @ingroup core
 */
typedef float chima_f32;
/*! @brief Alias for size_t.
 *
 *  @ingroup core
 */
typedef size_t chima_size;
/*! @brief Alias for booleans.
 *
 *  @ingroup core
 */
typedef uint32_t chima_bool;
/*! @brief Alias for 4 byte bitfields.
 *
 *  @ingroup core
 */
typedef chima_u32 chima_bitfield;

/*! @brief Function pointer used for internal allocations.
 *
 *  If your function returns `NULL`, a `CHIMA_ALLOC_FAILURE` will be produced.
 *
 *  Memory allocated from this function should be aligned for any object type.
 *  Just like malloc this should be `alignof(max_align_t)`.
 *
 *  The returned memory should be valid at least until it is deallocated
 *
 *  @thread_safety Does not need to be thread safe as long as you use your
 *  `chima_context` on a single thread.
 *
 *  @param[in] user User-defined pointer
 *  @param[in] size Minimum allocation size required
 *  @return The address of the memory block, or `NULL` if the allocation
 *  failed.
 *
 *  @ingroup core
 */
typedef void* (*PFN_chima_malloc)(void* user, size_t size);

/*! @brief Function pointer used for internal reallocations.
 *
 *  If your function returns `NULL`, a `CHIMA_ALLOC_FAILURE` will be produced.
 *
 *  Memory allocated from this function should be aligned for any object type.
 *  Just like malloc this should be `alignof(max_align_t)`.
 *
 *  The returned memory should be valid at least until it is deallocated
 *
 *  @thread_safety Does not need to be thread safe as long as you use your
 *  `chima_context` on a single thread.
 *
 *  @param[in] user User-defined pointer
 *  @param[in] ptr Memory block to reallocate
 *  @param[in] oldsz Size of memory block to reallocate
 *  @param[in] newsz Minimum allocation size required
 *  @return The address of the memory block, or `NULL` if the allocation
 *  failed.
 *
 *  @ingroup core
 */
typedef void* (*PFN_chima_realloc)(void* user, void* ptr, size_t oldsz, size_t newsz);

/*! @brief Function pointer used for freeing internal allocations.
 *
 *  @thread_safety Does not need to be thread safe as long as you use your
 *  `chima_context` on a single thread.
 *
 *  @param[in] user User-defined pointer
 *  @param[in] ptr Memory block to free generated from `PFN_chima_malloc` or
 *  `PFN_chima_realloc`
 *  @return The address of the memory block, or `NULL` if the allocation
 *  failed.
 *
 *  @ingroup core
 */
typedef void (*PFN_chima_free)(void* user, void* ptr);

/*! @brief Set of user-defined allocation callbacks.
 *
 *  All functions should be valid (not `NULL`). The `user`
 *  parameter is optional.
 *
 *  @ingroup core
 */
typedef struct chima_alloc {
  /*! User-defined pointer. Optional.
   */
  void* user;
  /*! Function pointer used for internal allocations.
   * See `PFN_chima_malloc`.
   */
  PFN_chima_malloc malloc;
  /*! Function pointer used for internal reallocations.
   * See `PFN_chima_realloc`.
   */
  PFN_chima_realloc realloc;
  /*! Function pointer used for freeing internal allocations.
   * See `PFN_chima_free`.
   */
  PFN_chima_free free;
} chima_alloc;

/*! @brief Opaque chimatools context object
 *
 *  Allocated using the user provided `chima_alloc` or
 *  the default allocator if none is provided.
 *
 *  Holds global settings and memory management options.
 *
 *  @ingroup core
 */
typedef struct chima_context_* chima_context;

/*! @brief Maximum character count for a `chima_string`
 *
 *  @ingroup core
 */
#define CHIMA_STRING_MAX_SIZE 256

/*! @brief Data for a fixed size character array with length.
 *
 *  @ingroup core
 */
typedef struct chima_string {
  /*! `NULL` terminated string data.
   */
  char data[CHIMA_STRING_MAX_SIZE];
  /*! string length (not including the `NULL` terminator).
   */
  chima_size len;
} chima_string;

/*! @brief Non-owning string view
 *
 *  Represents a string of size `len` starting at pointer `data`.
 *
 *  @ingroup core
 */
typedef struct chima_string_view {
  /*! Starting pointer for the string.
   */
  const char* data;
  /*! Length of the string. Does not include the `NULL` terminator (if any).
   */
  chima_size len;
} chima_string_view;

/*! @brief RGBA floating point color.
 *
 *  Represents color values in the range [0.0, 1.0].
 *
 *  @ingroup core
 */
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

/*! @brief Error codes produced by chimatools functions.
 *
 *  @ingroup core
 */
typedef enum chima_result {
  /*! No error produced.
   */
  CHIMA_NO_ERROR = 0,
  /*! Allocation failure.
   */
  CHIMA_ALLOC_FAILURE,
  /*! Failed to open file.
   */
  CHIMA_FILE_OPEN_FAILURE,
  /*! Failed to write file.
   */
  CHIMA_FILE_WRITE_FAILURE,
  /* Reached end of file.
   */
  CHIMA_FILE_EOF,
  /* Invalid file format.
   */
  CHIMA_INVALID_FILE_FORMAT,
  /* Failed to parse image data.
   */
  CHIMA_IMAGE_PARSE_FAILURE,
  /* Invalid parameter provided.
   */
  CHIMA_INVALID_VALUE,
  /* Unsupported format provided.
   */
  CHIMA_UNSUPPORTED_FORMAT,
  /* Rectangle packing failed.
   */
  CHIMA_PACKING_FAILED,

  _CHIMA_RESULT_COUNT,
  _CHIMA_RESULT_FORCE_32BIT = 0x7FFFFFFF,
} chima_result;

/*! @brief Get a `chima_result` value null terminated string representation.
 *
 *  @param[in] ret `chima_result` enum value.
 *  @return Null terminated string representation for the provided enum
 *  value. "Unknown error" if an invalid value is provided.
 *
 *  @ingroup core
 */
CHIMA_API const char* chima_error_string(chima_result ret);

/*! @brief Allocate and initialize a `chima_context` handle
 *
 *  Uses the provided allocation functions to allocate itself
 *  and all the following chimatools objects created using this
 *  context.
 *
 *  If the context creation fails, your handle is set to `NULL`.
 *
 *  @param[in] chima Context to initialize. Not `NULL`.
 *  @param[in] alloc User defined allocator. Optional.
 *  @return `CHIMA_NO_ERROR` on success. `CHIMA_ALLOC_FAILURE` on allocation
 *  failure. `CHIMA_INVALID_VALUE` if `chima` is `NULL`.
 *
 *  @ingroup image
 */
CHIMA_API chima_result chima_create_context(chima_context* chima, const chima_alloc* alloc);

/*! @brief Sets the atlas initial size. Context local.
 *
 *  `chima_create_atlas_image` uses this value as the initial size
 *  to create an image atlas.
 *
 *  @note The default context initial size is `512`
 *
 *  @param[in] chima Chima context. Must not be `NULL`.
 *  @param[in] initial Initial size. Must be >= 0.
 *  @return The previous initial size.
 *
 *  @ingroup image
 */
CHIMA_API chima_u32 chima_set_atlas_initial(chima_context chima, chima_u32 initial);

/*! @brief Sets the atlas grow factor. Context local.
 *
 *  When you create an atlas image using `chima_create_atlas_image`, the value
 *  defined by `chima_set_atlas_initial` is multiplied by this factor each time
 *  the atlas image needs to be recalculated.
 *
 *  @note The default context factor is `2.0f`
 *
 *  @param[in] chima Chima context. Must not be `NULL`.
 *  @param[in] factor Grow factor. Must be >= 1.0f.
 *  @return The previous grow factor.
 *
 *  @ingroup image
 */
CHIMA_API chima_f32 chima_set_atlas_factor(chima_context chima, chima_f32 factor);

/*! @brief Sets the image loader flip flag.
 *
 *  Used by image and image animation loading functions.
 *  If this flag is set, the resulting image bitmap will have its
 *  rows ordered from bottom to top. This is useful for loading
 *  images directly in OpenGL or similar.
 *
 *  @note The default value is `CHIMA_FALSE`
 *
 *  @param[in] chima Chima context. Must not be `NULL`.
 *  @param[in] flip_y Boolean flag
 *  @return The previous flip flag state
 *
 *  @ingroup image
 */
CHIMA_API chima_bool chima_set_flip_y(chima_context chima, chima_bool flip_y);

/*! @brief Destroy and deallocate a `chima_context` handle.
 *
 *  Does NOT destroy any previously allocated chimatools objects. It is the
 *  user's responsability to destroy them before the context is destroyed.
 *
 *  @note This function does nothing if the argument is `NULL`.
 *
 *  @param[in] chima Chima context. Can be `NULL`.
 *
 *  @ingroup image
 */
CHIMA_API void chima_destroy_context(chima_context chima);

/*! @brief
 */
typedef enum chima_image_format {
  CHIMA_FILE_FORMAT_RAW = 0,
  CHIMA_FILE_FORMAT_PNG,
  CHIMA_FILE_FORMAT_BMP,
  CHIMA_FILE_FORMAT_TGA,

  _CHIMA_FILE_FORMAT_COUNT,
  _CHIMA_FILE_FORMAT_FORCE_32BIT = 0x7FFFFFFF,
} chima_image_format;

typedef enum chima_image_depth {
  /* 1 byte per color component, integral format in range [0, 255]
   */
  CHIMA_DEPTH_8U = 0,
  /*! 2 bytes per color component, integral format in range [0, 65535]
   */
  CHIMA_DEPTH_16U,
  /*! 4 bytes per color component, floating point format in range [0.0, 1.0]
   */
  CHIMA_DEPTH_32F,

  _CHIMA_DEPTH_COUNT,
  _CHIMA_DEPTH_FORCE_32BIT = 0x7FFFFFFF,
} chima_image_depth;

typedef struct chima_image {
  chima_extent2d extent;
  chima_u32 channels;
  chima_image_depth depth;
  void* data;
} chima_image;

CHIMA_API chima_result chima_gen_blank_image(chima_context chima, chima_image* image,
                                             chima_u32 width, chima_u32 height, chima_u32 channels,
                                             chima_image_depth depth,
                                             chima_color background_color);

CHIMA_API chima_result chima_gen_atlas_image(chima_context chima, chima_image* atlas,
                                             chima_rect* sprites, chima_u32 padding,
                                             chima_color background_color,
                                             const chima_image* images, chima_size image_count);

CHIMA_API chima_result chima_load_image(chima_context chima, chima_image* image,
                                        chima_image_depth depth, const char* path);

CHIMA_API chima_result chima_load_image_file(chima_context chima, chima_image* image,
                                             chima_image_depth depth, FILE* f);

CHIMA_API chima_result chima_load_image_mem(chima_context chima, chima_image* image,
                                            chima_image_depth depth, const chima_u8* buffer,
                                            chima_size buffer_len);

CHIMA_API chima_result chima_write_image(chima_context chima, const chima_image* image,
                                         chima_image_format format, const char* path);

CHIMA_API chima_result chima_composite_image(chima_image* dst, const chima_image* src,
                                             chima_u32 xpos, chima_u32 ypos);

CHIMA_API void chima_destroy_image(chima_context chima, chima_image* image);

typedef struct chima_image_anim {
  chima_image* images;
  chima_u32* frametimes;
  chima_size image_count;
} chima_image_anim;

CHIMA_API chima_result chima_load_image_anim(chima_context chima, chima_image_anim* anim,
                                             const char* path);

CHIMA_API chima_result chima_load_image_anim_file(chima_context chima, chima_image_anim* anim,
                                                  FILE* f);

CHIMA_API void chima_destroy_image_anim(chima_context chima, chima_image_anim* anim);

typedef struct chima_sheet_data_* chima_sheet_data;

CHIMA_API chima_result chima_create_sheet_data(chima_context chima, chima_sheet_data* data);

CHIMA_API chima_result chima_sheet_add_image(chima_sheet_data data, const chima_image* image,
                                             const char* name);

CHIMA_API chima_result chima_sheet_add_image_sv(chima_sheet_data data, const chima_image* image,
                                                chima_string_view name);

CHIMA_API chima_result chima_sheet_add_images(chima_sheet_data data, const chima_image* images,
                                              const chima_u32* frametimes, chima_size count,
                                              const char* basename);

CHIMA_API chima_result chima_sheet_add_images_sv(chima_sheet_data data, const chima_image* images,
                                                 const chima_u32* frametimes, chima_size count,
                                                 chima_string_view basename);

CHIMA_API chima_result chima_sheet_add_image_anim(chima_sheet_data data,
                                                  const chima_image_anim* anim,
                                                  const char* basename);

CHIMA_API chima_result chima_sheet_add_image_anim_sv(chima_sheet_data data,
                                                     const chima_image_anim* anim,
                                                     chima_string_view basename);

CHIMA_API void chima_destroy_sheet_data(chima_sheet_data data);

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

CHIMA_API chima_result chima_gen_spritesheet(chima_context chima, chima_spritesheet* sheet,
                                             chima_sheet_data data, chima_u32 padding,
                                             chima_color background_color);

CHIMA_API chima_result chima_load_spritesheet(chima_context chima, chima_spritesheet* sheet,
                                              const char* path);

CHIMA_API chima_result chima_load_spritesheet_file(chima_context chima, chima_spritesheet* sheet,
                                                   FILE* f);

CHIMA_API chima_result chima_load_spritesheet_mem(chima_context chima, chima_spritesheet* sheet,
                                                  const chima_u8* buffer, chima_size buffer_len);

CHIMA_API chima_result chima_write_spritesheet(chima_context chima, const chima_spritesheet* sheet,
                                               chima_image_format format, const char* path);

CHIMA_API void chima_destroy_spritesheet(chima_context chima, chima_spritesheet* sheet);

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

CHIMA_API chima_uv_transf chima_calc_uv_transform(chima_u32 image_width, chima_u32 image_height,
                                                  chima_rect rect, chima_bitfield flags);

/*! @} */

#ifdef __cplusplus
}
#endif // #ifdef __cplusplus

#endif // #ifndef CHIMATOOLS_H_
