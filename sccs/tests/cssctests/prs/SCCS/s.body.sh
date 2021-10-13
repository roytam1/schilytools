hV6,sum=62530
s 00001/00001/00057
d D 1.2 2015/06/03 00:06:44+0200 joerg 2 1
S s 37630
c ../common/test-common -> ../../common/test-common
e
s 00058/00000/00000
d D 1.1 2010/04/29 02:05:14+0200 joerg 1 0
S s 37491
c date and time created 10/04/29 02:05:14 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ee1731f
G p sccs/tests/cssctests/prs/body.sh
t
T
I 1
#! /bin/sh

# body.sh:  Testing for :GB: keyword of prs.

# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

remove s.1 p.1 z.1 1 command.log DESC s.foo p.foo z.foo 

# Create file
echo "hello" > DESC
docommand b1 "${admin} -n -iDESC s.1" 0 "" IGNORE
remove DESC

# get -p should just emit the body.
docommand b2 "${get} -p s.1" 0 "hello
" IGNORE

# get -d :GB: should emit the body followed by a newline
docommand b3 "${vg_prs} -d:GB: s.1" 0 "hello

" IGNORE


# Also, keyword expansion should occur too. 
remove s.1 p.1 z.1 1 command.log DESC

# Create file again
echo "%Z%" > DESC
docommand b4 "${admin} -n -iDESC s.1" 0 "" IGNORE
remove DESC


# get -p should just emit the body, with kw expansion
docommand b5 "${get} -p s.1" 0 "@(#)
" IGNORE

# get -d :GB: should emit the body followed by a newline with kw expansion
docommand b6 "${vg_prs} -d:GB: s.1" 0 "@(#)

" IGNORE

# get -p -k should just emit the body, without kw expansion
# we have to make this check to ensure that prs was really
# going keyword expansion
docommand b7 "${get} -p -k s.1" 0 "%Z%
" IGNORE


## Testing for :BD:
docommand b7 "cp sample_foo s.foo" 0 IGNORE IGNORE

do_output b8 "${vg_prs} -d:BD: s.foo" 0 s_foo_bd_output.txt IGNORE



remove s.1 p.1 z.1 1 command.log s.foo p.foo z.foo 
success
E 1
