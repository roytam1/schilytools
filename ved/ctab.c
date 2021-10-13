/* @(#)ctab.c	1.16 18/08/23 Copyright 1986-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)ctab.c	1.16 18/08/23 Copyright 1986-2018 J. Schilling";
#endif
/*
 *	Character string and stringlength tables for screen
 *	output functions.
 *
 *	Copyright (c) 1986-2018 J. Schilling
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

#include "ved.h"

EXPORT	Uchar	csize[256];
EXPORT	Uchar	*ctab[256];

LOCAL	Uchar	*cmakestr	__PR((ewin_t *wp, Uchar* s));
EXPORT	void	init_charset	__PR((ewin_t *wp));
LOCAL	void	init_csize	__PR((ewin_t *wp));
LOCAL	void	init_ctab	__PR((ewin_t *wp));

LOCAL Uchar *
cmakestr(wp, s)
		ewin_t	*wp;
	register Uchar	*s;
{
		Uchar	*tmp;
	register Uchar	*s1;

	if ((tmp = (Uchar *) malloc(strlen(C s)+1)) == NULL) {
		rsttmodes(wp);
		raisecond("makstr", 0L);
	}
	for (s1 = tmp; (*s1++ = *s++) != '\0'; );
	return (tmp);
}

EXPORT void
init_charset(wp)
	ewin_t	*wp;
{
	init_ctab(wp);
	init_csize(wp);
}

LOCAL void
init_csize(wp)
	ewin_t	*wp;
{
	register unsigned c;
	register Uchar	*rcsize = csize;

	for (c = 0; c <= 255; c++, rcsize++) {
		if (c < SP || c == DEL)			/*ctl */
			*rcsize = 2;
#ifdef	DEL8
		else if ((c > DEL && c < SP8) || c == DEL8) /* 8bit ctl */
#else
		else if (c > DEL && c < SP8)		/* 8bit ctl */
#endif
			*rcsize = 3;
		else if (c >= SP8 && !wp->raw8)		/* 8bit norm */
			*rcsize = 2;
		else					/* normal char */
			*rcsize = 1;
	}
}

LOCAL char chpre[] = " ";
LOCAL char ctlpre[] = "^ ";
LOCAL char eightpre[] = "~ ";
LOCAL char eightctlpre[] = "~^ ";

LOCAL void
init_ctab(wp)
	ewin_t	*wp;
{
	register unsigned c;
	register Uchar	*p;
	register Uchar	**rctab	= ctab;
	register Uchar	*ch	= (Uchar *) chpre;
	register Uchar	*ctl	= (Uchar *) ctlpre;
	register Uchar	*eight	= (Uchar *) eightpre;
	register Uchar	*eightctl = (Uchar *) eightctlpre;

	for (c = 0; c <= 255; c++, rctab++) {
		if (c < SP || c == DEL) {		/* ctl char */
			p = cmakestr(wp, ctl);
			p[1] = c ^ 0100;
#ifdef	DEL8
		} else if ((c > DEL && c < SP8) || c == DEL8) { /* 8 bit ctl */
#else
		} else if (c > DEL && c < SP8) {	/* 8 bit ctl */
#endif
			p = cmakestr(wp, eightctl);
			p[2] = c ^ 0300;
		} else if (c >= SP8 && !wp->raw8) {	/* 8 bit char */
			p = cmakestr(wp, eight);
			p[1] = c & 0177;
		} else {				/* normal char */
			p = cmakestr(wp, ch);
			p[0] = (Uchar)c;
		}
		*rctab = p;
	}
}
