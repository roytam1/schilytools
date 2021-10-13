/* @(#)tgetent.c	1.50 21/07/22 Copyright 1986-2021 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)tgetent.c	1.50 21/07/22 Copyright 1986-2021 J. Schilling";
#endif
/*
 *	Access routines for TERMCAP database.
 *
 *	Copyright (c) 1986-2021 J. Schilling
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

/*
 * XXX Non POSIX imports from libschily: geterrno()
 */
#ifdef	BSH
#include <schily/stdio.h>
#include "bsh.h"
#endif

#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/fcntl.h>
#include <schily/string.h>
#include <schily/signal.h>
#include <schily/errno.h>
#include <schily/schily.h>
#include <schily/ioctl.h>	/* Need to be before termios.h (BSD botch) */
#include <schily/termios.h>
#include <schily/ctype.h>
#include <schily/utypes.h>
#include <schily/termcap.h>

#ifdef	NO_LIBSCHILY
#define	geterrno()	(errno)
#endif
#ifdef	BSH
#define	getenv		getcurenv
#endif

#ifdef	pdp11
#define	TRDBUF		512	/* Size for read(2) buffer		*/
#else
#define	TRDBUF		8192	/* Size for read(2) buffer		*/
#endif
#define	TMAX		1024	/* Historical termcap buffer size	*/
#define	MAXLOOP		64	/* Max loop count for tc= directive	*/
				/* A tc= nesting of 63 has been seen.	*/
#define	TSIZE_SPACE	14	/* Space needed for: "li#xxx:co#yyy:"	*/

LOCAL	char	_Eterm[]	= "TERM";
LOCAL	char	_Etermcap[]	= "TERMCAP";
LOCAL	char	_Etermpath[]	= "TERMPATH";
#if	defined(INS_BASE) && defined(PROTOTYPES)
LOCAL	char	_termpath[]	= ".termcap /etc/termcap" " " INS_BASE "/etc/termcap";
#else
LOCAL	char	_termpath[]	= ".termcap /etc/termcap";
#endif
LOCAL	char	_tc[]		= "tc";
				/*
				 * Additional terminfo quotes
				 * "e^[" "::" ",," "s " "l\n"
				 */
#ifdef	PROTOTYPES
LOCAL	char	_quotetab[]	= "E^^\\\\e::,,s l\nn\nr\rt\ta\ab\bf\fv\v";
#else
LOCAL	char	_quotetab[]	= "E^^\\\\e::,,s l\nn\nr\rt\ta\07b\bf\fv\v";
#endif

LOCAL	char	_etoolong[]	= "Termcap entry too long\n";
LOCAL	char	_ebad[]		= "Bad termcap entry\n";
LOCAL	char	_eloop[]	= "Infinite tc= loop\n";
LOCAL	char	_enomem[]	= "No memory for parsing termcap\n";

LOCAL	char	*tbuf = 0;
LOCAL	int	tbufsize = 0;
LOCAL	BOOL	tbufmalloc = FALSE;
LOCAL	int	tflags = 0;
LOCAL	int	loopcount = 0;

EXPORT	int	tgetent		__PR((char *bp, char *name));
EXPORT	int	tcsetflags	__PR((int flags));
EXPORT	char	*tcgetbuf	__PR((void));
LOCAL	int	tchktc		__PR((char *name));
LOCAL	BOOL	tmatch		__PR((char *name));
LOCAL	char	*tskip		__PR((char *ep));
LOCAL	char	*tfind		__PR((char *ep, char *ent));
EXPORT	int	tgetnum		__PR((char *ent));
EXPORT	BOOL	tgetflag	__PR((char *ent));
EXPORT	char	*tgetstr	__PR((char *ent, char **array));
EXPORT	char	*tdecode	__PR((char *ep, char **array));
#if	defined(TIOCGSIZE) || defined(TIOCGWINSZ)
LOCAL	void	tgetsize	__PR((void));
LOCAL	void	tdeldup		__PR((char *ent));
LOCAL	char	*tinsint	__PR((char *ep, int i));
#endif
LOCAL	void	tstrip		__PR((void));
LOCAL	char	*tmalloc	__PR((int size));
LOCAL	char	*trealloc	__PR((char *p, int size));
#ifdef	NO_LIBSCHILY
#define	ovstrcpy	_ovstrcpy
LOCAL	char	*ovstrcpy	__PR((char *p2, const char *p1));
#endif
LOCAL	void	e_tcname	__PR((char *name));

