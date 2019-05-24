
(deftest (measure-find)
  (let ((mea (ms-score-firstmeasure (ms-current-score))))
    (let ((seg (ms-measure-find-segment mea #x200 0)))
      (assert (ms-segment? seg)
              "Not a segment: ~s" seg))))

(deftest (measure-tick/s)
  (let ((mea (ms-score-firstmeasure (ms-current-score))))
    (assert (= 0 (ms-measure-tick mea))
            "first measure position is zero")
    (assert (> (ms-measure-ticks mea) 0)
            "measure length in ticks is zero")))
