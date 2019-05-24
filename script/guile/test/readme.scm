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
; (staffIdx startTrack endTrack numInstruments partname)
(define *score-information*
        '((name "Fugue_1"
                measures 29
                total-segments 505
                total-elements 1217
                staves 4
                staff-info (#(0  0  4 1 "Violin I"   )
                            #(1  4  8 1 "Violin II"  )
                            #(2  8 12 1 "Viola"      )
                            #(3 12 16 1 "Violoncello")))
          (name "Unclaimed_Gift"
                measures 40
                total-segments 231
                total-elements 226
                staves 1
                staff-info (#(0 0 4 1 "Piano")))
          (name "Reunion"
                measures 23
                total-segments 207
                total-elements 360
                staves 2
                staff-info (#(0 0 4 1 "Piano")
                            #(1 4 8 1 "Piano")))))

; Begin testing by looking at the scheme examples and run them.
; Some examples can be reused in testing, make a list of those:
(define *test-files* '("examples/elements.scm"
                       "examples/scores-nstaves.scm"
                       "examples/scores-staves.scm"
                       "examples/segments.scm"
                       "examples/staff-info.scm"))

; Load an scheme module that makes output print formatting nicer
(use-modules (ice-9 format))

; define a simple assert macro that lets out quit when
; an error has occured. Check that the unix-return-code is 0.
(define-syntax-rule (assert expr msg ...)
  (if (not expr)
    (begin
      (format #t "ERROR: ")
      (format #t msg ...)
      (format #t "~%")
      (error msg ...))))

; Try calling some very simple Scheme/C shim functions
(format #t "MuseScore name: ~a~%" (ms-name))
(assert (string? (ms-name)) "ms-name returned non string type")

(format #t "MuseScore version: ~a~%" (ms-version))
(assert (string? (ms-version)) "ms-version returned non string type")

; Note ms-version could contain things like "-beta" ?
(assert (ms-version-check (ms-version)) "ms-version self-version fail")

(let ((ver (ms-version))
      (maj (ms-version-major))
      (min (ms-version-minor))
      (upd (ms-version-update)))
  (assert (number? maj) "major-version is not a number")
  (assert (number? min) "minor-version is not a number")
  (assert (number? upd) "update-version is not a number"))

; Load all feasible examples

(for-each
  (lambda (file)
    (format #t "Loading example file ~s~%" file)
    ; primitive-load does not use this scripts load path as base,
    ; which enables loading this script through guile, and by STDIN.
    (primitive-load (string-append "script/guile/" file)))
  *test-files*)

; Run the examples

; MS-SCORES-COUNT should return a non-zero number
; if MuseScore has a score open.
(assert (= (ms-scores-count) *number-of-scores*)
  "ms-scores-count reported wrong number of opened scores.")

; Get some arbitrary information about each staff
(let ((lst (scores-staves-info)))
  (assert (list? lst) "scores-staves-info returned non-list type")
  (for-each (lambda (prop infos)
              (let ((infos2 (cadr (memq 'staff-info prop))))
                (format #t "score info: ~s ~s~%" infos infos2)
                (assert (equal? infos infos2)
                  "scores-staves-info has returned wrong staff-info: ~s, ~s" infos infos2)))
            *score-information* lst)
  "scores-staves-info test succeeded")

; Print a list of all staves in all opened scores
(let ((lst (scores-staves)))
  (assert (= (length lst) *number-of-scores*)
    "scores-staves returned a list of bad length")
  (for-each (lambda (prop staves)
              (let ((staves-number (cadr (memq 'staves prop))))
                (assert (= staves-number (length staves))
                  "scores-staves returned wrong number of staves: ~a != ~a." staves-number (length staves))))
            ; call the above lambda with elements from the two lists "zipped"
            *score-information*
            lst)
  "scores-staves test succeded")

(let ((lst (scores-nstaves)))
  (assert (list? lst) "scores-nstaves returned non-list type")
  (assert (= (length lst) *number-of-scores*) "scores-nstaves returned an empty list")
  (for-each (lambda (prop nstaves)
              (let ((num-staves (cadr (memq 'staves prop))))
                (assert (number? nstaves)
                  "scores-nstaves, nstaves is non-number")
                (assert (= nstaves num-staves)
                  "scores-staves returned wrong number of staves: ~a != ~a." num-staves nstaves)))
            *score-information* lst)
  "scores-nstaves test succeeded")

(let ((lst (scores-segments)))
  (assert (list? lst) "scores-segments returned non-list type")
  (for-each (lambda (prop measures)
              (let ((name (cadr (memq 'name prop)))
                    (num-measures (cadr (memq 'measures prop)))
                    (total-segments (cadr (memq 'total-segments prop))))
                (format #t "score has num-measures: ~s~%" (length measures))
                (let ((tot-segments 0))
                  (map (lambda (measure)
                         (set! tot-segments (+ tot-segments (length measure))))
                       measures)
                  (assert (= tot-segments total-segments)
                    "in score ~a, scores-segments returned wrong number of segments (sum of all segments in all measures): ~a != ~a." name tot-segments total-segments))))
            *score-information* lst)
  "scores-segments test succeeded")

(let ((lst (scores-elements)))
  (assert (list? lst) "scores-elements returned non-list type")
  (for-each (lambda (prop measures)
              (let ((num-measures (cadr (memq 'measures prop)))
                    (name (cadr (memq 'name prop)))
                    (total-elements (cadr (memq 'total-elements prop))))
                (assert (list? measures)
                  "in score ~a, scores-elements, measures list is not a list" name)
                (let ((tot-elements 0))
                  (map (lambda (segments)
                         (map (lambda (elements)
                                (set! tot-elements (+ tot-elements (length elements))))
                              segments))
                       measures)
                  (assert (= tot-elements total-elements)
                    "in score ~a, scores-staves returned wrong number of elements (summary): ~a != ~a." name tot-elements total-elements))))
            *score-information* lst)
  "scores-elements test succeeded")

(let ((lst (scores-staves-info)))
  (assert (list? lst) "scores-staves-info returned non-list type")
  (for-each (lambda (prop infos)
              (let ((infos2 (cadr (memq 'staff-info prop))))
                (format #t "score info: ~s ~s~%" infos infos2)
                (assert (equal? infos infos2)
                  "scores-staves-info has returned wrong staff-info: ~s, ~s" infos infos2)))
            *score-information* lst)
  "scores-staves-info test succeeded")

; End of examples tests
;;; Define some helper functions to remove boilerplate code
;;;; from testing code, also exit the script if we get
;;;; an error.

; flatten the tree of elements into a list
(define (elements-list)
  (let ((lst (scores-elements)))
    ; scores-elements returns a tree:
    ; lst :: [measures [segments [elements ...]]]
    ; by flattening the list (each flatten works on the most outer lists), we finally get a single list (no tree structure)
    ; we begin with an outer-list of scores, each score is a list of measures
    (set! lst (apply append lst)) ; :: list of all measures in scores
    (set! lst (apply append lst)) ; :: list of all segments
    (set! lst (apply append lst)) ; :: list of all elements
    lst))

(let ((elements (elements-list)))
  (for-each (lambda (element)
              (let ((type (ms-element-type element))
                    (info (ms-element-info element)))
                (assert (number? type) "element-type is not a number")
                (assert (or (vector? info)
                            (length info) 3) "bad element-info: ~s" info)))
            elements)
  "element-type/info test succeeded")
