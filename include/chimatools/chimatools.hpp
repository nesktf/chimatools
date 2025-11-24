#pragma once

#include <chimatools/chimatools.h>

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
#include <span>
#include <string>

#define LVALUE_RVALUE_METHOD_OVERLOAD(derived_type, signature, body) \
  derived_type& signature& {                                         \
    body return static_cast<derived_type&>(*this);                   \
  }                                                                  \
  derived_type&& signature&& {                                       \
    body return static_cast<derived_type&&>(*this);                  \
  }

namespace chima {

// Wrapper for `chima_return`. Thrown from constructors.
class error : public std::exception {
public:
  error() noexcept : _code{CHIMA_NO_ERROR} {}

  explicit error(chima_return ret) noexcept : _code{ret} {}

public:
  error& operator=(chima_return ret) noexcept {
    _code = ret;
    return *this;
  }

public:
  explicit operator bool() const noexcept { return _code != CHIMA_NO_ERROR; }

  const char* what() const noexcept override { return chima_error_string(_code); }

  chima_return code() const noexcept { return _code; }

private:
  chima_return _code;
};

// Wrapper for `chima_color`
class color {
public:
  color(float r = 0.f, float g = 0.f, float b = 0.f, float a = 0.f) noexcept :
      _color{r, g, b, a} {}

  color(const chima_color& col) noexcept : _color{col} {}

public:
  LVALUE_RVALUE_METHOD_OVERLOAD(color, r(float value), { _color.r = value; })
  LVALUE_RVALUE_METHOD_OVERLOAD(color, g(float value), { _color.g = value; })
  LVALUE_RVALUE_METHOD_OVERLOAD(color, b(float value), { _color.b = value; })
  LVALUE_RVALUE_METHOD_OVERLOAD(color, a(float value), { _color.a = value; })

  float r() { return _color.r; }

  float g() { return _color.g; }

  float b() { return _color.b; }

  float a() { return _color.a; }

public:
  operator chima_color() const noexcept { return _color; }

private:
  chima_color _color;
};

// Load a `chima_string` inside a `std::string_view`
constexpr std::string_view to_str_view(const chima_string& str) {
  return {&str.data[0], static_cast<size_t>(str.length)};
}

// Load a `chima_string` inside a `std::string`
constexpr std::string to_str(const chima_string& str) {
  return {&str.data[0], static_cast<size_t>(str.length)};
}

// Load a `chima_string` inside a `std::span`
constexpr std::span<const char> to_span(const chima_string& str) {
  return {&str.data[0], static_cast<size_t>(str.length)};
}

namespace impl {

constexpr std::pair<void*, size_t> calc_chima_image_data(const chima_image& img) {
  size_t img_size = img.width * img.height * img.channels;
  switch (img.channels) {
    case CHIMA_DEPTH_8U: {
      img_size *= sizeof(uint8_t);
      break;
    }
    case CHIMA_DEPTH_16U: {
      img_size *= sizeof(uint16_t);
      break;
    }
    case CHIMA_DEPTH_32F: {
      img_size *= sizeof(float);
      break;
    }
  }
  return {img.data, img_size};
}

template<typename Derived>
class context_base {
protected:
  explicit context_base(const chima_context& chima) : _chima{chima} {
    CHIMA_THROW_IF(_is_empty(_chima), ::chima::error{CHIMA_INVALID_VALUE})
    CHIMA_ASSERT(!_is_empty(chima));
  }

  explicit context_base(const chima_alloc* alloc) {
    const chima_return ret = chima_create_context(&_chima, alloc);
    CHIMA_THROW_IF(ret != CHIMA_NO_ERROR, ::chima::error{ret});
  }

public:
  chima_context get() const {
    CHIMA_ASSERT(!_is_empty(_chima));
    return _chima;
  }