/*
 * Returns:
 *	1	Entry found
 *	0	Could not malloc() buffer with tgetent(NULL, name)
 *	0	Database file opened, entry not found
 *	-1	Could not open database file
 */
EXPORT int
tgetent(bp, name)
	char	*bp;
	char	*name;
{
			char	rdbuf[TRDBUF];
			char	*term;
			char	termpath[TMAX];
			char	*tp;
	register	char	*ep;
	register	char	*rbuf = rdbuf;
	register	char	c;
	register	int	count	= 0;
	register	int	tfd;
			int	err = 0;

	tbufsize = TMAX;
	if (tbufmalloc) {
		if (tbuf)
			free(tbuf);
		tbufmalloc = FALSE;
	}
	tbuf = NULL;
	if (name == NULL || *name == '\0') {	/* No TERM name specfied */
		if (bp)
			bp[0] = '\0';
		return (0);
	}
	if ((tbuf = bp) == NULL) {
		tbufmalloc = TRUE;
		tbuf = bp = tmalloc(tbufsize);
	}
	if ((tbuf = bp) == NULL)		/* Could not malloc buffer */
		return (0);
	bp[0] = '\0';		/* Always start with clean termcap buffer */

	/*
	 * First look, if TERMCAP exists
	 */
	if (!(ep = getenv(_Etermcap)) || *ep == '\0') {
		/*
		 * If no TERMCAP environment or empty TERMCAP environment
		 * use default termpath.
		 * Search rules:
		 * 	If TERMPATH exists, use it
		 *	else concat $HOME/ with default termpath
		 *	default termpath is .termcap /etc/termcap
		 */
setpath:
		if ((ep = getenv(_Etermpath)) != NULL) {
			strncpy(termpath, ep, sizeof (termpath));
		} else {
			termpath[0] = '\0';
			if ((ep = getenv("HOME")) != NULL) {
				strncpy(termpath, ep,
					sizeof (termpath)-2-sizeof (_termpath));
				strcat(termpath, "/");
			}
			strcat(termpath, _termpath);
		}
	} else {
		if (*ep != '/') {
			/*
			 * This doesn't seem to be a filename...
			 * It must be a preparsed termcap entry.
			 */
			if (!(term = getenv(_Eterm)) ||
						strcmp(name, term) == 0) {
				/*
				 * If no TERM environment or TERM holds the
				 * same strings as "name" use preparsed entry.
				 */
				tbuf = ep;
				count = tmatch(name);
				tbuf = bp;
				if (count > 0) {
					count = strlen(ep) + 1 + TSIZE_SPACE;
					if (count > tbufsize) {
						if (tbufmalloc) {
							tbufsize = count;
							tbuf = trealloc(bp,
									count);
						} else {
							tbuf = NULL;
							e_tcname(name);
							write(STDERR_FILENO,
							_etoolong,
							sizeof (_etoolong) - 1);
						}
					}
					if (tbuf)
						ovstrcpy(tbuf, ep);
					goto out; /* We use the preparsed entry */
				}
			}
			/*
			 * If the preparsed termcap entry does not match
			 * our term, we need to read the file.
			 * Set up the internal or external TERMPATH for
			 * this purpose.
			 */
			goto setpath;
		}
		/*
		 * If TERMCAP starts with a '/' use it as TERMPATH.
		 */
		strncpy(termpath, ep, sizeof (termpath));
	}
	termpath[sizeof (termpath)-1] = '\0';
	tp = termpath;

nextfile:
	/*
	 * Loop over TERMPATH string.
	 */
	ep = tp;
	while (*tp++) {
		if (*tp == ' ' || *tp == ':') {
			*tp++ = '\0';
			break;
		}
	}
	if (*ep == '\0') {			/* End of TERMPATH */
		if (err != 0)
			return (-1);		/* Signal open error */
		return (0);			/* Signal not found */
	}

	if ((tfd = open(ep, O_RDONLY)) < 0) {
		err = geterrno();

		strncpy(tbuf, ep, TMAX);	/* Remember failed path */
		tbuf[TMAX-1] = 0;
#ifdef	SHOULD_WE
		if (err == ENOENT || err == EACCES)
			goto nextfile;
		return (-1);
#else
		goto nextfile;
#endif
	}

	/*
	 * Search TERM entry in one file.
	 */
	ep = bp;
	for (;;) {
		if (--count <= 0) {
			if ((count = read(tfd, rdbuf, sizeof (rdbuf))) <= 0) {
				close(tfd);
				goto nextfile;	/* Not found, check next */
			}
			rbuf = rdbuf;
		}
		c = *rbuf++;
		if (c == '\n') {
			if (ep > bp && ep[-1] == '\\') {
				ep--;
				continue;
			}
		} else if (ep >= bp + (tbufsize-1)) {
			if (tbufmalloc) {
				tbufsize += TMAX;
				if ((bp = trealloc(bp, tbufsize)) != NULL) {
					ep = bp + (ep - tbuf);
					tbuf = bp;
					*ep++ = c;
					continue;
				} else {	/* No memory for buffer */
					tbuf = NULL;
					goto out;
				}
			}
			e_tcname(name);
			write(STDERR_FILENO, _etoolong, sizeof (_etoolong) - 1);
		} else {
			*ep++ = c;
			continue;
		}
		*ep = '\0';
		if (tmatch(name)) {		/* Entry matches name */
			close(tfd);
			goto out;
		}
		ep = bp;
	}
out:
	count = tchktc(name);
	bp = tbuf;
	if (tbufmalloc) {
		if (count <= 0) {
#ifdef	__free_buffer__				/* Keep buf for failed path */
			if (bp) {
				free(bp);
				tbuf = NULL;
				tbufmalloc = FALSE;
			}
#endif
			return (count);
		}
		/*
		 * Did change size in tchktc() ?
		 */
		count = strlen(bp) + 1;
		if (count != tbufsize) {
			tbufsize = count;
			if ((tbuf = bp = trealloc(bp, tbufsize)) == NULL)
				return (0);
		}
		return (1);
	}
	if (count <= 0)		/* If no match (TERM not found) */
		bp[0] = '\0';	/* clear termcap buffer		*/
	return (count);
}

