;;;; test tools

(define-syntax-rule (assert expr msg ...)
  (if (not expr)
    (error msg ...)))