  operator chima_context() const { return get(); }

protected:
  static std::optional<Derived> create(const chima_alloc* alloc = nullptr,
                                       ::chima::error* err = nullptr) noexcept {
    chima_context chima;
    const chima_return ret = chima_create_context(&chima, alloc);
    if (ret != CHIMA_NO_ERROR) {
      if (err) {
        *err = ret;
      }
      return {};
    }
    return std::optional<Derived>{std::in_place, chima};
  }

public:
  LVALUE_RVALUE_METHOD_OVERLOAD(Derived, set_uv_x_flip(bool flag), {
    CHIMA_ASSERT(!_is_empty(_chima));
    chima_set_uv_x_flip(_chima, static_cast<chima_bool>(flag));
  })

  LVALUE_RVALUE_METHOD_OVERLOAD(Derived, set_uv_y_flip(bool flag), {
    CHIMA_ASSERT(!_is_empty(_chima));
    chima_set_uv_y_flip(_chima, static_cast<chima_bool>(flag));
  })

  LVALUE_RVALUE_METHOD_OVERLOAD(Derived, set_image_y_flip(bool flag), {
    CHIMA_ASSERT(!_is_empty(_chima));
    chima_set_image_y_flip(_chima, flag);
  })

  LVALUE_RVALUE_METHOD_OVERLOAD(Derived, set_atlas_factor(float fac), {
    CHIMA_ASSERT(!_is_empty(_chima));
    chima_set_atlas_factor(_chima, fac);
  })

  LVALUE_RVALUE_METHOD_OVERLOAD(Derived, set_sheet_initial(uint32_t extent), {
    CHIMA_ASSERT(!_is_empty(_chima));
    chima_set_sheet_initial(_chima, extent);
  })

  LVALUE_RVALUE_METHOD_OVERLOAD(Derived, set_sheet_format(chima_image_format format), {
    CHIMA_ASSERT(!_is_empty(_chima));
    chima_set_sheet_format(_chima, format);
  })

  LVALUE_RVALUE_METHOD_OVERLOAD(Derived, set_sheet_name(const char* name), {
    CHIMA_ASSERT(!_is_empty(_chima));
    chima_set_sheet_name(_chima, name);
  })

  LVALUE_RVALUE_METHOD_OVERLOAD(Derived, set_sheet_color(chima_color color), {
    CHIMA_ASSERT(!_is_empty(_chima));
    chima_set_sheet_color(_chima, color);
    return static_cast<Derived&>(*this);
  })

protected:
  static bool _is_empty(const chima_context& chima) noexcept { return chima == nullptr; }

  static void _destroy(chima_context& chima) noexcept { chima_destroy_context(chima); }

  void _reset() noexcept { _chima = nullptr; }

protected:
  chima_context _chima;
};

} // namespace impl

// Non owning `chima_context`
class context_view : public impl::context_base<context_view> {
private:
  using base_t = impl::context_base<context_view>;

public:
  context_view(const chima_context& chima) : base_t{chima} {}
};

// Owning `chima_context` RAII wrapper.
// Make sure to destroy this AFTER every `chima_image` or `chima_spritesheet` object.
class context : public impl::context_base<context> {
private:
  using base_t = impl::context_base<context>;

public:
  explicit context(const chima_context& chima) : base_t{chima} {}

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

// Owning `chima_image` RAII wrapper.
// Does NOT free the associated `chima_context` when it gets destroyed.
class image {
public:
  image(chima_image image) : _image{image} {
    CHIMA_THROW_IF(!image.ctx, ::chima::error{chima_return::CHIMA_INVALID_VALUE});

    CHIMA_ASSERT(image.data != nullptr);
    CHIMA_ASSERT(image.height * image.width > 0);
    CHIMA_ASSERT(image.depth < chima_image_depth::CHIMA_DEPTH_COUNT);
    CHIMA_ASSERT(image.channels > 0 && image.channels <= 4);
  }

  image(chima_context chima, const char* path, const char* name = nullptr) {
    CHIMA_THROW_IF(!chima, ::chima::error{chima_return::CHIMA_INVALID_VALUE});
    const chima_return ret = chima_load_image(chima, &_image, name, path);
    CHIMA_THROW_IF(ret != chima_return::CHIMA_NO_ERROR, ::chima::error{ret});
  }

