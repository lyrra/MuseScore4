;;;; test utilities for testing musescore

(define-syntax-rule (loop-list mem list exp ...)
  (for-each (lambda (mem)
              exp ...)
            list))

(define-syntax-rule (loop-vec i mem vec exp ...)
  (vector-for-each (lambda (i mem)
                     exp ...)
            vec))
