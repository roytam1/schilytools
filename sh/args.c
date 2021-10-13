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
#pragma ident	"@(#)args.c	1.11	05/09/14 SMI"
#endif

#include "defs.h"
#ifdef	DO_SYSALIAS
#include "abbrev.h"
#endif
#include "version.h"

/*
 * Copyright 2008-2021 J. Schilling
 *
 * @(#)args.c	1.96 21/02/27 2008-2021 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)args.c	1.96 21/02/27 2008-2021 J. Schilling";
#endif

/*
 *	UNIX shell
 */

#include	"sh_policy.h"

#if !defined(DO_SET_O)
#undef	DO_GLOBALALIASES
#undef	DO_LOCALALIASES
#endif

	void		prversion	__PR((void));
	int		options		__PR((int argc, unsigned char **argv));
	void		setopts		__PR((void));
	void		setargs		__PR((unsigned char *argi[]));
static void		freedolh	__PR((void));
	struct dolnod	*freeargs	__PR((struct dolnod *blk));
static struct dolnod	*copyargs	__PR((unsigned char *[], int));
static	struct dolnod	*clean_args	__PR((struct dolnod *blk));
	void		clearup		__PR((void));
	struct dolnod	*savargs	__PR((int funcntp));
	void		restorargs	__PR((struct dolnod *olddolh,
							int funcntp));
	struct dolnod	*useargs	__PR((void));
static	unsigned char	*lookcopt	__PR((int wc));
#if	defined(DO_SYSALIAS) && \
	(defined(DO_GLOBALALIASES) || defined(DO_LOCALALIASES))
static	void		listaliasowner	__PR((int parse, int flagidx));
#endif
#ifdef	DO_SET_O
static	void		listopts	__PR((int parse));
#ifdef	DO_HOSTPROMPT
static	void		hostprompt	__PR((int on));
#endif
#ifdef	DO_PS34
static	void		ps_reset	__PR((void));
#endif
#endif

static struct dolnod *dolh;

/* Used to save outermost positional parameters */
static struct dolnod *globdolh;
static unsigned char **globdolv;
static int globdolc;

unsigned char	flagadr[20];

unsigned char	flagchar[] =
{
#if	defined(DO_SYSALIAS) && \
	(defined(DO_GLOBALALIASES) || defined(DO_LOCALALIASES))
	0,			/* set -o aliasowner= */
#endif
	'a',			/* -a / -o allexport */
#ifdef	DO_BGNICE
	0,			/* -o bgnice */
#endif
	'e',
#ifdef	DO_FDPIPE
	0,			/* set -o fdpipe */
#endif
#ifdef	DO_FULLEXCODE
	0,			/* set -o fullexitcode */
#endif
#if	defined(DO_SYSALIAS) && defined(DO_GLOBALALIASES)
	0,			/* set -o globalaliases */
#endif
#ifdef	DO_GLOBSKIPDOT
	0,			/* set -o globskipdot */
#endif
	'h',
#ifdef	DO_HASHCMDS
	0,			/* -o hashcmds, enable # commands */
#endif
#ifdef	DO_HOSTPROMPT
	0,			/* -o hostprompt, "<host> <user>> " prompt */
#endif
#ifdef	INTERACTIVE
	0,			/* -o ignoreeof POSIX name */
#endif
	'i',
	'k',			/* -k / -o keyword */
#if	defined(DO_SYSALIAS) && defined(DO_LOCALALIASES)
	0,			/* set -o localaliases */
#endif
	'm',
#ifdef	DO_NOCLOBBER
	'C',			/* -C, set -o noclobber */
#endif
	'n',
	'f',			/* -f / -o noglob */
#ifdef	DO_NOTIFY
	'b',			/* -b / -o notify */
#endif
	'u',			/* -u / -o nounset */
	't',			/* -t / -o onecmd */
	'P',
#ifdef	DO_SET_O
	0,			/* -o posix */
#endif
	'p',
#ifdef	DO_PS34
	0,			/* -o promptcmdsubst */
#endif
	'r',
	STDFLG,			/* -s / -o stdin */
	'V',
#ifdef	DO_TIME
	0,			/* set -o time */
#endif
#ifdef	INTERACTIVE
	0,			/* set -o ved */
#endif
	'v',
#ifdef	INTERACTIVE
	0,			/* set -o vi */
#endif
	'x',
	0
};
#define	NFLAGCHAR	((sizeof (flagchar) / sizeof (flagchar[0])) - 1)

