//

#include <thread>
#include <iostream>
#include <libguile.h>

#include "libmscore/mscore.h"
#include "libmscore/part.h"

#include "musescore.h"
#include "scoreview.h"
#include "scoretab.h"

#include "guile-glue.h"
#include "guile.h"

namespace ScriptGuile {

static std::thread guileThread;

//
// support functions
//

// returns last cons in a single-listed list
// where data has been appended to
SCM s_push (SCM last, SCM data)
{
      if (last == SCM_EOL) {
            return scm_cons(data, SCM_EOL);
            }
      else {
            SCM cons = scm_cons(data, SCM_EOL);
            SCM_CDR(last) = cons; // extend chain with a new link
            return cons;
            }
      }

SCM scm_false_or_string (const char *s)
{
    if (s == NULL)
        return SCM_BOOL_F;
    else
        return scm_from_locale_string (s);
    }

//
// setup and launch GNU/Guile
//

// first parameter is a closure, not used here
static void
guile_main (void *, int argc, char **argv)
{
    std::cerr << "guile main started" << std::endl;

    init_guile_musescore_functions();

    std::cerr << "Guile entering REPL" << std::endl;
    scm_shell (argc, argv);
    }

static void*
guile_main2 (void *)
    {
    guile_main(NULL, 0, NULL);
    }

static void *
guile_run_script (void *file)
{
    char *filename = (char *) file;
    init_guile_musescore_functions();
    if(filename){
        std::cerr << "Guile load primitive file " << filename << std::endl;
        scm_c_primitive_load(filename);
        }
    else{
        std::cerr << "ERROR: no Guile/Scheme script file given" << std::endl;
        }
    return NULL;
    }

static void funcGuileThread()
{
    std::cerr << "funcGuileThread started" << std::endl;
    scm_boot_guile (0, nullptr, guile_main, 0);
    }

void start_shell (int argc, char **argv)
{
    scm_with_guile(guile_main2, NULL);
    }

Guile start ()
{
    Guile g;
    guileThread = std::thread(&funcGuileThread);
    return g;
    }

void start (char *filename)
{
    std::cerr << "Running Guile/Scheme script: " << filename << std::endl;
    scm_with_guile (guile_run_script, (void *)filename);
    return;
    }

long start_func (void *(*func)(void *))
{
    std::cerr << "Running function under Guile/Scheme." << std::endl;
    return (long) scm_with_guile (func, NULL);
    }

} // Eof namescape ScriptGuile
