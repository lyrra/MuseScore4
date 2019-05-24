; Define a function that return a list of all staves in all opened scores
(define (scores-staves)
  (map (lambda (score)
         (ms-score-staves score))
       (ms-scores)))
