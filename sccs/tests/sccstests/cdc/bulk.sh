#! /bin/sh

# Basic tests for SCCS flag -N

# Read test core functions
. ../../common/test-common

g=foo
s=s.$g
p=p.$g
z=z.$g
g2=foo2
s2=s.$g2
p2=p.$g2
z2=z.$g2

remove $z $s $p $g $z2 $s2 $p2 $g2 XXXX a

echo '%M%' > $g
echo '%F%' > $g2

docommand cd_bulk1 "${admin} -N-XXXX -i. $g $g2" 0 "" ""

docommand cd_bulk2 "${cdc} -NXXXX -r1.1 -ynew $g $g2" 0 IGNORE ""
docommand cd_bulk3 "${cdc} -N     -r1.1 -ynew XXXX/$g XXXX/$g2" 0 IGNORE ""
docommand cd_bulk4 "${cdc} -Ns.   -r1.1 -ynew XXXX/$s XXXX/$s2" 0 IGNORE ""
docommand cd_bulk5 "${cdc} -NXXXX/s. -r1.1 -ynew  XXXX/$s XXXX/$s2" 0 IGNORE ""

docommand cd_bulk11 "${admin} -NXXXX -fy -n a/b/c/$g a/b/c/$g2" 0 "" ""

docommand cd_bulk12 "${cdc} -NXXXX -r1.1 -ynew a/b/c/$g a/b/c/$g2" 0 IGNORE ""
docommand cd_bulk13 "${cdc} -N     -r1.1 -ynew a/b/c/XXXX/$g a/b/c/XXXX/$g2" 0 IGNORE ""
docommand cd_bulk14 "${cdc} -Ns.   -r1.1 -ynew a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""
docommand cd_bulk15 "${cdc} -NXXXX/s. -r1.1 -ynew a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""

remove $z $s $p $g $z2 $s2 $p2 $g2 XXXX a
success
