hV6,sum=11936
s 00001/00001/00025
d D 1.3 2015/06/03 00:06:44+0200 joerg 3 2
S s 35546
c ../common/test-common -> ../../common/test-common
e
s 00001/00001/00025
d D 1.2 2015/06/01 23:55:23+0200 joerg 2 1
S s 35407
c ../../testutils/ -> ${SRCROOT}/tests/testutils/
e
s 00026/00000/00000
d D 1.1 2010/05/03 03:11:28+0200 joerg 1 0
S s 34188
c date and time created 10/05/03 03:11:28 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ef007d8
G p sccs/tests/cssctests/sact/sf513800.sh
t
T
I 1
#! /bin/sh

# sf513800.sh:  Tests relating to SOurceForge bug 513800

# Import common functions & definitions.
D 3
. ../common/test-common
E 3
I 3
. ../../common/test-common
E 3

g=foo
s=s.$g
p=p.$g

remove $s $p $g

# Extract the test input files. 
for n in 1 2 
do
    filename=sf513800_${n}.uue
D 2
    ../../testutils/uu_decode --decode < $filename || miscarry could not uudecode $filename
E 2
I 2
    ${SRCROOT}/tests/testutils/uu_decode --decode < $filename || miscarry could not uudecode $filename
E 2
done

docommand s1 "${vg_sact} $s" 0 IGNORE IGNORE 



remove $s $p $g
success
E 1