/*
 * Set the termcap flags.
 * It allows e.g. to prevent tgetent() from following tc= entries
 * and from modifying the co# and li# entries.
 * This is a libxtermcap extension.
 */
EXPORT int
tcsetflags(flags)
	int	flags;
{
	int	oflags = tflags;

	tflags = flags;
	return (oflags);
}

/*
 * Return the current buffer that holds the parsed termcap entry.
 * This function is needed if the buffer is allocated and a user
 * likes to do own string parsing on the buffer.
 * This is a libxtermcap extension.
 */
EXPORT char *
tcgetbuf()
{
	return (tbuf);
}

LOCAL int
tchktc(name)
	char	*name;
{
	register	char	*ep;
	register	char	*np;
	register	char	*tcname;
			char	tcbuf[TMAX];
			char	*otbuf = tbuf;
			int	otbufsize = tbufsize;
			BOOL	otbufmalloc = tbufmalloc;
			BOOL	needfree;
			char	*xtbuf;
			int	ret;

	if (tbuf == NULL)
		return (0);

	ep = tbuf + strlen(tbuf) - 2;
	while (ep > tbuf && *--ep != ':') {
		if (ep <= tbuf) {
			/*
			 * There was no colon in tbuf...tbuf + strlen(tbuf) - 3,
			 * so there cannot be any valid capability in tbuf.
			 * First check for a valid but empty termcap entry,
			 * such as: "tname:"
			 */
			ep = tbuf + strlen(tbuf) - 1;
			if (ep > tbuf && *ep == ':') {
				return (1);	/* Success */
			}
			e_tcname(name);
			write(STDERR_FILENO, _ebad, sizeof (_ebad) - 1);
			return (0);
		}
	}
	ep++;
	if (ep[0] != 't' || ep[1] != 'c' || (tflags & TCF_NO_TC) != 0)
		goto out;

	ep = tfind(tbuf, _tc);
	if (ep == NULL || *ep != '=') {
		e_tcname(name);
		write(STDERR_FILENO, _ebad, sizeof (_ebad) - 1);
		return (0);
	}
	ep -= 2;				/* Correct for propper append */
	strncpy(tcbuf, &ep[2], sizeof (tcbuf));
	tcname = tcbuf;
	tcname[sizeof (tcbuf)-1] = '\0';

	do {
		tcname++;
		for (np = tcname; *np; np++)
			if (*np == ':')
				break;
		*np = '\0';
		if (++loopcount > MAXLOOP) {
			e_tcname(name);
			write(STDERR_FILENO, _eloop, sizeof (_eloop) - 1);
			return (0);
		}
		tbufmalloc = FALSE;		/* Do not free buffer now! */
		ret = tgetent(NULL, tcname);
		*np = ':';
		xtbuf = tbuf;
		needfree = tbufmalloc;
		tbuf = otbuf;
		tbufsize = otbufsize;
		tbufmalloc = otbufmalloc;
		loopcount = 0;
		if (ret != 1) {
			if (needfree && xtbuf != NULL)
				free(xtbuf);
			return (ret);
		}
		np = tskip(xtbuf);	/* skip over the name part */
		/*
		 * Add nullbyte and 14 bytes for the space needed by tgetsize()
		 */
		ret = ep - otbuf + strlen(np) + 1 + TSIZE_SPACE;
		if (ret >= (unsigned)(tbufsize-1)) {
			if (tbufmalloc) {
				tbufsize = ret;
				if ((otbuf =
				    trealloc(otbuf, tbufsize)) != NULL) {
					ep = otbuf + (ep - tbuf);
					tbuf = otbuf;
				} else {
					if (needfree && xtbuf != NULL)
						free(xtbuf);
					return (0);
				}
			} else {
				e_tcname(name);
				write(STDERR_FILENO, _etoolong,
							sizeof (_etoolong) - 1);
				ret = tbufsize - 1 - (ep - otbuf);
				if (ret < 0)
					ret = 0;
				np[ret] = '\0';
			}
		}
		strcpy(ep, np);
		ep += strlen(ep);
		if (needfree && xtbuf != NULL)
			free(xtbuf);

	} while ((tcname = tfind(tcname, _tc)) != NULL && *tcname == '=');
out:
#if	defined(TIOCGSIZE) || defined(TIOCGWINSZ)
	if ((tflags & TCF_NO_SIZE) == 0)
		tgetsize();
#endif
	if ((tflags & TCF_NO_STRIP) == 0)
		tstrip();
	return (1);
}