  image(chima_context chima, const uint8_t* data, size_t len, const char* name = nullptr) {
    CHIMA_THROW_IF(!chima, ::chima::error{chima_return::CHIMA_INVALID_VALUE});
    const chima_return ret = chima_load_image_mem(chima, &_image, name, data, len);
    CHIMA_THROW_IF(ret != chima_return::CHIMA_NO_ERROR, ::chima::error{ret});
  }

  ~image() noexcept { _destroy(); }

  image(image&& other) noexcept : _image{std::move(other._image)} { other._reset(); }

  image(const image&) noexcept = delete;

public:
  static std::optional<image> load(chima_context chima, const char* path,
                                   ::chima::error* err = nullptr,
                                   const char* name = nullptr) noexcept {
    CHIMA_ASSERT(chima);
    chima_image image;
    const chima_return ret = chima_load_image(chima, &image, name, path);
    if (ret != chima_return::CHIMA_NO_ERROR) {
      if (err) {
        *err = ret;
      }
      return {};
    }
    return {image};
  }

  static std::optional<image> load_from_mem(chima_context chima, const uint8_t* data, size_t len,
                                            ::chima::error* err = nullptr,
                                            const char* name = nullptr) noexcept {
    chima_image image;
    const chima_return ret = chima_load_image_mem(chima, &image, name, data, len);
    if (ret != chima_return::CHIMA_NO_ERROR) {
      if (err) {
        *err = ret;
      }
      return {};
    }
    return {image};
  }

public:
  image& operator=(image&& other) noexcept {
    _destroy();

    _image = std::move(other._image);
    other._reset();

    return *this;
  }

  image& operator=(const image&) noexcept = delete;

public:
  [[nodiscard]] chima_image release() noexcept {
    auto ret = _image;
    _reset();
    return ret;
  }

  const chima_image& get() const {
    CHIMA_ASSERT(!_moved_from());
    return _image;
  }

  chima_image& get() {
    CHIMA_ASSERT(!_moved_from());
    return _image;
  }

public:
  chima_context context() const {
    CHIMA_ASSERT(!_moved_from());
    return _image.ctx;
  }

  std::pair<uint32_t, uint32_t> extent() const {
    CHIMA_ASSERT(!_moved_from());
    return {_image.width, _image.height};
  }

  std::pair<void*, size_t> data_size() const {
    CHIMA_ASSERT(!_moved_from());
    return impl::calc_chima_image_data(_image);
  }

  void* data() const {
    CHIMA_ASSERT(!_moved_from());
    return _image.data;
  }

  std::string_view name() const {
    CHIMA_ASSERT(!_moved_from());
    return to_str_view(_image.name);
  }

public:
  image& write(const char* path, chima_image_format format) {
    CHIMA_ASSERT(!_moved_from());
    _do_write(path, format);
    return *this;
  }

  const image& write(const char* path, chima_image_format format) const {
    CHIMA_ASSERT(!_moved_from());
    _do_write(path, format);
    return *this;
  }

private:
  bool _moved_from() const noexcept { return _image.ctx == nullptr; }

  void _reset() noexcept { std::memset(&_image, 0, sizeof(_image)); }

  void _destroy() noexcept {
    if (!_moved_from()) {
      chima_destroy_image(&_image);
    }
  }

  void _do_write(const char* path, chima_image_format format) const {
    const chima_return ret = chima_write_image(&_image, path, format);
    CHIMA_THROW_IF(ret != chima_return::CHIMA_NO_ERROR, ::chima::error{ret});
  }

private:
  chima_image _image;
};

// Owning `chima_spritesheet` RAII wrapper.
// Does NOT free the associated `chima_context` when it gets destroyed.
class spritesheet {
public:
  spritesheet() noexcept = default;

  spritesheet(chima_spritesheet sheet) : _sheet{sheet} {
    CHIMA_THROW_IF(!sheet.ctx, ::chima::error{chima_return::CHIMA_INVALID_VALUE});
    CHIMA_ASSERT(sheet.sprites);
    CHIMA_ASSERT(sheet.sprite_count > 0);
  }

  spritesheet(chima_context chima, const char* path) {
    const chima_return ret = chima_load_spritesheet(chima, &_sheet, path);
    CHIMA_THROW_IF(ret != CHIMA_NO_ERROR, ::chima::error{ret});
  }

