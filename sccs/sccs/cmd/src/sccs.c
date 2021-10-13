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
 * Copyright 2005 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2006-2020 J. Schilling
 *
 * @(#)sccs.c	1.136 20/09/10 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)sccs.c 1.136 20/09/10 J. Schilling"
#endif
/*
 * @(#)sccs.c 1.85 06/12/12
 */
#define	SCCS_MAIN			/* define global vars */
#include <defines.h>
#ifndef lint
static UConst char sccsid[] = "@(#)sccs.c 1.2 2/27/90";
#endif
#include <version.h>
#include <i18n.h>
#include <schily/dirent.h>
#include <schily/errno.h>
#include <schily/signal.h>
#include <schily/sigset.h>
#include <schily/getopt.h>
#include <schily/sysexits.h>
#ifndef	EX_OK
#define	EX_OK 0
#define	EX_USAGE 64
#define	EX_NOINPUT 66
#define	EX_UNAVAILABLE 69
#define	EX_SOFTWARE 70
#define	EX_OSERR 71
#endif
#ifndef __STDC__
extern struct passwd *getpwnam();
extern char *getlogin();
extern char *getenv();
#endif
#define	VMS_VFORK_OK
#include <schily/vfork.h>
#include <schily/varargs.h>
#include <schily/wait.h>
#define	comgetline	__no_comgetl__
#include <schily/schily.h>
#undef	comgetline
#include <schily/pwd.h>

static  char **diffs_np, **diffs_ap;

static	int	didvfork;

/*
**  SCCS.C -- human-oriented front end to the SCCS system.
**
**	Without trying to add any functionality to speak of, this
**	program tries to make SCCS a little more accessible to human
**	types.  The main thing it does is automatically put the
**	string "SCCS/s." on the front of names.  Also, it has a
**	couple of things that are designed to shorten frequent
**	combinations, e.g., "delget" which expands to a "delta"
**	and a "get".
**
**	This program can also function as a setuid front end.
**	To do this, you should copy the source, renaming it to
**	whatever you want, e.g., "syssccs".  Change any defaults
**	in the program (e.g., syssccs might default -d to
**	"/usr/src/sys").  Then recompile and put the result
**	as setuid to whomever you want.  In this mode, sccs
**	knows to not run setuid for certain programs in order
**	to preserve security, and so forth.
**
**	Usage:
**		sccs [flags] command [args]
**
**	Flags:
**		-d<dir>		<dir> represents a directory to search
**				out of.  It should be a full pathname
**				for general usage.  E.g., if <dir> is
**				"/usr/src/sys", then a reference to the
**				file "dev/bio.c" becomes a reference to
**				"/usr/src/sys/dev/bio.c".
**		-p<path>	prepends <path> to the final component
**				of the pathname.  By default, this is
**				"SCCS".  For example, in the -d example
**				above, the path then gets modified to
**				"/usr/src/sys/dev/SCCS/s.bio.c".  In
**				more common usage (without the -d flag),
**				"prog.c" would get modified to
**				"SCCS/s.prog.c".  In both cases, the
**				"s." gets automatically prepended.
**		-r		run as the real user.
**
**	Commands:
**		admin,
**		get,
**		delta,
**		rmdel,
**		cdc,
**		etc.		Straight out of SCCS; only difference
**				is that pathnames get modified as
**				described above.
**		enter		Front end doing "sccs admin -i<name> <name>"
**		create		Macro for "enter" followed by "get".
**		edit		Macro for "get -e".
**		editor		Edit a file whether or not it is controlled
**				by SCCS. Retrieves a version for editing
**				before, if needed.
**		unedit		Removes a file being edited, knowing
**				about p-files, etc.
**		delget		Macro for "delta" followed by "get".
**		deledit		Macro for "delta" followed by "get -e".
**		branch		Macro for "get -b -e", followed by "delta
**				-s -n", followd by "get -e -t -g".
**		diffs		"diff" the specified version of files
**				and the checked-out version.
**		print		Macro for "prs -e" followed by "get -p -m".
**		tell		List what files are being edited.
**		info		Print information about files being edited.
**		clean		Remove all files that can be
**				regenerated from SCCS files.
**		check		Like info, but return exit status, for
**				use in makefiles.
**		fix		Remove a top delta & reedit, but save
**				the previous changes in that delta.
**		istext		Check whether the argument files need to be
**				encoded
**
**	Compilation Flags:
**		UIDUSER -- determine who the user is by looking at the
**			uid rather than the login name -- for machines
**			where SCCS gets the user in this way.
**		SCCSDIR -- if defined, forces the -d flag to take on
**			this value.  This is so that the setuid
**			aspects of this program cannot be abused.
**			This flag also disables the -p flag.
**		SCCSPATH -- the default for the -p flag.
**		MYNAME -- the title this program should print when it
**			gives error messages.
**
**	Compilation Instructions:
**		cc -O -n -s sccs.c
**		The flags listed above can be -D defined to simplify
**			recompilation for variant versions.
**
**	Author:
**		Eric Allman, UCB/INGRES
**		Copyright 1980 Regents of the University of California
*/

/*******************  Configuration Information  ********************/

#ifndef	SCCSPATH
#define	SCCSPATH	"SCCS"	/* pathname in which to find s-files */
#endif /* NOT SCCSPATH */

#ifndef	NSCCS
#define	NSCCS		"-N   SCCS" /* conversion option for SCCS cmds */
#endif /* NOT NSCCS */
#ifndef	NSCCSsd
#define	NSCCSsd		"-NSCCS/s." /* conversion option for libfind */
#endif /* NOT NSCCSsd */

#ifndef	MYNAME
#define	MYNAME		"sccs"	/* name used for printing errors */
#endif /* NOT MYNAME */

#ifdef	DESTDIR
#ifndef	V6
#if defined(__STDC__) || defined(PROTOTYPES)
#define	PROGPATH(name)	#name
#else
#define	PROGPATH(name)	"name"
#endif
#else
#if defined(__STDC__) || defined(PROTOTYPES)
#define	PROGPATH(name)	"/usr/ccs/bin/"	#name /* place to find binaries */
#else
#define	PROGPATH(name) "/usr/ccs/bin/name"	/* place to find binaries */
#endif
#endif /* V6 */
#else
#ifdef	XPG4
#if defined(__STDC__) || defined(PROTOTYPES)
#define	PROGPATH(name)	#name
#else
#define	PROGPATH(name)	"name"
#endif
#else
#if defined(__STDC__) || defined(PROTOTYPES)
#define	PROGPATH(name)	"/usr/ccs/bin/"	#name /* place to find binaries */
#else
#define	PROGPATH(name) "/usr/ccs/bin/name"	/* place to find binaries */
#endif
#endif /* XPG4 */
#endif /* DESTDIR */

#if	defined(INS_BASE) && !defined(XPG4)
#undef	PROGPATH

#if defined(__STDC__) || defined(PROTOTYPES)
#define	PROGPATH(name)	INS_BASE "/" SCCS_BIN_PRE "bin/" #name /* place to find binaries */
#else
/*
 * XXX With a K&R compiler, you need to edit the following string in case
 * XXX you like to change the install path.
 */
#define	PROGPATH(name) "/usr/ccs/bin/name"	/* place to find binaries */
#endif
#endif	/* defined(INS_BASE) && !defined(XPG4) */


/****************  End of Configuration Information  ****************/

typedef int	bool;
#define	TRUE	1
#define	FALSE	0

#define	FORCE_FORK	2	/* alternative "TRUE" forkflag */

#define	bitset(bit, word)	((bool) ((bit) & (word)))

struct sccsprog
{
	char	*sccsname;	/* name of SCCS routine */
	short	sccsoper;	/* opcode, see below */
	short	sccsflags;	/* flags, see below */
	char	*sccspath;	/* pathname of binary implementing */
};

struct list_files
{
	struct	list_files	*next;
	char			*filename;
	char			*s_filename;
};

/*
 * Macro to skip the following names: "", ".", "..".
 */
#define	dot_dotdot(n)	((n)[(n)[0] != '.' ? 0 : (n)[1] != '.' ? 1 : 2] == '\0')


int main __PR((int argc, char **argv));
static char *getNsid __PR((char *file, char *user));
int command __PR((char **argv, bool forkflag, char *arg0));
static void get_sccscomment __PR((void));
static void get_list_files __PR((struct list_files **listftailpp, char *filename, bool no_sdot));
static struct sccsprog *lookup __PR((char *name));
static int callprog __PR((char *progpath, int flags, char **argv, bool forkflag));
static char *makefile __PR((char *name, const char *in_SccsDir));
static bool isdir __PR((char *name));
static bool isfile __PR((char *name));
static bool safepath __PR((register char *p));
static char *xstrdup __PR((char *file));
static int fix __PR((int nfiles, char **argv));
static int clean __PR((int mode, char **argv));
static void nothingedited __PR((bool nobranch, const char *usernm));
static bool isbranch __PR((char *sid));
static int unedit __PR((int nfiles, char **argv));
static void do_unedit __PR((char *fn));
static char *tail __PR((register char *fn));
static struct p_file *getpfent __PR((FILE *pfp));
static int checkpfent __PR((struct p_file *pf));
static char *nextfield __PR((register char *p));
static void putpfent __PR((register struct p_file *pf, register FILE *f));
static void syserr	__PR((const char *f, ...));
static char *gstrcat __PR((char *to, char *from, unsigned int xlength));
static char *gstrncat __PR((char *to, char *from, int n, unsigned int xlength));
static char *gstrcpy __PR((char *to, const char *from, unsigned int xlength));
static void gstrbotch __PR((const char *str1, const char *str2));
static int diffs __PR((int nfiles, char **argv));
static void do_diffs __PR((char *file));
static int enter __PR((int nfiles, char **argv));
static int editor __PR((int nfiles, char **argv));
static int histfile __PR((int nfiles, int argc, char **argv));
static int istext __PR((int nfiles, int argc, char **argv));
static char *makegfile __PR((char *name));
#ifdef	USE_RECURSIVE
static int dorecurse __PR((char **argv, char **np, char *dir, struct sccsprog *cmd));
#endif
static int fgetchk	__PR((char *file, int dov6, int silent));

/* values for sccsoper */
#define	PROG		0	/* call a program */
#define	CMACRO		1	/* command substitution macro */
#define	FIX		2	/* fix a delta */
#define	CLEAN		3	/* clean out recreatable files */
#define	UNEDIT		4	/* unedit a file */
#ifdef	V6
#define	SHELL		5	/* call a shell file (like PROG) */
#endif
#define	DIFFS		6	/* diff between sccs & file out */
#define	ENTER		7	/* enter new files */
#define	EDITOR		8	/* get -e + call $EDITOR */
#define	ISTEXT		9	/* check whether file needs encoding */
#define	ADD		10	/* add specified files on next commit */
#define	COMMIT		11	/* commit changes to project repository */
#define	INIT		12	/* initialize empty project repository */
#define	REMOVE		13	/* remove specified files on next commit */
#define	RENAME		14	/* rename specified files on next commit */
#define	ROOT		15	/* show project root directory */
#define	STATUS		16	/* show changed files in the project */
#define	HISTFILE	17	/* give history file path for s-file */

/* bits for sccsflags */
#define	NO_SDOT		0001	/* no s. in front of args */
#define	REALUSER	0002	/* protected (e.g., admin) */
#define	RF_OK		0004	/* -R allowed with this command */
#define	PDOT		0010	/* process based on on p. files */
#define	COLLECT		0020	/* collect file names and call only once */
#define	NO_N		0040	/* -N option not supported by program */

/* modes for the "clean", "info", "check" ops */
#define	CLEANC		0	/* clean command */
#define	INFOC		1	/* info command */
#define	CHECKC		2	/* check command */
#define	TELLC		3	/* give list of files being edited */

/*
**  Description of commands known to this program.
**	First argument puts the command into a class.  Second arg is
**	info regarding treatment of this command.  Third arg is a
**	list of flags this command accepts from macros, etc.  Fourth
**	arg is the pathname of the implementing program, or the
**	macro definition, or the arg to a sub-algorithm.
*/

static struct sccsprog SccsProg[] =
{
	{ "admin",	PROG,	REALUSER,		PROGPATH(admin) },
	{ "cdc",	PROG,	0,			PROGPATH(rmdel) },
	{ "comb",	PROG,	0,			PROGPATH(comb) },
	{ "cvt",	PROG,	RF_OK,			PROGPATH(sccscvt) },
	{ "delta",	PROG,	RF_OK|PDOT,		PROGPATH(delta) },
	{ "get",	PROG,	RF_OK,			PROGPATH(get) },
	{ "help",	PROG,	NO_SDOT|NO_N,		PROGPATH(help) },
	{ "log",	PROG,	RF_OK|COLLECT,		PROGPATH(sccslog) },
	{ "prs",	PROG,	RF_OK,			PROGPATH(prs) },
	{ "prt",	PROG,	RF_OK,			PROGPATH(prt) },
	{ "rmdel",	PROG,	REALUSER,		PROGPATH(rmdel) },
	{ "sact",	PROG,	RF_OK,			PROGPATH(sact) },
	{ "val",	PROG,	RF_OK,			PROGPATH(val) },
	{ "what",	PROG,	NO_SDOT|NO_N,		PROGPATH(what) },
#ifndef V6
	{ "sccsdiff",	PROG,	REALUSER,		PROGPATH(sccsdiff) },
	{ "sccsdiffs",	PROG,	REALUSER,		PROGPATH(sccsdiff) },
	{ "rcs2sccs",	PROG,	REALUSER|NO_N,		PROGPATH(rcs2sccs) },
#else
	{ "sccsdiff",	SHELL,	REALUSER,		PROGPATH(sccsdiff) },
	{ "sccsdiffs",	SHELL,	REALUSER,		PROGPATH(sccsdiff) },
	{ "rcs2sccs",	SHELL,	REALUSER|NO_N,		PROGPATH(rcs2sccs) },
#endif /* V6 */
	{ "edit",	CMACRO,	RF_OK|NO_SDOT,		"get -e" },
	{ "editor",	EDITOR,	NO_SDOT,		NULL },
	{ "histfile",	HISTFILE, NO_SDOT,		NULL },
	{ "delget",	CMACRO,	RF_OK|NO_SDOT|PDOT,
	    "delta:mysropdfq/get:ixbeskclo -t" },
	{ "deledit",	CMACRO,	RF_OK|NO_SDOT|PDOT,
	    "delta:mysropdfq/get:ixbskclo -e -t -d" },
	{ "fix",	FIX,	NO_SDOT,		NULL },
	{ "clean",	CLEAN,	RF_OK|REALUSER|NO_SDOT,	(char *) CLEANC },
	{ "info",	CLEAN,	RF_OK|REALUSER|NO_SDOT,	(char *) INFOC },
	{ "check",	CLEAN,	RF_OK|REALUSER|NO_SDOT,	(char *) CHECKC },
	{ "tell",	CLEAN,	RF_OK|REALUSER|NO_SDOT,	(char *) TELLC },
	{ "istext",	ISTEXT,	REALUSER|NO_SDOT,	NULL },
	{ "unedit",	UNEDIT,	RF_OK|NO_SDOT|PDOT,	NULL },
	{ "unget",	PROG,	RF_OK|PDOT,		PROGPATH(unget) },
	{ "diffs",	DIFFS,	RF_OK|NO_SDOT|PDOT|REALUSER,	NULL },
	{ "ldiffs",	DIFFS,	RF_OK|NO_SDOT|PDOT|REALUSER,	NULL },
	{ "-diff",	PROG,	NO_SDOT|REALUSER|NO_N,	PROGPATH(diff) },
	{ "-ldiff",	PROG,	NO_SDOT|REALUSER|NO_N,	"diff" },
	{ "print",	CMACRO,	RF_OK|NO_SDOT,		"prs:ar -e/get:Anr -p -m -s" },
	{ "branch",	CMACRO,	RF_OK|NO_SDOT,
	    "get:ixrc -e -b/delta: -s -n -ybranch-place-holder/get:pl -e -t -g" },
	{ "enter",	ENTER,	NO_SDOT,		NULL },
	{ "create",	CMACRO,	NO_SDOT,
	    "enter:abdfmortyzV/get:ixbeskclo -t" },
	{ "add",	ADD,	REALUSER|NO_SDOT,	NULL },
	{ "commit",	COMMIT,	REALUSER|NO_SDOT,	NULL },
	{ "init",	INIT,	REALUSER|NO_SDOT,	NULL },
	{ "remove",	REMOVE,	REALUSER|NO_SDOT,	NULL },
	{ "rename",	RENAME,	REALUSER|NO_SDOT,	NULL },
	{ "root",	ROOT,	REALUSER|NO_SDOT,	NULL },
	{ "status",	STATUS,	REALUSER|NO_SDOT,	NULL },
	{ NULL,		-1,	0,			NULL }
};

/* one line from a p-file */
struct p_file
{
	char	*p_osid;	/* old SID */
	char	*p_nsid;	/* new SID */
	char	*p_user;	/* user who did edit */
	char	*p_date;	/* date of get */
	char	*p_time;	/* time of get */
	char	*p_aux;		/* extra info at end */
};

static char	*SccsPath = SCCSPATH;	/* pathname of SCCS files ("SCCS") */
static char	_NewOpt[] = NSCCS;	/* path opt. of SCCS files ("-NSCCS") */
static char	*NewOpt = _NewOpt;	/* make it modifyable		   */
static char	*NewOptsd = NSCCSsd;	/* path opt. of files ("-NSCCS/s.") */
#ifdef SCCSDIR
static char	*SccsDir = SCCSDIR;	/* directory to begin search from */
#else
static char	*SccsDir = "";
#endif
static char	MyName[] = MYNAME;	/* name used in messages */
static int	OutFile = -1;		/* override output file for commands */
static bool	RealUser;		/* if set, running as real user */
static bool	NewMode;		/* if set, we use -NSCCS */
#ifdef DEBUG
static bool	Debug;			/* turn on tracing */
#endif
#ifndef V6
#ifndef	PROTOTYPES
extern char	*getenv();
#endif
#endif /* V6 */

static Nparms	N;			/* Keep -NSCCS parameters	*/
static Nparms	Nsd;			/* Keep -NSCCS/s. parameters	*/

#ifdef XPG4
/*static char path[] = NOGETTEXT("PATH=/usr/xpg4/bin:/usr/ccs/bin:/usr/bin");*/
#ifdef	PROTOTYPES
static char path[] = NOGETTEXT("PATH=" INS_BASE "/xpg4/bin:" INS_BASE "/ccs/bin:/usr/bin");
#else
/*
 * XXX With a K&R compiler, you need to edit the following string in case
 * XXX you like to change the install path.
 */
static char path[] = NOGETTEXT("PATH=/usr/xpg4/bin:/usr/ccs/bin:/usr/bin");
#endif
#endif

static struct	sccsprog	*maincmd = NULL;
static struct	sccsprog	*curcmd  = NULL;

#ifdef	HAVE_STRSIGNAL
#else
#ifdef	HAVE_SYS_SIGLIST
#ifndef	HAVE_SYS_SIGLIST_DEF
extern char	*sys_siglist[];
#endif
#endif
#endif

extern	int Fcnt;

static int create_macro  = 0;	/* 1 if "sccs create ..."  command is running. */
static int del_macro	 = 0;	/* 1 if "sccs deledit ..." or "sccs delget ..." commands are running. */

static int Rflag	 = 0;	/* -R recursive operation selected */
static char *Cwd;		/* -C parameter for delta/get	   */
#ifdef	USE_RECURSIVE
static int Cwdlen;		/* Allocation length for Cwd	   */
#endif

static bool Tgotedit;
static bool Tnobranch;
static char *Tusernm;

#define	FBUFSIZ	BUFSIZ
#define	PFILELG	120

int
main(argc, argv)
	int argc;
	char **argv;
{
	register char *p;
	register int i;
	int current_optind, c;
	register char *argp;
	bool	use_old = FALSE;
	bool	use_new = FALSE;

#ifndef V6
#ifndef SCCSDIR
	register struct passwd *pw;
	char buf[FBUFSIZ], cwdpath[FBUFSIZ];
#endif

	/*
	 * Set locale for all categories.
	 */
	setlocale(LC_ALL, NOGETTEXT(""));

	sccs_setinsbase(INS_BASE);

	/*
	 * Set directory to search for general l10n SCCS messages.
	 */
#ifdef	PROTOTYPES
	(void) bindtextdomain(NOGETTEXT("SUNW_SPRO_SCCS"),
	    NOGETTEXT(INS_BASE "/" SCCS_BIN_PRE "lib/locale/"));
#else
	(void) bindtextdomain(NOGETTEXT("SUNW_SPRO_SCCS"),
	    NOGETTEXT("/usr/ccs/lib/locale/"));
#endif

	(void) textdomain(NOGETTEXT("SUNW_SPRO_SCCS"));

	tzset();	/* Set up timezome related vars */

#ifdef	SCHILY_BUILD
	save_args(argc, argv);
#endif

	Fflags = FTLEXIT | FTLMSG | FTLCLN;
#ifdef	SCCS_FATALHELP
	Fflags |= FTLFUNC;
	Ffunc = sccsfatalhelp;
#endif

#ifndef SCCSDIR
	/* Pull "SccsDir" out of the environment variable	  */
	/*							  */
	/* PROJECTDIR						  */
	/* If contains an absolute path name  (beginning  with  a */
	/* slash),  sccs  searches  for SCCS history files in the */
	/* directory given by that variable.			  */
	/*							  */
	/* If PROJECTDIR does not begin with a slash, it is taken */
	/* as  the  name  of a user, and sccs searches the src or */
	/* source subdirectory of that user's home directory  for */
	/* history  files.  If  such  a directory is found, it is */
	/* used. Otherwise, the value is used as a relative  path */
	/* name.						  */

	p = getenv(NOGETTEXT("PROJECTDIR"));
	if (p != NULL && p[0] != '\0') {
		if (p[0] == '/') {
			SccsDir = p;
		} else {
			SccsDir = NULL;
			pw = getpwnam(p);
			if (pw != NULL) {
				SccsDir = buf;
				gstrcpy(buf, pw->pw_dir, sizeof (buf));
				gstrcat(buf, NOGETTEXT("/src/."), sizeof (buf));
				if (access(buf, 0) < 0) {
					gstrcpy(buf, pw->pw_dir, sizeof (buf));
					gstrcat(buf, NOGETTEXT("/source/."),
								sizeof (buf));
					if (access(buf, 0) < 0) {
						gstrcpy(buf, pw->pw_dir,
								sizeof (buf));
						gstrcat(buf, p, sizeof (buf));
						if (access(buf, 0) < 0) {
							SccsDir = NULL;
						}
					}
				}
			}
			if (SccsDir == NULL) {
				if (getcwd(cwdpath, FBUFSIZ - 1) == NULL) {
					usrerr(
					gettext("cannot determine current directory!"));
					exit(EX_USAGE);
				} else { /* Try local directory */
					sprintf(buf, NOGETTEXT("%s/%s/."), cwdpath, p);
					if (access(buf, 0) < 0) {
						usrerr(
						gettext("project %s has no source!"), p);
						usrerr(
						gettext("directory %s doesn't exist in the current directory!"),
							p);
						exit(EX_USAGE);
					}
				}
				SccsDir = buf;
			}
			/*
			 * Remove trailing "/."
			 */
			SccsDir[strlen(SccsDir) - 2] = '\0';
		}
	}
#endif /* SCCSDIR */
#endif /* V6 */

	/*
	**  Detect and decode flags intended for this program.
	*/

#ifdef V6
	argv[argc] = NULL;
#endif

#ifdef XPG4
	if (putenv(path) != 0) {
	   perror(gettext("Sccs: no mem"));
	   exit(EX_OSERR);
	}
#endif
	if (argv[0] != NULL && lookup(argv[0]) == NULL)
	{

	current_optind = 1;
	optind = 1;
	opterr = 0;
	i = 1;
	/*CONSTCOND*/
	while (1) {
			if (current_optind < optind) {
				current_optind = optind;
				argv[i] = 0;
				if (optind > i+1) {
					argv[i+1] = NULL;
				}
				i = current_optind;
			}
			c = getopt(argc, argv, "()-rp:d:NORTV(version)");
			if (c == EOF) {
				break;
			}
			argp = optarg;
			switch (c) {
			  case 'r':		/* run as real user */
				if (setuid(getuid()) < 0)
					syserr(gettext("Cannot set uid"));
				RealUser++;
				break;

#ifndef SCCSDIR
			  case 'p':		/* path of sccs files */
				SccsPath = argp;
				NewOpt = NULL;
				break;

			  case 'd':		/* directory to search from */
				SccsDir = argp;
				break;
#endif

#ifdef DEBUG
			  case 'T':		/* trace */
				sccs_setdebug(++Debug);
				break;
#endif

			  case 'N':
				use_old = FALSE;
				use_new = TRUE;
				break;
			  case 'O':
				use_old = TRUE;
				use_new = FALSE;
				break;

#ifdef	USE_RECURSIVE
			  case 'R':		/* recursion */
				Rflag++;
				break;
#endif

			  case 'V':		/* version */
				printf(gettext(
				    "sccs %s-SCCS version %s %s (%s-%s-%s)\n"),
					PROVIDER,
					VERSION,
					VDATE,
					HOST_CPU, HOST_VENDOR, HOST_OS);
				exit(EX_OK);

			  default:
				usrerr("%s %s", gettext("unknown option"), argv[i]);
#ifdef	USE_RECURSIVE
				fprintf(stderr, gettext("Usage: sccs [-N][-O][-R][ -r ][ -drootprefix ][ -p subdir ]\n\t subcommand [ option ...] [ filename ...]\n"));
#else
				fprintf(stderr, gettext("Usage: sccs [ -r ][ -drootprefix ][ -p subdir ]\n\t subcommand [ option ...] [ filename ...]\n"));
#endif
				exit(EX_USAGE);
			}
	}
	argv += current_optind;
	if (SccsPath[0] == '\0')
	   SccsPath = ".";
	}
	if (SccsDir == NULL)	/* Paranoia */
		SccsDir = "";

	if (*argv == NULL)
	{
#ifdef	USE_RECURSIVE
		fprintf(stderr, gettext("Usage: sccs [-N][-O][-R][ -r ][ -drootprefix ][ -p subdir ]\n\t subcommand [ option ...] [ filename ...]\n"));
#else
		fprintf(stderr, gettext("Usage: sccs [ -r ][ -drootprefix ][ -p subdir ]\n\t subcommand [ option ...] [ filename ...]\n"));
#endif
		sccshelp(stderr, "basic sub-commands");
		exit(EX_USAGE);
		/*NOTREACHED*/
	}

	if (xsethome(NULL) > 0)
		NewMode = TRUE;

	if (use_old) {
		NewMode = FALSE;
	} else if (use_new) {
		NewMode = TRUE;
	} else if ((p = getenv("SCCS_NMODE")) != NULL) {
		/*
		 * XXX Should we also disable any other SCCS v6 extensions
		 * XXX instead of just disabling the use of -NSCCS?
		 */
		if (strcmp(p, "FALSE") == 0)
			NewMode = FALSE;
		if (strcmp(p, "TRUE") == 0)
			NewMode = TRUE;
		/*
		 * In all other cases, keep the value from xsethome(NULL).
		 */
	} else {
#ifndef	__not_yet__
		/*
		 * If environmenment "SCCS_NMODE" is missing,
		 * NewMode is currently always set to FALSE.
		 */
		NewMode = FALSE;
#endif
	}
	if (NewMode && NewOpt == NULL) {
		perror(gettext("Sccs: -p cannot be used in New Mode"));
		exit(EX_OSERR);
	}
	if (NewMode) {
		setnewmode();
		if (NewOpt == NULL) {
			NewOpt = malloc(strlen(SccsPath)+6);
			if (NewOpt == NULL) {
				perror(gettext("Sccs: no mem"));
				exit(EX_OSERR);
			}
			strcpy(NewOpt, "-N   "); /* Three flag placeholders */
			strcat(NewOpt, SccsPath);

			NewOptsd = malloc(strlen(SccsPath)+6);
			if (NewOptsd == NULL) {
				perror(gettext("Sccs: no mem"));
				exit(EX_OSERR);
			}
			strcpy(NewOptsd, "-N");
			strcat(NewOptsd, SccsPath);
			strcat(NewOptsd, "/s.");
		}
		initN(&N);
		initN(&Nsd);
		N.n_parm = &NewOpt[2];
		Nsd.n_parm = &NewOptsd[2];
		parseN(&N);
		parseN(&Nsd);
	} else {
		unsetnewmode();
	}

	i = command(argv, FALSE, "");
	return (i);
}


static int	NelemArrSids;
static int	size_ap_for_get;
static int	cur_num_file;
static char	Nsid[50];
static char *	user_name;
static char *	r_option_value;
static char **	ArrSids;
static char **	ap_for_get;
static char **	macro_files;

/* getNsid() returns the latest checked out version of file.  */
/* This function is used in 'deledit' macro (see 1083894 bug) */

static char *
getNsid(file, user)
	char * file;
	char * user;
{
	int		cnt = -1;
	char		line[BUFSIZ];
	char *		cp;
	struct packet	gpkt;
	struct pfile	pf;
	struct pfile	goodpf;
	FILE *		in;

	if (NewMode) {
		file = bulkprepare(&N, file);
		if (file == NULL) {
			/*
			 * The error is typically
			 * "directory specified as s-file (cm14)"
			 */
			fatal(gettext(bulkerror(&N)));
		}
	}
	sinit(&gpkt, file, SI_INIT);
	cp = auxf(gpkt.p_file, 'p');
	if (!exists(cp))
		return (NULL);
	strcpy(Nsid, "");
	zero((char *)&goodpf, sizeof (goodpf));
	in = xfopen(cp, O_RDONLY|O_BINARY);
	while (fgets(line, sizeof (line), in) != NULL) {
		pf_ab(line, &pf, 1);
		if (equal(pf.pf_user, user) || getuid() == 0) {
			if (++cnt) {
				fclose(in);
				return (r_option_value);
			}
			goodpf = pf;
			continue;
		}
	}
	fclose(in);
	if (NewMode) {
		bulkchdir(&N);	/* chdir() back to our previous "." */
	}
	if (!goodpf.pf_user[0])
		return (NULL);
	if (!goodpf.pf_nsid.s_br) {
		sprintf(Nsid, NOGETTEXT("-r%d.%d"),
		  goodpf.pf_nsid.s_rel, goodpf.pf_nsid.s_lev);
	} else {
		sprintf(Nsid, NOGETTEXT("-r%d.%d.%d.%d"),
		  goodpf.pf_nsid.s_rel, goodpf.pf_nsid.s_lev,
		  goodpf.pf_nsid.s_br,  goodpf.pf_nsid.s_seq);
	}
	return (xstrdup(Nsid));
}

/*
**  COMMAND -- look up and perform a command
**
**	This routine is the guts of this program.  Given an
**	argument vector, it looks up the "command" (argv[0])
**	in the configuration table and does the necessary stuff.
**
**	Parameters:
**		argv -- an argument vector to process.
**		forkflag -- if set, fork before executing the command.
**		editflag -- if set, only include flags listed in the
**			sccsklets field of the command descriptor.
**		arg0 -- a space-separated list of arguments to insert
**			before argv.
**
**	Returns:
**		zero -- command executed ok.
**		else -- error status.
**
**	Side Effects:
**		none.
*/

int
command(argv, forkflag, arg0)
	char **argv;
	bool forkflag;
	char *arg0;
{
	register struct sccsprog *cmd;
	register char *p;
	char buf[FILESIZE];
	char **nav;
	char macro_opstr[64];
	char **np, *nextp;
	register char **ap;
	register char *q;
	int ac = 0;
	int rval = 0;
	int hady = 0;
	int len;
	int nav_size, cnt_files_from_stdin;
	int nfiles;
	char *editchs, *macro_opstr_p;
	bool	no_sdot;
	struct	list_files	head_files;
	struct	list_files	*listftailp;
	struct	list_files	*listfilesp;

#ifdef DEBUG
	if (Debug)
	{
		printf(gettext("command:\n\t\"%s\"\n"), arg0);
		for (np = argv; *np != NULL; np++)
			printf("\t\"%s\"\n", *np);
	}
#endif
	fflush(stdout);

	/*
	 * Select the second half of the macro string for NewMode if avaliable.
	 * Macro string is: "old mode command %new mode command".
	 * If the "new mode command" command starts with a space, up to 3
	 * flag characters for the -N option follow.
	 */
	if (NewMode && (p = strchr(arg0, '%')) != NULL)
		arg0 = ++p;

	/* reserve the space for nav[] */
	nav_size = 0;
	for (p = arg0, q = buf; *p != '\0' && *p != '/' && *p != '%'; ) {
		nav_size++;
		while (*p == ' ')
			p++;
		while (*p != ' ' && *p != '\0' &&
		    *p != '/' && *p != '%' && *p != ':')
			p++;
		if (*p == ':') {
			while (*++p != '\0' &&
			    *p != '/' && *p != '%' && *p != ' ')
				;
		}
	}
	/* added seven elements for: */
	/* - command;		     */
	/* - additional DIFFS param; */
	/* - -ycoment;		     */
	/* - -Cworkdir;		     */
	/* - -NSCCS;		     */
	/* - -R spare dir element;   */
	/* - last element (NULL).    */
	for (nav_size += 7, ap = argv; *ap != NULL; ap++) {
		nav_size++;
	}
	if ((nav = malloc(nav_size * (sizeof (char *)))) == NULL) {
		perror(gettext("Sccs: no mem"));
		exit(EX_OSERR);
	}

	/*
	**  Copy arguments.
	**	Copy from arg0 & if necessary at most one arg
	**	from argv[0].
	*/

	np = ap = &nav[1];
	head_files.next = NULL;
	listftailp = &head_files;
	editchs = NULL;		/* arg0 -> cmd:editchs/next... */
	macro_opstr_p = NULL;
	buf[0] = '\0';
	if (NewMode) {
		NewOpt[2] = ' '; /* Reset flags to placeholders */
		NewOpt[3] = ' ';
		NewOpt[4] = ' ';
	}
	p = arg0;
	if (NewMode && *p == ' ') {
		if (*++p == '+')
			NewOpt[2] = *p++;
		if (*p == '-')
			NewOpt[3] = *p++;
		if (*p == ',')
			NewOpt[4] = *p++;
	}
	for (q = buf; *p != '\0' && *p != '/' && *p != '%'; ) {
		*np++ = q;
		while (*p == ' ')
			p++;
		while (*p != ' ' && *p != '\0' &&
		    *p != '/' && *p != '%' && *p != ':')
			*q++ = *p++;
		*q++ = '\0';
		if (*p == ':') {
			editchs = q;
			while (*++p != '\0' &&
			    *p != '/' && *p != '%' && *p != ' ')
				*q++ = *p;
			*q++ = '\0';
		}
	}
	*np = NULL;
	if (*ap == NULL)
		*np++ = *argv++;

	/*
	**  Look up command.
	**	At this point, *ap is the command name.
	*/

	curcmd = cmd = lookup(*ap);
	if (cmd == NULL) {
		usrerr("%s \"%s\"", gettext("Unknown command"), *ap);
		return (EX_USAGE);
	}
#ifdef	USE_RECURSIVE
	if (Rflag > 0 && !bitset(RF_OK, cmd->sccsflags)) {
		usrerr("%s \"%s\"", gettext("Recursion not supported for"), *ap);
		return (EX_USAGE);
	}
#endif
	if (maincmd == NULL)
	   maincmd = cmd;
	no_sdot = bitset(NO_SDOT, cmd->sccsflags);
	if (cmd->sccsoper == CMACRO) {

	   char *cp, *cp_opstr;

	   /*
	    * Fill the sum of all permitted option chars into "macro_opstr".
	    */
	   cp_opstr = NULL;
	   cp = cmd->sccspath;
	   while (*cp != '\0') {
	      while (*cp == ' ')
		 cp++;
	      while (*cp != ' ' && *cp != '\0' &&
		    *cp != '/' && *p != '%' && *cp != ':')
			cp++;
	      if (*cp == '\0')
	         continue;
	      if (*cp == ':') {
		 if (cp_opstr == NULL) {
		    macro_opstr_p = cp_opstr = &macro_opstr[0];
		 }
		 cp++;
		 while (*cp != '\0' && *cp != '/' && *p != '%' && *cp != ' ')
		    *cp_opstr++ = *cp++;
	      } else {
	         cp++;
	      }
	   }
	   if (cp_opstr != NULL) {
		*cp_opstr = '\0';
	   }
	}

	/*
	**  Copy remaining arguments doing editing as appropriate.
	*/

	cnt_files_from_stdin = 0;
	for (; *argv != NULL; argv++) {
		p = *argv;
		if (*p == '-') {
		   if (p[1] == '\0') {
		      struct stat _Statbuf;
		      char *ibuf, *str;
		      DIR  *dirf;
		      struct dirent *dir;
		      extern char *Ffile;

		      ibuf = malloc(BUFSIZ);
		      if (ibuf == NULL) {
			 perror(gettext("Sccs: no mem"));
			 exit(EX_OSERR);
		      }
		      while (fgets(ibuf, BUFSIZ, stdin) != NULL) {
			 char *cp;
			 int  nline;

			 nline = 0;
			 for (cp = ibuf; *cp != '\0'; cp++) {
				if (*cp == '\n') {
					*cp = '\0';
					nline = 1;
					break;
				}
			 }
			 if (!nline) {
				usrerr(gettext("bad file name \"%s\""), ibuf);
				exit(1);
			 }
			 if (_exists(ibuf)&&(_Statbuf.st_mode&S_IFMT) == S_IFDIR) {
			    Ffile = ibuf;
			    if ((dirf = opendir(ibuf)) == NULL)
			       break;
			    while ((dir = readdir(dirf)) != NULL) {
				if (dot_dotdot(dir->d_name))
					continue;
#ifdef	HAVE_DIRENT_D_INO
			       if (dir->d_ino == 0)
				  continue;
#endif
			       str = malloc(BUFSIZ);
			       if (str == NULL) {
			          perror(gettext("Sccs: no mem"));
			          exit(EX_OSERR);
			       }
			       sprintf(str, "%s/%s", ibuf, dir->d_name);
			       get_list_files(&listftailp, str, no_sdot);
			       cnt_files_from_stdin++;
			    }
			    closedir(dirf);
			 } else {
			    get_list_files(&listftailp, ibuf, no_sdot);
			    cnt_files_from_stdin++;
			    ibuf = malloc(BUFSIZ);
		            if (ibuf == NULL) {
			       perror(gettext("Sccs: no mem"));
			       exit(EX_OSERR);
		            }
			 }
		      }
	           } else {
	              char **pp = NULL;

	              if (macro_opstr_p != 0) {
	                 if (strchr(macro_opstr_p, p[1]) == NULL) {
			    usrerr("%s %s", gettext("unknown option"), p);
			    exit(EX_USAGE);
			 }
		      }
	              if (editchs == NULL || strchr(editchs, p[1]) != NULL) {
			 pp = np;
			 *np++ = p;
		      }
		      nextp = *(argv+1);
		      if (p[2] == '\0' && nextp != 0 && *nextp != '-') {
			if ((strcmp(maincmd->sccsname,"print")  != 0) &&
			    (strcmp(maincmd->sccsname,"prt")    != 0) &&
			    (strcmp(maincmd->sccsname,"branch") != 0) &&
			    (strcmp(maincmd->sccsname,"vc")     != 0)) {
                         switch(p[1]) {
			 case 'x':
			 case 'c':
			    if (strcmp(cmd->sccsname,"sccsdiff") != 0) {
			       if (editchs != NULL
			            && strchr(editchs, p[1]) == NULL) {
			          argv++;
			       } else {
			          *np++ = *++argv;
			       }
			    }
		            break;
			 case 'u':
			    if ((strcmp(cmd->sccsname,"sccsdiff") != 0) &&
				(strcmp(cmd->sccsname,"diffs") != 0) &&
				(strcmp(cmd->sccsname,"-diff") != 0)) {
			       if (editchs != NULL
			            && strchr(editchs, p[1]) == NULL) {
			          argv++;
			       } else {
			          *np++ = *++argv;
			       }
			    }
		            break;
			 case 'a':
			    if ((strcmp(cmd->sccsname,"prs") != 0) &&
			        (strcmp(cmd->sccsname,"log") != 0)) {
			       if (editchs != NULL
			            && strchr(editchs, p[1]) == NULL) {
			          argv++;
			       } else {
			          *np++ = *++argv;
			       }
			    }
		            break;
			 case 'e':
			    if ((strcmp(cmd->sccsname,"get")         != 0) &&
			        (strcmp(maincmd->sccsname,"sccsdiff") != 0) &&
			        (strcmp(maincmd->sccsname,"diffs")   != 0) &&
			        (strcmp(maincmd->sccsname,"deledit") != 0) &&
			        (strcmp(maincmd->sccsname,"delget")  != 0) &&
			        (strcmp(maincmd->sccsname,"create")  != 0) &&
			        (strcmp(maincmd->sccsname,"enter")   != 0) &&
			        (strcmp(cmd->sccsname,"prs")         != 0)) {
			       if (editchs != NULL
			            && strchr(editchs, p[1]) == NULL) {
			          argv++;
			       } else {
			          *np++ = *++argv;
			       }
			    }
		            break;
			 case 'f':
			    if ((strcmp(maincmd->sccsname,"create") == 0) ||
			        ((strcmp(maincmd->sccsname,"diffs") != 0) &&
			        (strcmp(maincmd->sccsname,"sccsdiff") != 0) &&
			        (strcmp(cmd->sccsname,"delta") != 0) &&
			        (strcmp(cmd->sccsname,"get") != 0))) {
			       if (editchs != NULL
			            && strchr(editchs, p[1]) == NULL) {
			          argv++;
			       } else {
			          *np++ = *++argv;
			       }
			    }
		            break;
			 case 'i':
			    if ((strcmp(cmd->sccsname,"admin") != 0) &&
			        (strcmp(maincmd->sccsname,"sccsdiff") != 0)) {
			       if (editchs != NULL
			            && strchr(editchs, p[1]) == NULL) {
			          argv++;
			       } else {
			          *np++ = *++argv;
			       }
			    }
		            break;
			 case 'g':
			    if ((strcmp(cmd->sccsname, "get") != 0)) {
			       if (editchs != NULL
			            && strchr(editchs, p[1]) == NULL) {
			          argv++;
			       } else {
			          *np++ = *++argv;
			       }
			    }
		            break;
			 case 'r':
			    if (strcmp(cmd->sccsname, "prs") != 0) {
			       if (strcmp(cmd->sccsname, "get") == 0) {
				  if (*(omit_sid(nextp)) != '\0') {
				     break;
				  }
			       }
			       if (editchs != NULL
			            && strchr(editchs, p[1]) == NULL) {
			          argv++;
			       } else {
			          *np++ = *++argv;
			       }
			    }
		            break;
			 case 'm':
			    if ((strcmp(maincmd->sccsname,"deledit") == 0) ||
			        (strcmp(maincmd->sccsname,"delget")  == 0) ||
			        (strcmp(maincmd->sccsname,"create")  == 0) ||
			        (strcmp(cmd->sccsname, "get") != 0)) {
			       if (editchs != NULL
			            && strchr(editchs, p[1]) == NULL) {
			          argv++;
			       } else {
			          *np++ = *++argv;
			       }
			    }
		            break;
			 case 'd':
			    if ((strcmp(cmd->sccsname,"prs")    == 0) ||
			        (strcmp(cmd->sccsname,"admin")  == 0) ||
			        (strcmp(maincmd->sccsname,"create") == 0) ||
			        (strcmp(cmd->sccsname, "enter")  == 0)) {
			       if (editchs != NULL
			            && strchr(editchs, p[1]) == NULL) {
			          argv++;
			       } else {
			          *np++ = *++argv;
			       }
			    }
		            break;
			 case 'p':
			    if (strcmp(cmd->sccsname, "comb") == 0) {
			       if (editchs != NULL
			            && strchr(editchs, p[1]) == NULL) {
			          argv++;
			       } else {
			          *np++ = *++argv;
			       }
			    }
		            break;
			 case 'y':
			    if (strcmp(cmd->sccsname, "val") == 0) {
			       if (editchs != NULL
			            && strchr(editchs, p[1]) == NULL) {
			          argv++;
			       } else {
			          *np++ = *++argv;
			       }
			    }
			    hady = 1;
		            break;
			 case 'G':
			 case 'w':
			    if (strcmp(cmd->sccsname, "get") == 0) {
			       if (editchs != NULL
			            && strchr(editchs, p[1]) == NULL) {
			          argv++;
			       } else {
			          *np++ = *++argv;
			       }
			    }
		            break;
			 case 'C':
			 case 'D':
			    if ((strcmp(cmd->sccsname,"sccsdiff") == 0) ||
				(strcmp(cmd->sccsname,"get")      == 0) ||
				(strcmp(cmd->sccsname,"diffs")    == 0)) {
			       if (editchs != NULL
			            && strchr(editchs, p[1]) == NULL) {
			          argv++;
			       } else {
			          *np++ = *++argv;
			       }
			    }
		            break;
			 case 'U':
			    if ((strcmp(cmd->sccsname,"sccsdiff") == 0) ||
				(strcmp(cmd->sccsname,"get")      == 0) ||
				(strcmp(cmd->sccsname,"diffs")    == 0) ||
				(strcmp(cmd->sccsname,"-diff")    == 0)) {
			       if (editchs != NULL
			            && strchr(editchs, p[1]) == NULL) {
			          argv++;
			       } else {
			          *np++ = *++argv;
			       }
			    }
		            break;
			 default:
			    break;
			 }
			}
		      }
		      if (!hady && strncmp(p, "-y", 2) == 0) {
			 if (strcmp(cmd->sccsname, "deledit") == 0 ||
			     strcmp(cmd->sccsname, "delget")  == 0)
				hady = 1;
		      }
		      if (strcmp(p,"-C") == 0) {
			 if (strcmp(cmd->sccsname, "-diff") == 0) {
			    if (pp != NULL)
			       *pp = "-c";
			 }
		      }
		      if (strcmp(p,"-I") == 0) {
			 if (strcmp(cmd->sccsname, "-diff") == 0) {
			    if (pp != NULL)
			       *pp = "-i";
			 }
		      }
		      pp = NULL;
		   }
		} else {
		   get_list_files(&listftailp, p, no_sdot);
		}
	}
	if (cnt_files_from_stdin) {
		char ** new_nav;

		nav_size += cnt_files_from_stdin;
		if ((new_nav = realloc(nav, nav_size * (sizeof (char *)))) == NULL) {
			perror(gettext("Sccs: no mem"));
			exit(EX_OSERR);
		}
		np  = new_nav + (np - nav);
		nav = new_nav;
		ap  = &new_nav[1];
	}
#ifdef	USE_RECURSIVE
	if (Rflag > 0) {
		np[0] = NULL;
		if (!hady &&
		    (strcmp(cmd->sccsname, "delta")  == 0 ||
		    strcmp(cmd->sccsname, "deledit") == 0 ||
		    strcmp(cmd->sccsname, "delget")  == 0)) {
			if (Comments == NULL)
				get_sccscomment();
			*np++ = Comments;
			hady = 1;
		}
		np[1] = NULL;

		listfilesp = head_files.next;
		if (listfilesp == NULL)
			rval |= dorecurse(ap, np, ".", cmd);
		else while (listfilesp != NULL) {
			if (cmd->sccsoper == CLEAN && listfilesp->next) {
				usrerr(gettext("too many args"));
				return (EX_USAGE);
			}
			rval |= dorecurse(ap, np, listfilesp->filename, cmd);
			listfilesp = listfilesp->next;
		}
		if (cmd->sccsoper == CLEAN && cmd->sccspath == (char *)INFOC &&
		    !Tgotedit) {
			nothingedited(Tnobranch, Tusernm);
		}
		return (rval);
	}
	if (Rflag && bitset(COLLECT, cmd->sccsflags) &&
	    strcmp(cmd->sccsname, "log") == 0) {
		*np++ = "-p";
		*np++ = SccsPath;
	} else if (Cwd && Cwd[2] && cmd->sccsoper == PROG &&
	    (strcmp(cmd->sccsname, "get")  == 0 ||
	    strcmp(cmd->sccsname, "delta") == 0)) {
		*np++ = Cwd;
	}
#endif
#ifdef	SHELL
	if (NewMode && (cmd->sccsoper == PROG || cmd->sccsoper == SHELL) &&
#else
	if (NewMode && cmd->sccsoper == PROG &&
#endif
	    (cmd->sccsflags & NO_N) == 0)
		*np++ = NewOpt;
	nfiles = 0;
	listfilesp = head_files.next;
	while (listfilesp != 0) {
	   if (!no_sdot) {
	      *np++ = listfilesp->s_filename;
	   } else {
	      *np++ = listfilesp->filename;
	   }
	   listfilesp = listfilesp->next;
	   nfiles++;
	}
	*np = NULL;
	ac = np - ap;

	/*
	**  Interpret operation associated with this command.
	*/

	switch (cmd->sccsoper)
	{
#ifdef V6
	  case SHELL:		/* call a shell file */
		*ap = cmd->sccspath;
		*--ap = NOGETTEXT("sh");
		rval = callprog(NOGETTEXT("/bin/sh"), cmd->sccsflags, ap, forkflag);
		break;
#endif

	  case PROG:		/* call an sccs prog */
		if (create_macro == 1 && strcmp(cmd->sccsname, "get") == 0) {
			/*
			 * The "sccs create ..." macro is running.
			 */
			char Gname[FILESIZE];
			char *  gfp;
			int     ind, ind1 = 0;

			for (ind = 0; ap[ind] != NULL; ind++) {
				ind1 = ind;
			}
			ind = ind1;
			size_ap_for_get = ind + 3;
			if (ap_for_get == NULL) {
				if ((ap_for_get = malloc(size_ap_for_get * (sizeof (char *)))) == NULL) {
					perror(gettext("Sccs: no mem"));
					      exit(EX_OSERR);
				}
				for (ind1 = 0; ind1 < ind; ind1++) {
					ap_for_get[ind1] = ap[ind1];
				}
			}
			/*
			 * Get length of possible prefix before SCCS/s.file
			 * to construct the full path name for the -G option.
			 */
			if (NewMode) {
			        strcpy(Gname, "-G");	/* Start -G option */
			        strcat(Gname, ap[ind]);	/* Copy g-file name */
			} else {
				gfp = auxf(ap[ind], 'g');
				strcpy(Gname, SccsPath);
				strcat(Gname, "/s.");
				strcat(Gname, gfp);
				len = strlen(ap[ind]) - strlen(Gname);
				strcpy(Gname, "-G");	/* Start -G option */
				strncat(Gname, ap[ind], /* Copy path base */
				    len);
				strcat(Gname, gfp);	/* Append g-file name */
			}
			ap_for_get[size_ap_for_get - 3] = Gname;
			ap_for_get[size_ap_for_get - 2] = ap[ind];
			ap_for_get[size_ap_for_get - 1] = NULL;
		        rval = callprog(cmd->sccspath, cmd->sccsflags, ap_for_get, TRUE);
		} else {
			if (del_macro == 1) {
				/*
				 * The "sccs deledit ..." or "sccs delget ..."
				 * macro is running.
				 */
				int	ind, ind1;
				char ** Arr = NULL;

				for (ind = ind1 = 0; ap[ind] != NULL; ind++) {
					ind1 = ind;
				}
				ind = ind1;
				if (!isdir(ap[ind])) {
					Arr = ArrSids;
					if (macro_files != NULL) {
						Arr += cur_num_file;
					}
				}
				if (strcmp(cmd->sccsname, "delta") == 0) {
					/* first part of del_macro (deledit or delget) */
					if (Arr != NULL) {
						*Arr = getNsid(ap[ind], user_name);
					}
					rval = callprog(cmd->sccspath, cmd->sccsflags, ap, TRUE);
				} else {
					/* second part of del_macro (deledit or delget) */
					if (ap_for_get == NULL) {
						size_ap_for_get = ind + 3;
						if ((ap_for_get = malloc(size_ap_for_get * (sizeof (char *)))) == NULL) {
							perror(gettext("Sccs: no mem"));
								exit(EX_OSERR);
						}
						for (ind1 = 0; ind1 < ind; ind1++) {
							ap_for_get[ind1] = ap[ind1];
						}
						ap_for_get[size_ap_for_get - 1] = NULL;
					}
					if (isdir(ap[ind])) {
						  ap_for_get[size_ap_for_get - 3] = ap[ind];
						  ap_for_get[size_ap_for_get - 2] = NULL;
					} else {
						/* 'delta' command closed delta of file. */
						/* its necessary to run get command */
						ap_for_get[size_ap_for_get - 3] = *Arr;

						/*
						 * If we call "delget -f -q", we
						 * have no p. file and thus *Arr
						 * is NULL. Do not add a sid
						 * argument in this case, but
						 * hope that get -t will do.
						 * Check out with -k to keep the
						 * file writable.
						 */
						if (*Arr == NULL)
							ap_for_get[size_ap_for_get - 3] = "-k";
						ap_for_get[size_ap_for_get - 2] = ap[ind];
					}
					rval = callprog(cmd->sccspath, cmd->sccsflags, ap_for_get, TRUE);
				}
			} else {
				/*
				 * All normal program calls.
				 */
				rval = callprog(cmd->sccspath, cmd->sccsflags, ap, forkflag);
			}
		}
		break;

	  case CMACRO:		/* command macro */
		{
		int cnt, ind, xsize, first_part_macro = 0, macro_rval = 0;
		char **ap1 = NULL, **next_file;
		char **cp,  **file_arg = NULL;

		/* step through & execute each part of the macro */
		ap_for_get = NULL;
		if (strcmp(cmd->sccsname, "create") == 0) {
			create_macro = 1;
		} else {
			if (strcmp(cmd->sccsname, "deledit") == 0 ||
			    strcmp(cmd->sccsname, "delget")  == 0) {

				del_macro = 1;
				if ((user_name = logname()) == NULL)
					fatal(gettext("User ID not in password file (cm9)"));
				for (ind = 0; ap[ind] != NULL; ind++) {
					if (!r_option_value) {
						/* in search of '-r' option */
						if (strstr(ap[ind], "-r") != NULL) {
							if (strcmp(ap[ind], "-r")) {
								r_option_value = xstrdup(ap[ind]);
							} else {
								if (ap[ind+1] != NULL)
									r_option_value = xstrdup(ap[ind+1]);
							}
						}
					}
				}
				if (nfiles > 0) {
					NelemArrSids = nfiles;
					if ((ArrSids = calloc(NelemArrSids, sizeof (char *))) == NULL) {
						perror(gettext("Sccs: no mem"));
						exit(EX_OSERR);
					}
				}
			} else {
				create_macro  = 0;
				del_macro = 0;
			}
		}
		if (nfiles > 1) {
			first_part_macro = 1;
		}

		/*
		 * Select the macro string for NewMode if avaliable.
		 */
		p = cmd->sccspath;
		if (NewMode && (q = strchr(p, '%')) != NULL)
			p = ++q;
		for (; *p != '\0'; p++)
		{
			if (!forkflag)		/* Keep FORCE_FORK value */
				forkflag = TRUE;
			q = p;
			while (*p != '\0' && *p != '/' && *p != '%')
				p++;
			if (*p == '\0' || *p == '%') {
				if (nfiles == 1 && !Rflag && forkflag == TRUE) {
					forkflag = FALSE;
				}
				/*
				 * In case command() returns, we need to check
				 * the string end before for(;;) increments p.
				 */
			}
			if (nfiles > 1) {
				if (first_part_macro) {
					macro_rval = first_part_macro = 0;
					for (cnt = 0; ap[cnt] != NULL; cnt++)
						;
					xsize = cnt - nfiles + 2;
					if (!hady) {
						if (strcmp(cmd->sccsname, "deledit") == 0 ||
						    strcmp(cmd->sccsname, "delget")  == 0) {
							/* additional element for '-ycomments' parameter */
							xsize++;
						}
					}
					if ((macro_files = calloc(nfiles, (sizeof (char *)))) == NULL ||
						   (ap1  = calloc(xsize, (sizeof (char *)))) == NULL) {
						perror(gettext("Sccs: no mem"));
						exit(EX_OSERR);
					}
					for (ind = 0; ind < (cnt - nfiles); ind++) {
						ap1[ind] = ap[ind];
					}
					if (!hady) {
						if (strcmp(cmd->sccsname, "deledit") == 0 ||
						    strcmp(cmd->sccsname, "delget")  == 0) {
							if (Comments == NULL)
								get_sccscomment();
							ap1[xsize - 3] = Comments;
						}
					}
					next_file = ap  + cnt  - nfiles;
					file_arg  = ap1 + xsize - 2;
				} else {
					next_file = macro_files;
				}
				cp = macro_files;
				for (ind = 0; ind < nfiles; ind++) {
					if (*next_file != NULL) {
						cur_num_file = ind;
						*file_arg = *next_file;
						if ((rval = command(&ap1[1], forkflag, q)) != 0) {
							macro_rval = rval;
							*cp	   = NULL;
						} else {
							*cp = *next_file;
						}
					}
					cp++;
					next_file++;
				}
			} else {
				if ((rval = command(&ap[1], forkflag, q)) != 0)
					break;
			}
			/*
			 * Stop at the end of the string.
			 * If we are not in NewMode, stop at the end of the
			 * old macro.
			 */
			if (*p == '\0' || *p == '%')
				break;
		}
		if (nfiles > 1) {
			rval = macro_rval;
			free(ap1);
			free(macro_files);
		}
		if (ap_for_get != NULL) {
			free(ap_for_get);
		}
		if (Comments != NULL && !Rflag) {
			free(Comments);
			Comments = NULL;
		}
		break;
		}

	  case FIX:		/* fix a delta */
		rval = fix(nfiles, ap);
		break;

	  case CLEAN:		/* clean out recreatable files */
		rval = clean((int) (Intptr_t)cmd->sccspath, ap);
		break;

	  case UNEDIT:
		rval = unedit(nfiles, ap);
		break;

	  case DIFFS:		/* diff between s-file & edit file */
		rval = diffs(nfiles, ap);
		break;

	  case ENTER:		/* enter new sccs files */
		rval = enter(nfiles, ap);
		break;

	  case EDITOR:		/* get -e + call $EDITOR */
		rval = editor(nfiles, ap);
		break;

	  case HISTFILE:	/* give history file path for s-file */
		rval = histfile(nfiles, ac, ap);
		break;

	  case ISTEXT:		/* check whether file needs encoding */
		rval = istext(nfiles, ac, ap);
		break;

	  case ADD:		/* add specified files on next commit  */
		rval = addcmd(nfiles, ac, ap);
		break;

	  case COMMIT:		/* commit changes to project repository */
		rval = commitcmd(nfiles, ac, ap);
		break;

	  case INIT:		/* initialize empty project repository */
		rval = initcmd(nfiles, ac, ap);
		break;

	  case REMOVE:		/* remove specified files on next commit  */
		rval = removecmd(nfiles, ac, ap);
		break;

	  case RENAME:		/* rename specified files on next commit  */
		rval = renamecmd(nfiles, ac, ap);
		break;

	  case ROOT:		/* show project root directory */
		rval = rootcmd(nfiles, ac, ap);
		break;

	  case STATUS:		/* show changed files in the project */
		rval = statuscmd(nfiles, ac, ap);
		break;

	  default:
		syserr("oper %d (sc2)", cmd->sccsoper);
		exit(EX_SOFTWARE);
	}
#ifdef DEBUG
	if (Debug)
		printf(gettext("command: rval=%d\n"), rval);
#endif
	free(nav);
	fflush(stdout);
	return (rval);
}

static void
get_sccscomment()
{
	if (Comments == NULL) {
		char * ccp;

		if (isatty(0) == 1)
			printf(gettext("comments? "));
		ccp = get_Sccs_Comments();
		if ((Comments = malloc(strlen(ccp) + 3)) == NULL) {
			perror(gettext("Sccs: no mem"));
			exit(EX_OSERR);
		}
		strcpy(Comments, "-y");
		strcat(Comments, ccp);
		free(ccp);
	}
}

static void
get_list_files(listftailpp, filename, no_sdot)
	struct list_files	**listftailpp;
	char			*filename;
	bool			no_sdot;
{
	struct list_files *listfilesp = *listftailpp;

	listfilesp->next = malloc(sizeof (struct list_files));
	if (listfilesp->next == NULL) {
		perror(gettext("Sccs: no mem"));
		exit(EX_OSERR);
	}
	listfilesp = listfilesp->next;
	listfilesp->next = NULL;
	listfilesp->filename = filename;
	if (!no_sdot) {
		filename = makefile(filename, SccsDir);
	}
	listfilesp->s_filename = filename;
	*listftailpp = listfilesp;
}

/*
**  LOOKUP -- look up an SCCS command name.
**
**	Parameters:
**		name -- the name of the command to look up.
**
**	Returns:
**		ptr to command descriptor for this command.
**		NULL if no such entry.
**
**	Side Effects:
**		none.
*/

static struct sccsprog *
lookup(name)
	char *name;
{
	register struct sccsprog *cmd;

	for (cmd = SccsProg; cmd->sccsname != NULL; cmd++)
	{
		if (strcmp(cmd->sccsname, name) == 0)
			return (cmd);
	}
	return (NULL);
}

/*
**  CALLPROG -- call a program
**
**	Used to call the SCCS programs.
**
**	Parameters:
**		progpath -- pathname of the program to call.
**		flags -- status flags from the command descriptors.
**		argv -- an argument vector to pass to the program.
**		forkflag -- if true, fork before calling, else just
**			exec.
**
**	Returns:
**		The exit status of the program.
**		Nothing if forkflag == FALSE.
**
**	Side Effects:
**		Can exit if forkflag == FALSE.
*/

static int
callprog(progpath, flags, argv, forkflag)
	char *progpath;
	short flags;
	char **argv;
	bool forkflag;
{
	register int i;
	register int wpid;
	auto int st;
	register int sigcode;
	register int coredumped;
	register const char *sigmsg;
#ifndef	HAVE_STRSIGNAL
#ifdef	HAVE_SYS_SIGLIST
	auto char sigmsgbuf[10+1];	/* "Signal 127" + terminating '\0' */
#endif
#endif

#ifdef DEBUG
	if (Debug)
	{
		printf("callprog:\n");
		for (i = 0; argv[i] != NULL; i++)
			printf("\t\"%s\"\n", argv[i]);
		/*
		 * Avoid flush() problems caused by fork()/vfork()
		 */
		if (forkflag)
			fflush(stdout);
	}
#endif

	if (*argv == NULL)
		return (-1);

	/*
	**  Fork if appropriate.
	*/

	if (forkflag)
	{
#ifdef DEBUG
		if (Debug)
			printf("Forking\n");
#endif
		i = vfork();
		if (i < 0) {
			syserr(gettext("cannot fork"));
			exit(EX_OSERR);
		} else if (i > 0) {
			while ((wpid = wait(&st)) != -1 && wpid != i)
				;
			if (WIFEXITED(st))
				st = WEXITSTATUS(st);
			else
			{
				coredumped = WCOREDUMP(st);
				sigcode =  WTERMSIG(st);
#ifdef	SIGPIPE
				if (sigcode != SIGINT && sigcode != SIGPIPE)
#else
				if (sigcode != SIGINT)
#endif
				{
#ifdef	HAVE_STRSIGNAL
					sigmsg = strsignal(sigcode);
#else
#ifdef	HAVE_SYS_SIGLIST
					if (sigcode < NSIG)
						sigmsg = sys_siglist[sigcode];
					else
					{
						sprintf(sigmsgbuf, "%s %d",
							gettext("Signal"),
							sigcode);
						sigmsg = sigmsgbuf;
					}
#else
	/* bandaid */
	sigmsg = gettext("fork() error");
#endif
#endif	/* HAVE_STRSIGNAL */
					fprintf(stderr, "sccs: %s: %s%s\n", argv[0],
					    sigmsg,
					    coredumped ? gettext(" - core dumped") : "");
				}
				st = EX_SOFTWARE;
			}
			if (OutFile >= 0)
			{
				close(OutFile);
				OutFile = -1;
			}
			return (st);
		}
#ifdef	HAVE_VFORK
		didvfork = 1;
#endif
	} else if (OutFile >= 0) {		/* !forkflag && ... */

		syserr(gettext("callprog: setting stdout w/o forking"));
		if (didvfork)
			_exit(EX_SOFTWARE);
		exit(EX_SOFTWARE);
	}

	/* set protection as appropriate */
	if (bitset(REALUSER, flags))
		if (setuid(getuid()) < 0)
			syserr(gettext("Cannot set uid"));

	/* change standard input & output if needed */
	if (OutFile >= 0)
	{
#ifdef	set_child_standard_fds
		set_child_standard_fds(STDIN_FILENO,
				OutFile,
				STDERR_FILENO);
#else
		close(1);
		(void) dup(OutFile);
		close(OutFile);
#endif
	}

	/* call real SCCS program */
#ifdef DEBUG
	if (Debug) {
		printf("exec: %s\n", argv[0]?argv[0]:"(NULL)");
		printf("progpath: %s\n", progpath);
		for (i = 0; argv[i] != NULL; i++)
			printf("\t\"%s\"\n", argv[i]);
		fflush(stdout);
	}
#endif
	if (getenv("SCCS_NOEXEC"))
		_exit(0);
#ifndef V6
	execvp(progpath, argv);
#else
	execv(progpath, argv);
#endif /* V6 */
	syserr(gettext("cannot execute %s"), progpath);
	if (didvfork)
		_exit(EX_UNAVAILABLE);
	exit(EX_UNAVAILABLE);
	/*NOTREACHED*/
}

/*
**  MAKEFILE -- make filename of SCCS file
**
**	If the name passed is already the name of an SCCS file,
**	just return it.  Otherwise, munge the name into the name
**	of the actual SCCS file.
**
**	There are cases when it is not clear what you want to
**	do.  For example, if SccsPath is an absolute pathname
**	and the name given is also an absolute pathname, we go
**	for SccsPath (& only use the last component of the name
**	passed) -- this is important for security reasons (if
**	sccs is being used as a setuid front end), but not
**	particularly intuitive.
**
**	Parameters:
**		name -- the file name to be munged.
**
**	Returns:
**		The pathname of the sccs file.
**		NULL on error.
**
**	Side Effects:
**		none.
*/

static char *
makefile(name, in_SccsDir)
	char		*name;
	const char	*in_SccsDir;
{
	register char *p;
	char buf[3*FBUFSIZ];
	register char *q;
	int Spath = FALSE;
	char *Sp, *np;
	struct stat _Statbuf;

	np = p = strrchr(name, '/');
	if (p == NULL) {
		p = name;
	} else {
		p++;
	}
	if (strcmp(p, SccsPath) == 0) {
		Spath = TRUE;
	} else {
		if (p != name) {
			/*
			 * If we do not check for /s., we get funny results
			 * for SCCS/a.file. If we do, we get SCCS/SCCS/s.a.file
			 * but if we like to use the new option -NSCCS, we
			 * should include the test.
			 */
			if (np[1] == 's' && np[2] == '.' &&
			    (Sp = strstr(name, SccsPath)) != 0) {
				if ((Sp+strlen(SccsPath)) == np) {
					Spath = TRUE;
				}
			}
		}
	}

	/*
	**  Check to see that the path is "safe", i.e., that we
	**  are not letting some nasty person use the setuid part
	**  of this program to look at or munge some presumably
	**  hidden files.
	*/

	if (in_SccsDir[0] == '/' && !safepath(name))
		return (NULL);

	/*
	**  Create the base pathname.
	*/

	/*
	 * first the directory part
	 */
	if ((in_SccsDir[0] != '\0') &&
	    (name[0] != '/') &&
	    (strncmp(name, "./", 2) != 0)) {
		gstrcpy(buf, in_SccsDir, sizeof (buf));
		gstrcat(buf, "/", sizeof (buf));
	} else {
		gstrcpy(buf, "", sizeof (buf));
	}

	/*
	 * then the head of the pathname
	 */
	gstrncat(buf, name, p - name, sizeof (buf));
	q = &buf[strlen(buf)];

	/*
	 * now copy the final part of the name, in case useful
	 */
	gstrcpy(q, p, sizeof (buf));

	/*
	 * so is it useful?
	 */
	if (Spath == FALSE && !NewMode) {
		if (strncmp(p, "s.", 2) != 0) {
			/*
			 * Definitely not a s.file name.
			 */
			if ((strcmp(curcmd->sccsname, "create") == 0) ||
			    (strcmp(curcmd->sccsname, "enter") == 0)  ||
			    (strcmp(curcmd->sccsname, "editor") == 0) ||
			    (strcmp(curcmd->sccsname, "admin") == 0)  ||
			    (isdir(buf) == 0)) {
				gstrcpy(q, SccsPath, sizeof (buf));
				gstrcat(buf, "/s.", sizeof (buf));
				gstrcat(buf, p, sizeof (buf));
			} else if (strcmp(curcmd->sccsname, "histfile") == 0)
				gstrcpy(q, SccsPath, sizeof (buf));
		} else {
			/*
			 * May be a s.file name, but a g-file may also start
			 * with "s.".
			 */
			if ((strcmp(curcmd->sccsname, "create") == 0) ||
			    (strcmp(curcmd->sccsname, "enter") == 0)  ||
			    (strcmp(curcmd->sccsname, "editor") == 0) ||
			    (strcmp(curcmd->sccsname, "admin") == 0)) {
				/*
				 * If it is related to new files, assume a
				 * g-file and add SCCS/s. before the final name.
				 */
				gstrcpy(q, SccsPath, sizeof (buf));
				gstrcat(buf, "/s.", sizeof (buf));
				gstrcat(buf, p, sizeof (buf));
			} else if (isdir(buf) == 0) {
				/*
				 * In other cases first assume a g-file, but
				 * check for the existence of the assumed
				 * s.file.
				 */
				gstrcpy(q, SccsPath, sizeof (buf));
				gstrcat(buf, "/s.", sizeof (buf));
				gstrcat(buf, p, sizeof (buf));
				if (!_exists(buf)) {
					/*
					 * The assumed related s.file is
					 * missing, try the given file name
					 * as s.file name.
					 */
					gstrcpy(q, p, sizeof (buf));
				}
			}
		}
	}

	/*
	 * if i haven't changed it, why did I do all this?
	 */
	if (strcmp(buf, name) == 0) {
		p = name;
	} else {
		/*
		 * but if I have, squirrel it away
		 */
		p = xstrdup(buf);
	}
	return (p);
}

/*
**  ISDIR -- return true if the argument is a directory.
**
**	Parameters:
**		name -- the pathname of the file to check.
**
**	Returns:
**		TRUE if 'name' is a directory, FALSE otherwise.
**
**	Side Effects:
**		none.
*/

static bool
isdir(name)
	char *name;
{
	struct stat stbuf;

	return (stat(name, &stbuf) >= 0 && (stbuf.st_mode & S_IFMT) == S_IFDIR);
}

/*
**  ISFILE -- return true if the argument is a normal file.
**
**	Parameters:
**		name -- the pathname of the file to check.
**
**	Returns:
**		TRUE if 'name' is a normal file, FALSE otherwise.
**
**	Side Effects:
**		none.
*/

static bool
isfile(name)
	char *name;
{
	struct stat stbuf;

	return (stat(name, &stbuf) == 0 && (stbuf.st_mode & S_IFMT) == S_IFREG);
}

/*
**  SAFEPATH -- determine whether a pathname is "safe"
**
**	"Safe" pathnames only allow you to get deeper into the
**	directory structure, i.e., full pathnames and ".." are
**	not allowed.
**
**	Parameters:
**		p -- the name to check.
**
**	Returns:
**		TRUE -- if the path is safe.
**		FALSE -- if the path is not safe.
**
**	Side Effects:
**		Prints a message if the path is not safe.
*/

static bool
safepath(p)
	register char *p;
{
	if (*p != '/') {
		while (strncmp(p, "../", 3) != 0 && strcmp(p, "..") != 0) {
			p = strchr(p, '/');
			if (p == NULL)
				return (TRUE);
			p++;
		}
	}
	usrerr(gettext("You may not use full pathname or \"..\"\n"));
	exit(EX_USAGE);

	/* NOTREACHED */
	return (FALSE);
}

static char *
xstrdup(file)
	char	*file;
{
	char	*dfile;

	dfile = strdup(file);
	if (dfile == NULL) {
		perror(gettext("Sccs: no mem"));
		exit(EX_OSERR);
	}
	return (dfile);
}

static int
fix(nfiles, argv)
	int	nfiles;
	char	**argv;
{
	register char	*p;
	int		rezult;
	int		rval = 0;
	char		**np;
	char		**ap = argv;
	int		n = 0;
	int		rflag = 0;
	char		*sidp = NULL;
	struct	sid	sid;

	/* find the end of the flag arguments */
	for (np = &ap[1]; *np != NULL && **np == '-'; np++) {
		if (**np == '-') {
			if (np[0][1] == 'r') {
				rflag = 1;
				if (np[0][2] == '\0') {
					np++;
					sidp = *np;
				} else {
					sidp = *np + 2;
				}
			}
		}
	}
	if (*np == NULL) {
		usrerr(gettext(" missing file arg (cm3)"));
		rval = EX_USAGE;
		exit(EX_USAGE);
	}
	if (rflag == 0) {
		usrerr(gettext("-r flag needed for fix command"));
		rval = EX_USAGE;
		exit(EX_USAGE);
	}
	sid_ab(sidp, &sid);
	if (sid.s_lev > 0 || sid.s_seq > 0) {
		for (n = length(sidp); n > 0; n--) {
			if (sidp[n] == '.') {
				break;
			}
		}
	}
	argv = np;
	/* for each file, do the fix */
	p = argv[1];
	rezult = 0;
	while (*np != NULL) {
		*argv = *np++;
		argv[1] = NULL;
		rval |= rezult;
		if (nfiles > 1) {
			printf("\n%s:\n", *argv);
			fflush(stdout);
		}
		/* mersy, but we need a null terminated argv */
		/* get the version with all changes */
		rezult = command(&ap[1], TRUE, NOGETTEXT("get: -k"));
		if (rezult != 0) {
			argv[1] = p;
			continue;
		}
		/* now remove that version from the s-file */
		rezult = command(&ap[1], TRUE, NOGETTEXT("rmdel:rd"));
		if (rezult != 0) {
			unlink(*argv);
			argv[1] = p;
			continue;
		}
		/* and edit the old version (but don't clobber new vers) */
		if (n > 0) {
			sidp[n] = '\0';
		}
		rezult = command(&ap[1], TRUE, NOGETTEXT("get:r -e -g"));
		sidp[n] = '.';
		argv[1] = p;
	}
	rval |= rezult;
	return (rval);
}

/*
**  CLEAN -- clean out recreatable files
**
**	Any file for which an "s." file exists but no "p." file
**	exists in the current directory is purged.
**
**	Parameters:
**		mode -- tells whether this came from a "clean", "info",
**			"tell" or "check" command.
**		argv -- the rest of the argument vector.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Removes files in the current directory.
**		Prints information regarding files being edited.
**		Exits if a "check" command.
*/

static int
clean(mode, argv)
	int mode;
	char **argv;
{
	struct dirent *dir;
	char buf[MAXPATHLEN];
	char namefile[MAXPATHLEN];
	char basebuf[MAXPATHLEN];
	char *bufend;
	char *baseend;
	register DIR *dir_fd;
	register char *basefile;
	bool gotedit;
	bool edited;
	bool gotpfent;
	FILE *pfp;
	bool nobranch = FALSE;
	register struct p_file *pf;
	register char **ap;
	char *usernm = NULL;
	char *subdir = NULL;
	struct stat _Statbuf;
	int ex_status;

	/*
	**  Process the argv
	*/

	ex_status = EX_OK;
	for (ap = argv; *++ap != NULL; )
	{
		if (**ap == '-') {
			/* we have a flag */
			switch ((*ap)[1]) {
			case 'b':
				nobranch = TRUE;
				break;

			case 'u':
				if ((*ap)[2] != '\0')
					usernm = sccs_user(&(*ap)[2]);
				else if (Rflag && (ap[1] == NULL || ap[2] == NULL))
					usernm = logname();
				else if (ap[1] != NULL && ap[1][0] != '-')
					usernm = sccs_user(*++ap);
				else
					usernm = logname();
				break;
			case 'U':
				usernm = logname();
				break;
			}
		} else {
			if (subdir != NULL)
				usrerr(gettext("too many args"));
			else
				subdir = *ap;
		}
	}

	/*
	**  Find and open the SCCS directory.
	*/
	if (NewMode) {
		if (subdir == NULL)
			subdir = ".";
		N.n_pflags =  NP_DIR;
		subdir = bulkprepare(&N, subdir);
		if (subdir == NULL) {
			/*
			 * The error is typically
			 * "non directory specified as argument (cm20)"
			 */
			fatal(gettext(bulkerror(&N)));
		}
		N.n_pflags =  0;
		strlcpy(buf, subdir, sizeof (buf));
	} else {
		gstrcpy(buf, SccsDir, sizeof (buf));
		if (buf[0] != '\0')
			gstrcat(buf, "/", sizeof (buf));
		if (subdir != NULL)
			gstrcat(buf, subdir, sizeof (buf));
		bufend = &buf[strlen(buf)-1];
		while (bufend > buf && *bufend == '/')
			*bufend-- = '\0';
		if ((bufend = strstr(buf, SccsPath)) == NULL ||
		    bufend[strlen(SccsPath)] != '\0') {
			if (subdir != NULL)
				gstrcat(buf, "/", sizeof (buf));
			gstrcat(buf, SccsPath, sizeof (buf));
		}
	}
	bufend = &buf[strlen(buf)];

	basebuf[0] = '\0';
	baseend = basebuf;
	if (Rflag) {
		gstrncat(basebuf, buf, bufend - buf - strlen(SccsPath),
			sizeof (basebuf));
		baseend = &basebuf[strlen(basebuf)];
	}

	dir_fd = opendir(buf);
	if (dir_fd == NULL)
	{
		usrerr(gettext("cannot open %s"), buf);
		return (EX_NOINPUT);
	}

	if (!check_permission_SccsDir(buf)) {
		return (EX_NOINPUT);
	}
	/*
	**  Scan the SCCS directory looking for s. files.
	**	gotedit tells whether we have tried to clean any
	**		files that are being edited.
	*/

	gotedit = FALSE;
	while ((dir = readdir(dir_fd)) != NULL) {
		if (strncmp(dir->d_name, "s.", 2) != 0) {
			continue;
		} else {
			*bufend = '\0';
			gstrcpy(namefile, buf, sizeof (namefile));
			gstrcat(namefile, "/", sizeof (namefile));
			gstrcat(namefile, dir->d_name, sizeof (namefile));
		}

		/* got an s. file -- see if the p. file exists */
		gstrcpy(bufend, NOGETTEXT("/p."), sizeof (buf) - (bufend - buf));
		basefile = bufend + 3;
		gstrcpy(basefile, &dir->d_name[2], sizeof (buf) - (basefile - buf));
		gstrcpy(baseend, &dir->d_name[2], sizeof (basebuf) - (baseend - basebuf));

		/*
		**  open and scan the p-file.
		**	'gotpfent' tells if we have found a valid p-file
		**		entry.
		*/

		pfp = fopen(buf, "rb");
		gotpfent = FALSE;
		edited = FALSE;
		if (pfp != NULL)
		{
			/* the file exists -- report it's contents */
			while ((pf = getpfent(pfp)) != NULL)
			{
				edited = TRUE;
				if (nobranch && isbranch(pf->p_nsid))
					continue;
				if (usernm != NULL && strcmp(usernm, pf->p_user) != 0 && mode != CLEANC)
					continue;
				gotedit = TRUE;
				gotpfent = TRUE;
				if (mode == TELLC)
				{
					printf("%s\n", Rflag ? basebuf : basefile);
					break;
				}
				if (checkpfent(pf)) {
					printf(gettext("%12s: being edited: "), Rflag ? basebuf : basefile);
					putpfent(pf, stdout);
				} else {
					fatal(gettext("bad p-file format (co17)"));
				}
			}
			fclose(pfp);
		}

		/* the s. file exists and no p. file exists -- unlink the g-file */
		if (mode == CLEANC && !gotpfent) {
		    if (_exists(basebuf) != 0) {
			if (((_Statbuf.st_mode & (S_IWUSR|S_IWGRP|S_IWOTH)) == 0)||
			    (edited != 0)) {
				unlink(basebuf);
		    } else {
			ex_status = 1;
			fprintf(stderr,
			    gettext("ERROR [%s]: the file `%s' is writable\n"),
			    namefile, Rflag ? basebuf : basefile);
		    }
		   }
		}
	}

	/* cleanup & report results */
	closedir(dir_fd);
	if (!Rflag && !gotedit && mode == INFOC)
		nothingedited(nobranch, usernm);
	Tgotedit  |= gotedit;
	Tnobranch |= nobranch;
	Tusernm    = usernm;

	if (mode == CHECKC)
		return (gotedit);
	else
		return (ex_status);
}

static void
nothingedited(nobranch, usernm)
	bool		nobranch;
	const char	*usernm;
{
	printf(gettext("Nothing being edited"));
	if (nobranch)
/*
TRANSLATION_NOTE
The following message is a possible continuation of the text
"Nothing being edited" ...
*/
		printf(gettext(" (on trunk)"));
	if (usernm == NULL)
		printf("\n");
	else
/*
TRANSLATION_NOTE
The following message is a possible continuation of the text
"Nothing being edited" ...
*/
		printf(gettext(" by %s\n"), usernm);
}

/*
**  ISBRANCH -- is the SID a branch?
**
**	Parameters:
**		sid -- the sid to check.
**
**	Returns:
**		TRUE if the sid represents a branch.
**		FALSE otherwise.
**
**	Side Effects:
**		none.
*/

static bool
isbranch(sid)
	char *sid;
{
	register char *p;
	int dots;

	dots = 0;
	for (p = sid; *p != '\0'; p++)
	{
		if (*p == '.')
			dots++;
		if (dots > 1)
			return (TRUE);
	}
	return (FALSE);
}


static int
unedit(nfiles, argv)
	int	nfiles;
	char	**argv;
{
	int	err;
	int	rval = 0;
	char	**np;
	char	**ap = argv;

	if (!nfiles) {
		usrerr(gettext(" missing file arg (cm3)"));
		rval = EX_USAGE;
		exit(EX_USAGE);
	}
	err = 0;
	for (argv = np = &ap[1]; *argv != NULL; argv++) {
		char *cp;

		if (strcmp(*np, "-o") == 0) { /* Keep get -o option */
			np++;
			continue;
		}
		cp = makefile(*argv, SccsDir);
		if (cp == NULL) {
			err = 1;
			continue;
		}
		do_file(cp, do_unedit, 1, 1, NULL);
		if (!Fcnt)
			*np++ = *argv;
		else
			err = 1;
	}
	*np = NULL;

	/* get all the files that we unedited successfully */
	if (np > &ap[1])
		rval = command(&ap[1], TRUE, NOGETTEXT("get"));

	if (rval == 0)
		rval = err;
	return (rval);
}

/*
**  UNEDIT -- unedit a file
**
**	Checks to see that the current user is actually editting
**	the file and arranges that s/he is not editting it.
**
**	Parameters:
**		fn -- the name of the file to be unedited.
**
**	Returns:
**		TRUE -- if the file was successfully unedited.
**		FALSE -- if the file was not unedited for some
**			reason.
**
**	Side Effects:
**		fn is removed
**		entries are removed from p_file.
*/

static void
do_unedit(fn)
	char *fn;
{
	register FILE *pfp;
	char *gfile, *gfn, *pfn;
	char *Gfile = NULL;
#ifdef	PROTOTYPES
	char   template[] = NOGETTEXT("/tmp/sccsXXXXXX");
#else
	char   *template = NOGETTEXT("/tmp/sccsXXXXXX");
#endif
	static char tfn[20];
	FILE *tfp;
	register char *q;
	bool delete = FALSE;
	bool others = FALSE;
	char *myname;
	struct p_file *pent;
	char buf[PFILELG];

	Fcnt = 1;
	/* make "s." filename & find the trailing component */
	/* assumed that fn is a "s." filename already */
	if (fn == NULL)
		return;

	if (NewMode) {
		/*
		 * In NewMode, "fn" is always the f-file name.
		 */
		gfn = fn;
		fn = bulkprepare(&N, fn);
		if (fn == NULL) {
			/*
			 * The error is typically
			 * "directory specified as s-file (cm14)"
			 */
			fatal(gettext(bulkerror(&N)));
		}
		pfn = xstrdup(fn);
		gfile = gfn;
	} else {
		pfn = xstrdup(fn);
		if (!sccsfile(pfn)) {
			usrerr(gettext("bad file name \"%s\""), fn);
			free(pfn);
			return;
		}
		gfile = auxf(pfn, 'g');
	}
	if (Cwd && Cwd[2]) {
		Gfile = malloc(strlen(&Cwd[2]) + strlen(gfile) + 1);
		if (Gfile == NULL) {
			perror(gettext("Sccs: no mem"));
			exit(EX_OSERR);
		}
		cat(Gfile, &Cwd[2], gfile, (char *)0);
		gfile = Gfile;
	}
	q = strrchr(pfn, '/');
	if (q == NULL)
		q = &pfn[-1];

	/* turn "s." into "p." & try to open it */
	*++q = 'p';

	pfp = fopen(pfn, "rb");
	if (pfp == NULL)
	{
		printf(gettext("%12s: not being edited\n"),
			gfile);
		free(pfn);
		if (Gfile)
			free(Gfile);
		return;
	}

	/* create temp file for editing p-file */
	strcpy(tfn, template);
#ifdef	HAVE_MKSTEMP
	tfp = fdopen(mkstemp(tfn), "wb");
#else
	mktemp(tfn);
	tfp = fopen(tfn, "wb");
#endif
	if (tfp == NULL)
	{
		usrerr(gettext("cannot create \"%s\""), tfn);
		exit(EX_OSERR);
	}
	setmode(fileno(tfp), O_BINARY);

	/* figure out who I am */
	myname = logname();

	/*
	**  Copy p-file to temp file, doing deletions as needed.
	*/

	while ((pent = getpfent(pfp)) != NULL)
	{
		if (strcmp(pent->p_user, myname) == 0)
		{
			/* a match */
			delete++;
			if (delete > 1) {
				printf(gettext(
				"%s: more than one delta of a file is checked out. Use 'sccs unget -r<SID>' instead.\n"),
					gfile);
				fclose(tfp);
				fclose(pfp);
				unlink(tfn);
				free(pfn);
				if (Gfile)
					free(Gfile);
				return;
			}
		}
		else
		{
			if (checkpfent(pent)) {
				/* output it again */
				putpfent(pent, tfp);
				others++;
			} else {
				fatal(gettext("bad p-file format (co17)"));
			}
		}
	}

	/*
	 * Before changing anything, make sure we can remove
	 * the file in question (assuming it exists).
	 */
	if (delete) {
		errno = 0;
		if (access(gfile, 0) < 0 && errno != ENOENT)
			goto bad;
		if (errno == 0)
			/*
			 * This is wrong, but the rest of the program
			 * has built in assumptions about "." as well,
			 * so why make unedit a special case?
			 */
			if (access(".", 2) < 0) {
	bad:
				printf(gettext("%12s: can't remove\n"), gfile);
				fclose(tfp);
				fclose(pfp);
				unlink(tfn);
				free(pfn);
				if (Gfile)
					free(Gfile);
				return;
			}
	}
	/* do final cleanup */
	if (others)
	{
		/* copy it back (perhaps it should be linked?) */
		if (freopen(tfn, "rb", tfp) == NULL)
		{
			syserr(gettext("cannot reopen \"%s\""), tfn);
			exit(EX_OSERR);
		}
		if (freopen(pfn, "wb", pfp) == NULL)
		{
			usrerr(gettext("cannot create \"%s\""), pfn);
			free(pfn);
			if (Gfile)
				free(Gfile);
			return;
		}
		while (fgets(buf, sizeof (buf), tfp) != NULL) {
			if (fputs(buf, pfp) == EOF) {
				xmsg(pfn, NOGETTEXT("unedit"));
			}
		}
	}
	else
	{
		/* it's empty -- remove it */
		if (unlink(pfn) == -1)
		{
			syserr(gettext("cannot remove \"%s\""), pfn);
			exit(EX_OSERR);
		}
	}
	fclose(tfp);
	fclose(pfp);
	unlink(tfn);

	/* actually remove the g-file */
	if (delete)
	{
		/*
		 * Since we've checked above, we can
		 * use the return from unlink to
		 * determine if the file existed or not.
		 */
		if (unlink(gfile) >= 0)
			printf(gettext("%12s: removed\n"), gfile);
		Fcnt = 0;
	}
	else
	{
		printf(gettext("%12s: not being edited by you\n"), gfile);
	}
	free(pfn);
	if (Gfile)
		free(Gfile);
}

/*
**  TAIL -- return tail of filename.
**
**	Parameters:
**		fn -- the filename.
**
**	Returns:
**		a pointer to the tail of the filename; e.g., given
**		"cmd/ls.c", "ls.c" is returned.
**
**	Side Effects:
**		none.
*/

static char *
tail(fn)
	register char *fn;
{
	register char *p;

	for (p = fn; *p != 0; p++)
		if (*p == '/' && p[1] != '\0' && p[1] != '/')
			fn = &p[1];
	return (fn);
}

/*
**  GETPFENT -- get an entry from the p-file
**
**	Parameters:
**		pfp -- p-file file pointer
**
**	Returns:
**		pointer to p-file struct for next entry
**		NULL on EOF or error
**
**	Side Effects:
**		Each call wipes out results of previous call.
*/

static struct p_file *
getpfent(pfp)
	FILE *pfp;
{
	static struct p_file ent;
	static char buf[PFILELG];
	register char *p;

	if (fgets(buf, sizeof (buf), pfp) == NULL)
		return (NULL);

	ent.p_osid = p = buf;
	ent.p_nsid = p = nextfield(p);
	ent.p_user = p = nextfield(p);
	ent.p_date = p = nextfield(p);
	ent.p_time = p = nextfield(p);
	ent.p_aux = p = nextfield(p);

	return (&ent);
}

static int
checkpfent(pf)
struct p_file *pf;
{
	if (pf->p_osid == NULL ||
	    pf->p_nsid == NULL ||
	    pf->p_user == NULL ||
	    pf->p_date == NULL ||
	    pf->p_time == NULL) {
		return (0);
	} else {
		return (1);
	}
}

static char *
nextfield(p)
	register char *p;
{
	if (p == NULL || *p == '\0')
		return (NULL);
	while (*p != ' ' && *p != '\n' && *p != '\0')
		p++;
	if (*p == '\n' || *p == '\0')
	{
		*p = '\0';
		return (NULL);
	}
	*p++ = '\0';
	return (p);
}

/*
**  PUTPFENT -- output a p-file entry to a file
**
**	Parameters:
**		pf -- the p-file entry
**		f -- the file to put it on.
**
**	Returns:
**		none.
**
**	Side Effects:
**		pf is written onto file f.
*/

static void
putpfent(pf, f)
	register struct p_file *pf;
	register FILE *f;
{
	fprintf(f, "%s %s %s %s %s", pf->p_osid, pf->p_nsid,
		pf->p_user, pf->p_date, pf->p_time);
	if (pf->p_aux != NULL)
		fprintf(f, " %s", pf->p_aux);
	else
		fprintf(f, "\n");
}

/*
**  SYSERR -- print system-generated error.
**
**	Parameters:
**		f -- format string to a printf.
**		p1, p2, p3 -- parameters to f.
**
**	Returns:
**		never.
**
**	Side Effects:
**		none.
*/

#ifdef	PROTOTYPES
static void
syserr(const char *f, ...)
#else
static void
syserr(f, va_alist)
	const char	*f;
	va_dcl
#endif
{
	va_list	ap;

#ifdef	PROTOTYPES
	va_start(ap, f);
#else
	va_start(ap);
#endif
	fprintf(stderr, gettext("\n%s SYSERR: "), MyName);
	vfprintf(stderr, f, ap);
	fprintf(stderr, "\n");
	va_end(ap);

#ifdef	SCCS_FATALHELP
	if (errno == 0 && strchr(f, '(')) {
		sccsfatalhelp((char *)f);
		errno = 0;
	}
#endif
	if (errno == 0) {
		if (didvfork)
			_exit(EX_SOFTWARE);
		exit(EX_SOFTWARE);
	}
	else
	{
		perror(NULL);
		if (didvfork)
			_exit(EX_OSERR);
		exit(EX_OSERR);
	}
}

/*
**	Guarded string manipulation routines; the last argument
**	is the length of the buffer into which the strcpy or strcat
**	is to be done.
*/

static char *gstrcat(to, from, xlength)
	char	*to, *from;
	unsigned	xlength;
{
	if (strlen(from) + strlen(to) >= xlength) {
		gstrbotch(to, from);
	}
	return (strcat(to, from));
}

static char *gstrncat(to, from, n, xlength)
	char	*to, *from;
	int	n;
	unsigned	xlength;
{
	if (n + strlen(to) >= xlength) {
		gstrbotch(to, from);
	}
	return (strncat(to, from, n));
}

static char *gstrcpy(to, from, xlength)
	char		*to;
	const char	*from;
	unsigned	xlength;
{
	if (strlen(from) >= xlength) {
		gstrbotch(from, (char *)0);
	}
	return (strcpy(to, from));
}

static void
gstrbotch(str1, str2)
	const char	*str1, *str2;
{
	usrerr(gettext("Filename(s) too long: %s %s"),
		str1,
		str2);
}

static int
diffs(nfiles, argv)
	int	nfiles;
	char	**argv;
{
	register int i;
	int	err;
	int	rval = 0;
	char	**np;
	char	**ap = argv;
	int	nargs;
	char	**args;
	char	**cur_arg;

	/*
	 * find the end of the flag arguments
	 */
	for (np = &ap[1]; *np != NULL && **np == '-'; np++) {
		if (**np == '-') {
			if (np[0][2] == '\0') {
				switch (np[0][1]) {
				case 'r':
				case 'i':
				case 'x':
				case 'c':
				case 'D':
				case 'U':
					np++;
					break;
				}
			}
		}
	}
	if (*np == NULL) {
		usrerr(gettext(" missing file arg (cm3)"));
		rval = EX_USAGE;
		exit(EX_USAGE);
	}
	nargs = 0;
	for (argv = np; *argv != NULL; argv++) {
		nargs++;
	}
	args = cur_arg = malloc(sizeof (char **) * nargs);
	argv = np;
	for (i = 0; i < nargs; i++) {
		*cur_arg++ = *argv++;
	}
	/* for each file, do the diff */
	cur_arg  = args;
	np[2]    = NULL;
	err	 = 0;
	diffs_ap = ap;
	diffs_np = np;
	for (i = 0; i < nargs; i++) {
		do_file(*cur_arg, do_diffs, 1, 0, NULL);
		if (Fcnt) {
			err = 1;
		}
		cur_arg++;
	}
	diffs_ap = NULL;
	diffs_np = NULL;
	rval	 = err;
	free(args);

	return (rval);
}

static void
do_diffs(file)
char *file;
{
#ifdef	PROTOTYPES
	char	template[] = NOGETTEXT("/tmp/sccs.XXXXXX");
#else
	char	*template = NOGETTEXT("/tmp/sccs.XXXXXX");
#endif
	char	buf1[20];
	char	buf2[20];
	char	*tmp_file, *gfile, *p, *pfile, *getcmd;
	char	*newSccsDir = NOGETTEXT("");
	bool	sfile_exists = FALSE;

	if ((diffs_ap == NULL) && (diffs_np == NULL))
		return;
	if (NewMode) {
		/*
		 * We may not do a chdir() in bulkprepare() since this would
		 * affect subcommands like get and diff.
		 */
		N.n_pflags =  NP_NOCHDIR;
		Nsd.n_pflags =  NP_NOCHDIR;

		pfile = bulkprepare(&N, file);
		if (pfile == NULL) {
			/*
			 * The error is typically
			 * "directory specified as s-file (cm14)"
			 */
			fatal(gettext(bulkerror(&N)));
		}
		N.n_pflags =  0;
		Nsd.n_pflags =  0;

		pfile = xstrdup(pfile);
	} else if ((pfile = makefile(file, newSccsDir)) == NULL)
		return;
	if (NewMode)
		gfile = file;
	else if ((gfile = makegfile(pfile)) == NULL)
		return;
	sfile_exists = isfile(pfile);

	/* make "p." filename */
	if (pfile == file) {
		pfile = xstrdup(file);
	}
	p = strrchr(pfile, '/');
	if (p == NULL) {
		p = pfile;
	} else {
		p++;
	}
	*p = 'p';

	Fcnt  = 0;
	printf("\n------- %s -------\n", Rflag ? gfile : tail(gfile));
	fflush(stdout);
	if (isfile(pfile)) {
		getcmd = NOGETTEXT("get:Grcixt -o -s -k");
	} else {
		getcmd = NOGETTEXT("get:Grcixt -o -s");
	}
	strcpy(buf1, template);
#ifdef	HAVE_MKSTEMP
	close(mkstemp(buf1));	/* Still a bit safer than mktemp() */
	chmod(buf1, S_IRUSR);	/* get will not overwrite if writable */
#else
	mktemp(buf1);
#endif
	tmp_file = buf1;
	strcpy(buf2, NOGETTEXT("-G"));
	strcat(buf2, buf1);
	diffs_np[0] = buf2;
	diffs_np[1] = gfile;
	if (!command(&diffs_ap[1], TRUE, getcmd))
	{
		diffs_np[0] = tmp_file;
		diffs_np[1] = gfile;

		/*
		 * 1. We dont want to exec diff command in case when s-file exists and clear file doesnt.
		 * 2. We want to exec diff command in case when p-file exists and clear file doesnt.
		 */
		if (!(sfile_exists && !isfile(gfile)) ||
		    (isfile(pfile) && !isfile(gfile))) {
			char	*diffcmd = NOGETTEXT("-diff:elsfnhqabBNpwtCIDUu");

			if (strcmp(maincmd->sccsname, "ldiffs") == 0)
				diffcmd = NOGETTEXT("-ldiff:elsfnhqabBNpwtCIDUu");
			if (command(&diffs_ap[1], TRUE, diffcmd) > 1) {
				Fcnt = 1;
			}
		}
	} else {
		Fcnt = 1;
	}
	free(pfile);
	if (gfile != file)
		free(gfile);
	unlink(tmp_file);
}

static int
enter(nfiles, argv)
	int	nfiles;
	char	**argv;
{
	register char *p;
	int	rval = 0;
	int	len;
	char	**np;
	char	**ap = argv;
	char	buf[FILESIZE];
	struct stat statb;

	/* skip over flag arguments */
	for (np = &ap[1]; *np != NULL && **np == '-'; np++) {
		if (**np == '-') {
			if (np[0][2] == '\0') {
				switch (np[0][1]) {
				case 'a':
				case 'd':
				case 'f':
				case 'r':
				case 'm':
					np++;
					break;
				}
			}
		}
	}
	argv = np;
	/* do an admin for each file */
	p = argv[1];
	while (*np != NULL) {
		/*
		 * Make sure the directory to hold the s. files exists.
		 * If not, create it.  If it exists but is not a directory,
		 * complain.
		 */
		char *filep, *cp;

		filep = makefile(*np, SccsDir);
		if (!NewMode) {
			gstrcpy(buf, filep, sizeof (buf));
			cp = strrchr(buf, '/');
			if (cp != 0) {
				*cp = '\0';
			}
			if (stat(buf, &statb) == -1) {
				if (mkdir(buf, 0777) == -1) {
					syserr(gettext("Cannot mkdir %s"), buf);
					exit(EX_SOFTWARE);
				}
			} else {
				if (!(statb.st_mode & S_IFDIR)) {
					usrerr(
				    "File `%s' exists, but is not directory",
						buf);
					exit(EX_SOFTWARE);
				}
			}
		}
		printf("\n%s:\n", *np);
		strcpy(buf, NOGETTEXT("-i"));
		gstrcat(buf, *np, sizeof (buf));
		ap[0] = buf;
		argv[0] = *np;
		argv[1] = NULL;
		rval = command(ap, TRUE, "admin");
		argv[1] = p;
		if (rval == 0) {
			buf[0] = 0;
			if (strstr(*np, tail(*np)) != NULL) {
				len = strlen(*np) - strlen(tail(*np));
				strncpy(buf, *np, len);
				buf[len] = 0;
			}
			strcat(buf, ",");
			gstrcat(buf, tail(*np), sizeof (buf));
			(void) rename(*np, buf);
		}
		np++;
	}
	return (rval);
}

static int
editor(nfiles, argv)
	int	nfiles;
	char	**argv;
{
	register struct sccsprog *cmd;
	register char *q;
	register int i;
	int	rval = 0;
	char	**np;
	char	**ap = argv;
	struct stat statb;

	/* get -e + call $EDITOR */
	struct fs {
		char		*name;	/* file name to edit	    */
		struct stat	statb;	/* stat() after "sccs edit" */
		struct timespec	mtime;	/* mtime before "sccs edit" */
		int		nogfile; /* miss. before "sccs edit" */
	};
	struct fs	_fs[16];
	struct fs	*fs = _fs;
	struct fs	*rs = NULL;
	int		fslen = sizeof (_fs) / sizeof (struct fs);
	int		fsidx = 0;
	char		*xp[2];
	sigset_t	oldmask;

	/*
	 * prepare args, skip over flag arguments
	 */
	for (np = &ap[1]; *np != NULL; np++) {
		char	*filep;

		if (**np == '-')
			continue;
		filep = makefile(*np, SccsDir);
		if (!exists(filep))		/* No s.file,	  */
			continue;		/* not under SCCS */

		/*
		 * Run a lightweight "sact s.file"
		 */
		if (exists(auxf(filep, 'p')))	/* Already edited */
			continue;		/* so ignore	  */

		if (filep != *np)
			free(filep);

		if (fsidx >= fslen) {
			/*
			 * Expand rule name space.
			 */
			fslen += 16;
			fs = realloc(rs, fslen * sizeof (struct fs));
			if (fs == NULL) {
				perror(gettext("Sccs: no mem"));
				exit(EX_OSERR);
			}
			if (rs == NULL)
				memmove(fs, _fs, sizeof (_fs));
			rs = fs;
		}
		fs[fsidx].nogfile = 0;
		fs[fsidx].mtime.tv_sec = (time_t)0;
		fs[fsidx].mtime.tv_nsec = 0;
		if (!exists(*np)) {
			fs[fsidx].nogfile = 1;
		} else {
			fs[fsidx].mtime.tv_sec = Statbuf.st_mtime;
			fs[fsidx].mtime.tv_nsec = stat_mnsecs(&Statbuf);
		}
		xp[0] = *np;
		xp[1] = NULL;
		rval = command(xp, FORCE_FORK, "edit");
		if (rval != 0)			/* Checkout problem */
			continue;		/* so ignore	    */

		/*
		 * Save unedited state from after sccs edit *np
		 */
		fs[fsidx].name = *np;
		if (stat(*np, &fs[fsidx].statb) == -1)
			continue;
		fsidx++;
	}
	q = getenv("SCCS_EDITOR");
	if (q == NULL)
		q = getenv("EDITOR");
	if (q == NULL)
		q = "vi";
	cmd = lookup(q);
	if (cmd != NULL)
		fatal(gettext("illegal editor in environment (sc1)"));
	cmd = lookup(argv[0]);
	ap[0] = q;
	block_sigs(oldmask);
	rval = callprog(q, cmd->sccsflags, ap, TRUE);
	restore_sigs(oldmask);

	for (i = 0; i < fsidx; i++) {
		if (stat(fs[i].name, &statb) != -1) {
			if (fs[i].statb.st_mtime != statb.st_mtime)
				continue;
			if (stat_mnsecs(&fs[i].statb) !=
			    stat_mnsecs(&statb))
				continue;
		}
		xp[0] = fs[i].name;
		xp[1] = NULL;
		if (fs[i].nogfile) {
			rval = command(xp, TRUE, "unget -s");
		} else {
			struct timespec	ts[2];

			rval = command(xp, TRUE, "unedit");

			ts[0].tv_sec = statb.st_atime;
			ts[0].tv_nsec = stat_ansecs(&statb);
			ts[1].tv_sec = fs[i].mtime.tv_sec;
			ts[1].tv_nsec = fs[i].mtime.tv_nsec;

			utimensat(AT_FDCWD, fs[i].name, ts, 0);
		}
	}
	if (rs)
		free(rs);
	return (rval);
}

static int
histfile(nfiles, argc, argv)
	int	nfiles;
	int	argc;
	char	**argv;
{
	int	rval;
	char	**np;
	char	*g_name;
	char	*s_name;
	bool	is_dir;
	bool	get_g = FALSE;

	optind = 1;
	opt_sp = 1;
	while ((rval = getopt(argc, argv, ":g")) != -1) {
		switch (rval) {

		case 'g':
			/*
			 * The option -g is experimental and undocumented
			 * until it works correctly.
			 */
			get_g = TRUE;
			break;

		case ':':
			usrerr("%s %s",
				gettext("option requires an argument"),
				argv[optind-1]);
			rval = EX_USAGE;
			exit(EX_USAGE);
			/*NOTREACHED*/
		default:
			usrerr("%s %s",
				gettext("unknown option"),
				argv[optind-1]);
			rval = EX_USAGE;
			exit(EX_USAGE);
			/*NOTREACHED*/
		}
	}
	argc -= optind;

	if (argc <= 0)
		fatal(gettext("missing file arg (cm3)"));
	if (argc > 1)
		fatal(gettext("too many file args (cm18)"));

	rval = 0;
	for (np = &argv[optind]; *np != NULL; np++) {
		g_name = *np;
		is_dir = isdir(g_name);
		if (NewMode) {
			N.n_pflags =  NP_NOCHDIR;
			Nsd.n_pflags =  NP_NOCHDIR;
			if (is_dir) {
				N.n_pflags |=  NP_DIR;
				Nsd.n_pflags |=  NP_DIR;
			}
			s_name = bulkprepare(get_g?&Nsd:&N, g_name);
			if (s_name == NULL)
				fatal(gettext(bulkerror(&N)));
			N.n_pflags =  0;
			Nsd.n_pflags =  0;
		} else {
			if (get_g)
				s_name = makegfile(g_name);
			else
				s_name = makefile(g_name, SccsDir);
		}
		printf("%s\n", s_name);
	}

	return (rval);
}

static int
istext(nfiles, argc, argv)
	int	nfiles;
	int	argc;
	char	**argv;
{
	int	rval;
	char	**np;
	int	showdefault = 0;
	int	silent = 0;
	int	dov6 = NewMode?1:0;
	int	files = 0;

	optind = 1;
	opt_sp = 1;
	while ((rval = getopt(argc, argv, ":DsV:")) != -1) {
		switch (rval) {

		case 'D':
			showdefault = TRUE;
			break;
		case 's':
			silent = TRUE;
			break;
		case 'V':
			switch (*optarg) {
			case '4':
				dov6 = FALSE;
				break;
			case '6':
				dov6 = TRUE;
				break;
			default:
				usrerr("%s -V%s",
				    gettext("unknown option"),
				    optarg);
				rval = EX_USAGE;
				exit(EX_USAGE);
				break;
			}
			break;
		case ':':
			usrerr("%s %s",
				gettext("option requires an argument"),
				argv[optind-1]);
			rval = EX_USAGE;
			exit(EX_USAGE);
			/*NOTREACHED*/
		default:
			usrerr("%s %s",
				gettext("unknown option"),
				argv[optind-1]);
			rval = EX_USAGE;
			exit(EX_USAGE);
			/*NOTREACHED*/
		}
	}

	rval = 0;
	if (showdefault) {
		if (argv[optind]) {
			usrerr(gettext("too many args"));
			return (EX_USAGE);
		}
		printf(gettext("Current default is SCCSv%d.\n"), dov6?6:4);
		return (rval);
	}

	for (np = &argv[optind]; *np != NULL; np++) {
		files |= 1;
		rval |= fgetchk(*np, dov6, silent);
	}
	if (files == 0) {
		usrerr(gettext(" missing file arg (cm3)"));
		rval = EX_USAGE;
		exit(EX_USAGE);
	}
	return (rval);
}

/*
**  MAKEGFILE -- make filename of clear file
**
**	Parameters:
**		name -- the file name to be munged.
**
**	Returns:
**		The pathname of the clear file.
**		NULL on error.
**
*/

static char *
makegfile(name)
	char *name;
{
	register char *gname, *p, *g, *s;

	if (name == NULL || *name == '\0') {
		return (NULL);
	}
	if (sccsfile(name)) {
		gname = name;
	} else {
		gname = makefile(name, SccsDir);
		if (gname == NULL) {
			return (NULL);
		}
	}
	if (gname == name) {
		gname = xstrdup(name);
	}
	if (!sccsfile(gname)) {
		free(gname);
		return (NULL);
	}
	g = auxf(gname, 'g');
	s = malloc(strlen(SccsPath) + strlen(g) + 4);	/* "%s/s.%s" */
	if (s == NULL) {
		perror(gettext("Sccs: no mem"));
		exit(EX_OSERR);
	}
	sprintf(s, "%s/s.%s", SccsPath, g);
	p = gname + strlen(gname) - strlen(s);
	if (strcmp(p, s) != 0) {
		free(gname);
		free(s);
		return (NULL);
	}
	strcpy(p, g);
	free(s);
	return (gname);
}

#ifdef	USE_RECURSIVE

/*
 * Code to implement support for "sccs -R ..."
 */
#include <schily/walk.h>
#include <schily/find.h>
#include <schily/getcwd.h>

#undef	roundup
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))

