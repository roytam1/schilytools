/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */
/*
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * This file contains modifications Copyright 2006-2014 J. Schilling
 *
 * @(#)xcreat.c	1.7 14/03/31 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)xcreat.c 1.7 14/03/31 J. Schilling"
#endif
/*
 * @(#)xcreat.c 1.8 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)xcreat.c"
#pragma ident	"@(#)sccs:lib/mpwlib/xcreat.c"
#endif
# include	<defines.h>
# include       <i18n.h>
# include	<ccstypes.h>
/*
	"Sensible" creat: write permission in directory is required in
	all cases, and created file is guaranteed to have specified mode
	and be owned by effective user.
	(It does this by first unlinking the file to be created.)
	Returns file descriptor on success,
	fatal() on failure.
*/
#ifdef  PROTOTYPES
int
xcreat(char *name, mode_t mode)
#else
int
xcreat(name,mode)
char *name;
mode_t mode;
#endif
{
	register int fd;
	char d[FILESIZE];
	int creat_attempts = 0, errn;
        extern char SccsError[];
	struct stat buf;

	copy(name,d);
	if (!exists(dname(d))) {
		sprintf(SccsError,gettext("directory `%s' nonexistent (ut1)"),
			d);
		fatal(SccsError);
	}
	unlink(name);
	/*
	 * NFS bug work-around. SCCS fails spuriously with error (ut2)
	 * during NSE commands (preserve, reconcile) causing them to fail.
	 * When the mode of the file yields read access only, if the file
	 * is created but the NFS reply packet does not arrive, the 
	 * client retries and, since the file exists and has read access
	 * only, the second 'creat' fails with EACCES. Creating the file
	 * writeable and then chmod'ing it read-only should guarantee
	 * that any 'creat' retries made by the client will succeed. Since
	 * this has been empirically shown not to work, something is
	 * is wrong with this theory. An alternate approach, implemented
	 * below, is to retry the creat.
	 */
	/* Note that this introduces a small window in the file-locking
	 * between when the file is created and when it's chmod'd
	 * read-only.
	 * if ((fd = creat(name, mode | S_IWRITE)) >= 0) {
	 *	if (fchmod(fd, mode) >= 0) {
	 *		return(fd);
	 *	}
	 * }
	 */
	while (creat_attempts < SCCS_CREAT_ATTEMPTS) {
		if ((fd = creat(name, mode)) >= 0) {
			setmode(fd, O_BINARY);
			return(fd);
		/* if it fails due to NFS bug, remove it and retry */
		} else {
			errn = errno;
			if ((errno == EACCES) && (stat(name, &buf) == 0)
			   && (buf.st_size == 0)) {
			unlink(name);
			++creat_attempts;
			/* otherwise return the error */
			} else {
				errno = errn;
				break;
			}
		}
		
	}
	return(xmsg(name,NOGETTEXT("xcreat")));
}
