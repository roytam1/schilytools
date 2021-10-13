/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may use this file only in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
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
/* Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T */
/* All Rights Reserved */
/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2006-2020 J. Schilling
 *
 * @(#)prt.c	1.47 20/08/23 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)prt.c 1.47 20/08/23 J. Schilling"
#endif
/*
 * @(#)prt.c 1.22 06/12/12
 */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved. The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(sun)
#pragma ident	"@(#)ucbprt:prt.c"
#endif

/*
	Program to print parts or all of an SCCS file.
	Arguments to the program may appear in any order
	and consist of keyletters, which begin with '-',
	and named files.

	If a directory is given as an argument, each
	SCCS file within the directory is processed as if
	it had been specifically named. If a name of '-'
	is given, the standard input is read for a list
	of names of SCCS files to be processed.
	Non-SCCS files are ignored.
*/


#define		SCCS_MAIN			/* define global vars */
#include	<defines.h>
#include	<version.h>
#include	<had.h>
#include	<i18n.h>
#include	<schily/sysexits.h>

#define	NOEOF	0
#define	BLANK(p)	while (!(*p == '\0' || *p == ' ' || *p == '\t')) p++;

static Nparms	N;			/* Keep -N parameters		*/
static Xparms	X;			/* Keep -X parameters		*/
static FILE *iptr;
static char *line = NULL;
static size_t line_size = 0;
static char statistics[25];
static struct delent {
	char type;
	char *osid;
	char *datetime;
	char *pgmr;
	char *serial;
	char *pred;
} del;
static int num_files;
static int prefix;
static time_t	cutoff;
static time_t	revcut;
static int linenum;
static char *ysid;
static char *flagdesc[26] = {
			NOGETTEXT(""),
			NOGETTEXT("branch"),
			NOGETTEXT("ceiling"),
			NOGETTEXT("default SID"),
			NOGETTEXT("encoded"),
			NOGETTEXT("floor"),
			NOGETTEXT(""),
			NOGETTEXT(""),
			NOGETTEXT("id keywd err/warn"),
			NOGETTEXT("joint edit"),
			NOGETTEXT(""),
			NOGETTEXT("locked releases"),
			NOGETTEXT("module"),
			NOGETTEXT("null delta"),
			NOGETTEXT(""),
			NOGETTEXT(""),
			NOGETTEXT("csect name"),
			NOGETTEXT(""),
			NOGETTEXT("keywd scan lines"),
			NOGETTEXT("type"),
			NOGETTEXT(""),
			NOGETTEXT("validate MRs"),
			NOGETTEXT(""),
			NOGETTEXT("extensions"),
			NOGETTEXT("expand keywds"),
			NOGETTEXT("")
};

	int	main __PR((int argc, char **argv));
static void 	prt __PR((char *file));
static void	getdel __PR((register struct delent *delp, register char *lp));
static char	*read_to __PR((register int ch));
static char	*lineread __PR((register int eof));
static void	printdel __PR((register char *file, register struct delent *delp));
static void	printit __PR((register char *file, register char *str, register char *cp));

int
main(argc, argv)
int argc;
char *argv[];
{
	register int j;
	register char *p;
	int  c;
	int testklt;
	extern int Fcnt;
	int current_optind;
	int no_arg;

	/*
	 * Set locale for all categories.
	 */
	setlocale(LC_ALL, NOGETTEXT(""));

	sccs_setinsbase(INS_BASE);

	/*
	 * Set directory to search for general l10n SCCS messages.
	 */
#ifdef	PROTOTYPES
	(void) bindtextdomain(NOGETTEXT("SUNW_SPRO_SCCS"),
	    NOGETTEXT(INS_BASE "/" SCCS_BIN_PRE "lib/locale/"));
#else
	(void) bindtextdomain(NOGETTEXT("SUNW_SPRO_SCCS"),
	    NOGETTEXT("/usr/ccs/lib/locale/"));
#endif

	(void) textdomain(NOGETTEXT("SUNW_SPRO_SCCS"));

	tzset();	/* Set up timezome related vars */

#ifdef	SCHILY_BUILD
	save_args(argc, argv);
#endif
	/*
	Set flags for 'fatal' to issue message, call clean-up
	routine, and terminate processing.
	*/
	Fflags = FTLMSG | FTLCLN | FTLEXIT;
#ifdef	SCCS_FATALHELP
	Fflags |= FTLFUNC;
	Ffunc = sccsfatalhelp;
#endif

	testklt = 1;

	/*
	The following loop processes keyletters and arguments.
	Note that these are processed only once for each
	invocation of 'main'.
	*/

	current_optind = 1;
	optind = 1;
	opterr = 0;
	no_arg = 0;
	j = 1;
	/*CONSTCOND*/
	while (1) {
			if (current_optind < optind) {
			    if (optind > j+1) {
				if ((argv[j+1][0] != '-') && (no_arg == 0) &&
				    ((argv[j][0] == 'c' && argv[j+1][0] <= '9') ||
				    (argv[j][0] == 'r' && argv[j+1][0] <= '9') ||
				    (argv[j][0] == 'y' && argv[j+1][0] <= '9'))) {
					argv[j+1] = NULL;
				} else {
					optind = j+1;
					current_optind = optind;
				}
			    }
			    if (argv[j][0] == '-') {
				argv[j] = 0;
			    }
			    current_optind = optind;
			}
			no_arg = 0;
			j = current_optind;
			c = getopt(argc, argv, "()-r:c:y:esdaiuftbtN:X:V(version)");

				/* this takes care of options given after
				** file names.
				*/
			if (c == EOF) {
			    if (optind < argc) {
				/* if it's due to -- then break; */
				if (argv[j][0] == '-' &&
				    argv[j][1] == '-') {
					argv[j] = 0;
					break;
				}
				optind++;
				current_optind = optind;
				continue;
			    } else {
				break;
			    }
			}
			p = optarg;
			switch (c) {
			case 'e':	/* print everything but body */
			case 's':	/* print only delta desc. and stats */
			case 'd':	/* print whole delta table */
			case 'a':	/* print all deltas */
			case 'i':	/* print inc, exc, and ignore info */
			case 'u':	/* print users allowed to do deltas */
			case 'f':	/* print flags */
			case 't':	/* print descriptive user-text */
			case 'b':	/* print body */
				break;
			case 'y':	/* delta cutoff */
				ysid = p;
				if (p[0] < '0' || p[0] > '9') {
					ysid = "";
				}
				prefix++;
				break;

			case 'c':	/* time cutoff */
				if (HADR) {
				    fatal(gettext("both 'c' and 'r' keyletters specified (pr2)"));
				}
				if (p[0] ==  '-') {
				    /* no cutoff date given */
				    prefix++;
				    break;
				}
				if (p[0] > '9') {
				    prefix++;
				    break;
				}
				if (*p && parse_date(p, &cutoff, 0))
					fatal(gettext("bad date/time (cm5)"));
				prefix++;
				break;

			case 'r':	/* reverse time cutoff */
				if (HADC) {
				    fatal(gettext("both 'c' and 'r' keyletters specified (pr2)"));
				}
				if (p[0] ==  '-') {
				    /* no cutoff date given */
				    prefix++;
				    break;
				}
				if (p[0] > '9') {
				    prefix++;
				    break;
				}
				if (*p && parse_date(p, &revcut, 0))
					fatal(gettext("bad date/time (cm5)"));
				prefix++;
				break;

			case 'N':	/* Bulk names */
				initN(&N);
				if (optarg == argv[j+1]) {
				   no_arg = 1;
				   break;
				}
				N.n_parm = p;
				break;

			case 'X':	/* -Xtended options */
				X.x_parm = optarg;
				X.x_flags = XO_NULLPATH;
				if (!parseX(&X))
					goto err;
				had[NLOWER+c-'A'] = 0;	/* Allow mult -X */
				break;

			case 'V':		/* version */
				printf(gettext(
				    "prt %s-SCCS version %s %s (%s-%s-%s)\n"),
					PROVIDER,
					VERSION,
					VDATE,
					HOST_CPU, HOST_VENDOR, HOST_OS);
				exit(EX_OK);

			default:
			err:
				fatal(gettext("Usage: prt [ -abdefistu ][ -c date-time ]\n\t[ -r date-time ][ -ySID ][ -N[bulk-spec]][ -Xxopts ] s.filename..."));
			}

			/*
			 * Make sure that we only collect option letters from
			 * the range 'a'..'z' and 'A'..'Z'.
			 */
			if (ALPHA(c) &&
			    (had[LOWER(c)? c-'a' : NLOWER+c-'A']++ && testklt++)) {
				if (c != 'X')
					fatal(gettext("key letter twice (cm2)"));
			}
#if 0
			if (((c == 'c') || (c == 'r')||(c == 'y')) && (p[0] > '9')) {
			    argv[j] = NULL;
			    break;
			}
#endif
	}

	for (j = 1; j < argc; j++) {
		if (argv[j]) {
			num_files++;
		}
	}

	if (num_files == 0)
		fatal(gettext("missing file arg (cm3)"));

	if (HADC && HADR)
		fatal(gettext("both 'c' and 'r' keyletters specified (pr2)"));

	setsig();
	xsethome(NULL);
	if (HADUCN) {					/* Parse -N args  */
		parseN(&N);

		if (N.n_sdot && (sethomestat & SETHOME_OFFTREE))
			fatal(gettext("-Ns. not supported in off-tree project mode"));
	}

	/*
	Change flags for 'fatal' so that it will return to this
	routine (main) instead of terminating processing.
	*/
	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;

	/*
	Call 'prt' routine for each file argument.
	*/
	for (j = 1; j < argc; j++)
		if ((p = argv[j]) != NULL)
			do_file(p, prt, 1, N.n_sdot, &X);

	return (Fcnt ? 1 : 0);
}


/*
	Routine that actually performs the 'prt' functions.
*/

static void
prt(file)
char *file;
{
	int stopdel;
	int user, flag, text;
	char *p;
	time_t	bindate;
#if defined(BUG_1205145) || defined(GMT_TIME)
	time_t	tim;
	struct tm *tmp;
#endif	/* defined(BUG_1205145) || defined(GMT_TIME) */

	if (setjmp(Fjmp))	/* set up to return here from 'fatal' */
		return;		/* and return to caller of prt */
	if (HADUCN) {
#ifdef	__needed__
		char	*ofile = file;
#endif

		file = bulkprepare(&N, file);
		if (file == NULL) {
#ifdef	__needed__
			if (N.n_ifile)
				ofile = N.n_ifile;
#endif
			/*
			 * The error is typically
			 * "directory specified as s-file (cm14)"
			 */
			fatal(gettext(bulkerror(&N)));
		}
	}

	if (HADE)
		HADD = HADI = HADU = HADF = HADT = 1;

	if (!HADU && !HADF && !HADT && !HADB)
		HADD = 1;

	if (!HADD)
		HADR = HADS = HADA = HADI = HADY = HADC = 0;

	if (HADS && HADI)
		fatal(gettext("s and i conflict (pr1)"));

	iptr = xfopen(file, O_RDONLY|O_BINARY);
	linenum = 0;

	p = lineread(NOEOF);
	if (*p++ != CTLCHAR || *p != HEAD)
		fatal(gettext("not an sccs file (co2)"));

	stopdel = 0;

	if (!prefix) {
		printf("\n%s:\n", file);
	}
	if (HADD) {
		while (((p = lineread(NOEOF)) != NULL) && *p++ == CTLCHAR &&
				*p++ == STATS && !stopdel) {
			NONBLANK(p);
			copy(p, statistics);

			p = lineread(NOEOF);
			getdel(&del, p);

#if defined(BUG_1205145) || defined(GMT_TIME)
			date_ab(del.datetime, &bindate, 0);
			/*
			Local time corrections before date_ba() call.
			Because this function uses gmtime() instead of localtime().
			*/
			tmp = localtime(&bindate);
			tim = mklgmtime(tmp);
			/*
			 * Avoid to use more space as expectecd in del.datetime
			 */
#if SIZEOF_TIME_T == 4
			if (tim < Y1969)
#else
			if ((tim < Y1969) ||
			    (tim >= Y2069))
#endif
				date_bal(&tim, del.datetime, 0); /* 4 digit year */
			else
				date_ba(&tim, del.datetime, 0);	/* 2 digit year */

#endif	/* defined(BUG_1205145) || defined(GMT_TIME) */

			if (!HADA && (del.type != 'D' && del.type != 'U')) {
				(void) read_to(EDELTAB);
				continue;
			}
			if (HADC) {
#if !(defined(BUG_1205145) || defined(GMT_TIME))
				date_ab(del.datetime, &bindate, 0);
#endif
				if (bindate < cutoff) {
					stopdel = 1;
					break;
				}
			}
			if (HADR) {
#if !(defined(BUG_1205145) || defined(GMT_TIME))
				date_ab(del.datetime, &bindate, 0);
#endif
				if (bindate >= revcut) {
					(void) read_to(EDELTAB);
					continue;
				}
			}
			if (HADY && (equal(del.osid, ysid) || !(*ysid)))
				stopdel = 1;

			printdel(file, &del);

			while (((p = lineread(NOEOF)) != NULL) && *p++ == CTLCHAR) {
				if (*p == EDELTAB)
					break;
				switch (*p) {
				case INCLUDE:
					if (HADI)
						printit(file, gettext("Included:\t"), p);
					break;

				case EXCLUDE:
					if (HADI)
						printit(file, gettext("Excluded:\t"), p);
					break;

				case IGNORE:
					if (HADI)
						printit(file, gettext("Ignored:\t"), p);
					break;

				case MRNUM:
					if (!HADS)
						printit(file, gettext("MRs:\t"), p);
					break;

				case SIDEXTENS:
					if (!HADS)
						printit(file, gettext("SIDext:\t"), p);
					break;

				case COMMENTS:
					if (!HADS)
						printit(file, "", p);
					break;

				default:
					sprintf(SccsError, gettext("format error at line %d (co4)"), linenum);
					fatal(SccsError);
				}
			}
		}
		if (prefix)
			printf("\n");

		if (stopdel && !(line[0] == CTLCHAR && line[1] == BUSERNAM))
			(void) read_to(BUSERNAM);
	}
	else
		(void) read_to(BUSERNAM);

	if (HADU) {
		user = 0;
		printf(gettext("\n Users allowed to make deltas -- \n"));
		while (((p = lineread(NOEOF)) != NULL) && *p != CTLCHAR) {
			user = 1;
			printf("\t%s", p);
		}
		if (!user)
			printf(gettext("\teveryone\n"));
	}
	else
		(void) read_to(EUSERNAM);

	if (HADF) {
		flag = 0;
		printf("\n%s\n", "Flags --");
		while (((p = lineread(NOEOF)) != NULL) && *p++ == CTLCHAR &&
				*p++ == FLAG) {
			flag = 1;
			NONBLANK(p);
			/*
			 * The 'e' flag (file in encoded form) requires
			 * special treatment, as some versions of admin
			 * force the flag to be present (with operand 0)
			 * even when the user didn't explicitly specify
			 * it.  Stated differently, this flag has somewhat
			 * different semantics than the other binary-
			 * valued flags.
			 */
			if (*p == ENCODEFLAG) {
				/*
				 * Look for operand value; print description
				 * only if the operand value exists and is '1'.
				 */
				if (*++p) {
					int	i;

					NONBLANK(p);
					p = satoi(p, &i);
					if (*p == '\n' && (i & EF_UUENCODE))
						printf("\t%s\n",
						    flagdesc[ENCODEFLAG - 'a']);
				}
			} else if (*p - 'a' < 0 || *p - 'a' >= NFLAGS) {
				printf(gettext("\tUnknown flag '%c'\t"), *p);
				if (*++p) {
					NONBLANK(p);
					printf("\t%s", p);
				}
			} else {
				/*
				 * Standard flag: print description and
				 * operand value if present.
				 * The newline is included in the operand.
				 */
				printf("\t%s", flagdesc[*p - 'a']);

				if (*++p) {
					NONBLANK(p);
					printf("\t%s", p);
				}
			}
		}
		if (!flag)
			printf(gettext("\tnone\n"));
	}
	else
		(void) read_to(BUSERTXT);

	if (HADT) {
		text = 0;
		printf(gettext("\nDescription --\n"));
		while (((p = lineread(NOEOF)) != NULL) && *p != CTLCHAR) {
			text = 1;
			printf("\t%s", p);
		}
		if (!text)
			printf(gettext("\tnone\n"));
	}
	else
		(void) read_to(EUSERTXT);

	if (HADB) {
		printf("\n");
		while ((p = lineread(EOF)) != NULL)
			if (*p == CTLCHAR)
				printf("*** %s", ++p);
			else
				printf("\t%s", p);
	}

	fclose(iptr);
}


static void
getdel(delp, lp)
register struct delent *delp;
register char *lp;
{
	lp += 2;
	NONBLANK(lp);
	delp->type = *lp++;
	NONBLANK(lp);
	delp->osid = lp;
	BLANK(lp);
	*lp++ = '\0';
	NONBLANK(lp);
	delp->datetime = lp;
	BLANK(lp);
	NONBLANK(lp);
	BLANK(lp);
	*lp++ = '\0';
	NONBLANK(lp);
	delp->pgmr = lp;
	BLANK(lp);
	*lp++ = '\0';
	NONBLANK(lp);
	delp->serial = lp;
	BLANK(lp);
	*lp++ = '\0';
	NONBLANK(lp);
	delp->pred = lp;
	repl(lp, '\n', '\0');
}


static char *
read_to(ch)
register char ch;
{
	char *n;

	while (((n = lineread(NOEOF)) != NULL) &&
			!(*n++ == CTLCHAR && *n == ch))
		continue;

	return (n);
}


static char *
lineread(eof)
register int eof;
{
	char	buf[512];
	size_t	nread, used = 0;

	do {
		if (fgets(buf, sizeof (buf), iptr) == NULL) {
			if (!eof) {
				fatal(gettext("premature eof (co5)"));
			}
			return ((used) ? line : NULL);
		}

		nread = strlen(buf);

		if ((used + nread) >= line_size) {
			line_size += sizeof (buf);
			line = (char *) realloc(line, line_size);
			if (line == NULL) {
				efatal(gettext("OUT OF SPACE (ut9)"));
			}
		}

		strcpy(line + used, buf);
		used += nread;
	} while (line[used - 1] != '\n');

	linenum++;

	return (line);
}


static void
printdel(file, delp)
register char *file;
register struct delent *delp;
{
	printf("\n");

	if (prefix) {
		statistics[length(statistics) - 1] = '\0';
		printf("%s:\t", file);
	}

	printf("%c %s\t%s %s\t%s %s\t%s", delp->type, delp->osid,
		delp->datetime, delp->pgmr, delp->serial, delp->pred, statistics);
}


/*ARGSUSED*/
static void
printit(file, str, cp)
register char *file;
register char *str, *cp;
{
	cp++;
	NONBLANK(cp);

	if (prefix) {
		cp[length(cp) - 1] = '\0';
		printf(" ");
	}

	printf("%s%s", str, cp);
}
