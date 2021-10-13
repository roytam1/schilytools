h52122
s 00009/00002/00044
d D 1.3 16/08/14 23:16:32 joerg 3 2
c tail +2 -> tail $plustwo und $plustwo wird automatish angepasst
e
s 00001/00001/00045
d D 1.2 15/06/03 00:06:45 joerg 2 1
c ../common/test-common -> ../../common/test-common
e
s 00046/00000/00000
d D 1.1 11/05/29 21:09:29 joerg 1 0
c date and time created 11/05/29 21:09:29 by joerg
e
u
U
f e 0
t
T
I 1
#! /bin/sh

# Basic tests for degenerated SCCS comment

# Read test core functions
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

g=foo
s=s.$g
p=p.$g
z=z.$g

remove $z $s $p $g

#
# Checking whether SCCS supports to hide extensions in degentrated comments
# that do not look like '^Ac comment'
#
cp s.comment $s
expect_fail=true
docommand cm1 "${val} $s" 0 "" ""
remove $s

I 3
echo test | tail +2 > /dev/null 2>/dev/null
if [ "$?" -eq 0 ]; then
	plustwo=+2
else
	plustwo='-n +2'
fi

E 3
cp s.comment $s
docommand cm2d "${admin} -db $s" 0 "" ""
if diff s.comment $s > /dev/null
then
	fail "SCCS hidden extensions in degenerated comment are not supported"	
else
	docommand cm2f "${admin} -fb $s" 0 "" ""

	# Flag 'b' may appear as '^Af b' or '^Af b ', so the checksum may vary
D 3
	tail +2 s.comment	> s.o
	tail +2 $s		> s.n
E 3
I 3
	tail $plustwo s.comment	> s.o
	tail $plustwo $s	> s.n
E 3
	if diff -w s.o s.n > /dev/null
	then
		echo "SCCS hidden extensions in degenerated comment are supported"
		remove s.o s.n
	else
		fail "SCCS hidden extensions in degenerated comment are not supported"
	fi
fi

remove s.o s.n
remove $z $s $p $g
success
E 1
