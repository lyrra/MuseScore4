;;;; FFI libmusescore functions interface

(define (get-dynfunc mangled)
  (dynamic-func mangled (dynamic-link)))

;;; class Score

(let-syntax
  ((def
    (syntax-rules ()
      ((def name mangled)
       (define (name score)
         (let* ((cfun (pointer->procedure int
                   (get-dynfunc mangled)
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
                   (get-dynfunc mangled)
                   (list '* int)))
                (c-score (struct-ref/unboxed score 0))
                (ptr (make-pointer c-score)))
           (let ((retptr (cfun ptr tick)))
             (if (= (pointer-address retptr) 0)
               #f retptr))))))))
  (def ms-score-tick2measure "_ZNK2Ms5Score12tick2measureEi")
  (def ms-score-tick2segment "_ZNK2Ms5Score12tick2segmentEi")
  (def ms-score-tick2measureMM "_ZNK2Ms5Score14tick2measureMMEi")
  (def ms-score-tick2segmentMM "_ZNK2Ms5Score14tick2segmentMMEi"))

(let-syntax
  ((def
    (syntax-rules ()
      ((def name mangled)
       (define (name score)
         (let* ((cfun (pointer->procedure void
                   (get-dynfunc mangled)
                   '(*)))
                (c-score (struct-ref/unboxed score 0))
                (ptr (make-pointer c-score)))
           (cfun ptr)))))))
  (def ms-score-doLayout "_ZN2Ms5Score8doLayoutEv")
  (def ms-score-update "_ZN2Ms5Score6updateEv"))

(let-syntax
  ((def
    (syntax-rules ()
      ((def name mangled)
       (define (name score)
         (let* ((cfun (pointer->procedure void
                   (get-dynfunc mangled)
                   '(*)))
                (c-score (struct-ref/unboxed score 0))
                (ptr (make-pointer c-score)))
           (cfun ptr)))))))
  (def ms-score-first "_ZNK2Ms5Score5firstEv")
  (def ms-score-firstMM "_ZNK2Ms5Score7firstMMEv")
  (def ms-score-firstMeasure "_ZNK2Ms5Score12firstMeasureEv")
  (def ms-score-firstMeasureMM "_ZNK2Ms5Score14firstMeasureMMEv"))

(define (ms-score-crMeasure score idx)
  (let* ((cfun (pointer->procedure '*
          (get-dynfunc "_ZNK2Ms5Score9crMeasureEi")
          (list '* int)))
         (c-score (struct-ref/unboxed score 0))
         (ptr (make-pointer c-score)))
    (cfun ptr idx)))

(define (ms-score-selectAdd score elm)
  (let* ((cfun (pointer->procedure void
          (get-dynfunc "_ZN2Ms5Score9selectAddEPNS_7ElementE")
          '(* *)))
         (c-score (struct-ref/unboxed score 0))
         (ptr (make-pointer c-score)))
    (cfun ptr elm)))

(define (ms-score-select score elm type staff)
  (let* ((cfun (pointer->procedure void
          (get-dynfunc "_ZN2Ms5Score6selectEPNS_7ElementENS_10SelectTypeEi")
          (list '* '* int int)))
         (c-score (struct-ref/unboxed score 0))
         (ptr (make-pointer c-score)))
    (cfun ptr elm type staff)))

(define (ms-score-selection score)
  (let* ((cfun (pointer->procedure '*
            (get-dynfunc "_ZN2Ms5Score9selectionEv")
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
      ((def name mangled)
       (define (name selection)
         (let* ((cfun (pointer->procedure int
                   (get-dynfunc mangled)
                   '(*))))
           (cfun selection)))))))
  (def ms-measure-noOffset "_ZNK2Ms11MeasureBase8noOffsetEv")
  (def ms-measure-no       "_ZNK2Ms11MeasureBase2noEv"))

(define (ms-measure-findSegmentR mea segtype tick)
  (let* ((cfun (pointer->procedure '*
                (get-dynfunc "_ZNK2Ms7Measure12findSegmentRENS_11SegmentTypeEi")
               (list '* int int))))
    (cfun mea segtype tick)))

;;; class Selection

(let-syntax
  ((def
    (syntax-rules ()
      ((def name mangled)
       (define (name selection)
         (let* ((cfun (pointer->procedure int
                   (get-dynfunc mangled)
                   '(*))))
           (if (= (cfun selection) 0) #f #t)))))))
  (def ms-selection-isRange "_ZNK2Ms9Selection7isRangeEv")
  (def ms-selection-isNone  "_ZNK2Ms9Selection6isNoneEv")
  (def ms-selection-isList  "_ZNK2Ms9Selection6isListEv")
  (def ms-selection-isSingle  "_ZNK2Ms9Selection8isSingleEv"))

(define (ms-selection-deselectAll selection)
  (let* ((cfun (pointer->procedure void
            (get-dynfunc "_ZN2Ms9Selection11deselectAllEv")
            '(*))))
    (cfun selection)))

(let-syntax
  ((def
    (syntax-rules ()
      ((def name arg1type mangled)
       (define (name sel arg)
         (let ((cfun (pointer->procedure void
                  (get-dynfunc mangled)
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
                    (get-dynfunc mangled)
                    '(*))))
             (cfun sel)))))))
  (def ms-selection-tickStart "_ZNK2Ms9Selection9tickStartEv")
  (def ms-selection-tickend "_ZNK2Ms9Selection7tickEndEv"))

(define (ms-selection-add sel elm)
  (let ((cfun (pointer->procedure void
               (get-dynfunc "_ZN2Ms9Selection3addEPNS_7ElementE")
               '(* *))))
    (cfun sel elm)))

(define (ms-selection-update sel)
  (let ((cfun (pointer->procedure void
               (get-dynfunc "_ZN2Ms9Selection6updateEv")
               '(*))))
    (cfun sel)))

(define (ms-selection-setState sel selstate)
  (let ((cfun (pointer->procedure void
               (get-dynfunc "_ZN2Ms9Selection8setStateENS_8SelStateE")
               (list '* int))))
    (cfun sel selstate)))

; suspect function (hairy logic)
(define (ms-selection-updateSelectedElements sel)
  (let ((cfun (pointer->procedure void
               (get-dynfunc "_ZN2Ms9Selection22updateSelectedElementsEv")
               '(*))))
    (cfun sel)))

(define (ms-selection-setRangeTicks selection tick1 tick2 staffstart staffend)
  (let* ((cfun (pointer->procedure void
            (get-dynfunc "_ZN2Ms9Selection13setRangeTicksEiiii")
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
      ((def name mangled)
         (define (name seg)
           (let ((cfun (pointer->procedure '*
                    (get-dynfunc mangled)
                    '(*))))
             (let ((seg/mea (cfun seg)))
               (if (= (pointer-address seg/mea) 0)
                   #f seg/mea))))))))
  (def ms-segment-next1   "_ZNK2Ms7Segment5next1Ev")
  (def ms-segment-next    "_ZNK2Ms7Segment4nextEv")
  (def ms-segment-prev    "_ZNK2Ms7Segment4prevEv")
  (def ms-segment-prev1   "_ZNK2Ms7Segment5prev1Ev")
  (def ms-segment-next1MM "_ZNK2Ms7Segment7next1MMEv")
  (def ms-segment-prev1MM "_ZNK2Ms7Segment7prev1MMEv")
  (def ms-segment-measure "_ZNK2Ms7Segment7measureEv"))

; segmentType() (NOTE: not the same as ms-element-type)
(define (ms-segment-type seg)
  (let ((cfun (pointer->procedure int
               (get-dynfunc "_ZNK2Ms7Segment11segmentTypeEv")
               '(*))))
    (cfun seg)))

; returns a SCM vector of elements
(define (ms-segment-elements seg)
  (ms-segment-elements-wrap (pointer->scm seg)))

(define (ms-segment-element seg track)
  (let ((cfun (pointer->procedure '*
               (get-dynfunc "_ZNK2Ms7Segment7elementEi")
               (list '* int))))
    (let ((elm (cfun seg track)))
      (if (= (pointer-address elm) 0)
          #f elm))))

;;; class Element

(let-syntax
  ((def
    (syntax-rules ()
      ((def name mangled)
         (define (name elm)
           (let ((cfun (pointer->procedure int
                    (get-dynfunc mangled)
                    '(*))))
             (cfun elm)))))))
  (def ms-element-tick "_ZNK2Ms7Element4tickEv"))

(define (ms-element-name elm)
  (let ((cfun (pointer->procedure '*
           (get-dynfunc "_ZNK2Ms12ScoreElement4nameEv")
           '(*))))
    (let ((ptr (cfun elm)))
      (pointer->string ptr))))
