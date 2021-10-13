h11555
s 00001/00001/00039
d D 1.2 15/06/03 00:06:45 joerg 2 1
c ../common/test-common -> ../../common/test-common
e
s 00040/00000/00000
d D 1.1 11/05/29 20:19:37 joerg 1 0
c date and time created 11/05/29 20:19:37 by joerg
e
u
U
f e 0
t
T
I 1
#! /bin/sh

# Basic tests for SCCS extreme year numbers

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
output=get.output

remove $z $s $p $g

expect_fail=true
docommand y1 "${prs} -e '-d:I: :D: :T:' s.dates" 0 IGNORE IGNORE
docommand y2 "${prs} -e '-d:I: :D: :T:' s.dates" 0 "1.6 68/12/31 23:59:59
1.5 38/01/01 00:00:00
1.4 37/12/31 23:59:59
1.3 00/01/01 00:00:00
1.2 99/12/31 23:59:59
1.1 70/01/01 00:00:00
" IGNORE

docommand y3 "${prs} -e '-d:I: :D: :T:' s.2069" 0 IGNORE IGNORE
docommand y4 "${prs} -e '-d:I: :D: :T:' s.2069" 0 "1.7 69/01/01 00:00:00
1.6 68/12/31 23:59:59
1.5 38/01/01 00:00:00
1.4 37/12/31 23:59:59
1.3 00/01/01 00:00:00
1.2 99/12/31 23:59:59
1.1 70/01/01 00:00:00
" IGNORE

docommand y5 "${prs} -e '-d:I: :D: :T:' s.1960" 0 IGNORE IGNORE
docommand y6 "${prs} -e '-d:I: :D: :T:' s.3000" 0 IGNORE IGNORE

remove $z $s $p $g $output
success
E 1
