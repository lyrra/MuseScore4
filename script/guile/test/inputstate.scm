
(define (test-inputstate)
  (let ((score (ms-current-score)))
    (let ((is (ms-score-inputstate score)))
      (if (not is)
        (error "cant get score-inputstate"))
      ; Clear inputstate segment
      (ms-inputstate-segment! is #f)
      (ms-inputstate-track! is 0))))
