;;;; test tools
;;;; probably replace this with some test framework

; Load an scheme module that makes output print formatting nicer
(use-modules (ice-9 format))

; define a simple assert macro that lets out quit when
; an error has occured. Check that the unix-return-code is 0.
(define-syntax-rule (assert expr msg ...)
  (if (not expr)
    (begin
      (format #t msg ...)
      (format #t "~%")
      (error msg ...))))

; Catch any errors and quit, needed if this script is loaded from STDIN, where guile will put us in debugger and continue taking forms

(define (load-test file)
  (catch #t
    (lambda () (primitive-load file))
    (lambda (key . args)
      (format #t "Load Test Error: ~s ~s~%" key args)
      (quit 111))))

(define (run-test name)
  (catch #t
    (lambda () (name))
    (lambda (key . args)
      (apply
          (lambda (a fmt arg b)
            (format #t "Run Test Error: test=~a, error=~s~%" name key)
            (apply format (cons* #t fmt arg))
            (newline)
            (quit 111))
          args))))