/*
 * Check if the current 'tbuf' contains a termcap entry for a terminal
 * that matches 'name'.
 */
LOCAL BOOL
tmatch(name)
	char	*name;
{
	register	char	*np;
	register	char	*ep;

	if (tbuf == NULL)
		return (FALSE);

	ep = tbuf;
	if (*ep == '#')					/* Kommentar */
		return (FALSE);
	for (; ; ep++) {
		for (np = name; *np; ep++, np++)	/* Solange name	*/
			if (*ep != *np)			/* gleich ist	*/
				break;
		if (*np == '\0') {			/* Name am Ende */
			if (*ep == '|' || *ep == ':' || *ep == '\0')
				return (TRUE);
		}
		while (*ep && *ep != '|' && *ep != ':')	/* Rest dieses	*/
			ep++;				/* Namens	*/
		if (*ep == ':' || *ep == '\0')
			return (FALSE);
	}
}

/*
 * Skip past next ':'.
 * If the are two consecutive ':', the returned pointer may point to ':'.
 */
LOCAL char *
tskip(ep)
	register	char	*ep;
{
	while (*ep) {
		if (*ep++ == ':')
			return (ep);	/* return first ':'	*/
	}
	return (ep);			/* not found		*/
}

/*
 * Find a two charater entry in string that is found in 'ep'.
 * Return the character that follows the two character entry (if found)
 * or NULL if the entry could not be found.
 */
LOCAL char *
tfind(ep, ent)
	register	char	*ep;
			char	*ent;
{
	register	char	e0 = ent[0];
	register	char	e1 = ent[1];

	for (;;) {
		ep = tskip(ep);
		if (*ep == '\0')
			break;
		if (*ep == ':')
			continue;
		if (e0 != *ep++)
			continue;
		if (*ep == '\0')
			break;
		if (e1 != *ep++)
			continue;
		return (ep);
	}
	return ((char *)NULL);
}

/*
 * Search for a numeric entry in form 'en#123' to represent a decimal number
 * or 'en#0123' to represent a octal number.
 * Return numeric value or -1 if found 'en@'.
 */
EXPORT int
tgetnum(ent)
	char	*ent;
{
	register	Uchar	*ep = (Uchar *)tbuf;
	register	int	val;
	register	int	base;

	if (tbuf == NULL)
		return (-1);

	for (;;) {
		ep = (Uchar *)tfind((char *)ep, ent);
		if (!ep || *ep == '@')
			return (-1);
		if (*ep == '#')
			break;
	}
	base = 10;
	if (*++ep == '0')
		base = 8;
	for (val = 0; isdigit(*ep); ) {
		val *= base;
		val += (*ep++ - '0');
	}
	return (val);
}

