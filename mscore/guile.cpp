//

#include <iostream>
#include <libguile.h>
#include "guile.h"

#include "scoretab.h"
#include "musescore.h"

namespace ScriptGuile {

static std::thread guileThread;

/***************************************************************/
// Simple extension example from
// https://www.gnu.org/software/guile/manual/html_node/Writing-Guile-Extensions.html#Writing-Gile-Extensions
double j0 (double x){
  // take the argument x that comes from scheme side,
  // feed it to some musescore function and get the result
  // convert the result back to a double and return it
  std::cerr << "guile: j0" << std::endl;
  return x + 1.0;
}

SCM
j0_wrapper (SCM x)
{
  return scm_from_double (j0 (scm_to_double (x)));
}

static SCM
my_hostname (void)
{
  char *s = getenv ("HOSTNAME");
  fprintf(stderr, "hostname: %s\n", s);
  if (s == NULL)
    return SCM_BOOL_F;
  else
    return scm_from_locale_string (s);
}

// End of Simple extension example
/***************************************************************/
// support functions
static inline SCM scm_false_or_string (const char *s)
{
    if (s == NULL)
        return SCM_BOOL_F;
    else
        return scm_from_locale_string (s);
    }

/***************************************************************/
// core musescore function, callable from scheme
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

// MasterScores

static SCM
ms_scores_count (void)
{
    int c = Ms::mscore->currentScoreTab()->count();
    return scm_from_int (c);
    }

// current score
//    Ms::mscore->currentScore()

// synti
// MasterSynthesizer* synti;

/***************************************************************/

static void
guile_main (void *closure, int argc, char **argv)
{
    std::cerr << "guile main started" << std::endl;
    scm_c_define_gsubr ("j0", 1, 0, 0, (void *)j0_wrapper);
    scm_c_define_gsubr ("my-hostname", 0, 0, 0, (void *)my_hostname);
    scm_c_define_gsubr ("ms-core-name", 0, 0, 0, (void *)core_appname);
    scm_c_define_gsubr ("ms-core-version", 0, 0, 0, (void *)core_appversion);
    scm_c_define_gsubr ("ms-core-experimental", 0, 0, 0, (void *)core_experimental);
    scm_c_define_gsubr ("ms-scores-count", 0, 0, 0, (void *)ms_scores_count);
    scm_shell (argc, argv);
    }

static void funcGuileThread()
{
    std::cerr << "funcGuileThread started" << std::endl;
    scm_boot_guile (0, nullptr, guile_main, 0);
    }

Guile start ()
{
    Guile g;
    guileThread = std::thread(&funcGuileThread);

    return g;
    }

} // Namespace ScriptGuile

