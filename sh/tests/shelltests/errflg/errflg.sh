#! /bin/sh
#
# @(#)errflg.sh	1.8 20/04/23 Copyright 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# This causes bash-3.x to fail as bash-3.x does not handle sh -e correctly.
#
docommand ef0 "$SHELL -ce 'for i in 1 2 3; do  ( echo \$i; if test -d . ; then (false; echo 4);  fi ) ; done'" "!=0" "1\n" ""
docommand ef0a "$SHELL -c 'set -e; for i in 1 2 3; do  ( echo \$i; if test -d . ; then (false; echo 4);  fi ) ; done'" "!=0" "1\n" ""

#
# POSIX: Check whether a I/O redirection error with a builtin or function stops the whole shell
#
docommand ef1 "$SHELL -c 'echo < nonexistant| echo OK'" 0 "OK\n" IGNORE
docommand ef2 "$SHELL -c 'x() { ls; }; x < nonexistant| echo OK'" 0 "OK\n" IGNORE

#
# false is now a builtin, check whether builtins work as expected
#
docommand ef3  "$SHELL -ce 'false && false; echo OK'" 0 "OK\n" ""
docommand ef4  "$SHELL -ce 'false || false; echo OK'" "!=0" "" ""
docommand ef5  "$SHELL -ce 'false; echo OK'" "!=0" "" ""
docommand ef3a  "$SHELL -c 'set -e; false && false; echo OK'" 0 "OK\n" ""
docommand ef4a  "$SHELL -c 'set -e; false || false; echo OK'" "!=0" "" ""
docommand ef5a  "$SHELL -c 'set -e; false; echo OK'" "!=0" "" ""

docommand ef6  "$SHELL -ce 'f() { false; }; while f; do :; done; echo OK'" 0 "OK\n" ""
docommand ef7  "$SHELL -ce 'f() { return \$1; }; f 42 || echo OK'" 0 "OK\n" ""
docommand ef8  "$SHELL -ce 'f() { return \$1; }; if f 42; then : ; fi; echo OK'" 0 "OK\n" ""
docommand ef9  "$SHELL -ce 'f() { return \$1; }; while f 42; do : ; done; echo OK'" 0 "OK\n" ""
docommand ef10 "$SHELL -ce 'f() { return \$1; }; { f 42; echo OK1; } && echo OK2' " 0 "OK1\nOK2\n" ""
docommand ef11 "$SHELL -ce 'f() { return \$1; }; { f 42; echo OK1; f 42; } && : ; echo OK2' " 0 "OK1\nOK2\n" ""
docommand ef12 "$SHELL -ce 'f() { echo OK1; return \$1; }; f 42 || echo OK2'" 0 "OK1\nOK2\n" ""
docommand ef13 "$SHELL -ce 'f() { return \$1; }; { f 42; echo OK1; f 42; } && : ; echo OK2'" 0 "OK1\nOK2\n" ""

docommand ef6a  "$SHELL -c 'set -e; f() { false; }; while f; do :; done; echo OK'" 0 "OK\n" ""
docommand ef7a  "$SHELL -c 'set -e; f() { return \$1; }; f 42 || echo OK'" 0 "OK\n" ""
docommand ef8a  "$SHELL -c 'set -e; f() { return \$1; }; if f 42; then : ; fi; echo OK'" 0 "OK\n" ""
docommand ef9a  "$SHELL -c 'set -e; f() { return \$1; }; while f 42; do : ; done; echo OK'" 0 "OK\n" ""
docommand ef10a "$SHELL -c 'set -e; f() { return \$1; }; { f 42; echo OK1; } && echo OK2' " 0 "OK1\nOK2\n" ""
docommand ef11a "$SHELL -c 'set -e; f() { return \$1; }; { f 42; echo OK1; f 42; } && : ; echo OK2' " 0 "OK1\nOK2\n" ""
docommand ef12a "$SHELL -c 'set -e; f() { echo OK1; return \$1; }; f 42 || echo OK2'" 0 "OK1\nOK2\n" ""
docommand ef13a "$SHELL -c 'set -e; f() { return \$1; }; { f 42; echo OK1; f 42; } && : ; echo OK2'" 0 "OK1\nOK2\n" ""

docommand ef14 "$SHELL -c 'set -e ; f() { return 1; echo err; }; f; echo OK'" 1 "" ""

#
# eval makes builtins a bit more complex
#
docommand ef20 "$SHELL -ce 'f() { return \$1; }; eval f 42 || echo OK'" 0 "OK\n" ""
docommand ef20a "$SHELL -c 'set -e; f() { return \$1; }; eval f 42 || echo OK'" 0 "OK\n" ""

#
# The exit code of a command substitution becomes the total exit code
# if there is no following command. So errexit is propagated (see ef32).
#
docommand ef30 "$SHELL -c 'a=\`false; echo 1\`; echo x \$a'" 0 "x 1\n" ""
docommand ef31 "$SHELL -c 'a=\`false; echo 1\` : ; echo x \$a'" 0 "x 1\n" ""
docommand ef32 "$SHELL -ce 'a=\`false; echo 1\`; echo x \$a'" "!=0" "" ""
docommand ef33 "$SHELL -ce 'a=\`false; echo 1\` : ; echo x \$a'" 0 "x\n" ""
docommand ef34 "$SHELL -c 'set -e; a=\`false; echo 1\`; echo x \$a'" "!=0" "" ""
docommand ef35 "$SHELL -c 'set -e; a=\`false; echo 1\` : ; echo x \$a'" 0 "x\n" ""

#
# This is an example from the POSIX standard
#
docommand ef50 "$SHELL -c 'set -e; (false; echo one) | cat; echo two'" 0 "two\n" ""

#
# The "errflg" in variable "flags" is sometimes partially cleared
# and later restored. Verify that we also use the "eflag" variable
# to build $-
#
docommand ef60 "$SHELL -c 'foo() { echo \$-; set -eu; echo \$-; set --; echo \$-; }; foo; echo \$-' arg" 0 "\neu\neu\neu\n" ""

#
# Check whether functions are not terminated in the middle wile the return code is used.
#
docommand ef70 "$SHELL -ce 'foo() { echo 1; false; echo 2; }; foo'" '!=0' "1\n" ""
docommand ef71 "$SHELL -c 'set -e; foo() { echo 1; false; echo 2; }; foo'" '!=0' "1\n" ""
docommand ef72 "$SHELL -ce 'foo() { echo 1; false; echo 2; }; if foo; then :; fi'" 0 "1\n2\n" ""
docommand ef73 "$SHELL -c 'set -e; foo() { echo 1; false; echo 2; }; if foo; then :; fi'" 0 "1\n2\n" ""

success