#ifdef	DO_SET_O
char	*flagname[] =
{
#if	defined(DO_SYSALIAS) && \
	(defined(DO_GLOBALALIASES) || defined(DO_LOCALALIASES))
	"aliasowner",
#endif
	"allexport",		/* -a POSIX */
#ifdef	DO_BGNICE
	"bgnice",		/* -o bgnice */
#endif
	"errexit",		/* -e POSIX */
#ifdef	DO_FDPIPE
	"fdpipe",		/* e.g. 2| for pipe from stderr */
#endif
#ifdef	DO_FULLEXCODE
	"fullexitcode",		/* -o fullexitcode, do not mask $? */
#endif
#if	defined(DO_SYSALIAS) && defined(DO_GLOBALALIASES)
	"globalaliases",
#endif
#ifdef	DO_GLOBSKIPDOT
	"globskipdot",
#endif
	"hashall",		/* -h bash name (ksh93 uses "trackall") */
#ifdef	DO_HASHCMDS
	"hashcmds",		/* -o hashcmds, enable # commands */
#endif
#ifdef	DO_HOSTPROMPT
	"hostprompt",		/* -o hostprompt, "<host> <user>> " prompt */
#endif
#ifdef	INTERACTIVE
	"ignoreeof",		/* -o ignoreeof POSIX name */
#endif
	"interactive",		/* -i ksh93 name */
	"keyword",		/* -k bash/ksh93 name */
#if	defined(DO_SYSALIAS) && defined(DO_LOCALALIASES)
	"localaliases",
#endif
	"monitor",		/* -m POSIX */
#ifdef	DO_NOCLOBBER
	"noclobber",		/* -C, set -o noclobber */
#endif
	"noexec",		/* -n POSIX */
	"noglob",		/* -f POSIX */
#ifdef	DO_NOTIFY
	"notify",		/* -b POSIX */
#endif
	"nounset",		/* -u POSIX */
	"onecmd",		/* -t bash name */
	"pfsh",			/* -P Schily Bourne Shell */
#ifdef	DO_SET_O
	"posix",		/* -o posix */
#endif
	"privileged",		/* -p ksh93: only if really privileged */
#ifdef	DO_PS34
	"promptcmdsubst",	/* -o promptcmdsubst */
#endif
	"restricted",		/* -r ksh93 name */
	"stdin",		/* -s Schily name */
	"version",		/* -V Schily Bourne Shell */
#ifdef	DO_TIME
	"time",			/* -o time, enable timing */
#endif
#ifdef	INTERACTIVE
	"ved",
#endif
	"verbose",		/* -v POSIX */
#ifdef	INTERACTIVE
	"vi",
#endif
	"xtrace",		/* -x POSIX */
	0
};
#endif

