/*
 * This file contains functions that is
 * called from scheme.
 *
 */

#include <iostream>
#include <libguile.h>

#include "libmscore/mscore.h"
#include "libmscore/part.h"

#include "musescore.h"
#include "scoreview.h"
#include "scoretab.h"

#include "guile.h"
#include "guile-glue.h"

namespace ScriptGuile {

static SCM ms_obj_score_type;

void
init_ms_obj_score_type ()
{
  SCM name, slo;
  scm_t_struct_finalize finalizer = NULL;
  name = scm_from_utf8_symbol ("ms-score");
  slo = scm_list_1 (scm_from_utf8_symbol ("score"));
  ms_obj_score_type = scm_make_foreign_object_type (name, slo, finalizer);
}

// To make a scheme-object containing a musescore-score-class object from the scheme-side, an index into scoreList is needed.
SCM
make_ms_obj_score (int idx)
{
  QList<Ms::MasterScore*> scoreList = Ms::mscore->scores();
  Ms::MasterScore *score = nullptr;
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

static SCM
core_appname (void)
{
      QString s = QCoreApplication::applicationName();
      QByteArray ba = s.toLocal8Bit();
      const char *cs = ba.data();
      return scm_false_or_string(cs);
      }

static SCM
core_appversion (void)
{
      QString s = QCoreApplication::applicationVersion();
      QByteArray ba = s.toLocal8Bit();
      const char *cs = ba.data();
      return scm_false_or_string(cs);
      }

static SCM
core_experimental (void)
{
      bool ee = Ms::enableExperimental;
      return scm_from_bool(ee);
      }

//
// musescore musical globals
//

static SCM
ms_panPlayback (void)
{
      bool ee = Ms::MScore::panPlayback;
      return scm_from_bool(ee);
      }

static SCM
ms_playRepeats (void)
{
      bool ee = Ms::MScore::playRepeats;
      return scm_from_bool(ee);
      }

//
// MasterScores
//

static SCM
ms_scores_count (void)
{
      int c = Ms::mscore->currentScoreTab()->count();
      return scm_from_int (c);
      }

static SCM
ms_scoreview_cmd (SCM str)
{
      char *cmd = scm_to_locale_string(str);
      // need to check mscore->currentScore() != NULL ?
      Ms::ScoreView* cv = Ms::mscore->currentScoreView();
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
      Ms::Score* _score = Ms::mscore->currentScore();
      return _score ? SCM_BOOL_T : SCM_BOOL_F;
      }

static SCM
ms_parts ()
      {
      Ms::Score* _score = Ms::mscore->currentScore();
      if (_score->parts().isEmpty()) {
            std::cerr << "No parts!" << std::endl;
            return SCM_EOL; // return an empty list
            }
      else {
            for (Ms::Part* part : _score->parts()) {
                  //const InstrumentList* il = part->instruments();
                  std::cerr << "part %s" << qPrintable(part->partName()) << std::endl;
                  }
            return SCM_EOL; // return an empty list
            }
      }

// ms_parts_instruments x :: List String
// where x :: Int -- 1 = instrumentId, 2 = trackName
// example: (ms_parts_instruments 1) => ("voice.alto" "voice.bass")
static SCM
ms_parts_instruments (SCM part)
      {
      int midx = scm_to_int(part); // member index
      SCM head = SCM_EOL; // head of (single-linked) list
      SCM last = SCM_EOL; // last cons in list
      Ms::Score* _score = Ms::mscore->currentScore();
      if (_score->parts().isEmpty()) {
            return SCM_EOL; // return an empty list
            }
      else {
            foreach(Ms::Part* part, _score->parts()) {
                  const Ms::InstrumentList* il = part->instruments();
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

// traverse Score->nstaves() over [MuseScoreCore->scores()]
// Example: (ms-scores-nstaves) => (3 2)
static SCM
ms_scores_nstaves (void)
      {
      SCM head = SCM_EOL; // head of (single-linked) list
      SCM last = SCM_EOL; // last cons in list
      QList<Ms::MasterScore*> scoreList = Ms::mscore->scores();
      for (auto &ms : scoreList) {
            int nstaves = ms->nstaves();
            SCM data = scm_from_int(nstaves);
            last = s_push(last, data);
            if (head == SCM_EOL) {
                  head = last;
                  }
            }
      return head; // return first element cons in list
      }

// make a scheme list of all musescore-score-class objects
static SCM
ms_scores (void)
      {
      SCM head = SCM_EOL; // head of (single-linked) list
      SCM last = SCM_EOL; // last cons in list
      QList<Ms::MasterScore*> scoreList = Ms::mscore->scores();
      for (auto &ms : scoreList) {
            SCM data = scm_make_foreign_object_1 ((SCM)ms_obj_score_type, (SCM) ms);
            last = s_push(last, data);
            if (head == SCM_EOL) {
                  head = last;
                  }
            }
      return head; // return first element cons in list
      }

// get the number of staves from a score
static SCM
ms_score_nstaves (SCM score_obj)
{
      scm_assert_foreign_object_type (ms_obj_score_type, score_obj);
      void* obj = scm_foreign_object_ref(score_obj, 0);
      Ms::MasterScore *ms_score = (Ms::MasterScore *) obj;
      int nstaves = ms_score->nstaves();
      return scm_from_int(nstaves);
}

void init_guile_musescore_functions ()
{
      // register all functions that is reachable from scheme
      scm_c_define_gsubr ("ms-core-name", 0, 0, 0, (void *)core_appname);
      scm_c_define_gsubr ("ms-core-version", 0, 0, 0, (void *)core_appversion);
      scm_c_define_gsubr ("ms-core-experimental", 0, 0, 0, (void *)core_experimental);

      scm_c_define_gsubr ("ms-pan-playback", 0, 0, 0, (void *)ms_panPlayback);
      scm_c_define_gsubr ("ms-play-repeats", 0, 0, 0, (void *)ms_playRepeats);
      scm_c_define_gsubr ("ms-scores-count", 0, 0, 0, (void *)ms_scores_count);
      scm_c_define_gsubr ("ms-current-score", 0, 0, 0, (void *)ms_current_score);
      scm_c_define_gsubr ("ms-parts", 0, 0, 0, (void *)ms_parts);
      scm_c_define_gsubr ("ms-scoreview-cmd", 1, 0, 0, (void *)ms_scoreview_cmd);
      scm_c_define_gsubr ("ms-parts-instruments", 1, 0, 0, (void *)ms_parts_instruments);
      scm_c_define_gsubr ("ms-scores-nstaves", 0, 0, 0, (void *)ms_scores_nstaves);
      scm_c_define_gsubr ("ms-scores", 0, 0, 0, (void *)ms_scores);
      scm_c_define_gsubr ("ms-score-nstaves", 1, 0, 0, (void *)ms_score_nstaves);

      // initialize types
      init_ms_obj_score_type();
}

} // Eof Namespace ScriptGuile
