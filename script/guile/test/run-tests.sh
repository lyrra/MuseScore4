# This script is supposed to run in the top-level Musescore source directory

# First build your musescore executable with debugging
# make debug VERBOSE=1

# Possible variations scheme scripts can be executed:
# * --no-gui flag on/off
# * Pass a scheme-script flag --script-scheme=myscript.scm
# * Pass a scheme-shell flag --script-scheme-shell
#   ** and if shell:  pass scheme-script to shell via STDIN

#build.debug/mscore/mscore --no-gui --script-scheme-shell demos/Fugue_1.mscx demos/Unclaimed_Gift.mscx demos/Reunion.mscz < script/guile/test/run-tests.scm
build.debug/mscore/mscore --no-gui demos/Fugue_1.mscx demos/Unclaimed_Gift.mscx demos/Reunion.mscz --script-scheme=script/guile/test/run-tests.scm

