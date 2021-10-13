hV6,sum=46482
s 00001/00001/00067
d D 1.2 2015/06/03 00:06:44+0200 joerg 2 1
S s 21011
c ../common/test-common -> ../../common/test-common
e
s 00068/00000/00000
d D 1.1 2010/04/18 18:20:27+0200 joerg 1 0
S s 20872
c date and time created 10/04/18 18:20:27 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ec7b266
G p sccs/tests/cssctests/delta/p-option.sh
t
T
I 1
#! /bin/sh
# p-option.sh:  Testing for the -p option of "delta"

# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

g=foo
s=s.$g
files="$s $g p.$g z.$g"
remove $files

# Create an SCCS file.
docommand p1 "${admin} -n $s"    0 "" IGNORE

# Check the file out for editing.
docommand p2 "${get} -e $s"      0 IGNORE IGNORE

# Append a line
echo "hello" >> $g
docommand p3 "cat $g" 0 "hello\n" ""

# Check the file back in with delta, using the -n option.
docommand p4 "${delta} -p -y $s" 0 \
"1.2
0a1
> hello
1 inserted
0 deleted
0 unchanged
" IGNORE


# Check the file out for editing again
docommand p5 "${get} -e $s"      0 IGNORE IGNORE

# Change the line.
echo "test" > $g
docommand p6 "cat $g" 0 "test\n" ""

docommand p7 "${delta} -p -y $s" 0 \
"1.3
1c1
< hello
---
> test
1 inserted
1 deleted
0 unchanged
" IGNORE


# Delete the (only) line
docommand p8 "${get} -e $s"      0 IGNORE IGNORE
true > $g
docommand p9 "cat $g" 0 "" ""

# Check the file back in with delta, using the -n option.
docommand p10 "${delta} -p -y $s" 0 \
"1.4
1d0
< test
0 inserted
1 deleted
0 unchanged
" IGNORE

remove $files
success
E 1
