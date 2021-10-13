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
/*
 * @(#)mdata.c	1.2 11/10/15 Copyright 2011 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)mdata.c 1.2 11/10/15 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)mdata.c"
#pragma ident	"@(#)sccs:lib/mpwlib/mdata.c"
#endif
#include	<defines.h>

	char	Null[1];
struct	stat	Statbuf;
	char	SccsError[MAXERRORLEN];

#ifdef	__never__
	char	Error[128];	/* Not really used in SCCS */
#endif

#if	defined(IS_MACOS_X)
/*
 * The Mac OS X static linker is too silly to link in .o files from static libs
 * if only a variable is referenced. The elegant workaround for this bug (using
 * common variables) triggers a different bug in the dynamic linker from Mac OS
 * that is unable to link common variables. This forces us to introduce funcs
 * that need to be called from central places to enforce to link in the vars.
 */
EXPORT	void __mpw __PR((void));

EXPORT void
__mpw()
{
}
#endif
