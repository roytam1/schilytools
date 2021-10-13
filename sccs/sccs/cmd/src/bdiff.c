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
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
/*
 * This file contains modifications Copyright 2006-2015 J. Schilling
 *
 * @(#)bdiff.c	1.16 15/04/23 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)bdiff.c 1.16 15/04/23 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)bdiff.c	1.15	05/06/08 SMI"
#endif

#include <schily/mconfig.h>
#include <fatal.h>
#include <schily/signal.h>
#include <schily/types.h>
#include <schily/unistd.h>
#include <schily/fcntl.h>
#include <schily/stdio.h>
#include <schily/ctype.h>
#include <schily/string.h>
#include <schily/stdlib.h>
#include <schily/wait.h>
#include <schily/utypes.h>	/* For Llong */
#include <schily/schily.h>
#define	VMS_VFORK_OK
#include <schily/vfork.h>
#include <schily/io.h>		/* for setmode() prototype */
#ifndef	HAVE_PRINTF_LL
#undef	printf
#define	printf	js_printf	/* Use libschily::printf() for %lld */
#endif

#undef	offset_t
#define	offset_t	Llong

#define	ONSIG	16

/*
 *	This program segments two files into pieces of <= seglim lines
 *	(which is passed as a third argument or defaulted to some number)
 *	and then executes diff upon the pieces. The output of
 *	'diff' is then processed to make it look as if 'diff' had
 *	processed the files whole. The reason for all this is that seglim
 *	is a reasonable upper limit on the size of files that diff can
 *	process.
 *	NOTE -- by segmenting the files in this manner, it cannot be
 *	guaranteed that the 'diffing' of the segments will generate
 *	a minimal set of differences.
 *	This process is most definitely not equivalent to 'diffing'
 *	the files whole, assuming 'diff' could handle such large files.
 *
 *	'diff' is executed by a child process, generated by forking,
 *	and communicates with this program through pipes.
 */

static char Error[128];

static int seglim;	/* limit of size of file segment to be generated */

#if	defined(PROTOTYPES) && defined(INS_BASE)
static char diffp[]  =  INS_BASE "/ccs/bin/" "diff";
#endif
static char diff[]  =  "/usr/bin/diff";
static char tempskel[] = "/tmp/bdXXXXXX"; /* used to generate temp file names */
static char tempfile[32];
static char otmp[32], ntmp[32];
static int	fflags;
static int	fatal_num = 1;		/* exit number for fatal exit */
static offset_t	linenum;
static size_t obufsiz, nbufsiz, dbufsiz;
	int main __PR((int argc, char *argv[]));
static void saverest __PR((char **linep, size_t *bufsizp, FILE *iptr));
static void putsave __PR((char **linep, size_t *bufsizp, int type));
static char *readline __PR((char **, size_t *, FILE *));
static void addgen __PR((char **, size_t *, FILE *));
static void delgen __PR((char **, size_t *, FILE *));
static void clean_up __PR((void));
static void fixnum __PR((char *));
static void fatal __PR((char *));
static void setsig __PR((void));
static void setsig1 __PR((int));
static char *satoi __PR((char *, offset_t *));
static FILE *maket __PR((char *));

static char *prognam;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	FILE *poldfile, *pnewfile;
	char *oline, *nline, *diffline;
	char *olp, *nlp, *dp;
	int otcnt, ntcnt;
	pid_t i;
	int pfd[2];
	FILE *poldtemp, *pnewtemp, *pipeinp;
	int status;

	prognam = argv[0];
	/*
	 * Set flags for 'fatal' so that it will clean up,
	 * produce a message, and terminate.
	 */
	fflags = FTLMSG | FTLCLN | FTLEXIT;

	setsig();

	if (argc < 3 || argc > 5)
		fatal("arg count");

	if (strcmp(argv[1], "-") == 0 && strcmp(argv[2], "-") == 0)
		fatal("both files standard input");
	if (strcmp(argv[1], "-") == 0)
		poldfile = stdin;
	else
		if ((poldfile = fopen(argv[1], "rb")) == NULL) {
			(void) snprintf(Error, sizeof (Error),
				"Can not open '%s'", argv[1]);
			fatal(Error);
		}
	if (strcmp(argv[2], "-") == 0)
		pnewfile = stdin;
	else
		if ((pnewfile = fopen(argv[2], "rb")) == NULL) {
			(void) snprintf(Error, sizeof (Error),
				"Can not open '%s'", argv[2]);
			fatal(Error);
		}

	seglim = 3500;

	if (argc > 3) {
		if (argv[3][0] == '-' && argv[3][1] == 's')
			fflags &= ~FTLMSG;
		else {
			if ((seglim = atoi(argv[3])) == 0)
				fatal("non-numeric limit");
			if (argc == 5 && argv[4][0] == '-' &&
					argv[4][1] == 's')
				fflags &= ~FTLMSG;
		}
	}

	linenum = 0;

	/* Allocate the buffers and initialize their lengths */

