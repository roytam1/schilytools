#ident "@(#)Defaults.gnu	1.4 08/12/26 "
###########################################################################
#
# global definitions for GNU (hurd) Systems
#
###########################################################################
#
# Compiler stuff
#
###########################################################################
#DEFCCOM=	cc
DEFCCOM=	gcc

###########################################################################
#
# Link mode for libraries that are part of the makefile system:
# If DEFLINKMODE is set to "static", no dynamic linking will be used
# If DEFLINKMODE is set to "dynamic", dynamic linking will be used
#
###########################################################################
DEFLINKMODE=	static

###########################################################################
#
# If the next line is commented out, compilation is done with max warn level
# If the next line is uncommented, compilation is done with minimal warnings
#
###########################################################################
CWARNOPTS=

DEFINCDIRS=	$(SRCROOT)/include /usr/src/linux/include
LDPATH=		-L/opt/schily/lib
RUNPATH=	-R$(INS_BASE)/lib -R/opt/schily/lib -R$(OLIBSDIR)

###########################################################################
#
# Installation config stuff
#
###########################################################################
INS_BASE=	/opt/schily
INS_KBASE=	/
INS_RBASE=	/
#
DEFUMASK=	002
#
DEFINSMODEF=	444
DEFINSMODEX=	755
DEFINSUSR=	root
DEFINSGRP=	bin
