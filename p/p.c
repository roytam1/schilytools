/* @(#)p.c	1.75 21/08/20 Copyright 1985-2021 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)p.c	1.75 21/08/20 Copyright 1985-2021 J. Schilling";
#endif
/*
 *	Print some files on screen
 *
 *	Copyright (c) 1985-2021 J. Schilling
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

#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/types.h>
#include <schily/utypes.h>
#include <schily/fcntl.h>
#include <schily/string.h>
#include <schily/signal.h>
#include <schily/sigset.h>
#include <schily/termcap.h>
#include <schily/libport.h>
#include <schily/errno.h>
#define	ungetch	dos_ungetch	/* Avoid DOS/curses ungetch() type clash */
#include <schily/termios.h>
#undef	ungetch			/* Restore our old value */
#include <schily/nlsdefs.h>
#include <schily/limits.h>	/* for  MB_LEN_MAX	*/
#include <schily/ctype.h>	/* For isprint()	*/
#include <schily/wchar.h>	/* wchar_t		*/
#include <schily/wctype.h>	/* For iswprint()	*/
#include <schily/patmatch.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>

#define	SEARCHSIZE	256
#define	DEF_PSIZE	24;
#define	DEF_LWIDTH	80;

#if	MB_LEN_MAX > 1
/*
 * We are on a platform that supports multi byte characters.
 */
#define	nextc()		nextwc()	/* Read next char */
#define	peekc()		peekwc()	/* Peek next char */
#define	getnextch()	getnextwch()	/* Consume last peeked char */
#define	ungetch(c)	ungetwch(c)	/* Unread char back to buffer */
#else
#define	nextc()	(--len >= 0 ? (int) *bp++ : \
			(fill_buf() <= 0 ? (len == 0 ? EOF : -2) : \
			(len--, (int) *bp++)))
#define	peekc()	(len > 0 ? (int) *bp : \
			(fill_buf() <= 0 ? (len == 0 ? EOF : -2) : \
			((int) *bp)))
#define	getnextch()	(len--, bp++)
#define	__peekc()	(ungetch(nextc()))
#endif

#	ifdef	USE_V7_TTY
struct sgttyb old;
struct sgttyb new;

#	else	/* USE_V7_TTY */

#ifdef	USE_TERMIOS
struct termios	old;
struct termios	new;
#endif
#	endif	/* USE_V7_TTY */
int	tty = -1;

FILE	*f;		/* the file being printed */
off_t	ofpos;		/* saved filepos for searching */
int	lineno;		/* current lineno (used for editing) */
int	colcnt;		/* column on screen we are going to print */
int	linecnt;	/* # of lines actually printed on screen */
int	lwidth;		/* length of a line on screen */
int	lines;		/* # of lines until we print a more prompt */
int	psize;		/* # of lines on a page */
int	supressblank;	/* supress multiple blank lines on output */
int	clear;		/* clear screen before displaying page */
int	dosmode;	/* wether to supress ^M^J sequences */
BOOL	autodos = TRUE;	/* whether to automatically detect DOS mode */
int	endline;	/* print a $ at each end of line */
int	raw;		/* do not expand control characters */
BOOL	raw8;		/* Ausgabe ohne ~ */
int	silent;		/* in silent mode there is no more prompt */
int	tab;		/* do nut expand tabs to spaces but to ^I */
int	visible;	/* _^H sequences are visible */
int	underline;	/* do underlining */
int	ununderline;	/* remove underlining */
int	help;		/* print online help */
int	prvers;		/* print version information */
BOOL	debug;		/* misc debug flag */
BOOL	nobeep;		/* be silent on errors */
char	*filename;	/* Filename fuer Ausgabezwecke */
int	direction;	/* direction to step through file list */
#define	FORWARD	0
char	*editor;
char	*shell;
int	searchnext;
#if	MB_LEN_MAX > 1
wchar_t	searchbuf[SEARCHSIZE];
#else
char	searchbuf[SEARCHSIZE];
#endif
int 	alt;
int 	*aux;
int 	*state;

char	*so;		/* start standout */
char	*se;		/* end standout */
char	*xso;		/* start abstract bold/standout */
char	*xse;		/* end abstract bold/standout */
char	*us;		/* start underline */
char	*ue;		/* end underline */
char	*md;		/* start bold */
char	*me;		/* end attributes */
char	*ce;		/* clear endline */
char	*cl;		/* clear screen */
int	li;		/* lines on screen */
int	co;		/* columns on screen */
BOOL	am = TRUE;	/* automatic margins */
BOOL	xn;		/* newline ignored > 80 */
BOOL	has_standout;
BOOL	has_bold;
BOOL	has_ul;
BOOL	has_termcap;
int	standout;
int	underl;

/*
 * As an incomplete multi byte character after a buffer refill
 * is partially before the begin of the buffer, we need to be
 * able to store a second character before that. This happens
 * when peekc() triggered a buffer refill and then ungetch()
 * is called.
 */
#define	P_BUFSIZE	((8*1024)+(2*MB_LEN_MAX))
unsigned char mybuf[P_BUFSIZE];	/* Fuer nextc */
unsigned char *bp;		/* ditto */
int len = 0;			/* ditto */
int clen = 0;			/* # of octects in nextwc() multi byte char */
int pclen = 0;			/* # of octects in peekwc() multi byte char */
unsigned char *cp;		/* Beginning of multi byte char in buffer   */
unsigned char *pcp;		/* Beginning of peeked multi byte char in buf */

#ifdef	BUFSIZ
char	buffer[BUFSIZ];		/* our buffer for stdout */
#endif
char	dcl[] = ":::::::::::::::";
/* BEGIN CSTYLED */
char	options[] =
"help,version,debug,nobeep,length#,l#,width#,w#,blank,b,clear,c,dos,nodos%0,end,e,raw,r,raw8,silent,s,tab,t,unul,visible,v";
/* END CSTYLED */

