/* @(#)xutimes.h	1.5 18/10/21 Copyright 1996, 2013-2018 J. Schilling */
/*
 *	Prototypes for xutimes users
 *
 *	Copyright (c) 1996, 2013-2018 J. Schilling
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

/*
 * star_unix.c
 */
extern	int	xutimes		__PR((char *name, struct timespec *tp,
					BOOL asymlink));
