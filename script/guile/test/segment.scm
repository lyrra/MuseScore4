(deftest (segment)
(let ((score (ms-current-score))
      (assert-segmenttype-chordrest
       (lambda (seg)
         (assert (= (ms-segment-type seg) #x200)
                 "wrong segment type: ~x expected #x200"
                 (ms-segment-type seg)))))
  ; get first measure in current score
  (let ((measure (car (ms-score-measures score))))
    (let ((segment (car (ms-measure-segments measure))))
      (assert (ms-segment? segment) "not a segment")

      (assert (ms-segment-next segment)
              "cant get segment-next")
      (assert (not (ms-segment-prev segment))
              "got prev on first segment")

      (let ((seg (ms-segment-next-type segment #x200)))
        (assert-segmenttype-chordrest seg))

      (assert (ms-segment-next1 segment)
              "cant get segment-next1")

      (let ((seg (ms-segment-next1-type segment #x200)))
        (assert-segmenttype-chordrest seg))

      (assert (ms-segment-element segment 0)
              "cant get segment-element")))

  ; get last measure in current score
  (let* ((measure (car (last (ms-score-measures score))))
         (segment (car (last (ms-measure-segments measure)))))
    (assert (ms-measure? measure) "not a measure")
    (assert (not (ms-segment-next segment))
            "got segment-next on last segment")
    (assert (ms-segment-prev segment)
            "got no prev on last segment")
    (assert (not (ms-segment-next1 segment))
            "got segment-next1 on last segment")
    (assert (ms-segment-prev1 segment)
            "got no prev1 on last segment"))))
