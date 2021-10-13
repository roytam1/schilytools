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
 * Copyright 1994 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * This file contains modifications Copyright 2006-2009 J. Schilling
 *
 * @(#)trnslat.c	1.6 09/11/08 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)trnslat.c 1.6 09/11/08 J. Schilling"
#endif
/*
 * @(#)trnslat.c 1.3 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)trnslat.c"
#pragma ident	"@(#)sccs:lib/mpwlib/trnslat.c"
#endif
/*
	Copy `str' to `result' replacing any character found
	in both `str' and `old' with the corresponding character from `new'.
	Return `result'.
*/
#include <defines.h>

char *trnslat(str,old,new,result)
register char *str;
char *old, *new, *result;
{
	register char *r, *o;

	for (r = result; (*r = *str++) != '\0'; r++)
		for (o = old; *o; )
			if (*r == *o++) {
				*r = new[o - old -1];
				break;
			}
	return(result);
}