#ifdef	HAVE_SETVBUF
	setvbuf(poldfile, NULL, _IOFBF, 32*1024);
	setvbuf(pnewfile, NULL, _IOFBF, 32*1024);
#endif
	obufsiz = BUFSIZ;
	nbufsiz = BUFSIZ;
	dbufsiz = BUFSIZ;

	if ((oline = (char *)malloc(obufsiz)) == NULL ||
	    (nline = (char *)malloc(nbufsiz)) == NULL ||
	    (diffline = (char *)malloc(dbufsiz)) == NULL)
		fatal("Out of memory");

	/*
	 * The following while-loop will prevent any lines
	 * common to the beginning of both files from being
	 * sent to 'diff'. Since the running time of 'diff' is
	 * non-linear, this will help improve performance.
	 * If, during this process, both files reach EOF, then
	 * the files are equal and the program will terminate.
	 * If either file reaches EOF before the other, the
	 * program will generate the appropriate 'diff' output
	 * itself, since this can be easily determined and will
	 * avoid executing 'diff' completely.
	 */
	for (;;) {
		olp = readline(&oline, &obufsiz, poldfile);
		nlp = readline(&nline, &nbufsiz, pnewfile);

		if (!olp && !nlp)	/* EOF found on both:  files equal */
			return (0);

		if (!olp) {
			/*
			 * The entire old file is a prefix of the
			 * new file. Generate the appropriate "append"
			 * 'diff'-like output, which is of the form:
			 * 		nan, n
			 * where 'n' represents a line-number.
			 */
			addgen(&nline, &nbufsiz, pnewfile);
		}

		if (!nlp) {
			/*
			 * The entire new file is a prefix of the
			 * old file. Generate the appropriate "delete"
			 * 'diff'-like output, which is of the form:
			 * 		n, ndn
			 * where 'n' represents a line-number.
			 */
			delgen(&oline, &obufsiz, poldfile);
		}

		if (strcmp(olp, nlp) == 0)
			linenum++;
		else
			break;
	}

	/*
	 * Here, first 'linenum' lines are equal.
	 * The following while-loop segments both files into
	 * seglim segments, forks and executes 'diff' on the
	 * segments, and processes the resulting output of
	 * 'diff', which is read from a pipe.
	 */
	for (;;) {
		/* If both files are at EOF, everything is done. */
		if (!olp && !nlp)	/* finished */
			return (0);

		if (!olp) {
			/*
			 * Generate appropriate "append"
			 * output without executing 'diff'.
			 */
			addgen(&nline, &nbufsiz, pnewfile);
		}

		if (!nlp) {
			/*
			 * Generate appropriate "delete"
			 * output without executing 'diff'.
			 */
			delgen(&oline, &obufsiz, poldfile);
		}

		/*
		 * Create a temporary file to hold a segment
		 * from the old file, and write it.
		 */
		poldtemp = maket(otmp);
		otcnt = 0;
		while (olp && otcnt < seglim) {
			(void) fputs(oline, poldtemp);
			if (ferror(poldtemp) != 0) {
				fflags |= FTLMSG;
				fatal("Can not write to temporary file");
			}
			olp = readline(&oline, &obufsiz, poldfile);
			otcnt++;
		}
		(void) fclose(poldtemp);

		/*
		 * Create a temporary file to hold a segment
		 * from the new file, and write it.
		 */
		pnewtemp = maket(ntmp);
		ntcnt = 0;
		while (nlp && ntcnt < seglim) {
			(void) fputs(nline, pnewtemp);
			if (ferror(pnewtemp) != 0) {
				fflags |= FTLMSG;
				fatal("Can not write to temporary file");
			}
			nlp = readline(&nline, &nbufsiz, pnewfile);
			ntcnt++;
		}
		(void) fclose(pnewtemp);

		/* Create pipes and fork.  */
		if ((pipe(pfd)) == -1)
			fatal("Can not create pipe");
		setmode(pfd[0], O_BINARY);
		setmode(pfd[1], O_BINARY);
		if ((i = vfork()) < (pid_t)0) {
			(void) close(pfd[0]);
			(void) close(pfd[1]);
			fatal("Can not fork, try again");
		} else if (i == (pid_t)0) {	/* child process */
#ifdef	set_child_standard_fds
			set_child_standard_fds(STDIN_FILENO,
						pfd[1],
						STDERR_FILENO);
#ifdef	F_SETFD
			fcntl(pfd[0], F_SETFD, 1);
#endif
#else
			(void) close(pfd[0]);
			(void) close(1);
			(void) dup(pfd[1]);
			(void) close(pfd[1]);
#endif

			/* Execute 'diff' on the segment files. */
#if	defined(PROTOTYPES) && defined(INS_BASE)
			(void) execlp(diffp, diffp, otmp, ntmp, (char *)0);
#endif
			(void) execlp(diff, diff, otmp, ntmp, (char *)0);

			/*
			 * Exit code here must be > 1.
			 * Parent process treats exit code of 1 from the child
			 * as non-error because the child process "diff" exits
			 * with a status of 1 when a difference is encountered.
			 * The error here is a true error--the parent process
			 * needs to detect it and exit with a non-zero status.
			 */
			(void) close(1);
			(void) snprintf(Error, sizeof (Error),
			    "Can not execute '%s'", diff);
			fatal_num = 2;
#ifdef	HAVE_VFORK
			fflags |= FTLVFORK;
#endif
			fatal(Error);
		} else {			/* parent process */
			(void) close(pfd[1]);
			pipeinp = fdopen(pfd[0], "rb");

			/* Process 'diff' output. */
			while ((dp = readline(&diffline, &dbufsiz, pipeinp))) {
				if (isdigit(* (unsigned char *)dp))
					fixnum(diffline);
				else
					(void) printf("%s", diffline);
			}

			(void) fclose(pipeinp);

			/* EOF on pipe. */
			(void) wait(&status);
			if (!WIFEXITED(status) || (WEXITSTATUS(status) & ~1)) {
				(void) snprintf(Error, sizeof (Error),
				    "'%s' failed", diff);
				fatal(Error);
			}
		}
		linenum += seglim;

		/* Remove temporary files. */
		(void) unlink(otmp);
		(void) unlink(ntmp);
	}
}

