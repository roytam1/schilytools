/* @(#)star_unix.c	1.120 19/11/24 Copyright 1985, 1995, 2001-2019 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)star_unix.c	1.120 19/11/24 Copyright 1985, 1995, 2001-2019 J. Schilling";
#endif
/*
 *	Stat / mode / owner routines for unix like
 *	operating systems
 *
 *	Copyright (c) 1985, 1995, 2001-2019 J. Schilling
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

#include <schily/mconfig.h>
#ifndef	HAVE_UTIMES
#define	utimes	__nothing__	/* BeOS has no utimes() but wrong prototype */
#endif
#include <schily/stdio.h>
#include <schily/errno.h>
#include "star.h"
#include "props.h"
#include "table.h"
#include <schily/standard.h>
#include <schily/unistd.h>
#include <schily/dirent.h>
#include <schily/fcntl.h>	/* For AT_FDCWD */
#include <schily/stat.h>
#include <schily/param.h>	/* For DEV_BSIZE */
#include <schily/device.h>
#include <schily/string.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include "dirtime.h"
#include "xutimes.h"
#ifndef	HAVE_UTIMES
#undef	utimes
#endif
#include "starsubs.h"
#include "checkerr.h"

#ifndef	HAVE_LSTAT
#define	lstat	stat
#undef	AT_SYMLINK_NOFOLLOW
#define	AT_SYMLINK_NOFOLLOW	0
#endif
#ifndef	HAVE_LCHOWN
#define	lchown	chown
#endif

#if	S_ISUID == TSUID && S_ISGID == TSGID && S_ISVTX == TSVTX && \
	S_IRUSR == TUREAD && S_IWUSR == TUWRITE && S_IXUSR == TUEXEC && \
	S_IRGRP == TGREAD && S_IWGRP == TGWRITE && S_IXGRP == TGEXEC && \
	S_IROTH == TOREAD && S_IWOTH == TOWRITE && S_IXOTH == TOEXEC

#define	HAVE_POSIX_MODE_BITS	/* st_mode bits are equal to TAR mode bits */
#endif

#define	ROOT_UID	0

extern	uid_t	my_uid;
extern	dev_t	curfs;
extern	BOOL	noatime;
extern	BOOL	nomtime;
extern	BOOL	nochown;
extern	BOOL	pflag;
extern	BOOL	follow;
extern	BOOL	paxfollow;
extern	BOOL	nodump;
extern	BOOL	doacl;
extern	BOOL	doxattr;
extern	BOOL	dolxattr;
extern	BOOL	dofflags;

EXPORT	BOOL	_getinfo	__PR((char *name, FINFO *info));
EXPORT	BOOL	_lgetinfo	__PR((char *name, FINFO *info));
EXPORT	BOOL	getinfo		__PR((char *name, FINFO *info));
#ifdef	HAVE_FSTATAT
EXPORT	BOOL	getinfoat	__PR((int fd, char *name, FINFO *info));
#endif
EXPORT	BOOL	getstat		__PR((char *name, struct stat *sp));
EXPORT	BOOL	stat_to_info	__PR((int fd, struct stat *sp, FINFO *info));
LOCAL	void	print_badnsec	__PR((FINFO *info, char *name, long val));
EXPORT	void	checkarch	__PR((FILE *f));
EXPORT	BOOL	archisnull	__PR((const char *name));
EXPORT	BOOL	samefile	__PR((FILE *fp1, FILE *fp2));
EXPORT	void	setmodes	__PR((FINFO *info));
LOCAL	int	sutimes		__PR((char *name, FINFO *info, BOOL asymlink));
EXPORT	int	snulltimes	__PR((char *name, FINFO *info));
EXPORT	int	sxsymlink	__PR((char *name, FINFO *info));
EXPORT	int	rs_acctime	__PR((int fd, FINFO *info));
EXPORT	void	setdirmodes	__PR((char *name, mode_t mode));
EXPORT	mode_t	osmode		__PR((mode_t tarmode));
#ifdef	HAVE_POSIX_MODE_BITS	/* st_mode bits are equal to TAR mode bits */
#else
LOCAL	int	dolchmodat	__PR((char *name, mode_t tarmode, int flag));
#endif

#ifdef	HAVE_POSIX_MODE_BITS	/* st_mode bits are equal to TAR mode bits */
#else
#define	lchmodat dolchmodat
#endif

#ifdef	USE_ACL

