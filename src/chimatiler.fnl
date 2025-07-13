(local magick (require :magick))
(local assimp (require :assimp))

(local in-file "chimata.png")
(local out-file "chimaout")
(local out-fmt "png")

;; GIMP 300ppi A4
(local paper-size {:w 2480 :h 3508})
(local paper-margin {:x 100 :y 100})
(local tiling-spacing {:x 20 :y 20})

(fn px->mm [px ppi]
  (* (/ px ppi) 25.4))

(fn mm->px [mm ppi]
  (math.floor (/ (* mm ppi) 25.4)))

(λ mm->str [ppi mm-w ?mm-h]
  (let [px-w (mm->px mm-w ppi)
        px-h (mm->px (or ?mm-h mm-w) ppi)]
    (string.format "%dx%d" px-w px-h)))

(fn calc-tiles [image w h paper-margin tile-spacing]
  (let [img-width (+ (image:get_width) tile-spacing.x)
        img-height (+ (image:get_height) tile-spacing.y)
        paper-width (- w paper-margin.x)
        paper-height (- h paper-margin.y)]
    {:cols (math.floor (/ paper-width img-width))
     :rows (math.floor (/ paper-height img-height))}))

; (fn make-tiled-full [image format w h mx my sx sy]
;   (let [paper (magick.new_from_color w h 1 1 1 1)
;         stride-w (+ (image:get_width) sx)
;         stride-h (+ (image:get_height) sy)
;         max-w (- w (* mx 2) stride-w)
;         max-h (- h (* my 2) stride-h)]
;     (paper:set_format format)
;     (for [x mx max-w stride-w]
;       (for [y my max-h stride-h]
;         (paper:composite image x y)))
;     paper))

(fn make-tiled-idx [image idx format w h tiles margin spacing]
  (let [paper (magick.new_from_color w h 1 1 1 1)
        cols tiles.cols
        row (% idx cols)
        col (math.floor (/ idx cols))
        {:x sx :y sy} spacing
        {:x mx :y my} margin
        stride-w (* (+ (image:get_width) sx) row)
        stride-h (* (+ (image:get_height) sy) col)]
    (paper:set_format format)
    (paper:composite image (+ mx stride-w) (+ my stride-h))
    paper))

(fn write-thing! [chima-in]
  (let [{: w : h} paper-size
        tiles (calc-tiles chima-in w h paper-margin tiling-spacing)
        chima-out (make-tiled-idx chima-in 0 out-fmt w h tiles paper-margin
                                  tiling-spacing)]
    (chima-out:write (string.format "%s.%s" out-file out-fmt))))

(fn on-die [msg code]
  (print (string.format "%s" msg))
  (os.exit (or code 1)))

(case (assimp.import_file "../game_test/res/chiruno/chiruno.gltf")
  scene (print (scene:get_name))
  (nil err) (print "ASSIMP: " err))

; (case (magick.load_image_from_blob (magick.thumb in-file (mm->str 300 40)))
;   chima-in (write-thing! chima-in)
;   (nil err) (on-die err))
