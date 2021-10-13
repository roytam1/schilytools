/* @(#)dirtime.h	1.8 18/06/20 Copyright 1996-2018 J. Schilling */
/*
 *	Prototypes for dirtime users
 *
 *	Copyright (c) 1996-2018 J. Schilling
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

#ifndef	_DIRTIME_H
#define	_DIRTIME_H

#ifndef _STAR_H
#include "star.h"
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * dirtime.c
 */
extern	void	sdirtimes	__PR((char *name, FINFO *info,
						BOOL do_times, BOOL do_mode));
extern	void	sdirmode	__PR((char *name, mode_t mode));
extern	void	flushdirtimes	__PR((void));

#ifdef	__cplusplus
}
#endif

#endif	/* _DIRTIME_H */
