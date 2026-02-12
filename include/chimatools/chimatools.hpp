#pragma once

#include "./chimatools.h"

#ifndef CHIMA_ASSERT
#include <cassert>
#define CHIMA_ASSERT(cond) assert(cond)
#endif

#if defined(CHIMA_DISABLE_EXCEPTIONS) && CHIMA_DISABLE_EXCEPTIONS
#define CHIMA_THROW(err)          CHIMA_ASSERT(false && "Thrown exception " #err)
#define CHIMA_THROW_IF(cond, err) CHIMA_ASSERT(false && "Condition " #cond " failed, thrown " #err)
#else
#define CHIMA_THROW(err) throw err
#define CHIMA_THROW_IF(cond, err) \
  if (cond) {                     \
    throw err;                    \
  }
#endif

#include <cstring>
#include <optional>
#include <string>
#include <utility>

#if __cplusplus >= 202002L
#include <span>
#endif

#if __cplusplus >= 202002L
#define CHIMA_CPP20_CONSTEXPR constexpr
#else
#define CHIMA_CPP20_CONSTEXPR inline
#endif

#define CHIMA_DEFINE_DELETER(type_, obj_)                \
  template<>                                             \
  struct chima_deleter<type_> {                          \
  public:                                                \
    chima_deleter(chima_context chima) : _chima(chima) { \
      CHIMA_ASSERT(_chima);                              \
    }                                                    \
                                                         \
  public:                                                \
    void operator()(type_& obj_) noexcept;               \
                                                         \
  private:                                               \
    chima_context _chima;                                \
  };                                                     \
  inline void chima_deleter<type_>::operator()(type_& obj_) noexcept

namespace chima {

// Wrapper for `chima_return`. Thrown from constructors.
class error : public std::exception {
public:
  error() noexcept : _code{CHIMA_NO_ERROR} {}

  explicit error(chima_result res) noexcept : _code{res} {}

public:
  const char* what() const noexcept override { return chima_error_string(_code); }

  chima_result code() const noexcept { return _code; }

public:
  explicit operator bool() const noexcept { return _code != CHIMA_NO_ERROR; }

  error& operator=(chima_result res) noexcept {
    _code = res;
    return *this;
  }

private:
  chima_result _code;
};

// Wrapper for `chima_color`
struct color : public chima_color {
  color() noexcept : chima_color() {}

  color(chima_f32 r, chima_f32 g, chima_f32 b, chima_f32 a = 1.f) noexcept :
      chima_color{r, g, b, a} {}
};

// Load a `chima_string` inside a `std::string_view`
constexpr inline std::string_view to_string_view(const chima_string& str) {
  return {&str.data[0], static_cast<chima_size>(str.len)};
}

// Load a `chima_string` inside a `std::string`
CHIMA_CPP20_CONSTEXPR inline std::string to_string(const chima_string& str) {
  return {&str.data[0], static_cast<chima_size>(str.len)};
}

// Load a `std::string_view` inside a `chima_string_view`
constexpr inline chima_string_view from_string_view(std::string_view sv) {
  return {sv.data(), sv.size()};
}

#if __cplusplus >= 202002L
// Load a `chima_string` inside a `std::span`
constexpr inline std::span<const char> to_span(const chima_string& str) {
  return std::span<const char>{&str.data[0], static_cast<chima_size>(str.len)};
}
#endif

namespace impl {

template<typename Derived>
class context_base {
protected:
  explicit context_base(chima_context chima) : _chima{chima} {
    CHIMA_THROW_IF(_is_empty(_chima), ::chima::error(CHIMA_INVALID_VALUE));
  }

  explicit context_base(const chima_alloc* alloc) {
    const chima_result ret = chima_create_context(&_chima, alloc);
    CHIMA_THROW_IF(ret != CHIMA_NO_ERROR, ::chima::error(ret));
  }

protected:
  static std::optional<Derived> create(const chima_alloc* alloc = nullptr,
                                       ::chima::error* err = nullptr) noexcept {
    chima_context chima;
    const chima_result ret = chima_create_context(&chima, alloc);
    if (ret != CHIMA_NO_ERROR) {
      if (err) {
        *err = ret;
      }
      return {};
    }
    return std::optional<Derived>{std::in_place, chima};
  }

public:
  Derived& set_flip_y(chima_bool flag) const {
    CHIMA_ASSERT(!_is_empty(_chima));
    chima_set_flip_y(_chima, flag);
  }

