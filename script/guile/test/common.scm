
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
