#ident @(#)prtman.mk	1.1 07/02/10 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	prt
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	prt.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