/*
 * Structure used to transfer date from dorecurse() into walkfun().
 */
struct wargs {
	int	rval;		/* Return value for dorecurse()	*/
	int	sccslen;	/* strlen(SccsPath)		*/
	short	sccsflags;	/* sccsflags for current cmd	*/
	char	**argv;		/* Current arg vector		*/
	int	argind;		/* Where to insert path name	*/
	int	argsize;	/* Size	of allocated argv	*/
};

LOCAL int walkfunc	__PR((char *_nm, struct stat *_fs,
				int _type, struct WALK *_state));

LOCAL int
walkfunc(nm, fs, type, state)
	char		*nm;
	struct stat	*fs;
	int		type;
	struct WALK	*state;
{
	if (type == WALK_NS) {
		if (curcmd->sccsoper == CLEAN && curcmd->sccspath == CLEANC) {
			/*
			 * With "sccs clean", we remove plenty of plain files
			 * and it would be a bad idea to complain about this.
			 * So return here early in case that the current path
			 * is not a path that is a possible argument for the
			 * clean command.
			 */
			if (strcmp(nm + state->base, SccsPath) != 0)
				return (0);
		}
		errmsg("Cannot stat '%s'.\n", nm);
		state->err = 1;
		return (0);
	} else if (type == WALK_SLN && (state->walkflags & WALK_PHYS) == 0) {
		errmsg("Cannot follow symlink '%s'.\n", nm);
		state->err = 1;
		return (0);
	} else if (type == WALK_DNR) {
		if (state->flags & WALK_WF_NOCHDIR)
			errmsg("Cannot chdir to '%s'.\n", nm);
		else
			errmsg("Cannot read '%s'.\n", nm);
		state->err = 1;
		return (0);
	}

