
(define (test-segment-next1)
  (let ((score (ms-current-score)))
    ; get first measure in current score
    (let ((measure (car (ms-score-measures score))))
      (let ((segment (car (ms-measure-segments measure))))
        (if (not (ms-segment-next1 segment))
          (error "cant get segment-next1"))
        (if (not (ms-segment-element segment 0))
          (error "cant get segment-element"))))))
