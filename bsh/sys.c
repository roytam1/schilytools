/* @(#)sys.c	1.84 21/07/22 Copyright 1986-2021 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)sys.c	1.84 21/07/22 Copyright 1986-2021 J. Schilling";
#endif
/*
 *	Copyright (c) 1986-2021 J. Schilling
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

#include <schily/stdio.h>
#include <schily/signal.h>
#include <schily/string.h>			/* Die system strings f�r strsignal()*/
#include <schily/unistd.h>
#include <schily/stdlib.h>
#include <schily/wait.h>
#include <schily/fcntl.h>
#include <schily/time.h>
#define	VMS_VFORK_OK
#include <schily/vfork.h>

/*
 * Check whether fexec may be implemented...
 */
#if	defined(HAVE_DUP) && (defined(HAVE_DUP2) || defined(F_DUPFD))
/* Everything is OK */
#else
Error fexec canno be implemented
#endif

/*#undef	HAVE_WAIT3*/
/*#undef	HAVE_SYS_PROCFS_H*/
#if	defined(HAVE_WAIT3) || (defined(HAVE_SYS_PROCFS_H) && defined(HAVE_WAITID)) /*see wait3.c*/
#	define	USE_WAIT3
#endif

#ifndef	HAVE_WAITID
	/*
	 * XXX Hack: Interix has sys/procfs.h but no waitid()
	 */
#undef	HAVE_SYS_PROCFS_H
#endif

#ifdef	NO_WAIT3
#undef	USE_WAIT3
#endif


#if	defined(HAVE_SYS_TIMES_H) && defined(HAVE_TIMES)
#include <schily/times.h>
#include <schily/param.h>
#endif	/* defined(HAVE_SYS_TIMES_H) && defined(HAVE_TIMES) */

#include <schily/resource.h>

/*#define	DEBUG*/

#include "bsh.h"
#include "str.h"
#include "abbrev.h"
#include "strsubs.h"			/* Die lokalen strings */
#include "ctype.h"
#include "limit.h"

#ifdef	VFORK
	char	*Vlist;
	char	*Vtmp;
	char	**Vav;
#endif

#ifdef	JOBCONTROL
extern	pid_t	pgrp;
extern	int	ttyflg;
	pid_t	lastsusp;	/* XXX Temporary !!! */
#endif

extern	pid_t	mypid;
extern	int	ex_status;
extern	int	do_status;
extern	int	firstsh;

EXPORT	void	start		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	pid_t	shfork		__PR((int flag));
EXPORT	void	pset		__PR((pid_t child, int flag));
EXPORT	void	block_sigs	__PR((void));
EXPORT	void	unblock_sigs	__PR((void));
LOCAL	void	psigsetup	__PR((int flag));
LOCAL	void	csigsetup	__PR((int flag));
LOCAL	void	ppgrpsetup	__PR((pid_t child, int flag));
LOCAL	void	cpgrpsetup	__PR((pid_t child, int flag));
EXPORT	int	ewait		__PR((pid_t child, int flag));
EXPORT	int	fexec		__PR((char **path, char *name, FILE *in, FILE *out, FILE *err, char **av, char **env));
LOCAL	void	fdmove		__PR((int fd1, int fd2));
LOCAL	BOOL	is_binary	__PR((char *name));
EXPORT	char	*_findinpath	__PR((char *name, int mode, BOOL plain_file));

#ifdef	HAVE_WAITID
LOCAL	void	ruget		__PR((struct rusage *rup));
LOCAL	void	ruadd		__PR((struct rusage *ru, struct rusage *ru2));
LOCAL	void	rusub		__PR((struct rusage *ru, struct rusage *ru2));
#endif

