;;;; unpolished tests

(define (check-type obj)
  (cond
    ((or (eq? obj #t) (eq? obj #f)) 'bool)
    ((string? obj) 'string)
    ((exact-integer? obj) 'int)
    ((number?  obj) 'double)
    ((staff? obj) 'staff)
    ((score? obj) 'score)
    ((measure? obj) 'measure)
    ((element? obj) 'element)
    ((segment? obj) 'segment)
    ((selection? obj) 'selection)
    ((inputstate? obj) 'inputstate)
    ((vector? obj) 'vec)
    ((list? obj)
      (if (> (length obj) 0)
        (list (check-type (car obj)))
        '()))
    (else 'error)))

(define (make-args parms)
  (map (lambda (parm)
         (cond
           ((string? parm) parm) ; pass-through strings
           ((number? parm) parm) ; pass-through numbers
           ((number? parm) parm) ; pass-through numbers
           ((eq? parm #t) parm)  ; pass-through numbers
           ((eq? parm #f) parm)  ; pass-through numbers
           ((eq? parm 'score) (ms-current-score))
           ((eq? parm 'measure) (car (ms-score-measures (ms-current-score))))
           ((eq? parm 'segment) (car (ms-measure-segments (ms-score-firstmeasure (ms-current-score)))))
           ((eq? parm 'element) (car (ms-segment-elements (ms-score-segment-last (ms-current-score)))))
           ((eq? parm 'staff) (car (ms-score-staves (ms-current-score))))
           ((eq? parm 'inputstate) (ms-score-inputstate (ms-current-score)))
           ((eq? parm 'selection) (ms-score-selection (ms-current-score)))
           (else 'error)))
       parms))

(define (run-func func type . info)
  (cond
    ((list? type)
     (let ((parms (reverse (cdr (reverse type))))
           (etype (car (reverse type))))
       ; parms -- parameters the procedure take
       ; etype -- type the expression evaluates to
       (format #t "Call ~a ~s => ~s~%" func parms etype)
       (let ((args (make-args parms)))
         (format #t "apply args: ~s~%" args)
         (let ((res (apply (primitive-eval func) args)))
           ; check result according to expected return type
           (let ((ctype (check-type res)))
             (format #t "=> ~s :: ~a want: ~s~%" res ctype etype)
             (assert (equal? etype ctype)
                     "~a has wrong result type: ~s, expected ~s"
                     func ctype etype))))))))

(for-each (lambda (args) (apply run-func args))
    '((ms-core-name          (string))
      (ms-core-version       (string))
      (ms-core-experimental  (bool))
      (ms-pan-playback       (bool))
      (ms-play-repeats       (bool))
      (ms-scores-count       (int))
      (ms-current-score      (score))
      (ms-parts              (())) ; FIX: flesh-out ms-parts
      (ms-scoreview-cmd 1    (score string))
      (ms-parts-instruments  (0 (string))) ; 0 = part-index
      (ms-scores-nstaves     ((int)))
      (ms-scores             ((score)))
      (ms-score-nstaves      (score int))
      (ms-score-staves       (score (staff)))
      (ms-score-measures     (score (measure)))
      (ms-score-firstmeasure (score measure))
      (ms-score-segment-last (score segment))
      (ms-score-selection    (score selection))
      (ms-score-inputstate   (score inputstate))
      (ms-inputstate-track!  (inputstate 0 bool))
      (ms-inputstate-segment! (inputstate segment bool))
      (ms-staff-info         (staff vec))
      (ms-measure-first      (measure segment))
      (ms-measure-first-type (measure #x200 segment)) ; SegmentType::ChordRest
      (ms-measure-segments   (measure (segment)))
      (ms-segment-elements   (segment (element)))
      (ms-segment-element (segment 0 element))
      (ms-segment-type  (segment int))
      (ms-segment-next  (segment segment))
      (ms-segment-next1 (segment segment))
      (ms-segment-next-type  (segment 92 segment)) ; chord/rest type
      (ms-segment-next1-type (segment 92 segment)) ; chord/rest type
      (ms-segment-tick (segment int))
      ; FIX: need type language: (or (List) (List note))
      ;(ms-element-notes  (element (note)))
      (ms-element-type (segment int))
      (ms-element-info (element vec))
      (ms-version-major  (int))
      (ms-version-minor  (int))
      (ms-version-update (int))
      (ms-version-check  ("1.2.3" bool))
      (ms-note-mark        (element bool))
      (ms-note-velo-offset (element int))
      (ms-note-subchannel  (element int))
      (ms-note-play        (element bool))
      ;(ms-note-playticks   (element int)) ; segfaults
      (ms-note-tpc         (element int))
      (ms-note-tpc1        (element int))
      (ms-note-tpc2        (element int))
      (ms-note-pitch       (element int))
      ;(ms-note-ppitch      (element int)) ; segfaults
      (ms-note-epitch      (element int))
      (ms-note-tuning      (element double))
      (ms-note-tuning!         (element 1.2 bool))
      (ms-note-subchannel!     (element 0   bool))
      (ms-note-velo-offset!    (element 12  bool))
      ;(ms-note-ontime-offset!  (element 23  bool)) ; segfaults
      ;(ms-note-offtime-offset! (element 32  bool)) ; segfaults
      (ms-note-play!           (element #t  bool))
      (ms-note-pitch!          (element 43  bool))
      ; selection
      (ms-selection-startsegment (selection bool)) ; segment or #f
      (ms-selection-endsegment (selection bool)) ; segment or #f
      (ms-selection-staffstart (selection int))
      (ms-selection-staffend (selection int))))
