;;; plib -- scheme library if syntax-case/rules is available

(define-syntax loop-list
  (syntax-rules ()
    ((_ (item list) . body)
     (do ((pair list (cdr pair)))
         ((eq? '() pair))
       (let ((item (car pair)))
         . body)))))
