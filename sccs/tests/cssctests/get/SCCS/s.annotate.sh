hV6,sum=09624
s 00001/00001/00058
d D 1.2 2015/06/03 00:06:44+0200 joerg 2 1
S s 49786
c ../common/test-common -> ../../common/test-common
e
s 00059/00000/00000
d D 1.1 2010/04/29 02:05:14+0200 joerg 1 0
S s 49647
c date and time created 10/04/29 02:05:14 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ed00bf7
G p sccs/tests/cssctests/get/annotate.sh
t
T
I 1
#! /bin/sh

# Tests for the -n and -m options of get.

# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2


f=1test
s=s.$f
p=p.$f
remove $f $s $p

echo "line1" >  $f
echo "line2" >> $f

docommand A1 "$admin -n -i$f $s" 0 "" IGNORE
test -r $s         || fail admin could not create $s

remove $f 

# Test the -n (annotate module name) option
docommand N1 "${vg_get} -p -n $s" 0 "$f\tline1\n$f\tline2\n" IGNORE

# Test the -m (annotate SID) option
docommand N2 "${vg_get} -p -m $s" 0 "1.1\tline1\n1.1\tline2\n" IGNORE

# Test both options together.
docommand N3 "${vg_get} -p -n -m $s" 0 "$f\t1.1\tline1\n$f\t1.1\tline2\n" IGNORE

# Make a new delta to further test the -m option.
docommand G1 "${vg_get} -e $s" 0 "1.1\nnew delta 1.2\n2 lines\n" ""

echo "line3" >> $f
docommand D1 "$delta '-yAdded line three' $s" 0 \
    "1.2\n1 inserted\n0 deleted\n2 unchanged\n" \
    IGNORE

# Test the -m (annotate SID) option with several deltas...
docommand N4 "${vg_get} -p -m $s" 0 \
    "1.1\tline1\n1.1\tline2\n1.2\tline3\n" \
    IGNORE

# Now make a branch.
docommand G2 "${vg_get} -e -r1.1 $s" 0 "1.1\nnew delta 1.1.1.1\n2 lines\n" ""

echo "line4 %Z%" >> $f
docommand D2 "$delta '-yAdded line: a branch' $s" 0 \
    "1.1.1.1\n1 inserted\n0 deleted\n2 unchanged\n" \
    IGNORE

# Test both options together.
docommand N5 "${vg_get} -p -m -n -r1.1.1.1 $s" 0 \
    "$f\t1.1\tline1\n$f\t1.1\tline2\n$f\t1.1.1.1\tline4 @(#)\n" \
    IGNORE

remove command.log
remove $f $s $p
success
E 1
