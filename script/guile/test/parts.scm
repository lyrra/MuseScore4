(deftest (parts)
  (loop-list score (ms-scores)
    (let ((parts (ms-score-parts score)))
      ; IMP could use score-information to compare expected parts
      (assert (> (length parts) 0) "score has no parts"))))
