/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
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
/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */
/*
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2006-2020 J. Schilling
 *
 * @(#)cat.c	1.10 20/09/06 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)cat.c 1.10 20/09/06 J. Schilling"
#endif
/*
 * @(#)cat.c 1.4 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)cat.c"
#pragma ident	"@(#)sccs:lib/mpwlib/cat.c"
#endif
#include	<defines.h>
#include	<schily/varargs.h>

/*
	Concatenate strings.
 
	cat(destination,source1,source2,...,sourcen, (char *)0);
*/

/*VARARGS*/
#ifdef	PROTOTYPES
char *
cat(char *dest, ...)
#else
char *
cat(dest, va_alist)
	char	*dest;
	va_dcl
#endif
{
	register char *d, *s;
	va_list ap;

#ifdef	PROTOTYPES
	va_start(ap, dest);
#else
	va_start(ap);
#endif
	d = dest;

	while ((s = va_arg(ap, char *)) != NULL) {
		while ((*d++ = *s++) != '\0') ;
		d--;
	}
	va_end(ap);
	return (dest);
}
