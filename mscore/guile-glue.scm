
(eval-when (expand load eval)
  (load-from-path "lib/common.scm")
  (use-modules (ice-9 match))
  (use-modules (ice-9 format))
  (load-from-path "build.scm"))

(define obj-types
        '("accidental"
          "bracket"
          "element"
          "inputstate"
          "measure"
          "note"
          "part"
          "score"  "segment"  "selection"  "staff" "system"))

(f "/*
 * This file contains functions that is
 * called from scheme.
 *
 */~%")

(map (lambda (file)
       (f "#include ~a~%" file))
     '("<iostream>"
       "<libguile.h>"
       "\"libmscore/mscore.h\"" "\"libmscore/part.h\""
       "\"libmscore/system.h\""
       "\"libmscore/staff.h\"" "\"libmscore/measurebase.h\""
       "\"libmscore/measure.h\"" "\"libmscore/chord.h\""
       "\"libmscore/segment.h\"" "\"libmscore/segmentlist.h\""
       "\"libmscore/element.h\""
       "\"libmscore/figuredbass.h\""
       "\"libmscore/utils.h\""
       "\"musescore.h\""
       "\"scoreview.h\"" "\"scoretab.h\""
       "\"guile.h\"" "\"guile-glue.h\""))

(f "// Keep this outside any c++ namespace because we want FFI access~%")
(for-each
  (lambda (obj-type)
    (f "SCM ms_obj_~a_type;~%" obj-type))
  obj-types)

