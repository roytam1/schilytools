hV6,sum=60750
s 00001/00001/00048
d D 1.2 2015/06/03 00:06:44+0200 joerg 2 1
S s 35677
c ../common/test-common -> ../../common/test-common
e
s 00049/00000/00000
d D 1.1 2010/05/11 11:30:00+0200 joerg 1 0
S s 35538
c date and time created 10/05/11 11:30:00 by joerg
e
u
U
f e 0
f y 
G r 0e46e8eed9277
G p sccs/tests/cssctests/rmdel/edit.sh
t
T
I 1
#! /bin/sh
# edit.sh:  Editing the last normal delta when the top delta
#           has been rmdel'ed -- this is what "sccs fix" does.

# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

g=testfile.txt
s=s.$g
z=z.$g
x=x.$g
p=p.$g

remove command.log log log.stdout log.stderr $g $s $z $x $p

remove $g
echo "hello, this is a test file" > $g

# Prepare an SCCS file with two revisions.
docommand a1 "${admin} -i${g} $s" 0 "" IGNORE
remove $g
docommand a2 "${get} -e $s" 0 "1.1\nnew delta 1.2\n1 lines\n" ""
docommand a3 "${delta} -yNoComment $s" 0 \
	"1.2\n0 inserted\n0 deleted\n1 unchanged\n"   \
    IGNORE

# Remove 1.2.
docommand a4 "${vg_rmdel} -r1.2 $s" 0 "" ""

# Now edit 1.1 and make sure that the new SID is 1.2.
docommand a5 "${get} -e $s" 0 "1.1\nnew delta 1.2\n1 lines\n" ""

# Make sure we can check it in too.
docommand a6 "${delta} -yNoComment $s" 0 \
	"1.2\n0 inserted\n0 deleted\n1 unchanged\n"   \
    IGNORE


# Do the same thing again, but in a new release.
docommand a7 "${get} -e -r2 $s" 0 "1.2\nnew delta 2.1\n1 lines\n" ""

# Make sure we can check it in too.
docommand a8 "${delta} -yNoComment $s" 0 \
	"2.1\n0 inserted\n0 deleted\n1 unchanged\n"   \
    IGNORE


remove command.log log log.stdout log.stderr $g $s $z $x $p
success
E 1
