/* @(#)fflags.c	1.27 19/03/19 Copyright 2001-2019 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)fflags.c	1.27 19/03/19 Copyright 2001-2019 J. Schilling";
#endif
/*
 *	Routines to handle extended file flags
 *
 *	Copyright (c) 2001-2019 J. Schilling
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

#ifdef	USE_FFLAGS
#include <schily/stdio.h>
#include <schily/errno.h>
#include "star.h"
#include "props.h"
#include "table.h"
#include <schily/standard.h>
#include <schily/unistd.h>
#include <schily/dirent.h>
#include <schily/string.h>
#include <schily/stat.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include "starsubs.h"
#ifdef	__linux__
#include <schily/fcntl.h>
#ifdef	HAVE_LINUX_FS_H
#include <linux/fs.h>
#endif
#ifdef	HAVE_EXT2FS_EXT2_FS_H
/*
 * The Linux Kernel appears to be unplanned and full of moving targets :-(
 */
#include <ext2fs/ext2_fs.h>

#else	/* !HAVE_EXT2FS_EXT2_FS_H */
#if defined(HAVE_USABLE_EXT2_FS_H) || defined(TRY_EXT2_FS)
/*
 * Be very careful in case that the Linux Kernel maintainers
 * unexpectedly fix the bugs in the Linux Lernel include files.
 * Only introduce the attempt for a workaround in case the include
 * files are broken anyway.
 *
 * If HAVE_USABLE_LINUX_EXT2_FS_H is defined, a simple
 * #include <linux/ex2_fs.h> will work.
 *
 * If TRY_EXT2_FS is defined, we will asume that
 */
#ifdef	TRY_EXT2_FS
#define	__KERNEL__
#ifdef	HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
#define	KBUILD_BASENAME	"foo"
#define	CONFIG_X86_L1_CACHE_SHIFT 7
#ifdef	HAVE_LINUX_GFP_H
#include <linux/gfp.h>
#endif
#ifdef	HAVE_ASM_TYPES_H
#include <asm/types.h>
#endif
#undef	__KERNEL__
#endif	/* TRY_EXT2_FS */
#include <linux/ext2_fs.h>
#endif	/* defined(HAVE_USABLE_EXT2_FS_H) || defined(TRY_EXT2_FS) */
#endif	/* HAVE_EXT2FS_EXT2_FS_H */
#include <schily/ioctl.h>

#if	!defined(FS_IOC_GETFLAGS) && defined(EXT2_IOC_GETFLAGS)
#define	FS_IOC_GETFLAGS	EXT2_IOC_GETFLAGS
#endif
#if	!defined(FS_IOC_SETFLAGS) && defined(EXT2_IOC_SETFLAGS)
#define	FS_IOC_SETFLAGS	EXT2_IOC_SETFLAGS
#endif

#if	!defined(FS_APPEND_FL) && defined(EXT2_APPEND_FL)
#define	FS_APPEND_FL	EXT2_APPEND_FL
#endif
#if	!defined(FS_COMPR_FL) && defined(EXT2_COMPR_FL)
#define	FS_COMPR_FL	EXT2_COMPR_FL
#endif
#if	!defined(FS_IMMUTABLE_FL) && defined(EXT2_IMMUTABLE_FL)
#define	FS_IMMUTABLE_FL	EXT2_IMMUTABLE_FL
#endif
#if	!defined(FS_NOATIME_FL) && defined(EXT2_NOATIME_FL)
#define	FS_NOATIME_FL	EXT2_NOATIME_FL
#endif
#if	!defined(FS_NODUMP_FL) && defined(EXT2_NODUMP_FL)
#define	FS_NODUMP_FL	EXT2_NODUMP_FL
#endif
#if	!defined(FS_SECRM_FL) && defined(EXT2_SECRM_FL)
#define	FS_SECRM_FL	EXT2_SECRM_FL
#endif
#if	!defined(FS_SYNC_FL) && defined(EXT2_SYNC_FL)
#define	FS_SYNC_FL	EXT2_SYNC_FL
#endif
#if	!defined(FS_UNRM_FL) && defined(EXT2_UNRM_FL)
#define	FS_UNRM_FL	EXT2_UNRM_FL
#endif

