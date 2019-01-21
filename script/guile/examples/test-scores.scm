
; MS-SCORES-COUNT should return a non-zero number
; if MuseScore has a score open.
(if (<= (ms-scores-count) 0)
  (error "MS-SCORES-COUNT reported <= 0 scores opened."))

; MS-CURRENT-SCORE should return a Score type object
(if (not (eq? (make-foreign-object-type '<ms_obj_score_type>)
              (ms-current-score)))
  (error "MS-CURRENT-SCORE didn't return a object of Score type."))