/*
 * Search for a boolean entry in form 'en' to represent a TRUE value
 * or 'en@' to represent a FALSE value.
 * An entry in the form 'en@' is mainly used to overwrite similar entries
 * found later from a tc= entry.
 */
EXPORT BOOL
tgetflag(ent)
	char	*ent;
{
	register	char	*ep = tbuf;

	if (tbuf == NULL)
		return (FALSE);

	for (;;) {
		ep = tfind(ep, ent);
		if (!ep || *ep == '@')
			return (FALSE);
		if (*ep == '\0' || *ep == ':')
			return (TRUE);
	}
}

/*
 * Search for a string entry in form 'en=val'.
 * Return string parameter or NULL if found 'en@'.
 */
EXPORT char *
tgetstr(ent, array)
	char	*ent;
	char	*array[];
{
	register	char	*ep = tbuf;
			char	*np = NULL;
			char	*ap = NULL;
			char	buf[TMAX];

	if (tbuf == NULL)
		return ((char *)0);

	if (array == NULL) {
		np = buf;
		array = &np;
	}
	for (;;) {
		ep = tfind(ep, ent);
		if (!ep || *ep == '@')
			return ((char *)NULL);
		if (*ep == '=') {
			if (np && strlen(ep) >= sizeof (buf)) {
				ap = np = tmalloc(strlen(ep));
				if (np == NULL)
					return (np);
				array = &np;
			}
			ep = tdecode(++ep, array);
			if (ep && np) {
				np = ep;
				ep = tmalloc(strlen(ep)+1);
				if (ep != NULL)
					strcpy(ep, np);
				if (ap)
					free(ap);
			}
			return (ep);
		}
	}
}

#define	isoctal(c)	((c) >= '0' && (c) <= '7')
/*
 * Decode a string and replace the escape sequences by what
 * they mean (e.g. \E by ESC).
 * The space used to hold the decoded string is taken from
 * the second parameter.
 * Note that old 'vi' implementations limit the total space for
 * all decoded strings to 256 bytes.
 */
EXPORT char *
tdecode(pp, array)
			char	*pp;
			char	*array[];
{
			int	i;
	register	Uchar	c;
	register	Uchar	*ep = (Uchar *)pp;
	register	Uchar	*bp;
	register	Uchar	*tp;

	bp = (Uchar *)array[0];

	for (; (c = *ep++) && c != ':'; *bp++ = c) {
		if (c == '^') {
			c = *ep++;
			if (c == '\0')
				break;
			else if (c == '?')
				c |= 0x40;
			else
				c &= 0x1f;
			continue;
		} else if (c != '\\') {
			continue;
		}
		/*
		 * Handle the \xxx and \C escape sequences here:
		 */
		c = *ep++;
		if (c == '\0')
			break;
		if (isoctal(c)) {
			for (c -= '0', i = 3; --i > 0 &&
			    *ep && isoctal(*ep); ) {
				c <<= 3;
				c |= *ep++ - '0';
			}
			if (*ep == '\0')
				break;
			/*
			 * Terminfo maps NULL chars to 0200
			 */
			if (c == '\0')
				c = '\200';
		} else for (tp = (Uchar *)_quotetab; *tp; tp++) {
			if (*tp++ == c) {
				c = *tp;
				break;
			}
		}
	}
	*bp++ = '\0';
	ep = (Uchar *)array[0];
	array[0] = (char *)bp;
	return ((char *)ep);
}

#if	defined(TIOCGSIZE) || defined(TIOCGWINSZ)

/*
 * Get the current size of the terminal (window) and insert the
 * apropriate values for 'li#' and 'co#' before the other terminal
 * capabilities.
 */
LOCAL void
tgetsize()
{
#ifdef	TIOCGWINSZ
	struct		winsize ws;
#else
	struct		ttysize	ts;
#endif
	register	int	lines = 0;
	register	int	cols = 0;
	register	char	*ep;
	register	char	*lp;
	register	char	*cp;
			int	len;

	if (tbuf == NULL)
		return;

#ifdef	TIOCGWINSZ
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, (char *)&ws) >= 0) {
		lines = ws.ws_row;
		cols = ws.ws_col;
	}
#else
	if (ioctl(STDOUT_FILENO, TIOCGSIZE, (char *)&ts) >= 0) {
		lines = ts.ts_lines;
		cols = ts.ts_cols;
	}
