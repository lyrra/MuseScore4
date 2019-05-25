
(define-module (test run-tests)
               #:use-module (musescore-c)
               #:use-module (lib musescore)
               #:use-module (lib common)
               #:use-module (srfi srfi-43) ; vector library
               )

(load-from-path "test/test.scm") ; Load testing framework
(load-from-path "test/test-musescore.scm") ; Load tools used for testing musescore

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

(use-modules (ice-9 ftw))
(let ((files (sort (map car
                        (cddr (file-system-tree "script/guile/test/t")))
                   string<)))
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
