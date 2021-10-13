#! /bin/sh
# errorcases.sh:  Testing for various error cases
#

# Import common functions & definitions.
. ../../common/test-common


ret_invalid_option=1

remove 


g=foo
s=s.$g
p=p.$g
z=z.$g
q=q.$g
d=d.$g
x=x.$g
files="$g $s $p $z ${g}_1 ${g}_2 $g $q $d $x command.log log log.stdout log.stderr"
remove $files


# Create the input files.
echo foo > $g

docommand de1 "${admin} -n -i$g $s" 0 IGNORE IGNORE
remove $g
docommand de2 "${vg_get} -s -p $s" 0 "foo\n" IGNORE
docommand de3 "${vg_get} -s -p -r1.1 $s" 0 "foo\n" IGNORE

# Attempt to get a nonexistent SID should fail. 
docommand de4 "${vg_get} -r1.2 $s" 1 "" IGNORE

# Attempt to get an invalid SID should fail (we try several)
docommand de5 "${vg_get}  -r2a $s"  ${ret_invalid_option} "" IGNORE
docommand de6 "${vg_get}  -r2_3 $s" ${ret_invalid_option} "" IGNORE

# Make a branch for later use
docommand de7 "${vg_get} -e $s" 0 "1.1\nnew delta 1.2\n1 lines\n" IGNORE
docommand de8 "${delta} -yNoComment $s" 0 IGNORE IGNORE
docommand de9 "${vg_get} -e -r1.1 $s" 0 "1.1\nnew delta 1.1.1.1\n1 lines\n" IGNORE
docommand de10 "${delta} -yNoComment $s" 0 IGNORE IGNORE

# Now get 1.1.1.1 but including the change for 1.2.
docommand de11 "${vg_get} -r1.1.1.1 -i1.2 $s" 0 "Included:
1.2
1.1.1.1\n1 lines
" IGNORE

# The next is trhe case we really want to test - trying to include an invalid
# SID.  We try several ways. 
docommand de12 "${vg_get} -r1.1.1.1 -ia1.2a $s"      ${ret_invalid_option} IGNORE IGNORE
docommand de13 "${vg_get} -r1.1.1.1 -i.1   $s"      ${ret_invalid_option} IGNORE IGNORE
docommand de14 "${vg_get} -r1.1.1.1 -i1.1.1.1.1 $s" ${ret_invalid_option} IGNORE IGNORE

# Now trying to exclude an invalid SID.  We try several ways. 
docommand de15 "${vg_get} -r1.1.1.1 -x1.2a $s"      ${ret_invalid_option} IGNORE IGNORE
docommand de16 "${vg_get} -r1.1.1.1 -x.1   $s"      ${ret_invalid_option} IGNORE IGNORE
docommand de17 "${vg_get} -r1.1.1.1 -x1.1.1.1.1 $s" ${ret_invalid_option} IGNORE IGNORE


remove ${files}
success
