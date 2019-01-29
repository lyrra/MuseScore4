;;;; build and test tools

; Hold a list of function name which are
; converted into scm_c_define_gsubr() c-code and put into
; c/scheme init function init_guile_musescore_functions()
; Each use of the macro scm/c-fun will populate this list.
(define *c-scheme-functions* '())

(define-syntax-rule (f exp ...)
  (format #t exp ...))

; crude indent
(define (indent n)
  (do ((i 0 (1+ i))) ((>= i n)) (f " ")))

(define (convert-c-name sname)
  (string-fold
   (lambda (a b)
     (format #f "~a~a" b
             (cond
               ((eq? a #\-) #\_)
               ((eq? a #\!) "_set")
               (else a))))
   "" sname))


(define (scheme-subr name cfun arn)
  (f "scm_c_define_gsubr (\"~a\", ~a, 0, 0, (void *)~a);~%"
     name arn cfun))

(define-syntax scm/c-fun
  (lambda (x)
    (syntax-case x ()
      ((scm/c-fun2 name args doc exp ...)
        (let ((c-name (convert-c-name (syntax->datum #'name))))
          #`(begin
    (push (list name #,c-name (length 'args))
          *c-scheme-functions*)
    (map (lambda (line) (f "// ~a~%" line)) doc)
    (f "static SCM~%~a (" #,c-name)
    (do ((arg 'args (cdr arg))
         (i 0 (+ i 1)))
        ((eq? '() arg))
        (if (> i 0) (f ", "))
        (f "~a" (car arg)))
    (f ")~%{~%")
    ; Emit c-function body
    exp ...
    (f "}~%~%")
    ))))))

; Generates code that transfers data in the c-variable 'input'
; to 'output'.
; The list 'flow' describes how to do the transfers.
(define (expand-transfer input flow output)
  (let ((lst '())
        (invar input))
    (do ((l flow (cdr l))
         (a 97 (+ a 1)))
        ((null? l))
      (let* ((line (car l))
             (typ  (nth 0 line)) ; variable type
             (ini  (nth 1 line)) ; assignment/initialization
             (meth #f) ; ass/init by obj-method
             (n 2))
        (if (eq? ini 'm) ; ini-prefix
          (begin (set! meth #t)
                 (set! ini (nth 2 line))
                 (set! n 3)))
        (let ((flags (nthcdr n line))
              (var (if (null? (cdr l)) ; last line
                     output
                     (format #f "~c" (integer->char a))))
              (castline ""))
          (if (eq? ini 'scm-ref)
            (set! ini (format #f "scm_foreign_object_ref(~a, 0)" invar)))
          (if (null? ini) (set! ini invar))
          (if meth
            (set! ini (format #f "~a->~a" invar ini)))
          (if (not (memq 'c flags)) ; skip explicit casting
            (set! castline (string-append "(" typ ") ")))
          (set! lst (cons (format #f "~a ~a = ~a~a;~%"
                                  typ var castline ini)
                          lst))
          (if (memq 'r flags) ; if NULL return scheme/false
            (set! lst (cons (format #f "if (! ~a) { return SCM_BOOL_F; }~%" var)
                            lst)))
          ; connect the current output variable becoming next input
          (set! invar var))))
    (reverse lst)))

(define-syntax-rule (var-transfer-expand i in ut ck)
  (for-each (lambda (line) (indent i) (f line))
            (expand-transfer in ck ut)))

(define-syntax-rule (c-make-scheme-list i dtype code)
  (let ((datastr (cond
                   ((eq? dtype 'int) "SCM data = scm_from_int(item);~%")
                   (else (format #f "SCM data = scm_make_foreign_object_1 ((SCM)~a, (SCM) item);~%" dtype)))))
  (begin
     (indent i) (f "SCM head = SCM_EOL; // head of (single-linked) list~%")
     (indent i) (f "SCM last = SCM_EOL; // last cons in list~%")
     (indent i) (f code)
     (indent (* i 2)) (f datastr)
     (indent (* i 2)) (f "last = s_push(last, data);~%")
     (indent (* i 2)) (f "if (head == SCM_EOL) {~%")
     (indent (* i 2)) (f "      head = last;~%")
     (indent (* i 2)) (f "      }~%")
     (indent (* i 2)) (f "}~%") ; // close the for-loop
     (indent i) (f "return head; // return first element cons in list~%"))))

(define-syntax-rule (c-make-scheme-list2 i dtype code postfor)
  (let ((datastr (cond
                   ((eq? dtype 'int) "SCM data = scm_from_int(item);~%")
                   (else (format #f "SCM data = scm_make_foreign_object_1 ((SCM)~a, (SCM) item);~%" dtype)))))
  (begin
     (indent i) (f "SCM head = SCM_EOL; // head of (single-linked) list~%")
     (indent i) (f "SCM last = SCM_EOL; // last cons in list~%")
     (indent i) (f code)
     (indent (* i 2)) (f datastr)
     (indent (* i 2)) (f "last = s_push(last, data);~%")
     (indent (* i 2)) (f "if (head == SCM_EOL) {~%")
     (indent (* i 2)) (f "      head = last;~%")
     (indent (* i 2)) (f "      }~%")
     ; bad hack
     (f postfor)
     (indent (* i 2)) (f "}~%") ; // close the for-loop
     (indent i) (f "return head; // return first element cons in list~%"))))
