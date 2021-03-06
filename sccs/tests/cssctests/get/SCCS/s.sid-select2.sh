hV6,sum=05484
s 00001/00001/00053
d D 1.2 2015/06/03 00:06:44+0200 joerg 2 1
S s 45450
c ../common/test-common -> ../../common/test-common
e
s 00054/00000/00000
d D 1.1 2010/04/29 02:05:14+0200 joerg 1 0
S s 45311
c date and time created 10/04/29 02:05:14 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ed60df8
G p sccs/tests/cssctests/get/sid-select2.sh
t
T
I 1
#! /bin/sh 
# sid-select2.sh: Do we select the correct SIDs? 


# This tests for a specific bug:-
# [ Bug #110537 ] Invalid SID got in branch where Rtrunk greater than Rbranch

# Import common functions & definitions. 
D 2
. ../common/test-common 
E 2
I 2
. ../../common/test-common 
E 2
# Get a test file... 
s=s.tst 
p=p.tst 
e=tst 
remove $s $p $e


get_expect () { 
label=$1 ; shift 
sid_expected=$1 ; shift 
docommand $label "${vg_get} -g $*" 0 "$sid_expected\n" IGNORE 
} 

# Create the file and set up the test conditions.
docommand p1 "${admin} -n            $s"   0 IGNORE IGNORE
docommand p2 "${vg_get}   -e            $s"   0 IGNORE IGNORE
docommand p3 "${delta}          -y'' $s"   0 IGNORE IGNORE
docommand p4 "${vg_get}   -e -r2        $s"   0 IGNORE IGNORE
docommand p5 "${delta}          -y'' $s"   0 IGNORE IGNORE
docommand p6 "${vg_get}   -e -r1.2      $s"   0 IGNORE IGNORE
docommand p7 "${delta}          -y'' $s"   0 IGNORE IGNORE



# Do various forms of get on the file and make sure we get the right SID. 
get_expect Y1 1.1 -r1.1 $s 
get_expect Y2 1.2 -r1.2 $s 
get_expect Y3 2.1 -r2.1 $s 
get_expect Y4 1.2.1.1 -r1.2.1.1 $s 


# now check for bug... 
docommand Z1 "${vg_get} -e -r1.2.1.1 $s" 0 \
    "1.2.1.1\nnew delta 1.2.1.2\n0 lines\n" IGNORE 
docommand Z2 "${delta} -yNoComment $s" 0 IGNORE IGNORE 

docommand Z3 "${admin} -fb $s" 0 IGNORE IGNORE 
docommand Z4 "${vg_get} -e -b -r1.2.1.2 $s" 0 \
    "1.2.1.2\nnew delta 1.2.2.1\n0 lines\n" IGNORE 

remove $s 
remove $p 
remove $e 
success 
#end test case.... 
E 1
