(begin

(load "ms.scm")

(for-each (lambda (elm-name)
  (let* ((e (ms-make-element elm-name))
         (g2 (ms-test-writeReadElement e)))
    (if (= (ms-element-type e) (ms-element-type g2))
        (ms-test-check-pass)
        (ms-test-check-fail))))
  %element-types)

)
