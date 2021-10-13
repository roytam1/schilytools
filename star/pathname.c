/* @(#)pathname.c	1.3 14/03/31 Copyright 2004-2014 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)pathname.c	1.3 14/03/31 Copyright 2004-2014 J. Schilling";
#endif
/*
 *	Copyright (c) 2004-2014 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include <schily/stdio.h>
#include <schily/types.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/jmpdefs.h>
#include <schily/schily.h>

#include "pathname.h"

EXPORT	int		init_pspace	__PR((FILE *f, pathstore_t *pathp));
EXPORT	void		free_pspace	__PR((pathstore_t *pathp));
EXPORT	ssize_t		incr_pspace	__PR((FILE *f, pathstore_t *pathp, size_t amt));
EXPORT	ssize_t		grow_pspace	__PR((FILE *f, pathstore_t *pathp, size_t amt));
EXPORT	ssize_t		set_pspace	__PR((FILE *f, pathstore_t *pathp, size_t amt));

EXPORT int
strcpy_pspace(f, pathp, nm)
	FILE		*f;
	pathstore_t	*pathp;
	const char	*nm;
{
	size_t		nlen;

	if (pathp->ps_path == NULL)
		pathp->ps_size = 0;

	/*
	 * If ps_path space is not sufficient, expand it.
	 */
	nlen = strlen(nm);
	if (pathp->ps_size < (nlen + 2)) {
		if (grow_pspace(f, pathp, (nlen + 2)) < 0)
			return (-1);
	}
	strcpy(pathp->ps_path, nm);
	pathp->ps_tail = nlen;
	return (0);
}

EXPORT int
init_pspace(f, pathp)
	FILE		*f;
	pathstore_t	*pathp;
{
	sigjmps_t	*jmp = JM_RETURN;

	if (f == PS_EXIT) {
		f = stderr;
		jmp = JM_EXIT;
	} else if (f == PS_STDERR) {
		f = stderr;
	}
	pathp->ps_tail = 0;
	pathp->ps_size = PS_INCR;
	pathp->ps_path = __fjmalloc(f, PS_INCR, "path buffer", jmp);
	if (pathp->ps_path == NULL) {
		pathp->ps_size = 0;
		return (-1);
	}
	pathp->ps_path[0] = '\0';
	return (0);
}

EXPORT ssize_t
set_pspace(f, pathp, amt)
	FILE		*f;
	pathstore_t	*pathp;
	size_t		amt;
{
	size_t	nsize;
	char	*new;
	sigjmps_t	*jmp = JM_RETURN;

	if (f == PS_EXIT) {
		f = stderr;
		jmp = JM_EXIT;
	} else if (f == PS_STDERR) {
		f = stderr;
	}

	nsize = (amt + PS_INCR - 1) / PS_INCR * PS_INCR;
	new = __fjrealloc(f, pathp->ps_path, nsize, "path buffer", jmp);
	if (new == NULL)
		return (-1);
	if (nsize < pathp->ps_size)	/* If it shrinks  */
		new[nsize-1] = '\0';	/* Null terminate */
	pathp->ps_path = new;
	pathp->ps_size = nsize;
	return (nsize);
}

EXPORT ssize_t
incr_pspace(f, pathp, amt)
	FILE		*f;
	pathstore_t	*pathp;
	size_t		amt;
{
	return (set_pspace(f, pathp, pathp->ps_size + amt));
}

EXPORT ssize_t
grow_pspace(f, pathp, amt)
	FILE		*f;
	pathstore_t	*pathp;
	size_t		amt;
{
	if (pathp->ps_size >= amt)
		return (pathp->ps_size);
	return (set_pspace(f, pathp, amt));
}

EXPORT void
free_pspace(pathp)
	pathstore_t	*pathp;
{
	free(pathp->ps_path);

	pathp->ps_path = NULL;
	pathp->ps_size = 0;
	pathp->ps_tail = 0;
}