#ifdef	OWN_ACLTEXT
#if	defined(UNIXWARE) && defined(HAVE_ACL)
#define	HAVE_SUN_ACL
#define	HAVE_ANY_ACL
#endif
#endif
/*
 * HAVE_ANY_ACL currently includes HAVE_POSIX_ACL and HAVE_SUN_ACL.
 * This definition must be in sync with the definition in acl_unix.c
 * As USE_ACL is used in star.h, we are not allowed to change the
 * value of USE_ACL before we did include star.h or we may not include
 * star.h at all.
 * HAVE_HP_ACL is currently not included in HAVE_ANY_ACL.
 */
#ifndef	HAVE_ANY_ACL
#undef	USE_ACL		/* Do not try to get or set ACLs */
#endif
#endif

/*
 * Simple getinfo() variant.
 */
EXPORT BOOL
_getinfo(name, info)
	char	*name;
	register FINFO	*info;
{
	BOOL	ret;
	BOOL	odoacl = doacl;
	BOOL	odoxattr = doxattr;
	BOOL	odolxattr = dolxattr;

	doacl	= FALSE;
	doxattr	= FALSE;
	ret = getinfo(name, info);
	doacl	= odoacl;
	doxattr	= odoxattr;
	dolxattr = odolxattr;

	return (ret);
}

/*
 * Simple getinfo() variant using lstat()
 */
EXPORT BOOL
_lgetinfo(name, info)
	char	*name;
	register FINFO	*info;
{
	BOOL	ret;
	BOOL	ofollow = follow;
	BOOL	opaxfollow = paxfollow;

	/*
	 * Always use lstat()
	 */
	follow = FALSE;
	paxfollow = FALSE;
	ret = _getinfo(name, info);
	follow = ofollow;
	paxfollow = opaxfollow;

	return (ret);
}

EXPORT BOOL
getinfo(name, info)
	char	*name;
	register FINFO	*info;
{
	struct stat	stbuf;

	info->f_filetype = -1;	/* Will be overwritten if stat() works */
newstat:
	if (paxfollow) {
		if (lstatat(name, &stbuf, 0 /* stat */) < 0) {
			if (geterrno() == EINTR)
				goto newstat;
			if (geterrno() != ENOENT)
				return (FALSE);

			while (lstatat(name, &stbuf, AT_SYMLINK_NOFOLLOW) < 0) {
				if (geterrno() != EINTR)
					return (FALSE);
			}
		}
	} else if (lstatat(name, &stbuf, follow?0:AT_SYMLINK_NOFOLLOW) < 0) {
		if (geterrno() == EINTR)
			goto newstat;
		return (FALSE);
	}
	info->f_sname = info->f_name = name;
	return (stat_to_info(AT_FDCWD, &stbuf, info));
}

#ifdef	HAVE_FSTATAT
EXPORT BOOL
getinfoat(fd, name, info)
	int	fd;
	char	*name;
	register FINFO	*info;
{
	struct stat	stbuf;

	info->f_filetype = -1;	/* Will be overwritten if stat() works */
newstat:
	if (paxfollow) {
		if (fstatat(fd, name, &stbuf, 0 /* stat */) < 0) {
			if (geterrno() == EINTR)
				goto newstat;
			if (geterrno() != ENOENT)
				return (FALSE);

			while (fstatat(fd, name, &stbuf, AT_SYMLINK_NOFOLLOW) < 0) {
				if (geterrno() != EINTR)
					return (FALSE);
			}
		}
	} else if (fstatat(fd, name, &stbuf, follow?0:AT_SYMLINK_NOFOLLOW) < 0) {
		if (geterrno() == EINTR)
			goto newstat;
		return (FALSE);
	}
	info->f_sname = info->f_name = name;
	return (stat_to_info(fd, &stbuf, info));
}
#endif

EXPORT BOOL
getstat(name, sp)
	char		*name;
	struct stat	*sp;
{
newstat:
	if (paxfollow) {
		if (lstatat(name, sp, 0 /* stat */) < 0) {
			if (geterrno() == EINTR)
				goto newstat;
			if (geterrno() != ENOENT)
				return (FALSE);

			while (lstatat(name, sp, AT_SYMLINK_NOFOLLOW) < 0) {
				if (geterrno() != EINTR)
					return (FALSE);
			}
		}
	} else if (lstatat(name, sp, follow?0:AT_SYMLINK_NOFOLLOW) < 0) {
		if (geterrno() == EINTR)
			goto newstat;
		return (FALSE);
	}
	return (TRUE);
}