  Derived& set_atlas_factor(chima_f32 fac) const {
    CHIMA_ASSERT(!_is_empty(_chima));
    chima_set_atlas_factor(_chima, fac);
  }

  Derived& set_atlas_initial(chima_u32 extent) const {
    CHIMA_ASSERT(!_is_empty(_chima));
    chima_set_atlas_initial(_chima, extent);
  }

public:
  chima_context get() const {
    CHIMA_ASSERT(!_is_empty(_chima));
    return _chima;
  }

public:
  operator chima_context() const { return get(); }

protected:
  static bool _is_empty(const chima_context& chima) noexcept { return chima == nullptr; }

  static void _destroy(chima_context& chima) noexcept { chima_destroy_context(chima); }

  void _reset() noexcept { _chima = nullptr; }

protected:
  chima_context _chima;
};

} // namespace impl

// Deleter for chimatools' objects. Called from `chima::scoped_resource`.
// NOT meant to be used for pointers (does not call `delete` operator).
template<typename T>
struct chima_deleter;

// RAII wrapper for chimatools' objects..
template<typename T>
class scoped_resource : private chima_deleter<T> {
public:
  scoped_resource(chima_context chima, T& obj) noexcept : chima_deleter<T>(chima), _obj(&obj) {}

  scoped_resource(scoped_resource&& other) noexcept :
      chima_deleter<T>(static_cast<chima_deleter<T>&&>(other)), _obj(other._obj) {
    other._obj = nullptr;
  }

  scoped_resource(const scoped_resource&) = delete;

  ~scoped_resource() noexcept { destroy(); }

public:
  void rebind(T& obj) noexcept { _obj = &obj; }

  void disengage() noexcept { _obj = nullptr; }

  void destroy() noexcept {
    if (_obj) {
      auto& self = static_cast<chima_deleter<T>&>(*this);
      self(*_obj);
      disengage();
    }
  }

public:
  scoped_resource& operator=(scoped_resource&& other) noexcept {
    destroy();

    _obj = other._obj;

    other._obj = nullptr;

    return *this;
  }

  scoped_resource& operator=(const scoped_resource&) = delete;

private:
  T* _obj;
};

CHIMA_DEFINE_DELETER(chima_image, image) {
  chima_destroy_image(_chima, &image);
}

CHIMA_DEFINE_DELETER(chima_image_anim, anim) {
  chima_destroy_image_anim(_chima, &anim);
}

CHIMA_DEFINE_DELETER(chima_spritesheet, sheet) {
  chima_destroy_spritesheet(_chima, &sheet);
}

CHIMA_DEFINE_DELETER(chima_sheet_data, data) {
  chima_destroy_sheet_data(data);
}

// Non owning `chima_context`
class context_view : public impl::context_base<context_view> {
private:
  using base_t = impl::context_base<context_view>;

public:
  context_view(chima_context chima) : base_t{chima} {}
};

// Owning `chima_context` RAII wrapper.
class context : public impl::context_base<context> {
private:
  using base_t = impl::context_base<context>;

public:
  explicit context(chima_context chima) : base_t{chima} {}

  explicit context(const chima_alloc* alloc = nullptr) : base_t{alloc} {}

  using base_t::create;

  ~context() noexcept {
    if (!_is_empty(_chima)) {
      _destroy(_chima);
    }
  }

  context(context&& other) noexcept : base_t{std::move(other._chima)} { other._reset(); }

  context(const context& other) noexcept = delete;

public:
  context& operator=(context&& other) noexcept {
    if (!_is_empty(_chima)) {
      _destroy(_chima);
    }

    _chima = std::move(other._chima);

    other._reset();

    return *this;
  }

  context& operator=(const context& other) noexcept = delete;

