(define-module (mtest)
               #:use-module (ice-9 format)
               #:use-module (ice-9 match)
               #:use-module (ice-9 ftw)
               #:use-module (srfi srfi-43) ; vector library
               #:use-module (test test)    ; test framework
               #:use-module (test common)  ; test helper function/macros
               #:use-module (test test-musescore)  ; musescore test helper function/macros
               #:use-module (test ffi)  ; musescore test helper function/macros
               #:use-module (musescore-c) ; c-exports used by scripts
               )

(let ((test-files (get-test-files "../../script/guile/test/t"))
      (n 0))
  (for-each (lambda (file)
              (if (not (eqv? (string-contains-ci file "t_") 0))
                (load-from-path (format #f "test/t/~a" file)))
              (set! n (+ n 1)))
            test-files)
  (format #t "Passed ~a tests.~%" n))