EXPORT BOOL
stat_to_info(fd, sp, info)
	int	fd;
	struct stat *sp;
	FINFO	*info;
{
	BOOL		first = TRUE;

again:
	info->f_uname = info->f_gname = NULL;
	info->f_umaxlen = info->f_gmaxlen = 0L;
	info->f_dev	= sp->st_dev;
	if (curfs == NODEV)
		curfs = info->f_dev;
	info->f_ino	= sp->st_ino;
	info->f_nlink	= sp->st_nlink;
#ifdef	HAVE_POSIX_MODE_BITS	/* st_mode bits are equal to TAR mode bits */
	info->f_mode	= sp->st_mode & 07777;
#else
	/*
	 * The unexpected case when the S_I* OS mode bits do not match
	 * the T* mode bits from tar.
	 */
	{ register mode_t m = sp->st_mode;

	info->f_mode	=    ((m & S_ISUID ? TSUID   : 0)
			    | (m & S_ISGID ? TSGID   : 0)
			    | (m & S_ISVTX ? TSVTX   : 0)
			    | (m & S_IRUSR ? TUREAD  : 0)
			    | (m & S_IWUSR ? TUWRITE : 0)
			    | (m & S_IXUSR ? TUEXEC  : 0)
			    | (m & S_IRGRP ? TGREAD  : 0)
			    | (m & S_IWGRP ? TGWRITE : 0)
			    | (m & S_IXGRP ? TGEXEC  : 0)
			    | (m & S_IROTH ? TOREAD  : 0)
			    | (m & S_IWOTH ? TOWRITE : 0)
			    | (m & S_IXOTH ? TOEXEC  : 0));
	}
#endif
	info->f_uid	 = sp->st_uid;
	info->f_gid	 = sp->st_gid;
	info->f_size	 = (off_t)0;	/* Size of file */
	info->f_rsize	 = (off_t)0;	/* Size on tape */
	info->f_flags	 = 0L;
	info->f_xflags	 = props.pr_xhdflags;
	info->f_typeflag = 0;
	info->f_type	 = sp->st_mode & ~07777;

	if (sizeof (sp->st_rdev) == sizeof (short)) {
		info->f_rdev = (Ushort) sp->st_rdev;
	} else if ((sizeof (int) != sizeof (long)) &&
			(sizeof (sp->st_rdev) == sizeof (int))) {
		info->f_rdev = (Uint) sp->st_rdev;
	} else {
		info->f_rdev = (Ulong) sp->st_rdev;
	}
	info->f_rdevmaj	= major(info->f_rdev);
	info->f_rdevmin	= minor(info->f_rdev);
	info->f_atime	= sp->st_atime;
	info->f_mtime	= sp->st_mtime;
	info->f_ctime	= sp->st_ctime;

	info->f_ansec	= stat_ansecs(sp);
	info->f_mnsec	= stat_mnsecs(sp);
	info->f_cnsec	= stat_cnsecs(sp);

	info->f_timeres	= 1;

#if	defined(_FOUND_STAT_NSECS_)
	info->f_flags	|= F_NSECS;
#ifdef	HAVE_ST_FSTYPE
	if (sp->st_fstype[0] == 'p' && streql(sp->st_fstype, "pcfs"))
		info->f_flags &= ~F_NSECS;
	if (sp->st_fstype[0] == 'u' &&
	    sp->st_fstype[1] == 'f' &&
	    sp->st_fstype[2] == 's')
		info->f_timeres = 1000;
#endif
#endif

	if (info->f_ansec < 0 || info->f_ansec >= 1000000000L) {
		print_badnsec(info, "atime", info->f_ansec);
		info->f_ansec = 0;
	}
	if (info->f_mnsec < 0 || info->f_mnsec >= 1000000000L) {
		print_badnsec(info, "mtime", info->f_mnsec);
		info->f_mnsec = 0;
	}
	if (info->f_cnsec < 0 || info->f_cnsec >= 1000000000L) {
		print_badnsec(info, "ctime", info->f_cnsec);
		info->f_cnsec = 0;
	}

#ifdef	HAVE_ST_FLAGS
	/*
	 * The *BSD based method is easy to handle.
	 */
	if (dofflags)
		info->f_fflags = sp->st_flags;
	else
		info->f_fflags = 0L;
	if (info->f_fflags != 0)
		info->f_xflags |= XF_FFLAGS;
#ifdef	UF_NODUMP				/* The *BSD 'nodump' flag */
	if ((sp->st_flags & UF_NODUMP) != 0)
		info->f_flags |= F_NODUMP;	/* Convert it to star flag */
#endif
#else	/* !HAVE_ST_FLAGS */
	/*
	 * The non *BSD case...
	 */
#ifdef	USE_FFLAGS
	if ((nodump || dofflags) && !S_ISLNK(sp->st_mode)) {
		get_fflags(info);
		if (!dofflags)
			info->f_xflags &= ~XF_FFLAGS;
	} else {
		info->f_fflags = 0L;
	}
#else
	info->f_fflags = 0L;
#endif
#endif

#ifdef	HAVE_ST_ACLCNT
	info->f_aclcnt = sp->st_aclcnt;
#endif

	switch ((int)(sp->st_mode & S_IFMT)) {

	case S_IFREG:	/* regular file */
			info->f_filetype = F_FILE;
			info->f_xftype = XT_FILE;
			info->f_rsize = info->f_size = sp->st_size;
			info->f_rdev = 0;
			info->f_rdevmaj	= 0;
			info->f_rdevmin	= 0;
			break;
#ifdef	S_IFCTG
	case S_IFCTG:	/* contiguous file */
			info->f_filetype = F_FILE;
			info->f_xftype = XT_CONT;
			info->f_rsize = info->f_size = sp->st_size;
			info->f_rdev = 0;
			info->f_rdevmaj	= 0;
			info->f_rdevmin	= 0;
			break;
#endif
	case S_IFDIR:	/* directory */
			info->f_filetype = F_DIR;
			info->f_xftype = XT_DIR;
			info->f_rdev = 0;
			info->f_rdevmaj	= 0;
			info->f_rdevmin	= 0;
			info->f_dir = NULL;	/* No directory content known */
			info->f_dirinos = NULL;	/* No inode list known */
			info->f_dirlen = 0;
			info->f_dirents = 0;
			break;
#ifdef	S_IFLNK
	case S_IFLNK:	/* symbolic link */
			info->f_filetype = F_SLINK;
			info->f_xftype = XT_SLINK;
			info->f_rdev = 0;
			info->f_rdevmaj	= 0;
			info->f_rdevmin	= 0;
			break;
#endif
#ifdef	S_IFCHR
	case S_IFCHR:	/* character special */
			info->f_filetype = F_SPEC;
			info->f_xftype = XT_CHR;
			break;
#endif
#ifdef	S_IFBLK
	case S_IFBLK:	/* block special */
			info->f_filetype = F_SPEC;
			info->f_xftype = XT_BLK;
			break;
#endif
#ifdef	S_IFIFO
	case S_IFIFO:	/* fifo */
			info->f_filetype = F_SPEC;
			info->f_xftype = XT_FIFO;
			info->f_rdev = 0;
			info->f_rdevmaj	= 0;
			info->f_rdevmin	= 0;
			break;
#endif
#ifdef	S_IFSOCK
	case S_IFSOCK:	/* socket */
			info->f_filetype = F_SPEC;
			info->f_xftype = XT_SOCK;
			break;
#endif
#ifdef	S_IFNAM
	case S_IFNAM:	/* Xenix named file */
			info->f_filetype = F_SPEC;

			/*
			 * 'st_rdev' field is really the subtype
			 * As S_INSEM & S_INSHD we may safely cast
			 * sp->st_rdev to int.
			 */
			switch ((int)sp->st_rdev) {
			case S_INSEM:
				info->f_xftype = XT_NSEM;
				break;
			case S_INSHD:
				info->f_xftype = XT_NSHD;
				break;
			default:
				info->f_xftype = XT_BAD;
				break;
			}
			break;
#endif
#ifdef	S_IFMPC
	case S_IFMPC:	/* multiplexed character special */
			info->f_filetype = F_SPEC;
			info->f_xftype = XT_MPC;
			break;
#endif
#ifdef	S_IFMPB
	case S_IFMPB:	/* multiplexed block special */
			info->f_filetype = F_SPEC;
			info->f_xftype = XT_MPB;
			break;
#endif
#ifdef	S_IFDOOR
	case S_IFDOOR:	/* door */
			info->f_filetype = F_SPEC;
			info->f_xftype = XT_DOOR;
			break;
#endif
#ifdef	S_IFWHT
	case S_IFWHT:	/* whiteout */
			info->f_filetype = F_SPEC;
			info->f_xftype = XT_WHT;
			break;
#endif

	default:	/* any other unknown file type */
			if ((sp->st_mode & S_IFMT) == 0) {
				/*
				 * If we come here, we either did not yet
				 * realize that somebody created a new file
				 * type with a value of 0 or the system did
				 * return an "unallocated file" with lstat().
				 * The latter happens if we are on old Solaris
				 * systems that did not yet add SOCKETS again.
				 * if somebody mounted a filesystem that
				 * has been written with a *BSD system like
				 * SunOS 4.x and this FS holds a socket we get
				 * a pseudo unallocated file...
				 */
				info->f_filetype = F_SPEC;	/* ??? */
				info->f_xftype = XT_NONE;
			} else {
				/*
				 * We don't know this file type!
				 */
				info->f_filetype = F_SPEC;
				info->f_xftype = XT_BAD;
			}
	}
	info->f_rxftype = info->f_xftype;

	if (first && pr_unsuptype(info)) {
		first = FALSE;
		if (fd == AT_FDCWD) {
			/*
			 * Cannot use fstatat() as this may be a very long
			 * path name.
			 */
			if (lstatat(info->f_sname, sp, AT_SYMLINK_NOFOLLOW) < 0)
				return (FALSE);
		} else {
			if (fstatat(fd, info->f_sname, sp, AT_SYMLINK_NOFOLLOW) < 0)
				return (FALSE);
		}
		goto again;
	}

#ifdef	HAVE_ST_BLOCKS
/*
 * The blocking factor for st_blocks is DEV_BSIZE fro sys/param.h
 */
#if	defined(hpux) || defined(__hpux)
	if (info->f_size > (sp->st_blocks * 1024 + 1024)) {
#else
	if (info->f_size > (sp->st_blocks * 512 + 512)) {
#endif
		info->f_flags |= F_SPARSE;

#ifdef	__no_longer__
		/*
		 * Some filesystems do not allocate disk space for files that
		 * consist of one hole and no written data.
		 * If we are on a platform that does not support to read hole
		 * lists for sparse files, this allows to avoid wasting time
		 * reading through the whole file.
		 *
		 * In October 2013, it turned out that at least NetAPP stores
		 * files up to 64 bytes in the inode and then returns
		 * sp->st_blocks == 0 for a non sparse file. We only come here
		 * if the file size is > DEV_BSIZE in hope that noone will
		 * implement a filesystem that hides larger amount of data
		 * without supporting SEEK_HOLE.
		 *
		 * Update: There seems to be a major problem in btrfs:
		 * There was a report that btrfs reports sp->st_blocks == 0
		 * for a file with an 8 GB hole followed by 512 bytes of 'A'.
		 * While this is most likely a btrfs bug, in theory a
		 * filesystem could compress the data past the hole and hold
		 * the compressed data inside the inode. As a result, we needed
		 * to disable the F_ALL_HOLE check.
		 */
		if ((info->f_size > 0) && (sp->st_blocks == 0))
			info->f_flags |= F_ALL_HOLE;
#endif
	}
#endif

	/*
	 * Only look for ACL's and other properties that go into the POSIX
	 * extended heaer if extended headers are allowed with the current
	 * archive format. Also don't include ACL's and other properties if
	 * we are creating a Sun vendor unique variant of the extended headers.
	 * Sun's tar will not grok access control lists and other extensions.
	 */
	if ((props.pr_flags & PR_XHDR) == 0 || (props.pr_xc != 'x'))
		return (TRUE);
#ifdef	USE_ACL
	/*
	 * If we return (FALSE) here, the file would not be archived at all.
	 * This is not what we want, so ignore return code from get_acls().
	 */

	/*
	 * Note: ACL check/fetch has been moved to create.c::take_file()
	 * for performance reasons.
	 */
#endif  /* USE_ACL */

#ifdef	USE_XATTR
	/*
	 * Note: Linux xattr check/fetch has been moved to create.c::take_file()
	 * for performance reasons.
	 */
#endif

	return (TRUE);
}

LOCAL void
print_badnsec(info, name, val)
	FINFO	*info;
	char	*name;
	long	val;
{
	errmsgno(EX_BAD, "Bad '%s' nanosecond value %ld for '%s'.\n",
		name, val, info->f_name);
}


EXPORT void
checkarch(f)
	FILE	*f;
{
	struct stat	stbuf;
	extern	dev_t	tape_dev;
	extern	ino_t	tape_ino;
	extern	BOOL	tape_isreg;

	tape_isreg = FALSE;
	tape_dev = (dev_t)0;
	tape_ino = (ino_t)0;

	if (fstat(fdown(f), &stbuf) < 0)
		return;

	if (S_ISREG(stbuf.st_mode)) {
		tape_dev = stbuf.st_dev;
		tape_ino = stbuf.st_ino;
		tape_isreg = TRUE;
	} else if (((stbuf.st_mode & S_IFMT) == 0) ||
			S_ISFIFO(stbuf.st_mode) ||
			S_ISSOCK(stbuf.st_mode)) {
		/*
		 * This is a pipe or similar on different UNIX implementations.
		 * (stbuf.st_mode & S_IFMT) == 0 may happen in stange cases.
		 */
		tape_dev = NODEV;
		tape_ino = (ino_t)-1;
	}
}

EXPORT BOOL
archisnull(name)
	const char	*name;
{
	struct stat	stbuf;
	struct stat	stnull;

	if (name == NULL)
		return (FALSE);

	if (streql(name, "-")) {
		if (fstat(fdown(stdout), &stbuf) < 0)
			return (FALSE);
	} else {
		if (stat(name, &stbuf) < 0)
			return (FALSE);
	}
	if (stat("/dev/null", &stnull) < 0)
		return (FALSE);

	if (stbuf.st_dev == stnull.st_dev &&
	    stbuf.st_ino == stnull.st_ino)
		return (TRUE);
	return (FALSE);
}

EXPORT BOOL
samefile(fp1, fp2)
	FILE	*fp1;
	FILE	*fp2;
{
	struct stat	stbuf1;
	struct stat	stbuf2;

	if (fp1 == NULL || fp2 == NULL)
		return (FALSE);

	if (fstat(fdown(fp1), &stbuf1) < 0)
		return (FALSE);

	if (fstat(fdown(fp2), &stbuf2) < 0)
		return (FALSE);

	if (stbuf1.st_dev == stbuf2.st_dev &&
	    stbuf1.st_ino == stbuf2.st_ino)
		return (TRUE);
	return (FALSE);
}

EXPORT void
setmodes(info)
	register FINFO	*info;
{
	BOOL	didutimes = FALSE;
	BOOL	asymlink = is_symlink(info);

	/*
	 * If it does not seem to be a symbolic link, we need to check whether
	 * this is an archive format that does not store the real file type.
	 * We need to avoid to set time stamps or modes for hard links on
	 * symbolic links.
	 */
	if (!asymlink &&
	    fis_link(info)) {		/* Real file type unknown */
		FINFO	finfo;

		if (_getinfo(info->f_name, &finfo) && is_symlink(&finfo))
			asymlink = TRUE;
	}

	if (!nomtime && !asymlink) {
		/*
		 * With Cygwin32,
		 * DOS will not allow us to set file times on read-only files.
		 * We set the time before we change the access modes to
		 * overcode this problem.
		 * XXX This will no longer work with the new -p flag handling
		 * XXX as the files may be read only from the creation.
		 */
		if (!is_dir(info)) {
			didutimes = TRUE;
			if (sutimes(info->f_name, info, asymlink) < 0) {
				if (!errhidden(E_SETTIME, info->f_name)) {
					if (!errwarnonly(E_SETTIME, info->f_name))
						xstats.s_settime++;
					(void) errabort(E_SETTIME,
							info->f_name, TRUE);
				}
			}
		}
	}

	if (pflag && !asymlink) {
		mode_t	mode = info->f_mode;

		if (is_dir(info))
			mode |= TUWRITE;
		if (lchmodat(info->f_name, mode, 0 /* chmod */) < 0) {
			if (!errhidden(E_SETMODE, info->f_name)) {
				if (!errwarnonly(E_SETMODE, info->f_name))
					xstats.s_setmodes++;
				(void) errabort(E_SETMODE, info->f_name, TRUE);
			}
		}
#ifdef	USE_ACL
		/*
		 * If there are no additional ACLs, set_acls() will
		 * simply do a fast return.
		 */
		if (doacl)
			set_acls(info);
#endif
	}
#ifdef	USE_FFLAGS
	if (dofflags && !asymlink)
		set_fflags(info);
#endif

	if (!nochown && my_uid == ROOT_UID) {

#if	defined(HAVE_CHOWN) || defined(HAVE_LCHOWN)
		/*
		 * Star will not allow non root users to give away files.
		 */
		lchownat(info->f_name, (int)info->f_uid, (int)info->f_gid,
			AT_SYMLINK_NOFOLLOW);
#endif

		if (pflag && !asymlink &&
			(info->f_mode & (TSUID | TSGID | TSVTX)) != 0) {
			mode_t	mode = info->f_mode;

			if (is_dir(info))
				mode |= TUWRITE;
			/*
			 * On a few systems, seeting the owner of a file
			 * destroys the suid and sgid bits.
			 * We repeat the chmod() in this case.
			 */
			if (lchmodat(info->f_name, mode, 0 /* chmod */) < 0) {
				/*
				 * Do not increment chmod() errors here,
				 * it did already fail above.
				 */
				/* EMPTY */
				;
			}
		}
	}

#ifdef	USE_XATTR
	if (dolxattr)
		set_xattr(info);
#endif

	/*
	 * utimensat() is able to set the time stamps on symlinks, if called
	 * with the AT_SYMLINK_NOFOLLOW flag, so we include symlinks in the
	 * list of file types that cause sutimes() to be called.
	 */
	if (!nomtime && !is_dir(info)) {
		if (sutimes(info->f_name, info, asymlink) < 0 && !didutimes)
			if (!errhidden(E_SETTIME, info->f_name)) {
				if (!errwarnonly(E_SETTIME, info->f_name))
					xstats.s_settime++;
				(void) errabort(E_SETTIME, info->f_name, TRUE);
			}
	}
	if (is_dir(info)) {
		sdirtimes(info->f_name, info, !nomtime, pflag);
	}
}

EXPORT	int	xutimes		__PR((char *name, struct timespec *tp,
					BOOL asymlink));

LOCAL int
sutimes(name, info, asymlink)
	char	*name;
	FINFO	*info;
	BOOL	asymlink;
{
	struct  timespec curtime;
	struct	timespec tp[3];

	if (noatime) {
		getnstimeofday(&curtime);
		tp[0].tv_sec = curtime.tv_sec;
		tp[0].tv_nsec = curtime.tv_nsec;
	} else {
		tp[0].tv_sec = info->f_atime;
		tp[0].tv_nsec = info->f_ansec;
	}

	tp[1].tv_sec = info->f_mtime;
	tp[1].tv_nsec = info->f_mnsec;
#ifdef	SET_CTIME
	tp[2].tv_sec = info->f_ctime;
	tp[2].tv_nsec = info->f_cnsec;
#else
	tp[2].tv_sec = 0;
	tp[2].tv_nsec = 0;
#endif
	return (xutimes(name, tp, asymlink));
}

EXPORT int
snulltimes(name, info)
	char	*name;
	FINFO	*info;
{
	struct	timespec tp[3];

	fillbytes((char *)tp, sizeof (tp), '\0');
	return (xutimes(name, tp, is_symlink(info)));
}

/*
 * Extended utimes function.
 * This is what we use in star as the default.
 */
EXPORT int
xutimes(name, tp, asymlink)
	char	*name;
	struct	timespec tp[3];
	BOOL	asymlink;
{
	struct  timespec curtime;
	struct  timespec pasttime;
	extern int Ctime;
	int	ret = 0;
	int	errsav;

#ifndef	HAVE_SETTIMEOFDAY
#undef	SET_CTIME
#endif

#ifdef	SET_CTIME
	if (Ctime) {
		getnstimeofday(&curtime);
		setnstimeofday(&tp[2]);
	}
#endif
#if	!defined(HAVE_UTIMENSAT) && \
	!defined(HAVE_LUTIMENS) && \
	!defined(HAVE_LUTIMES)
	/*
	 * AT_SYMLINK_NOFOLLOW not in emulation
	 */
	if (!asymlink)
#endif
		ret = lutimensat(name, tp, AT_SYMLINK_NOFOLLOW);
	errsav = geterrno();

#ifdef	SET_CTIME
	if (Ctime) {
		getnstimeofday(&pasttime);
		timespecsub(&pasttime, &tp[2]);
		timespecadd(&curtime, &pasttime);
		setnstimeofday(&curtime);
#ifdef	SET_CTIME_DEBUG
		error("pasttime: %d.%9.9d\n", pasttime.tv_sec, pasttime.tv_nsec);
#endif
	}
#endif
	seterrno(errsav);
	return (ret);
}

EXPORT int
sxsymlink(name, info)
	char	*name;
	FINFO	*info;
{
#ifdef	HAVE_SYMLINK
	struct	timespec tp[3];
	struct  timespec curtime;
	struct  timespec pasttime;
	char	*linkname;
	extern int Ctime;
	int	ret;
	int	errsav;
#ifdef	HAVE_POSIX_MODE_BITS	/* st_mode bits are equal to TAR mode bits */
	mode_t	omask;
#endif

	tp[0].tv_sec = info->f_atime;
	tp[0].tv_nsec = info->f_ansec;

	tp[1].tv_sec = info->f_mtime;
	tp[1].tv_nsec = info->f_mnsec;
#ifdef	SET_CTIME
	tp[2].tv_sec = info->f_ctime;
	tp[2].tv_nsec = info->f_cnsec;
#endif
	linkname = info->f_lname;

#ifdef	SET_CTIME
	if (Ctime) {
		getnstimeofday(&curtime);
		setnstimeofday(&tp[2]);
	}
#endif

#ifdef	HAVE_POSIX_MODE_BITS	/* st_mode bits are equal to TAR mode bits */
	/*
	 * At least HP-UX-11.x seems to honour the mask when creating symlinks.
	 * If we like to copy them correctly, we need to set the mask before.
	 * As umask(2) is a cheap syscall and symlinks are not very frequent
	 * this does not seem a real problem.
	 */
	omask = umask((mode_t)0);
	umask(info->f_mode ^ 07777);
#endif

	ret = lsymlink(linkname, name);
	errsav = geterrno();

#ifdef	HAVE_POSIX_MODE_BITS	/* st_mode bits are equal to TAR mode bits */
	umask(omask);
#endif

#ifdef	SET_CTIME
	if (Ctime) {
		getnstimeofday(&pasttime);
		timespecsub(&pasttime, &tp[2]);
		timespecadd(&curtime, &pasttime);
		setnstimeofday(&curtime);
#ifdef	SET_CTIME_DEBUG
		error("pasttime: %d.%9.9d\n", pasttime.tv_sec, pasttime.tv_nsec);
#endif
	}
#endif
	seterrno(errsav);
	return (ret);
#else
	seterrno(EINVAL);
	return (-1);
#endif
}

#ifdef	HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif

EXPORT int
rs_acctime(fd, info)
		int	fd;
	register FINFO	*info;
{
#ifdef	_FIOSATIME
	struct timeval	atv;
#endif

	if (is_symlink(info))
		return (0);

#ifdef	_FIOSATIME
	/*
	 * On Solaris 2.x root may reset accesstime without changing ctime.
	 */
	if (my_uid == ROOT_UID) {
		atv.tv_sec = info->f_atime;
		atv.tv_usec = info->f_ansec/1000;
		return (ioctl(fd, _FIOSATIME, &atv));
	}
#endif
	return (sutimes(info->f_name, info, FALSE));
}

#ifdef	HAVE_POSIX_MODE_BITS	/* st_mode bits are equal to TAR mode bits */
#define	OSMODE(tarmode)	    (tarmode)
#else
#define	OSMODE(tarmode)	    ((tarmode & TSUID   ? S_ISUID : 0)  \
			    | (tarmode & TSGID   ? S_ISGID : 0) \
			    | (tarmode & TSVTX   ? S_ISVTX : 0) \
			    | (tarmode & TUREAD  ? S_IRUSR : 0) \
			    | (tarmode & TUWRITE ? S_IWUSR : 0) \
			    | (tarmode & TUEXEC  ? S_IXUSR : 0) \
			    | (tarmode & TGREAD  ? S_IRGRP : 0) \
			    | (tarmode & TGWRITE ? S_IWGRP : 0) \
			    | (tarmode & TGEXEC  ? S_IXGRP : 0) \
			    | (tarmode & TOREAD  ? S_IROTH : 0) \
			    | (tarmode & TOWRITE ? S_IWOTH : 0) \
			    | (tarmode & TOEXEC  ? S_IXOTH : 0))
