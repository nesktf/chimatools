(local blob (require :chimatools.blob))
(local util (require :chimatools.util))
(local inspect (require :inspect))
(local ffi (require :ffi))
(local magick (require :magick))

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

(fn get-image-props [image]
  (let [props []
        wand image.wand
        arr (ffi.new "ssize_t[4]")
        num-images (tonumber (magick.lib.MagickGetNumberImages wand))]
    (magick.lib.MagickResetIterator wand)
    (for [i 0 (- num-images 1)]
      (magick.lib.MagickSetIteratorIndex wand i)
      (magick.lib.MagickGetImagePage wand arr (+ arr 1) (+ arr 2) (+ arr 3))
      (table.insert props
                    {:w (tonumber (. arr 0))
                     :h (tonumber (. (+ arr 1) 0))
                     :x (tonumber (. (+ arr 2) 0))
                     :y (tonumber (. (+ arr 3) 0))}))
    props))

(fn calc-atlas-sz [sprites initial-size]
  2200)

(fn generate-atlas [sprites]
  (local pad 5)
  (var x-pos 0)
  (var y-pos 0)
  (var max-y 0)
  (let [atlas-sz (calc-atlas-sz sprites 512)
        image (magick.new_from_color atlas-sz atlas-sz 1 1 1 0)]
    (image:set_format "png")
    (each [_i sprite (ipairs sprites)]
      (when (>= (+ x-pos sprite.width pad) atlas-sz)
        (set x-pos 0)
        (set y-pos (+ y-pos max-y pad))
        (set max-y 0))
      (local x-off (+ x-pos sprite.local-x-off))
      (local y-off (+ y-pos sprite.local-y-off))
      (magick.lib.MagickSetIteratorIndex sprite.magick-img.wand sprite.img-idx)
      (util.chima-log "Compositing sprite %s" sprite.name)
      (image:composite sprite.magick-img x-off y-off)
      (set x-pos (+ x-pos sprite.width pad))
      (set max-y (math.max max-y sprite.height)))
    (util.chima-log "Writing test image")
    (image:write "res/test.png")
    (util.chima-assert false "STOP")
    {: image}))

(fn parse-images [images]
  (local parsed-sprites [])
  (local parsed-anims [])
  (var image-count 0)
  (each [_i image (ipairs images)]
    (let [file-format (util.parse-file-format image.path)
          magick-img (magick.load_image image.path)
          props (get-image-props magick-img)
          prop-count (length props)
          anim? (> prop-count 1)]
      (when anim?
        (table.insert parsed-anims
                      {:sprite-idx image-count
                       :sprite-count prop-count
                       :name image.name})
        (each [img-idx prop (ipairs props)]
          (local name (.. image.name (string.format ".%d" img-idx)))
          (table.insert parsed-sprites
                        {: file-format
                         : name
                         : magick-img
                         :img-idx (- img-idx 1)
                         :width prop.w
                         :height prop.h
                         :local-x-off prop.x
                         :local-y-off prop.y})
          (set image-count (+ image-count 1))))
      (when (not anim?) ; anti functional hack
        (local name image.name)
        (local prop (. props 1))
        (table.insert parsed-sprites
                      {: file-format
                       : name
                       : magick-img
                       :img-idx 0
                       :width prop.w
                       :height prop.h
                       :local-x-off prop.x
                       :local-y-off prop.y}))))
  {:sprites parsed-sprites :anims parsed-anims})

(fn create-spritesheet [image-list]
  (let [{: sprites : anims} (parse-images image-list)
        {: image : uvs : offsets} (generate-atlas sprites)
        sprites-uvs (util.tbl-kmap sprites
                                   (fn [i sprite]
                                     {:width sprite.width
                                      :height sprite.height
                                      :x-off (. offsets i :x)
                                      :y-off (. offsets i :y)
                                      :uv-x-lin (. uvs i :x-lin)
                                      :uv-x-con (. uvs i :x-con)
                                      :uv-y-lin (. uvs i :y-lin)
                                      :uv-y-con (. uvs i :y-con)
                                      :name sprite.name}))]
    (create-spritesheet-blob sprites-uvs anims image)))

{:create-spritesheet (λ [image-list]
                       (case (pcall create-spritesheet image-list)
                         (true sheet-blob) sheet-blob
                         (false err) (values nil err)))}
