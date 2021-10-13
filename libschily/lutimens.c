/* @(#)lutimens.c	1.4 19/07/31 Copyright 2013-2019 J. Schilling */
/*
 *	Emulate the behavior of lutimens(const char *name,
 *					const struct timespec times[2])
 *
 *	Copyright (c) 2013-2019 J. Schilling
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
 * If we neither have utimensat() nor lutimes()/lutime(), we need to call
 * lstat() and since this is not a predictable call, we always compile this
 * module in largefile mode.
 */
#define	USE_LARGEFILES

#include <schily/unistd.h>
#include <schily/types.h>
#include <schily/time.h>
#include <schily/utime.h>
#include <schily/fcntl.h>
#include <schily/stat.h>
#include <schily/errno.h>
#include <schily/standard.h>
#include <schily/schily.h>

#ifndef	HAVE_LUTIMENS

#ifndef	HAVE_LSTAT
#define	lstat	stat
#endif

EXPORT int
lutimens(name, times)
	const char		*name;
	const struct timespec	times[2];
{
#ifdef	HAVE_UTIMENSAT
	return (utimensat(AT_FDCWD, name, times, AT_SYMLINK_NOFOLLOW));
#else
#ifdef	HAVE_LUTIMES
	struct timeval tv[2];

	if (times == NULL)
		return (lutimes(name, NULL));
	tv[0].tv_sec  = times[0].tv_sec;
	tv[0].tv_usec = times[0].tv_nsec/1000;
	tv[1].tv_sec  = times[1].tv_sec;
	tv[1].tv_usec = times[1].tv_nsec/1000;
	return (lutimes(name, tv));

#else
#ifdef	HAVE_LUTIME
	struct utimbuf	ut;

	if (times == NULL)
		return (lutime(name, NULL));
	ut.actime = times[0].tv_sec;
	ut.modtime = times[1].tv_sec;

	return (lutime(name, &ut));
#else
	/*
	 * Without lstat(), we expect that there are no symlinks either.
	 */
	struct stat	sb;

	if (lstat(name, &sb) >= 0) {
		if (!S_ISLNK(sb.st_mode))
			return (utimens(name, times));
	} else {
		/*
		 * Keep errno (e.g. ENAMETOOLONG)
		 */
		return (-1);
	}
#ifdef	ENOSYS
	seterrno(ENOSYS);
#else
	seterrno(EINVAL);
#endif
	return (-1);
#endif
#endif
#endif
}

#endif	/* HAVE_UTIMENS */