  operator context_view() const { return {_chima}; }

public:
  [[nodiscard]] chima_context release() {
    chima_context ret = _chima;
    _reset();
    return ret;
  }
};

constexpr chima_size image_bytes(const chima_image& img) {
  constexpr auto sizes = std::to_array<chima_size>({
    sizeof(chima_u8),
    sizeof(chima_u16),
    sizeof(chima_f32),
  });
  return img.depth > CHIMA_DEPTH_32F
         ? 0
         : img.extent.width * img.extent.height * img.channels * sizes[img.depth];
}

class image : private ::chima_image {
private:
  struct create_t {};

public:
  using deleter_type = ::chima::chima_deleter<::chima::image>;

public:
  image(create_t, chima_image&& image) noexcept : chima_image(image) {}

  explicit image(chima_image img) : chima_image(img) {
    CHIMA_ASSERT(img.data != nullptr);
    CHIMA_ASSERT(img.channels > 0 && img.channels <= 4);
    CHIMA_ASSERT(::chima::image_bytes(img) > 0);
  }

  image(chima_context chima, chima_image_depth depth, const char* path) {
    CHIMA_THROW_IF(!chima, ::chima::error(CHIMA_INVALID_VALUE));
    const auto res = chima_load_image(chima, &get(), depth, path);
    CHIMA_THROW_IF(res != CHIMA_NO_ERROR, ::chima::error(res));
  }

  explicit image(chima_context chima, chima_image_depth depth, FILE* file) {
    CHIMA_THROW_IF(!chima, ::chima::error(CHIMA_INVALID_VALUE));
    const auto res = chima_load_image_file(chima, &get(), depth, file);
    CHIMA_THROW_IF(res != CHIMA_NO_ERROR, ::chima::error(res));
  }

  image(chima_context chima, chima_image_depth depth, const chima_u8* buff, chima_size size) {
    CHIMA_THROW_IF(!chima, ::chima::error(CHIMA_INVALID_VALUE));
    const auto res = chima_load_image_mem(chima, &get(), depth, buff, size);
    CHIMA_THROW_IF(res != CHIMA_NO_ERROR, ::chima::error(res));
  }

#if __cplusplus >= 202002L
  explicit image(chima_context chima, chima_image_depth depth, std::span<const chima_u8> data) :
      image(chima, depth, data.data(), data.size()) {}
#endif

  image(chima_context chima, chima_u32 width, chima_u32 height, chima_u32 channels,
        chima_image_depth depth, const chima_color& color) {
    CHIMA_THROW_IF(!chima, ::chima::error(CHIMA_INVALID_VALUE));
    const auto res = chima_gen_blank_image(chima, &get(), width, height, channels, depth, color);
    CHIMA_THROW_IF(res != CHIMA_NO_ERROR, ::chima::error(res));
  }

  image(chima_context chima, chima_extent2d extent, chima_u32 channels, chima_image_depth depth,
        const chima_color& color) :
      image(chima, extent.width, extent.height, channels, depth, color) {}

public:
  static std::optional<::chima::image> load(chima_context chima, chima_image_depth depth,
                                            const char* path,
                                            ::chima::error* err = nullptr) noexcept {
    if (!chima) {
      return std::nullopt;
    }
    chima_image image;
    const auto res = chima_load_image(chima, &image, depth, path);
    if (res != CHIMA_NO_ERROR) {
      if (err) {
        *err = res;
      }
      return std::nullopt;
    }
    return std::optional<::chima::image>{std::in_place, create_t{}, std::move(image)};
  }

  static std::optional<::chima::image> load_from_file(chima_context chima, chima_image_depth depth,
                                                      FILE* file,
                                                      ::chima::error* err = nullptr) noexcept {
    if (!chima) {
      return std::nullopt;
    }
    chima_image image;
    const auto res = chima_load_image_file(chima, &image, depth, file);
    if (res != CHIMA_NO_ERROR) {
      if (err) {
        *err = res;
      }
      return std::nullopt;
    }
    return std::optional<::chima::image>{std::in_place, create_t{}, std::move(image)};
  }

  static std::optional<::chima::image> load_from_mem(chima_context chima, chima_image_depth depth,
                                                     const chima_u8* data, chima_size len,
                                                     ::chima::error* err = nullptr) noexcept {
    if (!chima) {
      return std::nullopt;
    }
    chima_image image;
    const auto res = chima_load_image_mem(chima, &image, depth, data, len);
    if (res != CHIMA_NO_ERROR) {
      if (err) {
        *err = res;
      }
      return std::nullopt;
    }
    return std::optional<::chima::image>{std::in_place, create_t{}, std::move(image)};
  }

#if __cplusplus >= 202002L
  static std::optional<::chima::image> load_from_mem(chima_context chima, chima_image_depth depth,
                                                     std::span<const chima_u8> data,
                                                     ::chima::error* err = nullptr) noexcept {
    return ::chima::image::load_from_mem(chima, depth, data.data(), data.size(), err);
  }
#endif

