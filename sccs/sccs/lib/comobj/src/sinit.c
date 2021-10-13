/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may use this file only in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
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
 * Copyright 2006-2020 J. Schilling
 *
 * @(#)sinit.c	1.15 20/07/27 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)sinit.c 1.15 20/07/27 J. Schilling"
#endif
/*
 * @(#)sinit.c 1.7 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)sinit.c"
#pragma ident	"@(#)sccs:lib/comobj/sinit.c"
#endif
# include	<defines.h>

static	void	dummy_enter	__PR((struct packet *pkt, int ch, int n, struct sid *sidp));
static	void	dummy_escdodelt	__PR((struct packet *pkt));
static	void	dummy_fredck	__PR((struct packet *pkt));

/*
	Does initialization for sccs files and packet.
*/

void
sinit(pkt,file,openflag)
register struct packet *pkt;
register char *file;
int openflag;
{
	register char *p;

#if	defined(IS_MACOS_X)
/*
 * The Mac OS X static linker is too silly to link in .o files from static libs
 * if only a variable is referenced. The elegant workaround for this bug (using
 * common variables) triggers a different bug in the dynamic linker from Mac OS
 * that is unable to link common variables. This forces us to introduce funcs
 * that need to be called from central places to enforce to link in the vars.
 */
extern	void __comobj __PR((void));

	__comobj();
#endif
	zero((char *)pkt, sizeof(*pkt));
	if (size(file) > FILESIZE)
		fatal(gettext("too long (co7)"));
	if ((openflag & SI_FORCE) == 0) {
		if (!sccsfile(file))
			fatal(gettext("not an SCCS file (co1)"));
	}
	copy(file,pkt->p_file);
	pkt->p_wrttn = 1;
	pkt->do_chksum = 1;	/* turn on checksum check for getline */
	if (openflag & SI_OPEN) {
		pkt->p_iop = xfopen(file, O_RDONLY|O_BINARY);
#ifdef	USE_SETVBUF
		setvbuf(pkt->p_iop, NULL, _IOFBF, VBUF_SIZE);
#else
		setbuf(pkt->p_iop, pkt->p_buf);
#endif
		fstat((int)fileno(pkt->p_iop),&Statbuf);
		if (Statbuf.st_nlink > 1)
			fatal(gettext("more than one link (co3)"));

#ifdef	HAVE_MMAP
		pkt->p_mmsize = Statbuf.st_size;
		if ((openflag & SI_NOMAP) == 0 &&
		    pkt->p_mmsize < (1024*1024*1024)) {
			/*
			 * Do not try to mmap() huge files.
			 * If we ever allow to mmap() files > 2G, we still need
			 * to prevent this for 32 bit binaries.
			 */
			pkt->p_mmbase = mmap((void *)0, pkt->p_mmsize,
					PROT_READ|PROT_WRITE, MAP_PRIVATE,
					fileno(pkt->p_iop),
					(off_t)0);
		}
		if (pkt->p_mmbase == MAP_FAILED) {
			/*
			 * Silently fall back to the read method.
			 */
			pkt->p_mmbase = NULL;
		} else if (pkt->p_mmbase) {
			pkt->p_mmnext = pkt->p_mmbase;
			pkt->p_mmend = pkt->p_mmbase + pkt->p_mmsize;
		}
#endif
		p = getline(pkt);
		if (p == NULL || (p = checkmagic(pkt, p)) == NULL) {
			sclose(pkt);
			fmterr(pkt);
		}
		p = satoi(p, &pkt->p_ihash);
	}
	pkt->p_chash = 0;
	pkt->p_uchash = 0;

	pkt->p_enter = dummy_enter;
	pkt->p_escdodelt = dummy_escdodelt;
	pkt->p_fredck = dummy_fredck;
}

char *
checkmagic(pkt, p)
	register struct packet	*pkt;
	register char		*p;
{
	register int	i;
	register int	c;

	if (*p++ != CTLCHAR || *p++ != HEAD)
		return (NULL);

	if (strncmp(p, "V6,sum=", 7) == 0) {
		pkt->p_flags |= PF_V6;
		p += 7;
	}
	for (i = 5; --i >= 0; ) {
		c = *p++ - '0';
		if (c < 0 || c > 9)
			return (NULL);
	}
	if (*p == '\n')
		return (&p[-5]);
	if ((pkt->p_flags & PF_V6) && *p == ',')
		return (&p[-5]);
	return (NULL);
}

/*ARGSUSED*/
static void
dummy_enter(pkt, ch, n, sidp)
	struct packet *pkt;
	char		ch;
	int		n;
	struct sid	*sidp;
{
}

/*
 * Null routine to satisfy external reference from dodelt()
 */
/*ARGSUSED*/
static void
dummy_escdodelt(pkt)
	struct packet *pkt;
{
}

/*
 * NULL routine to satisfy external reference from dodelt()
 */
/*ARGSUSED*/
static void
dummy_fredck(pkt)
	struct packet *pkt;
{
}
