
(define-module (lib chord)
               #:use-module (oop goops)      ; CLOS-like object orientation
               #:use-module (srfi srfi-1)
               #:use-module (ice-9 match)
               #:use-module (musescore-c)
               #:use-module (lib musescore)
               #:use-module (lib utils)
               #:use-module (lib common)
               #:export (walk-chord
                         blending
                         find-blend-class
                         find-blend-invl
                         invl-bit?))

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
                                 (push (list #:note note pc oct playticks staffIdx voice track)
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
; but perhaps the ear of any time period, will hear the blending
; of two individual notes in a isolated context, having same
; degree of blending.
; Return the degree of dissonance (blending) from highest 12 to lowest 0.
(define (blending notea noteb)
  (let* ((diff (note-invl notea noteb))
         (name (note-invl-name diff)))
    (cond
      ;                            ; #  Blend factor
      ((= diff 0)  (cons name  0)) ; P1 pure (also octave)
      ((= diff 1)  (cons name 10)) ; m2 discordant
      ((= diff 2)  (cons name  8)) ; M2 tense
      ((= diff 3)  (cons name  4)) ; m3 norm (imperfect cons.)
      ((= diff 4)  (cons name  4)) ; M3 norm (imperfect cons.)
      ((= diff 5)  (cons name  6)) ; P4 optimal (context dependent)
      ((= diff 6)  (cons name 12)) ; TT hit dissonance jackpot (tritone)
      ((= diff 7)  (cons name  2)) ; P5 optimal (perfect consonance)
      ((= diff 8)  (cons name  4)) ; m6 discordant (in context) impc
      ((= diff 9)  (cons name  4)) ; M6 tense impc
      ((= diff 10) (cons name  8)) ; m7 tense
      ((= diff 11) (cons name 10)) ; M7 discordant
      (else (error "too large note diff: ~a" diff)))))

(define (find-blend-class what blends)
  (fold (lambda (blend acc)
          (match blend
            ((name . diss)
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

(define (invl-bit? invl int)
  (logbit? (note-invl-index int) invl))