  static std::optional<::chima::image> make_blank(chima_context chima, chima_u32 width,
                                                  chima_u32 height, chima_u32 channels,
                                                  chima_image_depth depth,
                                                  const chima_color& color,
                                                  ::chima::error* err = nullptr) noexcept {
    if (!chima) {
      return std::nullopt;
    }
    chima_image image;
    const auto res = chima_gen_blank_image(chima, &image, width, height, channels, depth, color);
    if (res != CHIMA_NO_ERROR) {
      if (err) {
        *err = res;
      }
      return std::nullopt;
    }
    return std::optional<::chima::image>{std::in_place, create_t{}, std::move(image)};
  }

  static std::optional<::chima::image> make_blank(chima_context chima, chima_extent2d extent,
                                                  chima_u32 channels, chima_image_depth depth,
                                                  const chima_color& color,
                                                  ::chima::error* err = nullptr) noexcept {
    return ::chima::image::make_blank(chima, extent.width, extent.height, channels, depth, color,
                                      err);
  }

public:
  static void destroy(chima_context chima, ::chima::image& image) noexcept {
    chima_destroy_image(chima, &image.get());
  }

public:
  const chima_image& get() const noexcept { return static_cast<const chima_image&>(*this); }

  chima_image& get() noexcept { return const_cast<chima_image&>(std::as_const(*this).get()); }

public:
  const void* data() const noexcept { return get().data; }

  void* data() noexcept { return get().data; }

  chima_size size_bytes() const noexcept { return ::chima::image_bytes(get()); }

  chima_extent2d extent() const noexcept { return get().extent; }

  chima_image_depth depth() const noexcept { return get().depth; }

  chima_u32 channels() const noexcept { return get().channels; }

public:
  chima_result write(chima_context chima, chima_image_format format, const char* path) const {
    return chima_write_image(chima, &get(), format, path);
  }

  chima_result composite(const chima_image& src, chima_u32 xpos, chima_u32 ypos) {
    return chima_composite_image(&get(), &src, xpos, ypos);
  }
};

CHIMA_DEFINE_DELETER(::chima::image, image) {
  ::chima::image::destroy(_chima, image);
}

class image_anim : private ::chima_image_anim {
private:
  struct create_t {};

public:
  using deleter_type = ::chima::chima_deleter<image_anim>;

public:
  image_anim(create_t, chima_image_anim&& anim) noexcept : chima_image_anim(anim) {}

  explicit image_anim(chima_image_anim anim) : chima_image_anim(anim) {
    CHIMA_ASSERT(anim.image_count > 0);
    CHIMA_ASSERT(anim.frametimes != nullptr);
    CHIMA_ASSERT(anim.image_count > 0);
  }

  image_anim(chima_context chima, const char* path) {
    CHIMA_THROW_IF(!chima, ::chima::error(CHIMA_INVALID_VALUE));
    const auto res = chima_load_image_anim(chima, &get(), path);
    CHIMA_THROW_IF(res != CHIMA_NO_ERROR, ::chima::error(res));
  }

  explicit image_anim(chima_context chima, FILE* file) {
    CHIMA_THROW_IF(!chima, ::chima::error(CHIMA_INVALID_VALUE));
    const auto res = chima_load_image_anim_file(chima, &get(), file);
    CHIMA_THROW_IF(res != CHIMA_NO_ERROR, ::chima::error(res));
  }

public:
  static std::optional<::chima::image_anim> load(chima_context chima, const char* path,
                                                 ::chima::error* err = nullptr) noexcept {
    if (!chima) {
      return std::nullopt;
    }
    chima_image_anim anim;
    const auto res = chima_load_image_anim(chima, &anim, path);
    if (res != CHIMA_NO_ERROR) {
      if (err) {
        *err = res;
      }
      return std::nullopt;
    }
    return std::optional<::chima::image_anim>{std::in_place, create_t{}, std::move(anim)};
  }

