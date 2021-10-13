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
/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */
/*
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2006-2020 J. Schilling
 *
 * @(#)what.c	1.21 20/08/23 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)what.c 1.21 20/08/23 J. Schilling"
#endif
/*
 * @(#)what.c 1.11 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)what.c"
#pragma ident	"@(#)sccs:cmd/what.c"
#endif
#define		SCCS_MAIN			/* define global vars */
#include	<defines.h>
#include	<version.h>
#include	<i18n.h>
#include	<schily/sysexits.h>

#define	MINUS	'-'
#define	MINUS_S	"-s"
#define	TRUE	1
#define	FALSE	0


static int found = FALSE;
static int silent = FALSE;

static char	pattern[]  =  NOGETTEXT("@(#)");

	int	main	__PR((int argc, char **argv));
static void	dowhat	__PR((FILE *iop));
static int	trypat	__PR((FILE *iop, char *pat));


int
main(argc, argv)
	int		argc;
	register char	**argv;
{
	register int i;
	register FILE *iop;
	int current_optind, c;

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

	if (argc < 2)
		dowhat(stdin);
	else {

		current_optind = 1;
		optind = 1;
		opterr = 0;
		i = 1;
		/*CONSTCOND*/
		while (1) {
			if (current_optind < optind) {
			    current_optind = optind;
			    argv[i] = 0;
			    if (optind > i+1) {
				argv[i+1] = NULL;
			    }
			}
			i = current_optind;
			c = getopt(argc, argv, "()-sV(version)");

				/*
				 * This takes care of options given after
				 * file names.
				 */
			if (c == EOF) {
			    if (optind < argc) {
				/* if it's due to -- then break; */
				if (argv[i][0] == '-' &&
				    argv[i][1] == '-') {
					argv[i] = 0;
					break;
				}
				optind++;
				current_optind = optind;
				continue;
			    } else {
				break;
			    }
			}
			switch (c) {
			case 's' :
				silent = TRUE;
				break;

			case 'V':		/* version */
				printf(gettext(
				"what %s-SCCS version %s %s (%s-%s-%s)\n"),
					PROVIDER,
					VERSION,
					VDATE,
					HOST_CPU, HOST_VENDOR, HOST_OS);
				exit(EX_OK);

			default  :
				fatal(gettext(
					"Usage: what [ -s ] filename..."));
			}
		}
		for (i = 1; (i < argc); i++) {
			if (!argv[i]) {
				continue;
			}
			if ((iop = fopen(argv[i], NOGETTEXT("rb"))) == NULL)
				fprintf(stderr, gettext("can't open %s (26)\n"),
					argv[i]);
			else {
				printf("%s:\n", argv[i]);
				dowhat(iop);
			}
		}
	}
	return (!found);				/* shell return code */
}


static void
dowhat(iop)
register FILE *iop;
{
	register int c;

	while ((c = getc(iop)) != EOF) {
		if (c == pattern[0])
			if (trypat(iop, &pattern[1]) && silent) break;
	}
	fclose(iop);
}


static int
trypat(iop, pat)
register FILE *iop;
register char *pat;
{
	register int c = EOF;

	for (; *pat; pat++)
		if ((c = getc(iop)) != *pat)
			break;
	if (!*pat) {
		found = TRUE;
		putchar('\t');
		while ((c = getc(iop)) != EOF && c &&
		    !any(c, NOGETTEXT("\"\\>\n")))
			putchar(c);
		putchar('\n');
		if (silent)
			return (TRUE);
	} else if (c != EOF)
		ungetc(c, iop);
	return (FALSE);
}
