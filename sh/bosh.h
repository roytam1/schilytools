#ifndef _BOSH_H
#define	_BOSH_H

/*
 * Copyright 2018 J. Schilling
 *
 * @(#)bosh.h	1.2 18/08/01 Copyright 2018 J. Schilling
 *
 * Global variables structure for UNIX Shell
 *
 * WARNING: Add new members at the end or you will risk
 * incompatibilities with older loadable builtin commands.
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

#include <schily/utypes.h>

typedef struct bosh {
	int	intrcnt;			/* Intr ctr for builtin cmds */
	unsigned long	*flagsp;		/* ptr to set(1) flags	    */
	unsigned long	*flagsp2;		/* ptr to set(1) flags	    */
	Uchar **(*get_envptr) __PR((void));	/* Get tmp. env array	    */
	int	(*callsh) __PR((int ac, char **av)); /* callb. for find -call */
} bosh_t;

#endif /* _BOSH_H */
