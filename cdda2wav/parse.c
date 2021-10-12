/* %Z%%M%	%I% %E% Copyright 2001-2008 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"%Z%%M%	%I% %E% Copyright 2001-2008 J. Schilling";
#endif
/*
 *	Interactive command parser for cdda2wav
 *
 *	Copyright (c) 2001-2008 J. Schilling
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

/*
 *	Commands:
 *		read	<start-sector>
 *
 *	Replies:
 *		200	OK
 *		400	Bad Request
 *		404	Not Found
 */

#include <schily/mconfig.h>
#include <stdio.h>
#include <schily/standard.h>
#include <ctype.h>
#include <schily/jmpdefs.h>
#include <schily/string.h>
#include <schily/utypes.h>
#include <schily/varargs.h>
#include <schily/schily.h>

#include "toc.h"

#define	E_OK		200
#define	E_BAD		400
#define	E_NOTFOUND	404

#define	C_BAD		0
#define	C_STOP		1
#define	C_CONT		2
#define	C_READ		3
#define	C_EXIT		4

typedef struct cmd {
	int	cmd;
	int	argtype;
	long	arg;
} cmd_t;

typedef struct keyw {
	char	*k_name;
	int	k_type;
} keyw_t;

LOCAL keyw_t	keywords[] = {
	{ "stop",	C_STOP },
	{ "cont",	C_CONT },
	{ "read",	C_READ },
	{ "exit",	C_EXIT },
	{ "quit",	C_EXIT },
	{ NULL,		0 },
};

typedef struct err {
	int	num;
	char	*name;
} err_t;

LOCAL err_t	errs[] = {
	{ E_OK,		"OK" },
	{ E_BAD,	"Bad Request" },
	{ E_NOTFOUND,	"Not Found" },
	{ -1,		NULL },
};

LOCAL	sigjmps_t	jmp;


EXPORT	int	parse		__PR((long *lp));
LOCAL	keyw_t	*lookup		__PR((char *word, keyw_t table[]));

LOCAL	FILE	*pfopen		__PR((char *name));
LOCAL	char	*pfname		__PR((void));
LOCAL	char	*nextline	__PR((FILE *f));
LOCAL	void	ungetline	__PR((void));
LOCAL	char	*skipwhite	__PR((const char *s));
LOCAL	char	*peekword	__PR((void));
LOCAL	char	*lineend	__PR((void));
LOCAL	char	*markword	__PR((char *delim));
LOCAL	char	getworddelim	__PR((void));
LOCAL	char	*getnextitem	__PR((char *delim));
LOCAL	char	*neednextitem	__PR((char *delim));
LOCAL	char	*nextword	__PR((void));
LOCAL	char	*needword	__PR((void));
LOCAL	char	*curword	__PR((void));
LOCAL	char	*nextitem	__PR((void));
LOCAL	char	*needitem	__PR((void));
LOCAL	void	checkextra	__PR((void));
LOCAL	void	pabort		__PR((int errnum, const char *fmt, ...));
LOCAL	void	wok		__PR((void));


EXPORT int
parse(lp)
	long	*lp;
{
		long	l;
	register keyw_t	*kp;
		char	*p;
		cmd_t	cmd;
static		FILE	*f;

	if (f == NULL)
		f = pfopen(NULL);
	if (f == NULL)
		return (-1);
again:
	if (sigsetjmp(jmp.jb, 1) != 0) {
		/*
		 * We come here from siglongjmp()
		 */
		;
	}
	if ((p = nextline(f)) == NULL)
		return (-1);

	p = nextword();
	kp = lookup(p, keywords);
	if (kp) {
		extern	void drop_all_buffers	__PR((void));

		cmd.cmd = kp->k_type;
		switch (kp->k_type) {

		case C_STOP:
			/* Flush buffers */
			drop_all_buffers();
			wok();
			goto again;
		case C_CONT:
			wok();
			return (0);
		case C_READ:
			p = nextword();
			if (streql(p, "sectors")) {
				p = nextword();
				if (*astol(p, &l) != '\0') {
					pabort(E_BAD, "Not a number '%s'", p);
				}
				*lp = l;
			} else if (streql(p, "tracks")) {
				p = nextword();
				if (*astol(p, &l) != '\0') {
					pabort(E_BAD, "Not a number '%s'", p);
				}
				if (l < FirstAudioTrack() || l > LastAudioTrack())
					pabort(E_BAD, "Bad track number '%s'", p);
				*lp = Get_StartSector(l);
			} else {
				pabort(E_BAD, "Bad 'read' parameter '%s'", p);
			}
			wok();
			break;
		case C_EXIT:
			wok();
			return (-1);
		default:
			pabort(E_NOTFOUND, "Unknown command '%s'", p);
			return (0);
		}
		checkextra();
		return (0);
	}
/*	checkextra();*/
	pabort(E_NOTFOUND, "Unknown command '%s'", p);
	return (0);
}

LOCAL keyw_t *
lookup(word, table)
	char	*word;
	keyw_t	table[];
{
	register keyw_t	*kp = table;

	while (kp->k_name) {
		if (streql(kp->k_name, word))
			return (kp);
		kp++;
	}
	return (NULL);
}

