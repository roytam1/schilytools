/* @(#)job.c	1.2 11/11/14 Copyright 2009-2011 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)job.c	1.2 11/11/14 Copyright 2009-2011 J. Schilling";
#endif
/*
 *	Copyright (c) 2009-2011 J. Schilling
 */
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

#include "make.h"
#include "job.h"
#include <schily/schily.h>

job *
newjob()
{
	job	*new = ___malloc(sizeof (job), "newjob");

	return (new);
}