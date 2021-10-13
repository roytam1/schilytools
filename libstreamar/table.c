/* @(#)table.c	1.31 17/02/13 Copyright 1994-96 2000-2017 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)table.c	1.31 17/02/13 Copyright 1994-96 2000-2017 J. Schilling";
#endif
/*
 *	Conversion tables for efficient conversion
 *	of different file type representations
 *
 *	Copyright (c) 1994-96 2000-2017 J. Schilling
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
#include <schily/stat.h>
#include "table.h"

#ifndef	S_IFIFO			/* If system knows no fifo's		*/
#define	S_IFIFO		S_IFREG	/* Map fifo's to regular files 		*/
#endif
#ifndef	S_IFCHR			/* If system knows no character special	*/
#define	S_IFCHR		S_IFREG	/* Map character spec. to regular files	*/
#endif
#ifndef	S_IFMPC			/* If system knows no multiplexed c	*/
#define	S_IFMPC		S_IFREG	/* Map multiplexed c to regular files	*/
#endif
#ifndef	S_IFNAM			/* If system knows no named files	*/
#define	S_IFNAM		S_IFREG	/* Map named files to regular files 	*/
#endif
#ifndef	S_IFBLK			/* If system knows no block special	*/
#define	S_IFBLK		S_IFREG	/* Map block spec. to regular files	*/
#endif
#ifndef	S_IFMPB			/* If system knows no multiplexed b	*/
#define	S_IFMPB		S_IFREG	/* Map multiplexed b to regular files	*/
#endif
#ifndef	S_IFCTG			/* If system knows no contiguous files	*/
#define	S_IFCTG		S_IFREG	/* Map contiguous file to regular files */
#endif
#ifndef	S_IFLNK			/* If system knows no symbolic links	*/
#define	S_IFLNK		S_IFREG	/* Map symbolic links to regular files */
#endif
#ifndef	S_IFSOCK		/* If system knows no fifo's		*/
#define	S_IFSOCK	S_IFREG	/* Map fifo's to regular files 		*/
#endif
#ifndef	S_IFDOOR		/* If system knows no door's		*/
#define	S_IFDOOR	S_IFREG	/* Map door's to regular files		*/
#endif
#ifndef	S_IFWHT			/* If system knows no whiteout's	*/
#define	S_IFWHT		S_IFREG	/* Map whiteout's to regular files	*/
#endif
#ifndef	S_IFEVC			/* If system knows no eventcount's	*/
#define	S_IFEVC		S_IFREG	/* Map eventcount's to regular files	*/
#endif
#define	S_IFBAD	S_IFMT		/* XXX Have to use another val if someone */
				/* XXX starts to use S_IFMT for a */
				/* XXX useful file type */
#define	XT_NAM	XT_BAD		/* XXX JS has not seen it yet */
				/* XXX if we use it, we have to change */
				/* XXX table.h and some of the tables below */

/*
 * UNIX File type to XT_ table
 *
 * Maps the 16 UNIX (S_XXX >> 12) filetypes to star's internal XT_ types.
 * Note that this only works if all S_XXX defines are the same on all
 * UNIX versions. Fortunately, this is currently the case and there is no
 * big chance that somebody will do it differently.
 *
 * XXX If really somebody creates a different OS we will need to change this
 * XXX table and the associated macros or make this table dynamically
 * XXX created at startup of star.
 *
 * UNIX file types (see table.h):
 *	0 Unallocated	1 FIFO		2 Chr special	3 MPX chr
 *	4 Directory	5 NAM special	6 BLK special	7 MPX blk
 *	8 Regular File	9 Contig File	10 Symlink	11 Sol Shadow ino
 *	12 Socket	13 DOOR special	14 Whiteout	15 UNOS event count
 *
 * No bound checking in hope that S_IFMT will never hold more than 4 bits.
 */
