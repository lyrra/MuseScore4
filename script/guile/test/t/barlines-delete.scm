; This test will select a multimeasure
; and perform slash-fill action on the
; selection.
; Ensure outcome doesn't crash, succeed or not.

(let ((score (test-read-score-sxlm
 '(*TOP*
  (museScore (@ (version "3.01"))
   (Score
     (Part
       (Staff (@ (id 1))
         (StaffType (@ (group pitched))
           (name stdNormal))))
     (Staff (@ (id 1))
       (Measure
         (voice
           (TimeSig (sigN 4) (sigD 4))
           (Tempo (tempo 2)
                  (followText 1)
                  (text Tempo text))
           (Chord (durationType quarter)
                  (Note (pitch 72) (tpc 14)))
           (Chord (durationType quarter)
                  (Note (pitch 76) (tpc 18)))
           (Chord (durationType quarter)
                  (Note (pitch 79) (tpc 15)))
           (Chord (durationType quarter)
                  (Note (pitch 76) (tpc 18)))
           (BarLine (subtype double))))
       (Measure
         (voice
           (Chord (durationType quarter)
                  (Note (pitch 72) (tpc 14)))
           (Chord (durationType quarter)
                  (Note (pitch 67) (tpc 15)))
           (Chord (durationType quarter)
                  (Note (pitch 72) (tpc 14)))
           (Chord (durationType quarter)
                  (Note (pitch 76) (tpc 18)))
           (BarLine (subtype dashed))))
       (Measure
         (voice
           (RehearsalMark (text 1))
           (Chord
             (durationType quarter) 
             (Note (pitch 79) (tpc 15))) 
           (Rest (durationType quarter)) 
           (Rest (durationType half)))) 
       (Measure 
         (LayoutBreak (subtype line)) 
         (voice 
           (Rest (durationType measure) 
                 (duration "4/4")))) 
       (Measure 
         (voice 
           (Rest (durationType measure) 
                 (duration "4/4")) 
           (BarLine (subtype double)))) 
       (Measure 
         (voice 
           (StaffText (text Staff text)) 
           (Rest (durationType measure) 
                 (duration "4/4")) 
           (Fermata (subtype fermataAbove)))) 
       (Measure 
         (voice 
           (Rest (durationType measure) 
                 (duration "4/4")) 
           (BarLine (spanFromOffset 2) 
                    (spanToOffset -2)))) 
       (Measure 
         (voice 
           (SystemText (text System text)) 
           (Rest (durationType measure) 
                 (duration "4/4")))) 
       (Measure 
         (voice 
           (TimeSig (sigN 3) (sigD 4)) 
           (Rest (durationType measure) 
                 (duration "3/4")))) 
       (Measure 
         (voice 
           (Rest (durationType measure) 
                 (duration "3/4")))))))))))
  ; this test will not do a full diff, instead it
  ; will just check that the barlines are intact
  (let ((before '())
        (after  '())
        (collect-barline-types
         (lambda ()
           (collect push
             (walk-score #f #f 
               (lambda (elm)
                 (if (string= (ms-element-name elm) "BarLine")
                   (push (ms-barline-type elm))))
               score)))))
      ; collect all barline types into a 'before' list 
      (set! before (collect-barline-types))
      ; perform the select-all and delete operations
      (ms-score-cmd score "select-all")
      (ms-score-cmd score "delete")
      (ms-score-doLayout score)
      ; Now collect all barlines again into an 'after' list
      (set! after (collect-barline-types))
      (assert (equal? before after)
              "Barlines are not equal after select/delete:~%Before: ~s~%After: ~s~%" before after))
  (ms-score-forget score)) ; C++ delete score