#endif

EXPORT void
#ifdef	PROTOTYPES
setdirmodes(char *name, mode_t tarmode)
#else
setdirmodes(name, tarmode)
	char	*name;
	mode_t	tarmode;
#endif
{
	register mode_t		_osmode;

	_osmode = OSMODE(tarmode);
	if (lchmodat(name, _osmode, 0 /* chmod */) < 0) {	/* OK for dir */
		if (!errhidden(E_SETMODE, name)) {
			if (!errwarnonly(E_SETMODE, name))
				xstats.s_setmodes++;
			errmsg("Can't set permission for '%s'.\n", name);
			(void) errabort(E_SETMODE, name, TRUE);
		}
	}
}


EXPORT mode_t
#ifdef	PROTOTYPES
osmode(register mode_t tarmode)
#else
osmode(tarmode)
	register mode_t		tarmode;
#endif
{
	register mode_t		_osmode;

	_osmode = OSMODE(tarmode);
	return (_osmode);
}

#ifdef	HAVE_POSIX_MODE_BITS	/* st_mode bits are equal to TAR mode bits */
#else
#undef	lchmodat
LOCAL int
#ifdef	PROTOTYPES
dolchmodat(register char *name, register mode_t tarmode, int flag)
#else
dolchmodat(name, tarmode, flag)
	register char		*name;
	register mode_t		tarmode;
		int		flag;
#endif
{
	register mode_t		_osmode;

	_osmode = OSMODE(tarmode);
	return (lchmodat(name, _osmode, flag));
}
#endif
