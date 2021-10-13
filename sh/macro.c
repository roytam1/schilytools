/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * A copy of the CDDL is also available via the Internet at
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
/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#if defined(sun)
#pragma ident	"@(#)macro.c	1.16	06/10/09 SMI"
#endif

#include "defs.h"

/*
 * Copyright 2008-2021 J. Schilling
 *
 * @(#)macro.c	1.102 21/07/21 2008-2021 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)macro.c	1.102 21/07/21 2008-2021 J. Schilling";
#endif

/*
 * UNIX shell
 */

#ifdef	SCHILY_INCLUDES
#include	"sym.h"
#include	<schily/param.h>
#include	<schily/wait.h>
#else
#include	"sym.h"
#include	<sys/param.h>
#include	<wait.h>
#endif

#ifndef	MAXPATHLEN
#define	MAXPATHLEN	1024
#endif

#define	no_pipe	(int *)0

		int	macflag;
static unsigned char	quote;	/* used locally */
static unsigned char	quoted;	/* used locally */

#define	COM_BACKQUOTE	0	/* `command`  type command substitution */
#define	COM_DOL_PAREN	1	/* $(command) type command substitution */

static void	copyto		__PR((unsigned char endch, int trimflag));
static void	skipto		__PR((unsigned char endch));
#ifdef	DO_DOT_SH_PARAMS
static unsigned char *shvar	__PR((unsigned char *v));
#endif
static unsigned int dolname	__PR((unsigned char **argpp,
					unsigned int c, unsigned int addc));
static int	getch		__PR((unsigned char endch, int trimflag));
	unsigned char *macro	__PR((unsigned char *as));
	unsigned char *_macro	__PR((unsigned char *as));
static void	comsubst	__PR((int, int));
	void	subst		__PR((int in, int ot));
static void	flush		__PR((int));

#ifdef	DO_SUBSTRING
static int	mbschars	__PR((unsigned char *v));
static unsigned char	*prefsubstr __PR((unsigned char *v, unsigned char *pat,
					int largest));
static unsigned char	*mbdecr	__PR((unsigned char *s, unsigned char *sp));
static int		suffsubstr __PR((unsigned char *v, unsigned char *pat,
					int largest));
#ifdef	__needed__
static Uchar	*globesc	__PR((Uchar *argp));
#endif
#endif
static void	sizecpy		__PR((int vsize, Uchar *v, int trimflag));
static Uchar	*trimcpy	__PR((Uchar *argp, int trimflag));

#ifdef	PROTOTYPES
static void
copyto(unsigned char endch, int trimflag)
#else
static void
copyto(endch, trimflag)
	unsigned char	endch;
	int		trimflag;
#endif
/* trimflag - flag to check if argument will be trimmed */
{
	unsigned int	c;
	unsigned int	d;
	unsigned char *pc;

	while ((c = getch(endch, trimflag)) != endch && c) {
		if (quote) {
			if (c == '\\') { /* don't interpret next character */
				GROWSTAKTOP();
				pushstak(c);
				d = readwc();
				if (!escchar(d)) {
					/*
					 * both \ and following
					 * character are quoted if next
					 * character is not $, `, ", or \
					 */
					GROWSTAKTOP();
					pushstak('\\');
					GROWSTAKTOP();
					pushstak('\\');
				}
				pc = readw(d);
				/*
				 * d might be NULL
				 * Even if d is NULL, we have to save it
				 */
				if (*pc) {
					while (*pc) {
						GROWSTAKTOP();
						pushstak(*pc++);
					}
				} else {
					GROWSTAKTOP();
					pushstak(*pc);
				}
			} else { /* push escapes onto stack to quote chars */
				pc = readw(c);
				GROWSTAKTOP();
				pushstak('\\');
				while (*pc) {
					GROWSTAKTOP();
					pushstak(*pc++);
				}
			}
		} else if (c == '\\') {
			c = readwc(); /* get character to be escaped */
			GROWSTAKTOP();
			pushstak('\\');
			pc = readw(c);
			/* c might be NULL */
			/* Evenif c is NULL, we have to save it */
			if (*pc) {
				while (*pc) {
					GROWSTAKTOP();
					pushstak(*pc++);
				}
			} else {
				GROWSTAKTOP();
				pushstak(*pc);
			}
		} else {
			pc = readw(c);
			while (*pc) {
				GROWSTAKTOP();
				pushstak(*pc++);
			}
		}
	}
	GROWSTAKTOP();
	zerostak();	/* Add nul byte */
	if (c != endch)
		error(badsub);
}

