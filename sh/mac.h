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
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


#ident	"@(#)mac.h	1.8	05/06/08 SMI"	/* SVr4.0 1.8	*/

/*
 * This file contains modifications Copyright 2008 J. Schilling
 *
 * @(#)mac.h	1.3 08/03/28 2008 J. Schilling
 */

/*
 *	UNIX shell
 */

#define TRUE	(-1)
#define FALSE	0
#define LOBYTE	0377
#define QUOTE	0200

#undef	EOF	/* may be defined from stdio.h */
#define EOF	0
#define NL	'\n'
#define SPACE	' '
#define LQ	'`'
#define RQ	'\''
#define MINUS	'-'
#define COLON	':'
#define TAB	'\t'

#undef	MAX		/* may be defined from sys/param.h */
#define MAX(a,b)	((a)>(b)?(a):(b))

#define blank()		prc(SPACE)
#define	tab()		prc(TAB)
#define newline()	prc(NL)

