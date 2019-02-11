(define-module (mtest)
               #:use-module (ice-9 format)
               #:use-module (ice-9 match)
               #:use-module (ice-9 ftw)
               #:use-module (srfi srfi-43) ; vector library
               #:use-module (system foreign)
               #:use-module (musescore-c))

(eval-when (expand load eval)
  (load-from-path "test/test.scm")
  (load-from-path "test/ffi.scm"))

(let ((files (sort (map car
                        (cddr (file-system-tree "../../script/guile/test/t")))
                   string<)))
  (for-each (lambda (file)
              (load-from-path (format #f "test/t/~a" file)))
            files))

(assert #t "1 is not ")
