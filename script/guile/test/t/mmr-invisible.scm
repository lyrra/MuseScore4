; Ensure no crash when enabling MMRest.
; An invisible staff caused crashes

(let ((score (test-read-score-sxlm
 '(*TOP*
  (museScore (@ (version "3.01")) 
   (Score 
     (Division 480) 
     (showInvisible 1) 
     (Part 
       (Staff (@ (id 1)) 
         (StaffType (@ (group pitched)) 
           (name stdNormal)))) 
     (Part 
       (Staff (@ (id 2)) 
         (StaffType (@ (group pitched)) 
           (name stdNormal)) 
         (defaultClef F)) 
       (show 0)) 
     (Staff (@ (id 1)) 
       (Measure 
         (voice 
           (TimeSig 
             (sigN 4) 
             (sigD 4)) 
           (Rest 
             (durationType measure) 
             (duration "4/4")))) 
       (Measure 
         (LayoutBreak 
           (subtype line)) 
         (voice 
           (Rest 
             (durationType measure) 
             (duration "4/4")))) 
       (Measure 
         (voice 
           (Chord 
             (durationType whole) 
             (Note 
               (pitch 67) 
               (tpc 15)))))) 
     (Staff (@ (id 2)) 
       (Measure 
         (voice 
           (TimeSig (sigN 4) (sigD 4)) 
           (Rest 
             (durationType measure) 
             (duration "4/4")))) 
       (Measure 
         (voice 
           (Chord 
             (durationType whole) 
             (Note 
               (Spanner (@ (type Tie)) 
                 (Tie) 
                 (next 
                   (location (measures 1)))) 
               (pitch 55) 
               (tpc 15))))) 
       (Measure 
         (voice 
           (Chord 
             (durationType whole) 
             (Note 
               (Spanner (@ (type Tie)) 
                 (prev 
                   (location (measures -1)))) 
               (pitch 55) 
               (tpc 15))))))))))))
  ;(test-print-score score)
  (ms-score-cmd score "toggle-mmrest")
  (ms-score-forget score)) ; C++ delete score