  static std::optional<::chima::image_anim>
  load_from_file(chima_context chima, FILE* file, ::chima::error* err = nullptr) noexcept {
    if (!chima) {
      return std::nullopt;
    }
    chima_image_anim anim;
    const auto res = chima_load_image_anim_file(chima, &anim, file);
    if (res != CHIMA_NO_ERROR) {
      if (err) {
        *err = res;
      }
      return std::nullopt;
    }
    return std::optional<::chima::image_anim>{std::in_place, create_t{}, std::move(anim)};
  }

public:
  static void destroy(chima_context chima, ::chima::image_anim& anim) noexcept {
    chima_destroy_image_anim(chima, &anim.get());
  }

public:
  const chima_image_anim& get() const noexcept {
    return static_cast<const chima_image_anim&>(*this);
  }

  chima_image_anim& get() noexcept {
    return const_cast<chima_image_anim&>(std::as_const(*this).get());
  }

public:
  chima_size image_count() const noexcept { return get().image_count; }

  const chima_u32* frametimes() const noexcept { return get().frametimes; }

  chima_u32* frametimes() noexcept { return get().frametimes; }

#if __cplusplus >= 202002L
  std::span<const chima_u32> frametimes_span() const noexcept {
    return {frametimes(), image_count()};
  }

  std::span<chima_u32> frametimes_span() noexcept { return {frametimes(), image_count()}; }
#endif

#ifdef CHIMA_NO_DOWNCASTING
  const chima_image* images() const noexcept { return static_cast<::chima::image*>(get().images); }

  chima_image* images() noexcept {
    return const_cast<chima_image*>(std::as_const(*this).images());
  }

#if __cplusplus >= 202002L
  std::span<const chima_image> images_span() const noexcept { return {images(), image_count()}; }

  std::span<chima_image> images_span() noexcept { return {images(), image_count()}; }
#endif
#else
  const ::chima::image* images() const noexcept {
    // Downcasting like this is probably violates strict aliasing
    // But it should be fine as long as they are the same size?
    static_assert(sizeof(::chima::image) == sizeof(::chima_image));
    static_assert(alignof(::chima::image) == alignof(::chima_image));
    return reinterpret_cast<::chima::image*>(get().images);
  }

  ::chima::image* images() noexcept {
    return const_cast<::chima::image*>(std::as_const(*this).images());
  }

#if __cplusplus >= 202002L
  std::span<const ::chima::image> images_span() const noexcept {
    return {images(), image_count()};
  }

  std::span<::chima::image> images_span() noexcept { return {images(), image_count()}; }
#endif
#endif
};

class sheet_data {
private:
  struct create_t {};

public:
  using deleter_type = ::chima::chima_deleter<::chima::sheet_data>;

public:
  sheet_data(create_t, chima_sheet_data data) noexcept : _data(data) {}

  explicit sheet_data(chima_sheet_data data) : _data(data) { CHIMA_ASSERT(data != nullptr); }

  explicit sheet_data(chima_context chima) {
    CHIMA_THROW_IF(!chima, ::chima::error(CHIMA_INVALID_VALUE));
    chima_sheet_data data;
    const auto res = chima_create_sheet_data(chima, &data);
    CHIMA_THROW_IF(res != CHIMA_NO_ERROR, ::chima::error(res));
    _data = data;
  }

public:
  static std::optional<::chima::sheet_data> create(chima_context chima,
                                                   ::chima::error* err = nullptr) noexcept {
    if (!chima) {
      return std::nullopt;
    }
    chima_sheet_data data;
    const auto res = chima_create_sheet_data(chima, &data);
    if (res != CHIMA_NO_ERROR) {
      if (err) {
        *err = res;
      }
      return std::nullopt;
    }
    return std::optional<::chima::sheet_data>{std::in_place, create_t{}, data};
  }

public:
  static void destroy(chima_context, ::chima::sheet_data& data) {
    chima_sheet_data d = data.get();
    chima_destroy_sheet_data(d);
    data._data = d;
  }

public:
  sheet_data& add_image(const chima_image& image, const char* name) {
    const auto res = chima_sheet_add_image(get(), &image, name);
    CHIMA_THROW_IF(res != CHIMA_NO_ERROR, ::chima::error(res));
    return *this;
  }

