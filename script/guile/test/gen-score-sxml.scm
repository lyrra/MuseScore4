
(use-modules (ice-9 textual-ports)) ; get-string-all
(use-modules (sxml simple))

; see the scripts script/guile/examples/read-score.scm for how to
; and script/guile/test/read-score-sxml.scm for more introductory
; information

; this script builds on read-score-sxml.scm and
; replaces the static sxml with generators 

; FIX: perhaps use a module, due to risc of name-collision

(define (score cont)
  (lambda ()
    (list (list '*TOP* ; (*PI* xml version="1.0" encoding="UTF-8")
                (list 'museScore '(@ (version 3.01)) 
                  '(programVersion "3.0.2") 
                  '(programRevision 3543170) 
                  (list 'Score
                        (cont)))))))

(define (chord pitch dur tpc)
  (list 'Chord 
    (list 'durationType dur) 
    (list 'Note (list 'pitch 74) (list 'tpc 16))))

(define (fig-bass ticks args)
  (append
   (list 'FiguredBass 
         (list 'ticks ticks))
   (map (lambda (pair)
          (match pair
            ((digit prefix)
             (append (list 'FiguredBassItem 
                           (list 'brackets '(@ (b4 0) (b3 0) (b2 0) (b1 0) (b0 0))) 
                           (if prefix (list 'prefix prefix) '()) 
                           (if digit (list 'digit digit) '()))))))
        args)))

(define (timesig n d)
  (list 'TimeSig (list 'sigN n) (list 'sigD d)))

(define (instrument tname sname body)
  (append (list 'Instrument
            (list 'trackName tname)
            (list 'shortName sname)
            '(minPitchP 60) '(maxPitchP 84)
            '(minPitchA 60) '(maxPitchA 79)
          '(Articulation
             (velocity 100)
             (gateTime 100)))
           body))

(define (measure)
  (list 'Measure
    (append
      (list 'voice (timesig 4 2))
      (apply append
       (map (lambda (args)
              (match args
                ((ticks pitch tpc fb)
                 (list
                   (fig-bass ticks fb)
                   (chord pitch 'half tpc)))))
            '((960 74 16 ((6 #f) (4 #f)))
              (960 72 14 ((6 #f)))
              (960 71 19 ((6 2)))
              (960 69 17 ((#f 4)))))))))

(define (meat measure-gen)
        (list
     (list 'Part 
       '(Staff (@ (id 1)) 
         (StaffType (@ (group pitched))) 
         (bracket (@ (type -1) (span 1) (col 0)))) 
       '(trackName Soprano) 
       (instrument "Soprano" "S"
         '((Channel 
           (program (@ (value 74))) 
           (controller (@ (value 30) (ctrl 93))) 
           (controller (@ (value 30) (ctrl 91))) 
           (synti Fluid)))))
     (list 'Part 
       '(Staff (@ (id 2)) 
         (StaffType (@ (group pitched)) 
           (name stdNormal))
         (defaultClef F)
         (barLineSpan 1)) 
       '(show 0) 
       '(trackName Harpsichord) 
       (instrument "Harpsichord" "Hch."
         '((instrumentId keyboard.harpsichord) 
           (clef (@ (staff 2)) F) 
           (Channel 
             (program (@ (value 6))) 
             (synti Fluid)))))
     (list 'Staff '(@ (id "1"))
       (measure-gen))
     '(Staff (@ (id "2")) 
       (Measure 
         (voice 
           (timesig 4 2)
           (Rest (durationType measure) (duration "4/2")))))))

;-----------------------------------------------------------------

(define (sxml-to-xml sxml)
  (with-output-to-string
   (lambda () (sxml->xml sxml))))

(define (test-read-score-sxlm sxml)
  (let ((str (sxml-to-xml sxml)))
    (let ((score (ms-score-read-string str)))
      (assert (ms-score? score) "Not a score obj")
      (format #t "  score: ~s~%" score)
      (let ((measures (ms-score-measures score)))
        (assert (= (length measures) 1) "Wrong number of measures")
        (for-each (lambda (mea)
                    (format #t "  measure: ~s~%" mea)
                    (assert (ms-measure? mea) "Not a measure obj")
                    (for-each (lambda (seg)
                                (format #t "    segment: ~s~%" seg)
                                (assert (ms-segment? seg) "Not a segment obj"))
                              (ms-measure-segments mea)))
                  (ms-score-measures score)))
      (ms-score-forget score))))

(deftest (gen-score-sxml)
  (let* ((cont (lambda ()
                 (meat measure)))
         (sgen (score cont)))
    (let ((sxml (sgen)))
      (test-read-score-sxlm sxml))))
