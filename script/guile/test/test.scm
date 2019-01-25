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

(define *tests* '())

(define (add-test name fun)
  (set! *tests*
        (cons (list name
                    fun)
              *tests*)))

(define-syntax-rule (deftest (name) exp ...)
  (add-test 'name
            (lambda ()
              exp ...
              123)))

; Catch any errors, needed if this script is loaded from STDIN, where guile will put us in debugger and continue taking forms
(define (run-test name func)
  (catch #t
    (lambda () (func))
    (lambda (key . args)
      (catch #t
        (lambda ()
          (cond
            ((eq? key 'misc-error)
             (apply
               (lambda (a fmt arg b)
                 (format #t "Run Test Error: test=~a, error=~s~%  " name key)
                 (apply format (cons* #t arg))
                 (newline)
                 'test-fail)
               args))
            (else
             (format #t "Test error:~%")
             (apply format (cons* #t (cadr args) (caddr args)))
             'unknown-error)))
        (lambda (key . args)
          ; A fail occured when trying to report the test-error
          ; just silently fail to avoid ending up in debugger
          'test-error-report-fail)))))

(define (run-tests)
  (let ((good 0))
    (map (lambda (pair)
           (format #t "Running test: ~s~%" (car pair))
           (let ((res (run-test (car pair) (cadr pair))))
             (if (not (equal? res 123))
               (format #t "  result: ~s~%" res))
             (if (equal? res 123)
               (set! good (+ good 1)))))
         *tests*)
    good))
