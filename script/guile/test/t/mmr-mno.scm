; This test will check measure numbering
; with both MMR-on and MMR-off

(let ((score (test-read-score-sxlm
 '(*TOP*
  (museScore (@ (version "3.01"))
   (Score 
     (Division 480) 
     (Style 
       (createMultiMeasureRests 1) 
       (showMeasureNumberOne 1)) 
     (Part 
       (Staff (@ (id 1)) 
         (StaffType (@ (group pitched))) 
         (bracket (@ (type 0) (span 2) (col 0))))) 
     (Staff (@ (id 1)) 
       (Measure 
         (noOffset 167) 
         (voice 
           (TimeSig (sigN 4) (sigD 4)) 
           (Rest 
             (durationType measure) 
             (duration "1/1")))) 
       (Measure (@ (len "8/4")) 
         (multiMeasureRest 2) 
         (voice 
           (TimeSig (sigN 4) (sigD 4)) 
           (Rest 
             (durationType measure) 
             (duration "8/4")))) 
       (Measure 
         (voice 
           (Rest 
             (durationType measure) 
             (duration "1/1")))))))))))
  ; test-program
  (ms-score-update score)
  ; Ensure that, after update (which is done also after each
  ; command (by libmusescore)), that each measure has a
  ; sane measure count and from correct offset
  ;(test-print-score score)
  (walk-score (lambda (mea)
                (assert (< (ms-measure-no mea) 170)
                        "Measure count is wrong"))
              #f #f score)
  (ms-score-cmd score "toggle-mmrest") ; will update score
  ;(test-print-score score)
  (walk-score (lambda (mea)
                (assert (< (ms-measure-no mea) 170)
                        "Measure count is wrong"))
              #f #f score)
  (ms-score-forget score)) ; C++ delete score