	if (state->tree == NULL ||
	    find_expr(nm, nm + state->base, fs, state, state->tree)) {
		if (S_ISDIR(fs->st_mode)) {
			if (strcmp(nm + state->base, SccsPath) != 0) {
				char	nb[MAXPATHLEN];

				nb[0] = '\0';
				if ((strlen(nm+state->base) + 11) < sizeof (nb))
					cat(nb, nm + state->base, "/",
						".sccsignore", (char *)0);

				/*
				 * Check whether this directory contains a file
				 * ".sccsignore" and thus this sub-tree should
				 * be ignored.
				 */
				if (nb[0] && access(nb, F_OK) >= 0)
					state->flags |= WALK_WF_PRUNE;
				/*
				 * This is not a "SCCS" directory, so do
				 * nothing and just return.
				 */
				return (0);
			}
		}
	} else {
		/*
		 * The find expression did not match, so return.
		 */
		return (0);
	}

	/*
	 * At this point we either found a SCCS directory or a matching file.
	 */
	{
		struct wargs	*wp = state->auxp;
		int		cwdlen;
		/*
		 * The chdir code is only needed as long as we do not have
		 * a portable treewalk() that does not require itself
		 * an internal chdir() to work correctly.
		 *
		 * First fetch the current directory in case we collect names
		 * and call the command only once.
		 */
#ifdef	HAVE_FCHDIR
		int f = -1;

		if (!bitset(COLLECT, wp->sccsflags)) {
			f = open(".", O_SEARCH);
			if (f < 0) {
				errmsg("Cannot get working directory.\n");
				state->flags |= WALK_WF_QUIT;
				return (0);
			}
		}
#else
		char	cwd[MAXPATHLEN+1];

		if (!bitset(COLLECT, wp->sccsflags) &&
		    getcwd(cwd, MAXPATHLEN) == NULL) {
			errmsg("Cannot get working directory.\n");
			state->flags |= WALK_WF_QUIT;
			return (0);
		}
#endif

		if (bitset(PDOT, wp->sccsflags)) {	/* SCCS/p.* */
			nm[state->base] = 's';
			cwdlen = state->base - wp->sccslen - 1;
		} else {				/* /SCCS/ */
			cwdlen = state->base;
			state->flags |= WALK_WF_PRUNE;	/* Don't go into SCCS */
		}
		if (bitset(COLLECT, wp->sccsflags)) {
			if ((wp->argind+2) > wp->argsize) {
				wp->argsize += 2;
				wp->argsize = roundup(wp->argsize, 64);
				wp->argv = realloc(wp->argv,
						wp->argsize * sizeof (char *));
				if (wp->argv == NULL) {
					perror(gettext("Sccs: no mem"));
					exit(EX_OSERR);
				}
			}
			wp->argv[wp->argind++] = xstrdup(nm);
			wp->argv[wp->argind] = NULL;
			return (0);
		}

		if ((cwdlen + 3) > Cwdlen) {
			Cwdlen = roundup(cwdlen+3, 64);
			Cwd = realloc(Cwd, Cwdlen);
			if (Cwd == NULL) {
				perror(gettext("Sccs: no mem"));
				exit(EX_OSERR);
			}
		}
		strlcpy(&Cwd[2], nm, cwdlen+1);

		if (walkhome(state) < 0) {
			state->flags |= WALK_WF_QUIT;
			return (0);
		}

		wp->argv[wp->argind] = nm;
		wp->rval |= command(wp->argv, TRUE, "");

#ifdef	HAVE_FCHDIR
		if (fchdir(f) < 0) {
			errmsg("Cannot chdir back.\n");
			state->flags |= WALK_WF_QUIT;
			return (0);
		}
		close(f);
#else
		if (chdir(cwd) < 0) {
			errmsg("Cannot chdir back.\n");
			state->flags |= WALK_WF_QUIT;
			return (0);
		}
#endif
	}
	return (0);
}

