/* @(#)unicode.c	1.23 20/07/03 Copyright 2001-2020 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)unicode.c	1.23 20/07/03 Copyright 2001-2020 J. Schilling";
#endif
/*
 *	Routines to convert from/to UNICODE
 *
 *	This is currently a very simple implementation that only
 *	handles ISO-8859-1 coding using intrinsic code and using
 *	iconv() in case of other encodings.
 *
 *	Copyright (c) 2001-2020 J. Schilling
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
#include <schily/types.h>
#include <schily/utypes.h>
#include <schily/iconv.h>
#include <schily/standard.h>
#include <schily/errno.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#ifdef	__STAR__
#include "star.h"
#include "starsubs.h"
#include "checkerr.h"
#else
#include "header.h"
#endif

EXPORT	void	utf8_init	__PR((int type));
EXPORT	void	utf8_fini	__PR((void));
EXPORT	size_t	to_utf8		__PR((Uchar *to, size_t tolen,
					Uchar *from, size_t len));
LOCAL	size_t	_to_utf8	__PR((Uchar *to, size_t tolen,
					Uchar *from, size_t len));
#ifdef	USE_ICONV
LOCAL	size_t	_to_iconv	__PR((Uchar *to, size_t tolen,
					Uchar *from, size_t len));
#endif
LOCAL	size_t	_to_none	__PR((Uchar *to, size_t tolen,
					Uchar *from, size_t len));
EXPORT	BOOL	from_utf8	__PR((Uchar *to, size_t tolen,
					Uchar *from, size_t *len));
LOCAL	BOOL	_from_utf8	__PR((Uchar *to, size_t tolen,
					Uchar *from, size_t *len));
#ifdef	USE_ICONV
LOCAL	BOOL	_from_iconv	__PR((Uchar *to, size_t tolen,
					Uchar *from, size_t *len));
#endif
LOCAL	BOOL	_from_none	__PR((Uchar *to, size_t tolen,
					Uchar *from, size_t *len));

LOCAL	size_t	(*p_to_utf8)	__PR((Uchar *to, size_t tolen,
					Uchar *from, size_t len)) = _to_utf8;
LOCAL	BOOL	(*p_from_utf8)	__PR((Uchar *to, size_t tolen,
					Uchar *from, size_t *len)) = _from_utf8;

LOCAL	iconv_t	ic_from	= (iconv_t)-1;
LOCAL	iconv_t	ic_to	= (iconv_t)-1;

#ifdef	__STAR__
extern	char		*codeset;
#else
LOCAL	const char	*codeset;

EXPORT void
utf8_codeset(code_set)
	const	char	*code_set;
{
	codeset = code_set;
}
#endif


EXPORT void
utf8_init(type)
	int	type;
{
	if (codeset == NULL)
		codeset = "ISO8859-1";
#ifndef	ICONV_DEBUG
	if (streql(codeset, "ISO8859-1") ||
	    streql(codeset, "ISO-8859-1") ||
	    streql(codeset, "ISO8859_1") ||
	    streql(codeset, "ISO_8859_1") ||
	    streql(codeset, "8859-1") ||
	    streql(codeset, "8859_1")) {
		p_to_utf8 = _to_utf8;
		p_from_utf8 = _from_utf8;
		return;
	}
	if (streql(codeset, "UTF-8") ||
	    streql(codeset, "UTF8") ||
	    streql(codeset, "UTF_8")) {
		p_to_utf8 = _to_none;
		p_from_utf8 = _from_none;
		return;
	}
#endif
	if (type & S_CREATE) {
#ifdef	USE_ICONV
		if (ic_to != (iconv_t)-1) {
			iconv_close(ic_to);
		}
		ic_to = iconv_open("UTF-8", codeset);
#ifdef	ICONV_DEBUG
		fprintf(stderr, "ic_to %p\n", ic_to);
#endif
		if (ic_to != (iconv_t)-1)
			p_to_utf8 = _to_iconv;
		else
#endif
			p_to_utf8 = _to_utf8;
	}
	if (type & S_EXTRACT) {
#ifdef	USE_ICONV
		if (ic_from != (iconv_t)-1) {
			iconv_close(ic_from);
		}
		ic_from = iconv_open(codeset, "UTF-8");
#ifdef	ICONV_DEBUG
		fprintf(stderr, "ic_from %p\n", ic_from);
#endif
		if (ic_from != (iconv_t)-1)
			p_from_utf8 = _from_iconv;
		else
#endif
			p_from_utf8 = _from_utf8;
	}
}

EXPORT void
utf8_fini()
{
#ifdef	USE_ICONV
	if (ic_to != (iconv_t)-1) {
		iconv_close(ic_to);
		ic_to = (iconv_t)-1;
	}
	if (ic_from != (iconv_t)-1) {
		iconv_close(ic_from);
		ic_from = (iconv_t)-1;
	}
#endif
}

EXPORT size_t
to_utf8(to, tolen, from, len)
	register Uchar	*to;
		size_t	tolen;
	register Uchar	*from;
	register size_t	len;
{
	return (p_to_utf8(to, tolen, from, len));
}

/*
 * First copy len bytes from the source, convert it to UTF-8 assuming that it
 * is in ISO-8859-1 encoding. Then add a final null byte. Return the number of
 * characters written to the destination excluding the final null byte
 * (strlen(to)).
 */
