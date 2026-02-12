(local ffi (require :ffi))
(local {: lib : check-err : color} (require :chimatools.lib))
(local {: image : anim} (require :chimatools.image))
(local {: sheet_data : spritesheet : sprite : sprite_anim}
       (require :chimatools.spritesheet))

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
        :set_image_flip_y (fn [self flag]
                            (lib.chima_set_image_y_flip self flag))
        :set_atlas_initial (fn [self size]
                             (lib.chima_set_atlas_initial self size))})

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
                   (err ret) (values nil err ret))))})

{:context chima-context
 : color
 :string str
 : image
 : anim
 : sheet_data
 : spritesheet
 : sprite
 : sprite_anim
 :_lib lib}