EXPORT void
start(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	register int	i = 0;
		char	*path = NULL;

	/*
	 * Allow to stop the command before execution.
	 * This may happen if the user hists ^C while the command
	 * is parsed or the arguments are expanded.
	 */
	if (ctlc)
		_exit(-1);
#ifdef DEBUG
	fprintf(stderr, "STARTING child=%ld %s with %d %d %d:",
		(long)getpid(),
		vp->av_av[0], fdown(std[0]), fdown(std[1]), fdown(std[2]));
	for (i = 1; i < vp->av_ac; i++)
		fprintf(stderr, " %s", vp->av_av[i]);
	i = 0;		/* Siehe pfexec */
	fputc('\n', stderr);
	fflush(stderr);
#endif
	if ((flag & IGNENV) == 0)
		update_cwd();	/* Er koennte sie veraendert haben */
#ifdef	EXECATTR_FILENAME
	if (pfshell) {
		i = pfexec(&path, vp->av_av[0], std[0], std[1], std[2], vp->av_av, evarray);
		if (i != 0)
			seterrno(i);
	}
	if (i == 0)	/* No pfexec() attrs for this command, retry exec() */
#endif
	i = fexec(&path, vp->av_av[0], std[0], std[1], std[2], vp->av_av, evarray);
	/*
	 * If we come here, this may be caused by the following reasons:
	 *
	 * ENOENT	File not found
	 * E2BIG	Too many args
	 * ENOMEM	Not enough memory for execution
	 * EACCES	No permission
	 * ENOEXEC	Invalid magic number (wrong format or shell script)
	 * Other problems are not of direct interest for us.
	 */
	if (i == ENOEXEC) {
		if (is_binary(path)) {
			berror("Cannot execute binary file.");
		} else {
			vac = vp->av_ac; vav = vp->av_av;	/* set args	*/
			i = firstsh;			/* save firstsh	*/
			firstsh = FALSE;		/* for vfork	*/
			dofile(path, GLOBAL_AB, 0, std, TRUE);
			free(path);
			path = NULL;
			firstsh = i;			/* restore firstsh*/
			if ((i = do_status) == 0)
				_exit(0);
		}
	}
	berror(ecantexecute, vp->av_av[0], errstr(i));
	if (path)
		free(path);
	_exit(i);
}


EXPORT pid_t
shfork(flag)
	int	flag;
{
	int	i = 0;
	pid_t	child;

	for (;;) {
		child = fork();
		if (child >= 0) {
			break;
		} else {
			i++;
			berror(ecantfork, errstr(geterrno())); /* XXX berror sezt evt. errno */
			if (i > 10 || ctlc)
				return (child);
			else
				sleep(1);
		}
	}
	pset(child, flag);
	return (child);
}

/*
 * Set up Parent/Child process signals / process groups
 */
#ifdef	PROTOTYPES
EXPORT void
pset(pid_t child, int flag)
#else
EXPORT void
pset(child, flag)
	pid_t	child;
	int	flag;
#endif
{
	if (child != 0) {			/* Papa */

		psigsetup(flag);		/* setup sigs */
		ppgrpsetup(child, flag);	/* setup process groups */

		if (flag & PRPID)
			berror("%ld", (long)child);

	} else {				/* Sohn */

		cpgrpsetup(child, flag);	/* setup process groups */
		csigsetup(flag);		/* setup sigs */
/*		firstsh = FALSE;*/		/* vfork doesn't like this */
	}
}

EXPORT void
block_sigs()
{
#ifdef	SVR4
	sighold(SIGINT);
#endif
}

EXPORT void
unblock_sigs()
{
#ifdef	SVR4
	sigrelse(SIGINT);
#endif
}

/*
 * Parent signal setup
 */
LOCAL void
psigsetup(flag)
	int	flag;
{
	extern	int	qflg;

	/*
	 * Bei Sun werden alle Signale im Kind ueberschrieben
	 */

#ifdef	VFORK
	signal(SIGINT, (sigtype) SIG_IGN);
	if (osig2 != (sigtype) SIG_IGN || firstsh)
		signal(SIGINT, intr);

#ifdef	SIGQUIT
	signal(SIGQUIT, (sigtype) SIG_IGN);
	if (qflg)
		signal(SIGQUIT, (sigtype) SIG_DFL);
#endif

	signal(SIGTERM, (sigtype) SIG_IGN);
#ifdef	SIGTSTP
	signal(SIGTSTP, (sigtype) SIG_IGN);
	signal(SIGTTIN, (sigtype) SIG_IGN);
	signal(SIGTTOU, (sigtype) SIG_IGN);
#endif
#endif	/* VFORK */
}

