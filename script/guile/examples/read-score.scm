
(define-module (convert-mscx)
               #:use-module (ice-9 format)
               #:use-module (ice-9 textual-ports) ; get-string-all
               #:use-module (sxml simple)
               ;#:use-module (lib musescore-c)
               ;#:use-module (lib musescore)
               )

; Open an mscx file, and read all its content (raw XML)

(let ((p (open-input-file (cadr (command-line)))))
  (let ((str (get-string-all p)))

    ; send that XML content to a musescore-reader
    ; that will read it and create an internal
    ; score object
    ;(let ((score (ms-score-read-string str)))
    ;  (format #t "score: ~s~%" score)
    ;  (format #t "  measures: ~s~%" (ms-score-measures score))
    ;  (for-each (lambda (mea)
    ;              (format #t "    measure ~s segments: ~s~%"
    ;                      mea (ms-measure-segments mea)))
    ;            (ms-score-measures score)))

    ; Or take that same raw XML content and
    ; convert it to Guile SXML tags:
    (let ((xml (xml->sxml str)))
      (display xml)
      (newline))))
