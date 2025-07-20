(local ffi (require :ffi))
(local {: lib : check-err} (require :chimatools.lib))
(local sprite (require :chimatools.sprite))

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

(local max-str-sz 256)
(local str-mt {:__tostring (fn [self]
                             (ffi.string self.data self.length))})

(set str-mt.__index str-mt)
(local str-ctype (ffi.metatype :chima_string str-mt))
(local str {:_ctype str-ctype
            :new (fn [luastr]
                   (let [strsz (length luastr)]
                     (if (<= strsz max-str-sz)
                         (let [out (ffi.new str-ctype)]
                           (ffi.copy out.data luastr)
                           (set out.length strsz)
                           out)
                         (values nil "String is too big"))))})

(local chima-context-mt
       {:set_atlas_factor (fn [self factor]
                            (lib.chima_set_atlas_factor self factor))
        :set_image_y_flip (fn [self flip_y]
                            (lib.chima_set_image_y_flip self flip_y))
        :set_image_uv_y_flip (fn [self flip_y]
                               (lib.chima_set_uv_y_flip self flip_y))
        :set_image_uv_x_flip (fn [self flip_x]
                               (lib.chima_set_uv_x_flip self flip_x))
        :set_atlas_color_comp (fn [self r g b a]
                                (lib.chima_set_sheet_color self
                                                           (color:new r g b a)))
        :set_atlas_color (fn [self color]
                           (lib.chima_set_sheet_color self color))
        :set_atlas_name (fn [self name]
                          (lib.chima_set_sheet_name self name))
        :set_atlas_initial (fn [self initial_sz]
                             (lib.chima_set_sheet_initial self initial_sz))})

(set chima-context-mt.__index chima-context-mt)

(local chima-context-ctype
       (ffi.metatype "struct chima_context_" chima-context-mt))

(local chima-context
       {:_ctype chima-context-ctype
        :new (fn []
               ;; Luajit quirks for opaque handles
               (let [ctx (ffi.new "struct chima_context_*[1]")]
                 (case (check-err (lib.chima_create_context ctx nil))
                   nil (ffi.gc (. ctx 0) lib.chima_destroy_context)
                   (err ret) (values err ret))))})

{:context chima-context
 : color
 :string str
 :image sprite.image
 :anim sprite.anim
 :sprite sprite.sprite
 :spritesheet sprite.spritesheet
 :sprite_anim sprite.sprite_anim
 :_lib lib}