  sheet_data& add_image(const chima_image& image, chima_string_view name) {
    const auto res = chima_sheet_add_image_sv(get(), &image, name);
    CHIMA_THROW_IF(res != CHIMA_NO_ERROR, ::chima::error(res));
    return *this;
  }

  sheet_data& add_image(const chima_image& image, std::string_view name) {
    const auto res = chima_sheet_add_image_sv(get(), &image, ::chima::from_string_view(name));
    CHIMA_THROW_IF(res != CHIMA_NO_ERROR, ::chima::error(res));
    return *this;
  }

  sheet_data& add_images(const chima_image* images, const chima_u32* frametimes, chima_size count,
                         const char* basename) {
    const auto res = chima_sheet_add_images(get(), images, frametimes, count, basename);
    CHIMA_THROW_IF(res != CHIMA_NO_ERROR, ::chima::error(res));
    return *this;
  }

  sheet_data& add_images(const chima_image* images, const chima_u32* frametimes, chima_size count,
                         chima_string_view basename) {
    const auto res = chima_sheet_add_images_sv(get(), images, frametimes, count, basename);
    CHIMA_THROW_IF(res != CHIMA_NO_ERROR, ::chima::error(res));
    return *this;
  }

  sheet_data& add_images(const chima_image* images, const chima_u32* frametimes, chima_size count,
                         std::string_view basename) {
    const auto res = chima_sheet_add_images_sv(get(), images, frametimes, count,
                                               ::chima::from_string_view(basename));
    CHIMA_THROW_IF(res != CHIMA_NO_ERROR, ::chima::error(res));
    return *this;
  }

public:
  chima_sheet_data get() const {
    CHIMA_ASSERT(_data);
    return _data;
  }

public:
  operator chima_sheet_data() const { return get(); }

private:
  chima_sheet_data _data;
};

CHIMA_DEFINE_DELETER(::chima::sheet_data, data) {
  ::chima::sheet_data::destroy(_chima, data);
}

class spritesheet : private ::chima_spritesheet {
private:
  struct create_t {};

public:
  using deleter_type = ::chima::chima_deleter<::chima::spritesheet>;

  struct sprite : private ::chima_sprite {
  public:
    sprite() noexcept = default;

    explicit sprite(chima_sprite spr) noexcept : chima_sprite(spr) {}

    sprite(chima_string name, chima_rect rect, chima_u32 frametime) noexcept :
        chima_sprite{name, rect, frametime} {}

  public:
    const chima_sprite& get() const noexcept { return static_cast<const chima_sprite&>(*this); }

    chima_sprite& get() noexcept { return const_cast<chima_sprite&>(std::as_const(*this).get()); }

  public:
    std::string_view name() const noexcept { return ::chima::to_string_view(get().name); }

    chima_rect rect() const noexcept { return get().rect; }

    chima_u32 frametime() const noexcept { return get().frametime; }
  };

  struct sprite_anim : private ::chima_sprite_anim {
  public:
    sprite_anim() noexcept = default;

    explicit sprite_anim(chima_sprite_anim anim) noexcept : chima_sprite_anim(anim) {}

    sprite_anim(chima_string name, chima_size start, chima_size count) noexcept :
        chima_sprite_anim{name, start, count} {}

  public:
    const chima_sprite_anim& get() const noexcept {
      return static_cast<const chima_sprite_anim&>(*this);
    }

    chima_sprite_anim& get() noexcept {
      return const_cast<chima_sprite_anim&>(std::as_const(*this).get());
    }

  public:
    std::string_view name() const noexcept { return ::chima::to_string_view(get().name); }

    std::pair<chima_size, chima_size> span_range() const noexcept {
      return std::make_pair(get().sprite_start, get().sprite_count);
    }
#if __cplusplus >= 202002L
    std::span<const chima_sprite> make_span(const chima_sprite* first) const noexcept {
      return {first + get().sprite_start, get().sprite_count};
    }

    std::span<chima_sprite> make_span(chima_sprite* first) const noexcept {
      return {first + get().sprite_start, get().sprite_count};
    }

    std::span<const spritesheet::sprite>
    make_span(const spritesheet::sprite* first) const noexcept {
      return {first + get().sprite_start, get().sprite_count};
    }

