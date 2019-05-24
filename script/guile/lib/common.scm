;;;; load and define commonly used code

(define-module (lib common)
               #:export (nth nthcdr
                         last push))

(define (nth idx lst)
  (if (= idx 0)
    (car lst)
    (if (null? (cdr lst))
      '()
      (nth (- idx 1) (cdr lst)))))

(define (nthcdr idx lst)
  (if (= idx 0)
    lst
    (if (null? (cdr lst))
      '()
      (nthcdr (- idx 1) (cdr lst)))))

(define (last lst)
  (do ((l lst (cdr l)))
      ((eq? '() (cdr l)) l)))

(define-syntax-rule (push item place)
  (set! place (cons item place)))
