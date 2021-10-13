hV6,sum=11633
s 00001/00001/00089
d D 1.5 2020/04/16 01:44:04+0200 joerg 5 4
S s 13539
c %Z% Neu um "No id keywords" Warnung zu vermeiden
e
s 00002/00002/00088
d D 1.4 2015/06/03 00:06:44+0200 joerg 4 3
S s 10532
c ../common/test-common -> ../../common/test-common
e
s 00009/00001/00081
d D 1.3 2015/04/25 18:43:53+0200 joerg 3 2
S s 10254
c test -w -> wtest -w ... wtest ist eine Funktion mit ls -l | grep
e
s 00025/00000/00057
d D 1.2 2011/05/30 19:30:32+0200 joerg 2 1
S s 46401
c setup()/restore() neu um SCCS nach XSCCS und zurueck zu wandeln
e
s 00057/00000/00000
d D 1.1 2010/04/29 02:05:14+0200 joerg 1 0
S s 19738
c date and time created 10/04/29 02:05:14 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ed81a95
G p sccs/tests/cssctests/get/middle-fail.sh
t
T
I 1
#! /bin/sh
#
# This is a test for SourceForge Bug ID 595594, reported by Joel Young.
# This is where "sccs get SCCS" where there are three fiules (a, b, c) in the
# SCCS difrectory stops processing at b, because a writable version of 
# b exists.  In fact iot should carry on a check out a copy of c.

D 4
. ../common/test-common
E 4
I 4
. ../../common/test-common
E 4
I 3

# The test suite fails if you run it as root, particularly because
# "test -w foo" returns 0 if you are root, even if foo is a readonly
# file. We try to avoid this by calling the "wtest" function instead
# of just "test".
# Please don't run the test suite as root, because it may spuriously
# fail.
E 3
D 4
. ../common/not-root
E 4
I 4
. ../../common/not-root
E 4


files="a b c"

I 2
setup() {
	if test -f SCCS/s.middle-fail.sh
	then
		if test -d XSCCS
		then
			miscarry "Cannot rename SCCS to XSCCS, XSCCS exists"
		else
			mv SCCS XSCCS
		fi
	fi
}
E 2

I 2
restore() {
	if test -f XSCCS/s.middle-fail.sh
	then
		if test -d SCCS
		then
			miscarry "Cannot rename XSCCS back to XSCCS, SCCS exists"
		else
			mv XSCCS SCCS
		fi
	fi
}

E 2
cleanup () {
    if [ -d SCCS ] 
    then
	( cd SCCS && for i in $files; do rm -f [spzd].$i; done )
	rm -f $files
	rmdir SCCS
    fi
    rm -f $files
}

I 2
setup
E 2
cleanup
remove command.log log log.stdout log.stderr 
mkdir SCCS

echo "Creating the input files..."
for i in $files
do
D 5
    echo "This is file $i" > $i
E 5
I 5
    echo "This is file $i %Z%" > $i	# Avoid "No id keywords" warning
E 5
    ${admin} -i$i SCCS/s.$i
    rm $i
done


docommand e1 "${vg_get} -e SCCS/s.b" 0 IGNORE IGNORE
docommand e2 "test -w b" 0 "" ""
docommand e3 "${vg_get} SCCS" 1 IGNORE IGNORE

# At this point, a read-only copy of a and c should exist.
# b should still be writable. 

for i in a c 
do
    docommand e4${i}1 "test -f $i" 0 "" ""
D 3
    docommand e4${i}2 "test -w $i" 1 "" ""
E 3
I 3
#    docommand e4${i}2 "test -w $i" 1 "" ""
    docommand e4${i}2 "wtest -w $i" 1 "" ""
E 3
done

docommand e5 "test -w b" 0 "" ""

cleanup
I 2
restore
E 2
success


    
E 1
