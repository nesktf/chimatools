(local ffi (require :ffi))
(local {: lib : check-err : gc-wrap} (require :chimatools.lib))

(local image-mt {:composite (λ [self other x y]
                              (case (check-err (lib.chima_composite_image self
                                                                          other
                                                                          x y))
                                nil nil
                                (err ret) (values err ret)))
                 :write (λ [self path format]
                          (case (check-err (lib.chima_write_image self path
                                                                  format))
                            nil nil
                            (err ret) (values err ret)))})

(set image-mt.__index image-mt)

(local image-ctype (ffi.metatype :chima_image image-mt))
(local image {:_ctype image-ctype
              :format {:raw 0 :png 1 :bmp 2 :tga 3}
              :new (λ [chima w h ch color ?name]
                     (let [img (ffi.new image-ctype)]
                       (case (check-err (lib.chima_create_image chima img w h
                                                                ch color ?name))
                         nil (gc-wrap chima img lib.chima_destroy_image)
                         (err ret) (values err ret))))
              :load (λ [chima ?name path]
                      (let [img (ffi.new image-ctype)]
                        (case (check-err (lib.chima_load_image chima img ?name
                                                               path))
                          nil (gc-wrap chima img lib.chima_destroy_image)
                          (err ret) (values err ret))))})

(local anim-mt {})
(set anim-mt.__index anim-mt)
(local anim-ctype (ffi.metatype :chima_anim anim-mt))
(local anim {:_ctype anim-ctype
             :load (λ [chima ?name path]
                     (let [anim (ffi.new anim-ctype)]
                       (case (check-err (lib.chima_load_anim chima anim ?name
                                                             path))
                         nil (gc-wrap chima anim lib.chima_destroy_anim)
                         (err ret) (values err ret))))})

(local spritesheet-mt
       {:write (λ [self path]
                 (case (check-err (lib.chima_write_spritesheet self path))
                   nil nil
                   (err ret) (values err ret)))})

(set spritesheet-mt.__index spritesheet-mt)

(local spritesheet-ctype (ffi.metatype :chima_spritesheet spritesheet-mt))

(local spritesheet
       {:_ctype spritesheet-ctype
        :new (λ [chima pad images animations]
               (let [img-len (length images)
                     imgs (ffi.new "chima_image[?]" img-len)
                     anim-len (length animations)
                     anims (ffi.new "chima_anim[?]" anim-len)
                     sheet (ffi.new spritesheet-ctype)]
                 (each [i image (ipairs images)]
                   (ffi.copy (+ imgs (- i 1)) image (ffi.sizeof :chima_image)))
                 (each [i anim (ipairs animations)]
                   (ffi.copy (+ anims (- i 1)) anim (ffi.sizeof :chima_anim)))
                 (case (check-err (lib.chima_create_spritesheet chima sheet pad
                                                                imgs img-len
                                                                anims anim-len))
                   nil (gc-wrap chima sheet lib.chima_destroy_spritesheet)
                   (err ret) (values err ret))))
        :load (λ [chima path]
                (let [sheet (ffi.new spritesheet-ctype)]
                  (case (check-err (lib.chima_load_spritesheet chima sheet path))
                    nil (gc-wrap chima sheet lib.chima_destroy_spritesheet)
                    (err ret) (values err ret))))})

(local chima-sprite-mt {})
(set chima-sprite-mt.__index chima-sprite-mt)
(local sprite-ctype (ffi.metatype :chima_sprite chima-sprite-mt))
(local sprite {:_ctype sprite-ctype
               :new (fn [name]
                      (let [sprite (ffi.new sprite-ctype)]
                        (ffi.fill sprite (ffi.sizeof sprite-ctype))
                        (when name
                          (local name-len (length name))
                          (ffi.copy sprite.name.data name name-len)
                          (set sprite.name.length name-len))
                        sprite))})

(local sprite-anim-mt {})
(set sprite-anim-mt.__index sprite-anim-mt)
(local sprite-anim-ctype (ffi.metatype :chima_sprite_anim sprite-anim-mt))
(local sprite_anim {:_ctype sprite-anim-ctype
                    :new (fn [name]
                           (let [anim (ffi.new sprite-anim-ctype)]
                             (ffi.fill anim (ffi.sizeof sprite-anim-ctype))
                             (when name
                               (local name-len (length name))
                               (ffi.copy anim.name.data name name-len)
                               (set anim.name.length name-len))
                             anim))})

{: image : anim : spritesheet : sprite : sprite_anim}
