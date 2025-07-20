(local chima (require :chimatools))
(local ctx (chima.context.new))

(local def-pad 0)
(local def-path "chimaout.chima")

(fn die [msg ...]
  (print (string.format msg ...))
  (os.exit 1))

(fn tbl-filter [tbl func]
  (let [out []]
    (each [_i obj (ipairs tbl)]
      (when (func obj)
        (table.insert out obj)))
    out))

(fn path-ext [str]
  (str:match "^.+(%..+)$"))

(fn path-file [str]
  (let [noext (str:match "(.*)%.")
        maybename (noext:match "^.+/(.+)$")]
    (if maybename
        maybename
        noext)))

(fn parse-sprites [images anims]
  (let [chima-images []
        chima-anims []]
    (each [_i image (ipairs images)]
      (let [(image-or-err ret) (chima.image.load ctx image.name image.path)]
        (when ret
          (error image-or-err))
        (table.insert chima-images image-or-err)))
    (each [_i anim (ipairs anims)]
      (let [(anim-or-err ret) (chima.anim.load ctx anim.name anim.path)]
        (when ret
          (error anim-or-err))
        (table.insert chima-anims anim-or-err)))
    {: chima-images : chima-anims}))

(fn do-names [paths]
  (let [out []]
    (each [_i path (ipairs paths)]
      (let [name (path-file path)]
        (table.insert out {: name : path})))
    out))

(fn sprite-packer [files confs]
  (let [anim-exts {:.gif 1}
        image-exts {:.png 1 :.jpg 2 :.jpeg 2 :.bmp 3 :.tga 4}
        anims (do-names (tbl-filter files
                                    (fn [file]
                                      (let [ext (: (path-ext file) :lower)]
                                        (not= (. anim-exts ext) nil)))))
        images (do-names (tbl-filter files
                                     (fn [file]
                                       (let [ext (: (path-ext file) :lower)]
                                         (not= (. image-exts ext) nil)))))
        pad (tonumber (or confs.pad def-pad))
        chimapath (or confs.chimapath def-path)
        {: chima-images : chima-anims} (parse-sprites images anims)
        chima-sheet (chima.spritesheet.new ctx pad chima-images chima-anims)]
    (chima-sheet:write chimapath)))

(local packers {:-s sprite-packer :--sprite sprite-packer})
(local conf-keys {:-o "chimpath" :--output "chimapath" :-p "pad" :--pad "pad"})

(fn parse-args []
  (local confs {})
  (local files [])
  (var packer nil)
  (var i 1)
  (while (<= i (length arg))
    (let [curr-arg (. arg i)
          maybe-packer (. packers curr-arg)
          conf-key (. conf-keys curr-arg)
          conf? (not= conf-key nil)]
      (when maybe-packer
        (set packer maybe-packer))
      (when conf?
        (set i (+ i 1))
        (set (. confs conf-key) (. arg i)))
      (when (and (not maybe-packer) (not conf?))
        (table.insert files curr-arg)))
    (set i (+ i 1)))
  {: confs : files : packer})

(local {: confs : files : packer} (parse-args))

(when (not packer)
  (die "Packing argument not provided"))

(when (not= confs.atlas nil)
  (ctx:set_atlas_))

(case (pcall packer files confs)
  true (print "Done")
  (false err) (die "Packer error: %s" err))
