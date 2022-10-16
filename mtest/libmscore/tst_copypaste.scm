
(emit '(load "./ms.scm"))

;#include <QtTest/QtTest>
;#include "mtest/testutils.h"
;#include "libmscore/score.h"
;#include "libmscore/measure.h"
;#include "libmscore/segment.h"
;#include "libmscore/chordrest.h"
;#include "libmscore/chord.h"
;#include "libmscore/xml.h"
;#include "libmscore/durationtype.h"

;#define DIR QString("libmscore/copypaste/")


      ; copypastestaff(const char*);
      ; copypastevoice(const char*, int);
      ; copypastetuplet(const char*);
      ; copypastetremolo();
      ; copypastenote(const QString&, Fraction = Fraction(1, 1));
#|

      (def (copyPaste2Voice2) (copypastevoice "14" 0))
      (def (copyPaste2Voice3) (copypastevoice "15" 1))
      (def (copyPaste2Voice4) (copypastevoice "16" 1)) ; shorten last cr
      (def (copyPaste2Voice5))                         ; cut and move
      (def (copypaste2Voice6))
      (def (copyPasteOnlySecondVoice))
;      (def copyPasteShortTremolo() { copypastevoice("21", 1); } // remove tremolo on shorten note #30411

;      void copypastestaff50() { copypastestaff("50"); }       // staff & slurs

;      void copyPastePartial();

;;      void copyPasteTuplet01() { copypastetuplet("01"); }
;      void copyPasteTuplet02() { copypastetuplet("02"); }
;      void copypasteQtrNoteOntoWholeRest() { copypastenote("01"); }
;      void copypasteQtrNoteOntoWholeNote() { copypastenote("02"); }
;      void copypasteQtrNoteOntoQtrRest() { copypastenote("03"); }
;      void copypasteQtrNoteOntoQtrNote() { copypastenote("04"); }
;      void copypasteWholeNoteOntoQtrNote() { copypastenote("05"); }
;      void copypasteWholeNoteOntoQtrRest() { copypastenote("06"); }
;      void copypasteQtrNoteOntoTriplet() { copypastenote("07"); }
;      void copypasteWholeNoteOntoTriplet() { copypastenote("08"); }
;      void copypasteQtrNoteIntoChord() { copypastenote("09"); }
;      void copypasteQtrNoteOntoMMRest() { copypastenote("10"); }
;      void copypasteQtrNoteDoubleDuration() { copypastenote("11", Fraction(2, 1)); }
;      void copyPasteTremolo01() { copypastetremolo(); }
;      };

|#

;  copypaste
;  copy measure 2, paste into measure 4
(emit '
(let ((filenums '(
      "01"          ;  start slur
      "02"          ;  end slur
      "03"          ;  slur
      "04"          ;  start tie
      "05"          ;  end tie
      "06"          ;  tie
      ; "07"           ;  start ottava
      "08"          ;  end ottava
      "09"          ;  ottava
      "10"          ;  two slurs
      "11"          ;  grace notes
      "12"            ;  voices
      "19"       ; chord symbols
      "22"           ; cross-staff slur
      "23"           ; full measure tuplet 10/8
      "24"           ; more complex non reduced tuplet
      "25"           ; copy full measure rest
      "26"           ; Copy chords (#298541)
      )))
  (format #t "  -- running copy-paste 01-26~%")

  (for-each
   (lambda (file)
     (let ((score (ms-mtest-readScore (format #f "libmscore/copypaste/copypaste~a.mscx" file))))
       (let* ((m1 (ms-score-firstMeasure score))
              (m2 (ms-measure-nextMeasure m1))
              (m3 (ms-measure-nextMeasure m2))
              (m4 (ms-measure-nextMeasure m3)))
         (ms-test-check (and m1 m2 m3 m4))
         (ms-score-select score m2) ; score->select(m2);
         ; if (score->nstaves() > 1)
         ;       score->select(m2, SelectType::RANGE, score->nstaves() - 1);
         (if (> (ms-score-nstaves score) 1)
             (ms-score-select m2 'SelectType-RANGE (- (ms-score-nstaves score) 1)))
         (let ((sel (ms-score-selection score)))
           ; QVERIFY(score->selection().canCopy());
           (ms-test-check (ms-selection-canCopy sel))
           ; QString mimeType = score->selection().mimeType();
           ; QVERIFY(!mimeType.isEmpty());
           (format #t "mimetype: ~s~%" (ms-selection-mimeType sel))
           ; QMimeData* mimeData = new QMimeData;
           ; QByteArray ba = score->selection().mimeData();
           ; mimeData->setData(mimeType, ba);
           ; QApplication::clipboard()->setMimeData(mimeData);
           ; QVERIFY(m4->first()->element(0) != 0);
           )
         ; score->select(m4->first()->element(0));
         ; score->startCmd();
         ; score->cmdPaste(mimeData,0);
         ; score->endCmd();

         ; QVERIFY(saveCompareScore(score, QString("copypaste%1.mscx").arg(idx),
         ;    DIR + QString("copypaste%1-ref.mscx").arg(idx)));
         ; delete score;

         ))) filenums)))

#|
//---------------------------------------------------------
//   copypaste
//    copy measure 2 from first staff, paste into staff 2
//---------------------------------------------------------

void TestCopyPaste::copypastestaff(const char* idx)
      {
      MasterScore* score = readScore(DIR + QString("copypaste%1.mscx").arg(idx));
      Measure* m1 = score->firstMeasure();
      Measure* m2 = m1->nextMeasure();    // src

      QVERIFY(m1 != 0);
      QVERIFY(m2 != 0);

      score->select(m2, SelectType::RANGE, 0);
      QVERIFY(score->selection().canCopy());
      QString mimeType = score->selection().mimeType();
      QVERIFY(!mimeType.isEmpty());
      QMimeData* mimeData = new QMimeData;
      mimeData->setData(mimeType, score->selection().mimeData());
      QApplication::clipboard()->setMimeData(mimeData);

      score->deselectAll();

      score->select(m2, SelectType::RANGE, 1);

      score->startCmd();
      score->cmdPaste(mimeData,0);
      score->endCmd();

      QVERIFY(saveCompareScore(score, QString("copypaste%1.mscx").arg(idx),
         DIR + QString("copypaste%1-ref.mscx").arg(idx)));
      delete score;
      }

void TestCopyPaste::copyPastePartial()
      {
      MasterScore* score = readScore(DIR + QString("copypaste_partial_01.mscx"));

      Measure* m1 = score->firstMeasure();

      Segment* s = m1->first(SegmentType::ChordRest);
      s = s->next(SegmentType::ChordRest);
      score->select(s->element(0));
      s = s->next(SegmentType::ChordRest);
      score->select(s->element(4), SelectType::RANGE);

      QVERIFY(score->selection().canCopy());
      QString mimeType = score->selection().mimeType();
      QVERIFY(!mimeType.isEmpty());
      QMimeData* mimeData = new QMimeData;
      mimeData->setData(mimeType, score->selection().mimeData());
      QApplication::clipboard()->setMimeData(mimeData);

      score->select(m1->first(SegmentType::ChordRest)->element(0));

      score->startCmd();
      score->cmdPaste(mimeData,0);
      score->endCmd();

      QVERIFY(saveCompareScore(score, QString("copypaste_partial_01.mscx"),
         DIR + QString("copypaste_partial_01-ref.mscx")));
      delete score;
      }

void TestCopyPaste::copyPaste2Voice()
      {
      MasterScore* score = readScore(DIR + QString("copypaste13.mscx"));
      Measure* m1 = score->firstMeasure();
      Measure* m2 = m1->nextMeasure();

      QVERIFY(m1 != 0);
      QVERIFY(m2 != 0);

      // select 2 chord rests at the start of the first measure
      Segment* s = m1->first(SegmentType::ChordRest);
      score->select(static_cast<Ms::Chord*>(s->element(0))->notes().at(0));
      s = s->next(SegmentType::ChordRest);
      score->select(s->element(0), SelectType::RANGE);

      QVERIFY(score->selection().canCopy());
      QString mimeType = score->selection().mimeType();
      QVERIFY(!mimeType.isEmpty());
      QMimeData* mimeData = new QMimeData;
      mimeData->setData(mimeType, score->selection().mimeData());
      QApplication::clipboard()->setMimeData(mimeData);

      // paste into the second CR of second measure
      Segment* secondCRSeg = m2->first()->next1(SegmentType::ChordRest);
      score->select(secondCRSeg->element(0));

      score->startCmd();
      score->cmdPaste(mimeData,0);
      score->endCmd();

      QVERIFY(saveCompareScore(score, QString("copypaste13.mscx"),
         DIR + QString("copypaste13-ref.mscx")));
      delete score;
      }

//---------------------------------------------------------
//   copypaste
//    copy measure 2 from first staff, paste into staff 2
//---------------------------------------------------------

void TestCopyPaste::copypastevoice(const char* idx, int voice)
      {
      MasterScore* score = readScore(DIR + QString("copypaste%1.mscx").arg(idx));
      Measure* m1 = score->firstMeasure();
      Measure* m2 = m1->nextMeasure();

      QVERIFY(m1 != 0);
      QVERIFY(m2 != 0);

      // create a range selection on 2 and 3 beat of first measure
      SegmentType segTypeCR = SegmentType::ChordRest;
      Segment* s = m1->first(segTypeCR)->next1(segTypeCR);
      score->select(static_cast<Ms::Chord*>(s->element(voice))->notes().at(0));
      s = s->next(SegmentType::ChordRest);
      score->select(s->element(voice), SelectType::RANGE);

      QVERIFY(score->selection().canCopy());
      QString mimeType = score->selection().mimeType();
      QVERIFY(!mimeType.isEmpty());
      QMimeData* mimeData = new QMimeData;
      mimeData->setData(mimeType, score->selection().mimeData());
      QApplication::clipboard()->setMimeData(mimeData);

      //paste to second measure
      score->select(m2->first()->element(0));

      score->startCmd();
      score->cmdPaste(mimeData,0);
      score->endCmd();

      QVERIFY(saveCompareScore(score, QString("copypaste%1.mscx").arg(idx),
         DIR + QString("copypaste%1-ref.mscx").arg(idx)));
      delete score;
      }

void TestCopyPaste::copyPaste2Voice5()
      {
      MasterScore* score = readScore(DIR + QString("copypaste17.mscx"));
      Measure* m1 = score->firstMeasure();

      QVERIFY(m1 != 0);

      // create a range selection from 2 eighth note to the end of first measure
      SegmentType segTypeCR = SegmentType::ChordRest;
      Segment* s = m1->first(segTypeCR)->next1(segTypeCR);
      score->select(static_cast<Ms::Chord*>(s->element(0))->notes().at(0));

      s = m1->last()->prev(SegmentType::ChordRest);
      score->select(s->element(0), SelectType::RANGE);

      QVERIFY(score->selection().canCopy());
      QString mimeType = score->selection().mimeType();
      QVERIFY(!mimeType.isEmpty());
      QMimeData* mimeData = new QMimeData;
      mimeData->setData(mimeType, score->selection().mimeData());
      QApplication::clipboard()->setMimeData(mimeData);

      score->cmdDeleteSelection(); //cut

      //paste to quarter rest
      Element* dest = m1->first()->next(segTypeCR)->next(segTypeCR)->next(segTypeCR)->element(0);
      QVERIFY(dest->isRest() && static_cast<ChordRest*>(dest)->durationType() == TDuration::DurationType::V_QUARTER);
      score->select(dest);

      score->startCmd();
      score->cmdPaste(mimeData,0);
      score->endCmd();

      QVERIFY(saveCompareScore(score, QString("copypaste17.mscx"),
         DIR + QString("copypaste17-ref.mscx")));
      delete score;
      }


void TestCopyPaste::copyPasteOnlySecondVoice()
      {
      MasterScore* score = readScore(DIR + QString("copypaste18.mscx"));
      Measure* m1 = score->firstMeasure();
      Measure* m2 = m1->nextMeasure();

      QVERIFY(m1 != 0);
      QVERIFY(m2 != 0);

      score->select(m1, SelectType::RANGE, 0);

      score->selectionFilter().setFiltered(SelectionFilterType::FIRST_VOICE,false);

      QVERIFY(score->selection().canCopy());
      QString mimeType = score->selection().mimeType();
      QVERIFY(!mimeType.isEmpty());
      QMimeData* mimeData = new QMimeData;
      mimeData->setData(mimeType, score->selection().mimeData());
      QApplication::clipboard()->setMimeData(mimeData);


      //paste to second measure
      score->deselectAll();
      score->selectionFilter().setFiltered(SelectionFilterType::FIRST_VOICE,true);
      score->select(m2,SelectType::RANGE);

      score->startCmd();
      score->cmdPaste(mimeData,0);
      score->endCmd();

      QVERIFY(saveCompareScore(score, QString("copypaste18.mscx"),
         DIR + QString("copypaste18-ref.mscx")));
      delete score;
      }

void TestCopyPaste::copypaste2Voice6()
      {
      MasterScore* score = readScore(DIR + QString("copypaste20.mscx"));
      Measure* m1 = score->firstMeasure();

      QVERIFY(m1 != 0);

      // create a range selection from 2nd eighth note to the end of first measure
      SegmentType segTypeCR = SegmentType::ChordRest;
      Segment* s = m1->first(segTypeCR)->next1(segTypeCR);
      score->select(static_cast<Ms::Chord*>(s->element(0))->notes().at(0));

      s = m1->last()->prev(SegmentType::ChordRest);
      score->select(s->element(1), SelectType::RANGE);

      QVERIFY(score->selection().canCopy());
      QString mimeType = score->selection().mimeType();
      QVERIFY(!mimeType.isEmpty());
      QMimeData* mimeData = new QMimeData;
      mimeData->setData(mimeType, score->selection().mimeData());
      QApplication::clipboard()->setMimeData(mimeData);

      //paste to 16th rest
      Element* dest = m1->first(segTypeCR)->next(segTypeCR)->next(segTypeCR)->next(segTypeCR)->next(segTypeCR)->element(0);
      qDebug() << int(dest->type());
      QVERIFY(dest->isRest()
              && static_cast<ChordRest*>(dest)->durationType() == TDuration::DurationType::V_16TH);
      score->select(dest);

      score->startCmd();
      score->cmdPaste(mimeData,0);
      score->endCmd();

      QVERIFY(saveCompareScore(score, QString("copypaste20.mscx"),
         DIR + QString("copypaste20-ref.mscx")));
      delete score;
      }

//---------------------------------------------------------
//   copypastetuplet
//---------------------------------------------------------

void TestCopyPaste::copypastetuplet(const char* idx)
      {
      MasterScore* score = readScore(DIR + QString("copypaste_tuplet_%1.mscx").arg(idx));

      Measure* m1 = score->firstMeasure();
      Measure* m2 = m1->nextMeasure();

      Segment* s = m1->first(SegmentType::ChordRest);
      score->select(toChord(s->element(0))->notes().at(0));
      s = s->next(SegmentType::ChordRest);
      score->select(s->element(0), SelectType::RANGE);

      QVERIFY(score->selection().canCopy());
      QString mimeType = score->selection().mimeType();
      QVERIFY(!mimeType.isEmpty());
      QMimeData* mimeData = new QMimeData;
      mimeData->setData(mimeType, score->selection().mimeData());
      QApplication::clipboard()->setMimeData(mimeData);

      Element* dest = m2->first(SegmentType::ChordRest)->element(0);
      score->select(dest);
      score->startCmd();
      score->cmdPaste(mimeData,0);
      score->endCmd();

      QVERIFY(saveCompareScore(score, QString("copypaste_tuplet_%1.mscx").arg(idx),
         DIR + QString("copypaste_tuplet_%1-ref.mscx").arg(idx)));
      delete score;
      }

//---------------------------------------------------------
//   copypastetremolo
//   copy-paste of tremolo between two notes
//---------------------------------------------------------

void TestCopyPaste::copypastetremolo()
      {
      MasterScore* score = readScore(DIR + QString("copypaste_tremolo.mscx"));
      Measure* m1 = score->firstMeasure();
      Measure* m2 = m1->nextMeasure();
      Measure* m3 = m2->nextMeasure();

      QVERIFY(m1 != 0);
      QVERIFY(m2 != 0);
      QVERIFY(m3 != 0);

      // create a range selection on 2nd to 3rd beat (voice 1) of first measure
      SegmentType segTypeCR = SegmentType::ChordRest;
      Segment* s = m1->first(segTypeCR)->next1(segTypeCR);
      score->select(static_cast<Ms::Chord*>(s->element(1))->notes().at(0));
      s = s->next1(segTypeCR);
      score->select(s->element(1), SelectType::RANGE);

      QVERIFY(score->selection().canCopy());
      QString mimeType = score->selection().mimeType();
      QVERIFY(!mimeType.isEmpty());
      QMimeData* mimeData = new QMimeData;
      mimeData->setData(mimeType, score->selection().mimeData());
      QApplication::clipboard()->setMimeData(mimeData);

      //paste to second measure
      score->select(m2->first()->element(0));

      score->startCmd();
      score->cmdPaste(mimeData,0);
      score->endCmd();

      // create a range selection on 2nd to 4th beat (voice 0) of first measure
      s = m1->first(segTypeCR)->next1(segTypeCR);
      score->select(static_cast<Ms::Chord*>(s->element(0))->notes().at(0));
      s = s->next1(segTypeCR)->next1(segTypeCR);
      score->select(s->element(0), SelectType::RANGE);

      QVERIFY(score->selection().canCopy());
      mimeType = score->selection().mimeType();
      QVERIFY(!mimeType.isEmpty());
      mimeData->setData(mimeType, score->selection().mimeData());
      QApplication::clipboard()->setMimeData(mimeData);

      //paste to third measure
      score->select(m3->first()->element(0));

      score->startCmd();
      score->cmdPaste(mimeData,0);
      score->endCmd();

      QVERIFY(saveCompareScore(score, QString("copypaste_tremolo.mscx"),
         DIR + QString("copypaste_tremolo-ref.mscx")));
      delete score;
      }

void TestCopyPaste::copypastenote(const QString& idx, Fraction scale)
      {
      score = readScore(DIR + "copypasteNote" + idx + ".mscx");
      Measure* m1 = score->firstMeasure();
      Measure* m2 = m1->nextMeasure();
      Segment* s = m2->first(SegmentType::ChordRest);
      score->select(toChord(s->element(0))->notes().at(0));
      QMimeData mimeData;
      mimeData.setData(score->selection().mimeType(), score->selection().mimeData());
      ChordRest* cr = m1->first(SegmentType::ChordRest)->nextChordRest(0);
      score->select(cr->isChord() ? toChord(cr)->upNote() : static_cast<Element*>(cr));
      score->startCmd();
      score->cmdPaste(&mimeData, 0, scale);
      score->endCmd();
      QVERIFY(saveCompareScore(score, "copypasteNote" + idx + ".mscx", DIR + "copypasteNote" + idx + "-ref.mscx"));
      }

QTEST_MAIN(TestCopyPaste)
#include "tst_copypaste.moc"

|#
