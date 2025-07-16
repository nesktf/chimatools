(local chimatools (require :chimatools))

(local images [{:path :res/cino.gif :name :chiruno_chan :fps 10}
               {:path :res/mariass.gif :name :marisa_dumpy :fps 20}
               {:path :res/honk.gif :name :cheeeeeen :fps 50}
               {:path :res/chimata.png :name :chimata}
               {:path :res/marisa_emacs.png :name :marisa_emacs}])

(case (chimatools.write_spritesheet images :res/chimaout.chima)
  chima-path (print (string.format "Sprite chima file written to %s" chima-path))
  (nil err) (print err))
