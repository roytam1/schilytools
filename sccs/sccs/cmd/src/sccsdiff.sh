#! /bin/sh
# @(#)sccsdiff.sh	1.14 20/10/31 Copyright 2011-2020 J. Schilling
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#
# Copyright (c) 1988 AT&T
# All Rights Reserved
# Copyright 2003 Sun Microsystems, Inc. All rights reserved.
# Use is subject to license terms.
#
# @(#)sccsdiff.sh 1.15 06/12/12
#
#ident	"@(#)sccsdiff.sh"
#ident	"@(#)sccs:cmd/sccsdiff.sh"
#	DESCRIPTION:
#		Execute diff(1) on two versions of a set of
#		SCCS files and optionally pipe through pr(1).
#		Optionally specify diff segmentation size.
# if running under control of the NSE (-q option given), will look for
# get in the directory from which it was run (if -q, $0 will be full pathname)
#
PATH=INS_BASE/SCCS_BIN_PREbin:$PATH:/usr/ccs/bin

trap "rm -f /tmp/get[abc]$$;exit 1" 1 2 3 15
umask 077

if [ $# -lt 3 ] && [ "$1" != -V ] && [ "$1" != -version ] && [ "$1" != --version ]
then
	echo "Usage: sccsdiff -r<sid1> -r<sid2> [-p] [-q] [-N[bulk-spec]] sccsfile ..." 1>&2
	exit 1
fi

nseflag=
rflag=
addflags=
Nflag=
get=get
for i in "$@"
do
	if [ "$addflags" = "yes" ]
	then
		flags="$flags $i"
		addflags=
		continue
	fi
	case "$i" in

	-*)
		case "$i" in

		-r*)
			if [ ! "$sid1" ]
			then
				sid1=`echo $i | sed -e 's/^-r//'`
				if [ "$sid1" = "" ]
				then
					rflag=yes
				fi	
			elif [ ! "$sid2" ]
			then
				sid2=`echo $i | sed -e 's/^-r//'`
				if [ "$sid2" = "" ]
				then 
					rflag=yes
				fi	
			fi
			;;
		-s*)
			num=`echo $i | sed -e 's/^-s//'`
			;;
		-p*)
			pipe=yes
			;;
		-q*)
			nseflag=-q
			get=`dirname $0`/get
			;;
		-[cefnhbwitu]*)
			flags="$flags $i"
			;;
		-D*)
			Dopt="-D"
			Darg=`echo $i | sed -e 's/^-D//'`
			if [ "$Darg" = "" ]
			then
				flags="$flags $i"
				addflags=yes
			else
				flags="$flags $Dopt $Darg"
			fi
			;;
		-C)
			flags="$flags $i"
			addflags=yes
			;;
		-U*)
			if [ `echo $i | wc -c` = 3 ]
			then
				flags="$flags $i"
				addflags=yes
			else
				opt=`echo $i | sed -e 's/^-U/-U /'`
				flags="$flags $opt"
			fi
			;;
		-N*)
			Nflag="$i"
			;;
		-V | -version | --version)
			echo "$0 PROVIDER-SCCS version VERSION VDATE (HOST_SUB)"
			exit 0
			;;
		*)
			echo "$0: unknown argument: $i" 1>&2
			exit 1
			;;
		esac
		;;
	s.*|*/s.*)
		files="$files $i"
		;;
	[1-9]*)
		if [ "$rflag" != "" ]
		then
			if [ ! "$sid1" ]
			then
				sid1=$i
			elif [ ! "$sid2" ]
			then
				sid2=$i	
			fi
		fi
		rflag=	
		;;	
	*)
		if [ "$Nflag" ]; then
			#
			# In NewMode, get(1) converts g-filenames into
			# s-filenames if needed, so need to let get(1)
			# do the checks.
			#
			files="$files $i"
		else
			echo "$0: $i not an SCCS file" 1>&2
		fi
		;;
	esac
done

if [ "$files" = "" ]
then
	echo "$0: missing file arg (cm3)" 1>&2
	exit 1
fi

error=0
for i in $files
do
	rm -f /tmp/get[abc]$$
	# Good place to check if tmp-files are not deleted
	# if [ -f /tmp/geta$$ ] ...

	#
	# $Nflag may contain spaces, so clear IFS for this command
	#
	OIFS=$IFS
	IFS=
	if $get $nseflag $Nflag -s -o -k -r$sid1 $i -G/tmp/geta$$
	then
		if $get $nseflag $Nflag -s -o -k -r$sid2 $i -G/tmp/getb$$
		then
			IFS=$OIFS
			diff $flags /tmp/geta$$ /tmp/getb$$ > /tmp/getc$$
		else
			error=1
			continue	# error: cannot get release $sid2 
		fi
	else
		error=1
		continue	# error: cannot get release $sid1
	fi
	IFS=$OIFS

	if [ ! -s /tmp/getc$$ ]
	then
		if [ -f /tmp/getc$$ ]
		then
			echo "$i: No differences" > /tmp/getc$$
		else
			error=1
			continue	# error: cannot get differences
		fi
	fi
	if [ "$pipe" ]
	then
		pr -h "$i: $sid1 vs. $sid2" /tmp/getc$$
	else
		file=`basename $i | sed -e 's/^s\.//'`
		echo
		echo "------- $file -------"
		cat /tmp/getc$$
	fi
done
rm -f /tmp/get[abc]$$
exit $error
