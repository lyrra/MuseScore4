
(use-modules (ice-9 textual-ports)) ; get-string-all
(use-modules (sxml simple))

(define (test-sxml-to-xml sxml)
  (with-output-to-string
   (lambda () (sxml->xml sxml))))

(define (test-read-score-sxlm sxml)
  (let ((str (test-sxml-to-xml sxml)))
    (let ((score (ms-score-read-string str)))
      score)))

(define (test-read-score-mscx-file file)
  (let ((fp (open-input-file file)))
    (let ((score (ms-score-read-string
                  (get-string-all fp))))
      (close-port fp)
      score)))

(define (test-print-score score)
  (format #t "--- Score: ---~%")
  (let ((firstseg (ms-score-tick2segment score 0))
        (lastseg #f)
        (oldmea 0))
    (format #t "  first-seg (tick 0): ~s~%" firstseg)
    (do ((seg firstseg (ms-segment-next1 seg)))
        ((not seg))
      (set! lastseg seg)
      (let ((mea (ms-segment-measure seg)))
        (if (not (= oldmea (pointer-address mea)))
          (begin
            (set! oldmea (pointer-address mea))
            (format #t "    mea:~s noOffset:~d no:~d~%" mea
                    (ms-measure-noOffset mea)
                    (ms-measure-no mea))
            (let ((elmvec (ms-measure-elements mea)))
              (do ((i 0 (+ i 1)))
                  ((>= i (vector-length elmvec)))
                (let ((elm (vector-ref elmvec i)))
                  (format #t "        elm: ~s ~s~%" elm (ms-element-name elm)))))))
        (format #t "      seg (tick ~a): ~s~%"
                (ms-element-tick seg)
                seg)
        (let ((elmvec (ms-segment-elements seg)))
          (do ((i 0 (+ i 1)))
              ((>= i (vector-length elmvec)))
            (let ((elm (vector-ref elmvec i)))
              (format #t "        elm: ~s ~s~%" elm (ms-element-name elm)))))))))

(define (walk-score meafun segfun elmfun score)
  (let ((firstseg (ms-score-tick2segment score 0))
        (lastseg #f)
        (oldmea 0))
    (do ((seg firstseg (ms-segment-next1 seg)))
        ((not seg))
      (set! lastseg seg)
      (if segfun (segfun seg))
      (let ((mea (ms-segment-measure seg)))
        (if (not (= oldmea (pointer-address mea)))
          (begin
            (if meafun (meafun mea))
            (set! oldmea (pointer-address mea))
            (if elmfun
              (let ((elmvec (ms-measure-elements mea)))
                (do ((i 0 (+ i 1)))
                    ((>= i (vector-length elmvec)))
                  (let ((elm (vector-ref elmvec i)))
                    (elmfun elm)))))))
        (if elmfun
          (let ((elmvec (ms-segment-elements seg)))
            (do ((i 0 (+ i 1)))
                ((>= i (vector-length elmvec)))
              (let ((elm (vector-ref elmvec i)))
                (elmfun elm)))))))))
