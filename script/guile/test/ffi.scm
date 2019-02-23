;;;; FFI libmusescore functions interface
(eval-when (expand load eval)

(define c++prefix "_ZN")

;;; C++ mangle table
(define mangle-table
  '((ms-score-nstaves (c Ms Score nstaves E v))
    (ms-score-ntracks (c Ms Score ntracks E v))
    (ms-score-pos     (  Ms Score pos E v))
    (ms-score-tick2measure   (c Ms Score tick2measure E i))
    (ms-score-tick2segment   (c Ms Score tick2segment E i))
    (ms-score-tick2measureMM (c Ms Score tick2measureMM E i))
    (ms-score-tick2segmentMM (c Ms Score tick2segmentMM E i))
    (ms-score-doLayout       (  Ms Score doLayout E v))
    (ms-score-update         (  Ms Score update E v))
    (ms-score-first          (c Ms Score first E v))
    (ms-score-firstMM        (c Ms Score firstMM E v))
    (ms-score-firstMeasure   (c Ms Score firstMeasure E v))
    (ms-score-firstMeasureMM (c Ms Score firstMeasureMM E v))
    (ms-score-crMeasure      (c Ms Score crMeasureE i))
    (ms-score-selectAdd      (  Ms Score selectAdd E "PNS_" Element E))
    (ms-score-select         (  Ms Score select E "PNS_" Element E "NS_" SelectType E i))
    (ms-score-selection      (  Ms Score selection E v))
    (ms-score-systems        (  Ms Score systems E v))
    (ms-score-measures       (  Ms Score measures E v))

    (ms-system-staves        (  Ms System staves E v))

    (ms-measure-noOffset     (c Ms MeasureBase noOffset E v))
    (ms-measure-no           (c Ms MeasureBase no E v))
    (ms-measure-mmrest?      (c Ms Measure isMMRest E v))
    (ms-measure-findSegmentR
        (c Ms Measure findSegmentR E "NS_" SegmentType E i))
    (ms-selection-isRange     (c Ms Selection isRange E v))
    (ms-selection-isNone      (c Ms Selection isNone E v))
    (ms-selection-isList      (c Ms Selection isList E v))
    (ms-selection-isSingle    (c Ms Selection isSingle E v))
    (ms-selection-deselectAll (  Ms Selection deselectAll E v))
    (ms-selection-setStartSegment (Ms Selection setStartSegment E "PNS_" Segment E))
    (ms-selection-setEndSegment (Ms Selection setEndSegment E "PNS_" Segment E))
    (ms-selection-tickStart (c Ms Selection tickStart E v))
    (ms-selection-tickend   (c Ms Selection tickEnd E v))
    (ms-selection-add       (  Ms Selection add E "PNS_" Element E))
    (ms-selection-update    (  Ms Selection update E v))
    (ms-selection-setState  (  Ms Selection setState E "NS_" SelState E))
    (ms-selection-updateSelectedElements
               (Ms Selection updateSelectedElements E v))
    (ms-selection-setRangeTicks
               (Ms Selection setRangeTicks E "iiii"))

    (ms-segment-next1   (c Ms Segment next1 E v))
    (ms-segment-next    (c Ms Segment next E v))
    (ms-segment-prev    (c Ms Segment prev E v))
    (ms-segment-prev1   (c Ms Segment prev1 E v))
    (ms-segment-next1MM (c Ms Segment next1MM E v))
    (ms-segment-prev1MM (c Ms Segment prev1MM E v))
    (ms-segment-measure (c Ms Segment measure E v))
    (ms-segment-type    (c Ms Segment segmentType E v))
    (ms-segment-element (c Ms Segment element E i))

    (ms-element-parent  (c Ms Element parent E v))
    (ms-element-tick    (c Ms Element tick E v))
    (ms-element-name    (c Ms ScoreElement name E v))
    (ms-element-track   (c Ms Element track E v))
    (ms-element-x       (c Ms Element x     E v))
    (ms-element-y       (c Ms Element y     E v))
    (ms-element-rx      (  Ms Element rxpos E v))
    (ms-element-ry      (  Ms Element rypos E v))
    (ms-barline-type    (c Ms BarLine barLineType E v))

    (ms-chord-notes (c Ms Chord notes E v))
    (ms-chordrest-crossmeasure (c Ms ChordRest crossMeasure E v))
    ))


;;;

