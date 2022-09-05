//

#include <thread>
#include <iostream>
#include <libguile.h>

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
    std::cerr << "Guile entering REPL" << std::endl;
    scm_shell (argc, argv);
    }

static void*
guile_main2 (void *)
    {
    guile_main(NULL, 0, NULL);
    return NULL;
    }

static void *
guile_run_script (void *file)
{
    char *filename = (char *) file;
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

void start_shell (int, char **) // argc, argv
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
    return (uint64_t) scm_with_guile (func, NULL);
    }

// This function is needed to make guile initialize
// in a thread-safe manner, see:
// https://lists.gnu.org/archive/html/guile-user/2014-01/msg00018.html
void * guile_init_workout (void *)
{
    /* Exercise guile with some arbitrary work
     * this will initialize module system, here and now where
     * we are running single threaded and initialization is not
     * thread safe.
     * In this case we happen to do something useful, create a
     * module where all Musescore-C bindings will go
     */
    scm_c_eval_string ("(define-module (musescore-c))");
    init_guile_musescore_functions(); // load all scheme-to-c/musescore functions
    SCM_TICK; // Checks any pending GC
    return NULL;
    }

void guile_init ()
{
    scm_with_guile(guile_init_workout, NULL);
    }

} // Eof namescape ScriptGuile
