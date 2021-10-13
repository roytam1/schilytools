/* @(#)defaults.c	1.13 09/07/11 Copyright 1998-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)defaults.c	1.13 09/07/11 Copyright 1998-2009 J. Schilling";
#endif
/*
 *	Copyright (c) 1998-2009 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/deflts.h>
#include <schily/utypes.h>
#include <schily/schily.h>
#include "star.h"
#include "starsubs.h"

extern	const char	*tarfiles[];
extern	long		bs;
extern	int		nblocks;
extern	BOOL		not_tape;
extern	Ullong		tsize;

EXPORT	char	*get_stardefaults __PR((char *name));
LOCAL	int	open_stardefaults __PR((char *dfltname));
EXPORT	void	star_defaults	__PR((long *fsp, char *dfltname));
EXPORT	BOOL	star_darchive	__PR((char *arname, char *dfltname));

EXPORT char *
get_stardefaults(name)
	char	*name;
{
	if (name)
		return (name);
	/*
	 * WARNING you are only allowed to change this filename if you also
	 * change the documentation and add a statement that makes clear
	 * where the official location of the file is why you did choose a
	 * nonstandard location and that the nonstandard location only refers
	 * to inofficial star versions.
	 *
	 * I was forced to add this because some people change star without
	 * rational reason and then publish the result. As those people
	 * don't contribute work and don't give support, they are causing extra
	 * work for me and this way slow down the star development.
	 */
	return ("/etc/default/star");
}

LOCAL int
open_stardefaults(dfltname)
	char	*dfltname;
{
	/*
	 * We may later set options here....
	 */
	return (defltopen(dfltname));
}

EXPORT void
star_defaults(fsp, dfltname)
	long	*fsp;
	char	*dfltname;
{
	long	fs_cur	= 0L;
	long	fs_max	= -1L;

	dfltname = get_stardefaults(dfltname);

	if (fsp != NULL)
		fs_cur = *fsp;

	if (fs_cur <= 0L) {
		char	*p = NULL;

		if (open_stardefaults(dfltname) == 0) {
			p = defltread("STAR_FIFOSIZE=");
		}
		if (p) {
			if (getnum(p, &fs_cur) != 1)
				comerrno(EX_BAD, "Bad fifo size default.\n");
		}
	}
	if (fs_cur > 0L) {
		char	*p = NULL;

		if (open_stardefaults(dfltname) == 0) {
			p = defltread("STAR_FIFOSIZE_MAX=");
		}
		if (p) {
			if (getnum(p, &fs_max) != 1) {
				comerrno(EX_BAD,
					"Bad max fifo size default.\n");
			}
		}
		if (fs_cur > fs_max)
			fs_cur = fs_max;
	}

	if (fs_cur > 0L && fsp != NULL)
		*fsp = fs_cur;

	defltclose();
}

/*
 * Check 'dfltname' for an 'arname' pattern.
 *
 * A correct entry has 3..4 space separated entries:
 *	1)	The device (archive0=/dev/tape)
 *	2)	The bloking factor in 512 byte units (20)
 *	3)	The max media size in 1024 byte units
 *		0 means unlimited (no multi volume handling)
 *	4)	Whether this is a tape or not
 * Examples:
 *	archive0=/dev/tape 512 0 y
 *	archive1=/dev/fd0 1 1440 n
 */
EXPORT BOOL
star_darchive(arname, dfltname)
	char	*arname;
	char	*dfltname;
{
	char	*p;
	Llong	llbs	= 0;

	dfltname = get_stardefaults(dfltname);
	if (dfltname == NULL)
		return (FALSE);
	if (open_stardefaults(dfltname) != 0)
		return (FALSE);

	/*
	 * XXX Sun tar arbeitet mit Ignore Case
	 */
	if ((p = defltread(arname)) == NULL) {
		errmsgno(EX_BAD,
		    "Missing or invalid '%s' entry in %s.\n",
				arname, dfltname);
		return (FALSE);
	}
	if ((p = strtok(p, " \t")) == NULL) {
		errmsgno(EX_BAD,
		    "'%s' entry in %s is empty!\n", arname, dfltname);
		return (FALSE);
	} else {
		tarfiles[0] = ___savestr(p);
	}
	if ((p = strtok(NULL, " \t")) == NULL) {
		errmsgno(EX_BAD,
		    "Block component missing in '%s' entry in %s.\n",
		    arname, dfltname);
		return (FALSE);
	}
	if (getbnum(p, &llbs) != 1) {
		comerrno(EX_BAD, "Bad blocksize entry for '%s' in %s.\n",
			arname, dfltname);
	} else {
		int	iblocks = llbs;

		if ((llbs <= 0) || (iblocks != llbs)) {
			comerrno(EX_BAD,
				"Invalid blocksize '%s'.\n", p);
		}
		bs = llbs;
		nblocks = bs / TBLOCK;
	}
	if ((p = strtok(NULL, " \t")) == NULL) {
		errmsgno(EX_BAD,
		    "Size component missing in '%s' entry in %s.\n",
		    arname, dfltname);
		return (FALSE);
	}
	if (getknum(p, &llbs) != 1) {	/* tsize is Ullong */
		comerrno(EX_BAD, "Bad size entry for '%s' in %s.\n",
			arname, dfltname);
	}
	tsize = llbs / TBLOCK;
	/* XXX Sun Tar hat check auf min size von 250 kB */

	/*
	 * XXX Sun Tar setzt not_tape auch wenn kein Tape Feld vorhanden ist
	 * XXX und tsize != 0 ist.
	 */
	if ((p = strtok(NULL, " \t")) != NULL)		/* May be omited */
		not_tape = (*p == 'n' || *p == 'N');
	defltclose();
#ifdef DEBUG
	error("star_darchive: archive='%s'; tarfile='%s'\n", arname, tarfiles[0]);
	error("star_darchive: nblock='%d'; tsize='%llu'\n",
	    nblocks, tsize);
	error("star_darchive: not tape = %d\n", not_tape);
#endif
	return (TRUE);
}