BOOL nameprint = FALSE;

extern	unsigned char	csize[];
extern	unsigned char	*ctab[];

extern	void	init_charset	__PR((void));

LOCAL	void	tstp		__PR((int sig));
LOCAL	void	usage		__PR((int exitcode));
EXPORT	int	main		__PR((int ac, char **av));
LOCAL	void	page		__PR((void));
LOCAL	int	outchar		__PR((int c));
LOCAL	void	moreprompt	__PR((void));
LOCAL	BOOL	more		__PR((void));
LOCAL	int	get_action	__PR((void));
LOCAL	void	onlinehelp	__PR((void));
LOCAL	void	redraw		__PR((void));
LOCAL	int	inchar		__PR((void));
#if	MB_LEN_MAX > 1
LOCAL	int	inwchar		__PR((void));
#else
#define	inwchar	inchar
#endif
#ifndef	nextc
LOCAL	int	nextc		__PR((void));
#endif
#if	MB_LEN_MAX > 1
LOCAL	int	nextwc		__PR((void));
LOCAL	int	peekwc		__PR((void));
#endif
#ifndef	ungetch
LOCAL	int	ungetch		__PR((int c));
#endif
#if	MB_LEN_MAX > 1
LOCAL	int	ungetwch	__PR((int c));
LOCAL	void	getnextwch	__PR((void));
#endif
LOCAL	int	fill_buf	__PR((void));
LOCAL	BOOL	read_pattern	__PR((void));
LOCAL	int	do_search	__PR((void));
LOCAL	int	unul		__PR((Uchar *ob, Uchar *ib, int amt));
LOCAL	void	init_termcap	__PR((void));
LOCAL	int	oc		__PR((int c));
LOCAL	void	start_standout	__PR((void));
LOCAL	void	end_standout	__PR((void));
#ifdef	__needed__
LOCAL	void	start_bold	__PR((void));
#endif
LOCAL	void	end_attr	__PR((void));
LOCAL	void	start_xstandout	__PR((void));
LOCAL	void	end_xstandout	__PR((void));
LOCAL	void	start_ul	__PR((void));
LOCAL	void	end_ul		__PR((void));
LOCAL	void	clearline	__PR((void));
LOCAL	void	clearscreen	__PR((void));
LOCAL	void	init_tty_size	__PR((void));
LOCAL	int	get_modes	__PR((void));
LOCAL	void	set_modes	__PR((void));
LOCAL	void	reset_modes	__PR((void));
LOCAL	void	fixtty		__PR((int sig));

#ifdef	SIGTSTP
LOCAL void
tstp(sig)
	int	sig;
{
	/* ignore SIGTTOU so we don't get stopped if the shell modifies pgrp */
	signal(SIGTTOU, SIG_IGN);
	end_standout();
	end_attr();
	end_ul();
	clearline();
	reset_modes();
	signal(SIGTTOU, SIG_DFL);

	signal(SIGTSTP, SIG_DFL);
#ifdef	OLD
#ifdef	HAVE_SIGRELSE
	sigrelse(SIGTSTP);
#else
	(void) sigsetmask(0);
#endif
#else	/* NEW */
	unblock_sig(SIGTSTP);
#endif
	kill(getpid(), SIGTSTP);

	/* Hier stoppt 'p' */

	set_modes();
	moreprompt();
}
#endif


LOCAL void
usage(exitcode)
	int	exitcode;
{
	error("Usage:	p [options] [file1...filen]\n");
	error("Options:\n");
	error("\t-help\t\tprint this online help\n");
	error("\t-version\tprint version number\n");
	error("\t-debug\t\tprint additional debug output\n");
	error("\tlength=#,l=#\tlength of screen (default 24)\n");
	error("\twidth=#,w=#\twidth  of screen (default 80)\n");
	error("\t-blank,-b\tsupress multiple blank lines on output\n");
	error("\t-clear,-c\tclear screen before displaying new page\n");
	error("\t-dos\t\tsupress '\\r' in '\\r\\n'\n");
	error("\t-nodos\t\tsupress auto detecting the dos mode\n");
	error("\t-end,-e\t\tprint a $ at each end of line\n");
	error("\t-raw,-r\t\tdo not expand chars\n");
	error("\t-raw8\t\tdo not expand 8bit chars\n");
	error("\t-silent,-s\tdo not prompt for more stuff\n");
	error("\t-tab,-t\t\tdo not expand tabs to spaces but to ^I\n");
#ifdef	__needed__
	error("\t-unul\t\tremove underlining and bold sequences\n");
#endif
	error("\t-visible,-v\tunderlining/bold sequences become visible\n\n");
	error("	When asked for more:  confirm=page, n=no more, h=half page\n");
	error("		q=quarter page, l=single line, 1-9=# lines.\n");
	error("	When asked for next file:  confirm=yes, n=skip to next\n");
	error("		s=stop (exit), h,q,l,1-9=yes with that # lines.\n");
	exit(exitcode);
}


EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	int	i;
	int	cac;
	char	*const *cav;
	int	fac;
	char	**fav;

	save_args(ac, av);

	(void) setlocale(LC_ALL, "");

#ifdef  USE_NLS
#if !defined(TEXT_DOMAIN)	/* Should be defined by cc -D */
#define	TEXT_DOMAIN "p"		/* Use this only if it weren't */
#endif
	{ char	*dir;
	dir = searchfileinpath("share/locale", F_OK,
					SIP_ANY_FILE|SIP_NO_PATH, NULL);
	if (dir)
		(void) bindtextdomain(TEXT_DOMAIN, dir);
	else
#if defined(PROTOTYPES) && defined(INS_BASE)
	(void) bindtextdomain(TEXT_DOMAIN, INS_BASE "/share/locale");
#else
	(void) bindtextdomain(TEXT_DOMAIN, "/usr/share/locale");
#endif
	(void) textdomain(TEXT_DOMAIN);
	}
