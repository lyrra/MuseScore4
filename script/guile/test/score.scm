
(let ((score (ms-current-score)))
  (assert (ms-score? score) "Not a score: ~s" score))
