(local common (require :chimatools.common))

(local ffi (require :ffi))
(local magick (require :magick))
(local inspect (require :inspect))

(fn image-props [image]
  (let [arr (ffi.new "ssize_t[4]")]
    (magick.lib.MagickGetImagePage image.wand arr (+ arr 1) (+ arr 2) (+ arr 3))
    {:w (tonumber (. arr 0))
     :h (tonumber (. (+ arr 1) 0))
     :x (tonumber (. (+ arr 2) 0))
     :y (tonumber (. (+ arr 3) 0))}))

(fn paper-from-gif [image]
  (let [{: w : h} (image-props image)
        num-images (tonumber (magick.lib.MagickGetNumberImages image.wand))]
    (magick.new_from_color (* w num-images) h 1 1 1 1)))

(fn handle-image [image]
  (magick.lib.MagickResetIterator image.wand)
  (let [num-images (tonumber (magick.lib.MagickGetNumberImages image.wand))
        paper (paper-from-gif image)]
    (paper:set_format :png)
    (var x-pos 0)
    (for [i 0 (- num-images 1)]
      (magick.lib.MagickSetIteratorIndex image.wand i)
      (print (magick.lib.MagickGetImageDelay image.wand))
      (let [{: w : h : x : y} (image-props image)
            sz (ffi.new "size_t[1]")
            prop (magick.lib.MagickGetImageProperties image.wand "*" sz)]
        (print (tonumber (. sz 0)) (ffi.string prop))
        (magick.lib.MagickRelinquishMemory prop)
        (paper:composite image (+ x-pos x) y)
        (set x-pos (+ x-pos w))
        (print (string.format "Image %d: %dx%d %d,%d" i w h x y))))
    (paper:write :res/chimaout.png)
    (print :CHIMADONE)))

(fn gen-blob [image-list]
  (let [blob-header (common.make-chima-header :sprite)]
    blob-header))

(fn validate-image-list [image-list]
  image-list)

(fn create-spritesheet-blob [image-list]
  (case (validate-image-list image-list)
    images (gen-blob images)
    (nil err) (values nil err)))

{: create-spritesheet-blob}
