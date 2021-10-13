/* @(#)stdio.h	1.9 13/09/25 Copyright 2006-2013 J. Schilling */
/*
 *	Defines to make FILE * -> int *, used to allow
 *	the Bourne shell to use functions that expect stdio.
 *
 *	Copyright (c) 2006-2013 J. Schilling
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

#ifndef	_STDIO_H
#define	_STDIO_H

#define	_FILE_DEFINED	/* Prevent MSC from redefining FILE in wchar.h */

#undef	FILE
#define	FILE	int
#ifdef	linux
#undef	__FILE
#define	__FILE	int	/* Needed for Linux */
#endif
#define	EOF	(-1)

#define	fflush(a)	(0)
#define	clearerr(a)
#define	ferror(a)	(0)

#define	printf		shell_printf
#define	fprintf		shell_fprintf
#define	sprintf		shell_sprintf
#define	snprintf	shell_snprintf
#define	error		shell_error

#define	js_printf	shell_printf
#define	js_fprintf	shell_fprintf
#define	js_sprintf	shell_sprintf
#define	js_snprintf	shell_snprintf

extern	int	__in__;
extern	int	__out__;
extern	int	__err__;

#define	stdin		(&__in__)
#define	stdout		(&__out__)
#define	stderr		(&__err__)

#if	defined(HAVE_LARGEFILES)
#define	fileopen64	shell_fileopen64
#else
#define	fileopen	shell_fileopen
#endif
#define	fclose		shell_fclose

#define	getc		shell_getc
#define	fgetc		shell_fgetc

#define	putc		shell_putc

#define	filewrite	shell_filewrite

#define	fdown(f)	(*(f))


extern	int	fclose	__PR((FILE *f));
extern	int	getc	__PR((FILE *f));
extern	int	fgetc	__PR((FILE *f));
extern	int	putc	__PR((int c, FILE *f));

#define	expand	bsh_expand
#define	nullstr	bsh_nullstr

#define	prompts		shedit_prompts
#define	prompt		shedit_prompt
#define	delim		shedit_delim
#define	chghistory	shedit_chghistory
#define	remap		shedit_remap
#define	list_map	shedit_list_map
#define	add_map		shedit_add_map
#define	del_map		shedit_del_map

#define	ev_insert	shell_putenv
#define	getcurenv	shell_getenv

#endif /* _STDIO_H */
