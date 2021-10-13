#! /bin/sh
# sf865422.sh: tests for SourceForge bug number 865522

#
# This bug occurred if you did rmdel on a delta and then get -e to use, it
# and then (with the joint edit flag turned on) did get-e again.  The 
# code prior to the first fix didn;t notice that the p-file indicated that 
# the relevant delta was already in use.

# Import common functions & definitions.
. ../../common/test-common


g=foo.txt
s=s.$g
x=x.$g 
z=z.$g
p=p.$g


remove $g $s $x $z $p
touch $g

echo one > $g

docommand E1 "${admin} -i$g -fb -fj $s" 0 IGNORE IGNORE
docommand E2 "${vg_get} -e -g -r1.1 $s"    0 IGNORE IGNORE
docommand E3 "${delta} -n -yok $s"      0 IGNORE IGNORE
docommand E4 "${vg_get} -e -g -r1.2 $s"    0 IGNORE IGNORE
docommand E5 "${delta} -n -yok $s"      0 IGNORE IGNORE
docommand E6 "${rmdel} -r1.3 $s"        0 IGNORE IGNORE

# The two get commands under consideration follow.  The second
# of the two is the one that would fail.
docommand E7 "${vg_get} -e -g -r1.2 $s"    0 IGNORE IGNORE
docommand E8 "${vg_get} -e -g -r1.2 $s"    0 IGNORE IGNORE

# Check the pfile - make sure both edits are listed.
docommand E9  "${sact} $s | grep '^1\.2 1\.3 '"      0 IGNORE IGNORE
docommand E10 "${sact} $s | grep '^1\.2 1\.2.1.1 '"  0 IGNORE IGNORE

remove $g $s $x $z $p
success
