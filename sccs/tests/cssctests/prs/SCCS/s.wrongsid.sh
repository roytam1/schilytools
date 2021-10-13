hV6,sum=37837
s 00001/00001/00032
d D 1.3 2015/06/03 00:06:44+0200 joerg 3 2
S s 04297
c ../common/test-common -> ../../common/test-common
e
s 00000/00000/00033
d D 1.2 2015/06/01 23:55:23+0200 joerg 2 1
S s 04158
c ../../testutils/ -> ${SRCROOT}/tests/testutils/
e
s 00033/00000/00000
d D 1.1 2011/04/26 03:04:16+0200 joerg 1 0
S s 04158
c date and time created 11/04/26 03:04:16 by joerg
e
u
U
f e 0
f y 
G r 0e46e8edc931a
G p sccs/tests/cssctests/prs/wrongsid.sh
t
T
I 1
#! /bin/sh

# wrongsid.sh:  what happens if the desired SID does not exist?

# Import common functions & definitions.
D 3
. ../common/test-common
E 3
I 3
. ../../common/test-common
E 3

cleanup () {
    for prefix in s p z l
    do
	remove ${prefix}.foo ${prefix}.bar
    done
    remove command.log
}

cleanup

# Create files
docommand ws1 "${admin} -i/dev/null -r5.1 -n s.foo" 0 "" IGNORE
docommand ws2 "${admin} -i/dev/null -r10.1 -n s.bar" 0 "" IGNORE


# Basic successful operations.
docommand ws3 "${vg_prs} -r5.1 -d':M:-:I:\nX' s.foo" 0 "foo-5.1\nX\n" ""
docommand ws4 "${vg_prs} -r10.1 -d':M:-:I:\nX' s.bar" 0 "bar-10.1\nX\n" ""

# Similar but specifying a nonexistent SID
docommand ws5 "${vg_prs} -r10.1 -d':M:-:I:\nX' s.foo" 1 "" IGNORE
# This time, make sure the failure of the first command is remembered.
docommand ws6 "${vg_prs} -r10.1 -d':M:-:I:' s.foo s.bar" 1 "bar-10.1\n" IGNORE

cleanup
success
E 1
