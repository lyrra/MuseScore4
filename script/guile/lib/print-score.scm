
(define-module (lib print-score)
               #:use-module (musescore-c)
               #:use-module (lib utils)
               #:use-module (ice-9 match)
               #:use-module (srfi srfi-43) ; vector library
               #:export (print-score))

(define (print-score port)
  (format port "---------------------------------------------~%")
  (format port "measures: ~s~%" (ms-score-measures (ms-current-score)))
  (format port "measure-first: ~s~%" (ms-measure-first (car (ms-score-measures (ms-current-score)))))
  (loop-list mea (ms-score-measures (ms-current-score))
    (format port "measure: ~s~%" mea)
    (loop-list seg (ms-measure-segments mea)
      (format port "  segment: ~s~%" seg)
      ; Check for? SegmentType::ChordRest = 0x200
      (loop-list elm (ms-segment-elements seg)
        (let ((track (ms-element-track elm))
              (staffIdx (ms-element-staffIdx elm))
              (voice    (ms-element-voice elm)))
          (format port "    element: ~s type=~s track=~a voice=~a CR=~s~%"
                  elm (ms-element-type elm)
                  track voice
                  (= 92 (ms-element-type elm)) ; ElementType::CHORD = 92
                  )
          (match (ms-element-type elm)
            (92 ; ElementType::CHORD = 92
             (let ((notes (ms-element-notes elm)))
               (if (vector? notes)
                 (vector-for-each
                  (lambda (i note)
                    (let* ((pitch (ms-note-pitch note))
                           (pc (pitch-class pitch))
                           (oct (pitch-octave pitch))
                           (playticks (ms-note-playticks note)))
                      (format port "note[~a-~a ~a, ~a] ~s~%" pc oct pitch playticks note)))
                  notes))))
            (24 ; ElementType::REST  = 24
             (let ((ticks (ms-element-ticks elm))
                   (dots  (ms-element-dots  elm)))
               (format port "rest[~a ~a]~%" ticks dots)))
            (any any))))))
  (format port "---------------------------------------------~%"))