/* Routine to save remainder of a file. */
static void
saverest(linep, bufsizp, iptr)
	char **linep;
	size_t *bufsizp;
	FILE *iptr;
{
	char *lp;
	FILE *temptr;

	temptr = maket(tempfile);

	lp = *linep;

	while (lp) {
		(void) fputs(*linep, temptr);
		linenum++;
		lp = readline(linep, bufsizp, iptr);
	}
	(void) fclose(temptr);
}

/* Routine to write out data saved by 'saverest' and to remove the file. */
static void
putsave(linep, bufsizp, type)
	char **linep;
	size_t *bufsizp;
	char type;
{
	FILE *temptr;
	size_t	len;

	if ((temptr = fopen(tempfile, "rb")) == NULL) {
		(void) snprintf(Error, sizeof (Error),
		    "Can not open tempfile ('%s')", tempfile); fatal(Error);
	}

	while (readline(linep, bufsizp, temptr))
		(void) printf("%c %s", type, *linep);

	/*
	 * JS: make bdiff compatible to diff as claimed in the man page.
	 */
	len = strlen(*linep);
	if (len == 0 || (*linep)[len-1] != '\n')
		(void) printf("\n");

	(void) fclose(temptr);

	(void) unlink(tempfile);
}

static void
fixnum(lp)
	char *lp;
{
	offset_t num;

	while (*lp) {
		switch (*lp) {

		case 'a':
		case 'c':
		case 'd':
		case ',':
		case '\n':
			(void) printf("%c", *lp);
			lp++;
			break;

		default:
			lp = satoi(lp, &num);
			num += linenum;
			(void) printf("%lld", num);
		}
	}
}

static void
addgen(lpp, bufsizp, fp)
	char **lpp;
	size_t *bufsizp;
	FILE *fp;
{
	offset_t oldline;
	(void) printf("%llda%lld", linenum, linenum+1);

	/* Save lines of new file. */
	oldline = linenum + 1;
	saverest(lpp, bufsizp, fp);

	if (oldline < linenum)
		(void) printf(",%lld\n", linenum);
	else
		(void) printf("\n");

	/* Output saved lines, as 'diff' would. */
	putsave(lpp, bufsizp, '>');

	exit(0);
}

static void
delgen(lpp, bufsizp, fp)
	char **lpp;
	size_t *bufsizp;
	FILE *fp;
{
	offset_t savenum;

	(void) printf("%lld", linenum+1);
	savenum = linenum;

	/* Save lines of old file. */
	saverest(lpp, bufsizp, fp);

	if (savenum +1 != linenum)
		(void) printf(",%lldd%lld\n", linenum, savenum);
	else
		(void) printf("d%lld\n", savenum);

	/* Output saved lines, as 'diff' would.  */
	putsave(lpp, bufsizp, '<');

	exit(0);
}

