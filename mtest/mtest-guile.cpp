//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2012 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include <vector>
#include <QApplication>
#include <libguile.h>
#include "all.h"
#include "synthesizer/msynthesizer.h"
#include "libmscore/musescoreCore.h"
#include "libmscore/xml.h"
#include "libmscore/score.h"
#include "libmscore/segment.h"
#include "mtest/testutils.h"

SCM ms_foobar2 (void *pa)
      {
      fprintf(stderr, "foobar2: %p\n", pa);
      return scm_from_int(0);
      }

SCM ms_obj_score_type;

namespace ScriptGuile {
using namespace Ms;


class ScriptGuile : public MTest {
      public:
      ScriptGuile () {
            initMTest();
            }
      EditData& get_ed() { return ed; }
      MScore* get_mscore() { return mscore; }
      MasterScore* _readScore(const char *file) {
            return readScore(QString(file));
            }
      };

ScriptGuile *g_sg;

MasterScore* readScoreCString(char *content)
      {
      bool ignoreVersionError = false;
      MasterScore* score = new MasterScore(MScore::defaultStyle());
      //MasterScore* score = new MasterScore(g_sg->get_mscore()->baseStyle());

      QString msx(content);
      XmlReader r(msx, QString());
      //r.setReadAheadDevice(f);
      if (score->read1(r, ignoreVersionError) != Score::FileError::FILE_NO_ERROR) {
            return nullptr;
            }
      score->rebuildMidiMapping();
      score->setSoloMute();
      for (Score* s : score->scoreList()) {
            s->setPlaylistDirty();
            s->addLayoutFlags(LayoutFlag::FIX_PITCH_VELO);
            s->setLayoutAll();
            }
      score->updateChannel();
      score->setSaved(false);
      score->update();
      if (! score->sanityCheck(QString())) {
            return nullptr;
            }
      return score;
      }

SCM ms_score_read_string (SCM scmstr)
      {
      char *s = scm_to_locale_string(scmstr);
      MasterScore* score = readScoreCString(s);
      if (! score) { return SCM_BOOL_F; }
      return scm_make_foreign_object_1 ((SCM) ms_obj_score_type,
                                        (SCM) score);
      }

SCM ms_score_read_file (SCM scmstr)
      {
      char *s = scm_to_locale_string(scmstr);
      MasterScore* score = g_sg->_readScore(s);
      if (! score) { return SCM_BOOL_F; }
      return scm_make_foreign_object_1 ((SCM) ms_obj_score_type,
                                        (SCM) score);
      }

SCM ms_score_forget (SCM score_obj)
      {
      MasterScore* score = (MasterScore*) scm_foreign_object_ref(score_obj, 0);
      scm_foreign_object_set_x(score_obj, 0, nullptr);
      delete score;
      return SCM_BOOL_T;
      }

SCM ms_score_cmd (SCM score_obj, SCM cmd_obj)
      {
      char *str = scm_to_locale_string(cmd_obj);
      Score* score = (Score*) scm_foreign_object_ref(score_obj, 0);
      QAction act(QString(str), nullptr);
      act.setData(QVariant(str));
      score->cmd(&act, g_sg->get_ed());
      return SCM_BOOL_T;
      }

SCM ms_segment_elements_wrap (Segment *seg)
      {
      std::vector<Element*> vec = seg->elist();
      int n = 0;
      for (auto &item : vec) {
            if (item) { // only count occupied slots in vector
                  n++;
                  }
            }
      SCM v = scm_c_make_vector(n, SCM_EOL);
      int i = 0;
      for (auto &item : vec) {
            SCM_SIMPLE_VECTOR_SET(v, i, scm_from_pointer(item, NULL));
            i++;
            }
      return v;
      }

SCM
init_ms_object_1 (const char *type_name, const char *slotname1)
      {
      SCM name, slo;
      scm_t_struct_finalize finalizer = NULL;
      name = scm_from_utf8_symbol (type_name);
      slo = scm_list_1 (scm_from_utf8_symbol (slotname1));
      return scm_make_foreign_object_type (name, slo, finalizer);
      }

// FFI to more complicated stuff not easily FFIable
void init_guile_shim ()
      {
      // initialize module system (guile init is not thread safe)
      scm_c_eval_string ("(define-module (musescore-c))");
      // change into musescore-c module
      scm_set_current_module(scm_c_resolve_module("musescore-c"));
      SCM_TICK; // Checks any pending GC

      ms_obj_score_type = init_ms_object_1("<ms-score>", "score");
      scm_c_define_gsubr ("ms-score-read-file", 1, 0, 0, (void *)ms_score_read_file);
      scm_c_define_gsubr ("ms-score-read-string", 1, 0, 0, (void *)ms_score_read_string);
      scm_c_define_gsubr ("ms-score-forget", 1, 0, 0, (void *)ms_score_forget);
      scm_c_define_gsubr ("ms-score-cmd", 2, 0, 0, (void *)ms_score_cmd);
      scm_c_define_gsubr ("ms-segment-elements-wrap", 1, 0, 0, (void *)ms_segment_elements_wrap);
      scm_c_export ("ms-score-read-file", NULL);
      scm_c_export ("ms-score-read-string", NULL);
      scm_c_export ("ms-score-forget", NULL);
      scm_c_export ("ms-score-cmd", NULL);
      scm_c_export ("ms-segment-elements-wrap", NULL);
      }

// first parameter is a closure, not used here
static void
guile_main (void *, int argc, char **argv)
      {
      init_guile_shim();
      scm_shell (argc, argv);
      }

QGuiApplication *my_qapp;
static void init_musescore () {
      int argc = 0;
      char **argv = nullptr;
      my_qapp = new QGuiApplication(argc, argv);
      QCoreApplication::setApplicationName("mtest-guile");
      g_sg = new ScriptGuile();
      qApp->processEvents();
      StaffType st;
      fprintf(stderr, "stafftype: %p\n", &st);
      }

}

int main (int argc, char* argv[])
      {
      ScriptGuile::init_musescore();
      scm_boot_guile (argc, argv, ScriptGuile::guile_main, 0);
      return 0;
      }

