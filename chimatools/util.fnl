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

(fn parse-file-format [path]
  "nil")

(fn file-exists [path]
  true)

(fn chima-err [err ...]
  (error (string.format err ...)))

(fn chima-assert [cond ?err ...]
  (if (not= ?err nil)
      (assert cond (string.format ?err ...))
      (assert cond)))

(fn chima-log [msg ...]
  (print (string.format (.. "[CHIMA-LOG] " msg) ...)))

{: chima-err
 : chima-assert
 : chima-log
 : parse-file-format
 : file-exists
 : tbl-reduce
 : tbl-kreduce
 : tbl-map
 : tbl-kmap}
