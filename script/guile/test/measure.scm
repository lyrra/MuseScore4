
(define (test-score-firstmeasure)
  (for-each (lambda (score)
              (assert (ms-score-firstmeasure score)
                      "firstMeasure failed"))
            (ms-scores)))
