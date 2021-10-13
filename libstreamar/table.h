/* @(#)table.h	1.19 17/02/13 Copyright 1994, 1996, 2000-2017 J. Schilling */
/*
 *	Conversion table definitions for efficient conversion
 *	of different file type representations
 *
 *	Copyright (c) 1994, 1996, 2000-2017 J. Schilling
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

#ifndef	_TABLE_H
#define	_TABLE_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_UTYPES_H
#include <schily/utypes.h>
#endif

/*
 * Unix uses the following file types
 */
#ifdef	comment
		0000000		/* Unallocated			*/
S_IFIFO		0010000	FIFO	/* Named pipe			*/
S_IFCHR		0020000	CHR	/* Character special		*/
S_IFMPC		0030000		/* UNUSED multiplexed c		*/
S_IFDIR		0040000	DIR	/* Directory			*/
S_IFNAM		0050000	NAM	/* Named file (XENIX)		*/
S_IFBLK		0060000	BLK	/* Block special		*/
S_IFMPB		0070000		/* UNUSED multiplexed b		*/
S_IFREG		0100000	REG	/* Regular file 		*/
S_IFCTG		0110000	CTG	/* Contiguous file		*/
S_IFLNK		0120000	SLNK	/* Symbolic link		*/
S_IFSHAD	0130000		/* Solaris shadow inode		*/
S_IFSOCK	0140000	SOCK	/* UNIX domain socket		*/
S_IFDOOR	0150000		/* Solaris DOOR			*/
S_IFWHT		0160000		/* BSD whiteout			*/
		0160200		/* Solaris cpio acl		*/
		0170000		/* UNUSED on UNIX		*/
S_IFEVC		0170000		/* UNOS event count		*/
				/* UNOS/UNIX compat only	*/
#endif

/*
 * Internal table of file types.
 *
 * N.B. The order in this table is not important,
 * new real file types may be added before XT_DUMPDIR,
 * new symbolic file types may be added before XT_BAD.
 */
#define	XT_NONE		0	/* unallocated file			    */
#define	XT_FILE		1	/* regular file				    */
#define	XT_CONT		2	/* contiguous file			    */
#define	XT_LINK		3	/* hard link (needed for internal use)	    */
#define	XT_SLINK	4	/* symbolic link			    */
#define	XT_DIR		5	/* directory				    */
#define	XT_CHR		6	/* character special			    */
#define	XT_BLK		7	/* block special			    */
#define	XT_FIFO		8	/* fifo (named pipe)			    */
#define	XT_SOCK		9	/* unix domain socket			    */
#define	XT_MPC		10	/* multiplexed character special	    */
#define	XT_MPB		11	/* multiplexed block special		    */
#define	XT_NSEM		12	/* XENIX named semaphore		    */
#define	XT_NSHD		13	/* XENIX named shared data		    */
#define	XT_DOOR		14	/* Solaris DOOR				    */
#define	XT_EVENT	15	/* UNOS Event Count			    */
#define	XT_WHT		16	/* BSD whiteout				    */
				/* ... Reserved ...			    */
#define	XT_DUMPDIR	20	/* Dir entry containing filenames	    */
#define	XT_LONGLINK	21	/* Next file on tape has a long link	    */
#define	XT_LONGNAME	22	/* Next file on tape has a long name	    */
#define	XT_MULTIVOL	23	/* Continuation of a file from another tape */
#define	XT_NAMES	24	/* OLD					    */
#define	XT_SPARSE	25	/* for files with holes in it		    */
#define	XT_VOLHDR	26	/* Tape Volume header			    */
#define	XT_META		27	/* Inode meta data only			    */
#define	XT_BAD		31	/* illegal file type			    */

#define	iftoxt_tab	strar_iftoxt_tab
#define	xttoif_tab	strar_xttoif_tab
#define	xttostr_tab	strar_xttostr_tab
#define	xttoname_tab	strar_xttoname_tab
#define	xtnamelen_tab	strar_xtnamelen_tab

extern UInt8_t	iftoxt_tab[];

extern mode_t	xttoif_tab[];

extern char	*xttostr_tab[];
extern char	*xttoname_tab[];
extern UInt8_t	xtnamelen_tab[];

#define	IFTOXT(t)	(iftoxt_tab[((t)&S_IFMT)>>12])	/* UNIX to XT	*/

#define	XTTOIF(t)	(xttoif_tab[(t)])		/* XT to UNIX	*/
#define	XTTOSTR(t)	(xttostr_tab[(t)])		/* XT to string	*/
#define	XTTONAME(t)	(xttoname_tab[(t)])		/* XT to name	*/

#endif	/* _TABLE_H */
