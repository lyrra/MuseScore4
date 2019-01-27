
  (let-syntax
    ((for
     (syntax-rules ()
       ((for item list body ...)
        (for-each (lambda (item) body ...) list)))))
    (let* ((score (ms-current-score))
           (measures (ms-score-measures score)))
      (for mea measures
        (let ((segments (ms-measure-segments mea)))
          (for seg segments
            (let ((elements (ms-segment-elements seg)))
              (for elm elements
                (let ((elm-info (ms-element-info elm)))
                  (assert (vector? elm-info)
                          "Elm-info not a vector: ~s" elm-info)
                  (assert (= 4 (vector-length elm-info))
                          "Wrong elm-info size: ~s" elm-info)))))))))
