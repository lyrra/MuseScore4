(define-module (test common)
               #:use-module (srfi srfi-1)  ; fold
               #:use-module (ice-9 ftw)    ; file-system-tree
               #:export (test-filter-args
                         fold-find-inc/exc
                         fold-string-contains
                         apply-filter-file
                         get-test-files

                         collect
                         tree-walk
                         between

                         loop-list
                         loop-vec
                         ))

;;;; test helpers to get a list of test files to run

(define (test-filter-args args)
  (fold (lambda (item acc)
          (cond
            ((equal? (string-contains item "--inc=") 0)
             (cons (cons #:inc (substring item 6)) acc))
            ((equal? (string-contains item "--exc=") 0)
             (cons (cons #:exc (substring item 6)) acc))
            (else acc)))
        '()
        (if args args '())))

(define (fold-find-inc/exc name filter)
  (fold (lambda (item acc)
          (if (and (pair? item)
                   (eq? (car item) name))
              (cons (cdr item) acc)
              acc))
        '() filter))

(define (fold-string-contains str lst beginacc found)
  (fold (lambda (filt acc)
          (if (string-contains str filt)
              found acc))
        beginacc lst))

(define (apply-filter-file file test-filter)
  (let ((inc (fold-find-inc/exc #:inc test-filter))
        (exc (fold-find-inc/exc #:exc test-filter)))
    (cond
      ((not (null? inc))
       (fold-string-contains file inc #f #t))
      ((not (null? exc))
       (fold-string-contains file exc #t #f))
     (else #t))))

(define (get-test-files path)
  (let ((test-filter (test-filter-args (command-line)))
        (test-files (sort (map car
                               (cddr (file-system-tree path)))
                          string<)))
    (fold (lambda (file acc)
            (if (apply-filter-file file test-filter)
                (cons file acc)
                acc))
          '()
          test-files)))

;;;; more miscellaneous functions/macros

(define-syntax-rule (collect push exp ...)
  (let* ((lst '())
         (push (lambda (x) (set! lst (cons x lst)))))
    exp ...
    (reverse lst)))

(define (tree-walk func form)
  (cond
    ((pair? form)
     (cons (func (car form))
           (tree-walk func (cdr form))))
    (else (func form))))

(define (between proc lst)
  (cdr (reduce (lambda (c p)
                 (if (list? p)
                   (cons c (cons (proc c (car p)) (cdr p)))
                   (cons c (list (proc c p)))))
               '()
               lst)))

(define-syntax-rule (loop-list mem list exp ...)
  (for-each (lambda (mem)
              exp ...)
            list))

(define-syntax-rule (loop-vec i mem vec exp ...)
  (vector-for-each (lambda (i mem)
                     exp ...)
            vec))
