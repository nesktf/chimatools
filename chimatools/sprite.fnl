(local blob (require :chimatools.blob))
(local util (require :chimatools.util))

(fn create-spritesheet-blob [sprite-list anim-list image]
  (let [parse-sprite-names (fn []
                             (local names [])
                             (each [_i sprite (ipairs sprite-list)]
                               (table.insert names sprite.name))
                             (each [_i anim (ipairs anim-list)]
                               (table.insert names anim.name))
                             names)
        sprite-count (length sprite-list)
        sprite-data-sz (* sprite-count blob.sizes.sprite)
        anim-count (length anim-list)
        anim-data-sz (* anim-count blob.sizes.anim)
        names (parse-sprite-names sprite-list anim-list)
        string-offset (+ blob.sizes.header blob.sizes.sprite-meta
                         sprite-data-sz anim-data-sz)
        strings (blob.fill-strings names string-offset)
        header-data (blob.fill-chima-header :sprite)
        image-offset (+ string-offset strings.size)
        meta-data (blob.fill-sprite-meta {: sprite-count
                                          : anim-count
                                          : image-offset
                                          :image-width image.width
                                          :image-height image.height
                                          :image-channels image.channels
                                          :image-format image.format})
        sprite-data (blob.fill-sprite sprite-list sprite-count strings)
        anim-data (blob.fill-anim anim-list anim-count sprite-count strings)
        string-data strings.data
        image-data image.data
        blob-sz (+ image-offset image.size)]
    (util.chima-log "Sprite blob created (%d bytes)" blob-sz)
    {:data (.. header-data meta-data sprite-data anim-data string-data
               image-data)
     :size blob-sz}))

(local ffi (require :ffi))
(local magick (require :magick))

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

(fn generate-atlas [sprites]
  {})

(fn parse-images [images]
  {})

(local format-checks
       {:png (fn [...])
        :gif (fn [image i]
               (util.chima-assert (= (type image.fps) :number)
                                  "Invalid fps at animation %d" i))})

(fn validate-images [images]
  (each [i image (ipairs images)]
    (util.chima-assert (= (type image.path) :string) "Invalid path at image %d"
                       i)
    (util.chima-assert (util.file-exists image.path)
                       "File \"%s\" not found at image %d" image.path i)
    (util.chima-assert (= (type image.name) :string) "Invalid name at image %d"
                       i)
    (let [image-format (util.parse-file-format image.path)
          checker (. format-checks image-format)]
      (util.chima-assert (not= checker nil)
                         "Format \"%s\" not supported at image %d" image-format
                         i)
      (checker image i)
      (set image.format image-format)))
  images)

(λ create-spritesheet [image-list]
  (case (pcall validate-images image-list)
    (true images) (let [{: sprites : anims} (parse-images images)
                        image (generate-atlas sprites)]
                    (create-spritesheet-blob sprites anims image))
    (false err) (values nil err)))

{: create-spritesheet}