/*
 * Child signal setup
 */
LOCAL void
csigsetup(flag)
	int	flag;
{
	sigtype	sig2;
	sigtype	sig3;

	if (flag & NOSIG) {
		sig2 = (sigtype) SIG_IGN;
		sig3 = (sigtype) SIG_IGN;
	} else {
		sig2 = osig2;
		sig3 = osig3;
	}
	signal(SIGINT, sig2);
#ifdef	SIGQUIT
	signal(SIGQUIT, sig3);
#endif
	signal(SIGTERM, osig15);
#ifdef	SIGTSTP
	signal(SIGTSTP, osig18);
	signal(SIGTTIN, osig21);
	signal(SIGTTOU, osig22);
#endif
}

/*
 * Parent process group setup
 */
#ifdef	PROTOTYPES
LOCAL void
ppgrpsetup(pid_t child, int flag)
#else
LOCAL void
ppgrpsetup(child, flag)
	pid_t	child;
	int	flag;
#endif
{
#ifdef	JOBCONTROL
	if (flag & NOPGRP)
		return;

/*#define	TTEST*/
#ifdef TTEST
if (!(flag & BGRND))
	if (tty_setpgrp(0, pgrp) < 0)
		berror("TIOCSPGRP(%ld): %s\n", (long)child, errstr(geterrno()));
#endif	/* TEST */

/*	if (pgrp == mypid) berror("mypid: %ld getpid(): %ld child: %ld", (long)mypid, (long)getpid(), (long)child);*/

	if (pgrp == mypid)
/*	if (pgrp == 0)*/
		pgrp = child;
#endif	/* JOBCONTROL */
}

/*
 * Child process group setup
 */
#ifdef	PROTOTYPES
LOCAL void
cpgrpsetup(pid_t child, int flag)
#else
LOCAL void
cpgrpsetup(child, flag)
	pid_t	child;
	int	flag;
#endif
{
	int	i;
#ifdef	HAVE_USLEEP
#define	MAX_SPGID_RETRY	30
#else
#define	MAX_SPGID_RETRY	1000
#endif
#ifdef	JOBCONTROL
	if (flag & NOPGRP)
		return;
	if (ttyflg) {
/*	if (pgrp == mypid) berror("mypid: %ld getpid(): %ld child: %ld", (long)mypid, (long)getpid(), (long)child);*/

		if (pgrp == mypid) {
/*		if (pgrp == 0) {*/
			pgrp = getpid();
		}
#ifdef	POSIXJOBS
		/*
		 * Wenn man mit POSIXJOBS die Terminalprozessgruppe
		 * auf einen Wert, der nicht der Prozessgruppe des
		 * aktuellen Prozesses entspricht stellen will
		 * bekommt mann EPERM, daher setpgid *vorher*
		 */

		/*
		 * Der bsh ist der Vater aller Prozesse einer Pipe und daher
		 * kann es passieren, da� der Prozessgroupleader noch nicht
		 * losgelaufen ist und selbst ein setpgid(0, pgrp) gemacht hat
		 * w�hrend schon der 2. Pipeprozess das gleiche versucht und
		 * scheitert weil die betreffende Prozesgruppe noch nicht
		 * existiert. Wir warten hier (geben damit die CPU ab und dem
		 * Prozessgroupleader die Chance setpgid(0, pgrp) zu rufen) und
		 * versuchen es wieder. Bei ausschlie�licher Verwendung von
		 * vfork() w�rde dieses Problem nicht auftreten weil immer
		 * das Kind zuerst losl�uft.
		 */
		for (i = 0; i < MAX_SPGID_RETRY && setpgid(0, pgrp) < 0; i++) {
#ifdef	HAVE_USLEEP
			usleep(1);
#endif
		}
#endif	/* POSIXJOBS */

		if (!(flag & BGRND))
			tty_setpgrp(0, pgrp);

#ifndef	POSIXJOBS
		/*
		 * Ich kann leider nicht nachpr�fen, ob es auch
		 * unter BSD 4.2 ff. korrekt geht, wenn setpgid
		 * voerher gestellt wird, daher...
		 */
		for (i = 0; i < MAX_SPGID_RETRY && setpgid(0, pgrp) < 0; i++) {
#ifdef	HAVE_USLEEP
			usleep(1);
#endif
		}
#endif	/* !POSIXJOBS */
	}
#endif	/* JOBCONTROL */
}

