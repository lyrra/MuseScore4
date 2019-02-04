
(define-module (lib selection)
               #:use-module (oop goops)      ; CLOS-like object orientation
               #:use-module (musescore-c)
               #:use-module (lib musescore)
               #:export (get-selection-or-whole
                         <selection>))

(define-class <selection> ()
  (score         #:init-keyword #:score)
  (start-segment #:init-keyword #:start-segment)
  (end-segment   #:init-keyword #:end-segment)
  (start-staff   #:init-keyword #:start-staff)
  (end-staff     #:init-keyword #:end-staff))

(define (get-selection-or-whole)
  (let ((score (ms-current-score)))
    (if (not (and score (ms-score? score)))
      (error "Please open a score file!"))
    (let* ((selection (ms-score-selection score))
           (start-segment (ms-selection-startsegment selection))
           (end-segment   (ms-selection-endsegment   selection))
           (start-staff   (ms-selection-staffstart   selection))
           (end-staff     (ms-selection-staffend     selection)))
      (if (and start-segment end-segment)
        (begin ; a selection has been made
          (if (not (ms-segment? start-segment)) (error "not a segment"))
          (set! end-staff (- end-staff 1)))
        (begin ; no selection, use whole score
          (if (not (ms-score? score)) (error "not a score"))
          (set! start-segment (ms-measure-first (ms-score-firstmeasure score)))
          (set! end-segment (ms-score-segment-last score))
          (set! start-staff 0)
          (set! end-staff (- (ms-score-nstaves score) 1))))
      (make <selection>
            #:score score
            #:start-segment start-segment
            #:end-segment end-segment
            #:start-staff start-staff
            #:end-staff end-staff))))