#endif 	/* USE_NLS */


	cac = --ac;
	cav = ++av;

	if (getallargs(&cac, &cav, options, &help, &prvers, &debug, &nobeep,
			&psize, &psize,
			&lwidth, &lwidth,
			&supressblank, &supressblank,
			&clear, &clear,
			&dosmode, &autodos,
			&endline, &endline,
			&raw, &raw,
			&raw8,
			&silent, &silent,
			&tab, &tab,
			&ununderline,
			&visible, &visible) < 0) {
		errmsgno(EX_BAD, "Bad flag: %s.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help) usage(0);
	if (prvers) {
		/* BEGIN CSTYLED */
		gtprintf("p %s %s (%s-%s-%s)\n\n", "2.4", "2021/08/20", HOST_CPU, HOST_VENDOR, HOST_OS);
		gtprintf("Copyright (C) 1985, 87-92, 95-99, 2000-2021 %s\n", _("J�rg Schilling"));
		gtprintf("This is free software; see the source for copying conditions.  There is NO\n");
		gtprintf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
		/* END CSTYLED */
		exit(0);
	}

	shell = getenv("BEEP");
	if (shell != NULL && streql(shell, "off"))
		nobeep = TRUE;
	if ((editor = getenv("EDITOR")) == NULL)
		editor = "vi";
	if ((shell = getenv("SHELL")) == NULL)
		shell = "sh";

	underline = !raw && !visible;

	cac = ac;
	cav = av;
	for (i = 0; getfiles(&cac, &cav, options) > 0; i++, cac--, cav++);

	if (0 /*fav = (char **)malloc(i*sizeof(char *))*/) {
		cac = ac;
		cav = av;
		fac = i;
		for (i = 0; getfiles(&cac, &cav, options) > 0;
		    i++, cac--, cav++) {
			fav[i] = *cav;
		}
	} else {
		cac = ac;
		cav = av;
		getfiles(&cac, &cav, options);
	}

	if (cac == 0 && isatty(fdown(stdin)))
		usage(EX_BAD);

	if (get_modes() < 0) {
		silent++;
		if (!ununderline)
			underline = FALSE;
	}


/*	if (underline || !silent)*/
		init_termcap();
	init_tty_size();

	if (!silent) {
		usleep(150000);	/* XXX Hack bis Jobcontrol im bsh geht !!! */
		set_modes();
	}

	init_charset();

#ifdef	BUFSIZ			/* XXX #ifdef HAVE_SETBUF ??? */
	setbuf(stdout, buffer);
#endif

	/*
	 * If we evaluate "has_termcap" here, we could avoid to reduce the width
	 * of the terminal in case that "am" but not "xn" is present. This
	 * however would result in missing line breaks in files created by the
	 * screen(1) utility.
	 */
	if (am && !xn && lwidth > 2)
		lwidth--;
	lines = psize-2;

	fac = cac;
#ifdef	DEBUG
	printf("%d\n", cac);
	for (i = 0; i < cac; i++)
		printf("%s\n", cav[i]);
#endif

	if (cac == 0) {
		filename = "";
		linecnt = 0;
		f = stdin;
		page();
	} else {
		if (cac > 1)
			nameprint++;
		for (;;) {
			if ((f = fileopen(*cav, "r")) == (FILE *) NULL) {
				errmsg("Can not open '%s'.\n", *cav);
			} else {
				filename = *cav;
				if (nameprint) {
					printf("%s\n%s\n%s\n",
							dcl, filename, dcl);
					linecnt = 3;
				} else
					linecnt = 0;
				page();
				fclose(f);
				f = (FILE *)0;
			}
			for (i = 0; ; ) {
				if (direction == FORWARD) {
					cac--, cav++;
					if (cac <= 0)
						reset_modes(), exit(0);
				} else {
					if (cac < fac) {
						cac++, cav--;
					}
				}
				if (silent) break;
				if (i++ == 0) putchar('\n');
				err:

				start_standout();
				gtprintf("NEXT FILE (%s)?", *cav);
				end_standout();
				fflush(stdout);
				lines = psize-2;

				switch (get_action()) {

				case -1:
					goto err;
				case  1:
					continue;
				}
				break;
			}
			clearline();
		}
	}
	reset_modes();
	exit(0);
	return (0);	/* Keep lint happy */
}

LOCAL void
page()
{
	register int	c;
	register int	cnt;
	register unsigned char	*s;
	register unsigned char	**rctab;
		int	ocolcnt = -1;

	rctab = ctab;
	ofpos = (off_t)0;
	len = 0;			/* fill_buf() on next nextc() */
	file_raise(f, FALSE);
#ifdef	__nonono__			/* FreeBSD would read single bytes */
	setbuf(f, NULL);
#endif
	lineno = 0;
	colcnt = 0;

	if (searchnext && do_search() < 0) {
		if (!more())
			return;
	}

	while ((c = nextc()) != EOF) {
		if (c < 0) {				/* -2 on error */
			errmsg("Error reading '%s'.\n", filename);
			return;
		}
		if (c == '_' && underline) {		/* underlining */
			if (peekc() != '\b') {
				if (outchar('_'))
					return;
				continue;
			}
			getnextch();			/* it _is_ a ^H ! */
			if (peekc() == '_')
				goto bold;
			underl++;

		} else if (c == '+' && underline && peekc() == '\b') {
			getnextch();
		} else if (c == '\r' &&
			    (autodos || dosmode) && peekc() == '\n') {
			/* EMPTY */
		} else if (c == '\n') {
			if (ocolcnt == 0 && colcnt == 0 && supressblank)
				continue;
			ocolcnt = colcnt;
			lineno++;
			if (endline) {
				if (outchar('$'))
					return;
			}
			if (outchar('\n'))
				return;
		} else if (c == '\t' && !tab) {
			cnt = 8 - (colcnt&7);
			while (--cnt >= 0)
				if (outchar(' '))
					return;
		} else if (peekc() == '\b' && underline) {
			getnextch();		/* ^H */
		bold:
			while (peekc() == c) {
				getnextch();
				if (peekc() == '\b')
					getnextch();
				else
					break;
			}
			ungetch(c);
			standout++;
		} else {
			if (raw || iswprint(c)) {
				if (outchar(c))
					return;
			} else {
#if	MB_LEN_MAX > 1
				int	i = clen;
				while (--i >= 0) {
					s = rctab[*cp++];
					while (*s)
						if (outchar(*s++))
							return;
				}
#else
				s = rctab[c];
				while (*s)
					if (outchar(*s++))
						return;
#endif
			}
		}
	}
	fflush(stdout);
}

LOCAL int
outchar(c)
	int	c;
{
#if	MB_LEN_MAX > 1
	unsigned char	b[MB_LEN_MAX];
	unsigned char	*p = b;
	int		l;


	l = wctomb((char *)b, c);
	if (l < 0) {
		l = 1;
		b[0] = '?';
	}
#endif

	if (c == '\n') {
		if (len <= lwidth)
			fflush(stdout);
		if (++linecnt >= lines) {
			if (!more())
				return (1);
			clearline();
			linecnt = 0;
		} else {
			putc('\n', stdout);
		}
		colcnt = 0;
		return (0);
	} else {
		int	w = wcwidth(c);

		if (w < 0)
			w = 1;
		colcnt += w;
		if (colcnt > lwidth) {
			if (outchar('\n'))
				return (1);
			colcnt = 1;
		}
		if (standout) {
			standout--;
			if (underl) {
				underl--;
				start_ul();
				start_xstandout();
#if	MB_LEN_MAX > 1
				while (--l >= 0)
					putc(*p++, stdout);
#else
				putc(c, stdout);
#endif
				end_xstandout();
				end_ul();
			} else {
				start_xstandout();
#if	MB_LEN_MAX > 1
				while (--l >= 0)
					putc(*p++, stdout);
#else
				putc(c, stdout);
#endif
				end_xstandout();
			}
		} else if (underl) {
			underl--;
			start_ul();
#if	MB_LEN_MAX > 1
			while (--l >= 0)
				putc(*p++, stdout);
#else
			putc(c, stdout);
#endif
			end_ul();
		} else {
#if	MB_LEN_MAX > 1
			while (--l >= 0)
				putc(*p++, stdout);
#else
			putc(c, stdout);
#endif
		}
		return (0);
	}
}

LOCAL void
moreprompt()
{
	float	percent;
	off_t	size	= -1;
	off_t	pos	= -1;

	lines = psize-2;
	if (f) {
		size = filesize(f);
		pos = filepos(f) - len;
	}
	start_standout();
	if (!*filename || !f || size < 0 || pos < 0)
		gtprintf("%s%c MORE?",
				filename,
				*filename ? ':' : '-');
	else {
		percent = (float)pos;
		percent /= (float)size;
		percent *= 100.0;
		gtprintf("%s%c %.1f %% MORE?",
				filename,
				*filename ? ':' : '-',
				percent);
	}
	end_standout();
	fflush(stdout);
}

LOCAL BOOL
more()
{
	if (silent)
		return (TRUE);
	putchar('\n');
	for (;;) {
		moreprompt();
		switch (get_action()) {

		case  1:
			return (FALSE);
		case -1:
			continue;
		}
		ofpos = filepos(f)-len;
		break;
	}
	return (TRUE);
}


/*
 *	Return:
 *		-1	error
 *		 0	continue this file
 *		 1	stop on this file
 *	EXIT:
 *		 on demand
 */
LOCAL int
get_action()
{
	int	c;
	char	buf[128];

	direction = FORWARD;
	searchnext = 0;

	switch (c = inwchar()) {

	case 'p':
	case 'P':
	case 'n':
	case 'N':
		direction = (c == 'P' || c == 'p');
		clearline();
		return (1);
	case 's':
	case 'S':
	case 003 :				/* ^C  */
	case 004 :				/* ^D  */
	case 034 :				/* ^\  */
	case 0177:				/* DEL */
	case EOF :
		fixtty(0);
		/* NOTREACHED */
	case 'Y':
	case 'y':
	case '\r':
	case '\n':
	case ' ':
		if (clear)
			clearscreen();
		break;
	case 'H':
	case 'h':
		lines = lines/2;
		break;
	case 'Q':
	case 'q':
		lines = lines/4;
		break;
	case 'L':
	case 'l':
		lines = 1;
		break;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		lines = c-'0';
		break;
	case '/':
		clearline();
		printf("/");
		fflush(stdout);
		if (!read_pattern())
			return (-1);
		/* FALLTHRU */
	case 'r':
	case 'R':
		if (f == (FILE *)0)
			searchnext = 1;
		else if (do_search() < 0)
			return (-1);
		break;
	case '\f':
		redraw();	/* XXX eigentlich nicht ?? */
		break;
	case '?':
		clearline();
		onlinehelp();
		return (-1);
	case '!':
		clearline();
		sprintf(buf, "%s -t", shell);
		(void) system(buf);
		return (-1);
	case 'v':
	case 'V':
		if (f != stdin) {
			clearline();
			sprintf(buf, "%s +%d %s", editor, lineno, filename);
			printf("%s", buf);
			fflush(stdout);
			(void) system(buf);
			return (-1);
		}
		/* FALLTHRU */
	default:
		if (!nobeep)
			putchar('\007');
		clearline();
		return (-1);
	}
	return (0);
}

LOCAL void
onlinehelp()
{
	/* BEGIN CSTYLED */
	gtprintf("\n---------------------------------------------------------------\n");
	gtprintf("N, n			Next File\n");
	gtprintf("P, p			Previous File\n");
	gtprintf("S, s, ^C, ^D, ^\\, DEL	Exit (stop)\n");
	gtprintf("Y, y, <return>, <space>	Display next screenfull of text\n");
	gtprintf("H, h			Display next half screenfull of text\n");
	gtprintf("Q, q			Display next quarter screenfull of text\n");
	gtprintf("L, l			Display next line of text\n");
	gtprintf("1-9			Display next <n> lines of text\n");
	gtprintf("/pattern		Search for <pattern>\n");
	gtprintf("R, r			Re-search for <pattern>\n");
	gtprintf("^L			Redraw screen\n");
	gtprintf("?			Display this message\n");
	gtprintf("!			Execute command\n");
	gtprintf("V, v			Edit file\n");
	gtprintf("---------------------------------------------------------------\n");
	/* END CSTYLED */
}

LOCAL void
redraw()
{
	if (f && fileseek(f, ofpos) < 0) {
		if (!nobeep)
			putchar('\007');
		clearline();
		return;
	}
	len = 0;
	clearscreen();
}

LOCAL int
inchar()
{
	char	c;
	int	ret = 0;

	c = '\004';		/* return ^D on EOF */
	do {
#	ifdef	USE_GETCH
		c = getch();	/* DOS console input	*/
#	else
		ret = read(tty, &c, 1);
#	endif
	} while (ret < 0 && geterrno() == EINTR);
	if (ret == 0)
		return (EOF);
	return (c);
}

#if	MB_LEN_MAX > 1
/*
 * Get next wide character from tty
 */
LOCAL int
inwchar()
{
	register int	cur_max = MB_CUR_MAX;
	register int	i;
	int		mlen;
	wchar_t		c = -1;
	char		cstr[MB_LEN_MAX];
	char		*csp;
	int		ic;

	(void) mbtowc(NULL, NULL, 0);
	for (i = 1, csp = cstr; i <= cur_max; i++) {
		ic = inchar();
		if (ic == EOF)
			break;
		*csp++ = (char)ic;
		mlen = mbtowc(&c, cstr, i);
		if (mlen >= 0)
			break;
		(void) mbtowc(NULL, NULL, 0);
	}
#ifdef	_NEXTWC_DEBUG
	fprintf(stderr, "C %d %x\n", c, c);
#endif
	return ((int)c);
}
#endif


#ifndef	nextc
LOCAL int
nextc()
{
	if (--len >= 0) {
		return ((int) *bp++);
	} else {
		if (fill_buf() <= 0)
			return (len == 0 ? EOF : -2);
	}
	len--;
	return ((int) *bp++);
}
#endif

#if	MB_LEN_MAX > 1
/*
 * Read the next multi byte character from the buffer.
 * If the buffer is empty or if less than a multi byte character is inside,
 * it is refilled. When an illegal byte sequence is discovered, a single
 * byte is removed from the buffer in hope to be able to resync.
 */
LOCAL int
nextwc()
{
	BOOL	eof = FALSE;
	int	mlen;
	wchar_t	c;

	if (len <= 0) {
		int	olen;
again:
		olen = len;
		if (fill_buf() <= 0) {
			return (len == 0 ? EOF : -2);
		} else if (len > 0 && olen == len) {
			eof = TRUE;
		}
	}
	mlen = mbtowc(&c, (char *)bp, len);
	if (mlen >= 0) {
		if (mlen == 0)
			mlen = 1;
		clen = mlen;
		cp = bp;
		bp += mlen;
		len -= mlen;
		return (c);
	} else {
		mbtowc(NULL, NULL, 0);
		if (len < MB_CUR_MAX && !eof) {
			seterrno(0);
			goto again;
		}
		clen = 1;
		cp = bp;
		bp++;
		len--;
#ifdef	EILSEQ
		if (geterrno() == EILSEQ) {
			return (*cp);
		}
#endif
	}
	return (-2);
}

/*
 * The same as nextwc(), but the buffer is untouched.
 */
LOCAL int
peekwc()
{
	BOOL	eof = FALSE;
	int	mlen;
	wchar_t	c;

	if (len <= 0) {
		int	olen;
again:
		olen = len;
		if (fill_buf() <= 0) {
			return (len == 0 ? EOF : -2);
		} else if (len > 0 && olen == len) {
			eof = TRUE;
		}
	}
	mlen = mbtowc(&c, (char *)bp, len);
	if (mlen >= 0) {
		if (mlen == 0)
			mlen = 1;
		pclen = mlen;
		pcp = bp;
		return (c);
	} else {
		mbtowc(NULL, NULL, 0);
		if (len < MB_CUR_MAX && !eof) {
			seterrno(0);
			goto again;
		}
		pclen = 1;
		pcp = bp;
#ifdef	EILSEQ
		if (geterrno() == EILSEQ) {
			return (*pcp);
		}
#endif
	}
	return (-2);
}
#endif

#ifndef	ungetch
LOCAL int
ungetch(c)
	int	c;
{
	if (c == EOF)
		return (c);
	/*
	 * If bp is at the beginning of the buffer, this may go
	 * to one character before the buffer (see below).
	 */
	len++;
	return (*--bp = c);
}
#endif

#if	MB_LEN_MAX > 1
/*
 * Return the character back to the buffer.
 */
LOCAL int
ungetwch(c)
	int	c;
{
	unsigned char	b[MB_LEN_MAX];
	unsigned char	*p;
	int		l;

	if (c == EOF)
		return (c);

	l = wctomb((char *)b, c);
	if (l < 0) {
		l = 1;
		b[0] = '?';
	}
	len += l;
	p = &b[l];
	while (--l >= 0)
		*--bp = *--p;

	return (c);
}

/*
 * Consume the last character fetched by peekwc()
 */
LOCAL void
getnextwch()
{
	len -= pclen;
	bp += pclen;
	cp = pcp;
	pclen = 0;
}
#endif

LOCAL int
fill_buf()
{
	int		i;

	/*
	 * Allow ungetch() to always put back one character.
	 * This is done by reserving one char before the normal
	 * space in "mybuf".
	 */

	if (len > 0) {
		unsigned char	*p = bp;

		/*
		 * Move a partial character left at the end of the buffer
		 * to the space before the beginning of the buffer.
		 */
		if (len > MB_LEN_MAX)
			len = MB_LEN_MAX;
		p = &mybuf[(2*MB_LEN_MAX) - len];
		for (i = len; --i >= 0; ) {
			*p++ = *bp++;
		}
		bp = &mybuf[(2*MB_LEN_MAX) - len];
	} else {
		len = 0;
		bp = &mybuf[(2*MB_LEN_MAX)];
	}
	i = fileread(f, &mybuf[2*MB_LEN_MAX], sizeof (mybuf) - 2*MB_LEN_MAX);
	if (i < 0)
		return (i);
	return (len += i);
}

LOCAL BOOL
read_pattern()
{
		int	patlen;
	register int	c;
	register int	count = 0;
#if	MB_LEN_MAX > 1
	register wchar_t *s = searchbuf;
	unsigned char	b[MB_LEN_MAX];
#else
	register char	*s = searchbuf;
#endif
	register unsigned char	*p;
	register int	i;

	while ((c = inwchar()) != EOF &&
			count++ < (SEARCHSIZE - 1) &&
			c != '\004' && c != '\r' && c != '\n') {

		if (c == 0177) {		/* DEL */
			if (s == searchbuf) {
				if (!nobeep)
					putchar('\007');
			} else {
				--count;
				--s;
#if	MB_LEN_MAX > 1 && defined(HAVE_WCWIDTH)
				if (iswprint(*s))
					i = wcwidth(*s);
				else
					i = csize[*s & 0xFF];
#else
				i = csize[*s & 0xFF];
#endif
				for (; --i >= 0; )
					printf("\b \b");
			}
			fflush(stdout);
		} else {
#if	MB_LEN_MAX > 1 && defined(HAVE_WCWIDTH)
			int	l;

			if (iswprint(c)) {
				l = wctomb((char *)b, c);
				if (l < 1) {
					p = ctab[c & 0xFF];
				} else {
					p = b;
					b[l] = '\0';
				}
			} else {
				p = ctab[c & 0xFF];
			}
#else
			p = ctab[c & 0xFF];
#endif
			*s++ = c;
			while (*p)
				putchar(*p++);
			fflush(stdout);
		}
	}
	*s = '\0';
	if (aux)
		free((char *)aux);
#if	MB_LEN_MAX > 1
	patlen = wcslen(searchbuf);
#else
	patlen = strlen(searchbuf);
#endif
	aux = (int *) malloc(patlen * sizeof (int));
	state = (int *) malloc((patlen+1) * sizeof (int));
#if	MB_LEN_MAX > 1
	alt = patwcompile(searchbuf, patlen, aux);
#else
	alt = patcompile((unsigned char *)searchbuf, patlen, aux);
#endif
	if (!alt) {
		gtprintf("Bad Pattern.\r");
		fflush(stdout);
		sleep(1);
		return (FALSE);
	}
	return (TRUE);
}

LOCAL int
do_search()
{
	register unsigned char *lp;
	register unsigned char *rbp;
		unsigned char *sp;
	register int	rest = len;
		off_t	curpos;
		BOOL	skipping = FALSE;
		int	newlines = 0;
	unsigned char	sbuf[P_BUFSIZE];

	if (!alt) {
		if (!nobeep)
			putchar('\007');
		gtprintf("No previous search.\r");
		fflush(stdout);
#ifdef	DEBUG
		sleep(1);
#endif
		return (-1);
	}
	curpos = filepos(f)-len;

#ifdef	DEBUG
	fprintf(stderr,
		"Efilepos: %lld len: %d bp: 0x%X\n",
		(Llong)filepos(f), len, bp);
#endif
	for (;;) {
		if (rest < MB_LEN_MAX) {
			int	olen;
			/*
			 * We may be operating on a copy and thus
			 * need to set "len" to the remaining characters to
			 * tell fill_buf() that it needs to do a refill.
			 */
			olen = len = rest;
			if (fill_buf() <= 0 || olen == len) {
				if (!nobeep)
					putchar('\007');
				gtprintf("Pattern not found.\r");
				fflush(stdout);
				if (f && fileseek(f, curpos) >= 0)
					len = 0;
				return (-1);
			}
			newlines = 0;
		}
		rbp = lp = sp = bp;
		rest = len;
		if (underline &&
		    findbytes(lp, rest, '\b') != NULL) {
			rest = unul(sbuf, rbp, len);
			rbp = lp = sp = sbuf;
		}
		for (; rest > 0; rest--) {
#if	MB_LEN_MAX > 1
			if (patmbmatch(searchbuf,
#else
			if (patmatch((unsigned char *)searchbuf,
#endif
					aux, rbp, 0, rest, alt, state)) {
#ifdef	DEBUG
			fprintf(stderr,
				"Afilepos: %lld rest: %d rbp: 0x%X lp: 0x%X\n",
				(Llong)filepos(f), rest, rbp, lp);
#endif
				if (skipping) {
					printf("\n");
					if (sp == bp) {
						bp = lp;
						len = rest + (rbp - lp);
					} else {
						lp = bp;
						rbp = &bp[len];
						while (--newlines >= 0) {
							unsigned char *olp = lp;
							if ((lp = (Uchar *)
								findbytes(lp,
								    rbp - lp,
								    '\n')) ==
								    NULL) {
								lp = olp;
								break;
							}
							lp++;
						}
						bp = lp;
						len = rbp - lp;
					}
				}
#ifdef	DEBUG
				fprintf(stderr,
					"Afilepos: %lld len: %d bp: 0x%X\n",
					(Llong)filepos(f), len, bp);
#endif
				return (0);
			}
			if (*rbp++ == '\n') {
				/*
				 * Remember last line start pointer.
				 */
				lp = rbp;
				newlines++;
				if (!skipping) {
					skipping = TRUE;
					gtprintf("skipping...\r");
					fflush(stdout);
				}
#ifdef	DEBUG
				fprintf(stderr, ".");
#endif
			}
		}
	}
}

/*
 * Remove underlining and overstriking sequences.
 * Put the result into "ob".
 */
LOCAL int
unul(ob, ib, amt)
	register Uchar	*ob;
	register Uchar	*ib;
	register int	amt;
{
	register Uchar	*oob = ob;
		wchar_t	c;
	register ssize_t wclen;

	mbtowc(NULL, NULL, 0);
	while (amt > 0) {
		wclen = mbtowc(&c, (char *)ib, amt);
		if (wclen < 1) {
			*ob++ = *ib++;
			amt--;
			continue;
		}
		amt -= wclen;
		ib += wclen;
		if (c == '_') {				/* underlining */
			if (ib[0] != '\b') {
				ib -= wclen;
				while (--wclen >= 0)
					*ob++ = *ib++;
				continue;
			}
			ib++;				/* eat ^H */
			amt--;
			wclen = mbtowc(&c, (char *)ib, amt);
			if (wclen > 0 && c == '_') {	/* _ ^H _ */
				goto bold;
			}
		} else if (c == '+' && ib[0] == '\b') {	/* overstriking */
			/* + ^H o */

			ib++;				/* eat ^H */
			amt--;
		} else if (ib[0] == '\b') {
			wchar_t	c2;
			ssize_t	wclen2;
			Uchar	*oib;

			/* N ^H N ^H N ^H N */
			ib++;				/* eat ^H */
			amt--;
		bold:
			oib = ib - (wclen +1);
			while (amt > 0) {
				wclen2 = mbtowc(&c2, (char *)ib, amt);
				if (wclen2 < 1)
					break;
				if (c2 != c)
					break;
				ib += wclen2;		/* eat c2 */
				amt -= wclen2;
				if (ib[0] == '\b') {	/* Check for ^H */
					ib++;		/* eat ^H */
					amt--;
				} else
					break;
			}
			while (--wclen >= 0)
				*ob++ = *oib++;
		} else {
			ib -= wclen;
			while (--wclen >= 0)
				*ob++ = *ib++;
		}
	}
	*ob = '\0';
	return (ob - oob);
}

char	stbuf[1024];	/* Bufer for termcap array (i.e. so and se) */

LOCAL void
init_termcap()
{
	char	*tname;
	char	*sbp;

	sbp = stbuf;

	if ((tname = getenv("TERM")) && tgetent(NULL, tname) == 1) {
		has_termcap = TRUE;

		so = tgetstr("so", &sbp);	/* start standout */
		se = tgetstr("se", &sbp);	/* end standout */
		us = tgetstr("us", &sbp);	/* start underline */
		ue = tgetstr("ue", &sbp);	/* end underline */
		md = tgetstr("md", &sbp);	/* start bold */
		me = tgetstr("me", &sbp);	/* end attributes */
		ce = tgetstr("ce", &sbp);	/* clear endline */
		cl = tgetstr("cl", &sbp);	/* clear screen */
		li = tgetnum("li");		/* lines on screen */
		co = tgetnum("co");		/* columns on screen */
		am = tgetflag("am");		/* automatic margins */
		xn = tgetflag("xn");		/* newline ignored > 80 */
		if (so != NULL && se != NULL) {
			has_standout = TRUE;
		} else {
			so = se = NULL;
		}
		if (md != NULL && me != NULL) {
			has_bold = TRUE;
		} else {
			md = NULL;
		}
		if (us != NULL && ue != NULL) {
			has_ul = TRUE;
		} else {
			us = ue = NULL;
		}
		if (has_bold) {
			xso = md;
			xse = me;
		} else if (has_standout) {
			xso = so;
			xse = se;
		} else {
			xso = xse = NULL;
		}
		if (debug) {
			printf("so: %d bo: %d ul: %d\n",
				has_standout, has_bold, has_ul);
			sleep(1);
		}
	}
}

LOCAL int
oc(c)
	int	c;
{
	return (putchar(c));
}

LOCAL void
start_standout()
{
	tputs(so, 1, oc);
}

LOCAL void
end_standout()
{
	tputs(se, 1, oc);
}

#ifdef	__needed__
LOCAL void
start_bold()
{
	tputs(md, 1, oc);
}
#endif

LOCAL void
end_attr()
{
	tputs(me, 1, oc);
}

LOCAL void
start_xstandout()
{
	tputs(xso, 1, oc);
}

LOCAL void
end_xstandout()
{
	tputs(xse, 1, oc);
}

LOCAL void
start_ul()
{
	if (has_ul)
		tputs(us, 1, oc);
}

LOCAL void
end_ul()
{
	if (has_ul)
		tputs(ue, 1, oc);
}

LOCAL void
clearline()
{
	int	i;

	if (silent) {
		putchar('\n');
		return;
	}
	putchar('\r');
	if (ce) {
		tputs(ce, 1, oc);
	} else {
		for (i = 1; i < lwidth; i++)
			putchar(' ');
		putchar('\r');
	}
	fflush(stdout);
}

LOCAL void
clearscreen()
{
	if (cl)
		tputs(cl, 1, oc);
}

LOCAL void
init_tty_size()
{
#ifdef no_TIOCGSIZE
#if	defined(TIOCGSIZE) || defined(TIOCGWINSZ)
#ifdef	TIOCGWINSZ
	struct		winsize ws;

	ws.ws_rows = 0;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, (char *)&ws) >= 0) {
		if (ws.ws_rows) {
			psize = ws.ws_rows;
			lwidth = ws.ws_cols;
		}
	}
#else
	struct		ttysize	ts;

	ts.ts_lines = 0;
	if (ioctl(STDOUT_FILENO, TIOCGSIZE, (char *)&ts) >= 0) {
		if (ts.ts_lines) {
			psize = ts.ts_lines;
			lwidth = ts.ts_cols;
		}
	}
#endif
#endif
#else
	if (psize == 0) {
		if (li > 0 && li < 1000)
			psize = li;
		else
			psize = DEF_PSIZE;
	}
	if (lwidth == 0) {
		if (co > 0 && co < 1000)
			lwidth = co;
		else
			lwidth = DEF_LWIDTH;
	}
#endif
}

LOCAL int
get_modes()
{
#	ifdef	USE_V7_TTY
	return (ioctl(STDOUT_FILENO, TIOCGETP, &old));

#	else	/* USE_V7_TTY */

#	ifdef	USE_TERMIOS
#	ifdef	TCSANOW
	return (tcgetattr(STDOUT_FILENO, &old));
#	else
	return (ioctl(STDOUT_FILENO, TCGETS, &old));
#	endif
#	else	/* USE_TERMIOS */
	return (0);
#	endif	/* USE_TERMIOS */
#	endif	/* USE_V7_TTY */
}

LOCAL void
set_modes()
{
#ifdef	HAVE__DEV_TTY
	if (tty < 0 && (tty = open("/dev/tty", O_RDONLY)) < 0)
		errmsg("Can't open '/dev/tty'\n");
#endif
	if (tty < 0)
		tty = fileno(stderr);

	/*
	 * Do signal handling only if they are not already
	 * ignored.
	 */
	if (signal(SIGINT, SIG_IGN) != SIG_IGN) {
		signal(SIGINT, fixtty);
#ifdef	SIGQUIT
		signal(SIGQUIT, fixtty);
#endif
#ifdef	SIGTSTP
		if (signal(SIGTSTP, SIG_IGN) == SIG_DFL)
			signal(SIGTSTP, tstp);
#endif
	}
#	ifdef	USE_V7_TTY
	movebytes(&old, &new, sizeof (old));
#		ifdef	LPASS8
	{
		int	lmode;

		ioctl(STDOUT_FILENO, TIOCLGET, &lmode);
		if (lmode & LPASS8)
			raw8 = TRUE;
	}
#		else
	if (old.sg_flags & RAW)
		raw8 = TRUE;
#		endif
	new.sg_flags |= CBREAK;
	new.sg_flags &= ~ECHO;
	if (ioctl(STDOUT_FILENO, TIOCSETN, &new) < 0)
		comerr("Can not set new modes.\n");

#	else	/* USE_V7_TTY */

#	ifdef	USE_TERMIOS
	movebytes(&old, &new, sizeof (old));
	if (!(old.c_iflag & ISTRIP))
		raw8 = TRUE;
#ifdef	__never__
	new.c_iflag = ICRNL;
	new.c_oflag = (OPOST|ONLCR);
	new.c_lflag = ISIG;
#endif
	new.c_lflag &= ~(ICANON|ECHO);
	new.c_cc[VMIN] = 1;
	new.c_cc[VTIME] = 0;
#	ifdef	TCSANOW
	if (tcsetattr(STDOUT_FILENO, TCSADRAIN, &new) < 0)
#	else
	if (ioctl(STDOUT_FILENO, TCSETSW, &new) < 0)
#	endif
		comerr("Can not set new modes.\n");
#	endif	/* USE_TERMIOS */

#	endif	/* USE_V7_TTY */

#ifdef	CATCH_SIGCONT_here
#ifdef	SIGCONT
	signal(SIGONT, redraw); /* XXX ??? */
#endif
#endif
}

/*
 * Reset to previous tty modes.
 */
LOCAL void
reset_modes()
{
	if (!silent) {
#	ifdef	USE_V7_TTY
		if (ioctl(STDOUT_FILENO, TIOCSETN, &old) < 0)

#	else	/* USE_V7_TTY */

#	ifdef	USE_TERMIOS
#		ifdef	TCSANOW
		if (tcsetattr(STDOUT_FILENO, TCSADRAIN, &old) < 0)
#		else
		if (ioctl(STDOUT_FILENO, TCSETSW, &old) < 0)
#		endif
#	else	/* USE_TERMIOS */
		if (0)
#	endif	/* USE_TERMIOS */
#	endif	/* USE_V7_TTY */
			comerr("Can not reset old modes.\n");
	}
}

/*
 * Fix tty and exit.
 */
LOCAL void
fixtty(sig)
	int	sig;
{
	end_standout();
	end_attr();
	end_ul();
	clearline();
	reset_modes();
	exit(0);
}