#ifdef	PROTOTYPES
EXPORT int
ewait(pid_t child, int flag)
#else
EXPORT int
ewait(child, flag)
	pid_t	child;
	int	flag;
#endif
{
	pid_t died;
#ifdef	HAVE_WAITID
	siginfo_t	sinfo;
	struct rusage	rustart;

	struct {
		int type;
		int exit;
	} status;
#else	/* !HAVE_WAITID */
#if defined(i386) || defined(__x86) || defined(vax)
	struct {
		char 	type;
		char 	exit;
		unsigned filler:16;	/* Joerg */
	} status;
#else
	struct {
		unsigned filler:16;	/* Joerg */
		char 	exit;
		char 	type;
	} status;
#endif	/* defined(i386) || defined(__x86) || defined(vax) */

	int	stype = 0;

#endif	/* !HAVE_WAITID */
	struct rusage prusage;
#if	!defined(USE_WAIT3)	/* see wait3.c and above */
#if	defined(HAVE_SYS_TIMES_H) && defined(HAVE_TIMES)
	struct tms	tms1;
	struct tms	tms2;
#endif
#endif

	const char *mesg;

	fillbytes(&prusage, sizeof (struct rusage), 0);

#ifdef DEBUG
	printf("ewait: child=%ld, flag=%x\n", (long)child, flag);
	seterrno(0);
#endif
	do {
#ifdef	HAVE_WAITID
		ruget(&rustart);
		do {
			seterrno(0);
			died = waitid(P_ALL, 0, &sinfo, WEXITED|WTRAPPED|WSTOPPED);
			status.exit = sinfo.si_status;
			status.type = sinfo.si_code;
#ifdef DEBUG
printf("ewait: back from child.\n");
printf("       died       = %lx (%ld) errno= %d\n",
			(long)sinfo.si_pid, (long)sinfo.si_pid, geterrno());
printf("       wait       = %4.4x\n", *(int *)&status);
printf("       exit_state = %2.2x\n", status.exit);
printf("       return     = %2.2x\n", status.type);
#endif
			if (status.type == CLD_EXITED)
				status.type = 0;
		} while (died < 0 && geterrno() == EINTR);
		if (died < 0) {
			status.type = geterrno();
			berror("%s", enochildren);
			break;			/* out of outer wait loop */
		} else {
			ruget(&prusage);
			rusub(&prusage, &rustart);
			ex_status = status.exit;
			died = sinfo.si_pid;
#ifdef	JOBCONTROL
			if (sinfo.si_code == CLD_KILLED &&
			    sinfo.si_status == SIGINT)
				ctlc++;
			if (sinfo.si_code == CLD_STOPPED)
				lastsusp = died;	/* XXX Temporary !!! */
#endif
			mesg = strsignal(status.exit);
			if (mesg != NULL && sinfo.si_code != CLD_EXITED) {
				fprintf(stderr, "\r\n");
				if (child != died ||
				    sinfo.si_code == CLD_STOPPED)
					fprintf(stderr, "%ld: ", (long)died);

				fprintf(stderr, "%s%s\n",
					mesg,
					sinfo.si_code == CLD_DUMPED ?
							ecore:nullstr);
			}
		}
		/*
		 * We do not come here in case of died < 0.
		 */
		if ((flag & NOTMS) == 0 && getcurenv("TIME"))
			prtimes(gstd, &prusage);
#else	/* !HAVE_WAITID */
#	if	defined(USE_WAIT3)	/* see wait3.c and above */

		/* Brain damaged AIX requires loop */
		do {
#ifdef	__hpux
/*			died = wait3((WAIT_T *)&status, WUNTRACED, 0);*/
			died = wait3((WAIT_T *)&status, WUNTRACED, &prusage);
#else
			died = wait3((WAIT_T *)&status, WUNTRACED, &prusage);
#endif
		} while (died < 0 && geterrno() == EINTR);

	/*
	 * SVR4/POSIX hat WSTOPPED als Alias f�r WUNTRACED
	 * WSTOPFLG entspricht dem Wert im wait Status.
	 * BSD hat dafuer _WSTOPPED und teilweise fehlerhaterweise
	 * auch WSTOPPED. Workaround ist in schily/wait.h
	 */
		if (status.type == WSTOPFLG) {
/* BSD Korrekt!	if (status.type == _WSTOPPED) {*/
#ifdef DEBUG
printf("ewait: back from child (WSTOPFLG).\n");
#endif
#ifdef	JOBCONTROL
			lastsusp = died;	/* XXX Temporary !!! */
#endif
			stype = status.type;
			status.type = status.exit;
			status.exit = stype;
		}
#	else	/* !defined(USE_WAIT3) */
#if	defined(HAVE_SYS_TIMES_H) && defined(HAVE_TIMES)
		times(&tms1);
#endif
		do {
			died = wait(&status);
		} while (died < 0 && geterrno() == EINTR);
#if	defined(HAVE_SYS_TIMES_H) && defined(HAVE_TIMES)
		if (times(&tms2) != -1) {
/*#define	TIMES_DEBUG*/
#ifdef	TIMES_DEBUG
			printf("CLK_TCK: %d\n", CLK_TCK);
			printf("tms_utime %ld\n", (long) (tms2.tms_cutime - tms1.tms_cutime));
			printf("tms_stime %ld\n", (long) (tms2.tms_cstime - tms1.tms_cstime));
#endif	/* TIMES_DEBUG */
			/*
			 * Hack for Haiku, where the times may sometime be negative
			 */
			if (tms2.tms_cutime < tms1.tms_cutime)
				tms2.tms_cutime = tms1.tms_cutime;
			if (tms2.tms_cstime < tms1.tms_cstime)
				tms2.tms_cstime = tms1.tms_cstime;
			prusage.ru_utime.tv_sec  = (tms2.tms_cutime - tms1.tms_cutime) / CLK_TCK;
			prusage.ru_utime.tv_usec = ((tms2.tms_cutime - tms1.tms_cutime) % CLK_TCK) * (1000000/CLK_TCK);
			prusage.ru_stime.tv_sec  = (tms2.tms_cstime - tms1.tms_cstime) / CLK_TCK;
			prusage.ru_stime.tv_usec = ((tms2.tms_cstime - tms1.tms_cstime) % CLK_TCK) * (1000000/CLK_TCK);
		}
#endif
#	endif	/* !defined(USE_WAIT3) */

#if defined(__BEOS__) || defined(__HAIKU__)	/* Dirty Hack for BeOS, we should better use the W* macros */
		{	int i = status.exit;
			status.exit = status.type;
			status.type = i;
		}
#endif
#ifdef DEBUG
printf("ewait: back from child.\n");
printf("       died       = %lx (%ld) errno= %d\n", (long)died, (long)died, geterrno());
printf("       wait       = %4.4x\n", *(int *)&status);
printf("       exit_state = %2.2x\n", status.exit);
printf("       return     = %2.2x\n", status.type);
#endif
		ex_status = status.exit;
		if (!ex_status && status.type)		/* Joerg */
			ex_status = status.type & 0177;

		if (died <= 0) {
			status.type = geterrno();
			berror("%s", enochildren);
			break;
		} else {
#ifdef	JOBCONTROL
			if (status.type == SIGINT)
				ctlc++;
#endif
			mesg = strsignal(status.type & 0177);
			if (mesg != NULL && (status.type & 0177) != 0) {
				fprintf(stderr, "\r\n");
				if (child != died || stype == WSTOPFLG)
/* BSD Korrekt!			if (child != died || stype == _WSTOPPED)*/
					fprintf(stderr, "%ld: ", (long)died);

				fprintf(stderr, "%s%s\n",
					mesg,
					status.type&0200?ecore:nullstr);
			}
		}
		if ((flag & NOTMS) == 0 && getcurenv("TIME"))
			prtimes(gstd, &prusage);
#endif	/* !HAVE_WAITID */
	} while (child != died && child != 0 && (flag & WALL) != 0);
#ifdef DEBUG
printf("ewait: returning %x (%d)\n", status.type, status.type);
#endif
	esigs();
	return (status.type);
}

