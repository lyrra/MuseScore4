
(define-module (lib chord)
               #:use-module (oop goops)      ; CLOS-like object orientation
               #:use-module (srfi srfi-1)
               #:use-module (musescore-c)
               #:use-module (lib musescore)
               #:use-module (lib utils)
               #:export (walk-chord
                         blending
                         find-blend-class
                         find-blend-invl))

(define (walk-chord sel func)
  (let ((score (slot-ref sel 'score))
        (sseg  (slot-ref sel 'start-segment))
        (eseg  (slot-ref sel 'end-segment))
        (ssta  (slot-ref sel 'start-staff))
        (esta  (slot-ref sel 'end-staff)))
     (let ((stick (ms-segment-tick sseg))
           (etick (ms-segment-tick eseg)))
       (loop-list mea (ms-score-measures (ms-current-score))
         (loop-list seg (ms-measure-segments mea)
           (let ((tick (ms-segment-tick seg)))
             (if (and (>= tick stick) (<= tick etick))
               (let ((chord '()))
                 (loop-list elm (ms-segment-elements seg)
                   (let ((track (ms-element-track elm))
                         (staffIdx (ms-element-staffIdx elm))
                         (voice    (ms-element-voice elm)))
                     (match (ms-element-type elm)
                       (92 ; chord
                        (let ((notes (ms-element-notes elm)))
                          (if (vector? notes)
                            (loop-vec i note notes
                               (let* ((pitch (ms-note-pitch note))
                                      (pc (pitch-class pitch))
                                      (oct (pitch-octave pitch))
                                      (playticks (ms-note-playticks note)))
                                 (push (list #:note pc oct playticks staffIdx voice track)
                                       chord))))))
                       (24 ; rest
                        (let ((ticks (ms-element-ticks elm))
                              (dots  (ms-element-dots  elm)))
                          (push (list #:rest ticks dots staffIdx voice track) chord)))
                       (any any))))
                 (if (not (eq? chord '()))
                   (func seg tick chord))))))))))

; calculate the blending of two notes

; different musical style will treat dissonances separately
; but perhaps the ear will hear the blending of two individual notes
; in isolated context with an absolute degree of dissonance
; Return the degree of dissonance from highest 12 to lowest 0.
(define (blending notea noteb)
  (let* ((diff (note-invl notea noteb))
         (name (note-invl-name diff)))
    (cond
      ((= diff 0)  (cons name  0)) ; P1 unison
      ((= diff 1)  (cons name 10)) ; m2 semitone
      ((= diff 2)  (cons name  8)) ; M2 tone
      ((= diff 3)  (cons name  4)) ; m3 imperfect consonance
      ((= diff 4)  (cons name  4)) ; M3 imperfect consonance
      ((= diff 5)  (cons name  6)) ; P4 dissonance (without context)
      ((= diff 6)  (cons name 12)) ; TT hit dissonance jackpot (tritone)
      ((= diff 7)  (cons name  2)) ; P5 consonance
      ((= diff 8)  (cons name  4)) ; m6 imperfect consonance
      ((= diff 9)  (cons name  4)) ; M6 imperfect consonance
      ((= diff 10) (cons name  8)) ; m7 imperfect consonance
      ((= diff 11) (cons name 10)) ; m7 imperfect consonance
      (else (error "too large note diff: ~a" diff)))))

(define (find-blend-class what blends)
  (fold (lambda (blend acc)
          (match blend
            ((name . diff)
              (if (eq? (invl-class name) what)
                #t acc))
            (else acc)))
        #f
        blends))

(define (find-blend-invl invl blends)
  (fold (lambda (blend acc)
          (match blend
            ((name . diff)
              (if (eq? name invl)
                #t acc))
            (else acc)))
        #f
        blends))
