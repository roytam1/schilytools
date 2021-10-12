/* @(#)buffer.h	1.16 04/03/12 Copyright 1984, 1988, 1996-2004 J. Schilling */
/*
 *	Definitions for the paged virtual memory susbsystem of ved
 *	The lower level routines are located in buffer.c,
 *	the higher level routines are located in storage.c
 *
 *	Copyright (c) 1984, 1988, 1996-2004 J. Schilling
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

#ifndef	_BUFFER_H
#define	_BUFFER_H

/*
 * The paged virtual memory subsystem is made of a linked list of headers
 * that point to buffers which may be in memory or written out into the
 * backup swap file.
 * To make it easier to find a buffer that may be swapped out, a most recently
 * list is maintained. The least recently buffer is used for swapping out.
 * If the MODIFIED flag is not set, the buffer may simply be thrown away.
 */

#ifndef	BUFFERSIZE
#ifdef	__OLDSIZE
#define	BUFFERSIZE	1024	/* size of a buffer page in virtual memory   */
#else
#define	BUFFERSIZE	8192	/* size of a buffer page in virtual memory   */
#endif
#endif

#if	(BUFFERSIZE > 4096)
#define	MAXBUFFERS	10	/* # of incore buffers in virtual memory    */
#else
#define	MAXBUFFERS	20	/* # of incore buffers in virtual memory    */
#endif

/*
 * Header for a page in the paged virtual memory.
 * One header is allocated for each BUFFERSIZE part of the file.
 * With 1024 bytes/buffer page and 32 bit pointers this uses 32 bytes/header.
 * This is 32768 bytes per MB of the edited file.
 */
typedef	struct _header	headr_t;
struct _header {
	headr_t	*next;		/* next header in the linked list	    */
	headr_t	*prev;		/* previous header in the linked list	    */
	headr_t	*nextr;		/* next header in most recently used list   */
	headr_t	*prevr;		/* previous header in most recently used list */
	Uchar	*buf;		/* pointer to in-memory buffer		    */
	Uchar	*cont;		/* pointer to first valid char in the buffer */
	off_t	fpos;		/* position of the buffer in backup file    */
	short	size;		/* number of valid characters in the buffer */
	short	flags;		/* flags see below			    */
};

/*
 * Flags for each header.
 */
#define	INMEMORY	0x0001	/* The buffer for this header is in memory   */
#define	MODIFIED	0x0002	/* The buffer for this header is modified    */
#define	INVALID		0x0004	/* This header is deleted (no longer valid)  */
#define	ONSWAP		0x0008	/* This header was written to the swapfile   */

/*
 * buffer.c
 */
extern	void	initbuffers	__PR((ewin_t *wp, int nbuf));
extern	void	termbuffers	__PR((ewin_t *wp));
extern	headr_t	*addbuffer	__PR((ewin_t *wp, headr_t *prev));
extern	headr_t	*deletebuffer	__PR((ewin_t *wp, headr_t *linkp));
extern	void	splitbuffer	__PR((ewin_t *wp, headr_t *linkp, int pos));
extern	void	compressbuffer	__PR((ewin_t *wp, headr_t *linkp));
extern	void	readybuffer	__PR((ewin_t *wp, headr_t  *linkp));
extern	void	bufdebug	__PR((ewin_t *wp));

/*
 * storage.c
 */
extern	void	clearifwpos	__PR((ewin_t *wp, headr_t *this));
extern	void	findpos		__PR((ewin_t *wp, epos_t pos, headr_t ** returnlink, int *returnpos));

#endif	/* _BUFFER_H */
