
(eval-when (expand load eval)
  (load "../script/guile/common.scm")
  (load "../script/guile/build.scm"))

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
       "\"libmscore/staff.h\"" "\"libmscore/measurebase.h\""
       "\"libmscore/measure.h\"" "\"libmscore/chord.h\""
       "\"libmscore/segment.h\"" "\"libmscore/segmentlist.h\""
       "\"libmscore/element.h\""
       "\"libmscore/utils.h\""
       "\"musescore.h\""
       "\"scoreview.h\"" "\"scoretab.h\""
       "\"guile.h\"" "\"guile-glue.h\""))

(f "
// Keep this outside any c++ namespace because we want FFI access
SCM ms_obj_score_type;
SCM ms_obj_staff_type;
SCM ms_obj_measure_type;
SCM ms_obj_segment_type;
SCM ms_obj_element_type;
SCM ms_obj_note_type;
SCM ms_obj_accidental_type;
SCM ms_obj_selection_type;
SCM ms_obj_inputstate_type;

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

// To make a scheme-object containing a musescore-score-class object from the scheme-side, an index into scoreList is needed.
SCM
make_ms_obj_score (int idx)
{
  QList<MasterScore*> scoreList = mscore->scores();
  MasterScore *score = nullptr;
  int n = 0;
  for (auto &ms : scoreList) {
    if(n == idx){
      score = ms;
      break;
    }
    n++;
  }
  if(! score){ // index was out of range
    return SCM_EOL;
  }
  return scm_make_foreign_object_1 (ms_obj_score_type, (SCM) score);
}

//
// core musescore function
//

")

(let-syntax
  ((emit
    (syntax-rules ()
      ((emit name fun)
        (scm/c-fun name "" '()
          (f "QString s = QCoreApplication::~a();
          QByteArray ba = s.toLocal8Bit();
          const char *cs = ba.data();
          return scm_false_or_string(cs);~%" fun)
          )))))
  (emit "core_appname"    "applicationName")
  (emit "core_appversion" "applicationVersion"))

(scm/c-fun "ms_version_major" "" '()
  (f " return scm_from_int(majorVersion());~%"))

(scm/c-fun "ms_version_minor" "" '()
  (f " return scm_from_int(minorVersion());~%"))

(scm/c-fun "ms_version_update" "" '()
  (f " return scm_from_int(updateVersion());~%"))

; Returns true if running musescore version is
; higher or equal than given version argument.
(scm/c-fun "ms_version_check" "SCM version" '()
  (f "const char *verstr = scm_to_locale_string(version);
      QString ver = QString::fromUtf8(verstr);
      bool c = compareVersion(QCoreApplication::applicationVersion(), ver);
      return (c ? SCM_BOOL_F : SCM_BOOL_T);~%"))

(scm/c-fun "core_experimental" "" '()
  (f "bool ee = enableExperimental;
      return scm_from_bool(ee);~%"))

(f "//
// musescore musical globals
//~%")

(scm/c-fun "ms_panPlayback" "" '()
  (indent 6) (f "bool ee = MScore::panPlayback;
      return scm_from_bool(ee);~%"))

(scm/c-fun "ms_playRepeats" "" '()
  (indent 6) (f "bool ee = MScore::playRepeats;
      return scm_from_bool(ee);~%"))

(f "//
// MasterScores
//~%")

(scm/c-fun "ms_scores_count" "" '()
  (f "int c = mscore->currentScoreTab()->count();
      return scm_from_int (c);~%"))

(f "
static SCM
ms_scoreview_cmd (SCM str)
{
      char *cmd = scm_to_locale_string(str);
      // need to check mscore->currentScore() != NULL ?
      ScoreView* cv = mscore->currentScoreView();
      if(cv){
            cv->cmd(cmd);
            }
      else {
            return scm_from_int (-666);
            }
      return SCM_EOL;
      }

static SCM
ms_current_score ()
      {
      Score* score = mscore->currentScore();
      return scm_make_foreign_object_1 ((SCM)ms_obj_score_type, (SCM) score);
      }

static SCM
ms_parts ()
      {
      Score* _score = mscore->currentScore();
      if (_score->parts().isEmpty()) {
            std::cerr << \"No parts!\" << std::endl;
            return SCM_EOL; // return an empty list
            }
      else {
            for (Part* part : _score->parts()) {
                  //const InstrumentList* il = part->instruments();
                  std::cerr << \"part %s\" << qPrintable(part->partName()) << std::endl;
                  }
            return SCM_EOL; // return an empty list
            }
      }

// ms_parts_instruments x :: List String
// where x :: Int -- 1 = instrumentId, 2 = trackName
// example: (ms_parts_instruments 1) => (\"voice.alto\" \"voice.bass\")
static SCM
ms_parts_instruments (SCM part)
      {
      int midx = scm_to_int(part); // member index
      SCM head = SCM_EOL; // head of (single-linked) list
      SCM last = SCM_EOL; // last cons in list
      Score* _score = mscore->currentScore();
      if (_score->parts().isEmpty()) {
            return SCM_EOL; // return an empty list
            }
      else {
            foreach(Part* part, _score->parts()) {
                  const InstrumentList* il = part->instruments();
                  // il :: a std:map of class Instrument*
                  for(auto inst = il->begin(); inst != il->end(); inst++) {
                        // inst :: (Pair idx (class Instrument))
                        QString iid = inst->second->instrumentId();
                        QString trn = inst->second->trackName();
                        // append to list
                        SCM data;
                        if(midx == 1){
                              data = scm_from_locale_string(iid.toLocal8Bit().data());
                              }
                        else {
                              data = scm_from_locale_string(trn.toLocal8Bit().data());
                              }
                        last = s_push(last, data);
                        if (head == SCM_EOL) {
                              head = last;
                              }
                        }
                  }
            return head; // return first element cons in list
            }
      }

")

(scm/c-fun "ms_scores_nstaves" ""
  '("traverse Score->nstaves() over [MuseScoreCore->scores()]"
    "Example: (ms-scores-nstaves) => (3 2)")
  (c-make-scheme-list 6 'int
    "QList<MasterScore*> scoreList = mscore->scores();
    for (auto &ms : scoreList) {
      int item = ms->nstaves();~%"))

(scm/c-fun "ms_scores" ""
  '("make a scheme list of all musescore-score-class objects")
  (c-make-scheme-list 6 "ms_obj_score_type"
    "QList<MasterScore*> scoreList = mscore->scores();
      for (auto &item : scoreList) {~%"))

(scm/c-fun "ms_score_nstaves" "SCM score_obj"
  '("get the number of staves from a score")
  (var-transfer-expand 6 "score_obj" "nstaves"
   '(("void*" scm-ref c) ("MasterScore*")
     ("int" m"nstaves()" c)))
  (f "return scm_from_int(nstaves);~%"))


(scm/c-fun "ms_score_staves" "SCM score_obj"
  '("make a scheme list of all staves in a score")
  (c-make-scheme-list 6 "ms_obj_staff_type"
    "void* obj = scm_foreign_object_ref(score_obj, 0);
    MasterScore *ms_score = (MasterScore *) obj;
    QList<Staff*>& staves = ms_score->staves();
    for (auto &item : staves) {~%"))

(scm/c-fun "ms_staff_info" "SCM staff_obj"
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
       double volume = part->volume();
       SCM v = scm_c_make_vector(6, SCM_EOL);
       SCM_SIMPLE_VECTOR_SET(v, 0, scm_from_int(staffIdx));
       SCM_SIMPLE_VECTOR_SET(v, 1, scm_from_int(startTrack));
       SCM_SIMPLE_VECTOR_SET(v, 2, scm_from_int(endTrack));
       SCM_SIMPLE_VECTOR_SET(v, 3, scm_from_int(ninst));
       SCM_SIMPLE_VECTOR_SET(v, 4, partname);
       SCM_SIMPLE_VECTOR_SET(v, 5, scm_from_double(volume));
       return v;~%"))

(scm/c-fun "ms_score_segment_last" "SCM score_obj"
  '("get the last segment in score")
  (var-transfer-expand 6 "score_obj" "seg"
   '(("void*" scm-ref c) ("Score*")
     ("Segment*" m"lastSegment()" c)))
  (f "return scm_make_foreign_object_1 ((SCM)ms_obj_segment_type, (SCM) seg);~%"))

; See libmscore/score.cpp:Score::fixTicks for similar code
(scm/c-fun "ms_score_measures" "SCM score_obj"
  '("make a scheme list of all measures in a score")
  (c-make-scheme-list 6 "ms_obj_measure_type"
    "void* obj = scm_foreign_object_ref(score_obj, 0);
    MasterScore *ms_score = (MasterScore *) obj;
    for (MeasureBase* mb = ms_score->first(); mb; mb = mb->next()) {
          if (mb->type() != ElementType::MEASURE) {
                continue;
                }
          Measure *item = toMeasure(mb);~%"))

(scm/c-fun "ms_score_firstmeasure" "SCM score_obj"
  '("returns first measure in score")
  (var-transfer-expand 6 "score_obj" "mea"
   '(("void*" scm-ref c r) ("Score*")
     ("Measure*" m"firstMeasure()" c r)))
  (f "return scm_make_foreign_object_1 ((SCM)ms_obj_measure_type, (SCM) mea);~%"))

(scm/c-fun "ms_measure_first" "SCM measure_obj"
  '("first segment in measures segmentlist")
  (var-transfer-expand 6 "measure_obj" "seg"
   '(("void*" scm-ref c r) ("Measure*")
     ("Segment*" m"first()" c r)))
  (f "return scm_make_foreign_object_1 ((SCM)ms_obj_segment_type, (SCM) seg);~%"))

(scm/c-fun "ms_measure_first_type" "SCM measure_obj, SCM stype"
  '("first segment in measures segmentlist of type")
  (f "SegmentType st = (SegmentType) scm_to_int(stype);~%")
  (var-transfer-expand 6 "measure_obj" "seg"
   '(("void*" scm-ref c r) ("Measure*")
     ("Segment*" m"first(st)" c r)))
  (f "return scm_make_foreign_object_1 ((SCM)ms_obj_segment_type, (SCM) seg);~%"))

; no information is kept in SegmentList
(scm/c-fun "ms_measure_segments" "SCM measure_obj"
  '("list of Ms::Segment from a measure")
  (c-make-scheme-list 6 "ms_obj_segment_type"
    "void* obj = scm_foreign_object_ref(measure_obj, 0);
    Measure *m = (Measure *) obj;
    for (Segment *item = m->first(); item; item = item->next()) {~%"))

(scm/c-fun "ms_segment_elements" "SCM segment_obj"
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

(scm/c-fun "ms_segment_element" "SCM segment_obj, SCM index"
  '("returns element by index from segment")
  (f "int idx = scm_to_int(index);~%")
  (var-transfer-expand 6 "segment_obj" "elt"
   '(("void*" scm-ref c r) ("Segment*")
     ("Element*" m"element(idx)" c r)))
  (f "return scm_make_foreign_object_1 ((SCM)ms_obj_element_type, (SCM) elt);~%"))

(scm/c-fun "ms_segment_type" "SCM segment_obj"
  '("returns segment type")
  (var-transfer-expand 6 "segment_obj" "st"
   '(("void*" scm-ref c) ("Segment*")
     ("SegmentType" m"segmentType()" c)))
   (f "return scm_from_int((int) st);~%"))

(scm/c-fun "ms_segment_next" "SCM segment_obj"
  '("returns next segment")
  (var-transfer-expand 6 "segment_obj" "segnext"
   '(("void*" scm-ref c) ("Segment*")
     ("Segment*" m"next()" c r)))
   (f "return scm_make_foreign_object_1 ((SCM)ms_obj_segment_type, (SCM) segnext);~%"))

(scm/c-fun "ms_segment_next1" "SCM segment_obj"
  '("returns next segment (goes beyond measure if needed)")
  (var-transfer-expand 6 "segment_obj" "segnext"
   '(("void*" scm-ref c) ("Segment*")
     ("Segment*" m"next1()" c r)))
   (f "return scm_make_foreign_object_1 ((SCM)ms_obj_segment_type, (SCM) segnext);~%"))

(scm/c-fun "ms_segment_next_type" "SCM segment_obj, SCM stype"
  '("returns next segment of type stype (goes beyond measure if needed)")
  (f "SegmentType st = (SegmentType) scm_to_int(stype);~%")
  (var-transfer-expand 6 "segment_obj" "segnext"
   '(("void*" scm-ref c) ("Segment*")
     ("Segment*" m"next(st)" c r)))
   (f "return scm_make_foreign_object_1 ((SCM)ms_obj_segment_type, (SCM) segnext);~%"))

(scm/c-fun "ms_segment_next1_type" "SCM segment_obj, SCM stype"
  '("returns next segment of type stype (goes beyond measure if needed)")
  (f "SegmentType st = (SegmentType) scm_to_int(stype);~%")
  (var-transfer-expand 6 "segment_obj" "segnext"
   '(("void*" scm-ref c) ("Segment*")
     ("Segment*" m"next1(st)" c r)))
   (f "return scm_make_foreign_object_1 ((SCM)ms_obj_segment_type, (SCM) segnext);~%"))

(scm/c-fun "ms_segment_tick" "SCM segment_obj"
  '("returns segment tick")
  (var-transfer-expand 6 "segment_obj" "seg"
   '(("void*" scm-ref c) ("Segment*")))
  (f "return scm_from_int(seg->tick());~%"))

(scm/c-fun "ms_element_type" "SCM element_obj"
  '("returns type of element")
  (var-transfer-expand 6 "element_obj" "etype"
   '(("void*" scm-ref c) ("Element*") ("int" m"type()")))
  (f "return scm_from_int(etype);~%"))

(scm/c-fun "ms_element_color" "SCM element_obj"
  '("returns element color")
  (var-transfer-expand 6 "element_obj" "col"
   '(("void*" scm-ref c) ("Element*") ("QColor" m"color()")))
  ; The typedef QRgb, is identical to unsigned int
  (f "QRgb qc = col.rgba();
      return scm_from_uint(qc);~%"))

(scm/c-fun "ms_element_set_color" "SCM element_obj, SCM color"
  '("set element color RGB")
  (var-transfer-expand 6 "element_obj" "elm"
   '(("void*" scm-ref c) ("Element*")))
  (f "unsigned int col = scm_to_uint(color);
      elm->setColor(QRgb(col));
      return SCM_BOOL_T;~%"))

(scm/c-fun "ms_element_notes" "SCM element_obj"
  '("return notes from element")
  (var-transfer-expand 6 "element_obj" "elm"
   '(("void*" scm-ref c) ("Element*")))
  (f "if (! elm->isChord()) { return SCM_EOL; }~%{~%")
  (var-transfer-expand 6 "elm" "notelist"
   '(("Chord*") ("std::vector<Note*>&" m"notes()" c)))
  (f "int numnotes = notelist.size();
      SCM v = scm_c_make_vector(numnotes, SCM_EOL);
      int n = 0;
      for (Note* note : notelist) {
            SCM scmnote = scm_make_foreign_object_1 ((SCM)ms_obj_note_type, (SCM) note);
            SCM_SIMPLE_VECTOR_SET(v, n, scmnote);
            n++;
      }
      return v;~%}~%"))

(scm/c-fun "ms_element_info" "SCM element_obj"
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

; Note object get functions
(let-syntax
  ((emit
    (syntax-rules ()
      ((emit name typ kod)
       (scm/c-fun name "SCM element_obj" '()
         (format #t "
      void* obj = scm_foreign_object_ref(element_obj, 0);
      Note *note = (Note *) obj;
      ~a r = note->~a();
      return scm_from_~a(r);~%" typ kod typ))))))
  ;     scheme-name         type   note-method
  (emit "ms_note_mark"      "bool" "mark")
  (emit "ms_note_velo_offset" "int"  "veloOffset")
  (emit "ms_note_subchannel"  "int"  "subchannel")
  (emit "ms_note_play"        "bool" "play")
  (emit "ms_note_playticks" "int"  "playTicks")
  (emit "ms_note_tpc"       "int"  "tpc")
  (emit "ms_note_tpc1"      "int"  "tpc1")
  (emit "ms_note_tpc2"      "int"  "tpc2")
  (emit "ms_note_pitch"     "int"  "pitch")
  (emit "ms_note_ppitch"    "int"  "ppitch")
  (emit "ms_note_epitch"    "int"  "epitch")
  (emit "ms_note_tuning"    "double" "tuning")
  )

; Note object set arg-1
(let-syntax
  ((emit
   (lambda (_)
     (syntax-case _ ()
       ((emit name typ kod)
        #`(scm/c-fun name "SCM element_obj, SCM val" '()
            (format #t "
      void* obj = scm_foreign_object_ref(element_obj, 0);
      Note *note = (Note *) obj;
      ~a v = scm_to_~a(val);
      note->~a(v);
      return SCM_BOOL_T;~%" typ typ kod)))))))
  ;     scheme-name       type          note-method
  (emit "ms_note_set_tuning" "double"   "setTuning")
  (emit "ms_note_set_subchannel" "int"  "setSubchannel")
  (emit "ms_note_set_velo_offset" "int" "setVeloOffset")
  (emit "ms_note_set_ontime_offset" "int" "setOnTimeOffset")
  (emit "ms_note_set_offtime_offset" "int" "setOffTimeOffset")
  (emit "ms_note_set_play"        "bool" "setPlay")
  (emit "ms_note_set_pitch"       "int"  "setPitch")
  )

(scm/c-fun "ms_note_accidental" "SCM note_obj"
  '("return note accidental")
  (var-transfer-expand 6 "note_obj" "note"
   '(("void*" scm-ref c) ("Note*")))
  (f "Accidental* acc = note->accidental();
      if (! acc) { return SCM_BOOL_F; }
      return scm_make_foreign_object_1 ((SCM)ms_obj_accidental_type, (SCM) acc);~%"))

(scm/c-fun "ms_note_numdots" "SCM note_obj"
  '("return number of note dots")
  (var-transfer-expand 6 "note_obj" "num"
   '(("void*" scm-ref c) ("Note*")
     ("int" m"qmlDotsCount()" c)))
  (f "return scm_from_int(num);~%"))

(scm/c-fun "ms_note_dots" "SCM note_obj"
  '("return note dots")
  (var-transfer-expand 6 "note_obj" "dots"
   '(("void*" scm-ref c) ("Note*")
     ("QVector<NoteDot*>&" m"dots()" c)))
  (c-make-scheme-list 6 "ms_obj_element_type"
    "for (auto &item : dots) {~%"))

(scm/c-fun "ms_score_inputstate" "SCM score_obj"
  '("returns inputstate object from score")
  (f "void* obj = scm_foreign_object_ref(score_obj, 0);
      Score *s = (Score *) obj;
      InputState *is = &(s->inputState());
      return scm_make_foreign_object_1 ((SCM)ms_obj_inputstate_type, (SCM) is);~%"))

(scm/c-fun "ms_inputstate_settrack" "SCM inputstate_obj, SCM track"
  '("sets track index in inputstate")
  (f "void* obj = scm_foreign_object_ref(inputstate_obj, 0);
      InputState *is = (InputState *) obj;
      is->setTrack(scm_to_int(track));
      return SCM_BOOL_T;~%"))

; special in the way that segment can be NULL, and is represented by scheme-false object (#f).
(scm/c-fun "ms_inputstate_setsegment" "SCM inputstate_obj, SCM segment"
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

(scm/c-fun "ms_score_selection" "SCM score_obj"
  '("returns selection object from score")
  (f "void* obj = scm_foreign_object_ref(score_obj, 0);
      Score *s = (Score *) obj;
      Selection *sel = &(s->selection());
      return scm_make_foreign_object_1 ((SCM)ms_obj_selection_type, (SCM) sel);~%"))

(scm/c-fun "ms_selection_startsegment" "SCM selection_obj"
  '("returns first segment in selection")
  (var-transfer-expand 6 "selection_obj" "seg"
   '(("void*" scm-ref c) ("Selection*")
     ("Segment*" m"startSegment()" c r)))
  (f "return scm_make_foreign_object_1 ((SCM)ms_obj_segment_type, (SCM) seg);~%"))

(scm/c-fun "ms_selection_endsegment" "SCM selection_obj"
  '("returns last segment in selection")
  (var-transfer-expand 6 "selection_obj" "seg"
   '(("void*" scm-ref c) ("Selection*")
     ("Segment*" m"endSegment()" c r)))
  (f "return scm_make_foreign_object_1 ((SCM)ms_obj_segment_type, (SCM) seg);~%"))

(scm/c-fun "ms_selection_staffstart" "SCM selection_obj"
  '("returns staffstart index")
  (f "void* obj = scm_foreign_object_ref(selection_obj, 0);
      Selection *sel = (Selection *) obj;
      return scm_from_int(sel->staffStart());~%"))

(scm/c-fun "ms_selection_staffend" "SCM selection_obj"
  '("returns staffstart index")
  (f "void* obj = scm_foreign_object_ref(selection_obj, 0);
      Selection *sel = (Selection *) obj;
      return scm_from_int(sel->staffEnd());~%"))

(f "
void init_guile_musescore_functions ()
{
      // register all functions that is reachable from scheme
")

      (map (lambda (lst)
             (apply (lambda (name cfun aritry)
                      (indent 6) (scheme-subr name cfun aritry))
                    lst))
            '(("ms-core-name" "core_appname" 0)
              ("ms-core-version" "core_appversion" 0)
              ("ms-core-experimental" "core_experimental" 0)
              ("ms-pan-playback" "ms_panPlayback" 0)
              ("ms-play-repeats" "ms_playRepeats" 0)
              ("ms-scores-count" "ms_scores_count" 0)
              ("ms-current-score" "ms_current_score" 0)
              ("ms-parts"         "ms_parts" 0)
              ("ms-scoreview-cmd" "ms_scoreview_cmd" 1)
              ("ms-parts-instruments" "ms_parts_instruments" 1)
              ("ms-scores-nstaves" "ms_scores_nstaves" 0)
              ("ms-scores"         "ms_scores" 0)
              ("ms-score-nstaves"  "ms_score_nstaves" 1)
              ("ms-score-staves"   "ms_score_staves" 1)
              ("ms-score-measures"   "ms_score_measures" 1)
              ("ms-score-firstmeasure" "ms_score_firstmeasure" 1)
              ("ms-score-segment-last" "ms_score_segment_last" 1)
              ("ms-score-selection" "ms_score_selection" 1)
              ("ms-score-inputstate" "ms_score_inputstate" 1)
              ("ms-inputstate-track!"   "ms_inputstate_settrack" 2)
              ("ms-inputstate-segment!" "ms_inputstate_setsegment" 2)
              ("ms-staff-info" "ms_staff_info" 1)
              ("ms-measure-first"   "ms_measure_first" 1)
              ("ms-measure-first-type"   "ms_measure_first_type" 2)
              ("ms-measure-segments"   "ms_measure_segments" 1)
              ("ms-segment-elements"   "ms_segment_elements" 1)
              ("ms-segment-element" "ms_segment_element" 2)

              ("ms-segment-type"  "ms_segment_type" 1)
              ("ms-segment-next"  "ms_segment_next" 1)
              ("ms-segment-next1" "ms_segment_next1" 1)
              ("ms-segment-next-type"  "ms_segment_next_type" 2)
              ("ms-segment-next1-type" "ms_segment_next1_type" 2)
              ("ms-segment-tick" "ms_segment_tick" 1)
              ("ms-element-type" "ms_element_type" 1)
              ("ms-element-info" "ms_element_info" 1)
              ("ms-element-color" "ms_element_color" 1)
              ("ms-element-color!" "ms_element_set_color" 2)
              ("ms-element-notes" "ms_element_notes" 1)
              ("ms-version-major" "ms_version_major" 0)
              ("ms-version-minor" "ms_version_minor" 0)
              ("ms-version-update" "ms_version_update" 0)
              ("ms-version-check" "ms_version_check" 1)
              ("ms-note-mark" "ms_note_mark" 1)
              ("ms-note-velo-offset" "ms_note_velo_offset" 1)
              ("ms-note-subchannel" "ms_note_subchannel" 1)
              ("ms-note-play" "ms_note_play" 1)
              ("ms-note-playticks" "ms_note_playticks" 1)
              ("ms-note-tpc" "ms_note_tpc" 1)
              ("ms-note-tpc1" "ms_note_tpc1"   1)
              ("ms-note-tpc2" "ms_note_tpc2"   1)
              ("ms-note-pitch" "ms_note_pitch" 1)
              ("ms-note-ppitch" "ms_note_ppitch" 1)
              ("ms-note-epitch" "ms_note_epitch" 1)
              ("ms-note-tuning" "ms_note_tuning" 1)
              ("ms-note-tuning!" "ms_note_set_tuning" 2)
              ("ms-note-subchannel!" "ms_note_set_subchannel" 2)
              ("ms-note-velo-offset!" "ms_note_set_velo_offset" 2)
              ("ms-note-ontime-offset!" "ms_note_set_ontime_offset" 2)
              ("ms-note-offtime-offset!" "ms_note_set_offtime_offset" 2)
              ("ms-note-play!" "ms_note_set_play" 2)
              ("ms-note-pitch!" "ms_note_set_pitch" 2)
              ("ms-note-accidental" "ms_note_accidental" 1)
              ("ms-note-numdots" "ms_note_numdots" 1)
              ("ms-note-dots" "ms_note_dots" 1)
              ; selection
              ("ms-selection-startsegment" "ms_selection_startsegment" 1)
              ("ms-selection-endsegment" "ms_selection_endsegment" 1)
              ("ms-selection-staffstart" "ms_selection_staffstart" 1)
              ("ms-selection-staffend" "ms_selection_staffend" 1)
              ))

(f "
      // initialize types
      ms_obj_score_type = init_ms_object_1(\"<ms-score>\", \"score\");
      ms_obj_staff_type = init_ms_object_1(\"<ms-staff>\", \"staff\");
      ms_obj_measure_type = init_ms_object_1(\"<ms-measure>\", \"measure\");
      ms_obj_segment_type = init_ms_object_1(\"<ms-segment>\", \"segment\");
      ms_obj_element_type = init_ms_object_1(\"<ms-element>\", \"element\");
      ms_obj_note_type = init_ms_object_1(\"<ms-note>\", \"note\");
      ms_obj_accidental_type = init_ms_object_1(\"<ms-accidental>\", \"accidental\");
      ms_obj_selection_type = init_ms_object_1(\"<ms-selection>\", \"selection\");
      ms_obj_inputstate_type = init_ms_object_1(\"<ms-inputstate>\", \"inputstate\");
}

} // Eof Namespace ScriptGuile
")