/*--------------------------------------------------------------------------*/
/*
 * Parser low level functions start here...
 */

LOCAL	char	linebuf[4096];
LOCAL	char	*fname;
LOCAL	char	*linep;
LOCAL	char	*wordendp;
LOCAL	char	wordendc;
LOCAL	int	olinelen;
LOCAL	int	linelen;
LOCAL	int	lineno;

LOCAL	char	worddelim[] = "=:,/";
LOCAL	char	nulldelim[] = "";

#ifdef	DEBUG
LOCAL void
wdebug()
{
		printf("WORD: '%s' rest '%s'\n", linep, peekword());
		printf("linep %lX peekword %lX end %lX\n",
			(long)linep, (long)peekword(), (long)&linebuf[linelen]);
}
#endif

LOCAL FILE *
pfopen(name)
	char	*name;
{
	FILE	*f;

	if (name == NULL) {
		fname = "stdin";
		return (stdin);
	}
	f = fileopen(name, "r");
	if (f == NULL)
		comerr("Cannot open '%s'.\n", name);

	fname = name;
	return (f);
}

LOCAL char *
pfname()
{
	return (fname);
}

LOCAL char *
nextline(f)
	FILE	*f;
{
	register int	len;

	do {
		fillbytes(linebuf, sizeof (linebuf), '\0');
		len = fgetline(f, linebuf, sizeof (linebuf));
		if (len < 0)
			return (NULL);
		if (len > 0 && linebuf[len-1] == '\r') {
			linebuf[len-1] = '\0';
			len--;
		}
		linelen = len;
		lineno++;
	} while (linebuf[0] == '#');

	olinelen = linelen;
	linep = linebuf;
	wordendp = linep;
	wordendc = *linep;

	return (linep);
}

LOCAL void
ungetline()
{
	linelen = olinelen;
	linep = linebuf;
	*wordendp = wordendc;
	wordendp = linep;
	wordendc = *linep;
}

LOCAL char *
skipwhite(s)
	const char	*s;
{
	register const Uchar	*p = (const Uchar *)s;

	while (*p) {
		if (!isspace(*p))
			break;
		p++;
	}
	return ((char *)p);
}

LOCAL char *
peekword()
{
	return (&wordendp[1]);
}

LOCAL char *
lineend()
{
	return (&linebuf[linelen]);
}

LOCAL char *
markword(delim)
	char	*delim;
{
	register	BOOL	quoted = FALSE;
	register	Uchar	c;
	register	Uchar	*s;
	register	Uchar	*from;
	register	Uchar	*to;

	for (s = (Uchar *)linep; (c = *s) != '\0'; s++) {
		if (c == '"') {
			quoted = !quoted;
/*			strcpy((char *)s, (char *)&s[1]);*/
			for (to = s, from = &s[1]; *from; ) {
				c = *from++;
				if (c == '\\' && quoted && (*from == '\\' || *from == '"'))
					c = *from++;
				*to++ = c;
			}
			*to = '\0';
			c = *s;
linelen--;
		}
		if (!quoted && isspace(c))
			break;
		if (!quoted && strchr(delim, c) && s > (Uchar *)linep)
			break;
	}
	wordendp = (char *)s;
	wordendc = (char)*s;
	*s = '\0';

	return (linep);
}

LOCAL char
getworddelim()
{
	return (wordendc);
}

LOCAL char *
getnextitem(delim)
	char	*delim;
{
	*wordendp = wordendc;

	linep = skipwhite(wordendp);
	return (markword(delim));
}

LOCAL char *
neednextitem(delim)
	char	*delim;
{
	char	*olinep = linep;
	char	*nlinep;

	nlinep = getnextitem(delim);

	if ((olinep == nlinep) || (*nlinep == '\0'))
		pabort(E_BAD, "Missing text");

	return (nlinep);
}

LOCAL char *
nextword()
{
	return (getnextitem(worddelim));
}

LOCAL char *
needword()
{
	return (neednextitem(worddelim));
}

LOCAL char *
curword()
{
	return (linep);
}

LOCAL char *
nextitem()
{
	return (getnextitem(nulldelim));
}

LOCAL char *
needitem()
{
	return (neednextitem(nulldelim));
}

LOCAL void
checkextra()
{
	if (peekword() < lineend())
		pabort(E_BAD, "Extra text '%s'", peekword());
}

/* VARARGS1 */
#ifdef	PROTOTYPES
LOCAL void
pabort(int errnum, const char *fmt, ...)
#else
LOCAL void
pabort(errnum, fmt, va_alist)
	int	errnum;
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;
	err_t	*ep = errs;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	while (ep->num >= 0) {
		if (ep->num == errnum)
			break;
		ep++;
	}
	if (ep->num >= 0) {
		error("%d %s. ", ep->num, ep->name);
	}
	errmsgno(EX_BAD, "%r on line %d in '%s'.\n",
		fmt, args, lineno, fname);
	va_end(args);
	siglongjmp(jmp.jb, 1);
}

LOCAL void
wok()
{
	error("200 OK\n");
}
