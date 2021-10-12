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

/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#pragma ident	"@(#)cmd.c	1.17	08/01/29 SMI"

#include "defs.h"

/*
 * This file contains modifications Copyright 2008-2009 J. Schilling
 *
 * @(#)cmd.c	1.11 09/01/10 2008-2009 J. Schilling
 */
#ifndef lint
static	const char sccsid[] =
	"@(#)cmd.c	1.11 09/01/10 2008-2009 J. Schilling";
#endif

/*
 * UNIX shell
 */

#include	"sym.h"

static	unsigned char *getstor	__PR((int asize));
	struct trenod *makefork	__PR((int flgs, struct trenod *i));
static struct trenod *makelist	__PR((int type, struct trenod *i, struct trenod *r));
	struct trenod *cmd	__PR((int sym, int flg));
static	struct trenod *list	__PR((int flg));
static	struct trenod *term	__PR((int flg));
static	struct regnod *syncase	__PR((int esym));
static	struct trenod *item	__PR((BOOL flag));
static	int	skipnl		__PR((void));
static	struct ionod *inout	__PR((struct ionod *lastio));
static	void	chkword		__PR((void));
static	void	chksym		__PR((int sym));
static	void	prsym		__PR((int sym));
static	void	synbad		__PR((void));


/* ======== storage allocation for functions ======== */

static unsigned char *
getstor(asize)
	int asize;
{
	if (fndef)
		return((unsigned char *)alloc(asize));
	else
		return(getstak((Intptr_t)asize));
}


/* ========	command line decoding	========*/




struct trenod *
makefork(flgs, i)
	int	flgs;
	struct trenod *i;
{
	struct forknod *t;

	t = (struct forknod *)getstor(sizeof(struct forknod));
	t->forktyp = flgs|TFORK;
	t->forktre = i;
	t->forkio = 0;
	return((struct trenod *)t);
}

static struct trenod *
makelist(type, i, r)
	int	type;
	struct trenod *i, *r;
{
	struct lstnod *t = 0;

	if (i == 0 || r == 0)
		synbad();
	else
	{
		t = (struct lstnod *)getstor(sizeof(struct lstnod));
		t->lsttyp = type;
		t->lstlef = i;
		t->lstrit = r;
	}
	return((struct trenod *)t);
}

/*
 * cmd
 *	empty
 *	list
 *	list & [ cmd ]
 *	list [ ; cmd ]
 */
struct trenod *
cmd(sym, flg)
	int	sym;
	int		flg;
{
	struct trenod *i, *e;
	i = list(flg);
	if (wdval == NL)
	{
		if (flg & NLFLG)
		{
			wdval = ';';
			chkpr();
		}
	}
	else if (i == 0 && (flg & MTFLG) == 0)
		synbad();

	switch (wdval)
	{
	case '&':
		if (i)
			i = makefork(FAMP, i);
		else
			synbad();

	case ';':
		if ((e = cmd(sym, flg | MTFLG)) != NULL)
			i = makelist(TLST, i, e);
		else if (i == 0)
			synbad();
		break;

	case EOFSYM:
		if (sym == NL)
			break;

	default:
		if (sym)
			chksym(sym);
	}
	return(i);
}

/*
 * list
 *	term
 *	list && term
 *	list || term
 */
static struct trenod *
list(flg)
	int	flg;
{
	struct trenod *r;
	int		b;
	r = term(flg);
	while (r && ((b = (wdval == ANDFSYM)) || wdval == ORFSYM))
		r = makelist((b ? TAND : TORF), r, term(NLFLG));
	return(r);
}

/*
 * term
 *	item
 *	item |^ term
 */
static struct trenod *
term(flg)
	int	flg;
{
	struct trenod *t;

	reserv++;
	if (flg & NLFLG)
		skipnl();
	else
		word();
	if ((t = item(TRUE)) && (wdval == '^' || wdval == '|'))
	{
		struct trenod	*left;
		struct trenod	*right;

		left = makefork(FPOU, t);
		right = makefork(FPIN, term(NLFLG));
		return(makefork(0, makelist(TFIL, left, right)));
	}
	else
		return(t);
}


