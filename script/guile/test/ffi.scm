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

    (ms-element-tick    (c Ms Element tick E v))
    (ms-element-name    (c Ms ScoreElement name E v))
    (ms-barline-type    (c Ms BarLine barLineType E v))
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

;(define (get-dynfunc mangled) (dynamic-func mangled (dynamic-link)))

) ; eval-when

;;; class Score

(let-syntax
  ((def
    (syntax-rules ()
      ((def name)
       (define (name score)
         (let* ((cfun (pointer->procedure int
                   (get-dynfunc name)
                   '(*)))
                (c-score (struct-ref/unboxed score 0))
                (ptr (make-pointer c-score)))
           (cfun ptr)))))))
  (def ms-score-nstaves)
  (def ms-score-ntracks)
  (def ms-score-pos))

(let-syntax
  ((def
    (syntax-rules ()
      ((def name)
       (define (name score tick)
         (let* ((cfun (pointer->procedure '*
                   (get-dynfunc name)
                   (list '* int)))
                (c-score (struct-ref/unboxed score 0))
                (ptr (make-pointer c-score)))
           (let ((retptr (cfun ptr tick)))
             (if (= (pointer-address retptr) 0)
               #f retptr))))))))
  (def ms-score-tick2measure)
  (def ms-score-tick2segment)
  (def ms-score-tick2measureMM)
  (def ms-score-tick2segmentMM))

(let-syntax
  ((def
    (syntax-rules ()
      ((def name)
       (define (name score)
         (let* ((cfun (pointer->procedure void
                   (get-dynfunc name)
                   '(*)))
                (c-score (struct-ref/unboxed score 0))
                (ptr (make-pointer c-score)))
           (cfun ptr)))))))
  (def ms-score-doLayout)
  (def ms-score-update))

(let-syntax
  ((def
    (syntax-rules ()
      ((def name)
       (define (name score)
         (let* ((cfun (pointer->procedure void
                   (get-dynfunc name)
                   '(*)))
                (c-score (struct-ref/unboxed score 0))
                (ptr (make-pointer c-score)))
           (cfun ptr)))))))
  (def ms-score-first)
  (def ms-score-firstMM)
  (def ms-score-firstMeasure)
  (def ms-score-firstMeasureMM))

(define (ms-score-crMeasure score idx)
  (let* ((cfun (pointer->procedure '*
          (get-dynfunc ms-score-crMeasure)
          (list '* int)))
         (c-score (struct-ref/unboxed score 0))
         (ptr (make-pointer c-score)))
    (cfun ptr idx)))

(define (ms-score-selectAdd score elm)
  (let* ((cfun (pointer->procedure void
          (get-dynfunc ms-score-selectAdd)
          '(* *)))
         (c-score (struct-ref/unboxed score 0))
         (ptr (make-pointer c-score)))
    (cfun ptr elm)))

(define (ms-score-select score elm type staff)
  (let* ((cfun (pointer->procedure void
          (get-dynfunc ms-score-select)
          (list '* '* int int)))
         (c-score (struct-ref/unboxed score 0))
         (ptr (make-pointer c-score)))
    (cfun ptr elm type staff)))

(define (ms-score-selection score)
  (let* ((cfun (pointer->procedure '*
            (get-dynfunc ms-score-selection)
            '(*)))
         (c-score (struct-ref/unboxed score 0))
         (ptr (make-pointer c-score)))
    (cfun ptr)))

;;; class measure

; returns a SCM vector of elements
(define (ms-measure-elements mea)
  (ms-measure-elements-wrap (pointer->scm mea)))

(let-syntax
  ((def
    (syntax-rules ()
      ((def name)
       (define (name selection)
         (let* ((cfun (pointer->procedure int
                   (get-dynfunc name)
                   '(*))))
           (cfun selection)))))))
  (def ms-measure-noOffset)
  (def ms-measure-no))

(define (ms-measure-findSegmentR mea segtype tick)
  (let* ((cfun (pointer->procedure '*
                (get-dynfunc ms-measure-findSegmentR)
               (list '* int int))))
    (cfun mea segtype tick)))

(define (ms-measure-mmrest? mea)
  (let* ((cfun (pointer->procedure int
                (get-dynfunc ms-measure-mmrest?)
               '(*))))
    (if (= 0 (cfun mea)) #f #t)))

;;; class Selection

(let-syntax
  ((def
    (syntax-rules ()
      ((def name)
       (define (name selection)
         (let* ((cfun (pointer->procedure int
                   (get-dynfunc name)
                   '(*))))
           (if (= (cfun selection) 0) #f #t)))))))
  (def ms-selection-isRange)
  (def ms-selection-isNone)
  (def ms-selection-isList)
  (def ms-selection-isSingle))

(define (ms-selection-deselectAll selection)
  (let* ((cfun (pointer->procedure void
            (get-dynfunc ms-selection-deselectAll) ;"_ZN2Ms9Selection11deselectAllEv")
            '(*))))
    (cfun selection)))

(let-syntax
  ((def
    (syntax-rules ()
      ((def name arg1type)
       (define (name sel arg)
         (let ((cfun (pointer->procedure void
                  (get-dynfunc name)
                  (list '* arg1type))))
           (cfun sel arg)))))))
  (def ms-selection-setStartSegment '*)
  (def ms-selection-setEndSegment '*)
  ; These are not exported as symbols!
  ; (def ms-selection-setStaffStart int "???") ;  setStaffStart(int v)
  ; (def ms-selection-setStaffEnd int  "???") ; setStaffEnd(int v)
  )

(let-syntax
  ((def
    (syntax-rules ()
      ((def name)
         (define (name sel)
           (let ((cfun (pointer->procedure int
                    (get-dynfunc name)
                    '(*))))
             (cfun sel)))))))
  (def ms-selection-tickStart)
  (def ms-selection-tickend))

(define (ms-selection-add sel elm)
  (let ((cfun (pointer->procedure void
               (get-dynfunc ms-selection-add)
               '(* *))))
    (cfun sel elm)))

(define (ms-selection-update sel)
  (let ((cfun (pointer->procedure void
               (get-dynfunc ms-selection-update)
               '(*))))
    (cfun sel)))

(define (ms-selection-setState sel selstate)
  (let ((cfun (pointer->procedure void
               (get-dynfunc ms-selection-setState)
               (list '* int))))
    (cfun sel selstate)))

; suspect function (hairy logic)
(define (ms-selection-updateSelectedElements sel)
  (let ((cfun (pointer->procedure void
               (get-dynfunc ms-selection-updateSelectedElements)
               '(*))))
    (cfun sel)))

(define (ms-selection-setRangeTicks selection tick1 tick2 staffstart staffend)
  (let* ((cfun (pointer->procedure void
            (get-dynfunc ms-selection-setRangeTicks)
            (list '* int int int int))))
    (cfun selection
          tick1
          tick2
          staffstart
          staffend)))

;;; class Segment

(let-syntax
  ((def
    (syntax-rules ()
      ((def name)
         (define (name seg)
           (let ((cfun (pointer->procedure '*
                    (get-dynfunc name)
                    '(*))))
             (let ((seg/mea (cfun seg)))
               (if (= (pointer-address seg/mea) 0)
                   #f seg/mea))))))))
  (def ms-segment-next1)
  (def ms-segment-next)
  (def ms-segment-prev)
  (def ms-segment-prev1)
  (def ms-segment-next1MM)
  (def ms-segment-prev1MM)
  (def ms-segment-measure))

; segmentType() (NOTE: not the same as ms-element-type)
(define (ms-segment-type seg)
  (let ((cfun (pointer->procedure int
               (get-dynfunc ms-segment-type)
               '(*))))
    (cfun seg)))

; returns a SCM vector of elements
(define (ms-segment-elements seg)
  (ms-segment-elements-wrap (pointer->scm seg)))

(define (ms-segment-element seg track)
  (let ((cfun (pointer->procedure '*
               (get-dynfunc ms-segment-element)
               (list '* int))))
    (let ((elm (cfun seg track)))
      (if (= (pointer-address elm) 0)
          #f elm))))

;;; class Element

(let-syntax
  ((def
    (syntax-rules ()
      ((def name)
         (define (name elm)
           (let ((cfun (pointer->procedure int
                    (get-dynfunc name)
                    '(*))))
             (cfun elm)))))))
  (def ms-element-tick)
  (def ms-barline-type)
  )

(define (ms-element-name elm)
  (let ((cfun (pointer->procedure '*
           (get-dynfunc ms-element-name)
           '(*))))
    (let ((ptr (cfun elm)))
      (pointer->string ptr))))
