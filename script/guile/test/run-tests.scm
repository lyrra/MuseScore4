; This script is written in a story telling style, and tries
; to make it possible with little background to pick up
; both Scheme language, and MuseScore internals.
; By being progressively more complex and advanced in the test cases.
;
; Function name with prefix ms- are Scheme to C calls.
; Ie, a Scheme procedure that will call a MuseScore C function,
; in most cases via a C shim function that converts C data structure
; to a Scheme type structure.
;
; This script assumes MuseScore executable has been started,
; and has loaded three demos:
; demos/Fugue_1.mscx demos/Unclaimed_Gift.mscx demos/Reunion.mscz

(define *number-of-scores* 3)

; Score-information.
; Staff-info is a bit cryptic. It is a list of:
; (staffIdx startTrack endTrack numInstruments)
(define *score-information*
        '((name "Fugue_1"
                measures 29
                total-segments 505
                total-elements 8080
                staves 4
                staff-info (#(0 0 4 1) #(1 4 8 1)
                            #(2 8 12 1) #(3 12 16 1)))
          (name "Unclaimed_Gift"
                measures 40
                total-segments 230
                total-elements 920
                staves 1
                staff-info (#(0 0 4 1)))
          (name "Reunion"
                measures 23
                total-segments 207
                total-elements 1656
                staves 2
                staff-info (#(0 0 4 1) #(1 4 8 1)))))

; Begin testing by looking at the scheme examples and run them.
; Some examples can be reused in testing, make a list of those:
(define *test-files* '("examples/elements.scm"
                       "examples/scores-nstaves.scm"
                       "examples/scores-staves.scm"
                       "examples/segments.scm"
                       "examples/staff-info.scm"))

; Load an scheme module that makes output print formatting nicer
(use-modules (ice-9 format))

; Try calling some very simple Scheme/C shim functions
(format #t "MuseScore name: ~a~%" (ms-core-name))
(if (not (string? (ms-core-name))) (error "ms-core-name returned non string type"))

(format #t "MuseScore version: ~a~%" (ms-core-version))
(if (not (string? (ms-core-version))) (error "ms-core-version returned non string type"))

; Load all feasible examples

(for-each
  (lambda (file)
    ; Guile changes working directory to MS/build.debug/mscore
    (format #t "Loading example file ~s~%" file)
    (load (string-append "script/guile/" file)))
  *test-files*)

; Run the examples

; MS-SCORES-COUNT should return a non-zero number
; if MuseScore has a score open.
(if (not (= (ms-scores-count) *number-of-scores*))
  (error "ms-scores-count reported wrong number of opened scores."))

; Get some arbitrary information about each staff
(let ((lst (scores-staves-info)))
  (if (not (list? lst)) (error "scores-staves-info returned non-list type"))
  (for-each (lambda (prop infos)
              (let ((infos2 (cadr (memq 'staff-info prop))))
                (format #t "score info: ~s ~s~%" infos infos2)
                (if (not (equal? infos infos2))
                  (error (format #f "scores-staves-info has returned wrong staff-info: ~s, ~s" infos infos2)))))
            *score-information* lst)
  "scores-staves-info test succeeded")

; Print a list of all staves in all opened scores
(let ((lst (scores-staves)))
  (if (not (= (length lst) *number-of-scores*))
    (error "scores-staves returned a list of bad length"))
  (for-each (lambda (prop staves)
              (let ((staves-number (cadr (memq 'staves prop))))
                (if (not (= staves-number (length staves)))
                  (error (format #f "scores-staves returned wrong number of staves: ~a != ~a." staves-number (length staves))))))
            ; call the above lambda with elements from the two lists "zipped"
            *score-information*
            lst)
  "scores-staves test succeded")

(let ((lst (scores-nstaves)))
  (if (not (list? lst)) (error "scores-nstaves returned non-list type"))
  (if (not (= (length lst) *number-of-scores*)) (error "scores-nstaves returned an empty list"))
  (for-each (lambda (prop nstaves)
              (let ((num-staves (cadr (memq 'staves prop))))
                (if (not (number? nstaves))
                  (error "scores-nstaves, nstaves is non-number"))
                (if (not (= nstaves num-staves))
                  (error (format #f "scores-staves returned wrong number of staves: ~a != ~a." num-staves nstaves)))))
            *score-information* lst)
  "scores-nstaves test succeeded")

(let ((lst (scores-segments)))
  (if (not (list? lst)) (error "scores-segments returned non-list type"))
  (for-each (lambda (prop measures)
              (let ((num-measures (cadr (memq 'measures prop)))
                    (total-segments (cadr (memq 'total-segments prop))))
                (format #t "score has num-measures: ~s~%" (length measures))
                (let ((tot-segments 0))
                  (map (lambda (measure)
                         (set! tot-segments (+ tot-segments (length measure))))
                       measures)
                  (if (not (= tot-segments total-segments))
                    (error (format #f "scores-staves returned wrong number of segments (sum of all segments in all measures): ~a != ~a." tot-segments total-segments))))))
            *score-information* lst)
  "scores-segments test succeeded")

(let ((lst (scores-elements)))
  (if (not (list? lst)) (error "scores-elements returned non-list type"))
  (for-each (lambda (prop measures)
              (let ((num-measures (cadr (memq 'measures prop)))
                    (total-elements (cadr (memq 'total-elements prop))))
                (if (not (list? measures))
                  (error "scores-elements, measures list is not a list"))
                (let ((tot-elements 0))
                  (map (lambda (segments)
                         (map (lambda (elements)
                                (set! tot-elements (+ tot-elements (length elements))))
                              segments))
                       measures)
                  (if (not (= tot-elements total-elements))
                    (error (format #f "scores-staves returned wrong number of elements (summary): ~a != ~a." tot-elements total-elements))))))
            *score-information* lst)
  "scores-elements test succeeded")

(let ((lst (scores-staves-info)))
  (if (not (list? lst)) (error "scores-staves-info returned non-list type"))
  (for-each (lambda (prop infos)
              (let ((infos2 (cadr (memq 'staff-info prop))))
                (format #t "score info: ~s ~s~%" infos infos2)
                (if (not (equal? infos infos2))
                  (error (format #f "scores-staves-info has returned wrong staff-info: ~s, ~s" infos infos2)))))
            *score-information* lst)
  "scores-staves-info test succeeded")

; End of examples tests