    std::span<spritesheet::sprite> make_span(spritesheet::sprite* first) const noexcept {
      return {first + get().sprite_start, get().sprite_count};
    }
#endif
  };

public:
  spritesheet(create_t, chima_spritesheet&& sheet) noexcept : chima_spritesheet(sheet) {}

  explicit spritesheet(chima_spritesheet sheet) : chima_spritesheet(sheet) {
    CHIMA_ASSERT(sheet.sprites != nullptr);
    CHIMA_ASSERT(sheet.sprite_count > 0);
    CHIMA_ASSERT(sheet.atlas.data != nullptr);
    CHIMA_ASSERT(sheet.atlas.channels > 0 && sheet.atlas.channels <= 4);
    CHIMA_ASSERT(::chima::image_bytes(sheet.atlas) > 0);
  }

  spritesheet(chima_context chima, chima_sheet_data data, chima_u32 padding,
              const chima_color& background) {
    CHIMA_THROW_IF(!chima, ::chima::error(CHIMA_INVALID_VALUE));
    CHIMA_THROW_IF(!data, ::chima::error(CHIMA_INVALID_VALUE));
    const auto res = chima_gen_spritesheet(chima, &get(), data, padding, background);
    CHIMA_THROW_IF(res != CHIMA_NO_ERROR, ::chima::error(res));
  }

  spritesheet(chima_context chima, const char* path) {
    CHIMA_THROW_IF(!chima, ::chima::error(CHIMA_INVALID_VALUE));
    const auto res = chima_load_spritesheet(chima, &get(), path);
    CHIMA_THROW_IF(res != CHIMA_NO_ERROR, ::chima::error(res));
  }

  spritesheet(chima_context chima, const chima_u8* buff, chima_size size) {
    CHIMA_THROW_IF(!chima, ::chima::error(CHIMA_INVALID_VALUE));
    const auto res = chima_load_spritesheet_mem(chima, &get(), buff, size);
    CHIMA_THROW_IF(res != CHIMA_NO_ERROR, ::chima::error(res));
  }

#if __cplusplus >= 202002L
  explicit spritesheet(chima_context chima, std::span<const chima_u8> data) :
      spritesheet(chima, data.data(), data.size()) {}
#endif

public:
  static std::optional<::chima::spritesheet>
  make_from_data(chima_context chima, chima_sheet_data data, chima_u32 padding,
                 const chima_color& background, ::chima::error* err = nullptr) noexcept {
    if (!chima || !data) {
      return std::nullopt;
    }
    chima_spritesheet sheet;
    const auto res = chima_gen_spritesheet(chima, &sheet, data, padding, background);
    if (res != CHIMA_NO_ERROR) {
      if (err) {
        *err = res;
      }
      return std::nullopt;
    }
    return std::optional<::chima::spritesheet>{std::in_place, create_t{}, std::move(sheet)};
  }

  static std::optional<::chima::spritesheet> load(chima_context chima, const char* path,
                                                  ::chima::error* err = nullptr) noexcept {
    if (!chima) {
      return std::nullopt;
    }
    chima_spritesheet sheet;
    const auto res = chima_load_spritesheet(chima, &sheet, path);
    if (res != CHIMA_NO_ERROR) {
      if (err) {
        *err = res;
      }
      return std::nullopt;
    }
    return std::optional<::chima::spritesheet>{std::in_place, create_t{}, std::move(sheet)};
  }

  static std::optional<::chima::spritesheet>
  load_from_mem(chima_context chima, const chima_u8* buff, chima_size buff_len,
                ::chima::error* err = nullptr) noexcept {
    if (!chima) {
      return std::nullopt;
    }
    chima_spritesheet sheet;
    const auto res = chima_load_spritesheet_mem(chima, &sheet, buff, buff_len);
    if (res != CHIMA_NO_ERROR) {
      if (err) {
        *err = res;
      }
      return std::nullopt;
    }
    return std::optional<::chima::spritesheet>{std::in_place, create_t{}, std::move(sheet)};
  }

#if __cplusplus >= 202002L
  static std::optional<::chima::spritesheet>
  load_from_mem(chima_context chima, std::span<const chima_u8> data,
                ::chima::error* err = nullptr) noexcept {
    if (!chima) {
      return std::nullopt;
    }
    chima_spritesheet sheet;
    const auto res = chima_load_spritesheet_mem(chima, &sheet, data.data(), data.size());
    if (res != CHIMA_NO_ERROR) {
      if (err) {
        *err = res;
      }
      return std::nullopt;
    }
    return std::optional<::chima::spritesheet>{std::in_place, create_t{}, std::move(sheet)};
  }
#endif

public:
  static void destroy(chima_context chima, ::chima::spritesheet& sheet) noexcept {
    chima_destroy_spritesheet(chima, &sheet.get());
  }

public:
  const chima_spritesheet& get() const noexcept {
    return static_cast<const chima_spritesheet&>(*this);
  }

