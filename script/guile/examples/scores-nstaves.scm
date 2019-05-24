; define a function that returns a list of number of staves in each opened score
(define (scores-nstaves)
  (map (lambda (score)
         (ms-score-nstaves score))
       (ms-scores)))