LOCAL size_t
_to_utf8(to, tolen, from, len)
	register Uchar	*to;
		size_t	tolen;
	register Uchar	*from;
	register size_t	len;
{
	register Uchar	*oto = to;
	register Uchar	c;

	if (len == 0)
		goto out;

	do {
		c = *from++;
		if (c <= 0x7F) {
			*to++ = c;
		} else if (c <= 0xBF) {
			*to++ = 0xC2;
			*to++ = c;
		} else { /* c <= 0xFF */
			*to++ = 0xC3;
			*to++ = c & 0xBF;
		}
		/*
		 * XXX We have plenty of space in "to" when we are called.
		 * XXX Should we check wether we did hit "tolen"?
		 */
	} while (--len > 0);
out:
	*to = '\0';
	return (to - oto);
}

#ifdef	USE_ICONV
LOCAL size_t
_to_iconv(to, tolen, from, len)
	Uchar	*to;
	size_t	tolen;
	Uchar	*from;
	size_t	len;
{
#ifdef	HAVE_ICONV_CONST
	const char	*fp = (char *)from;
#else
	char		*fp = (char *)from;
#endif
	char		*tp = (char *)to;
	size_t		frl = len;
	size_t		tol = tolen;
	size_t		ret;

	seterrno(0);
	ret = iconv(ic_to, &fp, &frl, &tp, &tol);
	if (tol > 0)
		*tp = '\0';
	if (ret != 0) {	/* Error (-1) or nonidentical translations (>0) */
#ifdef	__STAR__
		if (!errhidden(E_ICONV, (char *)from)) {
			if (!errwarnonly(E_ICONV, (char *)from))
				xstats.s_iconv++;
#endif
			errmsg("Cannot convert '%s' to UTF-8.\n", from);
#ifdef	__STAR__
			(void) errabort(E_ICONV, (char *)from, TRUE);
		}
#endif
	}
	/*
	 * Reset shift state
	 */
	(void) iconv(ic_to, NULL, NULL, NULL, NULL);
	return (tolen - tol);
}
#endif

LOCAL size_t
_to_none(to, tolen, from, len)
	Uchar	*to;
	size_t	tolen;
	Uchar	*from;
	size_t	len;
{
	if (tolen < len) {
		movebytes(from, to, tolen);
		return (tolen);
	}
	*movebytes(from, to, len) = '\0';
	return (len);
}

EXPORT BOOL
from_utf8(to, tolen, from, lenp)
	Uchar	*to;
	size_t	tolen;
	Uchar	*from;
	size_t	*lenp;
{
	return (p_from_utf8(to, tolen, from, lenp));
}

/*
 * First copy len bytes from the source and convert it from UTF-8 assuming
 * ISO-8859-1 encoding. Then add a final null byte. Set *lenp to the number of
 * bytes written to the destination excluding the final null byte (strlen(to)).
 * Return FALSE in case that an illegal ISO-8859-1 character was seen in the
 * UTF-8 stream.
 */
