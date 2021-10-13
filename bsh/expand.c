/* @(#)expand.c	1.58 19/06/26 Copyright 1985-2019 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)expand.c	1.58 19/06/26 Copyright 1985-2019 J. Schilling";
#endif
/*
 *	Expand a pattern (do shell name globbing)
 *
 *	Copyright (c) 1985-2019 J. Schilling
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

#include <schily/fcntl.h>
#include <schily/stdio.h>
#include <schily/string.h>
#include <schily/stdlib.h>
#include <schily/dirent.h>	/* Must be before bsh.h/schily.h/libport.h */
#include <schily/patmatch.h>
#include "bsh.h"
#include "node.h"
#include "str.h"
#include "strsubs.h"

#ifdef	DEBUG
#define	EDEBUG(a)	printf a
#else
#define	EDEBUG(a)
#endif

static char mchars[] = "!#%*{}[]?\\";

#define	exp	_exp	/* Some compilers do not like exp() */

LOCAL	int	dncmp		__PR((char *s1, char *s2));
LOCAL	char	*save_base	__PR((char *s, char *endptr));
EXPORT	BOOL	any_match	__PR((char *s));
LOCAL	Tnode	*mklist		__PR((Tnode *l));
LOCAL	int	xcmp		__PR((char *s1, char *s2));
LOCAL	void	xsort		__PR((char **from, char **to));
LOCAL	Tnode	*exp		__PR((char *n, int i, Tnode * l, BOOL patm));
EXPORT	Tnode	*expand		__PR((char *s));
EXPORT	Tnode	*bexpand	__PR((char *s));
EXPORT	int	bsh_hop_dirs	__PR((char *name, char **np));
LOCAL	DIR	*lopendir	__PR((char *name));

LOCAL int
dncmp(s1, s2)
	register char	*s1;
	register char	*s2;
{
	for (; *s1 == *s2; s1++, s2++) {
		if (*s1 == '\0')
			return (0);
	}
	if (*s1 == '\0' && *s2 == '/')
		return (0);
	return (*s1 - *s2);
}

LOCAL char *
save_base(s, endptr)
	register char	*s;
	register char	*endptr;
{
	register char	*tmp;
	register char	*r;

	tmp = malloc((size_t)(endptr-s+1));
	if (tmp == (char *)NULL)
		return (tmp);
	for (r = tmp; s < endptr; )
		*r++ = *s++;
	*r = '\0';
	return (tmp);
}

EXPORT BOOL
any_match(s)
	register char	*s;
{
	register char	*rm = mchars;

	while (*s && !strchr(rm, *s))
		s++;
	return ((BOOL)*s);
}

LOCAL Tnode *
mklist(l)
		Tnode	*l;
{
	register int	ac;
	register char	**p;
	register Tnode	*l1;
	register Argvec	*vp;

	if (l == (Tnode *)NULL)
		return ((Tnode *)NULL);

	ac = listlen(l);
	vp = allocvec(ac);
	if (vp == (Argvec *)NULL) {
		freetree(l);
		return ((Tnode *)NULL);
	}
	vp->av_ac = ac;
	for (l1 = l, p = &vp->av_av[0]; --ac >= 0; ) {
		*p++ = l1->tn_left.tn_str;
		l1->tn_type = STRING;	/* Type LSTRING -> STRING */
		l1 = l1->tn_right.tn_node;
	}
	xsort(&vp->av_av[0], &vp->av_av[vp->av_ac]);

	ac = vp->av_ac;

	for (l1 = l, p = &vp->av_av[0]; --ac >= 0; ) {
		l1->tn_left.tn_str = *p++;
		l1 = l1->tn_right.tn_node;
	}
	free((char *)vp);
	return (l);
}

LOCAL int
xcmp(s1, s2)
	register char	*s1;
	register char	*s2;
{
	while (*s1++ == *s2)
		if (*s2++ == 0)
			return (0);
	return (*--s1 - *s2);
}