#if	!defined(FS_DIRSYNC_FL) && defined(EXT2_DIRSYNC_FL)
#define	FS_DIRSYNC_FL	EXT2_DIRSYNC_FL
#endif
#if	!defined(FS_JOURNAL_DATA_FL) && defined(EXT3_JOURNAL_DATA_FL)
#define	FS_JOURNAL_DATA_FL	EXT3_JOURNAL_DATA_FL
#endif
#if	!defined(FS_NOCOW_FL) && defined(EXT2_NOCOW_FL)
#define	FS_NOCOW_FL	EXT2_NOCOW_FL
#endif
#if	!defined(FS_NOCOW_FL) && defined(EXT2_NOCOW_FL)
#define	FS_NOCOW_FL	EXT2_NOCOW_FL
#endif
#if	!defined(FS_NOTAIL_FL) && defined(EXT2_NOTAIL_FL)
#define	FS_NOTAIL_FL	EXT2_NOTAIL_FL
#endif
#if	!defined(FS_PROJINHERIT_FL) && defined(EXT2_PROJINHERIT_FL)
#define	FS_PROJINHERIT_FL	EXT22_PROJINHERIT_FL
#endif
#if	!defined(FS_TOPDIR_FL) && defined(EXT2_TOPDIR_FL)
#define	FS_TOPDIR_FL	EXT2_TOPDIR_FL
#endif

#endif	/* __linux__ */

EXPORT	void	opt_fflags	__PR((void));
EXPORT	void	get_fflags	__PR((FINFO *info));
EXPORT	void	set_fflags	__PR((FINFO *info));
EXPORT	char	*textfromflags	__PR((FINFO *info, char *buf));
EXPORT	int	texttoflags	__PR((FINFO *info, char *buf));

EXPORT void
opt_fflags()
{
	/*	Linux							*BSD */
#if (defined(__linux__) && defined(FS_IOC_GETFLAGS)) || defined(HAVE_ST_FLAGS)
	printf(" fflags");
#endif
}

EXPORT void
get_fflags(info)
	register FINFO	*info;
{
#if defined(__linux__) && defined(FS_IOC_GETFLAGS)
	int	f;
	long	l = 0L;

	if ((f = open(info->f_name, O_RDONLY|O_NDELAY)) >= 0) {
		if (ioctl(f, FS_IOC_GETFLAGS, &l) >= 0) {
			info->f_fflags = l;
			if ((l & FS_NODUMP_FL) != 0)
				info->f_flags |= F_NODUMP;
			if (info->f_fflags != 0)
				info->f_xflags |= XF_FFLAGS;
		} else {
			info->f_fflags = 0L;
		}
		close(f);
	}
#else	/* !__linux__ */
	info->f_fflags = 0L;
#endif
}

EXPORT void
set_fflags(info)
	register FINFO	*info;
{
/*
 * Be careful: True64 includes a chflags() stub but no #defines for st_flags
 */
#if	defined(HAVE_CHFLAGS) && defined(UF_SETTABLE)
	char	buf[512];
	BOOL	err = TRUE;

	/*
	 * As for 14.2.2002 the man page of chflags() is wrong, the following
	 * code is a result of kernel source study.
	 * If we are not allowed to set the flags, try to only set the user
	 * settable flags.
	 */
	if (chflags(info->f_name, info->f_fflags) >= 0)
		err = FALSE;
	else if (geterrno() == EPERM &&
	    chflags(info->f_name, info->f_fflags & UF_SETTABLE) >= 0)
		err = FALSE;

	if (err)
		errmsg("Cannot set file flags '%s' for '%s'.\n",
				textfromflags(info, buf), info->f_name);
#else
#if defined(__linux__) && defined(FS_IOC_GETFLAGS)
	char	buf[512];
	int	f;
	Ulong	flags;
	Ulong	oldflags;
	BOOL	err = TRUE;
	/*
	 * Linux bites again! There is no define for the flags that are only
	 * settable by the root user.
	 */
#ifdef	FS_JOURNAL_DATA_FL			/* 'j' */
#define	SF_MASK		(FS_IMMUTABLE_FL|FS_APPEND_FL|FS_JOURNAL_DATA_FL)
#else
#define	SF_MASK		(FS_IMMUTABLE_FL|FS_APPEND_FL)
#endif

	if ((f = open(info->f_name, O_RDONLY|O_NONBLOCK)) >= 0) {
		if (ioctl(f, FS_IOC_SETFLAGS, &info->f_fflags) >= 0) {
			err = FALSE;

		} else if (geterrno() == EPERM) {
			if (ioctl(f, FS_IOC_GETFLAGS, &oldflags) >= 0) {

				flags	 =  info->f_fflags & ~SF_MASK;
				oldflags &= SF_MASK;
				flags	 |= oldflags;
				if (ioctl(f, FS_IOC_SETFLAGS, &flags) >= 0)
					err = FALSE;
			}
		}
		close(f);
	}
	if (err)
		errmsg("Cannot set file flags '%s' for '%s'.\n",
				textfromflags(info, buf), info->f_name);
#endif

#endif
}


