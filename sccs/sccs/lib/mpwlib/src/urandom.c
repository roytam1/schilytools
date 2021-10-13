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
 *	Unified randomm number generator
 *
 *	The algorithm in use is able to work for more than 500000 years
 *	with 64 bits of "urand" data.
 *
 *	The low 20 bits give some sort of randomness (usually derived from
 *	miocroseconds) that is sufficient for our purpose, the upper bits are
 *	derived from the current time and help to make clashes less probable.
 *	Unified randomm number are used to create unique names for removed
 *	files.
 *
 *	We asume that any platform that does not implement a 64 bit
 *	scalar type will be unable to work past 2038 Jan 19 03:14:07 GMT.
 *	For this reason, it is sufficient to support 52 bits for such systems.
 *
 * @(#)urandom.c	1.1 11/10/29 Copyright 2011 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)urandom.c	1.1 11/10/29 Copyright 2011 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)urandom.c"
#pragma ident	"@(#)sccs:lib/mpwlib/urandom.c"
#endif
#include	<defines.h>

LOCAL void	rtime	__PR((struct timeval *tvp));
#ifndef	HAVE_LONG_LONG
LOCAL void	tv2urand __PR((struct timeval *tvp, urand_t *urp));
LOCAL void	urand2tv __PR((urand_t *urp, struct timeval *tvp));
#endif

LOCAL struct timeval	otv;

EXPORT int
urandom(urp)
	urand_t	*urp;
{
	struct timeval	tv;

	/*
	 * XXX Here we need to get the last used urand value in order to make
	 * XXX sure that we will not suffer from turning the clock back.
	 */
	rtime(&tv);
	if (tv.tv_sec < otv.tv_sec) {
		fprintf(stderr, gettext("Clock going backwards\n"));
	}
	if (tv.tv_usec == otv.tv_usec) {
		tv.tv_usec++;
		timerfix(&tv);
	}
	otv = tv;
	tv.tv_sec -= 0x40000000;
#ifdef	HAVE_LONG_LONG
	*urp = tv.tv_sec * 1000000LL + tv.tv_usec;
#else
	tv2urand(&tv, urp);
#endif
	return (0);
}

LOCAL	int	have_usec;

LOCAL void
rtime(tvp)
	struct timeval	*tvp;
{
	struct timeval	tv2;
	int		i;
	unsigned int	u;

	gettimeofday(tvp, 0);
	if (have_usec > 0)
		return;

	if (tvp->tv_usec == 0 && have_usec == 0) {
		for (i = 0; i < 1000; i++) {
			gettimeofday(&tv2, 0);
			if (tvp->tv_usec != tv2.tv_usec)
				break;
		}
		if (tvp->tv_usec != tv2.tv_usec) {
			have_usec = 1;
			return;
		}
		have_usec = -1;
		/*
		 * Initialize the seed for rand()
		 */
		u = (unsigned)getpid();
		u *= tvp->tv_sec;
		srand(u);
	}
	tvp->tv_usec = rand() % 1000000;
}

#ifndef	HAVE_LONG_LONG
/*
 * The next two funtions work correclty for an unsigned int up to 52 bits.
 * This is sufficient for any date before 2038 Jan 19 03:14:08 GMT.
 */
LOCAL void
tv2urand(tvp, urp)
	struct timeval	*tvp;
	urand_t		*urp;
{
	unsigned int	low;
	unsigned int	high;
	unsigned int	u;
	unsigned int	u2;

	u = tvp->tv_sec;

	low = (u & 0xFF) * 1000000;
	u >>= 8;
	high = u * 1000000;
	low += high << 8;
	high >>= 24;

	u2 = (u>>4) * (1000000>>4);
	u2 >>= 16;
	u2 &= ~0xFF;
	high += u2;

	u2 = (u>>8) * (1000000>>8);
	u2 >>= 8;
	u2 &= ~0xFFFF;
	high += u2;

	low += tvp->tv_usec;

	urp->low = low;
	urp->high = high;
}

LOCAL void
urand2tv(urp, tvp)
	urand_t		*urp;
	struct timeval	*tvp;
{
	unsigned int	low;
	unsigned int	high;
	unsigned int	u;
	unsigned int	u2;

	low = urp->low;
	high = urp->high;

	high <<= 12;
	high |= low >> 20;

	low &= 0xFFFFF;		/* 20 Bits bleiben */

	u = high / 1000000;
	high -= u * 1000000;

	u2 = u << 20;


	high <<= 12;
	high |= low >> 8;

	low &= 0xFF;		/* 8 Bits bleiben */

	u = high / 1000000;
	high -= u * 1000000;

	u2 |= u << 8;


	high <<= 8;
	high |= low;

	u = high / 1000000;
	high -= u * 1000000;

	u2 |= u;

	tvp->tv_sec = u2;
	tvp->tv_usec = high;
}
#endif
