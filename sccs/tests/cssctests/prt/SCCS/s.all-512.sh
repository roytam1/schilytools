hV6,sum=44943
s 00002/00002/00024
d D 1.3 2015/06/03 00:06:44+0200 joerg 3 2
S s 58420
c ../common/test-common -> ../../common/test-common
e
s 00002/00002/00024
d D 1.2 2015/06/01 23:55:23+0200 joerg 2 1
S s 58142
c ../../testutils/ -> ${SRCROOT}/tests/testutils/
e
s 00026/00000/00000
d D 1.1 2010/05/11 11:30:00+0200 joerg 1 0
S s 55704
c date and time created 10/05/11 11:30:00 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ee8b9ec
G p sccs/tests/cssctests/prt/all-512.sh
t
T
I 1
#! /bin/sh
D 3
. ../common/test-common
. ../common/need-prt
E 3
I 3
. ../../common/test-common
. ../../common/need-prt
E 3
export prt


remove s.testfile2
D 2
../../testutils/uu_decode --decode < s.testfile2.uue || miscarry could not uudecode testfile2.uue.
E 2
I 2
${SRCROOT}/tests/testutils/uu_decode --decode < s.testfile2.uue || miscarry could not uudecode testfile2.uue.
E 2

sh all-variations.txt 2>&1 >got.stdout | 
    grep -v "feature not fully tested: excluded delta"

remove all.expected

D 2
/bin/sh ../../testutils/decompress_stdin.sh <all.expected.Z >all.expected \
E 2
I 2
/bin/sh ${SRCROOT}/tests/testutils/decompress_stdin.sh <all.expected.Z >all.expected \
E 2
    || miscarry could not decompress expected output 

if diff all.expected got.stdout >/dev/null 
then
    remove all.expected s.testfile2
    success
else
    echo "output differs --"
    diff -c all.expected got.stdout | head -30
    fail output differs
fi
E 1
