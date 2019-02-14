;;;; FFI libmusescore functions interface

(define (ms-score-doLayout score)
  (let* ((cfun (pointer->procedure void
            (dynamic-func "_ZN2Ms5Score8doLayoutEv" (dynamic-link))
            '(*)))
         (c-score (struct-ref/unboxed score 0))
         (ptr (make-pointer c-score)))
    (cfun ptr)))
