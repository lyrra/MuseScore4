;;;; test utilities for testing musescore

;;;; foreign-objects holding c-pointers to various
;;;; musescore objects

(let-syntax
    ((def
      (syntax-rules ()
        ((def name objname)
          (define (name obj)
            (and (struct? obj)
                 (instance? obj)
                 (eq? 'objname (struct-vtable-name (struct-vtable obj)))))))))

  (def staff?   <ms-staff>)
  (def score?   <ms-score>)
  (def element? <ms-element>)
  (def measure? <ms-measure>)
  (def segment? <ms-segment>)
  (def note?    <ms-note>)
  (def selection?  <ms-selection>)
  (def inputstate? <ms-inputstate>))
