; run this test as so:
; build.debug/mscore/mscore --no-gui --script-scheme=script/guile/test/note-simple.scm e1.mscz
; Load an scheme module that makes output print formatting nicer
(use-modules (ice-9 format))

; define a simple assert macro that lets out quit when
; an error has occured. Check that the unix-return-code is 0.
(define-syntax-rule (assert expr msg ...)
  (if (not expr)
    (begin
      (format #t msg ...)
      (format #t "~%")
      (quit 111))))

(primitive-load "script/guile/examples/elements.scm")

(let ((elements (scores-elements))
      (note #f))
  (set! elements (apply append elements))
  (set! elements (apply append elements))
  (set! elements (apply append elements))
  (format #t "~s~%" elements)
  (for-each (lambda (element)
              (if (and (not note) ; get first chord/note element
                       (= (ms-element-type element) 92)) ; chord-type
                (set! note element)))
            elements)
  (format #t "element info ~s~%" (ms-element-info note))
  (format #t "note info ~s~%"
          (map (lambda (fun)
                 (apply fun (list note)))
               (list
                ms-note-mark         ; bool
                ms-note-velo-offset  ; int
                ms-note-subchannel   ; int
                ms-note-play         ; bool
                ;ms-note-playticks    ; int will SIGSEGV
                ms-note-tpc          ; int
                ms-note-tpc1         ; int
                ms-note-tpc2         ; int
                ms-note-pitch        ; int
                ;ms-note-ppitch       ; int SIGSEGV Ms::Fraction::ticks
                ms-note-epitch       ; int
                ms-note-tuning       ; qreal
                )))
  (for-each (lambda (lst)
              (apply (lambda (getfun setfun val)
                       (format #t "  A: ~a ~s~%" getfun (getfun note))
                       (setfun note val)
                       (format #t "  B: ~a ~s~%" getfun (getfun note)))
                     lst))
   (list (list ms-note-tuning ms-note-tuning! 1.23)
         (list ms-note-subchannel ms-note-subchannel! 1)
         (list ms-note-velo-offset ms-note-velo-offset! 12)
         ;(list ms-note-ontime-offset ms-note-ontime-offset! 10)
         ;(list ms-note-offtime-offset ms-note-offtime-offset! 20)
         (list ms-note-play ms-note-play! #t)
         (list ms-note-pitch ms-note-pitch! 34)
         ))
  )

(format #t "Testing has finished.~%")