LOCAL BOOL
_from_utf8(to, tolen, from, lenp)
	register Uchar	*to;
		size_t	tolen;
	register Uchar	*from;
		size_t	*lenp;
{
	register Uchar	*oto = to;
	register Uchar	c;
	register BOOL	ret = TRUE;
	register size_t	len = *lenp;
		Uchar	*endp = to + tolen;

	if (len == 0)
		goto out;

	do {
		c = *from++;
		if (c <= 0x7F) {
			*to++ = c;
		} else if (c == 0xC0) {
			*to++ = *from++ & 0x7F;
			if (--len == 0)
				break;
		} else if (c == 0xC1) {
			*to++ = (*from++ | 0x40) & 0x7F;
			if (--len == 0)
				break;
		} else if (c == 0xC2) {
			*to++ = *from++;
			if (--len == 0)
				break;
		} else if (c == 0xC3) {
			*to++ = *from++ | 0x40;
			if (--len == 0)
				break;
		} else {
			ret = FALSE;		/* unknown/illegal UTF-8 char */
			*to++ = '_';		/* use default character    */
			if (c < 0xE0) {
				from++;		/* 2 bytes in total */
				if (--len == 0)
					break;
			} else if (c < 0xF0) {
				from += 2;	/* 3 bytes in total */
				if (len <= 2)
					break;
				len -= 2;
			} else if (c < 0xF8) {
				from += 3;	/* 4 bytes in total */
				if (len <= 3)
					break;
				len -= 3;
			} else if (c < 0xFC) {
				from += 4;	/* 5 bytes in total */
				if (len <= 4)
					break;
				len -= 4;
			} else if (c < 0xFE) {
				from += 5;	/* 6 bytes in total */
				if (len <= 5)
					break;
				len -= 5;
			} else {
				while (len > 0) {
					c = *from;
					/*
					 * Test for 7 bit ASCII + non prefix
					 */
					if (c <= 0xBF)
						break;
					from++;
					if (--len == 0)
						break;
				}
				if (len == 0)
					break;
			}
		}
		/*
		 * It is easy to check, since the result is always only one
		 * character. We need to stop here since the new path handling
		 * may need to grow the result in case of an overflow.
		 */
		if (to >= endp)
			break;
	} while (--len > 0);
out:
	if (to < endp)
		*to = '\0';
	*lenp = (to - oto);
	return (ret);
}

#ifdef	USE_ICONV
LOCAL BOOL
_from_iconv(to, tolen, from, len)
	Uchar	*to;
	size_t	tolen;
	Uchar	*from;
	size_t	*len;
{
#ifdef	HAVE_ICONV_CONST
	const char	*fp = (char *)from;
#else
	char		*fp = (char *)from;
#endif
	char		*tp = (char *)to;
	size_t		frl = *len;
	size_t		tol = tolen;
	size_t		ret;
	BOOL		rc = TRUE;

	seterrno(0);
	ret = iconv(ic_from, &fp, &frl, &tp, &tol);
	if (tol > 0)
		*tp = '\0';
	*len = tolen - tol;
	if (ret == -1 && geterrno() == E2BIG) {
		/*
		 * in case of an overflow signal this via *len,
		 * even if on Linux where "tol" is 0 in such a case.
		 */
		*len = tolen;
		rc = FALSE;
	} else if (ret != 0) {	/* -1 or # of nonidentical translations (>0) */
#ifdef	__STAR__
		if (!errhidden(E_ICONV, (char *)from)) {
			if (!errwarnonly(E_ICONV, (char *)from))
				xstats.s_iconv++;
#endif
			errmsg("Cannot convert '%s' to local charset.\n", from);
#ifdef	__STAR__
			(void) errabort(E_ICONV, (char *)from, TRUE);
		}
#endif
		rc = FALSE;
	}
	/*
	 * Reset shift state
	 */
	(void) iconv(ic_from, NULL, NULL, NULL, NULL);
	return (rc);
}
#endif

LOCAL BOOL
_from_none(to, tolen, from, len)
	Uchar	*to;
	size_t	tolen;
	Uchar	*from;
	size_t	*len;
{
	size_t	clen = *len;

	if (tolen < clen) {
		movebytes(from, to, tolen);
		*len = tolen;
		return (TRUE);
	}
	*movebytes(from, to, clen) = '\0';
	return (TRUE);
}
