/* @(#)kill.c	1.2 19/09/01 Copyright 2011-2019 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)kill.c	1.2 19/09/01 Copyright 2011-2019 J. Schilling";
#endif
/*
 *	Copyright (c) 2011-2019 J. Schilling
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

#ifndef	HAVE_KILL

#include <schily/standard.h>
#include <schily/types.h>
#include <schily/schily.h>
#include <schily/errno.h>

EXPORT	int		kill __PR((pid_t pid, int sig));

#if	defined(__MINGW32__) || defined(_MSC_VER)
#include <schily/windows.h>
#include <schily/utypes.h>
#include <psapi.h>

#pragma comment(lib, "Psapi.lib")

LOCAL	int		have_pid __PR((pid_t pid));

#define	MAX_PIDS	1024

LOCAL int
have_pid(pid)
	pid_t	pid;
{
	UInt32_t	procs[MAX_PIDS];
	UInt32_t	nents;
	int		i;

	if (pid == 0)
		return (0);

	if (!EnumProcesses(procs, sizeof (procs), &nents)) {
		seterrno(ESRCH);
		return (-1);
	}
	nents /= sizeof (UInt32_t);

	for (i = 0; i < nents; i++) {
		if (pid == procs[i])
			return (0);
	}
	seterrno(ESRCH);
	return (-1);
}

EXPORT int
kill(pid, sig)
	pid_t	pid;
	int	sig;
{
	if (sig != 0) {
		seterrno(ENOSYS);
		return (-1);
	}
	return (have_pid(pid));
}

#else	/* defined(__MINGW32__) || defined(_MSC_VER) */

EXPORT	int
kill(pid, sig)
	pid_t	pid;
	int	sig;
{
#ifdef	ENOSYS
	seterrno(ENOSYS);
#else
	seterrno(EINVAL);
#endif
	return (-1);
}

#endif	/* defined(__MINGW32__) || defined(_MSC_VER) */

#endif	/* HAVE_KILL */
