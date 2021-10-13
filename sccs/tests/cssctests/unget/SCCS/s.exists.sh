hV6,sum=11609
s 00001/00001/00035
d D 1.2 2015/06/03 00:06:44+0200 joerg 2 1
S s 51821
c ../common/test-common -> ../../common/test-common
e
s 00036/00000/00000
d D 1.1 2010/05/16 00:06:03+0200 joerg 1 0
S s 51682
c date and time created 10/05/16 00:06:03 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ef1915a
G p sccs/tests/cssctests/unget/exists.sh
t
T
I 1
#! /bin/sh

# exists.sh:  What if the input file doesn't exist?

# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

g1=new1.txt
g2=new2.txt
s1=s.$g1
s2=s.$g2
p1=p.$g1
p2=p.$g2
all="$s1 $g1 $p1 $s2 $g2 $p2 xxx1 xxx2 old.$g1 old.$g2"

m=mfile
remove $all $m


echo "%M%" >$m || miscarry could not create $m.

rm -f $p1 || miscarry could not remove $p1
ln -s / $p1 || miscarry could not ln -s / $p1
docommand e1 "${vg_unget} -r1.2 $s1" 1 "IGNORE" "IGNORE" 
rm -f $p1


docommand e2 "${admin} -i $s1" 0 "" "" <$m
docommand e3 "${get} -e $s1" 0 IGNORE IGNORE
docommand e4 "${vg_unget} -r1.2 $s1" 0 "IGNORE" "IGNORE" 

###
### Cleanup and exit.
###
remove $all $m
success
E 1
