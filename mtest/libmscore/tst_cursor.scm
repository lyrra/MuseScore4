
(emit '(load "./ms.scm"))

; Add an element (note) and verify the tick updated with the correct amount

(emit '
(let ((score (ms-make-score)))
  #f
  ; test setup
  ;  score = new Score();
  ;  score->appendPart("voice");
  ;  score->appendMeasures(2);
  ;  Cursor c(score);
  ; creation of cursor - check default values
  ;  QCOMPARE(c.score(), score);
  ;  QCOMPARE(c.track(), 0);
  ;  QCOMPARE(c.voice(), 0);
  ;  QCOMPARE(c.tick(), 0);
  ;  QVERIFY(c.segment() == nullptr);
  ; set the cursor at input position (start of score)
  ;  c.rewind(0);
  ;  QVERIFY(c.segment() != nullptr);

  ; actual test
  ; add 4 times a 1/8th, totalling a half note
  ;  c.setDuration(1, 8);
  ;  c.addNote(60);
  ;  c.addNote(60);
  ;  c.addNote(60);
  ;  c.addNote(60);
  ;  QCOMPARE(c.tick(), MScore::division * 2); //one division == 1 crotchet
  ))

