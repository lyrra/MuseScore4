
(deftest (system-brackets)
  (let ((scr (ms-current-score)))
    (loop-list sys (ms-score-systems scr)
      (loop-list bra (ms-system-brackets sys)
        (assert (ms-bracket? bra)
                "Not a bracket: ~s" bra)))))