#define	sys_exec(n, in, out, err, av, ev) (execve(n, av, ev), geterrno())
#define	enofile(t)			((t) == ENOENT || \
					(t)  == ENOTDIR || \
					(t)  == EISDIR || \
					(t)  == EIO)

#ifdef	F_GETFD
#define	fd_getfd(fd)		fcntl((fd), F_GETFD, 0)
#else
#define	fd_getfd(fd)		(0)
#endif
#ifdef	F_SETFD
#define	fd_setfd(fd, val)	fcntl((fd), F_SETFD, (val));
#else
#define	fd_setfd(fd, val)	(0)
#endif

EXPORT int
fexec(path, name, in, out, err, av, env)
		char	**path;
	register char	*name;
		FILE	*in;
		FILE	*out;
		FILE	*err;
		char	*av[];
		char	*env[];
{
	register char	*pathlist;
	register char	*tmp;
	register char	*p1;
	register char	*p2;
	register int	t = 0;
	register int	exerr;
	char	*av0 = av[0];
	int	din;
	int	dout;
	int	derr;
#ifndef	set_child_standard_fds
	int	o[3];		/* Old fd's for stdinin/stdout/stderr */
	int	f[3];		/* Old close on exec flags for above  */

	o[0] = o[1] = o[2] = -1;
	f[0] = f[1] = f[2] = 0;
#endif

	exerr = 0;
	fflush(out); fflush(err);
	din  = fdown(in);
	dout = fdown(out);
	derr = fdown(err);

#ifdef	set_child_standard_fds
	set_child_standard_fds(din, dout, derr);
#else
	if (din != STDIN_FILENO) {
		f[0] = fd_getfd(STDIN_FILENO);
		o[0] = dup(STDIN_FILENO);
		fd_setfd(o[0], FD_CLOEXEC);
		fdmove(din, STDIN_FILENO);
	}
	if (dout != STDOUT_FILENO) {
		f[1] = fd_getfd(STDOUT_FILENO);
		o[1] = dup(STDOUT_FILENO);
		fd_setfd(o[1], FD_CLOEXEC);
		fdmove(dout, STDOUT_FILENO);
	}
	if (derr != STDERR_FILENO) {
		f[2] = fd_getfd(STDERR_FILENO);
		o[2] = dup(STDERR_FILENO);
		fd_setfd(o[2], FD_CLOEXEC);
		fdmove(derr, STDERR_FILENO);
	}
#endif

	/* if has slash try exec and set error code */

	if (strchr(name, '/')) {
		tmp = makestr(name);
#ifdef	VFORK
		Vtmp = tmp;
#endif
		exerr = sys_exec(tmp, din, dout, derr, av, env);
		av[0] = av0;	/* BeOS destroys things ... */
	} else {
					/* "PATH" */
		if ((pathlist = getcurenv(pathname)) == NULL)
			pathlist = defpath;
		p2 = pathlist = makestr(pathlist);
#ifdef	VFORK
		Vlist = pathlist;
#endif
		for (;;) {
			p1 = p2;
			if ((p2 = strchr(p2, ':')) != NULL)
				*p2++ = '\0';
			if (*p1 == '\0')
				tmp = makestr(name);
			else
				tmp = concat(p1, slash, name, (char *)NULL);
#ifdef	VFORK
			Vtmp = tmp;
#endif
			t = sys_exec(tmp, din, dout, derr, av, env);
			av[0] = av0;	/* BeOS destroys things ... */

			if (exerr == 0 && !enofile(t))
				exerr = t;
			if ((!enofile(t) && !(t == EACCES)) || p2 == NULL)
				break;
			free(tmp);
		}
		free(pathlist);
#ifdef	VFORK
		Vlist = 0;
#endif
	}

#ifndef	set_child_standard_fds
	if (derr != STDERR_FILENO) {
		fdmove(STDERR_FILENO, derr);
		fdmove(o[2], STDERR_FILENO);
		if (f[2] == 0)
			fd_setfd(STDERR_FILENO, 0);
	}
	if (dout != STDOUT_FILENO) {
		fdmove(STDOUT_FILENO, dout);
		fdmove(o[1], STDOUT_FILENO);
		if (f[1] == 0)
			fd_setfd(STDOUT_FILENO, 0);
	}
	if (din != STDIN_FILENO) {
		fdmove(STDIN_FILENO, din);
		fdmove(o[0], STDIN_FILENO);
		if (f[0] == 0)
			fd_setfd(STDIN_FILENO, 0);
	}
	if (exerr == 0)
		exerr = t;
	seterrno(exerr);
#endif

#ifdef	VFORK
	Vtmp = 0;
	Vlist = 0;
#endif
	if (path)
		*path = tmp;
	else
		free(tmp);
	return (exerr);
}