#ifdef	PROTOTYPES
static void
skipto(unsigned char endch)
#else
static void
skipto(endch)
	unsigned char	endch;
#endif
{
	/*
	 * skip chars up to }
	 */
	unsigned int	c;

	while ((c = readwc()) != '\0' && c != endch) {
		switch (c) {
		case SQUOTE:
			skipto(SQUOTE);
			break;

		case DQUOTE:
			skipto(DQUOTE);
			break;

		case DOLLAR:
			if ((c = readwc()) == BRACE)
				skipto('}');
			else if (c == SQUOTE || c == DQUOTE)
				skipto(c);
			else if (c == 0)
				goto out;
			break;

		case ESCAPE:
			if (!(c = readwc()))
				goto out;
		}
	}
out:
	if (c != endch)
		error(badsub);
}

/*
 * Expand special shell variables ${.sh.xxx}.
 */
#ifdef	DO_DOT_SH_PARAMS
static unsigned char *
shvar(v)
	unsigned char	*v;
{
	if (eq(v, "status")) {			/* exit status */
		sitos(retex.ex_status);
		v = numbuf;
	} else if (eq(v, "termsig")) {		/* kill signame */
		numbuf[0] = '\0';
		sig2str(retex.ex_status, (char *)numbuf);
		v = numbuf;
		if (numbuf[0] == '\0')
			strcpy((char *)numbuf, "UNKNOWN");
	} else if (eq(v, "code")) {		/* exit code (reason) */
		itos(retex.ex_code);
		v = numbuf;
	} else if (eq(v, "codename")) {		/* text for above */
		v = UC code2str(retex.ex_code);
	} else if (eq(v, "pid")) {		/* exited pid */
		itos(retex.ex_pid);
		v = numbuf;
	} else if (eq(v, "signo")) {		/* SIGCHLD or trapsig */
		itos(retex.ex_signo);
		v = numbuf;
	} else if (eq(v, "signame")) {		/* text for above */
		sig2str(retex.ex_signo, (char *)numbuf);
		v = numbuf;
	} else if (eq(v, "shell")) {		/* Shell implementation name */
		v = UC shname;
	} else if (eq(v, "version")) {		/* Shell version */
		v = UC shvers;
	} else if (eq(v, "path")) {		/* Shell path */
		static unsigned char *shpath = NULL;
		char		pbuf[MAXPATHLEN + 1];
		unsigned char	*pname;
#ifdef	HAVE_GETEXECNAME
		const char	*exname = getexecname();
#else
			char	*exname = getexecpath();
#endif
		if (shpath)
			return (shpath);
		if (exname == NULL)
			return (NULL);
		if (*exname == '/') {
			pname = UC exname;
		} else {
#ifdef	HAVE_REALPATH
			pname = UC realpath(exname, pbuf);
#else
			pname = UC abspath(exname, pbuf, sizeof (pbuf));
#endif
		}
		if (pname) {
			/*
			 * NUMBUFLEN is sufficient in most cases.
			 * We cannot do this on stak as we need to
			 * keep the current stak segment open.
			 */
			if (length(pname) > NUMBUFLEN) {
				shpath = pname = make(pname);
			} else {
				movstr(pname, numbuf);
				return (numbuf);
			}
		}
#ifndef	HAVE_GETEXECNAME
		libc_free(exname);
#endif
		v = UC pname;
	} else {
		return (NULL);
	}
	return (v);
}
#endif

/*
 * Collect the parameter name.
 * If "addc" is null, collect a normal parameter name,
 * else "addc" is an additional permitted character.
 * This is typically '.' for the ".sh.xxx" parameters.
 * Returns the first non-matching character to allow the rest
 * of the parser to recover.
 */
static unsigned int
dolname(argpp, c, addc)
	unsigned char	**argpp;
	unsigned int	c;
	unsigned int	addc;
{
	unsigned char	*argp;

	argp = (unsigned char *)relstak();
	while (alphanum(c) || (addc && c == addc)) {
		GROWSTAKTOP();
		pushstak(c);
		c = readwc();
	}
	GROWSTAKTOP();
	zerostak();
	*argpp = argp;		/* Return start offset against stakbot */
	return (c);
}

#ifdef	PROTOTYPES
static int
getch(unsigned char endch, int trimflag)
#else
static int
getch(endch, trimflag)
	unsigned char	endch;
	/*
	 * flag to check if an argument is going to be trimmed, here document
	 * output is never trimmed
	 */
	int	trimflag;
