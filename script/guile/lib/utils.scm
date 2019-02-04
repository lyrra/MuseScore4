
(define-module (lib utils)
               #:use-module (oop goops)
               #:use-module (ice-9 match)
               #:use-module (srfi srfi-43) ; vector library
               #:use-module (musescore-c)
               #:export (loop-list loop-vec
                         VOICES
                         <selection>
                         get-segment-track-notes
                         pitch-class pitch-octave
                         note-index  note>
                         note-invl note-invl-name
                         invl-class
                         swap-cons))

(define-syntax-rule (loop-list mem list exp ...)
  (for-each (lambda (mem)
              exp ...)
            list))

(define-syntax-rule (loop-vec i mem vec exp ...)
  (vector-for-each (lambda (i mem)
                     exp ...)
            vec))

(define (swap-cons ca cb)
  (let ((cc (cons (car ca) (cdr ca))))
    (set-car! ca (car cb))
    (set-cdr! ca (cdr cb))
    (set-car! cb (car cc))
    (set-cdr! cb (cdr cc))))

(define VOICES 4)

(define (get-segment-track-notes seg track)
  (let ((elm (ms-segment-element seg track)))
    (if (and elm
             (= 92 (ms-element-type elm)))
      (ms-element-notes elm))))

;;; pitch class

(define (pitch-class pitch)
  (let ((i (remainder pitch 12)))
    (vector-ref
      #(c c% d eb e f f% g g% a bb b) i)))

(define (pitch-octave pitch)
  (- (quotient pitch 12) 1))

(define (note-index na)
  (cond ((eq? na 'c ) 0)  ((eq? na 'c%) 1)
        ((eq? na 'd ) 2)  ((eq? na 'eb) 3)
        ((eq? na 'e ) 4)  ((eq? na 'f ) 5)
        ((eq? na 'f%) 6)  ((eq? na 'g ) 7)
        ((eq? na 'g%) 8)  ((eq? na 'a ) 9)
        ((eq? na 'bb) 10) ((eq? na 'b ) 11)
        (else (error "bad note name: ~s" na))))

(define (note> na nb)
  (let ((nai (note-index na))
        (nbi (note-index nb)))
    (> nai nbi)))

(define (note-invl notea noteb)
  ; establish so that variable notea has the lower note
  (let ((na (cons (car notea) (cdr notea)))
        (nb (cons (car noteb) (cdr noteb))))
    (cond
      ((> (cdr notea) (cdr noteb)) ; swap
        (swap-cons na nb))
      ((and (= (cdr notea) (cdr noteb))
            (note> (car notea) (car noteb)))
        (swap-cons na nb))
      (else
        (set! na notea)
        (set! nb noteb)))
    (let* ((nan (note-index (car na)))
           (nbn (note-index (car nb)))
           (diff (- nbn nan)))
      (if (< diff 0) (set! diff (+ diff 12))) ; octave correct
      diff)))

(define (note-invl-name invl)
  (cond
    ((= invl 0)  'P1)
    ((= invl 1)  'm2)
    ((= invl 2)  'M2)
    ((= invl 3)  'm3)
    ((= invl 4)  'M3)
    ((= invl 5)  'P4)
    ((= invl 6)  'TT)
    ((= invl 7)  'P5)
    ((= invl 8)  'm6)
    ((= invl 9)  'M6)
    ((= invl 10) 'm7)
    ((= invl 11) 'M7)
    (else (error "too large note intervall: ~a" invl))))

(define (invl-class invl-name)
  (cond
    ((eq? invl-name 'P1) 'con)
    ((eq? invl-name 'P5) 'con)
    ((eq? invl-name 'P4) 'p4)
    ((eq? invl-name 'TT) 'tt)
    ((or (eq? invl-name 'm3) (eq? invl-name 'M3)
         (eq? invl-name 'm6) (eq? invl-name 'M6))
       'imcon)
    ((or (eq? invl-name 'm2) (eq? invl-name 'M2)
         (eq? invl-name 'm7) (eq? invl-name 'M7))
       'dis)))
