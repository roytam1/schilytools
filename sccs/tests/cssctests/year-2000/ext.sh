#! /bin/sh

#################################################################
###          WARNING: this test is CSSC-specific!             ###
#################################################################

# ext.sh:       Testing for the century-specification
#               of CSSC.  This is an extension; other
#               SCCS implementations do not do this.

# Import common functions & definitions.
. ../common/test-common
. ../common/real-thing


s=s.y2k.txt

brief='"-d:I: :D: :T:"'

r1_5="1.5 68/12/31 23:59:59\n" # 2068: the last year we have
r1_4="1.4 00/02/29 00:00:00\n" # Year 2000 is a leap year.
r1_3="1.3 00/01/01 00:00:00\n" # Just after the milennium
r1_2="1.2 99/12/31 23:59:59\n" # Just before the milennium
r1_1="1.1 69/01/01 00:00:00\n" # 1969: the earliest year we have

allrevs="${r1_5}${r1_4}${r1_3}${r1_2}${r1_1}"


if "$TESTING_REAL_CSSC"
then
    echo Testing the CSSC century specifier.
    ## Tests for the century field.

    # Ask for exerything after the end of 1968.  Since the first
    # year we have int he s. file is 1969, we should get everything.
    docommand c1 "${vg_prs} ${brief} -l -c19681231235959  $s" 0 \
	"${allrevs}" ""

    # Ask for exerything before the end of 1968.  Since the first
    # year we have int he s. file is 1969, we should get NOTHING.
    docommand c2 "${vg_prs} ${brief} -e -c19681231235959  $s" 0 \
	"" ""

    # Ask for exerything before the end of 2069.  
    # We chould get everything.
    docommand c3 "${vg_prs} ${brief} -e -c20691231235959  $s" 0 \
	"${allrevs}" ""


else
    echo No testing done for CSSC century specifier.
fi

if "$TESTING_REAL_SCCS"
then
    echo "Testing the SCCS century specifier."
    echo "This may fail for times before January 1 1970 on some platforms."
    expect_fail=true
    ## Tests for the century field.

    # Ask for exerything after the end of 1968.  Since the first
    # year we have int he s. file is 1969, we should get everything.
    docommand c1 "${vg_prs} ${brief} -l -c1968/1231235959  $s" 0 \
	"${allrevs}" ""

    # Ask for exerything before the end of 1968.  Since the first
    # year we have int he s. file is 1969, we should get NOTHING.
    docommand c2 "${vg_prs} ${brief} -e -c1968/1231235959  $s" 0 \
	"" ""

    # Ask for exerything before the end of 2069.  
    # We chould get everything.
    docommand c3 "${vg_prs} ${brief} -e -c2069/1231235959  $s" 0 \
	"${allrevs}" ""


else
    echo No testing done for SCCS century specifier.
fi


remove command.log
success
