(use-modules (ice-9 format))

(define-syntax-rule (f exp ...)
  (format #t exp ...))

; crude indent
(define (indent n)
  (do ((i 0 (1+ i))) ((>= i n)) (f " ")))


(define (scheme-subr name cfun arn)
  (f "scm_c_define_gsubr (\"~a\", ~a, 0, 0, (void *)~a);~%"
     name arn cfun))

(define-syntax-rule (scm/c-fun name args doc exp ...)
  (begin
    (map (lambda (line) (f "// ~a~%" line)) doc)
    (f "static SCM
~a (~a)
{~%" name args)
    exp ...
    (f "}~%~%")
    ))

(define-syntax-rule (c-make-scheme-list i dtype code)
  (let ((datastr (cond
                   ((eq? dtype 'int) "SCM data = scm_from_int(item);~%")
                   (else (format #f "SCM data = scm_make_foreign_object_1 ((SCM)~a, (SCM) item);~%" dtype)))))
  (begin
     (indent i) (f "SCM head = SCM_EOL; // head of (single-linked) list~%")
     (indent i) (f "SCM last = SCM_EOL; // last cons in list~%")
     (indent i) (f code)
     (indent (* i 2)) (f datastr)
     (indent (* i 2)) (f "last = s_push(last, data);~%")
     (indent (* i 2)) (f "if (head == SCM_EOL) {~%")
     (indent (* i 2)) (f "      head = last;~%")
     (indent (* i 2)) (f "      }~%")
     (indent (* i 2)) (f "}~%")
     (indent i) (f "return head; // return first element cons in list~%"))))

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
       "\"libmscore/measure.h\""
       "\"musescore.h\""
       "\"scoreview.h\"" "\"scoretab.h\""
       "\"guile.h\"" "\"guile-glue.h\""))

(f "
namespace ScriptGuile {
using namespace Ms;
//
// musescore types
//

static SCM ms_obj_score_type;
static SCM ms_obj_staff_type;
static SCM ms_obj_measure_type;

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
      Score* _score = mscore->currentScore();
      return _score ? SCM_BOOL_T : SCM_BOOL_F;
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
  (f "scm_assert_foreign_object_type (ms_obj_score_type, score_obj);
      void* obj = scm_foreign_object_ref(score_obj, 0);
      MasterScore *ms_score = (MasterScore *) obj;
      int nstaves = ms_score->nstaves();
      return scm_from_int(nstaves);~%"))

(scm/c-fun "ms_score_staves" "SCM score_obj"
  '("make a scheme list of all staves in a score")
  (c-make-scheme-list 6 "ms_obj_staff_type"
    "void* obj = scm_foreign_object_ref(score_obj, 0);
    MasterScore *ms_score = (MasterScore *) obj;
    QList<Staff*>& staves = ms_score->staves();
    for (auto &item : staves) {~%"))

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
              ("ms-score-measures"   "ms_score_measures" 1)))
(f "
      // initialize types
      ms_obj_score_type = init_ms_object_1(\"ms-score\", \"score\");
      ms_obj_staff_type = init_ms_object_1(\"ms-staff\", \"staff\");
      ms_obj_measure_type = init_ms_object_1(\"ms-measure\", \"measure\");
}

} // Eof Namespace ScriptGuile
")