#define	USE_QSORT
#ifdef	USE_QSORT
/*
 *	quicksort algorithm
 *	on array of elsize elements from lowp to hip-1
 */
#define	exchange(a, b)	{ register char *p = *(a); *(a) = *(b); *(b) = p; }

LOCAL void
xsort(lowp, hip)
	register char	*lowp[];
	register char	*hip[];
{
	register char **olp;
	register char **ohp;
	register char **pivp;

	hip--;

	while (hip > lowp) {
		if (hip == (lowp + 1)) {	/* two elements */
			if (xcmp(*hip, *lowp) < 0)
				exchange(lowp, hip);
			return;
		}
		olp = lowp;
		ohp = hip;

		pivp = lowp+((((unsigned)(hip-lowp))+1)/2);
		exchange(pivp, hip);
		pivp = hip;
		hip--;				/* point past pivot element */

		while (lowp <= hip) {
			while ((hip >= lowp) && (xcmp(*hip, *pivp) >= 0))
				hip--;
			while ((lowp <= hip) && (xcmp(*lowp, *pivp) < 0))
				lowp++;
			if (lowp < hip) {
				exchange(lowp, hip);
				hip--;
				lowp++;
			}
		}
		/*
		 *	now lowp points at first member not in low set
		 *	and hip points at first member not in high set.
		 *	Since high set contains the pivot element (by
		 *	definition) it has at least one element.
		 *	low set might not.  check for this and make the
		 *	pivot element part of the low set if its is empty.
		 *	sort smaller of remaining pieces
		 *	then larger, to cut down on recursion
		 */
		if (lowp == olp) {			/* its empty */
			exchange(lowp, pivp);
			lowp++;		 /* point past sigle element(pivot) */
			hip = ohp;
		} else if ((olp - lowp-1) > (hip+1 - ohp)) {
			xsort(hip+1, ohp+1);
			/* hip = lowp - elsize; set right alread */
			lowp = olp;
		} else {
			xsort(olp, lowp);
			/* lowp = hip+elsize;  set right already */
			hip = ohp;
		}

	}
}
#else
/*
 *	shellsort algorithm
 *	on array of elsize elements from lowp to hip-1
 */
