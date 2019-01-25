(let ((score (ms-current-score))
      (assert-segmenttype-chordrest
       (lambda (seg)
         (assert (= (ms-segment-type seg) #x200)
                 "wrong segment type: ~x expected #x200"
                 (ms-segment-type seg)))))
  ; get first measure in current score
  (let ((measure (car (ms-score-measures score))))
    (let ((segment (car (ms-measure-segments measure))))
      (assert (segment? segment) "not a segment")

      (if (not (ms-segment-next segment))
        (error "cant get segment-next"))

      (let ((seg (ms-segment-next-type segment #x200)))
        (assert-segmenttype-chordrest seg))

      (if (not (ms-segment-next1 segment))
        (error "cant get segment-next1"))

      (let ((seg (ms-segment-next1-type segment #x200)))
        (assert-segmenttype-chordrest seg))

      (if (not (ms-segment-element segment 0))
        (error "cant get segment-element")))))
