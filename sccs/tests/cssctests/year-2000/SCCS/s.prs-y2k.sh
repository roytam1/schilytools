h06909
s 00002/00002/00109
d D 1.3 15/06/03 00:06:44 joerg 3 2
c ../common/test-common -> ../../common/test-common
e
s 00009/00000/00102
d D 1.2 11/05/31 21:54:19 joerg 2 1
c expect_fail=true fuer AIX um einen Abbruch zu vermeiden
e
s 00102/00000/00000
d D 1.1 11/04/26 11:44:52 joerg 1 0
c date and time created 11/04/26 11:44:52 by joerg
e
u
U
f e 0
f y 
t
T
I 1
#! /bin/sh


# prs-y2k.sh:  Testing for correct operation of prs
#               with regard to date issues.

# Import common functions & definitions.
D 3
. ../common/test-common
. ../common/real-thing
E 3
I 3
. ../../common/test-common
. ../../common/real-thing
E 3


s=s.y2k.txt

brief='"-d:I: :D: :T:"'

r1_5="1.5 68/12/31 23:59:59\n" # 2068: the last year we have
r1_4="1.4 00/02/29 00:00:00\n" # Year 2000 is a leap year.
r1_3="1.3 00/01/01 00:00:00\n" # Just after the milennium
r1_2="1.2 99/12/31 23:59:59\n" # Just before the milennium
r1_1="1.1 69/01/01 00:00:00\n" # 1969: the earliest year we have

allrevs="${r1_5}${r1_4}${r1_3}${r1_2}${r1_1}"

# First some easy tests that any y2k-compliant version should pass.
docommand ez2 "${vg_prs} ${brief} -r1.2  $s" 0 "${r1_2}" ""
docommand ez3 "${vg_prs} ${brief} -r1.3  $s" 0 "${r1_3}" ""
docommand ez4 "${vg_prs} ${brief} -r1.4  $s" 0 "${r1_4}" ""


if "$TESTING_CSSC"
then
    expect_fail=false
I 2
    OS=`uname`
    #
    # AIX does not support any date before January 1 1970 with the
    # time functions in libc
    #
    if test .$OS = .AIX
    then
	expect_fail=true
    fi
E 2
else
    # Many versions of SCCS are y2k-safe, but don't work right out to
    # the boundary dates specified by the POSIX windowing scheme.
    # For example OpenSolaris 2009.06 renders "68/12/31 23:59:59"
    # as "32/11/25 17:31:43".
    expect_fail=true
fi

## And now the harder tests.

## If we just specify -e without -c we should get all the revisions.
## Check that the dates are printed correctly.
docommand A1 "${vg_prs} ${brief} -e $s" 0 "${allrevs}" ""

docommand t1 "${vg_prs} ${brief} -e -c690101000000  $s" 0 \
    "${r1_1}" ""
docommand t2 "${vg_prs} ${brief} -l -c690101000000  $s" 0 \
    "${allrevs}" ""

docommand t3 "${vg_prs} ${brief} -l -c690101000001  $s" 0 \
    "${r1_5}${r1_4}${r1_3}${r1_2}" ""

docommand t4 "${vg_prs} ${brief} -e -c991231235959  $s" 0 \
    "${r1_2}${r1_1}" ""

docommand t5 "${vg_prs} ${brief} -l -c991231235959  $s" 0 \
    "${r1_5}${r1_4}${r1_3}${r1_2}" ""

docommand t6 "${vg_prs} ${brief} -e -c000101000000  $s" 0 \
    "${r1_3}${r1_2}${r1_1}" ""
docommand t7 "${vg_prs} ${brief} -l -c000101000000  $s" 0 \
    "${r1_5}${r1_4}${r1_3}" ""

docommand t8 "${vg_prs} ${brief} -l -c000101000001  $s" 0 \
    "${r1_5}${r1_4}" ""

docommand t9 "${vg_prs} ${brief} -e -c000229000000  $s" 0 \
    "${r1_4}${r1_3}${r1_2}${r1_1}" ""
docommand t10 "${vg_prs} ${brief} -e -c000229000001  $s" 0 \
    "${r1_4}${r1_3}${r1_2}${r1_1}" ""
docommand t11 "${vg_prs} ${brief} -l -c000229000000  $s" 0 \
    "${r1_5}${r1_4}" ""

docommand t12 "${vg_prs} ${brief} -l -c681231235959  $s" 0 \
    "${r1_5}" ""


## Tests involving fields that take default values...

# Just giving the year should be equivalent to explicitly
# specifying the last second of that year.
docommand d1 "${vg_prs} ${brief} -l -c99  $s" 0 \
    "${r1_5}${r1_4}${r1_3}${r1_2}" ""

docommand d2 "${vg_prs} ${brief} -l -c0001  $s" 0 \
    "${r1_5}${r1_4}" ""

docommand d3 "${vg_prs} ${brief} -l -c000228  $s" 0 \
    "${r1_5}${r1_4}" ""
docommand d4 "${vg_prs} ${brief} -e -c68  $s" 0 \
    "${r1_5}${r1_4}${r1_3}${r1_2}${r1_1}" ""

docommand d5 "${vg_prs} ${brief} -l -c68  $s" 0 \
    "${r1_5}" ""




remove command.log
success
E 1
