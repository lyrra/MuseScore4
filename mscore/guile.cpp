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

static char *scheme_filename = NULL;
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

    if(scheme_filename){
        std::cerr << "Guile load primitive file " << scheme_filename << std::endl;
        scm_c_primitive_load(scheme_filename);
        }
    else{
        std::cerr << "Guile entering REPL" << std::endl;
        scm_shell (argc, argv);
        }
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

Guile start (char *filename)
{
    Guile g;
    scheme_filename = filename;
    std::cerr << "Starting Guile thread, will load file " << filename << std::endl;
    guileThread = std::thread(&funcGuileThread);

    return g;
    }

} // Eof namescape ScriptGuile
