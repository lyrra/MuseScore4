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

/* starts Guile/Scheme
 * if given an argument, it is
 * an scheme-file that will be
 * loaded an evaluated.
 */
Guile start ();
Guile start (char* filename);

} // namespace ScriptGuile

#endif // SCRIPTGUILE