LOCAL int
dorecurse(argv, np, dir, cmd)
	char		**argv;
	char		**np;
	char		*dir;
	struct sccsprog *cmd;
{
	int		ac;
	char		*av[10];
	char		*dpath = NULL;
	char		*ppath = NULL;
	char		*oSccsDir = SccsDir;
	finda_t		fa;
	findn_t		*find_node;
	struct WALK	walkstate;
	struct wargs	wa;
	struct stat	sb;

	if (Cwd == NULL) {
		Cwdlen = 64;
		Cwd = malloc(Cwdlen);
		if (Cwd == NULL) {
			perror(gettext("Sccs: no mem"));
			exit(EX_OSERR);
		}
		strcpy(Cwd, "-C");
	}
	Cwd[2] = '\0';

	if (SccsDir && SccsDir[0]) {
		dpath = malloc(strlen(SccsDir) + strlen(dir) + 2);
		if (dpath == NULL) {
			perror(gettext("Sccs: no mem"));
			exit(EX_OSERR);
		}
		cat(dpath, SccsDir, "/", dir, (char *)0);
		SccsDir = "";
	}

	/*
	 * If recursion should find all files, "dir" equals "." and we will not
	 * use the following block that only applies to commands with explicit
	 * path names to non-directories.
	 */
	if (stat(dir, &sb) < 0 || !S_ISDIR(sb.st_mode)) {
		char	*p = strrchr(dir, '/');
		int	cwdlen;

		cwdlen = p - dir;
		if (p == NULL) {
			strcpy(&Cwd[2], "./");
			cwdlen = 1;
		} else if (p == dir) {
			strcpy(&Cwd[2], "/");
		} else {
			if ((cwdlen + 4) > Cwdlen) {
				Cwdlen = roundup(cwdlen+4, 64);
				Cwd = realloc(Cwd, Cwdlen);
				if (Cwd == NULL) {
					perror(gettext("Sccs: no mem"));
					exit(EX_OSERR);
				}
			}
			strlcpy(&Cwd[2], dir, cwdlen + 2);
		}

		*np = dpath ? dpath : dir;
		Rflag = -1;			/* avoid further recursion */
		ac = command(argv, TRUE, "");
		Rflag = 1;
		SccsDir = oSccsDir;
		return (ac);
	}