static void
clean_up()
{
	(void) unlink(tempfile);
	(void) unlink(otmp);
	(void) unlink(ntmp);
}

static FILE *
maket(file)
	char *file;
{
	FILE *iop = NULL;
	int fd;

	(void) strcpy(file, tempskel);
	if ((fd = mkstemp(file)) == -1 ||
		(iop = fdopen(fd, "wb+")) == NULL) {
		(void) snprintf(Error, sizeof (Error),
		    "Can not open/create temp file ('%s')", file);
		fatal(Error);
	}
	setmode(fileno(iop), O_BINARY);
	return (iop);
}

static void
fatal(msg)
	char *msg;
/*
 *	General purpose error handler.
 *
 *	The argument to fatal is a pointer to an error message string.
 *	The action of this routine is driven completely from
 *	the "fflags" global word (see <fatal.h>).
 *
 *	The FTLMSG bit controls the writing of the error
 *	message on file descriptor 2.  A newline is written
 *	after the user supplied message.
 *
 *	If the FTLCLN bit is on, clean_up is called.
 */
{
	if (fflags & FTLMSG) {
		if (fflags & FTLVFORK) {
			write(STDERR_FILENO, prognam, strlen(prognam));
			write(STDERR_FILENO, ": ", 2);
			write(STDERR_FILENO, msg, strlen(msg));
		}
		(void) fprintf(stderr, "%s: %s\n", prognam, msg);
	}
	if (fflags & FTLCLN)
		clean_up();
	if (fflags & FTLEXIT) {
		if (fflags & FTLVFORK)
			_exit(fatal_num);
		exit(fatal_num);
	}
}

static void
setsig()
/*
 *	General-purpose signal setting routine.
 *	All non-ignored, non-caught signals are caught.
 *	If a signal other than hangup, interrupt, or quit is caught,
 *	a "user-oriented" message is printed on file descriptor 2.
 *	If hangup, interrupt or quit is caught, that signal
 *	is set to ignore.
 *	Termination is like that of "fatal",
 *	via "clean_up()"
 */
{
	void (*act) __PR((int));
	int j;

	for (j = 1; j < ONSIG; j++) {
		act = signal(j, setsig1);
		if (act == SIG_ERR)
			continue;
		if (act == SIG_DFL)
			continue;
		(void) signal(j, act);
	}
}

static void
setsig1(sig)
	int sig;
{

	(void) signal(sig, SIG_IGN);
	clean_up();
	exit(1);
}

static char *
satoi(p, ip)
	char *p;
	offset_t *ip;
{
	offset_t sum;

	sum = 0;
	while (isdigit(* (unsigned char *)p))
		sum = sum * 10 + (*p++ - '0');
	*ip = sum;
	return (p);
}

/*
 * Read a line of data from a file.  If the current buffer is not large enough
 * to contain the line, double the size of the buffer and continue reading.
 * Loop until either the entire line is read or until there is no more space
 * to be malloc'd.
 */

static char *
readline(bufferp, bufsizp, filep)
	char **bufferp;
	size_t *bufsizp;
	FILE *filep;
{
	char *bufp;
	size_t newsize;		/* number of bytes to make buffer */
	size_t oldsize;

	(*bufferp)[*bufsizp - 1] = '\t'; /* arbitrary non-zero character */
	(*bufferp)[*bufsizp - 2] = ' ';	/* arbitrary non-newline char */
	bufp = fgets(*bufferp, *bufsizp, filep);
	if (bufp == NULL)
		return (bufp);
	while ((*bufferp)[*bufsizp -1] == '\0' &&
	    (*bufferp)[*bufsizp - 2] != '\n' &&
	    strlen(*bufferp) == *bufsizp - 1) {
		newsize = 2 * (*bufsizp);
		bufp = (char *)realloc((void *)*bufferp, newsize);
		if (bufp == NULL)
			fatal("Out of memory");
		oldsize = *bufsizp;
		*bufsizp = newsize;
		*bufferp = bufp;
		(*bufferp)[*bufsizp - 1] = '\t';
		(*bufferp)[*bufsizp - 2] = ' ';
		bufp = fgets(*bufferp + oldsize -1, oldsize + 1, filep);
		if (bufp == NULL) {
			if (feof(filep)) {
				bufp = *bufferp;
				break;
			} else
				fatal("Read error");
		} else
			bufp = *bufferp;
	}
	return (bufp);
}
