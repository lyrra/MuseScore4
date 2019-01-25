(let ((score (ms-current-score)))
  ; get first measure in current score
  (let ((measure (car (ms-score-measures score))))
    (let ((segment (car (ms-measure-segments measure))))
      (assert (segment? segment) "not a segment")

      (if (not (ms-segment-next segment))
        (error "cant get segment-next"))
      (if (not (ms-segment-next-type segment #x200)) ; ElementType::ChordRest
        (error "cant get segment-next-type"))

      (if (not (ms-segment-next1 segment))
        (error "cant get segment-next1"))
      (if (not (ms-segment-next1-type segment #x200)) ; ElementType::ChordRest
        (error "cant get segment-next1-type"))

      (if (not (ms-segment-element segment 0))
        (error "cant get segment-element"))
      )))