#endif
{
	unsigned int	d;
	/*
	 * atflag to check if $@ has already been seen within double quotes
	 */
	int atflag = 0;
retry:
	d = readwc();
	if (!subchar(d))
		return (d);

	if (d == DOLLAR) {
		unsigned int c;

		if ((c = readwc(), dolchar(c))) {
			struct namnod *n = (struct namnod *)NIL;
			int		dolg = 0;
#ifdef	DO_U_DOLAT_NOFAIL
			int		isg = 0;
#endif
#ifdef	DO_SUBSTRING
			int		largest = 0;
#endif
			int		vsize = -1;
			BOOL		bra;
			BOOL		nulflg;
			unsigned char	*argp, *v = NULL;
			unsigned char		idb[2];
			unsigned char		*id = idb;
			unsigned char	oquote;

			*id = '\0';

			if ((bra = (c == BRACE)) != FALSE)
				c = readwc();
#ifdef	DO_SUBSTRING
getname:
#endif
			if (letter(c)) {	/* valid parameter name */

				c = dolname(&argp, c, 0);
				n = lookup(absstak(argp));
				setstak(argp);
#ifndef	DO_POSIX_UNSET
				if (n->namflg & N_FUNCTN) {
					error(badsub);
					return (EOF);
				}
#endif
#ifdef	DO_LINENO
				if (n == &linenonod) {
					v = linenoval();
				} else
#endif
					v = n->namval;
				id = (unsigned char *)n->namid;
				peekc = c | MARK;
			} else if (digchar(c)) {	/* astchar or digits */
				*id = c;
				idb[1] = 0;
				if (astchar(c))	{	/* '*' or '@' */
					if (c == '@' && !atflag && quote) {
						quoted--;
						atflag = 1;
					}
#ifdef	DO_U_DOLAT_NOFAIL
					isg = dolg = 1;
#else
					dolg = 1;
#endif
					c = 1;
				} else if (digit(c)) {
					c -= '0';
#ifdef	DO_POSIX_PARAM
					if (bra) {
						int	dd;
						int	overflow = 0;
						int	maxmult = INT_MAX / 10;

						while ((dd = readwc(),
							digit(dd))) {
							dd -= '0';
							if (c > maxmult)
								overflow = 1;
							c *= 10;
							if (INT_MAX - c < dd)
								overflow = 1;
							c += dd;
						}
						peekc = dd | MARK;
						if (overflow)
							c = INT_MAX;
					}
#endif
				}
				/*
				 * Double cast is needed to work around a
				 * GCC bug.
				 */
				v = ((c == 0) ?
					cmdadr :
					(dolc > 0 && c <= dolc) ?
					dolv[c] :
					(unsigned char *)(Intptr_t)(dolg = 0));
			} else if (c == '$') {
				v = pidadr;
			} else if (c == '!') {
				v = pcsadr;
			} else if (c == '#') {
#ifdef	DO_SUBSTRING
				if (bra == 1) {
					c = readwc();
					if (c == ':' ||
					    c == '-' ||
					    c == '+' ||
					    c == '?' ||
					    c == '=') {
						/*
						 * Check for corner case ${#?}
						 */
						if (c == '?') {
							int nc = readwc();

							peekc = nc|MARK;
							if (nc == '}') {
								bra++;
								goto getname;
							}
						}
						itos(dolc);
						v = numbuf;
						goto docolon;
					} else if (c != '}') {
						bra++;
						goto getname;
					} else {
						bra = 0;
					}
				}
#endif
				itos(dolc);
				v = numbuf;
			} else if (c == '?') {
#ifdef	DO_SIGNED_EXIT
				sitos(retval);
#else
				itos(retval);
#endif
				v = numbuf;
#ifdef	DO_DOL_SLASH
			} else if (c == '/') {
				if (retex.ex_code == CLD_EXITED) {
					sitos(retex.ex_status);
					v = numbuf;
				} else if (retex.ex_code == C_NOEXEC ||
					    retex.ex_code == C_NOTFOUND) {
					v = UC code2str(retex.ex_code);
				} else {
					sig2str(retex.ex_status,
						    (char *)numbuf);
					v = numbuf;
				}
#endif
			} else if (c == '-') {
				v = flagadr;
#ifdef	DO_DOT_SH_PARAMS
			} else if (bra && c == '.') {
				unsigned char	*shv;

				c = dolname(&argp, c, '.');
				v = absstak(argp);	/* Variable name */
				if (v[0] == '.' &&
				    v[1] == 's' &&
				    v[2] == 'h' &&
				    v[3] == '.' &&
					(shv = shvar(&v[4])) != NULL) {
					v = shv;
				} else {
					v = NULL;
				}
				setstak(argp);
				peekc = c | MARK;
#endif
			} else if (bra) {
				error(badsub);
				return (EOF);
			} else {
				goto retry;
			}
			c = readwc();
#ifdef	DO_SUBSTRING
docolon:
#endif
			if (c == ':' && bra == 1) { /* null and unset fix */
				nulflg = 1;
				c = readwc();	/* c now holds char past ':' */
			} else {
				nulflg = 0;
			}
			if (!defchar(c) && bra) {	/* check "}=-+?#%" */
				error(badsub);
				return (EOF);
			}
			argp = 0;
			if (bra) {		/* ${...} */
				/*
				 * This place is probably the wrong place to
				 * mark the word as expanded, but before, we
				 * did not mark a substitution to word in
				 * ${var-word} if "var" is unset.
				 */
				macflag |= M_PARM;

				if (c != '}') {	/* word follows parameter */
					/*
					 * Collect word or null string depending
					 * on parameter value and setchar(c).
					 */
					argp = (unsigned char *)relstak();
					if ((v == 0 ||
					    (nulflg && *v == 0)) ^
					    (setchar(c))) {
						int	ntrim = trimflag;

						oquote = quote;
#ifdef	DO_SUBSTRING
						if (c == '#' || c == '%') {
							unsigned int	nc;

#ifdef	__needed__
							/*
							 * See globesc() below.
							 */
							ntrim = 0;
#endif
							nc = readwc();
							if (nc == c) {
								largest++;
							} else {
								peekc = nc|MARK;
							}
							quote = 0;
						} else {
							unsigned int	nc;

							nc = readwc();
							if (nc == DQUOTE)
								quote = 0;
							peekc = nc|MARK;
						}
#endif
						copyto('}', ntrim);
						quote = oquote;
					} else {
						skipto('}');
					}
					argp = absstak(argp);
				}
			} else {
				peekc = c | MARK;
				c = 0;
			}
#ifdef	DO_SUBSTRING
			if (bra > 1) {
				int	l = 0;

				if (c != '}' || argp) {
					error(badsub);
					return (EOF);
				}
				if (v)
					l = mbschars(v);
				itos(l);
				v = numbuf;
			}
			if ((c == '#' || c == '%')) {
				if (v) {
					UIntptr_t	b = relstakp(argp);

					if (dolg) {
						error(badsub);
						return (EOF);
					}
#ifdef	__needed__
					if (quote) {
						/*
						 * This is a copy that we may
						 * shrink.
						 */
						trim(argp);
					}
#endif

					/*
					 * Treat double quotes in glob pattern.
					 */
#ifdef	__needed__
					/*
					 * See ntrim = 0 above.
					 */
					argp = globesc(argp);
#endif
					if (c == '#') {
						v = prefsubstr(v, argp,
								largest);
					} else {
						vsize = suffsubstr(v, argp,
								largest);
					}
					setstak(b);
				} else {
					/*
					 * Clear to let it fail later with
					 * an unset error with set -u.
					 */
					argp = 0;
				}
			}
#endif
			if (v && (!nulflg || *v)) {
				/*
				 * Parameter is not unset and
				 * either non-null or the ':' is not present.
				 */

				if (c != '+') {
					Uchar sep = ' ';

#ifdef	DO_IFS_SEP
					if (ifsnod.namval)
						sep = *ifsnod.namval;
#endif
					/*
					 * Substitute parameter value
					 */
					(void) mbtowc(NULL, NULL, 0);
					for (;;) {
						if (*v == 0 && quote) {
							GROWSTAKTOP();
							pushstak('\\');
							GROWSTAKTOP();
							pushstak('\0');
						} else {
							macflag |= M_PARM;
							sizecpy(vsize, v,
								trimflag);
						}

						if (dolg == 0 ||
						    (++dolg > dolc)) {
							break;
						} else {
							/*
							 * $* and $@ expansion
							 */
							v = dolv[dolg];
							if (*id == '*' &&
							    quote) {
								/*
								 * push quoted
								 * separator so
								 * that "$*"
								 * will not be
								 * broken into
								 * separate
								 * arguments.
								 */
								if (!sep)
								    continue;
								GROWSTAKTOP();
								pushstak('\\');
								GROWSTAKTOP();
								pushstak(sep);
							} else {
								if (*id == '@')
								    macflag |=
									M_DOLAT;
								GROWSTAKTOP();
								pushstak(' ');
							}
						}
					}
				}
			} else if (argp) {
				if (c == '?') {
					if (trimflag)
						trim(argp);
					failed(id, *argp ? (const char *)argp :
					    badparam);
					return (EOF);
				} else if (c == '=') {
					if (n) {
						int slength = staktop - stakbot;
						UIntptr_t aoff = argp - stakbot;
						unsigned char *savp = fixstak();
						struct ionod *iosav = iotemp;
						unsigned char *newargp;

						/*
						 * The malloc()-based stak.c
						 * will relocate the last item
						 * if we call fixstak();
						 */
						argp = savp + aoff;

						/*
						 * copy word onto stack, trim
						 * it, and then do assignment
						 */
						usestak();
						argp = trimcpy(argp, trimflag);
						newargp = fixstak();
						assign(n, newargp);
						tdystak(savp, iosav);
						(void) memcpystak(stakbot,
								    savp,
								    slength);
						staktop = stakbot + slength;
					} else {
						error(badsub);
						return (EOF);
					}
				}
#ifdef	DO_U_DOLAT_NOFAIL
			} else if ((flags & setflg) && isg == 0) {
#else
			} else if (flags & setflg) {
#endif
				failed(id, unset);
				return (EOF);
			}
			goto retry;
#ifdef		DO_DOL_PAREN
		} else if (c == '(' && (macflag & M_NOCOMSUBST) == 0) {
			comsubst(trimflag, COM_DOL_PAREN);
			goto retry;
#endif
		} else {
			peekc = c | MARK;
		}
	} else if (d == endch) {
		return (d);
	} else if (d == SQUOTE) {
		if (macflag & M_NOCOMSUBST)
			return (d);
		comsubst(trimflag, COM_BACKQUOTE);
		goto retry;
	} else if (d == DQUOTE && trimflag) {
		if (!quote) {
			atflag = 0;
			quoted++;
		}
		quote ^= QUOTE;
		goto retry;
	}
	return (d);
}

