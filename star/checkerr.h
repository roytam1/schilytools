/* @(#)checkerr.h	1.12 12/11/13 Copyright 2003-2012 J. Schilling */
/*
 *	Error control for star.
 *
 *	Copyright (c) 2003-2012 J. Schilling
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

#ifndef	_CHECKERR_H
#define	_CHECKERR_H

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Error conditions handled by error control.
 */
#define	E_STAT		0x0001		/* Could not stat(2) file	   */
#define	E_GETACL	0x0002		/* Could not retrieve ACL info	   */
#define	E_OPEN		0x0004		/* Could not open file		   */
#define	E_READ		0x0008		/* Could not read file		   */
#define	E_WRITE		0x0010		/* Could not write file		   */
#define	E_GROW		0x0020		/* File did grow during backup	   */
#define	E_SHRINK	0x0040		/* File did shrink during backup   */
#define	E_MISSLINK	0x0080		/* Missing hard link(s) for file   */
#define	E_NAMETOOLONG	0x0100		/* File name too long for archive  */
#define	E_FILETOOBIG	0x0200		/* File too big for archive	   */
#define	E_SPECIALFILE	0x0400		/* Improper file type for archive  */
#define	E_READLINK	0x0800		/* Could not read symbolic link	   */
#define	E_GETXATTR	0x1000		/* Could not get xattr		   */
#define	E_CHDIR		0x2000		/* Could not chdir()		   */

/*
 * Currently unused: 0x4000 .. 0x8000
 */

#define	E_SETTIME	0x10000		/* Could not set file times	   */
#define	E_SETMODE	0x20000		/* Could not set access modes	   */
#define	E_SECURITY	0x40000		/* Skipped for security reasons	   */
#define	E_LSECURITY	0x80000		/* Link skipped for security	   */
#define	E_SAMEFILE	0x100000	/* Skipped from/to identical	   */
#define	E_BADACL	0x200000	/* ACL string conversion error	   */
#define	E_SETACL	0x400000	/* Could not set ACL for file	   */
#define	E_SETXATTR	0x800000	/* Could not set xattr		   */

/*
 * Currently unused: 0x1000000 .. 0x8000000
 */

#define	E_DIFF		0x10000000	/* Diffs encountered		   */
#define	E_WARN		0x20000000	/* Print this error but do exit(0) */
#define	E_ABORT		0x40000000	/* Abort on this error		   */
#define	E_EXT		0x80000000	/* Extended (TBD later)		   */

#define	E_ALL		(~(UInt32_t)(E_DIFF|E_ABORT))

extern	int	errconfig	__PR((char *name));
extern	BOOL	errhidden	__PR((int etype, const char *fname));
extern	BOOL	errwarnonly	__PR((int etype, const char *fname));
extern	BOOL	errabort	__PR((int etype, const char *fname,
					BOOL doexit));

#ifdef	__cplusplus
}
#endif

#endif	/* _CHECKERR_H */
