;;;; Guile/Scheme Musescore Library

(use-modules (oop goops))      ; CLOS-like object orientation
(use-modules (system foreign)) ; FFI access

; goops define-method needs the class-object
; available as a function

(define <ms-score>)
(define <ms-staff>)
(define <ms-measure>)
(define <ms-segment>)
(define <ms-element>)
(define <ms-note>)
(define <ms-selection>)
(define <ms-inputstate>)

(let-syntax
  ((def
    (syntax-rules ()
      ((def class-name c-name)
         (let* ((c-name-ptr (dynamic-pointer c-name (dynamic-link)))
                (class-obj (pointer->scm (dereference-pointer c-name-ptr))))
           (set! class-name class-obj))))))
  (def <ms-score>      "ms_obj_score_type")
  (def <ms-staff>      "ms_obj_staff_type")
  (def <ms-measure>    "ms_obj_measure_type")
  (def <ms-segment>    "ms_obj_segment_type")
  (def <ms-element>    "ms_obj_element_type")
  (def <ms-note>       "ms_obj_note_type")
  (def <ms-selection>  "ms_obj_selection_type")
  (def <ms-inputstate> "ms_obj_inputstate_type"))

; By default scheme will print the scheme-address
; to the scheme-object, and that will change for
; every instance. by also printing  the underlying
; c-object we provide more useful information.
;
; For example, a scheme object pointing to a musescore-measure
; will have some arbitrary pointer that is the
; address to the scheme-instance. Whereas the
; pointer to the measure object in C is useful.
;
; Define a print-method that is used when we print
; any musescore objects (wrapped in scheme),
; we want to print the pointer value to the c-objects

; Prints: #<ms-score C-ADDRESS, SCHEME-ADDRESS>

(let-syntax
  ((def
    (syntax-rules ()
      ((def class-name str-name)
        (define-method (write (obj class-name) port)
          (format port "#<~a ~x>"
                  str-name
                  (struct-ref/unboxed obj 0)
                  ;(pointer-address (scm->pointer obj))
                  ))))))
  (def <ms-score>      "ms-score")
  (def <ms-staff>      "ms-staff")
  (def <ms-measure>    "ms-measure")
  (def <ms-segment>    "ms-segment")
  (def <ms-element>    "ms-element")
  (def <ms-note>       "ms-note")
  (def <ms-selection>  "ms-selection")
  (def <ms-inputstate> "ms-inputstate"))
