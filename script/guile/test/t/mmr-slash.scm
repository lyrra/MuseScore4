; This test will select a multimeasure
; and perform slash-fill action on the
; selection.
; Ensure outcome doesn't crash, succeed or not.

(define (find-bad-measure score)
  (walk-score (lambda (mea)
                (let ((seg (ms-measure-findSegmentR mea #x200 0)))
                  (let ((elm (ms-segment-element seg 0)))
                    (assert (string= (ms-element-name elm) "Rest")
                            "Element is not of Rest type"))))
              #f #f score))

(let ((score (test-read-score-sxlm
 '(*TOP*
   (museScore (@ (version "3.01"))
    (Score
      (Style (createMultiMeasureRests 1))
      (Part (Staff (@ (id 1))))
      (Staff (@ (id 1))
        (Measure
          (voice
            (TimeSig (sigN 4) (sigD 4))
            (Rest
              (durationType measure)
              (duration "4/4"))))
        (Measure (@ (len "8/4"))
          (multiMeasureRest 2)
          (voice
            (TimeSig (sigN 4) (sigD 4))
            (Rest
              (durationType measure)
              (duration "8/4"))))
        (Measure
          (voice
            (Rest
              (durationType measure)
              (duration "4/4")))))))))))
  ;(test-print-score score)
  (let ((sel (ms-score-selection score)))
    (ms-selection-setRangeTicks sel 0 1920 0 1)
    (ms-selection-setStartSegment sel (ms-score-tick2segment score 0))
    (ms-selection-setEndSegment sel   (ms-score-tick2segment score 1920))
    (let ((seg (ms-score-tick2segment score 0)))
      (let ((elmvec (ms-segment-elements seg)))
        (ms-score-select score (vector-ref elmvec 0) 1 0)))
    (find-bad-measure score)
    (ms-score-cmd score "slash-fill")
    (find-bad-measure score))
  (ms-score-forget score)) ; C++ delete score