(f "

namespace ScriptGuile {
using namespace Ms;
//
// musescore types
//

SCM
init_ms_object_1 (const char *type_name, const char *slotname1)
{
      SCM name, slo;
      scm_t_struct_finalize finalizer = NULL;
      name = scm_from_utf8_symbol (type_name);
      slo = scm_list_1 (scm_from_utf8_symbol (slotname1));
      return scm_make_foreign_object_type (name, slo, finalizer);
      }

//
// core musescore function
//

")

(let-syntax
  ((emit
    (syntax-rules ()
      ((emit name fun)
       (scm/c-fun name () '()
         (f "QString s = QCoreApplication::~a();
         QByteArray ba = s.toLocal8Bit();
         const char *cs = ba.data();
         return scm_false_or_string(cs);~%" fun))))))
  (emit "ms-name"    "applicationName")
  (emit "ms-version" "applicationVersion"))

(scm/c-fun "ms-version-major" () '()
  (f " return scm_from_int(majorVersion());~%"))

(scm/c-fun "ms-version-minor" () '()
  (f " return scm_from_int(minorVersion());~%"))

(scm/c-fun "ms-version-update" () '()
  (f " return scm_from_int(updateVersion());~%"))

; Returns true if running musescore version is
; higher or equal than given version argument.

(scm/c-fun "ms-version-check" ("SCM version") '()
  (f "const char *verstr = scm_to_locale_string(version);
      QString ver = QString::fromUtf8(verstr);
      bool c = compareVersion(QCoreApplication::applicationVersion(), ver);
      return (c ? SCM_BOOL_F : SCM_BOOL_T);~%"))

(scm/c-fun "ms-experimental" () '()
  (f "bool ee = enableExperimental;
      return scm_from_bool(ee);~%"))

(f "//
// musescore musical globals
//~%")

(scm/c-fun "ms-panplayback" () '()
  (indent 6) (f "bool ee = MScore::panPlayback;
      return scm_from_bool(ee);~%"))

(scm/c-fun "ms-playrepeats" () '()
  (indent 6) (f "bool ee = MScore::playRepeats;
      return scm_from_bool(ee);~%"))

(f "//
// MasterScores
//~%")

(scm/c-fun "ms-score-read-string" ("SCM scmstr")
  '("creates a score from string (XML content)")
  (f "char *s = scm_to_locale_string(scmstr);
      MasterScore* score = readScoreCString(s);
      if (! score) { return SCM_BOOL_F; }
      return scm_make_foreign_object_1 ((SCM)ms_obj_score_type, (SCM) score);~%"))

(scm/c-fun "ms-score-forget" ("SCM score_obj")
  '("C++ delete a score (probably unsafe)")
  (var-transfer-expand 6 "score_obj" "ms"
   '(("void*" scm-ref c) ("MasterScore*")))
  (f "scm_foreign_object_set_x(score_obj, 0, nullptr);
      delete ms;
      return SCM_BOOL_T;~%"))

(scm/c-fun "ms-scores-count" () '()
  (f "int c = mscore->currentScoreTab()->count();
      return scm_from_int (c);~%"))

(scm/c-fun "ms-score-spatium" ("SCM score_obj")
  '("get spatium")
  (var-transfer-expand 6 "score_obj" "scr"
   '(("void*" scm-ref c) ("Score*")))
  (f "qreal s = scr->spatium();
      return scm_from_double(s);~%"))

(scm/c-fun "ms-score-systems" ("SCM score_obj")
  '("get score systems list")
  (var-transfer-expand 6 "score_obj" "scr"
   '(("void*" scm-ref c) ("Score*")))
  (c-make-scheme-list 6 "ms_obj_system_type"
    "for (auto &item : scr->systems()) {~%"))

(scm/c-fun "ms-score-inputstate" ("SCM score_obj")
  '("returns inputstate object from score")
  (f "void* obj = scm_foreign_object_ref(score_obj, 0);
      Score *s = (Score *) obj;
      InputState *is = &(s->inputState());
      return scm_make_foreign_object_1 ((SCM)ms_obj_inputstate_type, (SCM) is);~%"))


(scm/c-fun "ms-scoreview-cmd" ("SCM str") '()
 (f "char *cmd = scm_to_locale_string(str);
     // need to check mscore->currentScore() != NULL ?
     ScoreView* cv = mscore->currentScoreView();
     if (! cv) { return SCM_BOOL_F; }
     cv->cmd(cmd);
     return SCM_BOOL_T;~%"))

(scm/c-fun "ms-current-score" () '()
  (f "Score* score = mscore->currentScore();
      return scm_make_foreign_object_1 ((SCM)ms_obj_score_type, (SCM) score);~%"))

; const InstrumentList* il = part->instruments();
; part %s <- qPrintable(part->partName())
(scm/c-fun "ms-score-parts" ("SCM score_obj") '()
  (var-transfer-expand 6 "score_obj" "score"
   '(("void*" scm-ref c) ("Score*")))
  (f "if (score->parts().isEmpty()) {
          return SCM_EOL; // return an empty list
      }~%")
  (c-make-scheme-list 6 "ms_obj_part_type"
    "QList<Part*>& parts = score->parts();
     for (auto &item : parts) {~%
     "))

(scm/c-fun "ms-part-instruments" ("SCM part_obj") '()
  (var-transfer-expand 6 "part_obj" "part"
   '(("void*" scm-ref c) ("Part*")))
  (f "const InstrumentList* il = part->instruments();
      if (il) return SCM_EOL;
      // il :: a std:map of class Instrument*
      //for(auto inst = il->begin(); inst != il->end(); inst++) {
          // inst :: (Pair idx (class Instrument))
          //QString iid = inst->second->instrumentId();
          //QString trn = inst->second->trackName();
          //SCM data = scm_from_locale_string(iid.toLocal8Bit().data());
          // FIX: flesh-out, need instrument-object
      //}
      return SCM_EOL;~%"))

(scm/c-fun "ms-system-brackets" ("SCM system_obj")
  '("get brackets list")
  (var-transfer-expand 6 "system_obj" "sys"
   '(("void*" scm-ref c) ("System*")))
  (c-make-scheme-list 6 "ms_obj_bracket_type"
    "for (auto &item : sys->brackets()) {~%"))

(scm/c-fun "ms-system-measures" ("SCM system_obj")
  '("get measures list")
  (var-transfer-expand 6 "system_obj" "sys"
   '(("void*" scm-ref c) ("System*")))
  (c-make-scheme-list 6 "ms_obj_measure_type"
    "for (auto &item : sys->measures()) {~%"))

(scm/c-fun "ms-system-measure" ("SCM system_obj" "SCM idx_obj")
  '("get measure from list")
  (var-transfer-expand 6 "system_obj" "sys"
   '(("void*" scm-ref c) ("System*")))
  (f "int n = scm_to_int(idx_obj);
      return scm_make_foreign_object_1 ((SCM)ms_obj_measure_type, (SCM) sys->measure(n));~%"))

(let-syntax
  ((def
    (syntax-rules ()
      ((def name meth descr)
       (scm/c-fun name ("SCM part_obj") descr
         (var-transfer-expand 6 "part_obj" "n"
          '(("void*" scm-ref c) ("Part*")
            ("int" m meth c)))
         (f "return scm_from_int(n);~%"))))))
  (def "ms-part-nstaves"     "nstaves()"     '("number of staves in part"))
  (def "ms-part-starttrack"  "startTrack()"  '(""))
  (def "ms-part-endtrack"    "endTrack()"    '(""))
  (def "ms-part-midiprogram" "midiProgram()" '(""))
  (def "ms-part-midichannel" "midiChannel()" '(""))
  (def "ms-part-midiport"    "midiPort()"    '(""))
  (def "ms-part-color"       "color()"       '(""))
  (def "ms-part-lyricCount"  "lyricCount()"  '(""))
  (def "ms-part-harmonyCount" "harmonyCount()" '("")))

(scm/c-fun "ms-scores-nstaves" ()
  '("traverse Score->nstaves() over [MuseScoreCore->scores()]"
    "Example: (ms-scores-nstaves) => (3 2)")
  (c-make-scheme-list 6 'int
    "QList<MasterScore*> scoreList = mscore->scores();
    for (auto &ms : scoreList) {
      int item = ms->nstaves();~%"))

(scm/c-fun "ms-scores" ()
  '("make a scheme list of all musescore-score-class objects")
  (c-make-scheme-list 6 "ms_obj_score_type"
    "QList<MasterScore*> scoreList = mscore->scores();
      for (auto &item : scoreList) {~%"))

(scm/c-fun "ms-score-nstaves" ("SCM score_obj")
  '("get the number of staves from a score")
  (var-transfer-expand 6 "score_obj" "nstaves"
   '(("void*" scm-ref c) ("MasterScore*")
     ("int" m"nstaves()" c)))
  (f "return scm_from_int(nstaves);~%"))


(scm/c-fun "ms-score-staves" ("SCM score_obj")
  '("make a scheme list of all staves in a score")
  (c-make-scheme-list 6 "ms_obj_staff_type"
    "void* obj = scm_foreign_object_ref(score_obj, 0);
    MasterScore *ms_score = (MasterScore *) obj;
    QList<Staff*>& staves = ms_score->staves();
    for (auto &item : staves) {~%"))

(scm/c-fun "ms-staff-info" ("SCM staff_obj")
  '("returns a vector containing staff information")
   (f "void* obj = scm_foreign_object_ref(staff_obj, 0);
       Staff *staff = (Staff *) obj;
       // from score.cpp:cmdConcertPitchChanged
       int staffIdx   = staff->idx();
       int startTrack = staffIdx * VOICES;
       int endTrack   = startTrack + VOICES;
       Part *part = staff->part();
       int ninst = part->instruments()->size();
       SCM partname = scm_from_locale_string(qPrintable(part->partName()));
       SCM v = scm_c_make_vector(5, SCM_EOL);
       SCM_SIMPLE_VECTOR_SET(v, 0, scm_from_int(staffIdx));
       SCM_SIMPLE_VECTOR_SET(v, 1, scm_from_int(startTrack));
       SCM_SIMPLE_VECTOR_SET(v, 2, scm_from_int(endTrack));
       SCM_SIMPLE_VECTOR_SET(v, 3, scm_from_int(ninst));
       SCM_SIMPLE_VECTOR_SET(v, 4, partname);
       return v;~%"))

(scm/c-fun "ms-score-segment-last" ("SCM score_obj")
  '("get the last segment in score")
  (var-transfer-expand 6 "score_obj" "seg"
   '(("void*" scm-ref c) ("Score*")
     ("Segment*" m"lastSegment()" c)))
  (f "return scm_make_foreign_object_1 ((SCM)ms_obj_segment_type, (SCM) seg);~%"))

; See libmscore/score.cpp:Score::fixTicks for similar code
(scm/c-fun "ms-score-measures" ("SCM score_obj")
  '("make a scheme list of all measures in a score")
  (c-make-scheme-list 6 "ms_obj_measure_type"
    "void* obj = scm_foreign_object_ref(score_obj, 0);
    MasterScore *ms_score = (MasterScore *) obj;
    for (MeasureBase* mb = ms_score->first(); mb; mb = mb->next()) {
          if (mb->type() != ElementType::MEASURE) {
                continue;
                }
          Measure *item = toMeasure(mb);~%"))

(scm/c-fun "ms-score-firstmeasure" ("SCM score_obj")
  '("returns first measure in score")
  (var-transfer-expand 6 "score_obj" "mea"
   '(("void*" scm-ref c r) ("Score*")
     ("Measure*" m"firstMeasure()" c r)))
  (f "return scm_make_foreign_object_1 ((SCM)ms_obj_measure_type, (SCM) mea);~%"))

(scm/c-fun "ms-measure-first" ("SCM measure_obj")
  '("first segment in measures segmentlist")
  (var-transfer-expand 6 "measure_obj" "seg"
   '(("void*" scm-ref c r) ("Measure*")
     ("Segment*" m"first()" c r)))
  (f "return scm_make_foreign_object_1 ((SCM)ms_obj_segment_type, (SCM) seg);~%"))


(let-syntax
  ((def
    (syntax-rules ()
      ((def cname meth descr)
         (scm/c-fun cname ("SCM measure_obj")
           descr
           (var-transfer-expand 6 "measure_obj" "x"
            '(("void*" scm-ref c r) ("Measure*")
              ("int" m meth c)))
           (f "return scm_from_int(x);~%"))))))
  (def "ms-measure-no" "no()" '("measure ordinal number")))

(let-syntax
  ((def
    (syntax-rules ()
      ((def cname meth descr)
         (scm/c-fun cname ("SCM measure_obj")
           descr
           (var-transfer-expand 6 "measure_obj" "x"
            '(("void*" scm-ref c r) ("Measure*")
              ("Fraction" m meth c)))
           (f "return scm_from_int(x.ticks());~%"))))))
  (def "ms-measure-ticks" "ticks()" '("measure length in ticks"))
  (def "ms-measure-tick" "tick()" '("measure position in ticks")))

(scm/c-fun "ms-measure-first-type" ("SCM measure_obj" "SCM stype")
  '("first segment in measures segmentlist of type")
  (f "SegmentType st = (SegmentType) scm_to_int(stype);~%")
  (var-transfer-expand 6 "measure_obj" "seg"
   '(("void*" scm-ref c r) ("Measure*")
     ("Segment*" m"first(st)" c r)))
  (f "return scm_make_foreign_object_1 ((SCM)ms_obj_segment_type, (SCM) seg);~%"))

(scm/c-fun "ms-measure-find-segment" ("SCM measure_obj" "SCM stype_obj" "SCM tick_obj")
  '("find segment of type, and between ticks")
  (f "SegmentType st = (SegmentType) scm_to_int(stype_obj);
      int tick       =  scm_to_int(tick_obj);
      Fraction frac  =  Fraction(tick, 0);~%")
  (var-transfer-expand 6 "measure_obj" "seg"
   '(("void*" scm-ref c r) ("Measure*")
     ("Segment*" m"findSegment(st, frac)" c r)))
  (f "return scm_make_foreign_object_1 ((SCM)ms_obj_segment_type, (SCM) seg);~%"))

; no information is kept in SegmentList
(scm/c-fun "ms-measure-segments" ("SCM measure_obj")
  '("list of Ms::Segment from a measure")
  (c-make-scheme-list 6 "ms_obj_segment_type"
    "void* obj = scm_foreign_object_ref(measure_obj, 0);
    Measure *m = (Measure *) obj;
    for (Segment *item = m->first(); item; item = item->next()) {~%"))

(scm/c-fun "ms-segment-elements" ("SCM segment_obj")
  '("list of Ms::Element from a segment")
  (c-make-scheme-list2 6 "ms_obj_element_type"
    "void* obj = scm_foreign_object_ref(segment_obj, 0);
    Segment *seg = (Segment *) obj;
    for(auto &item : seg->elist()) {
      // only return elements elist() will return NULLs too
      if(item){
      ~%"
    "}~%" ; close for loop
    ))

(scm/c-fun "ms-segment-annotations" ("SCM segment_obj")
  '("list of annotation (Element objects) in segment")
  (var-transfer-expand 6 "segment_obj" "ann"
   '(("void*" scm-ref c r) ("Segment*")
     ("const std::vector<Element*>&" m"annotations()" c)))
  (c-make-scheme-list 6 "ms_obj_element_type"
    "for (auto item : ann) {~%"))

(scm/c-fun "ms-segment-element" ("SCM segment_obj" "SCM index")
  '("returns element by index from segment")
  (f "int idx = scm_to_int(index);~%")
  (var-transfer-expand 6 "segment_obj" "elt"
   '(("void*" scm-ref c r) ("Segment*")
     ("Element*" m"element(idx)" c r)))
  (f "return scm_make_foreign_object_1 ((SCM)ms_obj_element_type, (SCM) elt);~%"))

(scm/c-fun "ms-segment-type" ("SCM segment_obj")
  '("returns segment type")
  (var-transfer-expand 6 "segment_obj" "st"
   '(("void*" scm-ref c) ("Segment*")
     ("SegmentType" m"segmentType()" c)))
   (f "return scm_from_int((int) st);~%"))

(let-syntax
  ((def
    (syntax-rules ()
      ((def cname meth descr)
       (scm/c-fun cname ("SCM segment_obj")
         '(descr)
         (var-transfer-expand 6 "segment_obj" "seg"
          '(("void*" scm-ref c) ("Segment*")
            ("Segment*" m meth c r)))
          (f "return scm_make_foreign_object_1 ((SCM)ms_obj_segment_type, (SCM) seg);~%"))))))
  (def "ms-segment-next"  "next()" "returns next segment")
  (def "ms-segment-next1" "next1()" "returns next segment (goes beyond measure if needed)")
  (def "ms-segment-prev"  "prev()" "returns prev segment")
  (def "ms-segment-prev1" "prev1()" "returns prev segment (goes beyond measure if needed)")
)

(scm/c-fun "ms-segment-next-type" ("SCM segment_obj" "SCM stype")
  '("returns next segment of type stype (goes beyond measure if needed)")
  (f "SegmentType st = (SegmentType) scm_to_int(stype);~%")
  (var-transfer-expand 6 "segment_obj" "segnext"
   '(("void*" scm-ref c) ("Segment*")
     ("Segment*" m"next(st)" c r)))
   (f "return scm_make_foreign_object_1 ((SCM)ms_obj_segment_type, (SCM) segnext);~%"))

(scm/c-fun "ms-segment-next1-type" ("SCM segment_obj" "SCM stype")
  '("returns next segment of type stype (goes beyond measure if needed)")
  (f "SegmentType st = (SegmentType) scm_to_int(stype);~%")
  (var-transfer-expand 6 "segment_obj" "segnext"
   '(("void*" scm-ref c) ("Segment*")
     ("Segment*" m"next1(st)" c r)))
   (f "return scm_make_foreign_object_1 ((SCM)ms_obj_segment_type, (SCM) segnext);~%"))

(scm/c-fun "ms-segment-tick" ("SCM segment_obj")
  '("returns segment tick")
  (var-transfer-expand 6 "segment_obj" "seg"
   '(("void*" scm-ref c) ("Segment*")))
  (f "Fraction f = seg->ticks();
      return scm_from_int(f.ticks());~%"))

(scm/c-fun "ms-element-part" ("SCM element_obj")
  '("returns element part")
  (var-transfer-expand 6 "element_obj" "part"
   '(("void*" scm-ref c) ("Element*") ("Part*" m"part()")))
   (f "return scm_make_foreign_object_1 ((SCM)ms_obj_part_type, (SCM) part);~%"))

(scm/c-fun "ms-element-type" ("SCM element_obj")
  '("returns type of element")
  (var-transfer-expand 6 "element_obj" "etype"
   '(("void*" scm-ref c) ("Element*") ("int" m"type()")))
  (f "return scm_from_int(etype);~%"))

(let-syntax
  ((def
    (syntax-rules ()
      ((def name meth descr)
       (scm/c-fun name ("SCM element_obj") descr
         (var-transfer-expand 6 "element_obj" "num"
          '(("void*" scm-ref c) ("Element*") ("int" m meth c)))
         (f "return scm_from_int(num);~%"))))))
  (def "ms-element-track" "track()" '("returns element track"))
  (def "ms-element-staffIdx" "staffIdx()" '("returns element staffIdx"))
  (def "ms-element-voice" "voice()" '("returns element voice")))

(let-syntax
  ((def
    (syntax-rules ()
      ((def name meth descr)
       (scm/c-fun name ("SCM element_obj") descr
         (var-transfer-expand 6 "element_obj" "f"
          '(("void*" scm-ref c) ("Element*") ("Fraction" m meth c)))
         (f "return scm_from_int(f.ticks());~%"))))))
  (def "ms-element-tick" "tick()" '("returns element tick")))

(scm/c-fun "ms-element-color" ("SCM element_obj")
  '("returns element color")
  (var-transfer-expand 6 "element_obj" "col"
   '(("void*" scm-ref c) ("Element*") ("QColor" m"color()")))
  ; The typedef QRgb, is identical to unsigned int
  (f "QRgb qc = col.rgba();
      return scm_from_uint(qc);~%"))

(scm/c-fun "ms-element-color!" ("SCM element_obj" "SCM color")
  '("set element color RGB")
  (var-transfer-expand 6 "element_obj" "elm"
   '(("void*" scm-ref c) ("Element*")))
  (f "unsigned int col = scm_to_uint(color);
      elm->setColor(QRgb(col));
      return SCM_BOOL_T;~%"))

(scm/c-fun "ms-element-notes" ("SCM element_obj")
  '("return notes from element")
  (var-transfer-expand 6 "element_obj" "elm"
   '(("void*" scm-ref c) ("Element*")))
  (f "if (! elm->isChord()) { return SCM_EOL; }~%{~%")
  (var-transfer-expand 6 "elm" "notelist"
   '(("Ms::Chord*") ("std::vector<Note*>&" m"notes()" c)))
  (f "int numnotes = notelist.size();
      SCM v = scm_c_make_vector(numnotes, SCM_EOL);
      int n = 0;
      for (Note* note : notelist) {
            SCM scmnote = scm_make_foreign_object_1 ((SCM)ms_obj_note_type, (SCM) note);
            SCM_SIMPLE_VECTOR_SET(v, n, scmnote);
            n++;
      }
      return v;~%}~%"))

(let-syntax
  ((emit
    (syntax-rules ()
      ((emit name typ kod)
       (scm/c-fun name ("SCM element_obj") '()
         (var-transfer-expand 6 "element_obj" "elm"
          '(("void*" scm-ref c) ("Element*")))
         (f "if (! elm->isRest()) { return SCM_BOOL_F; }
      ChordRest *cr = (ChordRest *) elm;
      ~a r = cr->~a();
      return scm_from_~a(r);~%" typ kod typ))))))
  ;     scheme-name     type   rest-method
  (emit "ms-element-dots"  "int"  "actualDots"))

(let-syntax
  ((emit
    (syntax-rules ()
      ((emit name kod)
       (scm/c-fun name ("SCM element_obj") '()
         (var-transfer-expand 6 "element_obj" "elm"
          '(("void*" scm-ref c) ("Element*")))
         (f "if (! elm->isRest()) { return SCM_BOOL_F; }
      ChordRest *cr = (ChordRest *) elm;
      Fraction f = cr->~a();
      return scm_from_int(f.ticks());~%" kod))))))
  ;     scheme-name        rest-method
  (emit "ms-element-ticks" "durationTypeTicks"))

(scm/c-fun "ms-element-info" ("SCM element_obj")
  '("returns type of element")
  (f "void* obj = scm_foreign_object_ref(element_obj, 0);
      Element *e = (Element *) obj;
      ElementType etype = e->type();
      int ietype = (int) etype;
      const char* ename = e->name(etype);
      QString quname = e->userName();
      QByteArray baquname = quname.toLocal8Bit();
      const char *uname = baquname.data();
      SCM v = scm_c_make_vector(4, SCM_EOL);
      SCM_SIMPLE_VECTOR_SET(v, 0, scm_from_int(ietype));
      SCM_SIMPLE_VECTOR_SET(v, 1, scm_from_utf8_string(ename));
      SCM_SIMPLE_VECTOR_SET(v, 2, scm_from_utf8_string(uname));
      SCM_SIMPLE_VECTOR_SET(v, 3, scm_from_int(e->track()));
      return v;~%"))

(scm/c-fun "ms-element-rect" ("SCM element_obj")
  '("returns elements rectangle")
  (var-transfer-expand 6 "element_obj" "elm"
   '(("void*" scm-ref c) ("Element*")))
  (f "QRectF& bbox = elm->bbox();
      double x = bbox.x();
      double y = bbox.y();
      double w = bbox.width();
      double h = bbox.height();
      SCM v = scm_c_make_vector(4, SCM_BOOL_F);
      SCM_SIMPLE_VECTOR_SET(v, 0, scm_from_double(x));
      SCM_SIMPLE_VECTOR_SET(v, 1, scm_from_double(y));
      SCM_SIMPLE_VECTOR_SET(v, 2, scm_from_double(w));
      SCM_SIMPLE_VECTOR_SET(v, 3, scm_from_double(h));
      return v;~%"))

(scm/c-fun "ms-figuredbass-info" ("SCM element_obj")
  '("returns figuredbass info from element")
  (var-transfer-expand 6 "element_obj" "elm"
   '(("void*" scm-ref c) ("Element*")))
  (f "if (elm->type() != ElementType::FIGURED_BASS) {
          return SCM_BOOL_F;
      }
      FiguredBass* fb = static_cast<FiguredBass*>(elm);
      Fraction ticks = fb->ticks();
      int numitems = fb->numOfItems();
      std::vector<FiguredBassItem*> items = fb->getItems();
      SCM v = scm_c_make_vector(numitems * 5 + 2, SCM_EOL);
      SCM_SIMPLE_VECTOR_SET(v, 0, scm_from_int(ticks.numerator()));
      SCM_SIMPLE_VECTOR_SET(v, 1, scm_from_int(ticks.denominator()));
      int n = 2;
      for (FiguredBassItem* fbi : items) {
          FiguredBassItem::Modifier pre = fbi->prefix();
          FiguredBassItem::Modifier suf = fbi->suffix();
          int      dig = fbi->digit();
          FiguredBassItem::ContLine cline  = fbi->contLine();
          QString  qs  = fbi->normalizedText();
          SCM ntext = scm_from_utf8_string(qs.toLocal8Bit().data());
          SCM_SIMPLE_VECTOR_SET(v, n, ntext);
          SCM_SIMPLE_VECTOR_SET(v, n+1, scm_from_int(dig));
          SCM_SIMPLE_VECTOR_SET(v, n+2, scm_from_int((int)pre));
          SCM_SIMPLE_VECTOR_SET(v, n+3, scm_from_int((int)suf));
          SCM_SIMPLE_VECTOR_SET(v, n+4, scm_from_int((int)cline));
          n += 5;
      }
      return v;~%"))

; Note object get functions
(let-syntax
  ((emit
    (syntax-rules ()
      ((emit name typ kod)
       (scm/c-fun name ("SCM element_obj") '()
         (format #t "
      void* obj = scm_foreign_object_ref(element_obj, 0);
      Note *note = (Note *) obj;
      ~a r = note->~a();
      return scm_from_~a(r);~%" typ kod typ))))))
  ;     scheme-name         type   note-method
  (emit "ms-note-mark"      "bool" "mark")
  (emit "ms-note-velo-offset" "int"  "veloOffset")
  (emit "ms-note-subchannel"  "int"  "subchannel")
  (emit "ms-note-play"        "bool" "play")
  (emit "ms-note-playticks" "int"  "playTicks")
  (emit "ms-note-tpc"       "int"  "tpc")
  (emit "ms-note-tpc1"      "int"  "tpc1")
  (emit "ms-note-tpc2"      "int"  "tpc2")
  (emit "ms-note-pitch"     "int"  "pitch")
  (emit "ms-note-ppitch"    "int"  "ppitch")
  (emit "ms-note-epitch"    "int"  "epitch")
  (emit "ms-note-tuning"    "double" "tuning")
  )

; Note object set arg-1
(let-syntax
  ((emit
   (lambda (_)
     (syntax-case _ ()
       ((emit name typ kod)
        #`(scm/c-fun name ("SCM element_obj" "SCM val") '()
            (format #t "
      void* obj = scm_foreign_object_ref(element_obj, 0);
      Note *note = (Note *) obj;
      ~a v = scm_to_~a(val);
      note->~a(v);
      return SCM_BOOL_T;~%" typ typ kod)))))))
  ;     scheme-name       type          note-method
  (emit "ms-note-tuning!" "double"   "setTuning")
  (emit "ms-note-subchannel!" "int"  "setSubchannel")
  (emit "ms-note-velo-offset!" "int" "setVeloOffset")
  (emit "ms-note-ontime-offset!" "int" "setOnTimeOffset")
  (emit "ms-note-offtime-offset!" "int" "setOffTimeOffset")
  (emit "ms-note-play!"        "bool" "setPlay")
  (emit "ms-note-pitch!"       "int"  "setPitch")
  )

(scm/c-fun "ms-note-accidental" ("SCM note_obj")
  '("return note accidental")
  (var-transfer-expand 6 "note_obj" "note"
   '(("void*" scm-ref c) ("Note*")))
  (f "Accidental* acc = note->accidental();
      if (! acc) { return SCM_BOOL_F; }
      return scm_make_foreign_object_1 ((SCM)ms_obj_accidental_type, (SCM) acc);~%"))

(scm/c-fun "ms-note-numdots" ("SCM note_obj")
  '("return number of note dots")
  (var-transfer-expand 6 "note_obj" "num"
   '(("void*" scm-ref c) ("Note*")
     ("int" m"qmlDotsCount()" c)))
  (f "return scm_from_int(num);~%"))

(scm/c-fun "ms-note-dots" ("SCM note_obj")
  '("return note dots")
  (var-transfer-expand 6 "note_obj" "dots"
   '(("void*" scm-ref c) ("Note*")
     ("QVector<NoteDot*>&" m"dots()" c)))
  (c-make-scheme-list 6 "ms_obj_element_type"
    "for (auto &item : dots) {~%"))

(scm/c-fun "ms-inputstate-track!" ("SCM inputstate_obj" "SCM track")
  '("sets track index in inputstate")
  (f "void* obj = scm_foreign_object_ref(inputstate_obj, 0);
      InputState *is = (InputState *) obj;
      is->setTrack(scm_to_int(track));
      return SCM_BOOL_T;~%"))

; special in the way that segment can be NULL, and is represented by scheme-false object (#f).
(scm/c-fun "ms-inputstate-segment!" ("SCM inputstate_obj" "SCM segment")
  '("sets segment index in inputstate")
  (f "void* obj = scm_foreign_object_ref(inputstate_obj, 0);
      InputState *is = (InputState *) obj;
      if (segment == SCM_BOOL_F) {
            is->setSegment(nullptr);
            }
      else {
            Segment *seg = (Segment *) scm_foreign_object_ref(segment, 0);
            is->setSegment(seg);
           }
      return SCM_BOOL_T;~%"))

(scm/c-fun "ms-score-selection" ("SCM score_obj")
  '("returns selection object from score")
  (f "void* obj = scm_foreign_object_ref(score_obj, 0);
      Score *s = (Score *) obj;
      Selection *sel = &(s->selection());
      return scm_make_foreign_object_1 ((SCM)ms_obj_selection_type, (SCM) sel);~%"))

(scm/c-fun "ms-selection-startsegment" ("SCM selection_obj")
  '("returns first segment in selection")
  (var-transfer-expand 6 "selection_obj" "seg"
   '(("void*" scm-ref c) ("Selection*")
     ("Segment*" m"startSegment()" c r)))
  (f "return scm_make_foreign_object_1 ((SCM)ms_obj_segment_type, (SCM) seg);~%"))

(scm/c-fun "ms-selection-endsegment" ("SCM selection_obj")
  '("returns last segment in selection")
  (var-transfer-expand 6 "selection_obj" "seg"
   '(("void*" scm-ref c) ("Selection*")
     ("Segment*" m"endSegment()" c r)))
  (f "return scm_make_foreign_object_1 ((SCM)ms_obj_segment_type, (SCM) seg);~%"))

(scm/c-fun "ms-selection-staffstart" ("SCM selection_obj")
  '("returns staffstart index")
  (f "void* obj = scm_foreign_object_ref(selection_obj, 0);
      Selection *sel = (Selection *) obj;
      return scm_from_int(sel->staffStart());~%"))

(scm/c-fun "ms-selection-staffend" ("SCM selection_obj")
  '("returns staffstart index")
  (f "void* obj = scm_foreign_object_ref(selection_obj, 0);
      Selection *sel = (Selection *) obj;
      return scm_from_int(sel->staffEnd());~%"))

(f "
void init_guile_musescore_functions ()
{
      // change into musescore-c module
      scm_set_current_module(scm_c_resolve_module(\"musescore-c\"));

      // register all functions that is reachable from scheme
")
      (for-each
        (lambda (lst)
          (match lst
            ((name c-name numargs)
             (indent 6) (scheme-subr name c-name numargs)
             (indent 6) (export-proc name))))
        *c-scheme-functions*)

(f "// initialize types~%")
(for-each
  (lambda (obj-type)
    (f "    ms_obj_~a_type = init_ms_object_1(\"<ms-~a>\", \"~a\");~%" obj-type obj-type obj-type))
  obj-types)
(f "
}// Eof function init_guile_musescore_functions

} // Eof Namespace ScriptGuile
")
