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
 * Copyright 2006-2020 J. Schilling
 *
 * @(#)xpipe.c	1.8 20/09/06 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)xpipe.c 1.8 20/09/06 J. Schilling"
#endif
/*
 * @(#)xpipe.c 1.4 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)xpipe.c"
#pragma ident	"@(#)sccs:lib/mpwlib/xpipe.c"
#endif

#include <defines.h>
#include <i18n.h>

/*
	Interface to pipe(II) which handles all error conditions.
	Returns 0 on success,
	fatal() on failure.
*/


int
xpipe(t)
int *t;
{
       /*
	* Multiply space needed for C locale by 3.  This should be
	* adequate for the longest localized strings.
	*/
	static char p[16];	/* strlen("pipe") * 3 + 1 */

	strcpy(p, (const char *) NOGETTEXT("pipe"));
	if (pipe(t) == 0) {
		setmode(t[0], O_BINARY);
		setmode(t[1], O_BINARY);
		return(0);
	}
	return(xmsg(p,p));
}
