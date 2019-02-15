; This test will select a multimeasure
; and perform slash-fill action on the
; selection.
; Ensure outcome doesn't crash, succeed or not.

(let ((score (test-read-score-mscx-file "../../tmp/b2.mscx"))
      (score2 (test-read-score-sxlm
 '(*TOP*  (museScore (@ (version "3.01")) 
   (Score 
     (Part 
       (Staff (@ (id 1)) 
         (StaffType (@ (group pitched)) 
           (name stdNormal)))) 
     (Staff (@ (id 1)) 
       (Measure 
         (voice 
           (TimeSig (sigN 4) (sigD 4)) 
           (Rest 
             (linkedMain) 
             (durationType measure) 
             (duration "4/4")))) 
       (Measure 
         (LayoutBreak 
           (linkedMain) 
           (subtype section)) 
         (voice 
           (Rest 
             (linkedMain) 
             (durationType measure) 
             (duration "4/4")))) 
       (Measure 
         (voice 
           (Rest 
             (linkedMain) 
             (durationType measure) 
             (duration "4/4")))) 
       (Measure 
         (voice 
           (Rest 
             (linkedMain) 
             (durationType measure) 
             (duration "4/4"))))) 
     (Score 
       (Style 
         (createMultiMeasureRests 1)) 
       (Part 
         (Staff (@ (id 1)) 
           (linkedTo 1) 
           (StaffType (@ (group pitched)) 
             (name stdNormal)))) 
       (Staff (@ (id 1)) 
         (Measure 
           (voice 
             (TimeSig (sigN 4) (sigD 4)) 
             (Rest (linked) 
               (durationType measure) 
               (duration "4/4")))) 
         (Measure (@ (len "8/4")) 
           (multiMeasureRest 2) 
           (LayoutBreak (subtype section)) 
           (voice 
             (TimeSig (sigN 4) (sigD 4)) 
             (Rest 
               (durationType measure) 
               (duration "8/4")))) 
         (Measure 
           (LayoutBreak (linked) 
             (subtype section)) 
           (voice 
             (Rest 
               (linked) 
               (durationType measure) 
               (duration "4/4")))) 
         (Measure 
           (voice 
             (Rest 
               (linked) 
               (durationType measure) 
               (duration "4/4")))) 
         (Measure (@ (len "8/4")) 
           (multiMeasureRest 2) 
           (voice 
             (Rest 
               (durationType measure) 
               (duration "28/4")))) 
         (Measure 
           (voice 
             (Rest 
               (linked) 
               (durationType measure) 
               (duration "4/4"))))) 
       ;(name Piano)
       )))))))
  (let ((sel (ms-score-selection score))
        (section-break-element #f))
    ; fish-out the selection-break-element from the score
    (walk-score #f #f
     (lambda (elm)
       (if (string= (ms-element-name elm) "LayoutBreak")
         (set! section-break-element elm)))
     score)
    (ms-score-selectAdd score section-break-element)
    (ms-selection-update sel)

    ;(ms-selection-setRangeTicks sel 0 1920 0 1)
    ;(ms-selection-setStartSegment sel (ms-score-tick2segment score 0))
    ;(ms-selection-setEndSegment sel   (ms-score-tick2segment score 1920))
    ;(let ((seg (ms-score-tick2segment score 0)))
    ;  (let ((elmvec (ms-segment-elements seg)))
    ;    (ms-score-select score (vector-ref elmvec 0) 1 0)))
    (ms-score-cmd score "delete")
    )
  (ms-score-doLayout score)
  (ms-score-update score)
  (ms-score-forget score)) ; C++ delete score