unsigned char *
macro(as)
	unsigned char	*as;
{
	macflag &= ~M_SPLIT;
	(void) _macro(as);
	return (fixstak());
}

unsigned char *
_macro(as)
	unsigned char	*as;
{
	/*
	 * Strip "" and do $ substitution
	 * Leaves result on top of stack
	 */
	BOOL	savqu = quoted;
	unsigned char	savq = quote;
	UIntptr_t	b = relstak();
	struct filehdr	fb;

	fb.fsiz = 1;	/* It's a filehdr not a fileblk */
	push((struct fileblk *)&fb);
	estabf(as);
	usestak();
	quote = 0;
	quoted = 0;
	copyto(0, 1);
	pop();
	if (quoted && (stakbot == staktop)) {
		GROWSTAKTOP();
		pushstak('\\');
		GROWSTAKTOP();
		pushstak('\0');
		zerostak();
/*
 * above is the fix for *'.c' bug
 */
	}
	quote = savq;
	quoted = savqu;
	return (absstak(b));
}
/* Save file descriptor for command substitution */
int savpipe = -1;

static void
comsubst(trimflag, type)
	int	trimflag;
	int	type;
/* trimflag - used to determine if argument will later be trimmed */
{
	/*
	 * command substn
	 */
	struct fileblk	cb;
	unsigned int	d;
	int strlngth = staktop - stakbot;
	unsigned char *oldstaktop;
	unsigned char *savptr = fixstak();
	struct ionod *iosav = iotemp;
	struct ionod *fiosav = fiotemp;
	int		oiof = 0;
	int		ofiof = 0;
	unsigned char	*pc;
	struct trenod	*tc = NULL;
	int		omacflag = macflag;

	if (iosav) {
		oiof = iosav->iofile;
		iosav->iofile |= IOBARRIER;
	}
	if (fiosav) {
		ofiof = fiosav->iofile;
		fiosav->iofile |= IOBARRIER;
	}

	if (type == COM_BACKQUOTE) {  /* `command`  type command substitution */

	usestak();
	while ((d = readwc()) != SQUOTE && d) {
		if (d == '\\') {
			d = readwc();
			if (!escchar(d) || (d == '"' && !quote)) {
				/*
				 * trim quotes for `, \, or " if
				 * command substitution is within
				 * double quotes
				 */
				GROWSTAKTOP();
				pushstak('\\');
			}
		}
		pc = readw(d);
		/* d might be NULL */
		if (*pc) {
			while (*pc) {
				GROWSTAKTOP();
				pushstak(*pc++);
			}
		} else {
			GROWSTAKTOP();
			pushstak(*pc);
		}
	}
	{
		unsigned char	*argc;

		argc = fixstak();
		push(&cb);
		estabf(argc);  /* read from string */
		tc = cmd(EOFSYM, MTFLG | NLFLG | SEMIFLG);
	}
	}
#ifdef	DO_DOL_PAREN
	else {	/* $(command) type command substitution */
		d = readwc();
		if (d == '(') {		/* This is an arithmetic expression */
			Intmax_t	i;
			unsigned char	*argc;
			struct argnod	*arg = (struct argnod *)locstak();
			unsigned char	*argp = arg->argval;
			int		err;

			/*
			 * savptr holds strlngth bytes in a saved area
			 * containing the already parsed part of the string.
			 * Use match_arith() to find the end of the expresssion
			 * and call macro() with this string to expand
			 * variables and embedded command substitutions.
			 * NOTE: match_arith() expects the string to be part
			 * of struct argnod, so argp must not start at stakbot.
			 */
			*argp = '\0';
			staktop = match_arith(argp);
			arg = (struct argnod *)stakbot;
			argc = staktop-1;
			argp = arg->argval;
			while (argc > argp && *(--argc) != ')')
				;
			*argc = 0;
			arg = (struct argnod *)fixstak();
			argc = arg->argval;
			argc = macro(&argc[2]);
			(void) memcpystak(stakbot, savptr, strlngth);
			staktop = stakbot + strlngth;
			/*
			 * First implementation: just return the expanded string
			 */
			i = strexpr(argc, &err);
			if (err == 0) {
				staktop = movstrstak(&numbuf[slltos(i)],
								staktop);
			} else {
				exitval = err;
			}
			macflag = omacflag | M_ARITH;
			if (iosav)
				iosav->iofile = oiof;
			if (fiosav)
				fiosav->iofile = ofiof;
			return;
		}
		peekc = d | MARK;
		tc = cmd(')', MTFLG | NLFLG | SEMIFLG);
	}
#endif
	{
		struct trenod	*t;
		int		pv[2];
#ifdef	DO_POSIX_E
		int		oret = retval;
		struct excode	oex;

		oex = retex;
#endif

		/*
		 * this is done like this so that the pipe
		 * is open only when needed
		 */
		t = makefork(FPOU|STDOUT_FILENO, tc);
		chkpipe(pv);
		savpipe = pv[OTPIPE];
#ifdef	DO_DOL_PAREN
		if (type != COM_BACKQUOTE) {
			push(&cb);
			estabf(NULL);
		}
#endif
		initf(pv[INPIPE]); /* read from pipe */
#ifdef	PARSE_DEBUG
		prtree(t, "Command Substitution: ");
#endif
		/*
		 * The SVr4 version of the shell did not allocate a job
		 * slot when XEC_NOSTOP was specified. Since we use vfork()
		 * and optiomized pipes (-DDO_PIPE_PARENT) we also need to
		 * specify XEC_ALLOCJOB to avoid that we overwrite the
		 * existing job slot with command substitution.
		 */
		execute(t, XEC_NOSTOP|XEC_ALLOCJOB, (int)(flags & errflg),
			no_pipe, pv);
#ifdef	DO_POSIX_E
		retval = oret;	/* Restore old retval for $? */
		retex = oex;
#endif
		close(pv[OTPIPE]);
		savpipe = -1;
	}
#if 0
	/*
	 * Do we really need to call this here?
	 * execute() calls it already and if we leave this call here, we may
	 * end up with accessing already free()d memory later.
	 *
	 * Calling tdystak() would free only the space that was allocated by
	 * the parser (cmd()) and this seems to be of limited size. Given that
	 * comsubst() is called as part of another command execution, it seems
	 * that the lifetime of that space is not significantly enhanced.
	 */
	tdystak(savptr, iosav);
#endif
	(void) memcpystak(stakbot, savptr, strlngth);
	oldstaktop = staktop = stakbot + strlngth;
	while ((d = readwc()) != '\0') {
		if (quote || (d == '\\' && trimflag)) {
			unsigned char *rest;
			/*
			 * quote output from command subst. if within double
			 * quotes or backslash part of output
			 */
			rest = readw(d);
			GROWSTAKTOP();
			pushstak('\\');
			while ((d = *rest++) != '\0') {
			/* Pick up all of multibyte character */
				GROWSTAKTOP();
				pushstak(d);
			}
		} else {
			pc = readw(d);
			while (*pc) {
				GROWSTAKTOP();
				pushstak(*pc++);
			}
		}
	}
	{
		extern pid_t parent;
		int	rc;
		int	ret = 0;
		int	wstatus;
		int	wcode;

		while ((ret = wait_status(parent,
				&wcode, &wstatus,
				(WEXITED|WTRAPPED))) != parent) {
			/* break out if waitid(2) has failed */
			if (ret == -1)
				break;
		}
		rc = wstatus;
		if ((flags2 & fullexitcodeflg) == 0)
			rc &= 0xFF; /* As dumb as with historic wait */
		if (wcode == CLD_KILLED || wcode == CLD_DUMPED)
			rc |= SIGFLG;
#ifdef	DO_EXIT_MODFIX
		else if (wstatus != 0 && rc == 0)
			rc = SIGFLG;	/* Use special value 128 */
#endif
#ifndef	DO_POSIX_E
		if (rc && (flags & errflg))
			exitsh(rc);
#endif
		exitval = rc;
		ex.ex_status = wstatus;
		ex.ex_code = wcode;
		ex.ex_pid = parent;
		flags |= eflag;
#ifndef	DO_POSIX_E
		exitset();	/* Set retval from exitval for $? */
#endif
	}
	if (iosav)
		iosav->iofile = oiof;
	if (fiosav)
		fiosav->iofile = ofiof;
	while (oldstaktop != staktop) {
		/*
		 * strip off trailing newlines from command substitution only
		 */
		if ((*--staktop) != NL) {
			++staktop;
			break;
		} else if (quote)
			staktop--; /* skip past backslashes if quoting */
	}
	pop();

	macflag = omacflag | M_COMMAND;
}

