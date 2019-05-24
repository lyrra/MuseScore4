/* This file is seen by musescore */

#ifndef SCRIPTGUILE
#define SCRIPTGUILE

namespace ScriptGuile {

/* An object of this type, can
 * be used by musescore to move around
 * and get the state of the Guile/scheme
 * interpreter.
 */
class Guile {
    private:
    public:
    };

// Before doing anything guile, call guile_init
void guile_init ();

/* starts Guile/Scheme
 * if given an argument, it is
 * an scheme-file that will be
 * loaded an evaluated.
 */
Guile start ();
void  start (char *filename);
long  start_func  (void *(*func)(void*));
void  start_shell (int argc, char **argv);

} // namespace ScriptGuile

#endif // SCRIPTGUILE
