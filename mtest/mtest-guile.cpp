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


#include <math.h>

#include <libguile.h>
#include "all.h"
#include "muxseq/msynthesizer.h"
#include "libmscore/musescoreCore.h"
#include "libmscore/xml.h"
#include "libmscore/score.h"
#include "libmscore/system.h"
#include "libmscore/measurebase.h"
#include "libmscore/measure.h"
#include "libmscore/segment.h"
#include "libmscore/revisions.h"
#include "mtest/testutils.h"

extern SCM ms_obj_score_type;

namespace ScriptGuile {
using namespace Ms;

SCM init_ms_object_1 (const char *type_name, const char *slotname1);

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

class Myqiod : public QIODevice {
   private:
      std::string _score_string = std::string("");

   public:
      qint64 readData(char *data, qint64 maxlen);
      qint64 writeData(const char *data, qint64 len);
      std::string getData() { return _score_string; }
};

qint64 Myqiod::readData(char *, qint64)
      {
      fprintf(stderr, "WARNING: function Myqiod::readData is not used.\n");
      return -1;
      }
qint64 Myqiod::writeData(const char *data, qint64 len)
      {
      // we are assuming utf-8, not binary, ie can't contain zero
      _score_string += (char *) data;
      return len;
      }

SCM ms_test_read_score (SCM filename)
{
    char *s = scm_to_locale_string(filename);
    fprintf(stderr, "ms_test_read_score: %s\n", s);
    MasterScore* score = g_sg->readCreatedScore(s);
    if (! score) {
        fprintf(stderr, "failed to read score\n");
    }
    if (! score) { return SCM_BOOL_F; }
    return scm_make_foreign_object_1 ((SCM) ms_obj_score_type,
                                      (SCM) score);
}

std::string* writeScoreString(Score *score)
      {
      Myqiod mq;
      mq.open(QIODevice::ReadWrite);
      XmlWriter xml(score, &mq);
      xml.setWriteOmr(false); // not mscz-format
      xml.header();
      xml.stag("museScore version=\"3.01\"");
      score->write(xml, false); // false: onlySelection
      xml.etag();
      if (score->isMaster()) {
            score->masterScore()->revisions()->write(xml);
            }
      mq.close(); // flushes buffer
      return new std::string(mq.getData());
      }

SCM ms_score_write_string (SCM score_obj)
      {
      MasterScore* score = (MasterScore*) scm_foreign_object_ref(score_obj, 0);
      std::string *str = writeScoreString(score);
      SCM s = scm_from_locale_string(str->c_str());
      delete str;
      return s;
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

SCM std_vector_to_scm_vector (std::vector<Element*> vec)
      {
      int n = 0;
      for (auto item : vec) {
            if (item) { // only count occupied slots in vector
                  n++;
                  }
            }
      SCM v = scm_c_make_vector(n, SCM_EOL);
      int i = 0;
      for (auto item : vec) {
            if (item) { // only count occupied slots in vector
                  SCM_SIMPLE_VECTOR_SET(v, i, scm_from_pointer(item, NULL));
                  i++;
                  }
            }
      return v;
      }

SCM ms_measure_elements (SCM mea_scm)
      {
      MeasureBase *mea = (Measure*) scm_to_pointer(mea_scm);
      ElementList elmv = mea->el();
      return std_vector_to_scm_vector (elmv);
      }

SCM ms_segment_elements (SCM seg_scm)
      {
      Segment* seg = (Segment*) scm_to_pointer(seg_scm);
      std::vector<Element*> elmv = seg->elist();
      return std_vector_to_scm_vector (elmv);
      }

SCM ms_qrect_get (SCM qrect_scm)
      {
      void *qrect_ptr = scm_to_pointer(qrect_scm);
      QRectF *qrect = (QRectF *) qrect_ptr;
      qreal x, y, w, h;
      qrect->getRect(&x, &y, &w, &h);
      SCM v = scm_c_make_vector(4, SCM_EOL);
      SCM_SIMPLE_VECTOR_SET(v, 0, scm_from_double(x));
      SCM_SIMPLE_VECTOR_SET(v, 1, scm_from_double(y));
      SCM_SIMPLE_VECTOR_SET(v, 2, scm_from_double(w));
      SCM_SIMPLE_VECTOR_SET(v, 3, scm_from_double(h));
      return v;
      }

SCM qpoint_to_scm (QPointF *qp)
      {
      qreal x = qp->x(), y = qp->y();
      SCM v = scm_c_make_vector(2, SCM_EOL);
      SCM_SIMPLE_VECTOR_SET(v, 0, scm_from_double(x));
      SCM_SIMPLE_VECTOR_SET(v, 1, scm_from_double(y));
      return v;
      }

SCM ms_element_pagePos (SCM elm_scm)
      {
      void *elm_ptr = scm_to_pointer(elm_scm);
      Element* elm = (Element*) elm_ptr;
      QPointF qp = elm->pagePos();
      return qpoint_to_scm(&qp);
      }

SCM ms_element_canvasPos (SCM elm_scm)
      {
      void *elm_ptr = scm_to_pointer(elm_scm);
      Element* elm = (Element*) elm_ptr;
      QPointF qp = elm->canvasPos();
      return qpoint_to_scm(&qp);
      }

SCM ms_qlist_size (SCM ql_scm)
      {
      void *ql_ptr = scm_to_pointer(ql_scm);
      QList<void*> *ql = (QList<void*> *) ql_ptr;
      return scm_from_int(ql->size());
      }

SCM ms_qlist_at (SCM ql_scm, SCM idx_scm)
      {
      void *ql_ptr = scm_to_pointer(ql_scm);
      int idx = scm_to_int(idx_scm);
      QList<void*> *ql = (QList<void*> *) ql_ptr;
      return scm_from_pointer(ql->at(idx), NULL);
      }

SCM ms_stdvec_size (SCM sv_scm)
      {
      void *sv_ptr = scm_to_pointer(sv_scm);
      std::vector<void*> *sv = (std::vector<void*> *) sv_ptr;
      return scm_from_int(sv->size());
      }

SCM ms_stdvec_at (SCM sv_scm, SCM idx_scm)
      {
      void *sv_ptr = scm_to_pointer(sv_scm);
      int idx = scm_to_int(idx_scm);
      std::vector<void*> *sv = (std::vector<void*> *) sv_ptr;
      return scm_from_pointer(sv->at(idx), NULL);
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
      // mtest functions
      scm_c_define_gsubr ("ms-test-read-score", 1, 0, 0, (void *)ms_test_read_score);
      // mscore/libmscore functions
      scm_c_define_gsubr ("ms-score-read-file", 1, 0, 0, (void *)ms_score_read_file);
      scm_c_define_gsubr ("ms-score-read-string", 1, 0, 0, (void *)ms_score_read_string);
      scm_c_define_gsubr ("ms-score-write-string", 1, 0, 0, (void *)ms_score_write_string);
      scm_c_define_gsubr ("ms-score-forget", 1, 0, 0, (void *)ms_score_forget);
      scm_c_define_gsubr ("ms-score-cmd", 2, 0, 0, (void *)ms_score_cmd);
      scm_c_define_gsubr ("ms-segment-elements", 1, 0, 0, (void *)ms_segment_elements);
      scm_c_define_gsubr ("ms-measure-elements", 1, 0, 0, (void *)ms_measure_elements);
      scm_c_define_gsubr ("ms-qrect-get", 1, 0, 0, (void *)ms_qrect_get);
      scm_c_define_gsubr ("ms-element-pagePos", 1, 0, 0, (void *)ms_element_pagePos);
      scm_c_define_gsubr ("ms-element-canvasPos", 1, 0, 0, (void *)ms_element_canvasPos);

      scm_c_define_gsubr ("ms-qlist-size", 1, 0, 0, (void *)ms_qlist_size);
      scm_c_define_gsubr ("ms-qlist-at", 2, 0, 0, (void *)ms_qlist_at);
      scm_c_define_gsubr ("ms-stdvec-size", 1, 0, 0, (void *)ms_stdvec_size);
      scm_c_define_gsubr ("ms-stdvec-at", 2, 0, 0, (void *)ms_stdvec_at);
      //
      scm_c_export ("ms-test-read-score", NULL);
      scm_c_export ("ms-score-read-file", NULL);
      scm_c_export ("ms-score-read-string", NULL);
      scm_c_export ("ms-score-write-string", NULL);
      scm_c_export ("ms-score-forget", NULL);
      scm_c_export ("ms-score-cmd", NULL);
      scm_c_export ("ms-segment-elements", NULL);
      scm_c_export ("ms-measure-elements", NULL);
      scm_c_export ("ms-element-pagePos", NULL);
      scm_c_export ("ms-element-canvasPos", NULL);
      scm_c_export ("ms-qrect-get", NULL);
      scm_c_export ("ms-qlist-size", NULL);
      scm_c_export ("ms-qlist-at", NULL);
      scm_c_export ("ms-stdvec-size", NULL);
      scm_c_export ("ms-stdvec-at", NULL);

      }

// first parameter is a closure, not used here
static void
guile_main (void *, int argc, char **argv)
      {
      init_guile_shim();
      scm_shell (argc, argv);
      }

QGuiApplication* my_createApplication(int &argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (!qstrcmp(argv[i], "-no-gui"))
            return new QGuiApplication(argc, argv);
    }
    return new QApplication(argc, argv);
}

QGuiApplication *my_qapp;

static void init_musescore () {
      int argc = 0;
      char **argv = nullptr;
      my_qapp = my_createApplication(argc, argv);
      g_sg = new ScriptGuile();
      qApp->processEvents();
      }

}


int main (int argc, char* argv[])
      {
      ScriptGuile::init_musescore();
      scm_boot_guile (argc, argv, ScriptGuile::guile_main, 0);
      return 0;
      }