static struct regnod *
syncase(esym)
int	esym;
{
	skipnl();
	if (wdval == esym)
		return(0);
	else
	{
		struct regnod *r =
		    (struct regnod *)getstor(sizeof (struct regnod));
		struct argnod *argp;

		r->regptr = 0;
		for (;;)
		{
			if (fndef)
			{
				argp= wdarg;
				wdarg = (struct argnod *)alloc(length(argp->argval) + BYTESPERWORD);
				movstr(argp->argval, wdarg->argval);
			}

			wdarg->argnxt = r->regptr;
			r->regptr = wdarg;

			/* 'in' is not a reserved word in this case */
			if (wdval == INSYM){
				wdval = 0;
			}
			if (wdval || (word() != ')' && wdval != '|'))
				synbad();
			if (wdval == '|')
				word();
			else
				break;
		}
		r->regcom = cmd(0, NLFLG | MTFLG);
		if (wdval == ECSYM)
			r->regnxt = syncase(esym);
		else
		{
			chksym(esym);
			r->regnxt = 0;
		}
		return(r);
	}
}

/*
 * item
 *
 *	( cmd ) [ < in  ] [ > out ]
 *	word word* [ < in ] [ > out ]
 *	if ... then ... else ... fi
 *	for ... while ... do ... done
 *	case ... in ... esac
 *	begin ... end
 */
#ifdef	PROTOTYPES
static struct trenod *
item(BOOL flag)
#else
static struct trenod *
item(flag)
	BOOL	flag;
#endif
{
	struct trenod *r;
	struct ionod *io;

	if (flag)
		io = inout((struct ionod *)0);
	else
		io = 0;
	switch (wdval)
	{
	case CASYM:
		{
			struct swnod *t;
			
			t = (struct swnod *)getstor(sizeof(struct swnod));
			r = (struct trenod *)t;

			chkword();
			if (fndef)
				t->swarg = make(wdarg->argval);
			else
				t->swarg = wdarg->argval;
			skipnl();
			chksym(INSYM | BRSYM);
			t->swlst = syncase(wdval == INSYM ? ESSYM : KTSYM);
			t->swtyp = TSW;
			break;
		}

	case IFSYM:
		{
			int	w;
			struct ifnod *t;

			t = (struct ifnod *)getstor(sizeof(struct ifnod));
			r = (struct trenod *)t;

			t->iftyp = TIF;
			t->iftre = cmd(THSYM, NLFLG);
			t->thtre = cmd(ELSYM | FISYM | EFSYM, NLFLG);
			t->eltre = ((w = wdval) == ELSYM ? cmd(FISYM, NLFLG) : (w == EFSYM ? (wdval = IFSYM, item(0)) : 0));
			if (w == EFSYM)
				return(r);
			break;
		}

	case FORSYM:
		{
			struct fornod *t;

			t = (struct fornod *)getstor(sizeof(struct fornod));
			r = (struct trenod *)t;

			t->fortyp = TFOR;
			t->forlst = 0;
			chkword();
			if (fndef)
				t->fornam = make(wdarg->argval);
			else
				t->fornam = wdarg->argval;
			if (skipnl() == INSYM)
			{
				chkword();

				nohash++;
				t->forlst = (struct comnod *)item(0);
				nohash--;

				if (wdval != NL && wdval != ';')
					synbad();
				if (wdval == NL)
					chkpr();
				skipnl();
			}
			chksym(DOSYM | BRSYM);
			t->fortre = cmd(wdval == DOSYM ? ODSYM : KTSYM, NLFLG);
			break;
		}

	case WHSYM:
	case UNSYM:
		{
			struct whnod *t;

			t = (struct whnod *)getstor(sizeof(struct whnod));	
			r = (struct trenod *)t;

			t->whtyp = (wdval == WHSYM ? TWH : TUN);
			t->whtre = cmd(DOSYM, NLFLG);
			t->dotre = cmd(ODSYM, NLFLG);
			break;
		}

	case BRSYM:
		r = cmd(KTSYM, NLFLG);
		break;

	case '(':
		{
			struct parnod *p;

			p = (struct parnod *)getstor(sizeof(struct parnod));
			p->partre = cmd(')', NLFLG);
			p->partyp = TPAR;
			r = makefork(0, (struct trenod *)p);
			break;
		}

	default:
		if (io == 0)
			return(0);

	case 0:
		{
			struct comnod *t;
			struct argnod *argp;
			struct argnod **argtail;
			struct argnod **argset = 0;
			int	keywd = 1;
			unsigned char	*com;

			if ((wdval != NL) && ((peekn = skipwc()) == '('))
			{
				struct fndnod *f;
				struct ionod  *saveio;

				saveio = iotemp;
				peekn = 0;
				if (skipwc() != ')')
					synbad();

				f = (struct fndnod *)getstor(sizeof(struct fndnod));
				r = (struct trenod *)f;

				f->fndtyp = TFND;
				if (fndef)
					f->fndnam = make(wdarg->argval);
				else
					f->fndnam = wdarg->argval;
				reserv++;
				fndef++;
				skipnl();
				f->fndval = (struct trenod *)item(0);
				fndef--;

				if (iotemp != saveio)
				{
					struct ionod 	*ioptr = iotemp;

					while (ioptr->iolst != saveio)
						ioptr = ioptr->iolst;

					ioptr->iolst = fiotemp;
					fiotemp = iotemp;
					iotemp = saveio;
				}
				return(r);
			}
			else
			{
				t = (struct comnod *)getstor(sizeof(struct comnod));
				r = (struct trenod *)t;

				t->comio = io; /*initial io chain*/
				argtail = &(t->comarg);

				while (wdval == 0)
				{
					if (fndef)
					{
						argp = wdarg;
						wdarg = (struct argnod *)alloc(length(argp->argval) + BYTESPERWORD);
						movstr(argp->argval, wdarg->argval);
					}

					argp = wdarg;
					if (wdset && keywd)
					{
						argp->argnxt = (struct argnod *)argset;
						argset = (struct argnod **)argp;
					}
					else
					{
						*argtail = argp;
						argtail = &(argp->argnxt);
						keywd = flags & keyflg;
					}
					word();
					if (flag)
					{
						if (io)
						{
							while(io->ionxt)
								io = io->ionxt;
							io->ionxt = inout((struct ionod *)0);
						}
						else
							t->comio = io = inout((struct ionod *)0);
 					}
				}

				t->comtyp = TCOM;
				t->comset = (struct argnod *)argset;
				*argtail = 0;

				if (nohash == 0 && (fndef == 0 || (flags & hashflg)))
				{
					if (t->comarg)
					{
						com = t->comarg->argval;
						if (*com && *com != DOLLAR)
							pathlook(com, 0, t->comset);
					}
				}

				return(r);
			}
		}

	}
	reserv++;
	word();
	if ((io = inout(io)) != NULL)
	{
		r = makefork(0,r);
		r->treio = io;
	}
	return(r);
}