#define	CPYSIZ	512

void
subst(in, ot)
	int	in;
	int	ot;
{
	unsigned int	c;
	struct fileblk	fb;
	int	count = CPYSIZ;
	unsigned char	*pc;
#ifdef	DO_POSIX_HERE
	int	oquote = quote;
#endif

	push(&fb);
	initf(in);
	/*
	 * DQUOTE used to stop it from quoting
	 */
#ifdef	DO_POSIX_HERE
	quote = 0;
#endif
	while ((c = (getch(DQUOTE, 0))) != '\0') {
		/*
		 * read characters from here document and interpret them
		 */
		if (c == '\\') {
			c = readwc();
			/*
			 * check if character in here document is escaped
			 */
			if (!escchar(c) || c == '"') {
				GROWSTAKTOP();
				pushstak('\\');
			}
		}
		pc = readw(c);
		/* c might be NULL */
		if (*pc) {
			while (*pc) {
				GROWSTAKTOP();
				pushstak(*pc++);
			}
		} else {
			GROWSTAKTOP();
			pushstak(*pc);
		}
		if (--count == 0) {
			flush(ot);
			count = CPYSIZ;
		}
	}
#ifdef	DO_POSIX_HERE
	quote = oquote;
#endif
	flush(ot);
	pop();
}

