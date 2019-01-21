; This function will walk over each score,
; and for each score it will walk over each measure
; and for each measure it will walk over each segment
; and for each segment it will get a list of all elements
; It will thus return a tree-like list.
(define (scores-elements)
  (map (lambda (score)
         (map (lambda (measure)
                (map (lambda (segment)
                       (ms-segment-elements segment))
                     (ms-measure-segments measure)))
              (ms-score-measures score)))
       (ms-scores)))