  ~spritesheet() noexcept { _destroy(); }

  spritesheet(spritesheet&& other) noexcept : _sheet{std::move(other._sheet)} { other._reset(); }

  spritesheet(const spritesheet&) noexcept = delete;

public:
  static std::optional<spritesheet> load(chima_context chima, const char* path,
                                         ::chima::error* err = nullptr) {
    chima_spritesheet sheet;
    const chima_return ret = chima_load_spritesheet(chima, &sheet, path);
    if (ret != CHIMA_NO_ERROR) {
      if (err) {
        *err = ret;
      }
      return {};
    }
    return {sheet};
  }

public:
  spritesheet& operator=(spritesheet&& other) noexcept {
    _destroy();

    _sheet = std::move(other._sheet);

    other._reset();

    return *this;
  }

  spritesheet& operator=(const spritesheet&) noexcept = delete;

public:
  [[nodiscard]] chima_spritesheet release() noexcept {
    auto ret = _sheet;
    _reset();
    return ret;
  }

  const chima_image& get_image() const {
    CHIMA_ASSERT(!_moved_from());
    return _sheet.atlas;
  }

  chima_image& get_image() {
    CHIMA_ASSERT(!_moved_from());
    return _sheet.atlas;
  }

  const chima_spritesheet& get() const {
    CHIMA_ASSERT(!_moved_from());
    return _sheet;
  }

  chima_spritesheet& get() {
    CHIMA_ASSERT(!_moved_from());
    return _sheet;
  }

public:
  chima_context context() const {
    CHIMA_ASSERT(!_moved_from());
    return _sheet.ctx;
  }

  std::pair<uint32_t, uint32_t> atlas_extent() const {
    CHIMA_ASSERT(!_moved_from());
    return {_sheet.atlas.width, _sheet.atlas.height};
  }

  std::pair<void*, size_t> atlas_data_size() const {
    CHIMA_ASSERT(!_moved_from());
    return impl::calc_chima_image_data(_sheet.atlas);
  }

  void* atlas_data() const {
    CHIMA_ASSERT(!_moved_from());
    return _sheet.atlas.data;
  }

  std::span<chima_sprite> sprites() const {
    CHIMA_ASSERT(!_moved_from());
    return {_sheet.sprites, _sheet.sprite_count};
  }

  std::span<chima_sprite_anim> anims() const {
    CHIMA_ASSERT(!_moved_from());
    return {_sheet.anims, _sheet.anim_count};
  }

public:
  spritesheet& write(const char* path) {
    CHIMA_ASSERT(!_moved_from());
    _do_write(path);
    return *this;
  }

  const spritesheet& write(const char* path) const {
    CHIMA_ASSERT(!_moved_from());
    _do_write(path);
    return *this;
  }

  spritesheet& write_atlas(const char* path, chima_image_format format) {
    CHIMA_ASSERT(!_moved_from());
    _do_write_image(path, format);
    return *this;
  }

  const spritesheet& write_atlas(const char* path, chima_image_format format) const {
    CHIMA_ASSERT(!_moved_from());
    _do_write_image(path, format);
    return *this;
  }

private:
  bool _moved_from() const noexcept { return _sheet.ctx == nullptr; }

  void _reset() noexcept { std::memset(&_sheet, 0, sizeof(_sheet)); }

  void _destroy() noexcept {
    if (!_moved_from()) {
      chima_destroy_spritesheet(&_sheet);
    }
  }

  void _do_write(const char* path) const {
    const chima_return ret = chima_write_spritesheet(&_sheet, path);
    CHIMA_THROW_IF(ret != chima_return::CHIMA_NO_ERROR, ::chima::error{ret});
  }

  void _do_write_image(const char* path, chima_image_format format) const {
    const chima_return ret = chima_write_image(&_sheet.atlas, path, format);
    CHIMA_THROW_IF(ret != chima_return::CHIMA_NO_ERROR, ::chima::error{ret});
  }

private:
  chima_spritesheet _sheet;
};

} // namespace chima

#undef LVALUE_RVALUE_METHOD_OVERLOAD