	ac = 0;
	if (bitset(PDOT, cmd->sccsflags)) {
		int	l = strlen(SccsPath) + 6; /* "* /" SccsPath "/p.*" */

		ppath = malloc(l);
		if (ppath == NULL) {
			perror(gettext("Sccs: no mem"));
			exit(EX_OSERR);
		}
		cat(ppath, "*/", SccsPath, "/p.*", (char *)NULL);
		av[ac++] = "-type";
		av[ac++] = "f";
		av[ac++] = "-path";
		av[ac++] = ppath;
	} else {
		av[ac++] = "-type";
		av[ac++] = "d";
	}
	av[ac]	 = NULL;

	find_argsinit(&fa);
	fa.walkflags = WALK_CHDIR | WALK_PHYS | WALK_NOEXIT;
	fa.Argc = ac;
	fa.Argv = (char **)av;

	find_node = find_parse(&fa);
	if (fa.primtype == FIND_ERRARG || fa.primtype != FIND_ENDARGS) {
		wa.rval = 1;
		goto out;
	}

	walkinitstate(&walkstate);
	find_timeinit(time(0));
	walkstate.walkflags = fa.walkflags;
	walkstate.tree	    = find_node;
	wa.rval		    = 0;
	wa.sccslen	    = strlen(SccsPath);
	wa.sccsflags	    = cmd->sccsflags;
	wa.argv		    = argv;
	wa.argind	    = np - argv;
	wa.argsize	    = 0;
	walkstate.auxp	    = &wa;
	if (bitset(COLLECT, cmd->sccsflags)) {
		int	i;

		wa.argind++;
		wa.argsize = roundup(wa.argind, 64);
		wa.argv	   = malloc(wa.argsize * sizeof (char *));
		if (wa.argv == NULL) {
			perror(gettext("Sccs: no mem"));
			exit(EX_OSERR);
		}
		wa.argind--;
		for (i = 0; i <= wa.argind; i++)
			wa.argv[i] = argv[i];
	}
	if (fa.patlen > 0) {
		walkstate.patstate = malloc(sizeof (int) * fa.patlen);
		if (walkstate.patstate == NULL) {
			perror(gettext("Sccs: no mem"));
			exit(EX_OSERR);
		}
	}

