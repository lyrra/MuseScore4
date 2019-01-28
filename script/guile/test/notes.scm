
(define (test-note note)
  (ms-element-color! note #x876543)
  ; ff is alpha channel
  (assert (= #xff876543 (ms-element-color note)) "wrong note color"))

(for-each
    (lambda (score)
      (let ((mea (ms-score-firstmeasure score)))
        (assert mea "firstMeasure failed")
        (assert (measure? mea) "firstMeasure returned non-measure object")
        (for-each
            (lambda (seg)
              (assert (segment? seg) "not a segment")
              (for-each
                  (lambda (elm)
                    (assert (element? elm) "not an element")
                    ; Scheme/C shim ensures it is safe
                    ; to take the notes of elements no matter element-type
                    (let ((notes (ms-element-notes elm)))
                      ; if element-type was a chord/rest
                      (if (= (ms-element-type elm) 92)
                        (vector-for-each
                            (lambda (i note)
                              (assert (note? note) "not a note")
                              (test-note note))
                            notes))))
                  (ms-segment-elements seg)))
            (ms-measure-segments mea))))
    (ms-scores))
