(local chima (require :chimatools))
(local ctx (chima.context.new))

(local images [{:path :res/chimata.png :name :chimata}
               {:path :res/marisa_emacs.png :name :marisa_emacs}])

(local anims [{:path :res/cino.gif :name :chiruno_chan}
              {:path :res/mariass.gif :name :marisa_ass}
              {:path :res/honk.gif :name :cheeeeeen}])

(fn chima-parse []
  (let [chima-images []
        chima-anims []]
    (each [_i image (ipairs images)]
      (let [chima-image (chima.image.load ctx image.name image.path)]
        (table.insert chima-images chima-image)))
    (each [_i anim (ipairs anims)]
      (let [chima-anim (chima.anim.load ctx anim.name anim.path)]
        (table.insert chima-anims chima-anim)))
    {: chima-images : chima-anims}))

(let [{: chima-images : chima-anims} (chima-parse)
      chima-sheet (chima.spritesheet.new ctx 0 chima-images chima-anims)
      sheet-sz (tonumber chima-sheet.atlas.width)
      sprite-count (tonumber chima-sheet.sprite_count)]
  (print (string.format "SIZE: %dx%d" sheet-sz sheet-sz))
  (for [i 0 (- sprite-count 1)]
    (let [sprite (. chima-sheet.sprites i)
          name (tostring sprite.name)
          x (tonumber sprite.x_off)
          y (tonumber sprite.y_off)
          w (tonumber sprite.width)
          h (tonumber sprite.height)
          uv_y_lin (tonumber sprite.uv_y_lin)
          uv_x_lin (tonumber sprite.uv_x_lin)
          uv_y_con (tonumber sprite.uv_y_con)
          uv_x_con (tonumber sprite.uv_x_con)]
      (print (string.format "- %s: (%dx%d),(%d,%d) -> (%f + %f, %f + %f)" name
                            w h x y uv_x_lin uv_x_con uv_y_lin uv_y_con)))))
