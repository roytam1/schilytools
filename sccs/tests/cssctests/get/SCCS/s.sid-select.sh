hV6,sum=55116
s 00001/00001/00038
d D 1.3 2015/06/03 00:06:44+0200 joerg 3 2
S s 16100
c ../common/test-common -> ../../common/test-common
e
s 00001/00001/00038
d D 1.2 2015/06/01 23:55:23+0200 joerg 2 1
S s 15961
c ../../testutils/ -> ${SRCROOT}/tests/testutils/
e
s 00039/00000/00000
d D 1.1 2010/04/29 02:05:14+0200 joerg 1 0
S s 14742
c date and time created 10/04/29 02:05:14 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ecb49d8
G p sccs/tests/cssctests/get/sid-select.sh
t
T
I 1
#! /bin/sh
# sid-select.sh:  Do we select the correct SIDs?

# Import common functions & definitions.
D 3
. ../common/test-common
E 3
I 3
. ../../common/test-common
E 3


# Get a test file...
s=s.testfile
remove $s
D 2
../../testutils/uu_decode --decode < testfile.uue || 
E 2
I 2
${SRCROOT}/tests/testutils/uu_decode --decode < testfile.uue || 
E 2
    miscarry could not extract test file.

get_expect () {
label=$1         ; shift
sid_expected=$1  ; shift
docommand $label "${vg_get} -g $*" 0 "$sid_expected\n" IGNORE
}

# Do various forms of get on the file and make sure we get the right SID.
get_expect X1  1.1        -r1.1      $s
get_expect X2  1.2        -r1.2      $s
get_expect X3  1.3        -r1.3      $s
get_expect X4  1.4        -r1.4      $s
get_expect X5  1.5        -r1.5      $s
get_expect X6  1.5        -r1        $s
get_expect X7  1.3.1.1    -r1.3.1.1  $s
get_expect X8  1.3.1.1    -r1.3.1    $s
get_expect X9  2.1        -r2.1      $s
get_expect X10 2.1        -r2        $s
get_expect X11 2.1        ""         $s
get_expect X12 2.1        -r3        $s
get_expect X13 2.1        -r9000     $s

docommand F1 "${vg_get} -r3.1   s.testfile" 1 "" IGNORE
docommand F2 "${vg_get} -r3.1.1 s.testfile" 1 "" IGNORE

remove $s
success
E 1