/*
 * The first entry for a specific flag is used as output for the TAR archive.
 * Other entries with the same flag are recognized as aliases.
 *
 * UF_* flags are flags settable by any user, defied by *BSD
 * SF_* flags are *BSD flags settable obly be the super user
 * FS_* flags are Linux specific.
 */
LOCAL struct {
	char	*name;
	Ulong	flag;
} flagnames[] = {
	/* shorter names per flag first, all prefixed by "no" */
	/* Super user settable flags first */
#ifdef	SF_APPEND
	{ "sappnd",		SF_APPEND },
	{ "sappend",		SF_APPEND },
#endif

#ifdef	FS_APPEND_FL				/* 'a' */
	{ "sappnd",		FS_APPEND_FL },
	{ "sappend",		FS_APPEND_FL },
#endif

#ifdef	SF_ARCHIVED
	{ "arch",		SF_ARCHIVED },
	{ "archived",		SF_ARCHIVED },
#endif

#ifdef	SF_IMMUTABLE
	{ "schg",		SF_IMMUTABLE },
	{ "schange",		SF_IMMUTABLE },
	{ "simmutable",		SF_IMMUTABLE },
#endif
#ifdef	FS_IMMUTABLE_FL			/* 'i' */
	{ "schg",		FS_IMMUTABLE_FL },
	{ "schange",		FS_IMMUTABLE_FL },
	{ "simmutable",		FS_IMMUTABLE_FL },
#endif

#ifdef	SF_NOUNLINK
	{ "sunlnk",		SF_NOUNLINK },
	{ "sunlink",		SF_NOUNLINK },
#endif

#ifdef	FS_JOURNAL_DATA_FL			/* 'j' */
	{ "journal-data",	FS_JOURNAL_DATA_FL },
	{ "journal",		FS_JOURNAL_DATA_FL }, /* undoc libarchive al. */
#endif

/* ------------------------------------------------------------------------- */

	/* The following flags may be set without being super user. */
#ifdef	UF_APPEND
	{ "uappnd",		UF_APPEND },
	{ "uappend",		UF_APPEND },
#endif

#ifdef	UF_IMMUTABLE
	{ "uchg",		UF_IMMUTABLE },
	{ "uchange",		UF_IMMUTABLE },
	{ "uimmutable",		UF_IMMUTABLE },
#endif

#ifdef	FS_COMPR_FL				/* 'c' */
	{ "compress",		FS_COMPR_FL },
#endif

#ifdef	FS_NOATIME_FL				/* 'A' */
	{ "noatime",		FS_NOATIME_FL },
#endif

#ifdef	UF_NODUMP
	{ "nodump",		UF_NODUMP },
#endif
#ifdef	FS_NODUMP_FL				/* 'd' */
	{ "nodump",		FS_NODUMP_FL },
#endif

#ifdef	UF_OPAQUE
	{ "opaque",		UF_OPAQUE },
#endif

#ifdef	FS_SECRM_FL				/* 's' Purge before unlink */
	{ "secdel",		FS_SECRM_FL },
	{ "securedeletion",	FS_SECRM_FL },	/* undoc'd libarchive alias */
#endif

#ifdef	FS_SYNC_FL				/* 'S' */
	{ "sync",		FS_SYNC_FL },
#endif

#ifdef	FS_UNRM_FL				/* 'u' Allow to 'unrm' file the */
	{ "undel",		FS_UNRM_FL },
#endif

#ifdef	UF_NOUNLINK
	{ "uunlnk",		UF_NOUNLINK },
	{ "uunlink",		UF_NOUNLINK },
#endif

#ifdef	UF_COMPRESSED
	{ "compressed",		UF_COMPRESSED },
	{ "ucompressed",	UF_COMPRESSED },
#endif
#ifdef	UF_HIDDEN
	{ "hidden",		UF_HIDDEN },
	{ "uhidden",		UF_HIDDEN },
#endif
#ifdef	UF_OFFLINE
	{ "offline",		UF_OFFLINE },
	{ "uoffline",		UF_OFFLINE },
#endif
#ifdef	UF_READONLY
	{ "rdonly",		UF_READONLY },
	{ "urdonly",		UF_READONLY },
	{ "readonly",		UF_READONLY },
#endif
#ifdef	UF_REPARSE
	{ "reparse",		UF_REPARSE },
	{ "ureparse",		UF_REPARSE },
#endif
#ifdef	UF_SPARSE
	{ "sparse",		UF_SPARSE },
	{ "usparse",		UF_SPARSE },
#endif
#ifdef	UF_SYSTEM
	{ "system",		UF_SYSTEM },
	{ "usystem",		UF_SYSTEM },
#endif

#ifdef	FS_DIRSYNC_FL				/* 'D' */
	{ "dirsync",		FS_DIRSYNC_FL },
#endif
#ifdef	FS_NOCOW_FL				/* 'C' */
	{ "nocow",		FS_NOCOW_FL },
#endif
#ifdef	FS_NOTAIL_FL				/* 't' */
	{ "notail",		FS_NOTAIL_FL },
#endif
#ifdef	FS_PROJINHERIT_FL			/* 'P' */
	{ "projinherit",	FS_PROJINHERIT_FL },
#endif
#ifdef	FS_TOPDIR_FL				/* 'T' */
	{ "topdir",		FS_TOPDIR_FL },
#endif

	{ NULL,			0 }
};
#define	nflagnames	((sizeof (flagnames) / sizeof (flagnames[0])) -1)

