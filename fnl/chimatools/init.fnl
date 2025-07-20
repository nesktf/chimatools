(local ffi (require :ffi))
(local {: lib : check-err} (require :chimatools.lib))
(local sprite (require :chimatools.sprite))

(local color-mt {})
(local color-ctype (ffi.metatype :chima_color color-mt))
(local color {:_ctype color-ctype
              :new (fn [r g b a]
                     (let [col (ffi.new color-ctype)]
                       (set (. col :r) (or r 0))
                       (set (. col :g) (or g 0))
                       (set (. col :b) (or b 0))
                       (set (. col :a) (or a 0))
                       col))})

(local chima-context-mt
       {:set_atlas_factor (fn [self factor]
                            (lib.chima_set_atlas_factor self factor))
        :set_y_flip (fn [self flip_y]
                      (lib.chima_set_y_flip self flip_y))
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
 :image sprite.image
 :anim sprite.anim
 :sprite sprite.sprite
 :spritesheet sprite.spritesheet
 :sprite_anim sprite.sprite_anim}