#endif
	if (lines == 0 || cols == 0 || lines > 999 || cols > 999)
		return;

	len = strlen(tbuf) + 1 + TSIZE_SPACE;
	if (len > tbufsize) {
		if (tbufmalloc) {
			tbufsize = len;
			if ((tbuf = trealloc(tbuf, tbufsize)) == NULL)
				return;
		} else {
			return;
		}
	}
	ep = tskip(tbuf);		/* skip over the name part */
	/*
	 * Backwards copy to create a gap for the string we like to add.
	 */
	lp = &tbuf[len-1-TSIZE_SPACE];	/* The curent end of the buffer */
	for (cp = &lp[TSIZE_SPACE]; lp >= ep; cp--, lp--)
		*cp = *lp;

	*ep++ = 'l';
	*ep++ = 'i';
	*ep++ = '#';
	ep = tinsint(ep, lines);
	*ep++ = ':';
	*ep++ = 'c';
	*ep++ = 'o';
	*ep++ = '#';
	ep = tinsint(ep, cols);
	*ep++ = ':';
	while (ep <= cp)
		*ep++ = ' ';
	*--ep = ':';
}

/*
 * Delete duplicate named numeric entries.
 */
LOCAL void
tdeldup(ent)
			char	*ent;
{
	register	char	*ep;
	register	char	*p;

	if (tbuf == NULL)
		return;

	if ((ep = tfind(tbuf, ent)) != NULL) {
		while ((ep = tfind(ep, ent)) && *ep == '#') {
			p = ep;
			while (*p)
				if (*p++ == ':')
					break;
			ep -= 3;
			ovstrcpy(ep, --p);
		}
	}
}

/*
 * Insert a number into a terminal capability buffer.
 */
LOCAL char *
tinsint(ep, i)
	register	char	*ep;
	register	int	i;
{
	register	char	c;

	if ((c = i / 100) != 0) {
		*ep++ = c + '0';
		i %= 100;
		if (i / 10 == 0)
			*ep++ = '0';
	}
	if ((c = i / 10) != 0)
		*ep++ = c + '0';
	*ep++ = i % 10 + '0';
	return (ep);
}

#endif	/* defined(TIOCGSIZE) || defined(TIOCGWINSZ) */

/*
 * Strip down the termcap entry to make it as short as possible.
 * This is done by first deleting duplicate 'li#' and 'co#' entries
 * and then removing succesive ':' chars and spaces between ':'.
 */
LOCAL void
tstrip()
{
	register	char	*bp = tbuf;
	register	char	*p;

	if (bp == NULL)
		return;

#if	defined(TIOCGSIZE) || defined(TIOCGWINSZ)
	tdeldup("li");
	tdeldup("co");
#endif

#ifdef	needed
	while (*bp) {
		if (*bp++ == ':') {
			if (*bp == ':') {
				p = bp;
				while (*p == ':')
					p++;
				ovstrcpy(bp, p);
			}
		}
	}
	bp = tbuf;
#endif
	while (*bp) {
		if (*bp == '\\') {
			++bp;
			if (*bp++ == '\0')
				break;
			continue;
		}
		if (*bp++ == ':') {
			if (*bp == ':' || *bp == ' ' || *bp == '\t') {
				p = bp;
				while (*p)
					if (*p++ == ':')
						break;
				ovstrcpy(bp--, p);
			}
		}
	}
}

LOCAL char *
tmalloc(size)
	int	size;
{
	char	*ret;

	if ((ret = malloc(size)) != NULL)
		return (ret);
	write(STDERR_FILENO, _enomem, sizeof (_enomem) - 1);
	return ((char *)NULL);
}

LOCAL char *
trealloc(p, size)
	char	*p;
	int	size;
{
	char	*ret;

	if ((ret = realloc(p, size)) != NULL)
		return (ret);
	write(STDERR_FILENO, _enomem, sizeof (_enomem) - 1);
	return ((char *)NULL);
}

#ifdef	NO_LIBSCHILY
/*
 * A strcpy() that works with overlapping buffers
 */
LOCAL char *
ovstrcpy(p2, p1)
	register char		*p2;
	register const char	*p1;
{
	char	*ret = p2;

	while ((*p2++ = *p1++) != '\0')
		;

	return (ret);
}
#endif

LOCAL void
e_tcname(name)
	char	*name;
{
	write(STDERR_FILENO, name, strlen(name));
	write(STDERR_FILENO, ": ", 2);
}
