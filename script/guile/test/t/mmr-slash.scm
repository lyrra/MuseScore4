; This test will select a multimeasure
; and perform slash-fill action on the
; selection.
; Ensure outcome doesn't crash, succeed or not.

(define (find-bad-measure score)
  (walk-score (lambda (mea)
                (let ((seg (ms-measure-findSegmentR mea #x200 (tick2fraction 0))))
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
  (let ((sel (ms-score-selection score))
        (sf (tick2fraction 0))
        (ef (tick2fraction 1920)))
    (ms-selection-setRangeTicks sel sf ef 0 1)
    (ms-selection-setStartSegment sel (ms-score-tick2segment score sf))
    (ms-selection-setEndSegment sel   (ms-score-tick2segment score ef))
    (let ((seg (ms-score-tick2segment score sf)))
      (let ((elmvec (ms-segment-elements seg)))
        (ms-score-select score (vector-ref elmvec 0) 1 0)))
    (find-bad-measure score)
    (ms-score-cmd score "slash-fill")
    (find-bad-measure score))
  (ms-score-forget score)) ; C++ delete score
