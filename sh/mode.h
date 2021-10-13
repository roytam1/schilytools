/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
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
/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
/* Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T */
/* All Rights Reserved */

#if defined(sun)
#pragma ident	"@(#)mode.h	1.13	06/05/17 SMI"
#endif

#ifndef _MODE_H
#define	_MODE_H

/*
 * This file contains modifications Copyright 2008-2015 J. Schilling
 *
 * @(#)mode.h	1.17 15/09/01 2008-2015 J. Schilling
 */

/*
 *	UNIX shell
 */
#ifdef	SCHILY_INCLUDES
#include <schily/unistd.h>
#include <schily/utypes.h>
#else
#include <unistd.h>
#endif

#ifdef pdp11
typedef char BOOL;
#else
typedef short BOOL;
#endif

#define	BYTESPERWORD	(sizeof (char *))
#define	ALIGNSIZ	(sizeof (double))
#define	NIL	((char *)0)


/*
 * the following nonsense is required
 * because casts turn an Lvalue
 * into an Rvalue so two cheats
 * are necessary, one for each context.
 */
#define	Rcheat(a)	((Intptr_t)(a))


/* address puns for storage allocation */
typedef union
{
	struct forknod	*_forkptr;
	struct comnod	*_comptr;
	struct fndnod	*_fndptr;
	struct parnod	*_parptr;
	struct ifnod	*_ifptr;
	struct whnod	*_whptr;
	struct fornod	*_forptr;
	struct lstnod	*_lstptr;
	struct blk	*_blkptr;
	struct namnod	*_namptr;
	char	*_bytptr;
} address;


/*
 * largefile converson hack note.
 * the shell uses the *fnxt and *fend pointers when
 * parsing a script. However, it was also using the
 * difference between them when doing lseeks. Because
 * that doesn't work in the largefile world, I have
 * added a parallel set of offset counters that need to
 * be updated whenever the "buffer" offsets the shell
 * uses get changed. Most of this code is in word.c.
 * If you change it, have fun...
 */

#define	BUFFERSIZE	128
struct fileblk
{
	int	fdes;
	unsigned flin;
	BOOL	feof;
	unsigned char	fsiz;
	unsigned char	*fnxt;
	unsigned char	*fend;
	off_t		nxtoff;		/* file offset */
	off_t		endoff;		/* file offset */
	unsigned char	**feval;
	struct fileblk	*fstak;
	unsigned char	fbuf[BUFFERSIZE];
};

struct tempblk
{
	int fdes;
	struct tempblk *fstak;
};


/* for files not used with file descriptors */
struct filehdr
{
	int	fdes;
	unsigned	flin;
	BOOL	feof;
	unsigned char	fsiz;
	unsigned char	*fnxt;
	unsigned char	*fend;
	off_t		nxtoff;		/* file offset */
	off_t		endoff;		/* file offset */
	unsigned char	**feval;
	struct fileblk	*fstak;
	unsigned char	_fbuf[1];
};

/*
 * For the results from waitid()
 */
struct excode {
	int	ex_code;	/* Child status code */
	int	ex_status;	/* Child exit code or signal number */
	pid_t	ex_pid;		/* Child pid */
	int	ex_signo;	/* Causing signal, SIGCLD for wait() */
};

struct sysnod
{
	char	*sysnam;	/* Name of reserved / builtin	*/
	UInt16_t sysval;	/* Value to identify above	*/
	UInt16_t sysflg;	/* Flag for builtins		*/
};

typedef  void    (*bftype) __PR((int argc, char *argv[]));

struct sysnod2
{
	char	*sysnam;	/* Name of reserved / builtin	*/
	UInt16_t sysval;	/* Value to identify above	*/
	UInt16_t sysflg;	/* Flag for builtins		*/
	bftype	sysptr;		/* Ptr to function		*/
};

/* this node is a proforma for those that follow */
struct trenod
{
	int	tretyp;
	struct ionod	*treio;
};

/* dummy for access only */
struct argnod
{
	struct argnod	*argnxt;
	unsigned char	argval[1];
};

struct dolnod
{
	struct dolnod	*dolnxt;
	int	doluse;
	unsigned char	**dolarg;
};

struct forknod
{
	int	forktyp;
	struct ionod	*forkio;
	struct trenod	*forktre;
};

struct comnod
{
	int	comtyp;
	struct ionod	*comio;
	struct argnod	*comarg;
	struct argnod	*comset;
};

struct fndnod
{
	int 	fndtyp;
	unsigned char	*fndnam;
	struct trenod	*fndval;
	int	fndref;
};

struct ifnod
{
	int	iftyp;
	struct trenod	*iftre;
	struct trenod	*thtre;
	struct trenod	*eltre;
};

struct whnod
{
	int	whtyp;
	struct trenod	*whtre;
	struct trenod	*dotre;
};

struct fornod
{
	int	fortyp;
	struct trenod	*fortre;
	unsigned char	*fornam;
	struct comnod	*forlst;
};

struct swnod
{
	int	swtyp;
	unsigned char *swarg;
	struct regnod	*swlst;
};

struct regnod
{
	struct argnod	*regptr;
	struct trenod	*regcom;
	struct regnod	*regnxt;
};

struct parnod
{
	int	partyp;
	struct trenod	*partre;
};

struct lstnod
{
	int	lsttyp;
	struct trenod	*lstlef;
	struct trenod	*lstrit;
};

struct ionod
{
	int	iofile;
	char	*ioname;
	char	*iolink;
	struct ionod	*ionxt;
	struct ionod	*iolst;
};

struct fdsave
{
	int org_fd;
	int dup_fd;
};

struct optv
{
	int	opterr;
	int	optind;
	int	optopt;
	int	opt_sp;
	char	*optarg;
};

#define		fndptr(x)	((struct fndnod *)x)
#define		comptr(x)	((struct comnod *)x)
#define		forkptr(x)	((struct forknod *)x)
#define		parptr(x)	((struct parnod *)x)
#define		lstptr(x)	((struct lstnod *)x)
#define		forptr(x)	((struct fornod *)x)
#define		whptr(x)	((struct whnod *)x)
#define		ifptr(x)	((struct ifnod *)x)
#define		swptr(x)	((struct swnod *)x)

#endif /* _MODE_H */