static void
flush(ot)
	int	ot;
{
	write(ot, stakbot, staktop - stakbot);
	if (flags & execpr)
		write(output, stakbot, staktop - stakbot);
	staktop = stakbot;
}

#ifdef	DO_SUBSTRING
static int
mbschars(v)
	unsigned char	*v;
{
	register unsigned char	*s = v;
		wchar_t		wc;
		int		len;
		int		chars = 0;

	while (*s) {
		if ((len = mbtowc(&wc, (char *)s, MB_LEN_MAX)) <= 0) {
			(void) mbtowc(NULL, NULL, 0);
			len = 1;
		}
		s += len;
		chars++;
	}
	return (chars);
}

/*
 * Prefix substring matcher for ${var#pat} and ${var##pat}
 *
 * Returns pointer to first non-matching character in the string.
 */
static unsigned char *
prefsubstr(v, pat, largest)
	unsigned char	*v;		/* The data value to check	*/
	unsigned char	*pat;		/* The pattern to match against */
	int		largest;	/* Whether to match largest str	*/
{
	register unsigned char	*s = v;
	register unsigned char	*r = v;
	register unsigned int	c;
		wchar_t		wc;
		int		len;

	do {
		c = *s;
		*s = '\0';
		if (gmatch(C v, C pat)) {
			if (!largest) {
				*s = c;
				return (s);
			}
			r = s;
		}
		*s = c;

		if ((len = mbtowc(&wc, (char *)s, MB_LEN_MAX)) <= 0) {
			(void) mbtowc(NULL, NULL, 0);
			len = 1;
		}
		s += len;
	} while (c);
	return (r);
}

