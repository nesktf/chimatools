(local ffi (require :ffi))
(local {: lib : check-err : color} (require :chimatools.lib))

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

(fn gc-wrap-image [chima image]
  (ffi.gc image #(lib.chima_destroy_image chima $1)))

(local image-ctype (ffi.metatype :chima_image image-mt))
(local image {:_ctype image-ctype
              :format {:raw 0 :png 1 :bmp 2 :tga 3}
              :depth {:u8 0 :u16 1 :f32 2}
              :new (λ [chima w h ch ?depth ?background-color]
                     (let [img (ffi.new image-ctype)
                           depth (or ?depth 0)
                           col (or ?background-color (color.new 0 0 0 0))]
                       (case (check-err (lib.chima_gen_blank_image chima img w
                                                                   h ch depth
                                                                   col))
                         nil (gc-wrap-image chima img)
                         (err ret) (values nil err ret))))
              :new_atlas (λ [chima images padding ?background-color]
                           (let [atlas (ffi.new image-ctype)
                                 img-count (length images)
                                 rects (ffi.new "chima_rect[?]" img-count)
                                 col (or ?background-color (color.new 0 0 0 0))]
                             (case (check-err (lib.chima_gen_atlas_image chima
                                                                         atlas
                                                                         padding
                                                                         col
                                                                         images
                                                                         img-count))
                               nil {: rects :atlas (gc-wrap-image chima atlas)}
                               (err ret) (values nil err ret))))
              :load (λ [chima ?depth path]
                      (let [img (ffi.new image-ctype)
                            depth (or ?depth 0)]
                        (case (check-err (lib.chima_load_image chima img depth
                                                               path))
                          nil (gc-wrap-image chima img)
                          (err ret) (values nil err ret))))})

(local anim-mt {})
(set anim-mt.__index anim-mt)
(local anim-ctype (ffi.metatype :chima_anim anim-mt))
(local anim {:_ctype anim-ctype
             :load (λ [chima path]
                     (let [anim (ffi.new anim-ctype)]
                       (case (check-err (lib.chima_load_anim chima anim path))
                         nil (ffi.gc anim
                                     #(lib.chima_destroy_image_anim chima $1))
                         (err ret) (values nil err ret))))})

{: image : anim}