unsigned long	flagval[]  =
{
#if	defined(DO_SYSALIAS) && \
	(defined(DO_GLOBALALIASES) || defined(DO_LOCALALIASES))
	fl2 | aliasownerflg,	/* -o aliasowner= */
#endif
	exportflg,		/* -a / -o allexport */
#ifdef	DO_BGNICE
	fl2 | bgniceflg,	/* -o bgnice */
#endif
	errflg,			/* -e */
#ifdef	DO_FDPIPE
	fl2 | fdpipeflg,	/* -o fdpipe */
#endif
#ifdef	DO_FULLEXCODE
	fl2 | fullexitcodeflg,	/* -o fullexitcode */
#endif
#if	defined(DO_SYSALIAS) && defined(DO_GLOBALALIASES)
	fl2 | globalaliasflg,	/* -o globalaliases */
#endif
#ifdef	DO_GLOBSKIPDOT
	fl2 | globskipdot,	/* -o globskipdot */
#endif
	hashflg,		/* -h / -o hashall */
#ifdef	DO_HASHCMDS
	fl2 | hashcmdsflg,	/* -o hashcmds, enable # commands */
#endif
#ifdef	DO_HOSTPROMPT
	fl2 | hostpromptflg,	/* -o hostprompt, "<host> <user>> " prompt */
#endif
#ifdef	INTERACTIVE
	fl2 | ignoreeofflg,	/* -o ignoreeof POSIX name */
#endif
	intflg,			/* -i / -o interactive */
	keyflg,			/* -k / -o keyword */
#if	defined(DO_SYSALIAS) && defined(DO_LOCALALIASES)
	fl2 | localaliasflg,	/* -o localaliases */
#endif
	monitorflg,		/* -m / -o monitor */
#ifdef	DO_NOCLOBBER
	fl2 | noclobberflg,	/* -C, set -o noclobber */
#endif
	noexec,			/* -n / -o noexec */
	nofngflg,		/* -f / -o noglob */
#ifdef	DO_NOTIFY
	notifyflg,		/* -b / -o notify */
#endif
	setflg,			/* -u / -o nounset */
	oneflg,			/* -t / -o onecmd */
	pfshflg,		/* -P */
#ifdef	DO_SET_O
	fl2 | posixflg,		/* -o posix */
#endif
	privflg,		/* -p */
#ifdef	DO_PS34
	fl2 | promptcmdsubst,	/* -o promptcmdsubst */
#endif
	rshflg,			/* -r / -o restrictive */
	stdflg,			/* -s / -o stdin */
	fl2 | versflg,		/* -V */
#ifdef	DO_TIME
	fl2 | timeflg,		/* -o time */
#endif
#ifdef	INTERACTIVE
	fl2 | vedflg,		/* -o ved */
#endif
	readpr,			/* -v / -o verbose */
#ifdef	INTERACTIVE
	fl2 | viflg,		/* -o vi */
#endif
	execpr,			/* -x / -o xtrace */
	0
};

unsigned char *shvers;

/* ========	option handling	======== */

#ifndef	VSHNAME
#define	VSHNAME	"sh"
#endif

void
prversion()
{
	char	vbuf[BUFFERSIZE];

	snprintf(vbuf, sizeof (vbuf),
	    "%s %s\n",
	    shname, shvers);
	prs(UC vbuf);
	if (dolv == NULL) {
		/*
		 * We have been called as a result of a sh command line flag.
		 * Print the version information and exit.
		 */
		prs(UC "\n");
		prs(UC "Copyright (C) 1984-1989 AT&T\n");
		prs(UC "Copyright (C) 1989-2009 Sun Microsystems\n");
#ifdef	INTERACTIVE
		prs(UC "Copyright (C) 1982-2021 Joerg Schilling\n");
#else
		prs(UC "Copyright (C) 1985-2021 Joerg Schilling\n");
#endif
		exitsh(0);
	}
}

