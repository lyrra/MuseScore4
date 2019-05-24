(deftest (inputstate)
(let ((score (ms-current-score)))
  (let ((is (ms-score-inputstate score)))
    (assert is (error "cant get score-inputstate"))
    (assert (ms-inputstate? is) "not inputstate type: ~s" is)
    ; Clear inputstate segment
    (ms-inputstate-segment! is #f)
    (ms-inputstate-track! is 0))))