/*
 * Return a pointer to the last multi byte character before "sp".
 * Currently always called after gmatch() and thus from an intact mbstate.
 */
static unsigned char *
mbdecr(s, sp)
	unsigned char	*s;
	unsigned char	*sp;
{
	wchar_t		wc;
	int		len;

	while (s < sp) {
		if ((len = mbtowc(&wc, (char *)s, MB_LEN_MAX)) <= 0) {
			(void) mbtowc(NULL, NULL, 0);
			len = 1;
		}
		if ((s + len) >= sp)
			return (s);
		s += len;
	}
	return (sp-1);
}

/*
 * Suffix substring matcher for ${var%pat} and ${var%%pat}
 *
 * Returns size of non-matching initial part in the string.
 */
static int
suffsubstr(v, pat, largest)
	unsigned char	*v;		/* The data value to check	*/
	unsigned char	*pat;		/* The pattern to match against */
	int		largest;	/* Whether to match largest str	*/
{
	register unsigned char	*s = v;
	register int		size = strlen(C v);

	s += size;
	while (s >= v) {
		if (gmatch(C s, C pat)) {
			size = s - v;
			if (!largest)
				break;
		}
		s = mbdecr(v, s);
	}
	return (size);
}

/*
 * Convert prefix and suffix pattern into something that is
 * accepted by glob().
 */
