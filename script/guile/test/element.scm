
(let* ((score (ms-current-score))
       (measures (ms-score-measures score)))
  (loop-list mea measures
    (let ((segments (ms-measure-segments mea)))
      (loop-list seg segments
        (let ((elements (ms-segment-elements seg)))
          (loop-list elm elements
            (let ((elm-info (ms-element-info elm)))
              (assert (vector? elm-info)
                      "Elm-info not a vector: ~s" elm-info)
              (assert (= 4 (vector-length elm-info))
                      "Wrong elm-info size: ~s" elm-info))))))))
