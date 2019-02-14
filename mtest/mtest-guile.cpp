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

#include <libguile.h>
#include "all.h"
#include "libmscore/musescoreCore.h"
#include "libmscore/xml.h"
#include "libmscore/score.h"

SCM ms_obj_score_type;

namespace ScriptGuile {
using namespace Ms;

MasterScore* readScoreCString(char *content)
      {
      bool ignoreVersionError = false;
      MasterScore* score = new MasterScore(MScore::defaultStyle());
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
      scm_c_define_gsubr ("ms-score-read-string", 1, 0, 0, (void *)ms_score_read_string);
      scm_c_export ("ms-score-read-string");
      }

// first parameter is a closure, not used here
static void
guile_main (void *, int argc, char **argv)
      {
      init_guile_shim();
      scm_shell (argc, argv);
      }

}

int main (int argc, char* argv[])
      {
      scm_boot_guile (argc, argv, ScriptGuile::guile_main, 0);
      return 0;
      }