	Rflag = -1;				/* avoid further recursion */
	treewalk(dpath ? dpath : dir, walkfunc, &walkstate);
	Rflag = 1;

	if (bitset(COLLECT, cmd->sccsflags)) {
		Cwd[2] = '\0';
		Rflag = -1;			/* avoid further recursion */
		wa.rval |= command(wa.argv, TRUE, "");
		Rflag = 1;
	}

	if (walkstate.patstate == NULL)
		free(walkstate.patstate);
out:
	if (wa.argv != argv)
		free(wa.argv);
	find_free(find_node, &fa);
	if (dpath)
		free(dpath);
	if (ppath)
		free(ppath);

	SccsDir = oSccsDir;
	return (wa.rval);
}

#endif	/* USE_RECURSIVE */



LOCAL int
fgetchk(file, dov6, silent)
	char	*file;
	int	dov6;
	int	silent;
{
	FILE	*inptr;
	char	*p = NULL;	/* Intialize to make gcc quiet */
	char	*pn =  NULL;
	char	line[VBUF_SIZE];
	int	idx = 0;
	off_t	nline;
	off_t	soh = 0;
	int	err = 0;
	char	lastchar;

	inptr = fopen(file, "rb");
	if (inptr == (FILE *)NULL) {
		if (!silent)
			fprintf(stderr,
			gettext(
			"%s: Cannot open.\n"), file);
		return (0);
	}
	setvbuf(inptr, NULL, _IOFBF, VBUF_SIZE);

	/*
	 * This gives the illusion that a zero-length file ends
	 * in a newline so that it won't be mistaken for a
	 * binary file.
	 */
	lastchar = '\n';
	(void) memset(line, '\377', sizeof (line));
	nline = 0;
	/*
	 * In most cases (non record oriented I/O), we can optimize the way we
	 * scan files for '\0' bytes, line-ends '\n' and ^A '\1'. The optimized
	 * algorithm allows to avoid to do a reverse scan for '\0' from the end
	 * of the buffer.
	 */
	while ((idx = fread(line, 1, sizeof (line), inptr)) > 0) {
		if (lastchar == '\n' && line[0] == CTLCHAR) {
			if (soh == 0 && !dov6) {
				soh = nline + 1;
				goto issoh;
			}
		}
		lastchar = line[idx-1];
		p = findbytes(line, idx, '\0');
		if (p != NULL)
			pn = p;
		for (p = line;
		    (p = findbytes(p, idx - (p-line), '\n')) != NULL; p++) {
			if (pn && p > pn) {
	errout:
				if (inptr)
					fclose(inptr);
				if (silent)
					return (1);
				fprintf(stderr,
				gettext(
				"%s: line %jd contains '\\000' (de14)\n"),
				file, (Intmax_t)++nline);
				return (1);
			}
			nline++;
			if ((p - line) >= (idx-1))
				break;

			if (p[1] == CTLCHAR) {
				if (soh == 0 && !dov6) {
					soh = nline + 1;
					goto issoh;
				}
			}
		}
	}
issoh:
	fclose(inptr);
	inptr = NULL;
	if (soh) {
		if (!silent)
			fprintf(stderr,
			gettext(
			"%s: line %jd begins with '\\001' (de20)\n"),
				file, (Intmax_t)soh);
		err = 1;
	}
	if (lastchar != '\n') {
		if (pn && nline == 0)	/* Found null byte but no newline */
			goto errout;
		if (dov6)
			return (err);
		if (!silent)
			fprintf(stderr,
			gettext("%s: no newline at end of file (de18)\n"),
			file);
		err = 1;
	}
	return (err);
}
