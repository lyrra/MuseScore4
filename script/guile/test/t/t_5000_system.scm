
(deftest (score-system)
  (let ((scr (ms-current-score)))
    (let ((syslst (ms-score-systems scr)))
      (loop-list sys syslst
        (assert (ms-system? sys)
                "Not a system: ~s" sys)))))

(deftest (score-system)
  (let ((scr (ms-current-score)))
    (let ((syslst (ms-score-systems scr)))
      (loop-list sys syslst
        (let ((mea (car (ms-system-measures sys))))
          (assert (ms-measure? mea)
                  "Not a measure: ~s" sys))
        (let ((mea (ms-system-measure sys 2)))
          (assert (ms-measure? mea)
                  "Not a measure: ~s" sys))))))
