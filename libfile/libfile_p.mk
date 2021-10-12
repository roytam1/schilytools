#ident @(#)libfile_p.mk	1.3 07/06/30 
###########################################################################
# Sample makefile for non-shared libraries
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/profiled
INSDIR=		lib
TARGETLIB=	file
#CPPOPTS +=	-DFOKUS
CPPOPTS +=	-DSCHILY_PRINT
COPTS +=	$(COPTGPROF)

CFILES=		file.c apprentice.c softmagic.c 
LIBS=
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
