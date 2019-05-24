;;;; FFI libmusescore functions interface
(eval-when (expand load eval)

(define c++prefix "_ZN")

;; See https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling
;; for how to perform g++ conversion (Itanium IA64 ABI).
(define (c++mangle namelist)
  (apply string-append
   (append (list c++prefix)
    (map (lambda (name)
           (match name
             ('c1 "C1") ; complete object constructor (no memory allocation)
             ('c2 "C2") ; base object constructor (no memory allocation, no virtual classes)
             ('c "K") ; const
             ('E "E") ; const
             ('v "v") ; void argument (takes no arguments)
             ('i "i") ; integer argument
             (m
              (cond
                ((string? m) m)
                (else
                  (let ((m (format #f "~a" m)))
                    (format #f "~d~a" (string-length m) m)))))))
         namelist))))

(define (get-score-ptr score)
  (let ((c-score (struct-ref/unboxed score 0)))
    (make-pointer c-score)))

; the caller has done syntax->datum on each argument
; therefore we need to syntax wrap back name, and
; also new symbols, like the lambda paramters we create
(define (make-ffi-function stx name parms ret
                           mangle-directive
                           keyargs)
  (let* ((c++mangled (c++mangle mangle-directive))
         (sname (datum->syntax stx name))
         (numparms (length parms))
         (i 0)
         ; make unique lambda parameter names
         (parmnames (map-in-order
                     (lambda (cn)
                       (set! i (+ i 1))
                       (datum->syntax stx
                        (string->symbol
                         (format #f "a~a~a"
                                 cn i)))) parms))
         ; translate c-argument short-hand type to FFI-types
         (cargs (map-in-order
                  (lambda (type)
                    (match type
                      ('p #''*)
                      ('i #'int)
                      ('s #'s)
                      (x (error "unknown parameter type ~s" x))))
                  parms))
         ; translate return short-hand type to FFI-type
         (rettype (match ret
                    ('p #''*)
                    ('n #''*)
                    ('s #''*) ; FIX: add string code
                    ('d #'double)
                    ('i #'int)
                    ('b #'int) ; boolean (c++ integer)
                    ('v #'void)
                    (x (error "unknown return type ~s" x)))))
    ;(format #t "  mangled c++ name: ~s~%" c++mangled)
    ;(format #t "Create an FFI function, that is callable from scheme and which will call into C. It has following properties:~%")
    ;(format #t "  numparms : ~s -- it takes this many arguments~%" numparms)
    ;(format #t "  parmnames: ~s -- names of the arguments~%" (tree-walk syntax->datum parmnames))
    ;(format #t "  cargs  : ~s => ~s -- the types of each parameter given to the C function, (left is your code, and right is what FFI type that translates into)~%" parms (tree-walk syntax->datum cargs))
    ;(format #t "  rettype: ~s => ~s -- the type of datum that C will return~%" ret rettype)
    ; #,@ unsyntax-splicing
    (let ((form
      #`(lambda (#,@parmnames)
          ;(format #t "Welcome to ~s, parms: ~s~%" ',name ',parmnames)
          (let ((cfun (pointer->procedure #,rettype
                       (dynamic-func #,c++mangled (dynamic-link))
                       (list #,@cargs))))
            #,(cond
               ; translate nullptr to boolean (#t #f)
               ((eq? ret 'n)
                #`(let ((retptr (cfun #,@parmnames)))
                   (if (= (pointer-address retptr) 0)
                     #f retptr)))
               ; translate c-string-pointer to scheme-string
               ((eq? ret 's)
                #`(pointer->string (cfun #,@parmnames)))
               ; translate integer to boolean
               ((eq? ret 'b)
                #`(if (= 0 (cfun #,@parmnames)) #f #t))
               ; else give return value as is (only SCM converted)
               (else
                 #`(cfun #,@parmnames)))))))
      ;(format #t "  FFI scheme code wrapper: ~s -> ~s~%" name (tree-walk syntax->datum form))
      form)))

(define-syntax def-ffi-c
  (lambda (stx)
    (syntax-case stx ()
      ((def-ffi-c name parms ret mangle-directive . keyargs)
       #`(define name
                 #,(make-ffi-function stx
                    (syntax->datum #'name)
                    (syntax->datum #'parms)
                    (syntax->datum #'ret)
                    (syntax->datum #'mangle-directive)
                    (syntax->datum #'keyargs)))))))

) ; eval-when

;;; class fraction

; Fraction(int, int) complete object constructor
(def-ffi-c ms-fraction-con (p i i) p
           (Ms Fraction c1 E i i))

;;; class system

(def-ffi-c ms-system-staves (p) p
        (  Ms System staves E v))

;;; class measure

(def-ffi-c ms-measure-noOffset (p) i
        (c Ms MeasureBase noOffset E v))
(def-ffi-c ms-measure-no       (p) i
        (c Ms MeasureBase no E v))

(def-ffi-c ms-measure-findSegmentR (p i p) p
        (c Ms Measure findSegmentR "ENS_" SegmentType "ERKNS_" Fraction E))
(def-ffi-c ms-measure-mmrest? (p) b
        (c Ms Measure isMMRest E v))

;;; class Selection

(def-ffi-c ms-selection-isRange  (p) n
        (c Ms Selection isRange E v))
(def-ffi-c ms-selection-isNone   (p) n
        (c Ms Selection isNone E v))
(def-ffi-c ms-selection-isList   (p) n
        (c Ms Selection isList E v))
(def-ffi-c ms-selection-isSingle (p) n
        (c Ms Selection isSingle E v))

(def-ffi-c ms-selection-deselectAll     (p) v
          (Ms Selection deselectAll E v))
(def-ffi-c ms-selection-setStartSegment (p p) v
          (Ms Selection setStartSegment E "PNS_" Segment E))
(def-ffi-c ms-selection-setEndSegment   (p p) v
          (Ms Selection setEndSegment E "PNS_" Segment E))

(def-ffi-c ms-selection-tickStart (p) i
        (c Ms Selection tickStart E v))
(def-ffi-c ms-selection-tickend   (p) i
        (c Ms Selection tickEnd E v))
(def-ffi-c ms-selection-add       (p p) v
        (  Ms Selection add E "PNS_" Element E))
(def-ffi-c ms-selection-update    (p) v
        (  Ms Selection update E v))
(def-ffi-c ms-selection-setState  (p i) v
        (  Ms Selection setState E "NS_" SelState E))

; suspect function (hairy logic)
(def-ffi-c ms-selection-updateSelectedElements (p) v
          (Ms Selection updateSelectedElements E v))
; sel tick1 tick2 staffstart staffend = > void
(def-ffi-c ms-selection-setRangeTicks (p p p i i) v
          (Ms Selection setRangeTicks "ERKNS_" Fraction "ES3_" "ii"))

; These are not exported as symbols!
; (def ms-selection-setStaffStart int "???") ;  setStaffStart(int v)
; (def ms-selection-setStaffEnd int  "???") ; setStaffEnd(int v)


;;; class Segment

(def-ffi-c ms-segment-next1   (p) n
        (c Ms Segment next1 E v))
(def-ffi-c ms-segment-next    (p) n
        (c Ms Segment next E v))
(def-ffi-c ms-segment-prev    (p) n
        (c Ms Segment prev E v))
(def-ffi-c ms-segment-prev1   (p) n
        (c Ms Segment prev1 E v))
(def-ffi-c ms-segment-next1MM (p) n
        (c Ms Segment next1MM E v))
(def-ffi-c ms-segment-prev1MM (p) n
        (c Ms Segment prev1MM E v))
(def-ffi-c ms-segment-measure (p) n
        (c Ms Segment measure E v))

; segmentType() (NOTE: not the same as ms-element-type)
(def-ffi-c ms-segment-type    (p) i
        (c Ms Segment segmentType E v))
(def-ffi-c ms-segment-element (p i) n
        (c Ms Segment element E i))

;;; class Element

(def-ffi-c ms-element-parent  (p) p
        (c Ms Element parent E v))
(def-ffi-c ms-element-tick    (p) i
        (c Ms Element tick E v))
(def-ffi-c ms-barline-type    (p) i
        (c Ms BarLine barLineType E v))
(def-ffi-c ms-element-name    (p) s
        (c Ms ScoreElement name E v))
(def-ffi-c ms-element-track   (p) i
        (c Ms Element track E v))
(def-ffi-c ms-element-x       (p) d
        (c Ms Element x     E v))
(def-ffi-c ms-element-y       (p) d
        (c Ms Element y     E v))
(def-ffi-c ms-element-rx      (p) d
        (  Ms Element rxpos E v))
(def-ffi-c ms-element-ry      (p) d
        (  Ms Element rypos E v))

;;; class chord
(def-ffi-c ms-chord-notes (p) p
        (c Ms Chord notes E v))

(def-ffi-c ms-chordrest-crossmeasure (p) i
        (c Ms ChordRest crossMeasure E v))

;;; class Score

; The first argument to these ffi-functions are a score-object
; and need to be re-wrapped before calling C.
(let-syntax
  ((def
    (lambda (stx)
      (syntax-case stx ()
        ((def name (score args ...) (c-ret c-args ...)
              mangle-directive)
         (let ((mangled (c++mangle (syntax->datum #'mangle-directive))))
           #`(define (name score args ...)
               (let ((cfun (pointer->procedure c-ret
                            (dynamic-func #,mangled
                                          (dynamic-link))
                            c-args ...))
                     (ptr (get-score-ptr score)))
                 (cfun ptr args ...)))))))))
  (def ms-score-nstaves (score) (int '(*))
    (c Ms Score nstaves E v))
  (def ms-score-ntracks (score) (int '(*))
    (c Ms Score ntracks E v))
  (def ms-score-pos     (score) (int '(*))
    (  Ms Score pos E v))

  (def ms-score-tick2measure   (score tick) ('* (list '* '*))
    (c Ms Score tick2measure "ERKNS_" Fraction E))
  (def ms-score-tick2segment   (score tick) ('* (list '* '*))
    (c Ms Score tick2segment "ERKNS_" Fraction E))
  (def ms-score-tick2measureMM (score tick) ('* (list '* '*))
    (c Ms Score tick2measureMM "ERKNS_" Fraction E))
  (def ms-score-tick2segmentMM (score tick) ('* (list '* '*))
    (c Ms Score tick2segmentMM "ERKNS_" Fraction E))

  (def ms-score-doLayout (score) (void '(*))
    (  Ms Score doLayout E v))
  (def ms-score-update   (score) (void '(*))
    (  Ms Score update E v))

  (def ms-score-first          (score) ('* '(*))
    (c Ms Score first E v))
  (def ms-score-firstMM        (score) ('* '(*))
    (c Ms Score firstMM E v))
  (def ms-score-firstMeasure   (score) ('* '(*))
    (c Ms Score firstMeasure E v))
  (def ms-score-firstMeasureMM (score) ('* '(*))
    (c Ms Score firstMeasureMM E v))

  (def ms-score-crMeasure (score idx) ('* (list '* int))
    (c Ms Score crMeasureE i))
  (def ms-score-selectAdd (score elm) (void '(* *))
    (  Ms Score selectAdd E "PNS_" Element E))
  (def ms-score-select (score elm type staff)
                       (void (list '* '* int int))
    (  Ms Score select E "PNS_" Element E "NS_" SelectType E i))

  (def ms-score-selection (score) ('* '(*))
      (Ms Score selection E v))
  (def ms-score-systems   (score) ('* '(*))
      (Ms Score systems E v))
  (def ms-score-measures  (score) ('* '(*))
      (Ms Score measures E v)))