#ifdef	__needed__
static Uchar *
globesc(argp)
	Uchar	*argp;
{
	int		c;
	int		escflag = FALSE;
	UIntptr_t	b = relstak();

	pushstak('\0');			/* Terminate current argp */
	(void) mbtowc(NULL, NULL, 0);
	while ((c = *argp) != '\0') {
		wchar_t		wc;
		int		len;

		if ((len = mbtowc(&wc, C argp, MB_LEN_MAX)) <= 0) {
			(void) mbtowc(NULL, NULL, 0);
			len = 1;
		}
		if (c == '"') {
			escflag = !escflag;
			argp += len;
			continue;
		}
		if (escflag) {
			switch (c) {

			case '\\':
			case '*':
			case '?':
			case '[':
				GROWSTAKTOP();
				pushstak('\\');
				break;
			default:
				;
			}
		}
		while (len-- > 0) {
			GROWSTAKTOP();
			pushstak(*argp++);
		}
	}
	zerostak();
	staktop = (absstak(b));
	return (&staktop[1]);	/* Point past first added null byte */
}
#endif
#endif

/*
 * If vsize is >= 0, copy vsize characters, else copy all.
 * The mbstate is reset from our caller, thus we do not need to
 * call mbtowc(NULL, NULL, 0)
 */
static void
sizecpy(vsize, v, trimflag)
	int	vsize;
	Uchar	*v;
	int	trimflag;
{
	int	c;

	while (vsize && (c = *v) != '\0') {
		wchar_t	wc;
		int	clength;

		if ((clength = mbtowc(&wc, (char *)v, MB_LEN_MAX)) <= 0) {
			(void) mbtowc(NULL, NULL, 0);
			clength = 1;
		}
		if (quote || (c == '\\' && trimflag)) {
			GROWSTAKTOP();
			pushstak('\\');
		}
		while (clength-- > 0) {
			GROWSTAKTOP();
			pushstak(*v++);
		}
		if (vsize > 0)
			vsize--;
	}
}

static Uchar *
trimcpy(argp, trimflag)
	Uchar	*argp;
	int	trimflag;
{
	int	c;

	(void) mbtowc(NULL, NULL, 0);
	while ((c = *argp) != '\0') {
		wchar_t		wc;
		int		len;

		if ((len = mbtowc(&wc, C argp, MB_LEN_MAX)) <= 0) {
			(void) mbtowc(NULL, NULL, 0);
			len = 1;
		}
		if (c == '\\' && trimflag) {
			argp++;
			if (*argp == 0) {
				argp++;
				continue;
			}
			if ((len = mbtowc(&wc, C argp, MB_LEN_MAX)) <= 0) {
				(void) mbtowc(NULL, NULL, 0);
				len = 1;
			}
		}
		while (len-- > 0) {
			GROWSTAKTOP();
			pushstak(*argp++);
		}
	}
	return (argp);
}
