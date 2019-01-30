
(use-modules (ice-9 textual-ports)) ; get-string-all
(use-modules (sxml simple))

; see the script/guile/examples/read-score.scm for how to
; take a string containing the mscx file contents and
; creating a musescore Score object.

; This script takes it a bit further where we
; convert a SXML tree into the string, and then
; convert that string (mscx) into a Score object.

(define read-score-sxml-1
        '(*TOP* ; (*PI* xml version="1.0" encoding="UTF-8")
 (museScore (@ (version 3.01)) 
   (programVersion "3.0.2") 
   (programRevision 3543170) 
   (Score 

     (Part 
       (Staff (@ (id 1)) 
         (StaffType (@ (group pitched))) 
         (bracket (@ (type -1) (span 1) (col 0)))) 
       (trackName Soprano) 
       (Instrument 
         (trackName Soprano) 
         (minPitchP 60) 
         (maxPitchP 84) 
         (minPitchA 60) 
         (maxPitchA 79) 
         (Articulation 
           (velocity 100) 
           (gateTime 100)) 
         (Articulation (@ (name staccato)) 
           (velocity 100) 
           (gateTime 50)) 
         (Articulation (@ (name tenuto)) 
           (velocity 100) 
           (gateTime 100)) 
         (Articulation (@ (name sforzato)) 
           (velocity 120) 
           (gateTime 100)) 
         (Channel 
           (program (@ (value 74))) 
           (controller (@ (value 30) (ctrl 93))) 
           (controller (@ (value 30) (ctrl 91))) 
           (synti Fluid)))) 

     (Part 
       (Staff (@ (id 2)) 
         (StaffType (@ (group pitched)) 
           (name stdNormal))
         (defaultClef F)
         (barLineSpan 1)) 
       (show 0) 
       (trackName Harpsichord) 
       (Instrument 
         (shortName Hch.) 
         (trackName Harpsichord) 
         (minPitchP 29) 
         (maxPitchP 89) 
         (minPitchA 29) 
         (maxPitchA 89) 
         (instrumentId keyboard.harpsichord) 
         (clef (@ (staff 2)) F) 
         (Articulation 
           (velocity 100) 
           (gateTime 100)) 
         (Articulation (@ (name staccatissimo)) 
           (velocity 100) 
           (gateTime 33)) 
         (Articulation (@ (name staccato)) 
           (velocity 100) 
           (gateTime 50)) 
         (Articulation (@ (name portato)) 
           (velocity 100) 
           (gateTime 67)) 
         (Articulation (@ (name tenuto)) 
           (velocity 100) 
           (gateTime 100)) 
         (Articulation (@ (name marcato)) 
           (velocity 120) 
           (gateTime 67)) 
         (Articulation (@ (name sforzato)) 
           (velocity 120) 
           (gateTime 100)) 
         (Channel 
           (program (@ (value 6))) 
           (synti Fluid)))) 

     (Staff (@ (id "1")) 
       (Measure 
         (voice 
           (TimeSig (sigN 4) (sigD 2)) 
           (FiguredBass 
             (ticks 960) 
             (FiguredBassItem 
               (brackets (@ (b4 0) (b3 0) (b2 0) (b1 0) (b0 0))) 
               (digit 6)) 
             (FiguredBassItem 
               (brackets (@ (b4 0) (b3 0) (b2 0) (b1 0) (b0 0))) 
               (digit 4))) 
           (Chord 
             (durationType half) 
             (Note (pitch 74) (tpc 16))) 
           (FiguredBass 
             (ticks 960) 
             (FiguredBassItem 
               (brackets (@ (b4 0) (b3 0) (b2 0) (b1 0) (b0 0))) 
               (digit 6))) 
           (Chord 
             (durationType half) 
             (Note (pitch 72) (tpc 14))) 
           (FiguredBass 
             (ticks 960) 
             (FiguredBassItem 
               (brackets (@ (b4 0) (b3 0) (b2 0) (b1 0) (b0 0))) 
               (prefix 2) 
               (digit 6))) 
           (Chord 
             (durationType half) 
             (Note (pitch 71) (tpc 19))) 
           (FiguredBass 
             (ticks 960) 
             (FiguredBassItem 
               (brackets (@ (b4 0) (b3 0) (b2 0) (b1 0) (b0 "0"))) 
               (prefix 4))) 
           (Chord 
             (durationType half) 
             (Note (pitch 69) (tpc 17)))))) 
     (Staff (@ (id "2")) 
       (Measure 
         (voice 
           (TimeSig 
             (sigN 4) 
             (sigD 2)) 
           (Rest 
             (durationType measure) 
             (duration "4/2")))))))))

;-----------------------------------------------------------------

(define (sxml-to-xml sxml)
  (with-output-to-string
   (lambda () (sxml->xml sxml))))

(define (test-read-score-sxlm sxml)
  (let ((str (sxml-to-xml sxml)))
    (let ((score (ms-score-read-string str)))
      (assert (ms-score? score) "Not a score obj")
      (let ((measures (ms-score-measures score)))
        (assert (= (length measures) 1) "Wrong number of measures")
        (for-each (lambda (mea)
                    (assert (ms-measure? mea) "Not a measure obj")
                    (for-each (lambda (seg)
                                (assert (ms-segment? seg) "Not a segment obj"))
                              (ms-measure-segments mea)))
                  (ms-score-measures score))))))

(test-read-score-sxlm read-score-sxml-1)
