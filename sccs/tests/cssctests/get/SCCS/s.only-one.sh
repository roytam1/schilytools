hV6,sum=64705
s 00001/00001/00024
d D 1.2 2015/06/03 00:06:44+0200 joerg 2 1
S s 39404
c ../common/test-common -> ../../common/test-common
e
s 00025/00000/00000
d D 1.1 2010/04/29 02:05:14+0200 joerg 1 0
S s 39265
c date and time created 10/04/29 02:05:14 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ecd55c0
G p sccs/tests/cssctests/get/only-one.sh
t
T
I 1
#! /bin/sh

# Tests with only one revision in the SCCS file.

# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2


f=1test
remove $f s.$f
: > $f
docommand O1 "$admin -n -i$f s.$f" 0 "" IGNORE
test -r s.$f         || fail admin did not create s.$f
remove $f out

# The get should succeed.
docommand O2 "${vg_get} s.$f" 0 "1.1\n0 lines\n" IGNORE

# With no SCCS file, get should fail.
remove s.$f 
docommand O3 "${vg_get} s.$f" 1 "" IGNORE
remove s.$f $f 

remove command.log
success
E 1
