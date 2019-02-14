;;; FFI libmusescore functions interface

;;; class Score

(let-syntax
  ((def
    (syntax-rules ()
      ((def name mangled)
       (define (name score)
         (let* ((cfun (pointer->procedure int
                   (dynamic-func mangled (dynamic-link))
                   '(*)))
                (c-score (struct-ref/unboxed score 0))
                (ptr (make-pointer c-score)))
           (cfun ptr)))))))
  (def ms-score-nstaves "_ZNK2Ms5Score7nstavesEv")
  (def ms-score-ntracks "_ZNK2Ms5Score7ntracksEv")
  (def ms-score-pos     "_ZN2Ms5Score3posEv"))

(let-syntax
  ((def
    (syntax-rules ()
      ((def name mangled)
       (define (name score tick)
         (let* ((cfun (pointer->procedure '*
                   (dynamic-func mangled (dynamic-link))
                   (list '* int)))
                (c-score (struct-ref/unboxed score 0))
                (ptr (make-pointer c-score)))
           (let ((retptr (cfun ptr tick)))
             (if (= (pointer-address retptr) 0)
               #f retptr))))))))
  (def ms-score-tick2measure "_ZNK2Ms5Score12tick2measureEi")
  (def ms-score-tick2segment "_ZNK2Ms5Score12tick2segmentEi"))

(let-syntax
  ((def
    (syntax-rules ()
      ((def name mangled)
       (define (name score)
         (let* ((cfun (pointer->procedure void
                   (dynamic-func mangled (dynamic-link))
                   '(*)))
                (c-score (struct-ref/unboxed score 0))
                (ptr (make-pointer c-score)))
           (cfun ptr)))))))
  (def ms-score-doLayout "_ZN2Ms5Score8doLayoutEv")
  (def ms-score-update "_ZN2Ms5Score6updateEv")
  )

(define (ms-score-selectAdd score elm)
  (let* ((cfun (pointer->procedure void
          (dynamic-func "_ZN2Ms5Score9selectAddEPNS_7ElementE" (dynamic-link))
          '(* *)))
         (c-score (struct-ref/unboxed score 0))
         (ptr (make-pointer c-score)))
    (cfun ptr elm)))

(define (ms-score-select score elm type staff)
  (let* ((cfun (pointer->procedure void
          (dynamic-func "_ZN2Ms5Score6selectEPNS_7ElementENS_10SelectTypeEi" (dynamic-link))
          (list '* '* int int)))
         (c-score (struct-ref/unboxed score 0))
         (ptr (make-pointer c-score)))
    (cfun ptr elm type staff)))

(define (ms-score-selection score)
  (let* ((cfun (pointer->procedure '*
            (dynamic-func "_ZN2Ms5Score9selectionEv" (dynamic-link))
            '(*)))
         (c-score (struct-ref/unboxed score 0))
         (ptr (make-pointer c-score)))
    (cfun ptr)))

;;; class Selection

(let-syntax
  ((def
    (syntax-rules ()
      ((def name mangled)
       (define (name selection)
         (let* ((cfun (pointer->procedure int
                   (dynamic-func mangled (dynamic-link))
                   '(*))))
           (if (= (cfun selection) 0) #f #t)))))))
  (def ms-selection-isRange "_ZNK2Ms9Selection7isRangeEv")
  (def ms-selection-isNone  "_ZNK2Ms9Selection6isNoneEv")
  (def ms-selection-isList  "_ZNK2Ms9Selection6isListEv")
  (def ms-selection-isSingle  "_ZNK2Ms9Selection8isSingleEv"))

(define (ms-selection-deselectAll selection)
  (let* ((cfun (pointer->procedure void
            (dynamic-func "_ZN2Ms9Selection11deselectAllEv" (dynamic-link))
            '(*))))
    (cfun selection)))

(let-syntax
  ((def
    (syntax-rules ()
      ((def name arg1type mangled)
       (define (name sel arg)
         (let ((cfun (pointer->procedure void
                  (dynamic-func mangled (dynamic-link))
                  (list '* arg1type))))
           (cfun sel arg)))))))
  (def ms-selection-setStartSegment '* "_ZN2Ms9Selection15setStartSegmentEPNS_7SegmentE")
  (def ms-selection-setEndSegment '* "_ZN2Ms9Selection13setEndSegmentEPNS_7SegmentE")
  ; These are not exported as symbols!
  ; (def ms-selection-setStaffStart int "???") ;  setStaffStart(int v)
  ; (def ms-selection-setStaffEnd int  "???") ; setStaffEnd(int v)
  )

(let-syntax
  ((def
    (syntax-rules ()
      ((def name mangled)
         (define (name sel)
           (let ((cfun (pointer->procedure int
                    (dynamic-func mangled (dynamic-link))
                    '(*))))
             (cfun sel)))))))
  (def ms-selection-tickStart "_ZNK2Ms9Selection9tickStartEv")
  (def ms-selection-tickend "_ZNK2Ms9Selection7tickEndEv"))

(define (ms-selection-add sel elm)
  (let ((cfun (pointer->procedure void
               (dynamic-func "_ZN2Ms9Selection3addEPNS_7ElementE" (dynamic-link))
               '(* *))))
    (cfun sel elm)))

(define (ms-selection-update sel)
  (let ((cfun (pointer->procedure void
               (dynamic-func "_ZN2Ms9Selection6updateEv" (dynamic-link))
               '(*))))
    (cfun sel)))

(define (ms-selection-setState sel selstate)
  (let ((cfun (pointer->procedure void
               (dynamic-func "_ZN2Ms9Selection8setStateENS_8SelStateE" (dynamic-link))
               (list '* int))))
    (cfun sel selstate)))

; suspect function (hairy logic)
(define (ms-selection-updateSelectedElements sel)
  (let ((cfun (pointer->procedure void
               (dynamic-func "_ZN2Ms9Selection22updateSelectedElementsEv" (dynamic-link))
               '(*))))
    (cfun sel)))

(define (ms-selection-setRangeTicks selection tick1 tick2 staffstart staffend)
  (let* ((cfun (pointer->procedure void
            (dynamic-func "_ZN2Ms9Selection13setRangeTicksEiiii" (dynamic-link))
            (list '* int int int int))))
    (cfun selection
          tick1
          tick2
          staffstart
          staffend)))

(define (ms-foobar2 seg)
  (let ((cfun (pointer->procedure void
               (dynamic-func "_Z10ms_foobar2Pv" (dynamic-link))
               '(*))))
    (cfun seg)))

;;; class Segment

(let-syntax
  ((def
    (syntax-rules ()
      ((def name mangled)
         (define (name seg)
           (let ((cfun (pointer->procedure '*
                    (dynamic-func mangled (dynamic-link))
                    '(*))))
             (let ((seg/mea (cfun seg)))
               (if (= (pointer-address seg/mea) 0)
                   #f seg/mea))))))))
  (def ms-segment-next1   "_ZNK2Ms7Segment5next1Ev")
  (def ms-segment-next    "_ZNK2Ms7Segment4nextEv")
  (def ms-segment-prev    "_ZNK2Ms7Segment4prevEv")
  (def ms-segment-prev1   "_ZNK2Ms7Segment5prev1Ev")
  (def ms-segment-measure "_ZNK2Ms7Segment7measureEv"))

; returns a SCM vector of elements
(define (ms-segment-elements seg)
  (ms-segment-elements-wrap (pointer->scm seg)))

;;; class Element

(let-syntax
  ((def
    (syntax-rules ()
      ((def name mangled)
         (define (name elm)
           (let ((cfun (pointer->procedure int
                    (dynamic-func mangled (dynamic-link))
                    '(*))))
             (cfun elm)))))))
  (def ms-element-tick "_ZNK2Ms7Element4tickEv"))

(define (ms-element-name elm)
  (let ((cfun (pointer->procedure '*
           (dynamic-func "_ZNK2Ms12ScoreElement4nameEv" (dynamic-link))
           '(*))))
    (let ((ptr (cfun elm)))
      (pointer->string ptr))))
