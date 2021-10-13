/* @(#)ctype.c	1.6 09/07/11 Copyright 1986-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)ctype.c	1.6 09/07/11 Copyright 1986-2009 J. Schilling";
#endif
/*
 *	Copyright (c) 1986-2009 J. Schilling
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

#include "ctype.h"

/*	 0	 1	 2	 3	 4	 5	 6	 7	*/

unsigned char	_ctype_a[] = { 0,

/*  0	NUL(@)	SOH(A)	STX(B)	ETX(C)	EOT(D)	ENQ(E)	ACK(F)	BEL(G)	*/
	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,

/*  8	BS(H)	HT(I)	LF(J)	VT(K)	FF(L)	CR(M)	SO(N)	SI(O)	*/
	_CTL,	_TAB,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,

/* 10	DLE(P)	DC1(Q)	DC2(R)	DC3(S)	DC4(T)	NAK(U)	SYN(V)	ETB(W)	*/
	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,

/* 18	CAN(X)	EM(Y)	SUB(Z)	ESC([)	FS(\)	GS(])	RS(^)	US(_)	*/
	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,

/* 20	SP	!	"	#	$	%	&	'	*/
	_WHT,	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,

/* 28	(	)	*	+	,	-	.	/	*/
	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,

/* 30	0	1	2	3	4	5	6	7	*/
	_ODIG,	_ODIG,	_ODIG,	_ODIG,	_ODIG,	_ODIG,	_ODIG,	_ODIG,

/* 38	8	9	:	;	<	=	>	?	*/
	_DIG,	_DIG,	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,

/* 40	@	A	B	C	D	E	F	G	*/
	_PUN,	_UCHEX,	_UCHEX,	_UCHEX,	_UCHEX,	_UCHEX,	_UCHEX,	_UPC,

/* 48	H	I	J	K	L	M	N	O	*/
	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,

/* 50	P	Q	R	S	T	U	V	W	*/
	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,

/* 58	X	Y	Z	[	\	]	^	_	*/
	_UPC,	_UPC,	_UPC,	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,

/* 60	`	a	b	c	d	e	f	g	*/
	_PUN,	_LCHEX,	_LCHEX,	_LCHEX,	_LCHEX,	_LCHEX,	_LCHEX,	_LOWC,

/* 68	h	i	j	k	l	m	n	o	*/
	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,

/* 70	p	q	r	s	t	u	v	w	*/
	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,

/* 78	x	y	z	{	|	}	~	DEL	*/
	_LOWC,	_LOWC,	_LOWC,	_PUN,	_PUN,	_PUN,	_PUN,	_CTL,

/* 80	NUL(�)SOH(�)STX(�)ETX(�)EOT(�)ENQ(�)ACK(�)BEL(�)*/
	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,

/* 88	BS(�)	HT(�)	LF(�)	VT(�)	FF(�)	CR(�)	SO(�)	SI(�)	*/
	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,

/* 90	DLE(�)DC1(�)DC2(�)DC3(�)DC4(�)NAK(�)SYN(�)ETB(�)*/
	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,

/* 98	CAN(�)EM(�)	SUB(�)ESC(�)FS(�)	GS(�)	RS(�)	US(�)	*/
	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,	_CTL,

/* A0	SP	�	�	�	�	�	�	�	*/
	_WHT,	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,

/* A8	�	�	�	�	�	�	�	�	*/
	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,

/* B0	�	�	�	�	�	�	�	�	*/
	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,

/* B8	�	�	�	�	�	�	�	�	*/
	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,	_PUN,

/* C0	�	�	�	�	�	�	�	�	*/
	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,

/* C8	�	�	�	�	�	�	�	�	*/
	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,

/* D0	�	�	�	�	�	�	�	�	*/
	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,

/* D8	�	�	�	�	�	�	�	�	*/
	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,	_UPC,	_LOWC,

/* E0	�	�	�	�	�	�	�	�	*/
	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,

/* E8	�	�	�	�	�	�	�	�	*/
	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,

/* F0	�	�	�	�	�	�	�	�	*/
	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,

/* F8	�	�	�	�	�	�	�	8DEL	*/
	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_LOWC,	_CTL,
};
