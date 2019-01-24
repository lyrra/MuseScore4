;;;; load and define commonly used code

(use-modules (ice-9 format))

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
