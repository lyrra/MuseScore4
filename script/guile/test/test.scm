;;;; test tools
;;;; probably replace this with some test framework

(define-module (test test)
               #:use-module (ice-9 format) ; easier and nicer formatting
               #:use-module (ice-9 match)  ; match (a more expressive cond)
               #:export (assert
                         deftest
                         *tests-num-pass*
                         *tests-num-total*))

; define a simple assert macro that lets out quit when
; an error has occured. Check that the unix-return-code is 0.
(define-syntax-rule (assert expr msg ...)
  (if (not expr)
    (begin
      (format #t "ERROR: ")
      (format #t msg ...)
      (format #t "~%")
      (error msg ...))))

(define *tests* '())
(define *tests-num-total* 0)
(define *tests-num-pass*  0)
(define *tests-cur-label* "")

(define (test-begin name)
  (set! *tests-num-total* (+ *tests-num-total* 1))
  (set! *tests-cur-label* name))

(define (test-pass)
  (set! *tests-num-pass* (+ *tests-num-pass* 1))
  123)

; Catch any errors, needed if this script is loaded from STDIN, where guile will put us in debugger and continue taking forms
(define (run-test name func)
  (catch #t
    (lambda () (func))
    (lambda (key . args)
      (catch #t
        (lambda ()
          (match args
            (((or #f #t) string (or #f #t) (or #f #t))
             (format #t "Test Error: test=~a, error=~s~%  " (or name *tests-cur-label*) key)
             'test-fail)
            (((or #f #t) string (arg ...) (or #f #t))
             (format #t "Test Error: test=~a, error=~s~%  " (or name *tests-cur-label*) key)
             (apply format (cons* #t arg))
             (newline)
             'test-fail)
            (_ (format #t "Test Error: test=~a, error=~s, ~s~%  " (or name *tests-cur-label*) key args)
               'test-fail)))
        (lambda (key . args)
          ; A fail occured when trying to report the test-error
          ; just silently fail to avoid ending up in debugger
          'test-error-report-fail)))))

(define (run-top-test name func)
  (let ((res (run-test name func)))
    (cond
      ((not (equal? res 123))
       (format #t "  TEST-FAIL, result: ~s~%" res)))))

(define-syntax-rule (deftest (name) exp ...)
  (run-top-test 'name
                (lambda ()
                  (test-begin 'name)
                  exp ...
                  (test-pass))))
