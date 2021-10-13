/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
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
 * Copyright 1995 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#if defined(sun)
#pragma ident	"@(#)expand.c	1.15	05/09/13 SMI"
#endif

#include "defs.h"

/*
 * Copyright 2008-2019 J. Schilling
 *
 * @(#)expand.c	1.29 19/09/25 2008-2019 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)expand.c	1.29 19/09/25 2008-2019 J. Schilling";
#endif

/*
 *	UNIX shell
 *
 */

#include	<schily/fcntl.h>
#include	<schily/errno.h>

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<dirent.h>



/*
 * globals (file name generation)
 *
 * "*" in params matches r.e ".*"
 * "?" in params matches r.e. "."
 * "[...]" in params matches character class
 * "[...a-z...]" in params matches a through z.
 *
 */
	int	expand	__PR((unsigned char *as, int rcnt));
static void	addg	__PR((unsigned char *, unsigned char *,
			    unsigned char *,
			    unsigned char *));
	void	makearg	__PR((struct argnod *));
static	DIR	*lopendir __PR((char *name));

#ifdef	DO_GLOBSKIPDOT
static char	*dots[] = { ".", ".." };
#endif

int
expand(as, rcnt)
	unsigned char	*as;
	int		rcnt;
{
	int	count;
	DIR	*dirf;
	unsigned char	*rescan = 0;
	unsigned char	*slashsav = 0;
	unsigned char	*s, *cs;
	unsigned char *s2 = 0;
	struct argnod	*schain = gchain;
	BOOL	slash;
	int	len;
	wchar_t	wc;

#ifdef	EXPAND_DEBUG
	fprintf(stderr, "expand(%s, %d)\n", as, rcnt);
#endif
	if (trapnote & SIGSET)
		return (0);
	s = cs = as;
	/*
	 * check for meta chars
	 */
	{
		BOOL openbr;

		slash = 0;
		openbr = 0;
		(void) mbtowc(NULL, NULL, 0);
		do
		{
			if ((len = mbtowc(&wc, (char *)cs, MB_LEN_MAX)) <= 0) {
				(void) mbtowc(NULL, NULL, 0);
				len = 1;
				wc = (unsigned char)*cs;
			}

			cs += len;
			switch (wc) {
			case 0:
				if (rcnt && slash)
					break;
				else
					return (0);

			case '/':
				slash++;
				openbr = 0;
				continue;

			case '[':
				openbr++;
				continue;

			case ']':
				if (openbr == 0)
					continue;
				/* FALLTHROUGH */
			case '?':
			case '*':
				if (rcnt > slash)
					continue;
				else
					cs--;
				break;

			case '\\':
				cs++;
			default:
				continue;
			}
			break;
			/* CONSTCOND */
		} while (TRUE);
	}

	for (;;) {
		if (cs == s) {
			s = (unsigned char *)nullstr;
			break;
		} else if (*--cs == '/') {
			*cs = 0;
			if (s == cs)
				s = (unsigned char *)"/";
			else {
				/*
				 * push trimmed copy of directory prefix
				 * onto stack
				 */
				s2 = cpystak(s);
				trim(s2);
				s = s2;
			}
			break;
		}
	}

	dirf = lopendir(*s ? (char *)s : (char *)".");

	/* Let s point to original string because it will be trimmed later */
	if (s2)
		s = as;
	count = 0;
	if (*cs == 0)
		slashsav = cs++; /* remember where first slash in as is */

	/* check for rescan */
	if (dirf) {
		unsigned char *rs;
		struct dirent *e;

		rs = cs;
		do /* find next / in as */
		{
			if (*rs == '/') {
				rescan = rs;
				*rs = 0;
				gchain = 0;
			}
		} while (*rs++);

#ifdef	DO_EXPAND_DIRSLASH
		if (*cs == 0) {
			/*
			 * We have been able to open the directory before, so
			 * the empty new name after "dir* /" is no problem.
			 * The whole path definitely applies to a directory.
			 * We thus pretend that we found a match and append
			 * the empty string to the current path.
			 */
			addg(s, (unsigned char *)"", rescan, slashsav);
			count++;
		} else
#endif
		{
#ifdef	DO_GLOBSKIPDOT
		if ((flags2 & globskipdot) == 0 && *cs == '.') {
			/*
			 * Synthesize "." and ".." to make sure they are present
			 * even if the current filesystem does not have them.
			 */
			for (len = 0; len < 2; len++) {
				if (gmatch(dots[len], (char *)cs)) {
					addg(s, (unsigned char *)dots[len],
					    rescan, slashsav);
					count++;
				}
			}
		}
#endif
		while ((e = readdir(dirf)) != 0 && (trapnote & SIGSET) == 0) {
			char	*name = e->d_name;

#ifdef	DO_GLOBSKIPDOT
			/*
			 * Skip the following names: "", ".", "..".
			 */
			if (name[name[0] != '.' ? 0 :
			    name[1] != '.' ? 1 : 2] == '\0')
				continue;
#endif
			if (name[0] == '.' && *cs != '.')
				continue;

			if (gmatch(name, (char *)cs)) {
				addg(s, (unsigned char *)name, rescan,
				    slashsav);
				count++;
			}
		}}
		(void) closedir(dirf);

		if (rescan) {
			struct argnod	*rchain;

			rchain = gchain;
			gchain = schain;
			if (count) {
				count = 0;
				while (rchain) {
					count += expand(rchain->argval,
							slash + 1);
					rchain = rchain->argnxt;
				}
			}
			*rescan = '/';
		}
	}

	if (slashsav)
		*slashsav = '/';
	return (count);
}

