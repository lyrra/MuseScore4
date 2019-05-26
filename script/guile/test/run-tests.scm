
(define-module (test run-tests)
               #:use-module (musescore-c)
               #:use-module (lib musescore)
               #:use-module (lib common)
               #:use-module (srfi srfi-43) ; vector library
               #:use-module (test test) ; test framework
               #:use-module (test common) ; test helper function/macros
               )

; test files

(load-from-path "test/readme.scm")

; Load test cases
(for-each (lambda (file)
           (load-from-path file))
         '("test/score.scm"
           "test/measure.scm"
           "test/parts.scm"
           "test/segment.scm"
           "test/element.scm"
           "test/notes.scm"
           "test/inputstate.scm"
           "test/slush.scm"
           "test/read-score-sxml.scm"))

(let ((files (get-test-files "script/guile/test/t")))
  (for-each (lambda (file)
              (if (eqv? (string-contains-ci file "t_") 0)
                (load-from-path (format #f "test/t/~a" file))))
            files))

(format #t "Testing has finished.~%")
(format #t "Test passed: ~s/~s~%"
        *tests-num-pass*
        *tests-num-total*)
(if (not (= *tests-num-pass*
            *tests-num-total*))
  (quit 1))
