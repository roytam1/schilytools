hV6,sum=00438
s 00002/00002/00078
d D 1.3 2015/06/03 00:06:44+0200 joerg 3 2
S s 24869
c ../common/test-common -> ../../common/test-common
e
s 00010/00002/00070
d D 1.2 2015/04/25 18:43:53+0200 joerg 2 1
S s 24591
c test -w -> wtest -w ... wtest ist eine Funktion mit ls -l | grep
e
s 00072/00000/00000
d D 1.1 2010/05/11 11:30:00+0200 joerg 1 0
S s 61310
c date and time created 10/05/11 11:30:00 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ed89d58
G p sccs/tests/cssctests/get/writable.sh
t
T
I 1
#! /bin/sh
# writable.sh:  Will get over-write a writable file?

# Import common functions & definitions.
D 3
. ../common/test-common
E 3
I 3
. ../../common/test-common
E 3

# You cannot run the test suite as root.
I 2
# The test suite fails if you run it as root, particularly because
# "test -w foo" returns 0 if you are root, even if foo is a readonly
# file. We try to avoid this by calling the "wtest" function instead
# of just "test".
# Please don't run the test suite as root, because it may spuriously
# fail.
E 2
D 3
. ../common/not-root
E 3
I 3
. ../../common/not-root
E 3

remove command.log log log.stdout log.stderr

f=wrtest
gfile=_g.$f
remove s.$f

# Generate empty file.
: > $f

# Create the s. file and make sure it exists.
docommand W1 "$admin -n -i$f s.$f" 0 "" IGNORE

test -r s.$f         || fail admin did not create s.$f
remove $f
echo foo > $f
chmod +w $f

# Try running get when gfile was writable -- it should fail.
docommand W2 "${vg_get} s.$f" 1 IGNORE IGNORE
remove $gfile
test -f $gfile	    && miscarry could not remove _g.$f

# Now run get with the -G option and it should work even
# though the file's usual name is occupied by a writable file.
docommand W3 "${vg_get} -G$gfile s.$f" 0 "1.1\n0 lines\n" IGNORE


# If you specify the "-k" option, the gotten file should be read-write.
# If you don't specify -k or -e, it will be read-only.  -e implies -k.
remove $gfile $f
docommand W4 "${vg_get} s.$f" 0 "1.1\n0 lines\n" IGNORE

# Make sure the file is read only.
echo_nonl "W5..."
D 2
if test -w $f 
E 2
I 2
#if test -w $f 
if wtest -w $f 
E 2
then
    fail W5: "get s.$f created writable $f"
fi
echo passed



# Now do the same again, using the -k option.
remove $gfile $f
docommand W6 "${vg_get} -k s.$f" 0 "1.1\n0 lines\n" IGNORE

# Make sure the file is read only.
echo_nonl "W7..."
D 2
if test -w $f 
E 2
I 2
#if test -w $f 
if wtest -w $f 
E 2
then
    true
else
    fail W5: "get -k s.$f created read-only $f.  It should be writable"
fi
echo passed





remove $f s.$f $gfile
remove command.log
success
E 1
