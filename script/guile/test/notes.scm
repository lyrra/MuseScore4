
(define (test-note note)
  ; test adjust color
  (ms-element-color! note #x876543)
  ; ff is alpha channel
  (assert (= #xff876543 (ms-element-color note)) "wrong note color")
  ; test for note accidental
  (let ((acc (ms-note-accidental note)))
    (if acc
      (assert (ms-accidental? acc) "not an accidental")))
  ; test for note dots
  (let ((dots (ms-note-dots note)))
    (if dots
      (begin
        (assert (list? dots) "not a dot-list")
        (loop-list dot dots
          (assert (element? dot) "dot is not an element"))))
    (assert (integer? (ms-note-numdots note))
            "number of dots is not an integer")))

(deftest (notes)
(loop-list score (ms-scores)
  (let ((mea (ms-score-firstmeasure score)))
    (assert mea "firstMeasure failed")
    (assert (ms-measure? mea) "firstMeasure returned non-measure object")
    (loop-list seg (ms-measure-segments mea)
      (assert (ms-segment? seg) "not a segment")
      (loop-list elm (ms-segment-elements seg)
        (assert (ms-element? elm) "not an element")
        ; Scheme/C shim ensures it is safe
        ; to take the notes of elements no matter element-type
        (let ((notes (ms-element-notes elm)))
          ; if element-type was a chord/rest
          (if (= (ms-element-type elm) 92)
            (loop-vec i note notes
              (assert (ms-note? note) "not a note")
              (test-note note)))))))))
