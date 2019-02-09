
(deftest (score-system)
  (let ((scr (ms-current-score)))
    (let ((syslst (ms-score-systems scr)))
      (loop-list sys syslst
        (assert (ms-system? sys)
                "Not a system: ~s" sys)))))