static void
addg(as1, as2, as3, as4)
	unsigned char	*as1;
	unsigned char	*as2;
	unsigned char	*as3;
	unsigned char	*as4;
{
	unsigned char	*s1, *s2;
	int	c;
	int		len;
	wchar_t		wc;

	s2 = locstak() + BYTESPERWORD;
	s1 = as1;
	if (as4) {
		while ((c = *s1++) != '\0') {
			GROWSTAK(s2);
			*s2++ = (char)c;
		}
		/*
		 * Restore first slash before the first metacharacter
		 * if as1 is not "/"
		 */
		if (as4 + 1 == s1) {
			GROWSTAK(s2);
			*s2++ = '/';
		}
	}
/* add matched entries, plus extra \\ to escape \\'s */
	s1 = as2;
	(void) mbtowc(NULL, NULL, 0);
	for (;;) {
		if ((len = mbtowc(&wc, (char *)s1, MB_LEN_MAX)) <= 0) {
			(void) mbtowc(NULL, NULL, 0);
			len = 1;
			wc = (unsigned char)*s1;
		}
		GROWSTAK(s2);

		if (wc == 0) {
			*s2 = *s1++;
			break;
		}

		if (wc == '\\') {
			*s2++ = '\\';
			GROWSTAK(s2);
			*s2++ = '\\';
			s1++;
			continue;
		}
		if ((s2 + len) >= brkend) {
			s2 = growstak(s2 + len);
			s2 -= len;
		}
		memcpy(s2, s1, len);
		s2 += len;
		s1 += len;
	}
	if ((s1 = as3) != NULL) {
		GROWSTAK(s2);
		*s2++ = '/';
		do {
			GROWSTAK(s2);
		} while ((*s2++ = *++s1) != '\0');
	}
	makearg((struct argnod *)endstak(s2));
}

void
makearg(args)
	struct argnod	*args;
{
	args->argnxt = gchain;
	gchain = args;
}

static DIR *
lopendir(name)
	char	*name;
{
#if	defined(HAVE_FCHDIR) && defined(DO_EXPAND_LONG)
	char	*p;
	int	fd;
	int	dfd;
#endif
	DIR	*ret = NULL;

	if ((ret = opendir(name)) == NULL && errno != ENAMETOOLONG)
		return ((DIR *)NULL);

#if	defined(HAVE_FCHDIR) && defined(DO_EXPAND_LONG)
	if (ret)
		return (ret);

	fd = sh_hop_dirs(name, &p);
	if (fd < 0)
		return ((DIR *)NULL);
	if ((dfd = openat(fd, p, O_RDONLY|O_DIRECTORY|O_NDELAY)) < 0) {
		close(fd);
		return ((DIR *)NULL);
	}
	close(fd);
	ret = fdopendir(dfd);
#endif
	return (ret);
}