#ifndef	set_child_standard_fds

LOCAL void
fdmove(fd1, fd2)
	int	fd1;
	int	fd2;
{
	close(fd2);
#ifdef	F_DUPFD
	fcntl(fd1, F_DUPFD, fd2);
#else
#ifdef	HAVE_DUP2
	dup2(fd1, fd2);
#endif
#endif
	close(fd1);
}

#endif

#if	defined(HAVE_ELF)
#	include <elf.h>
#else
# if	defined(HAVE_AOUT)
#	include	<a.out.h>
# endif
#endif

#ifndef	NMAGIC	/*XXX Elf & Coff ???*/

LOCAL BOOL
is_binary(name)
	char	*name;
{
	int		f = open(name, O_RDONLY);
	unsigned char	c = ' ';

	if (f < 0 || read(f, &c, 1) != 1) {
		close(f);
		return (TRUE);
	}
	return (!isprint(c) && !isspace(c));
}

#else

LOCAL BOOL
is_binary(name)
	char	*name;
{
	int	f = open(name, O_RDONLY);
	struct	exec x;
	unsigned char	c;

	fillbytes(&x, sizeof (x), '\0');

	if (f < 0 || read(f, &x, sizeof (x)) <= 1) {
		close(f);
		return (TRUE);
	}
	c = *((char *)&x);

	return (!N_BADMAG(x) || (!isprint(c) && !isspace(c)));
}
#endif

