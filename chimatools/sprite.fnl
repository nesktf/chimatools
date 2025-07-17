(local blob (require :chimatools.blob))
(local util (require :chimatools.util))
(local inspect (require :inspect))
(local ffi (require :ffi))
(local magick (require :magick))

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

(fn calc-uvs [atlas-w atlas-h width height x-off y-off]
  (let [x-lin (/ width atlas-h)
        y-lin (/ height atlas-h)
        x-con (/ (+ x-off width) atlas-w)
        y-con (/ (+ y-off height) atlas-h)]
    {: x-lin : y-lin : x-con : y-con}))

(fn generate-atlas [sprites]
  (util.chima-log "SPRITES: %s" (inspect sprites)) ; (util.chima-assert false "STOP")
  (let [pad 0
        offsets []
        uvs []
        atlas-sz (calc-atlas-sz sprites 512)
        image (magick.new_from_color atlas-sz atlas-sz 1 1 1 0)]
    (var x-pos 0)
    (var y-pos 0)
    (var max-y 0)
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
      (table.insert offsets {:x x-off :y y-off})
      (table.insert uvs (calc-uvs atlas-sz atlas-sz sprite.width sprite.height
                                  x-pos y-pos))
      (set x-pos (+ x-pos sprite.width pad))
      (set max-y (math.max max-y sprite.height)))
    (util.chima-log "Writing test image")
    (image:write "res/test.png")
    {: image : uvs : offsets}))

(fn collect-anim-sprites [magick-img name props]
  (let [sprites []]
    (each [img-idx prop (ipairs props)]
      (local sprite-name (.. name (string.format ".%d" img-idx)))
      (table.insert sprites {:name sprite-name
                             : magick-img
                             :anim name
                             :img-idx (- img-idx 1)
                             :width prop.w
                             :height prop.h
                             :local-x-off prop.x
                             :local-y-off prop.y}))
    {: sprites :anim? true}))

(fn collect-sprite [magick-img name prop]
  {:sprites [{: name
              : magick-img
              :img-idx 0
              :width prop.w
              :height prop.h
              :local-x-off prop.x
              :local-y-off prop.y}]
   :anim? false})

(fn load-magick-image [image]
  (let [magick-img (magick.load_image image.path)
        props (get-image-props magick-img)
        prop-count (length props)
        anim? (> prop-count 1)]
    (if anim?
        (collect-anim-sprites magick-img image.name props)
        (collect-sprite magick-img image.name (. props 1)))))

(fn get-anim-indices [sprites anim-name]
  (let [indices []
        anim-sprites []]
    (each [idx sprite (ipairs sprites)]
      (when (and sprite.anim (= sprite.anim anim-name))
        (table.insert anim-sprites {: idx :anim-pos sprite.img-idx})))
    (table.sort anim-sprites
                (fn [a b]
                  (< a.anim-pos b.anim-pos)))
    (each [_i sprite (ipairs anim-sprites)]
      (table.insert indices sprite.idx))
    indices))

(fn parse-images [images]
  (let [sprites []
        anims []
        anim-set {}]
    (each [_i image (ipairs images)]
      (let [{:sprites img-sprites : anim?} (load-magick-image image)]
        (util.tbl-merge-to! sprites img-sprites)
        (when anim?
          (set (. anim-set image.name)
               {:fps image.fps :sprite-count (length img-sprites)}))))
    (table.sort sprites (fn [a b]
                          (let [area-a (* a.width a.height)
                                area-b (* b.width b.height)]
                            (> area-a area-b))))
    (each [anim-name anim (pairs anim-set)]
      (table.insert anims
                    {:name anim-name
                     :fps anim.fps
                     :sprite-count anim.sprite-count
                     :sprite-indices (get-anim-indices sprites anim-name)}))
    {: sprites : anims}))

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

(fn create-spritesheet [image-list]
  (let [{:sprites pre-sprites : anims} (parse-images image-list)
        {: image : offsets : uvs} (generate-atlas pre-sprites)
        sprites (util.tbl-kmap pre-sprites
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
    (util.chima-log "ANIMS: %s" (inspect anims))
    (util.chima-log "SPRITES: %s" (inspect sprites))
    (util.chima-assert false "STOP")
    (create-spritesheet-blob sprites anims image)))

{:create-spritesheet (λ [image-list]
                       (case (pcall create-spritesheet image-list)
                         (true sheet-blob) sheet-blob
                         (false err) (values nil err)))}