LOCAL void
xsort(from, to)
	char	*from[];
	char	*to[];
{
	register int	i;
	register int	j;
		int	k;
		int	m;
		int	n;

	if ((n = to - from) <= 1)	/* Nothing to sort. */
		return;

	for (j = 1; j <= n; j *= 2)
		;

	for (m = 2 * j - 1; m /= 2; ) {
		k = n - m;
		for (j = 0; j < k; j++) {
			for (i = j; i >= 0; i -= m) {
				register char **fromi;
#ifdef	cmplocal
				register char	*s1;
				register char	*s2;
#endif

				fromi = &from[i];
#ifdef	cmplocal
				s1 = fromi[m];
				s2 = fromi[0];

				/* schneller als strcmp() ??? */

				while (*s1++ == *s2) {
					if (*s2++ == 0) {
						--s2;
						break;
					}
				}
				if ((*--s1 - *s2) > 0) {
#else
				if (xcmp(fromi[m], fromi[0]) > 0) {
#endif
					break;
				} else {
					char *s;

					s = fromi[m];
					fromi[m] = fromi[0];
					fromi[0] = s;
				}
			}
		}
	}
}
#endif

LOCAL Tnode *
exp(n, i, l, patm)
		char	*n;		/* name to rescan */
		int	i;		/* index in name to start rescan */
		Tnode	*l;		/* list of Tnodes already found */
		BOOL	patm;		/* use pattern matching not strbeg */
{
	register char	*cp;
	register char	*dp;		/* pointer past end of current dir */
		char	*dir	= NULL;
		char	*tmp;
	register char	*cname;		/* concatenated name dir+d_ent */
		DIR	*dirp;
	struct dirent	*dent;
		int	*aux;
		int	*state;
	register int	patlen;
	register int	alt;
		int	rescan	= 0;
		Tnode	*l1	= l;

	cp = dp = &n[i];

	EDEBUG(("name: '%s' i: %d dp: '%s'\n", n, i, dp));

	if (patm) {
		while (*cp && !strchr(mchars, *cp)) /* go past non glob parts */
			if (*cp++ == '/')
				dp = cp;

		while (*cp && *cp != '/') /* find end of name component */
			cp++;

		patlen = cp-dp;
		i = dp - n;		/* make &n[i] == dp (pattern start) */

		/*
		 * Prepare to use the pattern matcher.
		 */
		EDEBUG(("patlen: %d pattern: '%.*s'\n", patlen, patlen, dp));

		aux = malloc((size_t)patlen*(sizeof (int)));
		state = malloc((size_t)(patlen+1)*(sizeof (int)));
		if (aux == (int *)NULL || state == (int *)NULL)
			goto cannot;
		if ((alt = patcompile((unsigned char *)dp, patlen, aux)) == 0 &&
		    patlen != 0) {
			EDEBUG(("Bad pattern\n"));
			free((char *)aux);
			free((char *)state);
			return (l1);
		}
	} else {			/* Non-pattern matching variant */
		alt = 0;
		aux = NULL;
		state = NULL;

		while (*cp) {		/* go past directory parts */
			if (*cp++ == '/')
				dp = cp;
		}

		patlen = cp-dp;
		i = dp - n;		/* make &n[i] == dp (pattern start) */
	}

	dir = save_base(n, dp);		/* get dirname part */
	if (dir == (char *)NULL ||
	    (dirp = lopendir(dp == n ? "." : dir)) == (DIR *)NULL)
		goto cannot;

	EDEBUG(("dir: '%s' match: '%.*s'\n", dp == n?".":dir, patlen, dp));
	if (patlen == 0 && patm) {
		/*
		 * match auf /pattern/ Daher kein Match wenn keine
		 * Directory! opendir() Test ist daher notwendig.
		 * Fuer libshedit (ohne patm) wird aber die Directory
		 * komplett expandiert.
		 */
		l1 = allocnode(STRING, (Tnode *)makestr(dir), l1);

	} else while ((dent = readdir(dirp)) != 0 && !ctlc) {
		int	namlen;
		char	*name = dent->d_name;

		/*
		 * Skip the following names: "", ".", "..".
		 */
		if (name[name[0] != '.' ? 0 : name[1] != '.' ? 1 : 2] == '\0') {
			/*
			 * Do not skip . and .. if there is a plain match.
			 * We need this to let ..TAB expand to ../ in the
			 * command line editor.
			 */
			if ((name[0] == '.' && dp[0] == '.') &&
			    ((name[1] == '\0' && dp[1] == '\0') ||
			    ((name[1] == '.' && dp[1] == '.') &&
			     (name[2] == '\0' && dp[2] =='\0')))) {
				/* EMPTY */;
			} else {
				continue;
			}
		}

		/*
		 * Are we interested in files starting with '.'?
		 */
		if (name[0] == '.' && *dp != '.')
			continue;
		namlen = DIR_NAMELEN(dent);
		if (patm) {
			tmp = (char *)patmatch((unsigned char *)dp, aux,
				(unsigned char *)name, 0, namlen,
				alt, state);
		} else {
			if (strstr(name, dp) == name)
				tmp = "";
			else
				tmp = NULL;
		}

#ifdef	DEBUG
		if (tmp != NULL || (name[0] == dp[0] &&
		    patlen == namlen))
			EDEBUG(("match? '%s' end: '%s'\n", name, tmp));
#endif
		/*
		 * *tmp == '\0' is a result of an exact pattern match.
		 *
		 * dncmp(dent->d_name, dp) == 0 happens when
		 * a pattern contains a pattern matcher special
		 * character (e.g. "foo#1"), but the pattern would not
		 * match itself using regular expressions. We use a
		 * literal compare in this case.
		 */

		if ((tmp != NULL && *tmp == '\0') ||
		    (patlen == namlen &&
		    name[0] == dp[0] &&
		    dncmp(name, dp) == 0)) {
			EDEBUG(("found: '%s'\n", name));

			cname = concat(dir, name, cp, (char *)NULL);
			if (*cp == '/') {
				EDEBUG(("rescan: '%s'\n", cname));
				rescan++;
			}
			if (cname) {
				l1 = allocnode(sxnlen(i+namlen),
						(Tnode *)cname, l1);
			} else {
				EDEBUG(("cannot concat: '%s%s%s'\n",
				/* EDEBUG */	dir, name, cp));
				break;
			}
		}
	}
	closedir(dirp);
cannot:
	if (dir)
		free(dir);
	if (aux)
		free((char *)aux);
	if (state)
		free((char *)state);

	if (rescan > 0 && l1 != (Tnode *)NULL) {
		for (alt = rescan; --alt >= 0; ) {
			register Tnode	*l2;

			l = exp(l1->tn_left.tn_str, nlen(l1), l, patm);
			free(l1->tn_left.tn_str);
			l2 = l1->tn_right.tn_node;
			free(l1);
			l1 = l2;
		}
		return (l);
	}

	return (l1);
}

/*
 * The expand function used for globbing
 */
EXPORT Tnode *
expand(s)
	char	*s;
{
	if (!any_match(s))
		return ((Tnode *)NULL);
	else
		return (mklist(exp(s, 0, (Tnode *)NULL, TRUE)));
}

/*
 * Begin expand, used for file name completion
 */
EXPORT Tnode *
bexpand(s)
	char	*s;
{
	return (mklist(exp(s, 0, (Tnode *)NULL, FALSE)));
}

#ifdef	HAVE_FCHDIR
EXPORT int
bsh_hop_dirs(name, np)
	char	*name;
	char	**np;
{
	char	*p;
	char	*p2;
	int	fd;
	int	dfd;
	int	err;

	p = name;
	fd = AT_FDCWD;
	if (*p == '/') {
		fd = openat(fd, "/", O_SEARCH|O_DIRECTORY|O_NDELAY);
		while (*p == '/')
			p++;
	}
	while (*p) {
		if ((p2 = strchr(p, '/')) != NULL) {
			if (p2[1] == '\0')
				break;
			*p2 = '\0';
		} else {
			break;	/* No slash remains, return the prefix fd */
		}
		if ((dfd = openat(fd, p, O_SEARCH|O_DIRECTORY|O_NDELAY)) < 0) {
			err = geterrno();

			close(fd);
			if (err == EMFILE)
				seterrno(err);
			else
				seterrno(ENAMETOOLONG);
			*p2 = '/';
			return (dfd);
		}
		close(fd);	/* Don't care about AT_FDCWD, it is negative */
		fd = dfd;
		*p2++ = '/';	/* p2 is always != NULL here */
		while (*p2 == '/')
			p2++;
		p = p2;
	}
	*np = p;
	return (fd);
}
#endif

LOCAL DIR *
lopendir(name)
	char	*name;
{
#ifdef	HAVE_FCHDIR
	char	*p;
	int	fd;
	int	dfd;
#endif
	DIR	*ret = NULL;

	if ((ret = opendir(name)) == NULL && geterrno() != ENAMETOOLONG)
		return ((DIR *)NULL);

#ifdef	HAVE_FCHDIR
	if (ret)
		return (ret);

	fd = bsh_hop_dirs(name, &p);
	if ((dfd = openat(fd, p, O_RDONLY|O_DIRECTORY|O_NDELAY)) < 0) {
		close(fd);
		return ((DIR *)NULL);
	}
	close(fd);
	ret = fdopendir(dfd);
#endif
	return (ret);
}
