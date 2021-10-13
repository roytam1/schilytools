hV6,sum=52279
s 00001/00000/00072
d D 1.6 2018/04/30 13:06:08+0200 joerg 6 5
S s 47788
c Username mit SPACE wird in Underline im History File gewandelt
e
s 00009/00002/00063
d D 1.5 2016/08/14 23:16:32+0200 joerg 5 4
S s 44286
c tail +2 -> tail $plustwo und $plustwo wird automatish angepasst
e
s 00005/00001/00060
d D 1.4 2016/06/23 00:34:00+0200 joerg 4 3
S s 34752
c erstmal id -un statt logname
e
s 00001/00001/00060
d D 1.3 2016/06/22 22:25:17+0200 joerg 3 2
S s 30803
c Die Sekundenbruchteile bei SCCSv6 koennen auch weniger als 10 Stellen haben
e
s 00001/00001/00060
d D 1.2 2016/06/21 22:14:49+0200 joerg 2 1
S s 30841
c joerg: -> $logname:
e
s 00061/00000/00000
d D 1.1 2016/06/20 00:54:11+0200 joerg 1 0
S s 30611
c date and time created 16/06/20 00:54:11 by joerg
e
u
U
f e 0
G r 0e46e8b5ee30f
G p sccs/tests/sccstests/format/format.sh
t
T
I 1
#! /bin/sh

# Basic tests for SCCS time stamps

# Read test core functions
. ../../common/test-common

cmd=admin		# for ../../common/optv
ocmd=${admin}		# for ../../common/optv
g=foo
s=s.$g
p=p.$g
z=z.$g
output=got.output	# for ../../common/optv
error=got.error		# for ../../common/optv

remove $z $s $p $g

#
# Checking whether SCCS ${cmd} supports extended options
#
. ../../common/optv

docommand format01 "${admin} -V6 -n $s" 0 IGNORE IGNORE

if grep -s 'hV6' $s > /dev/null; then
	format=v6
else
	format=v4
fi

D 4
logname=`logname`
E 4
I 4
logname=`id -un`
if test -z "$logname"
then
	logname=`logname`
fi
I 6
logname=`echo $logname | sed -e 's/ /_/g'`
E 6
E 4

I 5
echo test | tail +2 > /dev/null 2>/dev/null
if [ "$?" -eq 0 ]; then
	plustwo=+2
else
	plustwo='-n +2'
fi

E 5
if test "$format" = v6
then
D 5
	tail +2 $s | sed \
E 5
I 5
	tail $plustwo $s | sed \
E 5
		-e 's^ [0-9]*/../.. ..:..:..^ yy/mm/dd hh:mm:ss^' \
D 3
		-e 's/ss\........../ss.fffffffff/' \
E 3
I 3
		-e 's/ss\.[0-9]*/ss.fffffffff/' \
E 3
		-e 's/[+-][0-9][0-9][0-9][0-9] /+zzzz /' \
D 2
		-e 's: joerg: uuu:' \
E 2
I 2
		-e "s: $logname: uuu:" \
E 2
		-e 's/G r .............*/G r xxxxxxxxxxxxx/' | \
		grep -v 'G p ' > format
	diffs=`diff format reference-v6`
	if test ! -z "$diffs"
	then
		fail "Deviations from SCCSv6 format: $diffs"
	fi
else
D 5
	tail +2 $s | sed \
E 5
I 5
	tail $plustwo $s | sed \
E 5
		-e 's^ [0-9]*/../.. ..:..:..^ yy/mm/dd hh:mm:ss^' \
		-e "s: $logname: uuu:" > format
	diffs=`diff format reference-v4`
	if test ! -z "$diffs"
	then
		fail "Deviations from SCCSv4 format: $diffs"
	fi
fi
echo "format$format...passed"

remove $z $s $p $g $output $error format
success
E 1
