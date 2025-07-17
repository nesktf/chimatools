(local ffi (require :ffi))
(local util (require :chimatools.util))

;; TODO: Maybe don't abuse the FFI C structs for writing data? (lol)
(ffi.cdef "
  typedef enum chima_asset_type {
    CHIMA_ASSET_INVALID = 0,
    CHIMA_ASSET_SPRITESHEET,
    CHIMA_ASSET_MODEL3D,
    CHIMA_ASSET_COUNT,

    _CHIMA_ASSET_FORCE_16BIT = 0xFFFF,
  } chima_asset_type;

  typedef struct chima_header_t {
    uint8_t magic[12];
    uint16_t file_enum;
    uint8_t ver_maj;
    uint8_t ver_min;
  } chima_header_t; // sizeof: 16 bytes

  typedef struct chima_sprite_meta_t {
    uint32_t sprite_count;
    uint32_t anim_count;
    uint32_t image_width;
    uint32_t image_height;
    uint32_t image_offset;
    uint8_t image_channels;
    char image_format[7]; // Up to 6 chars + null terminator
  } chima_sprite_meta_t; // sizeof: 28 bytes

  typedef struct chima_sprite_t {
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
  } chima_sprite_t; // sizeof: 40 bytes

  // sprite_uv.x = atlas_tex_uv.x*uv_x_lin + uv_x_con;
  // sprite_uv.y = atlas_tex_uv.y*uv_y_lin + uv_y_con;

  typedef struct chima_anim_t {
    uint32_t sprite_idx;
    uint32_t sprite_count;
    uint32_t name_offset;
    uint32_t name_size;
    float fps;
  } chima_anim_t; // sizeof: 20 bytes
")

(local sizes {:header (ffi.sizeof :chima_header_t)
              :sprite-meta (ffi.sizeof :chima_sprite_meta_t)
              :sprite (ffi.sizeof :chima_sprite_t)
              :anim (ffi.sizeof :chima_anim_t)})

(util.chima-assert (= sizes.anim 20))
(util.chima-assert (= sizes.header 16))
(util.chima-assert (= sizes.sprite 40))
(util.chima-assert (= sizes.sprite-meta 28))

(local chima-magic "CHIMA.ASSET")
(local chima-version [0 1])
(local file-enum {:sprite 1 :model 2})

(fn fill-chima-header [file-type]
  "Create a chima file header blob"
  (let [data (ffi.new :chima_header_t)
        enum (util.chima-assert (. file-enum file-type))
        chima-magic-len 11]
    (util.chima-assert (= (length chima-magic) chima-magic-len))
    (set (. data :magic 0) 0xFF)
    (ffi.copy (+ data.magic 1) chima-magic chima-magic-len)
    (set data.file_enum enum)
    (set data.ver_maj (. chima-version 1))
    (set data.ver_min (. chima-version 2))
    (ffi.string data sizes.header)))

(fn calc-array-offsets [string-list sizes blob-offset]
  (let [sum-off-until (fn [idx]
                        (var off blob-offset)
                        (for [i 1 (- idx 1)]
                          (set off (+ off (. sizes i) blob-offset)))
                        off)]
    (util.tbl-kmap string-list
                   (fn [i _val]
                     (sum-off-until i)))))

(fn fill-strings [string-list blob-offset]
  "Create a string blob with offsets from a list of strings and an initial byte offset"
  "The blob is in the format (len, data...)"
  (let [sizes (util.tbl-map string-list
                            (fn [val]
                              (length val)))
        offsets (calc-array-offsets string-list sizes blob-offset)
        string-data (util.tbl-reduce string-list
                                     (fn [str val]
                                       (.. str val))
                                     "")
        data-len (length string-data)
        bytes-len (ffi.new "uint32_t[1]")
        bytes-data (ffi.new "uint8_t[?]" data-len)]
    (set (. bytes-len 0) data-len)
    (ffi.copy bytes-data string-data data-len)
    {: sizes
     : offsets
     :data (.. (ffi.string bytes-len 4) (ffi.string bytes-data data-len))
     :size (+ 4 data-len)}))

(fn fill-indices [indices-list blob-offset]
  "Create an indices blob from a list of lists of indices and an initial byte offset"
  "The blob is in the format (len, data...)"
  (let [sizes (util.tbl-map indices-list
                            (fn [indices]
                              (length indices)))
        offsets (calc-array-offsets indices-list sizes blob-offset)
        idx-count (util.tbl-reduce sizes
                                   (fn [sz val]
                                     (+ sz val))
                                   1)
        bytes-size (* idx-count 4)
        bytes-data (ffi.new "uint32_t[?]" idx-count)]
    (set (. bytes-data 0) idx-count)
    (var byte-pos 1)
    (each [_i indices (ipairs indices-list)]
      (each [_j idx (ipairs indices)]
        (set (. bytes-data byte-pos) idx)
        (set byte-pos (+ byte-pos 1))))
    {: sizes
     : offsets
     :data (ffi.string bytes-data bytes-size)
     :size bytes-size}))

(fn fill-sprite-meta [meta]
  "Create a sprite metadata blob"
  (let [data (ffi.new :chima_sprite_meta_t)
        data-format-len 7
        format-len (length meta.image-format)]
    (util.chima-assert (< format-len data-format-len) "Invalid format size")
    (set data.sprite_count meta.sprite-count)
    (set data.anim_count meta.anim-count)
    (set data.image_width meta.image-width)
    (set data.image_height meta.image-height)
    (set data.image_offset meta.image-offset)
    (set data.image_channels meta.image-channels)
    (ffi.copy data.image_format meta.image-format format-len)
    (ffi.fill (+ data.image_format format-len) (- data-format-len format-len))
    (ffi.string data sizes.sprite-meta)))

(fn fill-sprite [sprite-list sprite-count strings]
  "Create a sprite data blob"
  (let [sprite-arr (ffi.new "chima_sprite_t[?]" sprite-count)
        blob-size (* sprite-count sizes.sprite)]
    (each [i sprite (ipairs sprite-list)]
      (let [data (. sprite-arr i)
            name-size (. strings :sizes i)
            name-offset (. strings :offsets i)]
        (set data.width sprite.width)
        (set data.height sprite.height)
        (set data.x_off sprite.x-off)
        (set data.y_off sprite.y-off)
        (set data.uv_x_lin sprite.uv-x-lin)
        (set data.uv_x_con sprite.uv-x-con)
        (set data.uv_y_lin sprite.uv-y-lin)
        (set data.uv_y_con sprite.uv-y-con)
        (set data.name_offset name-offset)
        (set data.name_size name-size)))
    (ffi.string sprite-arr blob-size)))

(fn fill-anim [anim-list anim-count sprite-count strings]
  "Create an animation data blob"
  (let [anim-arr (ffi.new "chima_anim_t[?]" anim-count)
        blob-size (* anim-count sizes.anim)]
    (each [i anim (ipairs anim-list)]
      (let [data (. anim-arr i)
            name-size (. strings :sizes (+ sprite-count i))
            name-offset (. strings :offsets (+ sprite-count i))]
        (set data.sprite_idx anim.sprite-idx)
        (set data.sprite_count anim.sprite-count)
        (set data.name_offset name-offset)
        (set data.name_size name-size)
        (set data.fps anim.fps)))
    (ffi.string anim-arr blob-size)))

{: fill-chima-header
 : fill-strings
 : fill-indices
 : fill-sprite-meta
 : fill-sprite
 : fill-anim
 : sizes}
