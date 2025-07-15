(local ffi (require :ffi))
(local inspect (require :inspect))

(local chima-magic "CHIMA.ASSET.FILE")
(local chima-version [0 0 1])
(local file-enum {:sprite 1 :model 2})

(fn tbl-reduce [tbl func ?initial]
  (var acc ?initial)
  (each [_k val (pairs tbl)]
    (let [ret (func acc val)]
      (set acc ret)))
  acc)

(fn tbl-kreduce [tbl func ?initial]
  (var acc ?initial)
  (each [k val (pairs tbl)]
    (let [ret (func acc k val)]
      (set acc ret)))
  acc)

(fn tbl-map [tbl func]
  (local out {})
  (each [k val (pairs tbl)]
    (set (. out k) (func val)))
  out)

(fn tbl-kmap [tbl func]
  (local out {})
  (each [k val (pairs tbl)]
    (set (. out k) (func k val)))
  out)

(fn chima-die [err code]
  (print (string.format "%s" err))
  (os.exit (or code 1)))

(fn make-chima-header [file-type]
  "Create a chima file header blob"
  (let [bytes (ffi.new "uint8_t[20]")
        enum (assert (. file-enum file-type))]
    (ffi.copy bytes chima-magic 16)
    (set (. bytes 16) (. chima-version 1))
    (set (. bytes 17) (. chima-version 2))
    (set (. bytes 18) (. chima-version 3))
    (set (. bytes 19) enum)
    (ffi.string bytes 20)))

(fn calc-string-offsets [string-list sizes bank-offset]
  (let [sum-off-until (fn [idx]
                        (var off bank-offset)
                        (for [i 1 (- idx 1)]
                          (set off (+ off (. sizes i) bank-offset)))
                        off)]
    (tbl-kmap string-list (fn [i _val]
                            (sum-off-until i)))))

(fn make-string-bank [string-list bank-offset]
  "Create a string blob with offsets from a list of strings and an initial byte offset"
  "The blob is in the format (len, data...)"
  (let [sizes (tbl-map string-list
                       (fn [val]
                         (length val)))
        offsets (calc-string-offsets string-list sizes bank-offset)
        bank-data (tbl-reduce string-list
                              (fn [str val]
                                (.. str val)) "")
        bank-len (length bank-data)
        bytes-len (ffi.new "uint32_t[1]")
        bytes-data (ffi.new "uint8_t[?]" bank-len)]
    (set (. bytes-len 0) bank-len)
    (ffi.copy bytes-data bank-data bank-len)
    {: sizes
     : offsets
     :blob (.. (ffi.string bytes-len 4) (ffi.string bytes-data bank-len))
     :blob-size (+ 4 bank-len)}))

(print (inspect (make-string-bank ["test" "amoma"] 0)))

{: chima-die
 : make-chima-header
 : tbl-reduce
 : tbl-kreduce
 : tbl-map
 : tbl-kmap}
