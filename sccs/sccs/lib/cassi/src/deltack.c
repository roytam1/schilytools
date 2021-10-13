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
 * Copyright 2006-2018 J. Schilling
 *
 * @(#)deltack.c	1.17 18/04/29 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)deltack.c 1.17 18/04/29 J. Schilling"
#endif
/*
 * @(#)deltack.c 1.8 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)deltack.c"
#pragma ident	"@(#)sccs:lib/cassi/deltack.c"
#endif
#include <defines.h>
#include <had.h>
#include <filehand.h>
#include <i18n.h>
#include <ccstypes.h>

#define	FOREVER		1
#define	MAXLIST		15
#define	MAXLIST2	20
#define	MAXLENCMR	12
static char errorlog[FILESIZE];		 /* log cmts errors here */
static FILE *efd;

	int	deltack __PR((char *pfile, char *mrs, char *nsid, char *apl, char *sflags[NFLAGS]));
static int	promdelt __PR((char *cmrs, char *statp, char *type, char *fred, char *sflags[NFLAGS]));
static int	msg __PR((char *syst, char *name, char *cmrs, char *stats, char *types, char *sids, char *fred, char *sflags[NFLAGS]));
static int	verif __PR((char *cmr, char *fred));
static int	getinfo __PR((char **freddy, char *sys));
static char *	xgets	__PR((char *buf, size_t len));

/*
 * deltack(pfile, mrs, nsid, apl) peforms the nessesary validations on a delta
 * involving the CMF FRED file of active CMR's
 */

int
deltack(pfile, mrs, nsid, apl, sflags)
	char pfile[];	/* the pfile name */

	char *mrs;		/* list of mrs from pfile */
	char *nsid;		/* sid id */
	char *apl;		/* application from the file */
	char *sflags[NFLAGS];
{
	static char type[10], sthold[10];
	char hold[302], *h, *fred;

	/* check for the existance of the p.file */
	if (!pfile) {
		cmrerror(gettext("Pfile non existant at deltack "));
		return (0);
	}
	/* if no application serious error */
	if (!apl) {
		cmrerror(gettext("no application found with -fz flag"));
		return (0);
	}
	/* check for and retrieve FRED given the application name */
	if (!getinfo(&fred, apl)) {
		cmrerror(NOGETTEXT("no FRED file or system name in admin directory "));
		return (0);
	}
	strlcpy(errorlog, fred, sizeof (errorlog));
	*(errorlog + strlen(errorlog) - 11) = '\0';
	strlcat(errorlog, NOGETTEXT("LOG"), sizeof (errorlog));
	if (!(mrs)) {
		if ((efd = fopen(errorlog, "ab")) != NULL) {
			fprintf(efd, gettext("***CASSI REPORTS ERROR: no CMRS in pfile: %s\n"), pfile);
			(void) fclose(efd);
			efd = NULL;
		}
		cmrerror(gettext("CMRs not on P.file -serious inconsistancy"));
		return (0);
	}
	if (!promdelt(mrs, sthold, type, fred, sflags)) {
		return (0);
	}
	/* now build the chpost line  */
	(void) strlcpy(hold, mrs, sizeof (hold));
	h = strtok(hold, ",\0");
	(void) msg(apl, pfile, h, sthold, type, nsid, fred, sflags);
	while ((h = strtok(0, ",\0 ")) != NULL) {
		(void) msg(apl, pfile, h, sthold, type, nsid, fred, sflags);
	}
	return (1);
}



/*
 * promdelt(cmrs, statp, type, fred) allows one to modify the cmrs list and
 * enrter the type and status for the input to the MR system via the net
 */
static int
promdelt(cmrs, statp, type, fred, sflags)
	char *cmrs, *statp, *type, *fred;
	char	*sflags[NFLAGS];
{
	static char hold[300], nold[300], *cmrlist[MAXLIST2 + 1];
	char answ[100];
	int i, j, numcmrs, fdflag = 0, eqflag = 0, badflag = 0;
	char *h;

	/* place the cmrs list in the array of pointers and remove the commas */
	strlcpy(hold, cmrs, sizeof (hold));
	(void) strtok(hold, ",");
	cmrlist[0] = hold;
	for (i = 1; i < MAXLIST; i++) {
		if ((cmrlist[i] = strtok(0, ",\0")) == (char *)NULL) {
			break;
		}
	}
	numcmrs = i;
	/* remove invalid cmrs from the list if now none set flag */
	for (i = 0; cmrlist[i]; i++) {
		if (!verif(cmrlist[i], fred)) {
			/* a 'sd' cmr has been found */
			if (numcmrs > 1) {
				for (j = i; j < numcmrs; j++) {
					cmrlist[j] = cmrlist[j + 1];
				}
				numcmrs--;
			} else { /* there is a last cmr to delete set flag */
				cmrlist[0] = NULL;
				badflag = 1;
				numcmrs--;
			}
		}
	}
	if (HADZ) { /* force delta flag on */
		if (badflag) { /* no legal cmrs in list */
		    if ((efd = fopen(errorlog, "ab")) != NULL) {
			fprintf(efd, gettext("***CASSI REPORTS ERROR: no CMRS at sd\n"));
			(void) fclose(efd);
			efd = NULL;
		    }
			(void) fatal(gettext("no CMR's left, delta forbidden\n"));
		}
		(void) strcpy(statp, "sd");
		if (sflags[TYPEFLAG - 'a'])
			strcpy(type, sflags[TYPEFLAG - 'a']);
		else
			strcpy(type, NOGETTEXT("sw"));
			/* rebuild cmr comma separated list */
		cat(nold, cmrlist[0], (char *)0);
		for (i = 1; i < numcmrs; i++) {
			cat(nold, ",", cmrlist[i], (char *)0);
		}
		strcpy(cmrs, nold);
		return (1);
	}
	/* CONSTCOND */
	while (FOREVER) {
		if (!badflag) {
			printf(gettext("the CMRs for this delta now are:\n"));
			for (i = 0; cmrlist[i + 1]; i++) {
				printf(" %s, ", cmrlist[i]);
			}
			printf("%s\n", cmrlist[i]);
			printf(NOGETTEXT("OK ??"));
			(void) xgets(answ, sizeof (answ));
			if((strcmp(answ,NOGETTEXT("y")) == 0) || (strcmp(answ,NOGETTEXT("ye")) == 0) || (strcmp(answ,NOGETTEXT("yes")) == 0)) {

				break;
			}
		} else {
			printf(gettext("you must input at least 1 valid cmr number \n"));
		}
		/* now prompt for new cmrs to add to the list */
		/* CONSTCOND */
		while (FOREVER) {
			eqflag = 0;
			printf(gettext("enter new CMR number or 'CR' "));
			(void) xgets(answ, sizeof (answ));
			if (answ[0] == '\0') {
				if (!badflag) {
					break;
				} else {
					continue;
				}
			}
			h = (char *) malloc((unsigned)(strlen(answ) + 6));
			strcpy(h, answ);
			/* check for duplicate */
			for (i = 0; i < numcmrs; i++) {
				if (strcmp(h, cmrlist[i]) == 0) {
					eqflag = 1;
					break;
				}
			}
			if (eqflag == 1) {
				printf(gettext(" \n duplicate CMR number ignored\n"));
				continue;
			}
			/* now verify that the cmr is in FRED */
			if (!verif(h, fred)) {
				printf(gettext(" \n invalid CMR ignored \n"));
				continue;
			}
			/* the addition is valid */
			cmrlist[numcmrs] = h;
			badflag = 0; /* turn off the no cmrs found indicator */
			if (++numcmrs > MAXLIST2) {
				printf(gettext(" \n too many CMRs added no more allowed \n"));
				break;
			}
		}
		/* now delete mrs from list */
		/* CONSTCOND */
		while (FOREVER) {
			fdflag = 0;
			printf(gettext(" \n CMR number to delete or (CR) ? "));
			(void) xgets(answ, sizeof (answ));
			if (!(*answ)) {
				break;
			}
			/* if one left break */
			if (numcmrs == 1) {
				printf(gettext("\n only one CMR left can't delete more\n"));
				break;
			}
			/* check if request is on list */
			for (i = 0; i < numcmrs; i++) {
				if (strcmp(answ, cmrlist[i]) == 0) {
					fdflag = 1;
					for (j = i; j < numcmrs; j++) {
						cmrlist[j] = cmrlist[j+1];
					}
					break;
				}
			}
			if (fdflag == 0) {
				printf(gettext("\n not on list request ignored\n"));
				continue;
			} else {
				numcmrs--;
				/* we have oneless cmr */
			}
		}
	}
	/* here ends the cmr loop */
	/* set type to proper value */
	if (sflags[TYPEFLAG - 'a'])
		strcpy(type, sflags[TYPEFLAG - 'a']);
	else
		strcpy(type, NOGETTEXT("sw"));
	/* set status */
	strcpy(statp, "sd");
	/* reformat the cmrlist into a comma separated cmr list */
	cat(nold, cmrlist[0], (char *)0);
	for (i = 1; i < numcmrs; i++) {
		cat(nold, nold, ",", cmrlist[i], (char *)0);
	}
	strcpy(cmrs, nold);
	return (1);
}



/*
 * msg(syst, cmrs, stats, types, sids) formats a message and calls cmrpost
 */
static int
msg(syst, name, cmrs, stats, types, sids, fred, sflags)
	char *syst, *name, *cmrs, *stats, *types, *sids, *fred;
	char	*sflags[NFLAGS];
{
	FILE *fd;
	char *k;
	char pname[FILESIZE], *ptr, holdfred[100], dir[100], path[FILESIZE];
	struct stat stbuf;
	int noexist = 0;

	/* if -fm flag contains a value substitute a the value for name */
	if ((k = sflags[MODFLAG - 'a']) != NULL) {
		name = k;
	}
	if (*name != '/') { /* not full path name */
		if (getcwd(path, sizeof (path)) == NULL)
			(void) fatal(gettext("getcwd() failed (ge20)"));
		cat(pname, path, "/", name, (char *)0);
	} else {
		strlcpy(pname, name, sizeof (pname));
	}
	(void) fixpath(pname);				/* get rid of . and .. */
/******** the net is replaced by psudonet ******
*	  sprintf(holdit, "netq %s chpost  %s q %s %s MID=%s MFS=%s q q", syst, cmrs, pname, types, sids, stats);
*	 system(holdit);
************************************************/
	/* build the name of the  termLOG file */
	strlcpy(holdfred, fred, sizeof (holdfred));
	ptr = strchr(holdfred, '.');
	*ptr = '\0';
	strlcat(holdfred, NOGETTEXT("source"), sizeof (holdfred));
	strlcpy(dir, holdfred, sizeof (dir));
	strlcat(holdfred, NOGETTEXT("/termLOG"), sizeof (holdfred));
	if (stat(holdfred, &stbuf) == -1)
		noexist = 1; /* new termLOG */
	if (!(fd = fopen(holdfred, "ab"))) {
		if ((efd = fopen(errorlog, "ab")) != NULL) {
			fprintf(efd, gettext("***CASSI REPORTS ERROR: can't write to FRED : %s\n"), pname);
			(void) fclose(efd);
			efd = NULL;
		}
		(void) fatal(gettext(" Cassi Interface Msg not writable\n"));
		return (0);
	}
#if 0
	fprintf(fd, "%s chpost %s q %s %s MID=%s MFS=%s MPA=%s q q\n", syst, cmrs, pname, types, sids, stats, logname());
#else
	fprintf(fd, "%s chpost %s q %s %s MID=%s MFS=%s MPA=%s q q\n", syst, cmrs, pname, types, sids, stats, saveid);
#endif
	(void) fclose(fd);
	fd = NULL;
	if (noexist) { /* new termLOG make owner of /BD/source owner of file */
	    if (stat(dir, &stbuf) == -1) {
		if ((efd = fopen(errorlog, "ab")) != NULL) {
			fprintf(efd, gettext("***CASSI REPORTS ERROR: can't write to BD/source : %s\n"), pname);
			(void) fclose(efd);
			efd = NULL;
		}
		(void) fatal(gettext("Cassi BD/source not writeable\n"));
	    }
	    (void) chmod(holdfred, (mode_t)0666);
	    (void) chown(holdfred, stbuf.st_uid, stbuf.st_gid);
	}
	return (1);
}

/*
 * verif(cmr, fred) calls the verification prog and returns 0 if failed
 */
static int
verif(cmr, fred)
	char *cmr, *fred;
	{
	int res;
	char *cmrpass[2];

	/*
	 * if length of cmr number not = MAXLENCMR error
	 *    all cmr numbers are 12 characters long
	 */
	if (strlen(cmr) != MAXLENCMR)
		{
			return (0);
		}
	cmrpass[0] = cmr;
	cmrpass[1] = NULL;
	res = sweep(SEQVERIFY, fred, NULL, '\n', WHITE, 40, cmrpass, NULL, NULL,
		(int(*) __PR((char *, int, char **))) NULL, (int (*) __PR((char **, char **, int))) NULL);
	if (res != FOUND) {
		return (0);
	} else {
		return (1);
	}
}

/*
 * getinfo(freddy, sys)
 *    get the name of the fred file and the system name
 */
static int
getinfo(freddy, sys)
	char **freddy, *sys;
{
	struct stat buf;

	*freddy = gf(sys);
	if (!(**freddy)) {
		printf(gettext("got to bad FRED file %s\n"), *freddy);
		return (0);
	}
	if (stat(*freddy, &buf)) {
		return (0);
	}
	return (1);
}

static char *
xgets(buf, len)
	char	*buf;
	size_t	len;
{
	size_t	l;

	if (fgets(buf, len, stdin) == NULL)
		return (NULL);
	l = strlen(buf);
	if (l > 0 && buf[l-1] == '\n')
		buf[l-1] = '\0';
	return (buf);
}