/*
 * With 32 bits for flags and 512 bytes for the text buffer any name
 * for a single flag may be <= 16 bytes.
 */
EXPORT char *
textfromflags(info, buf)
	register FINFO	*info;
	register char	*buf;
{
	register Ulong	flags = info->f_fflags;
	register char	*n;
	register char	*p;
	register int	i;

	buf[0] = '\0';
	p = buf;

	for (i = 0; i < nflagnames; i++) {
		if (flags & flagnames[i].flag) {
			flags &= ~flagnames[i].flag;
			if (p != buf)
				*p++ = ',';
			for (n = flagnames[i].name; *n; *p++ = *n++)
				;
		}
	}
	*p = '\0';
	return (buf);
}

EXPORT int
texttoflags(info, buf)
	register FINFO	*info;
	register char	*buf;
{
	register char	*p;
	register char	*sep;
	register int	i;
	register Ulong	flags = 0;

	p = buf;

	while (*p) {
		if ((sep = strchr(p, ',')) != NULL)
			*sep = '\0';

		for (i = 0; i < nflagnames; i++) {
			if (streql(flagnames[i].name, p)) {
				flags |= flagnames[i].flag;
				break;
			}
		}
#ifdef	nonono
		if (i == nflagnames) {
			not found!
		}
#endif
		if (sep != NULL) {
			*sep++ = ',';
			p = sep;
		} else {
			break;
		}
	}
	info->f_fflags = flags;
	return (0);
}

#endif  /* USE_FFLAGS */
