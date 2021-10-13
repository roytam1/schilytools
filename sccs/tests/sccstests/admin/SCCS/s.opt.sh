hV6,sum=17364
s 00004/00000/00055
d D 1.3 2016/08/18 22:20:29+0200 joerg 3 2
S s 35745
c touch -t fuer gtouch
e
s 00006/00006/00049
d D 1.2 2015/06/03 00:06:45+0200 joerg 2 1
S s 28199
c ../common/test-common -> ../../common/test-common
e
s 00055/00000/00000
d D 1.1 2011/05/29 23:25:40+0200 joerg 1 0
S s 27365
c date and time created 11/05/29 23:25:40 by joerg
e
u
U
f e 0
f y 
G r 0e46e8b56f350
G p sccs/tests/sccstests/admin/opt.sh
t
T
I 1
#! /bin/sh

# Basic tests for extended SCCS options

# Read test core functions
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

D 2
cmd=admin		# for ../common/optv
ocmd=${admin}		# for ../common/optv
E 2
I 2
cmd=admin		# for ../../common/optv
ocmd=${admin}		# for ../../common/optv
E 2
g=foo
s=s.$g
p=p.$g
z=z.$g
D 2
output=got.output	# for ../common/optv
error=got.error		# for ../common/optv
E 2
I 2
output=got.output	# for ../../common/optv
error=got.error		# for ../../common/optv
E 2

remove $z $s $p $g

#
# Checking whether SCCS ${cmd} supports extended options
#
D 2
. ../common/optv
E 2
I 2
. ../../common/optv
E 2

echo '%M%' > $g		|| miscarry "Could not create $g"
touch 0101000090 $g	|| miscarry "Could not touch $g"
I 3
if [ -f 0101000090 ]; then
	remove 0101000090
	touch -t 199001010000 $g	|| miscarry "Could not touch $g"
fi
E 3
expect_fail=true
docommand go1 "${admin} -o -i$g -n $s" 0 IGNORE IGNORE
if  grep 90/01/01 $s > /dev/null 2> /dev/null
then
	echo "SCCS admin -o is supported"
	remove $s
	echo '%M%' > $g		|| miscarry "Could not create $g"
	touch -t 196001010000 $g
	ret=$?
	ls -l $g | grep 1960 > /dev/null
	ret2=$?
	if test $ret -eq 0 -a $ret2 -eq 0
	then
		docommand go2 "${admin} -o -i$g -n $s" 0 IGNORE IGNORE
		if grep 1960/01/01 $s
		then
			echo "SCCS admin -o works for file from 1960"
		else
			fail "SCCS admin -o failed for file from 1960"
		fi
	else
		fail "touch with date before 1970 not supported"
		echo "Skipping test go2"
	fi
else
	fail "SCCS admin -o unsupported"
fi

remove $z $s $p $g $output $error
success
E 1
