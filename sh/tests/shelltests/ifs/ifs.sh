#! /bin/sh
#
# @(#)ifs.sh	1.3 16/12/29 Copyright 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

cmd="set . .; printf '%s ' \\\$*; printf '%s ' \\\$@; printf '%s ' \\\"\\\$*\\\"; printf '%s ' \\\"\\\$@\\\""
docommand ifs1 "$SHELL -c \"$cmd\"" 0 ". . . . . . . . " ""

cmd="set . .; unset IFS; printf '%s ' \\\$*; printf '%s ' \\\$@; printf '%s ' \\\"\\\$*\\\"; printf '%s ' \\\"\\\$@\\\""
docommand ifs2 "$SHELL -c \"$cmd\"" 0 ". . . . . . . . " ""

cmd="set . .; IFS='x'; printf '%s ' \\\$*; printf '%s ' \\\$@; printf '%s ' \\\"\\\$*\\\"; printf '%s ' \\\"\\\$@\\\""
docommand ifs3 "$SHELL -c \"$cmd\"" 0 ". . . . .x. . . " ""

cmd="set . .; IFS=''; printf '%s ' \\\$*; printf '%s ' \\\$@; printf '%s ' \\\"\\\$*\\\"; printf '%s ' \\\"\\\$@\\\""
docommand ifs4 "$SHELL -c \"$cmd\"" 0 ". . . . .. . . " ""

cmd="set . .; IFS='\'; printf '%s ' \\\$*; printf '%s ' \\\$@; printf '%s ' \\\"\\\$*\\\"; printf '%s ' \\\"\\\$@\\\""
docommand ifs5 "$SHELL -c \"$cmd\"" 0 ". . . . .\. . . " ""

docommand ifs6 "$SHELL -c 'IFS=\"T \"; echo \$*' 0 1 2 3" 0 "1 2 3\n" ""
docommand ifs7 "$SHELL -c 'IFS=\"T \"; echo \"\$*\"' 0 1 2 3" 0 "1T2T3\n" ""

docommand ifs8 "$SHELL -c 'IFS=\"T \"; echo \$@' 0 1 2 3" 0 "1 2 3\n" ""
docommand ifs9 "$SHELL -c 'IFS=\"T \"; echo \"\$@\"' 0 1 2 3" 0 "1 2 3\n" ""

docommand ifs10 "$SHELL -c 'IFS=\"T \"; echo \${*}' 0 1 2 3" 0 "1 2 3\n" ""
docommand ifs11 "$SHELL -c 'IFS=\"T \"; echo \"\${*}\"' 0 1 2 3" 0 "1T2T3\n" ""

docommand ifs12 "$SHELL -c 'IFS=\"T \"; echo \${@}' 0 1 2 3" 0 "1 2 3\n" ""
docommand ifs13 "$SHELL -c 'IFS=\"T \"; echo \"\${@}\"' 0 1 2 3" 0 "1 2 3\n" ""

docommand ifs14 "$SHELL -c 'IFS=\"\"; echo \$*' 0 1 2 3" 0 "1 2 3\n" ""
docommand ifs15 "$SHELL -c 'IFS=\"\"; echo \"\$*\"' 0 1 2 3" 0 "123\n" ""

docommand ifs16 "$SHELL -c 'IFS=\"\"; echo \$@' 0 1 2 3" 0 "1 2 3\n" ""
docommand ifs17 "$SHELL -c 'IFS=\"\"; echo \"\$@\"' 0 1 2 3" 0 "1 2 3\n" ""

docommand ifs18 "$SHELL -c 'IFS=\"\"; echo \${*}' 0 1 2 3" 0 "1 2 3\n" ""
docommand ifs19 "$SHELL -c 'IFS=\"\"; echo \"\${*}\"' 0 1 2 3" 0 "123\n" ""

docommand ifs20 "$SHELL -c 'IFS=\"\"; echo \${@}' 0 1 2 3" 0 "1 2 3\n" ""
docommand ifs21 "$SHELL -c 'IFS=\"\"; echo \"\${@}\"' 0 1 2 3" 0 "1 2 3\n" ""

cat > x << \XEOF
null_ifs() {
	IFS=
	set -- "$@"
	printf "\$# -> %d\n" $#
} 
null_ifs 1 2 3
XEOF
docommand ifs22 "$SHELL x" 0 "\$# -> 3\n" ""

remove x
success
