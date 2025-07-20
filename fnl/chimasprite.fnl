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
      chima-sheet (chima.spritesheet.new ctx 0 chima-images chima-anims)]
  (chima-sheet.atlas:write "test.png" chima.image.format.png)
  (chima-sheet:write "test.chima"))
