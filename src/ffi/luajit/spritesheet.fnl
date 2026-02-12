(local ffi (require :ffi))
(local {: lib : check-err : image : color} (require :chimatools.lib))

(local sheet-data-mt
       {:add_image (λ [self image name]
                     (case (check-err (lib.chima_sheet_add_image self image
                                                                 name))
                       nil nil
                       (err ret) (values err ret)))
        :add_images (λ [self images frametimes basename]
                      (let [img-count (length images)
                            times (ffi.new "uint32_t[?]" img-count)]
                        (assert (= img-count (length frametimes)))
                        (each [i frametime (ipairs frametimes)]
                          (ffi.copy (+ times (- i 1) frametime
                                       (ffi.sizeof :uint32_t))))
                        (case (check-err (lib.chima_sheet_add_images self
                                                                     images
                                                                     times
                                                                     img-count
                                                                     basename))
                          nil nil
                          (err ret) (values err ret))))
        :add_anim (λ [self anim basename]
                    (case (check-err (lib.chima_sheet_add_anim self anim
                                                               basename))
                      nil nil
                      (err ret) (values err ret)))})

(set sheet-data-mt.__index sheet-data-mt)
(local sheet-data-ctype (ffi.metatype "struct chima_sheet_data_" sheet-data-mt))

(local sheet_data
       {:_ctype sheet-data-ctype
        :new (λ [chima]
               ;; Luajit quirks for opaque handles
               (let [data (ffi.new "struct chima_sheet_data_*[1]")]
                 (case (check-err (lib.chima_create_sheet_data chima data))
                   nil (ffi.gc data
                               (fn []
                                 (let [_chima-extend-life chima]
                                   (lib.chima_destroy_sheet_data (. data 0)))))
                   (err ret) (values nil err ret))))})

(local spritesheet-mt
       {:write (λ [self path format]
                 (case (check-err (lib.chima_write_spritesheet self path format))
                   nil nil
                   (err ret) (values err ret)))})

(fn sheet-gc-wrap [chima sheet]
  (ffi.gc sheet #(lib.chima_destroy_spritesheet chima $1)))

(set spritesheet-mt.__index spritesheet-mt)
(local spritesheet-ctype (ffi.metatype :chima_spritesheet spritesheet-mt))
(local spritesheet
       {:_ctype spritesheet-ctype
        :format image.format
        :new (λ [chima data padding ?background-color]
               (let [sheet (ffi.new spritesheet-ctype)
                     col (or ?background-color (color.new 0 0 0 0))]
                 (case (check-err (lib.chima_gen_spritesheet chima sheet data
                                                             padding col))
                   nil (sheet-gc-wrap chima sheet)
                   (err ret) (values nil err ret))))
        :load (λ [chima path]
                (let [sheet (ffi.new spritesheet-ctype)]
                  (case (check-err (lib.chima_load_spritesheet chima sheet path))
                    nil (sheet-gc-wrap chima sheet)
                    (err ret) (values err ret))))})

(local chima-sprite-mt {})
(set chima-sprite-mt.__index chima-sprite-mt)
(local sprite-ctype (ffi.metatype :chima_sprite chima-sprite-mt))
(local sprite {:_ctype sprite-ctype})

(local sprite-anim-mt {})
(set sprite-anim-mt.__index sprite-anim-mt)
(local sprite-anim-ctype (ffi.metatype :chima_sprite_anim sprite-anim-mt))
(local sprite_anim {:_ctype sprite-anim-ctype})

{: sheet_data : spritesheet : sprite : sprite_anim}
