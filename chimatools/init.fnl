(local {: create-spritesheet-blob} (require :chimatools.sprite))

(fn write-to-file! [blob file-path]
  (let [file (io.open file-path "wb")
        write-blob (fn []
                     (file:write blob)
                     (file:close)
                     file-path)]
    (if (not= file nil)
        (write-blob)
        (values nil "Failed to open file"))))

{:write_spritesheet (λ [image-list file-path]
                      (case (create-spritesheet-blob image-list)
                        sprite-blob (write-to-file! sprite-blob file-path)
                        (nil err) (values nil err)))
 :create_spritesheet (λ [image-list]
                       (create-spritesheet-blob image-list))}
