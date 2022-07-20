#include <cstdlib>
#include <cstdio>
#include "s7/s7.h"
#include "s7/utils.h"

#include <QtTest/QtTest>
#include "libmscore/score.h"
#include "libmscore/element.h"
#include "libmscore/chord.h"
#include "mtest/testutils.h"
using namespace Ms;
#include "s7gen.h"


extern Ms::MTest* g_mtest;
extern int g_test_check_pass;
extern int g_test_check_fail;

s7_pointer ms_test_check (s7_scheme *sc, s7_pointer args)
{
    s7_pointer s = s7_car(args);
    if (s7_is_boolean(s)) {
        if (s7_boolean(sc, s)) {
            g_test_check_pass++;
        } else {
            g_test_check_fail++;
        }
    } else {
        g_test_check_fail++;
    }
    return s7_nil(sc);
}

s7_pointer ms_test_check_pass (s7_scheme *sc, s7_pointer args)
{
    g_test_check_pass++;
    return s7_nil(sc);
}

s7_pointer ms_test_check_fail (s7_scheme *sc, s7_pointer args)
{
    g_test_check_fail++;
    return s7_nil(sc);
}

s7_pointer ms_element_type (s7_scheme *sc, s7_pointer args)
{
    s7_pointer s = s7_car(args);
    if (! c_is_goo(sc, s)) {
        return s7_nil(sc);
    }
    goo_t *g = (goo_t *)s7_c_object_value(s);
    Element* e = (Element*) g->cd;
    return(s7_make_integer(sc, static_cast<uint64_t>(e->type())));
}

s7_pointer ms_mtest_writeReadElement(s7_scheme *sc, s7_pointer args)
{
    s7_pointer s = s7_car(args);
    if (! c_is_goo(sc, s)) {
        return s7_nil(sc);
    }
    goo_t *g = (goo_t *)s7_c_object_value(s);
    Element* e = (Element*) g->cd;
    return c_make_goo(sc, g->ty, g->data, g_mtest->writeReadElement(e));
}

s7_pointer ms_make_chord (s7_scheme *sc, s7_pointer args)
{
    return c_make_goo(sc,
                      static_cast<uint64_t>(GOO_TYPE::CHORD),
                      s7_nil(sc),
                      new Ms::Chord(g_mtest->score));
}

s7_pointer ms_make_note (s7_scheme *sc, s7_pointer args)
{
    return c_make_goo(sc,
                      static_cast<uint64_t>(GOO_TYPE::NOTE),
                      s7_nil(sc),
                      new Ms::Note(g_mtest->score));
}

s7_pointer ms_chord_add_note (s7_scheme *sc, s7_pointer args)
{
    s7_pointer chord_s = s7_car(args);
    if (! c_is_goo(sc, chord_s)) {
        return s7_nil(sc);
    }
    s7_pointer note_s = s7_cadr(args);
    if (! c_is_goo(sc, note_s)) {
        return s7_nil(sc);
    }
    goo_t *chord_g = (goo_t *)s7_c_object_value(chord_s);
    goo_t *note_g  = (goo_t *)s7_c_object_value(note_s);
    Ms::Chord* chord = (Ms::Chord*) chord_g->cd;
    Ms::Note* note = (Ms::Note*) note_g->cd;
    chord->add(note);
}

static s7_pointer ms_note_set_tpc_from_pitch (s7_scheme *sc, s7_pointer args)
{
    goo_t *g = (goo_t *)s7_c_object_value(s7_car(args));
    Ms::Note* o = (Ms::Note*) g->cd;
    o->setTpcFromPitch();
    return s7_nil(sc);
}

s7_pointer ms_note_usermirror (s7_scheme *sc, s7_pointer args)
{
    goo_t *g = (goo_t *)s7_c_object_value(s7_car(args));
    Ms::Note* o = (Ms::Note*) g->cd;
    return s7_make_symbol(sc, usermirror_to_string(o->userMirror()));
}

s7_pointer ms_set_note_usermirror (s7_scheme *sc, s7_pointer args)
{
    goo_t *g = (goo_t *)s7_c_object_value(s7_car(args));
    Ms::Note* o = (Ms::Note*) g->cd;
    s7_pointer sym = s7_cadr(args);
    const char *symname = s7_symbol_name(sym);
    o->setUserMirror(string_to_usermirror(symname));
    return sym;
}

void mtest_s7_define_functions(s7_scheme *sc) {
    init_goo(sc);
    init_gen_s7(sc);
    s7_define_function(sc, "ms-test-check", ms_test_check, 1, 0, false, "(ms-test-check cond)");
    s7_define_function(sc, "ms-test-check-pass", ms_test_check_pass, 0, 0, false, "(ms-test-check-pass)");
    s7_define_function(sc, "ms-test-check-fail", ms_test_check_fail, 0, 0, false, "(ms-test-check-fail)");
    s7_define_function(sc, "ms-test-writeReadElement", ms_mtest_writeReadElement, 1, 0, false, "(ms-mtest-writeReadElement Element)");
    s7_define_function(sc, "ms-make-element", ms_make_element, 1, 0, false, "(ms-make-element element-typename_sym)");
    s7_define_function(sc, "ms-make-chord", ms_make_chord, 0, 0, false, "(ms-make-chord)");
    s7_define_function(sc, "ms-make-note", ms_make_note, 0, 0, false, "(ms-make-note)");
    s7_define_function(sc, "ms-element-type", ms_element_type, 1, 0, false, "(ms-element-type element)");
    s7_define_function(sc, "ms-chord-add-note", ms_chord_add_note, 2, 0, false, "(ms-chord-add-note)");

    s7_define_function(sc, "ms-note-set-tpc-from-pitch", ms_note_set_tpc_from_pitch, 1, 0, false, "(ms-note-set-tpc-from-pitch)");

}

int run_scheme_script(const char *filename)
{
    s7_scheme *s7;
    s7 = s7_init();
    mtest_s7_define_functions(s7);
    if (!s7_load(s7, filename)) {
        fprintf(stderr, "can't find %s\n", filename);
    }
    free(s7);
    return 0;
}
