#ident "%W% %E% %Q%"
###########################################################################
# Sample makefile for installing localized shell scripts
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
INSMODE=	0755
TARGET=		cstyle.js
SCRFILE=	cstyle.pl
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.scr
###########################################################################