static int
skipnl()
{
	while ((reserv++, word() == NL))
		chkpr();
	return(wdval);
}

static struct ionod *
inout(lastio)
	struct ionod *lastio;
{
	int	iof;
	struct ionod *iop;
	unsigned int	c;

	iof = wdnum;
	switch (wdval)
	{
	case DOCSYM:	/*	<<	*/
		iof |= IODOC|IODOC_SUBST;
		break;

	case APPSYM:	/*	>>	*/
	case '>':
		if (wdnum == 0)
			iof |= 1;
		iof |= IOPUT;
		if (wdval == APPSYM)
		{
			iof |= IOAPP;
			break;
		}

	case '<':
		if ((c = nextwc()) == '&')
			iof |= IOMOV;
		else if (c == '>')
			iof |= IORDW;
		else
			peekn = c | MARK;
		break;

	default:
		return(lastio);
	}

	chkword();
	iop = (struct ionod *)getstor(sizeof(struct ionod));

	if (fndef)
		iop->ioname = (char *) make(wdarg->argval);
	else
		iop->ioname = (char *) (wdarg->argval);

	iop->iolink = 0;
	iop->iofile = iof;
	if (iof & IODOC)
	{
		iop->iolst = iopend;
		iopend = iop;
	}
	word();
	iop->ionxt = inout(lastio);
	return(iop);
}

static void
chkword()
{
	if (word())
		synbad();
}

static void
chksym(sym)
int sym;
{
	int	x = sym & wdval;

	if (((x & SYMFLG) ? x : sym) != wdval)
		synbad();
}

static void
prsym(sym)
int sym;
{
	if (sym & SYMFLG)
	{
		const struct sysnod *sp = reserved;

		while (sp->sysval && sp->sysval != sym)
			sp++;
		prs((unsigned char *)sp->sysnam);
	}
	else if (sym == EOFSYM)
		prs(_gettext(endoffile));
	else
	{
		if (sym & SYMREP)
			prc(sym);
		if (sym == NL)
			prs(_gettext(nlorsemi));
		else
			prc(sym);
	}
}

static void
synbad()
{
	prp();
	prs(_gettext(synmsg));
	if ((flags & ttyflg) == 0)
	{
		prs(_gettext(atline));
		prn(standin->flin);
	}
	prs((unsigned char *)colon);
	prc(LQ);
	if (wdval)
		prsym(wdval);
	else
		prs_cntl(wdarg->argval);
	prc(RQ);
	prs(_gettext(unexpected));
	newline();
	exitsh(SYNBAD);
}