int
options(argc, argv)
	int		argc;
	unsigned char	**argv;
{
	unsigned char *cp;
	unsigned char **argp = argv;
	unsigned char *flagc;
	int		len;
	wchar_t		wc;
	unsigned long	fv;

	if (shvers == NULL) {
		char	vbuf[BUFFERSIZE];
		size_t	vlen;

		vlen = snprintf(vbuf, sizeof (vbuf),
			    "version %s %s %s (%s-%s-%s)",
			    VSHNAME,
			    VERSION_DATE, VERSION_STR,
			    HOST_CPU, HOST_VENDOR, HOST_OS);
		shvers = alloc(vlen + 1);
		strcpy((char *)shvers, vbuf);
	}

#ifdef	DO_POSIX_SET
	dashdash = 0;
#endif
#ifdef	DO_MULTI_OPT
again:
#endif
	if (argc > 1 && *argp[1] == '-') {
		cp = argp[1];
		/*
		 * Allow "--version" by mapping it to "-V".
		 */
		if ((strcmp((char *)&cp[1], "version") == 0) ||
		    (cp[1] == '-' && strcmp((char *)&cp[2], "version") == 0)) {
			cp = UC "-V";
		} else if (cp[1] == '-' && cp[2] == '\0') {
			/*
			 * if first argument is "--" then options are not
			 * to be changed. Fix for problems getting
			 * $1 starting with a "-"
			 */
			argp[1] = argp[0];
			argc--;
#ifdef	DO_POSIX_SET
			if (comdiv == cp) {
				/*
				 * Support sh -c -- command
				 */
				if ((comdiv = argp[2]) == NULL) {
					failed(argv[1], mssgargn);
					return (-1);
				} else {
					argp[2] = argp[3]?argp[3]:argp[0];
					argc--;
				}
			}
			dashdash++;
#endif
#ifdef	DO_MULTI_OPT
			setopts();
#endif
			return (argc);
		}
#ifdef	DO_MULTI_OPT
		/*
		 * Mark that we will later need to correct comdiv.
		 */
		if (comdiv && dolv == NULL)
			comdiv = UC -1;
#endif
		if (cp[1] == '\0')
			flags &= ~(execpr|readpr);

		/*
		 * Step along 'flagchar[]' looking for matches.
		 * 'sicrp' are not legal with 'set' command.
		 */
		(void) mbtowc(NULL, NULL, 0);
		cp++;
		while (*cp) {
			if ((len = mbtowc(&wc, (char *)cp, MB_LEN_MAX)) <= 0) {
				(void) mbtowc(NULL, NULL, 0);
				len = 1;
				wc = (unsigned char)*cp;
				failed(argv[1], badopt);
				return (-1);
			}
			cp += len;

#ifdef	DO_SET_O
			if (wc == 'o') {		/* set set -o */
				unsigned char *argarg;
				int	dolistopts = argc <= 2 ||
						argp[2][0] == '-' ||
						argp[2][0] == '+';

				if (dolistopts) {
					listopts(0);
					continue;
				}
				argarg = UC strchr((char *)argp[2], '=');
				if (argarg != NULL)
					*argarg = '\0';
				if ((flagc = lookopt(argp[2])) != NULL) {
						argp[1] = argp[0];
						argp++;
						argc--;
						wc = *flagc;
#if	defined(DO_SYSALIAS) && \
	(defined(DO_GLOBALALIASES) || defined(DO_LOCALALIASES))
							/* LINTED */
						if (flagval[flagc-flagchar] ==
						    (fl2 | aliasownerflg)) {
							char *owner;

							if (argarg != NULL)
								owner = (char *)
								    &argarg[1];
							else
								owner = "";
							ab_setaltowner(
							    GLOBAL_AB, owner);
							ab_setaltowner(
							    LOCAL_AB, owner);
						}
#endif
				}
				if (argarg != NULL)
					*argarg = '=';
				if (flagc == NULL || wc != *flagc) {
					if (argc > 2) {
						failed(argp[2], badopt);
						return (-1);
					}
					continue;
				}
			} else {		/* Not set -o, but: set -c */
#else	/* !DO_SET_O */
			{
#endif
				flagc = lookcopt(wc);
			}
			if (wc == *flagc) {
				if (eq(argv[0], "set") &&
				    wc && any(wc, UC "sicrp")) {
					failed(argv[1], badopt);
					return (-1);
				} else {
					unsigned long *fp = &flags;
#ifdef	DO_PS34
					unsigned long oflags;
#endif

							/* LINTED */
					fv = flagval[flagc-flagchar];
					if (fv & fl2)
						fp = &flags2;
#ifdef	DO_PS34
					oflags = *fp;
#endif
					*fp |= fv & ~fl2;
					/*
					 * Disallow to set -n on an interactive
					 * shell as this cannot be reset.
					 */
					if (flags & intflg)
						flags &= ~noexec;
#ifdef	INTERACTIVE
					flags2 &= ~viflg;
#endif
					if (fv == errflg)
						eflag = errflg;

#ifdef	DO_MONITOR_SCRIPT
					if (fv == monitorflg &&
					    (flags & jcflg) == 0) {
						startjobs();
					}
#endif

#ifdef	EXECATTR_FILENAME		/* from <exec_attr.h> */
					if (fv == pfshflg)
						secpolicy_init();
#endif
					if (fv == (fl2 | versflg)) {
						flags2 &= ~versflg;
						prversion();
					}
#if	defined(DO_SYSALIAS) && defined(DO_GLOBALALIASES)
					if (fv == (fl2 | globalaliasflg)) {
						if (homenod.namval) {
						    catpath(homenod.namval,
						    UC globalname);
						    ab_use(GLOBAL_AB,
						    (char *)make(curstak()));
						}
					}
#endif
#if	defined(DO_SYSALIAS) && defined(DO_LOCALALIASES)
					if (fv == (fl2 | localaliasflg)) {
						ab_use(LOCAL_AB,
							(char *)localname);
					}
#endif
#ifdef	DO_HOSTPROMPT
					if (fv == (fl2 | hostpromptflg))
						hostprompt(TRUE);
#endif
#ifdef	DO_PS34
					if (fv == (fl2 | promptcmdsubst) &&
					    (oflags & promptcmdsubst) == 0)
						ps_reset();
#endif
#ifdef	DO_POSIX_EXPORT_ENV
					if (fv == (fl2 | posixflg))
						namscan(exportenv);
#endif
				}
			} else if (wc == 'c' && argc > 2 && comdiv == 0) {
				comdiv = argp[2];
				argp[1] = argp[0];
				argp++;
				argc--;
#ifdef	DO_POSIX_SET
				if (*argp[1] == '-' ||	/* Check for --    */
				    *argp[1] == '+')	/* or more options */
					goto again;
#endif
			} else {
				failed(argv[1], badopt);
				return (-1);
			}
		}
		argp[1] = argp[0];
		argc--;
		argp++;
	} else if (argc > 1 &&
		    *argp[1] == '+') { /* unset flags x, k, t, n, v, e, u */
#ifdef	DO_MULTI_OPT
		/*
		 * Mark that we will later need to correct comdiv.
		 */
		if (comdiv && dolv == NULL)
			comdiv = UC -1;
#endif
		(void) mbtowc(NULL, NULL, 0);
		cp = argp[1];
		cp++;
		while (*cp) {
			if ((len = mbtowc(&wc, (char *)cp, MB_LEN_MAX)) <= 0) {
				(void) mbtowc(NULL, NULL, 0);
				cp++;
				continue;
			}
			cp += len;

#ifdef	DO_SET_O
			if (wc == 'o') {		/* set +o */
				int	dolistopts = argc <= 2 ||
						argp[2][0] == '-' ||
						argp[2][0] == '+';

				if (dolistopts) {
					listopts(1);
					continue;
				}
				if ((flagc = lookopt(argp[2])) != NULL) {
						argp[1] = argp[0];
						argp++;
						argc--;
						wc = *flagc;
				}
				if (flagc == NULL || wc != *flagc) {
					if (argc > 2) {
						failed(argp[2], badopt);
						return (-1);
					}
					continue;
				}
			} else {		/* Not set +o, but: set +c */
#else	/* !DO_SET_O */
			{
#endif
				flagc = lookcopt(wc);
			}
			/*
			 * step through flags
			 */
			if (wc == 0 ||
			    (!any(wc, UC "sicrp") && wc == *flagc)) {
				unsigned long *fp = &flags;
							/* LINTED */
				fv = flagval[flagc-flagchar];
				if (fv & fl2)
					fp = &flags2;
				*fp &= ~fv;
				if (wc == 'e')
					eflag = 0;
#ifdef	EXECATTR_FILENAME
				if (fv == pfshflg)
					secpolicy_end();
#endif
#if	defined(DO_SYSALIAS) && defined(DO_GLOBALALIASES)
				if (fv == (fl2 | globalaliasflg)) {
					ab_use(GLOBAL_AB, NULL);
				}
#endif
#if	defined(DO_SYSALIAS) && defined(DO_LOCALALIASES)
				if (fv == (fl2 | localaliasflg)) {
					ab_use(LOCAL_AB, NULL);
				}
#endif
#if	defined(DO_SYSALIAS) && \
	(defined(DO_GLOBALALIASES) || defined(DO_LOCALALIASES))
				if (fv == (fl2 | aliasownerflg)) {
					ab_setaltowner(GLOBAL_AB, "");
					ab_setaltowner(LOCAL_AB, "");
				}
#endif
#ifdef	DO_HOSTPROMPT
				if (fv == (fl2 | hostpromptflg))
					hostprompt(FALSE);
#endif
#ifdef	DO_POSIX_EXPORT_ENV
				if (fv == (fl2 | posixflg))
					namscan(deexportenv);
#endif
			} else {
				failed(argv[1], badopt);
				return (-1);
			}
		}
		argp[1] = argp[0];
		argc--;
		argp++;
	}
#ifdef	DO_MULTI_OPT
	if ((comdiv == NULL || dolv != NULL) &&
	    argc > 1 && (*argp[1] == '-' || *argp[1] == '+'))
		goto again;

	if (comdiv == UC -1 && dolv == NULL) {
		/*
		 * Correct comdiv to point past last option.
		 */
		if (argc > 1) {
			comdiv = argp[1];
			argp[1] = argp[0];
			argv++;
			argc--;
		} else {
			failed(argv[1], mssgargn);
			return (-1);
		}
	}
#endif

	setopts();
	return (argc);
}

/*
 * set up $-
 * $- is only constructed from flag values in the basic "flags"
 */
void
setopts()
{
	unsigned char	*flagc;
	unsigned char	*flagp;
	unsigned long	oflags = flags;

	flagp = flagadr;
	flags |= eflag;
	if (flags) {
		flagc = flagchar;
		while (flagc < &flagchar[NFLAGCHAR]) {
			if (*flagc && optval(flagc))
				*flagp++ = *flagc;
			flagc++;
		}
	}
	*flagp = 0;
	flags = oflags;
}

/*
 * sets up positional parameters
 */
void
setargs(argi)
	unsigned char	*argi[];
{
	unsigned char **argp = argi;	/* count args */
	int argn = 0;

	while (*argp++ != UC ENDARGS)
		argn++;
	/*
	 * free old ones unless on for loop chain
	 */
	freedolh();
	dolh = copyargs(argi, argn);
	dolc = argn - 1;
}


static void
freedolh()
{
	unsigned char **argp;
	struct dolnod *argblk;

	if ((argblk = dolh) != NULL) {
		if ((--argblk->doluse) == 0) {
			for (argp = argblk->dolarg; *argp != UC ENDARGS; argp++)
				free(*argp);
			free(argblk->dolarg);
			free(argblk);
		}
	}
}

struct dolnod *
freeargs(blk)
	struct dolnod *blk;
{
	unsigned char **argp;
	struct dolnod *argr = 0;
	struct dolnod *argblk;
	int cnt;

	if ((argblk = blk) != NULL) {
		argr = argblk->dolnxt;
		cnt  = --argblk->doluse;

		if (argblk == dolh) {
			if (cnt == 1)
				return (argr);
			else
				return (argblk);
		} else {
			if (cnt == 0) {
				for (argp = argblk->dolarg;
				    *argp != UC ENDARGS; argp++) {
					free(*argp);
				}
				free(argblk->dolarg);
				free(argblk);
			}
		}
	}
	return (argr);
}

static struct dolnod *
copyargs(from, n)
	unsigned char	*from[];
	int		n;
{
	struct dolnod *np = (struct dolnod *)alloc(sizeof (struct dolnod));
	unsigned char **fp = from;
	unsigned char **pp;

	np -> dolnxt = 0;
	np->doluse = 1;	/* use count */
	pp = np->dolarg = (unsigned char **)alloc((n+1)*sizeof (char *));
	dolv = pp;

	while (n--)
		*pp++ = make(*fp++);
	*pp++ = ENDARGS;
	return (np);
}


static struct dolnod *
clean_args(blk)
	struct dolnod *blk;
{
	unsigned char **argp;
	struct dolnod *argr = 0;
	struct dolnod *argblk;

	if ((argblk = blk) != NULL) {
		argr = argblk->dolnxt;

		if (argblk == dolh) {
			argblk->doluse = 1;
		} else {
			for (argp = argblk->dolarg; *argp != UC ENDARGS; argp++)
				free(*argp);
			free(argblk->dolarg);
			free(argblk);
		}
	}
	return (argr);
}

void
clearup()
{
	/*
	 * force `for' $* lists to go away
	 */
	if (globdolv)
		dolv = globdolv;
	if (globdolc)
		dolc = globdolc;
	if (globdolh)
		dolh = globdolh;
	globdolv = 0;
	globdolc = 0;
	globdolh = 0;
	while ((argfor = clean_args(argfor)) != NULL)
		/* LINTED */
		;
	/*
	 * clean up io files
	 */
	while (pop())
		/* LINTED */
		;

	/*
	 * Clean up pipe file descriptor
	 * from command substitution
	 */

	if (savpipe != -1) {
		close(savpipe);
		savpipe = -1;
	}

	/*
	 * clean up tmp files
	 */
	while (poptemp())
		/* LINTED */
		;
}

/*
 * Save positiional parameters before outermost function invocation
 * in case we are interrupted.
 * Increment use count for current positional parameters so that they aren't
 * thrown away.
 */

struct dolnod *
savargs(funcntp)
	int	funcntp;
{
	if (!funcntp) {
		globdolh = dolh;
		globdolv = dolv;
		globdolc = dolc;
	}
	useargs();
	return (dolh);
}

/*
 * After function invocation, free positional parameters,
 * restore old positional parameters, and restore
 * use count.
 */

void
restorargs(olddolh, funcntp)
	struct dolnod	*olddolh;
	int		funcntp;
{
	if (argfor != olddolh)
		while ((argfor = clean_args(argfor)) != olddolh && argfor)
			/* LINTED */
			;
	if (!argfor)
		return;
	freedolh();
	dolh = olddolh;

	/*
	 * increment use count so arguments aren't freed
	 */
	if (dolh)
		dolh -> doluse++;
	argfor = freeargs(dolh);
	if (funcntp == 1) {
		globdolh = 0;
		globdolv = 0;
		globdolc = 0;
	}
}

struct dolnod *
useargs()
{
	if (dolh) {
		if (dolh->doluse++ == 1) {
			dolh->dolnxt = argfor;
			argfor = dolh;
		}
	}
	return (dolh);
}

static unsigned char *
lookcopt(wc)
	int		wc;
{
	unsigned char *flagc;

	flagc = flagchar;
	while (flagc < &flagchar[NFLAGCHAR]) {
		if (*flagc && wc == *flagc)
			break;
		flagc++;
	}
	return (flagc);
}

int
optval(flagc)
	unsigned char *flagc;
{
	unsigned long	fv;
	unsigned long	*fp;

	if (flagc == NULL)
		return (0);

				/* LINTED */
	fv = flagval[flagc-flagchar];
	fp = &flags;
	if (fv & fl2) {
		fp = &flags2;
		fv &= ~fl2;
	}
	return (*fp & fv ? 1:0);
}

#ifdef	DO_SET_O
unsigned char *
lookopt(name)
	unsigned char	*name;
{
	unsigned char *flagc;

	for (flagc = flagchar;
				/* LINTED */
	    flagname[flagc-flagchar]; flagc++) {
				/* LINTED */
		if (eq(name,
		    flagname[flagc-flagchar])) {
			return (flagc);
		}
	}
	return (NULL);
}

#if	defined(DO_SYSALIAS) && \
	(defined(DO_GLOBALALIASES) || defined(DO_LOCALALIASES))
static void
listaliasowner(parse, flagidx)
	int	parse;
	int	flagidx;
{
	uid_t	altuid = ab_getaltowner(GLOBAL_AB);

	if (parse) {
		prs_buff(UC "set ");
		if (altuid == (uid_t)-1)
			prs_buff(UC "+o ");
		else
			prs_buff(UC "-o ");
	}
	prs_buff(UC flagname[flagidx]);
	if (altuid == (uid_t)-1 && parse) {
		prc_buff(NL);
		return;
	}
	prs_buff(UC "=");
	if (altuid != (uid_t)-1)
		prs_buff(UC ab_getaltoname(GLOBAL_AB));
	prc_buff(NL);
}
#endif

static void
listopts(parse)
	int	parse;
{
	unsigned char *flagc;
	int		len;
	unsigned long	fv;

					/* LINTED */
	for (flagc = flagchar; flagname[flagc-flagchar]; flagc++) {
		if (*flagc == 'V')
			continue;
					/* LINTED */
		fv = flagval[flagc-flagchar];
#if	defined(DO_SYSALIAS) && \
	(defined(DO_GLOBALALIASES) || defined(DO_LOCALALIASES))
		if (fv == (fl2 | aliasownerflg)) {
					/* LINTED */
			listaliasowner(parse, flagc-flagchar);
			continue;
		}
#endif
		fv = optval(flagc);
		if (parse) {
			if (any(*flagc, UC "sicrp"))	/* Unsettable?    */
				continue;		/* so do not list */
			prs_buff(UC "set ");
			prs_buff(UC(fv ? "-":"+"));
			prs_buff(UC "o ");
		}
					/* LINTED */
		prs_buff(UC flagname[flagc-flagchar]);
		if (parse) {
			prc_buff(NL);
			continue;
		}
					/* LINTED */
		len = length(UC flagname[flagc-flagchar]);
		while (++len <= 16)
			prc_buff(SPACE);
		prc_buff(TAB);
		prs_buff(UC(fv ? "on":"off"));
		prc_buff(NL);
	}
}

#ifdef	DO_HOSTPROMPT
#include <schily/utsname.h>
#include <schily/pwd.h>
static void
hostprompt(on)
	int	on;
{
#ifdef	HAVE_UNAME
	struct utsname	un;
	struct passwd	*pw;
	unsigned char	pr[1000];
	unsigned char	*p;
	uid_t		euid = geteuid();

	if (on) {
		if (ps1nod.namval != NULL &&
		    !eq(ps1nod.namval, (euid ? stdprompt : supprompt)))
			return;
	}
	uname(&un);
	pw = getpwuid(euid);
	if (pw == NULL)
		return;
	if ((length(UC un.nodename) + length(UC pw->pw_name) + 3) >
	    sizeof (pr))
		return;
	p = movstr(UC un.nodename, pr);
	*p++ = ' ';
	p = movstr(UC pw->pw_name, p);
	*p++ = '>';
	*p++ = ' ';
	*p++ = '\0';
	if (!on) {
		if (ps1nod.namval != NULL &&
		    !eq(ps1nod.namval, pr))
			return;
	}
	if (on)
		assign(&ps1nod, pr);
	else
		assign(&ps1nod, UC(euid ? stdprompt : supprompt));
#endif
}
#endif	/* DO_HOSTPROMPT */

#ifdef	DO_PS34
/*
 * Reset user specific prompts to their default values.
 */
static void
ps_reset()
{
	assign(&ps1nod, UC(geteuid() ? stdprompt : supprompt));
	assign(&ps2nod, UC readmsg);
#ifdef	__needed_
	assign(&ps3nod, UC selectmsg);
#endif
	assign(&ps4nod, UC execpmsg);
#ifdef	DO_HOSTPROMPT
	if (flags2 & hostpromptflg)
		hostprompt(TRUE);
#endif
}
#endif	/* DO_PS34 */
#endif	/* DO_SET_O */