#include <schily/stat.h>

/*
 * Uour local version of findinpath() that is using getcurenv()
 * instead of getenv().
 */
EXPORT char *
_findinpath(name, mode, plain_file)
	char	*name;
	int	mode;
	BOOL	plain_file;
{
	char	*pathlist;
	char	*p1;
	char	*p2;
	char	*tmp;
	int	err = 0;
	int	exerr = 0;
	struct stat sb;

	if (name == NULL)
		return (NULL);
	if (strchr(name, '/'))
		return (makestr(name));

	if ((pathlist = getcurenv(pathname)) == NULL)
		pathlist = defpath;
	p2 = pathlist = makestr(pathlist);

	for (;;) {
		p1 = p2;
		if ((p2 = strchr(p2, ':')) != NULL)
			*p2++ = '\0';
		if (*p1 == '\0')
			tmp = makestr(name);
		else
			tmp = concat(p1, slash, name, (char *)NULL);

		seterrno(0);
		if (stat(tmp, &sb) >= 0) {
			if ((plain_file || S_ISREG(sb.st_mode)) &&
				(eaccess(tmp, mode) >= 0)) {
				free(pathlist);
				return (tmp);
			}
			if ((err = geterrno()) == 0)
				err = ENOEXEC;
		} else {
			err = geterrno();
		}
		free(tmp);
		if (exerr == 0 && !enofile(err))
			exerr = err;
		if ((!enofile(err) && !(err == EACCES)) || p2 == NULL)
			break;
	}
	free(pathlist);
	seterrno(exerr);
	return (NULL);
}

