(use-modules (test gen-score-sxml))

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