  chima_spritesheet& get() noexcept {
    return const_cast<chima_spritesheet&>(std::as_const(*this).get());
  }

public:
  chima_size sprite_count() const noexcept { return get().sprite_count; }

  chima_size anim_count() const noexcept { return get().anim_count; }

#ifdef CHIMA_NO_DOWNCASTING
  const chima_image& atlas() const noexcept { return get().atlas; }

  chima_image& atlas() noexcept { return const_cast<chima_image&>(std::as_const(*this).atlas()); }

  const chima_sprite* sprites() const noexcept { return get().sprites; }

  chima_sprite* sprites() noexcept {
    return const_cast<chima_sprite*>(std::as_const(*this).sprites());
  }

  const chima_sprite_anim* anims() const noexcept { return get().anims; }

  chima_sprite_anim* anims() noexcept {
    return const_cast<chima_sprite_anim*>(std::as_const(*this).anims());
  }

#if __cplusplus >= 202002L
  std::span<const chima_sprite> sprite_span() const noexcept {
    return {sprites(), sprite_count()};
  }

  std::span<chima_sprite> sprite_span() noexcept { return {sprites(), sprite_count()}; }

  std::span<const chima_sprite_anim> anim_span() const noexcept { return {anims(), anim_count()}; }

  std::span<chima_sprite_anim> anim_span() noexcept { return {anims(), anim_count()}; }
#endif
#else
  const ::chima::image& atlas() const noexcept {
    // Downcasting like this is probably violates strict aliasing
    // But it should be fine as long as they are the same size?
    static_assert(sizeof(::chima::image) == sizeof(::chima_image));
    static_assert(alignof(::chima::image) == alignof(::chima_image));
    return *reinterpret_cast<const ::chima::image*>(&get().atlas);
  }

  ::chima::image& atlas() noexcept {
    return const_cast<::chima::image&>(std::as_const(*this).atlas());
  }

  const sprite* sprites() const noexcept {
    // Downcasting like this is probably violates strict aliasing
    // But it should be fine as long as they are the same size?
    static_assert(sizeof(spritesheet::sprite) == sizeof(::chima_sprite));
    static_assert(alignof(spritesheet::sprite) == alignof(::chima_sprite));
    return reinterpret_cast<const sprite*>(&get().sprites);
  }

  sprite* sprites() noexcept { return const_cast<sprite*>(std::as_const(*this).sprites()); }

  const sprite_anim* anims() const noexcept {
    // Downcasting like this is probably violates strict aliasing
    // But it should be fine as long as they are the same size?
    static_assert(sizeof(spritesheet::sprite_anim) == sizeof(::chima_sprite_anim));
    static_assert(alignof(spritesheet::sprite_anim) == alignof(::chima_sprite_anim));
    return reinterpret_cast<const sprite_anim*>(&get().anims);
  }

  sprite_anim* anims() noexcept { return const_cast<sprite_anim*>(std::as_const(*this).anims()); }

#if __cplusplus >= 202002L
  std::span<const sprite> sprite_span() const noexcept { return {sprites(), sprite_count()}; }

  std::span<sprite> sprite_span() noexcept { return {sprites(), sprite_count()}; }

  std::span<const sprite_anim> anim_span() const noexcept { return {anims(), anim_count()}; }

  std::span<sprite_anim> anim_span() noexcept { return {anims(), anim_count()}; }
#endif
#endif
};

CHIMA_DEFINE_DELETER(::chima::spritesheet, sheet) {
  ::chima::spritesheet::destroy(_chima, sheet);
}

} // namespace chima

#undef CHIMA_DEFINE_DELETER
