/* @(#)fprint.c	1.5 17/08/03 Copyright 1985, 1989, 1995-2017 J. Schilling */
/*
 *	Copyright (c) 1985, 1989, 1995-2017 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/unistd.h>	/* include <sys/types.h> try to get size_t */
#include <schily/stdlib.h>	/* Try again for size_t	*/

#include <schily/stdio.h>
#include <schily/varargs.h>
#include <schily/standard.h>
#include <schily/schily.h>

#define	BFSIZ	256

typedef struct {
	short	cnt;
	char	*ptr;
	char	buf[BFSIZ];
	int	count;
	FILE	*f;
} *BUF, _BUF;

LOCAL	void	_bflush	__PR((BUF));
LOCAL	void	_bput	__PR((char, void *));
EXPORT	int	fprintf	__PR((FILE *, const char *, ...)) __printflike__(2, 3);
EXPORT	int	printf	__PR((const char *, ...))	  __printflike__(1, 2);

LOCAL void
_bflush(bp)
	register BUF	bp;
{
	bp->count += bp->ptr - bp->buf;
	if (filewrite(bp->f, bp->buf, bp->ptr - bp->buf) < 0)
		bp->count = EOF;
	bp->ptr = bp->buf;
	bp->cnt = BFSIZ;
}

#ifdef	PROTOTYPES
LOCAL void
_bput(char c, void *l)
#else
LOCAL void
_bput(c, l)
		char	c;
		void	*l;
#endif
{
	register BUF	bp = (BUF)l;

	*bp->ptr++ = c;
	if (--bp->cnt <= 0)
		_bflush(bp);
}

/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT int
printf(const char *form, ...)
#else
EXPORT int
printf(form, va_alist)
	char	*form;
	va_dcl
#endif
{
	va_list	args;
	_BUF	bb;

	bb.ptr = bb.buf;
	bb.cnt = BFSIZ;
	bb.count = 0;
	bb.f = stdout;
#ifdef	PROTOTYPES
	va_start(args, form);
#else
	va_start(args);
#endif
	format(_bput, &bb, form, args);
	va_end(args);
	if (bb.cnt < BFSIZ)
		_bflush(&bb);
	return (bb.count);
}

/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT int
fprintf(FILE *file, const char *form, ...)
#else
EXPORT int
fprintf(file, form, va_alist)
	FILE	*file;
	char	*form;
	va_dcl
#endif
{
	va_list	args;
	_BUF	bb;

	bb.ptr = bb.buf;
	bb.cnt = BFSIZ;
	bb.count = 0;
	bb.f = file;
#ifdef	PROTOTYPES
	va_start(args, form);
#else
	va_start(args);
#endif
	format(_bput, &bb, form, args);
	va_end(args);
	if (bb.cnt < BFSIZ)
		_bflush(&bb);
	return (bb.count);
}
/* -------------------------------------------------------------------------- */

EXPORT	int sprintf __PR((char *, const char *, ...));

#ifdef	PROTOTYPES
static void
_cput(char c, void *ba)
#else
static void
_cput(c, ba)
	char	c;
	void	*ba;
#endif
{
	*(*(char **)ba)++ = c;
}

/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT int
sprintf(char *buf, const char *form, ...)
#else
EXPORT int
sprintf(buf, form, va_alist)
	char	*buf;
	char	*form;
	va_dcl
#endif
{
	va_list	args;
	int	cnt;
	char	*bp = buf;

#ifdef	PROTOTYPES
	va_start(args, form);
#else
	va_start(args);
#endif
	cnt = format(_cput, &bp, form,  args);
	va_end(args);
	*bp = '\0';

	return (cnt);
}

EXPORT	int snprintf __PR((char *, size_t maxcnt, const char *, ...));

typedef struct {
	char	*ptr;
	int	count;
} *SBUF, _SBUF;

#ifdef	PROTOTYPES
static void
_scput(char c, void *l)
#else
static void
_scput(c, l)
	char	c;
	void	*l;
#endif
{
	register SBUF	bp = (SBUF)l;

	if (--bp->count > 0) {
		*bp->ptr++ = c;
	} else {
		/*
		 * Make sure that there will never be a negative overflow.
		 */
		bp->count++;
	}
}

/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT int
snprintf(char *buf, size_t maxcnt, const char *form, ...)
#else
EXPORT int
snprintf(buf, maxcnt, form, va_alist)
	char	*buf;
	unsigned maxcnt;
	char	*form;
	va_dcl
#endif
{
	va_list	args;
	int	cnt;
	_SBUF	bb;

	bb.ptr = buf;
	bb.count = maxcnt;

#ifdef	PROTOTYPES
	va_start(args, form);
#else
	va_start(args);
#endif
	cnt = format(_scput, &bb, form,  args);
	va_end(args);
	if (maxcnt > 0)
		*(bb.ptr) = '\0';
	if (bb.count < 0)
		return (-1);

	return (cnt);
}
