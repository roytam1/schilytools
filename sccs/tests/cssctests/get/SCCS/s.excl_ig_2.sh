hV6,sum=51093
s 00002/00002/00127
d D 1.4 2015/06/03 00:06:44+0200 joerg 4 3
S s 49864
c ../common/test-common -> ../../common/test-common
e
s 00000/00012/00129
d D 1.3 2015/02/19 22:31:35+0100 joerg 3 2
S s 49586
c Test ei27 klappt nun auch mit SCCSv6
e
s 00013/00000/00128
d D 1.2 2014/09/15 22:42:00+0200 joerg 2 1
S s 06469
c Hinweis auf SCCSv6 Bug mit delta -gsid
e
s 00128/00000/00000
d D 1.1 2010/04/29 02:05:14+0200 joerg 1 0
S s 47660
c date and time created 10/04/29 02:05:14 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ecdd99f
G p sccs/tests/cssctests/get/excl_ig_2.sh
t
T
I 1
#! /bin/sh
# excl_ig_2.sh:  More tests for exclusions and ignores.

# Import common functions & definitions.
D 4
. ../common/test-common
I 2
. ../common/real-thing
E 4
I 4
. ../../common/test-common
. ../../common/real-thing
E 4
E 2

g=foo
s=s.$g
x=x.$g 
z=z.$g
p=p.$g
files="$g $s $x $z $p"
remove $files



do_change() {
    baselabel="$1"; shift

    docommand ${baselabel}.0 "${vg_get} -e $s" 0 IGNORE IGNORE

    for sedcmd
    do
      rename $g $g.old
      remove $g.sed
      echo "$sedcmd" > $g.sed
      docommand "${baselabel}.sed" "sed -f $g.sed < $g.old > $g" 0 "" ""
      remove $g.old $g.sed
    done

    docommand "${baselabel}.delta" "${delta} -yNoComment $s" 0 IGNORE IGNORE
}


remove $g
echo "%I% inserted in 1.1" > $g
docommand ei1 "${admin} -n -i$g s.foo" 0 IGNORE IGNORE
remove $g

docommand ei2 "${vg_get} -s -p $s" 0 "1.1 inserted in 1.1\n" IGNORE

do_change ei3 '1 a\
This line inserted in 1.2 and deleted in 1.3'
docommand ei4 "${vg_get} -s -p $s" 0 '1.2 inserted in 1.1
This line inserted in 1.2 and deleted in 1.3
' IGNORE


docommand "bi1" "${vg_get} -e -r1.1 $s" 0 "1.1\nnew delta 1.1.1.1\n1 lines\n" ""
echo "inserted on branch 1.1.1.1" >> $g
docommand "bi1" "${delta} -yNone $s" 0 "1.1.1.1
1 inserted
0 deleted
1 unchanged
" ""


do_change ei5 '2d' 'a\
This line inserted in 1.3 and deleted in 1.4'
docommand ei6 "${vg_get} -s -p $s" 0 '1.3 inserted in 1.1
This line inserted in 1.3 and deleted in 1.4
' IGNORE

do_change ei7 '2d' 
docommand ei8 "${vg_get} -s -p $s" 0 '1.4 inserted in 1.1
' IGNORE

docommand ei9 "${vg_get} -g $s" 0 "1.4\n" "IGNORE" 

docommand ei10 "${vg_get} -e -i1.1.1.1 -x1.3 $s" 0 IGNORE IGNORE
echo "inserted in 1.5" >> $g
docommand ei11 "${delta} -yNone $s" 0 IGNORE IGNORE

expect_fail=true
docommand ei12 "${vg_get} -s -p $s" 0 '1.5 inserted in 1.1
inserted on branch 1.1.1.1
This line inserted in 1.2 and deleted in 1.3
inserted in 1.5
' IGNORE


docommand ei13 "${vg_get} -e -x1.4 $s" 0 IGNORE IGNORE
echo "inserted in 1.6" >> $g
docommand ei14 "${delta} -yNone $s" 0 IGNORE IGNORE
docommand ei15 "${vg_get} -s -p $s" 0 '1.6 inserted in 1.1
inserted on branch 1.1.1.1
This line inserted in 1.2 and deleted in 1.3
inserted in 1.5
inserted in 1.6
' IGNORE

docommand ei16 "${vg_get} -e -x1.1.1.1 $s" 0 IGNORE IGNORE
docommand ei17 "${delta} -yNone $s" 0 IGNORE IGNORE
docommand ei18 "${vg_get} -s -p $s" 0 '1.7 inserted in 1.1
This line inserted in 1.2 and deleted in 1.3
inserted in 1.5
inserted in 1.6
' IGNORE

docommand ei19 "${vg_get} -e $s" 0 IGNORE IGNORE
docommand ei20 "${delta} -yNone $s" 0 IGNORE IGNORE
docommand ei21 "${vg_get} -s -p $s" 0 '1.8 inserted in 1.1
This line inserted in 1.2 and deleted in 1.3
inserted in 1.5
inserted in 1.6
' IGNORE

docommand ei22 "${vg_get} -i1.1.1.1 -e $s" 0 IGNORE IGNORE
docommand ei23 "${delta} -yNone $s" 0 IGNORE IGNORE
docommand ei24 "${vg_get} -s -p $s" 0 '1.9 inserted in 1.1
inserted on branch 1.1.1.1
This line inserted in 1.2 and deleted in 1.3
inserted in 1.5
inserted in 1.6
' IGNORE

docommand ei25 "${vg_get}  -e $s" 0 IGNORE IGNORE
I 2
D 3
#cp $s $s.1
#cp $g $g.1
E 3
E 2
docommand ei26 "${delta} -g1.5 -yNone $s" 0 IGNORE IGNORE
I 2
D 3
if $TESTING_SCCS_V6
then
#	exit
	echo 'The next test is expected to fail due to a known SCCSv6 bug in delta.'
	echo 'See BUGS section in the delta(1) man page.'
fi
E 3
E 2
docommand ei27 "${vg_get} -s -p $s" 0 '1.10 inserted in 1.1
inserted on branch 1.1.1.1
inserted in 1.6
This line inserted in 1.3 and deleted in 1.4
' IGNORE


# cat $s
remove $files
I 2
D 3
if $TESTING_SCCS_V6
then
	remove get.*	# Remove file left over from failed v6 test above
fi
E 3
E 2
success
E 1