;; See https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling
;; for how to perform g++ conversion (Itanium IA64 ABI).
(define (c++mangle namelist)
  (apply string-append
   (append (list c++prefix)
    (map (lambda (name)
           (match name
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

(define (get-mangled name)
  (find (lambda (pair) (eq? (car pair) name))
        mangle-table))

(define-syntax mangle
   (lambda (x)
     (syntax-case x ()
       ((_ name)
         (let* ((mangpair (get-mangled (syntax->datum #'name)))
                (mangl (cadr mangpair))
                (mang (c++mangle mangl)))
           mang)))))

(define-syntax-rule (get-dynfunc symname)
  (dynamic-func (mangle symname) (dynamic-link)))

(define (get-score-ptr score)
  (let ((c-score (struct-ref/unboxed score 0)))
    (make-pointer c-score)))

; the caller has done syntax->datum on each argument
; therefore we need to syntax wrap back name, and
; also new symbols, like the lambda paramters we create
(define (make-ffi-function stx name parms ret)
  (let* ((sname (datum->syntax stx name))
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
                       (get-dynfunc #,sname)
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

(define-syntax def-ffi-c  ;-rule (def-ffi-c name (parms-type ...) ret-type)
  (lambda (stx)
    (syntax-case stx ()
      ((def-ffi-c name parms ret)
       #`(define name
                 #,(make-ffi-function stx (syntax->datum #'name) (syntax->datum #'parms) (syntax->datum #'ret)))))))

) ; eval-when

;;; class Score

; The first argument to these ffi-functions are a score-object
; and need to be re-wrapped before calling C.
(let-syntax
  ((def
    (syntax-rules ()
      ((def name (score args ...) (c-ret c-args ...))
       (define (name score args ...)
         (let ((cfun (pointer->procedure c-ret
                      (get-dynfunc name)
                      c-args ...))
               (ptr (get-score-ptr score)))
           (cfun ptr args ...)))))))
  (def ms-score-nstaves (score) (int '(*)))
  (def ms-score-ntracks (score) (int '(*)))
  (def ms-score-pos     (score) (int '(*)))

  (def ms-score-tick2measure   (score tick) ('* (list '* int)))
  (def ms-score-tick2segment   (score tick) ('* (list '* int)))
  (def ms-score-tick2measureMM (score tick) ('* (list '* int)))
  (def ms-score-tick2segmentMM (score tick) ('* (list '* int)))

  (def ms-score-doLayout (score) (void '(*)))
  (def ms-score-update   (score) (void '(*)))

  (def ms-score-first          (score) ('* '(*)))
  (def ms-score-firstMM        (score) ('* '(*)))
  (def ms-score-firstMeasure   (score) ('* '(*)))
  (def ms-score-firstMeasureMM (score) ('* '(*)))

  (def ms-score-crMeasure (score idx) ('* (list '* int)))
  (def ms-score-selectAdd (score elm) (void '(* *)))
  (def ms-score-select (score elm type staff)
                       (void (list '* '* int int)))

  (def ms-score-selection (score) ('* '(*)))
  (def ms-score-systems   (score) ('* '(*)))
  (def ms-score-measures  (score) ('* '(*))))

;;; class system

(def-ffi-c ms-system-staves (p) p)

;;; class measure

(def-ffi-c ms-measure-noOffset (p) i)
(def-ffi-c ms-measure-no       (p) i)

(def-ffi-c ms-measure-findSegmentR (p i i) p)
(def-ffi-c ms-measure-mmrest? (p) b)

;;; class Selection

(def-ffi-c ms-selection-isRange  (p) n)
(def-ffi-c ms-selection-isNone   (p) n)
(def-ffi-c ms-selection-isList   (p) n)
(def-ffi-c ms-selection-isSingle (p) n)

(def-ffi-c ms-selection-deselectAll     (p) v)
(def-ffi-c ms-selection-setStartSegment (p p) v)
(def-ffi-c ms-selection-setEndSegment   (p p) v)

(def-ffi-c ms-selection-tickStart (p) i)
(def-ffi-c ms-selection-tickend   (p) i)
(def-ffi-c ms-selection-add       (p p) v)
(def-ffi-c ms-selection-update    (p) v)
(def-ffi-c ms-selection-setState  (p i) v)

; suspect function (hairy logic)
(def-ffi-c ms-selection-updateSelectedElements (p) v)
; sel tick1 tick2 staffstart staffend = > void
(def-ffi-c ms-selection-setRangeTicks (p i i i i) v)

; These are not exported as symbols!
; (def ms-selection-setStaffStart int "???") ;  setStaffStart(int v)
; (def ms-selection-setStaffEnd int  "???") ; setStaffEnd(int v)

;;; class Segment

(def-ffi-c ms-segment-next1   (p) n)
(def-ffi-c ms-segment-next    (p) n)
(def-ffi-c ms-segment-prev    (p) n)
(def-ffi-c ms-segment-prev1   (p) n)
(def-ffi-c ms-segment-next1MM (p) n)
(def-ffi-c ms-segment-prev1MM (p) n)
(def-ffi-c ms-segment-measure (p) n)

; segmentType() (NOTE: not the same as ms-element-type)
(def-ffi-c ms-segment-type    (p) i)
(def-ffi-c ms-segment-element (p i) n)

;;; class Element

(def-ffi-c ms-element-parent  (p) p)
(def-ffi-c ms-element-tick    (p) i)
(def-ffi-c ms-barline-type    (p) i)
(def-ffi-c ms-element-name    (p) s)
(def-ffi-c ms-element-track   (p) i)
(def-ffi-c ms-element-x       (p) d)
(def-ffi-c ms-element-y       (p) d)
(def-ffi-c ms-element-rx      (p) d)
(def-ffi-c ms-element-ry      (p) d)

;;; class chord

(def-ffi-c ms-chord-notes (p) p)
(def-ffi-c ms-chordrest-crossmeasure (p) i)