/* BEGIN CSTYLED */
UInt8_t	iftoxt_tab[] = {
		/* 0 */	XT_NONE, XT_FIFO, XT_CHR,   XT_MPC,
		/* 4 */	XT_DIR,  XT_NAM,  XT_BLK,   XT_MPB,
		/* 8 */	XT_FILE, XT_CONT, XT_SLINK, XT_BAD,
		/*12 */	XT_SOCK, XT_DOOR, XT_WHT,   XT_BAD,
			};
/* END CSTYLED */

/*
 * XT_* codes used (see table.h):
 *				 0..16	Real file types and hard links
 *				20..26	Speudo file types (POSIX 'A' .. 'Z'
 *				31	XT_BAD (illegal file type)
 */

/*
 * XT_ to UNIX File type table
 *
 * XT_SPARSE and XT_META are just other (tar specific) views of regular files.
 */
/* BEGIN CSTYLED */
mode_t	xttoif_tab[] = {
		/* 0 */	0,       S_IFREG,  S_IFCTG, S_IFREG,
		/* 4 */	S_IFLNK, S_IFDIR,  S_IFCHR, S_IFBLK,
		/* 8 */	S_IFIFO, S_IFSOCK, S_IFMPC, S_IFMPB,
		/*12 */	S_IFNAM, S_IFNAM,  S_IFDOOR, S_IFEVC,
		/*16 */	S_IFWHT, S_IFBAD,  S_IFBAD, S_IFBAD,
		/*20 */	S_IFDIR, S_IFBAD,  S_IFBAD, S_IFBAD,
		/*24 */	S_IFBAD, S_IFREG,  S_IFBAD, S_IFREG,
		/*28 */	S_IFBAD, S_IFBAD,  S_IFBAD, S_IFBAD,
};
/* END CSTYLED */

/*
 * XT_ to String table
 */
/* BEGIN CSTYLED */
char	*xttostr_tab[] = {
#define	XT_DEBUG
#ifdef	XT_DEBUG
		/* 0 */	"U",	"-",	"C",	"H",
#else
		/* 0 */	"-",	"-",	"-",	"-",
#endif
		/* 4 */	"l",	"d",	"c",	"b",
		/* 8 */	"p",	"s",	"~",	"~",
		/*12 */	"~",	"~",	"D",	"~",
		/*16 */	"%",	"~",	"~",	"~",

		/*20 */	"D",	"K",	"L",	"M",
#ifdef	XT_DEBUG
		/*24 */	"N",	"S",	"V",	"m",
#else
		/*24 */	"N",	"-",	"V",	"-",
#endif

		/*28 */	"~",	"~",	"~",	"~",
};
/* END CSTYLED */

/*
 * XT_ to named file type text
 */
/* BEGIN CSTYLED */
char	*xttoname_tab[] = {
		/* 0 */	"unallocated",	"regular",	"contiguous",		"hardlink",
		/* 4 */	"symlink",	"directory",	"character special",	"block special",
		/* 8 */	"fifo",		"socket",	"mpx character special", "mpx block special",
		/*12 */	"XENIX nsem",	"XENIX nshd",	"door",		        "eventcount",
		/*16 */	"whiteout",	"reserved",	"reserved",		"reserved",
		/*20 */	"dumpdir",	"longlink",	"longname",		"multivol continuation",
		/*24 */	"names",	"sparse",	"volheader",		"meta",
		/*28 */	"reserved",	"reserved",	"reserved",		"unknown/bad",
};
/* END CSTYLED */

/*
 * String lengths for above table
 */
UInt8_t	xtnamelen_tab[] = {
		/*  0 */	11,  7, 10,  8,
		/*  4 */	 7,  9, 17, 13,
		/*  8 */	 4,  6, 21, 17,
		/* 12 */	10, 10,  4, 10,
		/* 16 */	 8,  8,  8,  8,
		/* 20 */	 7,  8,  8, 21,
		/* 24 */	 5,  6,  9,  4,
		/* 28 */	 8,  8,  8, 11,
};
