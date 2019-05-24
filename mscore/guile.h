/* This file is not seen by musescore
 * and has Guile types and internal stuff
 */

namespace ScriptGuile {

/* Register all musescore shim functions,
 * this will make them reachable from
 * scheme. To see what musescore/scheme
 * function are available, look at this
 * function.
 */
void init_guile_musescore_functions ();

/* push, will make a single-linked-list */
SCM s_push (SCM last, SCM data);

/* In scheme it is proper to return data
 * of different types, this is such a helper
 * function.
 */
SCM scm_false_or_string (const char *s);

} // namespace ScriptGuile
