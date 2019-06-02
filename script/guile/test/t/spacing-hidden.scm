; Ensure hidden staffs doesn't effect
; the non-hidden staffs note positioning.

(let ((score (test-read-score-sxlm
 '(*TOP*
   (museScore (@ (version "3.01"))
   (Score
     (LayerTag (@ (tag default) (id 0)))
     (currentLayer 0)
     (Synthesizer)
     (Division 480)
     (Style
       (pageWidth 8.5)
       (pageHeight 11)
       (pagePrintableWidth 7.7126)
       (Spatium 1.76389))
     (showInvisible 1)
     (showUnprintable 1)
     (showFrames 1)
     (showMargins 0)
     (Part
       (Staff (@ (id "1"))
         (StaffType (@ (group pitched))
           (name stdNormal))))
     (Part
       (Staff (@ (id "2"))
         (StaffType (@ (group pitched))
           (name stdNormal)))
       (show 0))
     (Staff (@ (id "1"))
       (Measure
         (voice
           (TimeSig (sigN 4) (sigD 4))
           (Chord
             (durationType quarter)
             (Note (pitch 72) (tpc 14)))
           (Chord
             (durationType quarter)
             (Note (pitch 74) (tpc 16)))
           (Chord
             (durationType quarter)
             (Note (pitch 76) (tpc 18)))
           (Chord
             (durationType quarter)
             (Note (pitch 72) (tpc 14))))))
     (Staff (@ (id "2"))
       (Measure
         (voice
           (TimeSig (sigN 4) (sigD 4))
           (Chord
             (durationType "16th")
             (Note (pitch 72) (tpc 14)))
           (Chord
             (durationType "16th")
             (Note (pitch 71) (tpc 19)))
           (Chord
             (durationType "16th")
             (Note (pitch 72) (tpc 14)))
           (Chord
             (durationType "16th")
             (Note (pitch 74) (tpc 16)))
           (Chord
             (durationType eighth)
             (Note (pitch 71) (tpc 19)))
           (Chord
             (durationType eighth)
             (Note (pitch 67) (tpc 15)))
           (Rest
             (durationType half)))))))))))
  ; collect all x-positions from the unhidden staff (track-0)
  (let ((track-xpos (collect keep
    (walk-score #f #f
      (lambda (elm)
        (if (string= (ms-element-name elm) "Chord")
          (let* ((notes (ms-chord-notes elm))
                 (sz (ms-stdvec-size notes)))
            (if (> sz 0)
              (let* ((note (ms-stdvec-at notes 0))
                     (track (ms-element-track note))
                     (pagepos (ms-element-pagePos note)))
                (if (= track 0)
                  (keep (vector-ref pagepos 0))))))))
      score))))
    ; check the all notes in first track, have a constant
    ; distance to its neighbors x-position.
    (let ((err (apply + (map abs
              (between - (between - track-xpos))))))
      (assert (< err 0.1)
              "Notes in first staff (track 0) are not equally spaced")))
  (ms-score-forget score)) ; C++ delete score