/*--------------------------------------------------------------------------*/
#ifdef	HAVE_WAITID
LOCAL void
ruget(rup)
	struct rusage	*rup;
{
	struct rusage	ruc;

	getrusage(RUSAGE_SELF, rup);
	getrusage(RUSAGE_CHILDREN, &ruc);
	ruadd(rup, &ruc);
}

LOCAL void
ruadd(ru, ru2)
	struct rusage	*ru;
	struct rusage	*ru2;
{
	timeradd(&ru->ru_utime, &ru2->ru_utime);
	timeradd(&ru->ru_stime, &ru2->ru_stime);

#if !defined(__BEOS__) && !defined(__HAIKU__) && \
    !defined(OS390) && !defined(__MVS__)
#ifdef	__future__
	if (ru2->ru_maxrss > ru->ru_maxrss)
		ru->ru_maxrss =	ru2->ru_maxrss;

	ru->ru_ixrss += ru2->ru_ixrss;
	ru->ru_idrss += ru2->ru_idrss;
	ru->ru_isrss += ru2->ru_isrss;
#endif
	ru->ru_minflt += ru2->ru_minflt;
	ru->ru_majflt += ru2->ru_majflt;
	ru->ru_nswap += ru2->ru_nswap;
	ru->ru_inblock += ru2->ru_inblock;
	ru->ru_oublock += ru2->ru_oublock;
	ru->ru_msgsnd += ru2->ru_msgsnd;
	ru->ru_msgrcv += ru2->ru_msgrcv;
	ru->ru_nsignals += ru2->ru_nsignals;
	ru->ru_nvcsw += ru2->ru_nvcsw;
	ru->ru_nivcsw += ru2->ru_nivcsw;
#endif /* !defined(__BEOS__) && !defined(__HAIKU__) */
}

LOCAL void
rusub(ru, ru2)
	struct rusage	*ru;
	struct rusage	*ru2;
{
	timersub(&ru->ru_utime, &ru2->ru_utime);
	timersub(&ru->ru_stime, &ru2->ru_stime);

#if !defined(__BEOS__) && !defined(__HAIKU__) && \
    !defined(OS390) && !defined(__MVS__)
#ifdef	__future__
	if (ru2->ru_maxrss > ru->ru_maxrss)
		ru->ru_maxrss =	ru2->ru_maxrss;

	ru->ru_ixrss -= ru2->ru_ixrss;
	ru->ru_idrss -= ru2->ru_idrss;
	ru->ru_isrss -= ru2->ru_isrss;
#endif
	ru->ru_minflt -= ru2->ru_minflt;
	ru->ru_majflt -= ru2->ru_majflt;
	ru->ru_nswap -= ru2->ru_nswap;
	ru->ru_inblock -= ru2->ru_inblock;
	ru->ru_oublock -= ru2->ru_oublock;
	ru->ru_msgsnd -= ru2->ru_msgsnd;
	ru->ru_msgrcv -= ru2->ru_msgrcv;
	ru->ru_nsignals -= ru2->ru_nsignals;
	ru->ru_nvcsw -= ru2->ru_nvcsw;
	ru->ru_nivcsw -= ru2->ru_nivcsw;
#endif /* !defined(__BEOS__) && !defined(__HAIKU__) */
}
#endif	/* HAVE_WAITID */
