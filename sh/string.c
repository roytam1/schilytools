/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 1995 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#if defined(sun)
#pragma ident	"@(#)string.c	1.12	05/09/13 SMI"
#endif

/*
 * UNIX shell
 */
#include	"defs.h"

/*
 * Copyright 2008-2017 J. Schilling
 *
 * @(#)string.c	1.19 17/08/27 2008-2017 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)string.c	1.19 17/08/27 2008-2017 J. Schilling";
#endif

/* ========	general purpose string handling ======== */


unsigned char *movstr	__PR((unsigned char *a, unsigned char *b));
int		any	__PR((wchar_t c, unsigned char *s));
int		anys	__PR((unsigned char *c, unsigned char *s));
int		cf	__PR((unsigned char *s1, unsigned char *s2));
int		length	__PR((unsigned char *as));
unsigned char *movstrn	__PR((unsigned char *a, unsigned char *b, int n));

/*
 * strcpy with arguments reversed and a more useful return value
 */
unsigned char *
movstr(a, b)
	unsigned char	*a;
	unsigned char	*b;
{
	while ((*b++ = *a++) != '\0')
		/* LINTED */
		;
	return (--b);
}

/*
 * simpler form of strchr with arguments reversed
 */
int
any(c, s)
	wchar_t		c;
	unsigned char	*s;
{
	unsigned int d;

	while ((d = *s++) != 0) {
		if (d == c)
			return (TRUE);
	}
	return (FALSE);
}

int
anys(c, s)
	unsigned char	*c;
	unsigned char	*s;
{
	wchar_t f, e;
	wchar_t d;
	int n;

	(void) mbtowc(NULL, NULL, 0);
	if ((n = mbtowc(&f, (char *)c, MULTI_BYTE_MAX)) <= 0) {
		(void) mbtowc(NULL, NULL, 0);
		return (FALSE);
	}
	d = f;
	/* CONSTCOND */
	while (1) {
		if ((n = mbtowc(&e, (char *)s, MULTI_BYTE_MAX)) <= 0) {
			(void) mbtowc(NULL, NULL, 0);
			return (FALSE);
		}
		if (e == 0)
			return (FALSE);
		if (d == e)
			return (TRUE);
		s += n;
	}
	/* NOTREACHED */
}

#ifdef	__needed__
int
clen(c)
	unsigned char	*c;
{
	wchar_t		f;
	int		n;

	(void) mbtowc(NULL, NULL, 0);
	if ((n = mbtowc(&f, (char *)c, MULTI_BYTE_MAX)) <= 0) {
		(void) mbtowc(NULL, NULL, 0);
		return (1);
	}
	return (n);
}
#endif

int
cf(s1, s2)
	unsigned char	*s1;
	unsigned char	*s2;
{
	while (*s1++ == *s2)
		if (*s2++ == 0)
			return (0);
	return (*--s1 - *s2);
}

/*
 * return size of as, including terminating NUL
 */
int
length(as)
	unsigned char	*as;
{
	unsigned char	*s;

	if ((s = as) != 0)
		while (*s++)
			/* LINTED */
			;
	return (s - as);
}

unsigned char *
movstrn(a, b, n)
	unsigned char	*a;
	unsigned char	*b;
	int		n;
{
	while ((n-- > 0) && *a)
		*b++ = *a++;

	return (b);
}
