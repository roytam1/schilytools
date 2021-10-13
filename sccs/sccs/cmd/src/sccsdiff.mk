#ident @(#)sccsdiff.mk	1.4 11/05/18 
###########################################################################
# Sample makefile for installing localized shell scripts
###########################################################################
SRCROOT=	../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

#INSDIR=	sccs
INSDIR=		ccs/bin
INSMODE=	0755
TARGET=		sccsdiff
SCRFILE=	sccsdiff.sh
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.scr
###########################################################################

CPPOPTS +=	-I../../hdr
VERSION=	../../hdr/version.c

LOCALIZE=	@echo "	==> LOCALIZING \"$@\""; $(RM_F) $@; \
	SVERS=`$(CPP) $(CPPFLAGS) $(VERSION) | grep '^version' | awk '{ print $$2 }' | sed 's/"//g'`\
	VDATE=`$(CPP) $(CPPFLAGS) $(VERSION) | grep '^vdate' | awk '{ print $$2 }' | sed 's/"//g'`\
	SPROV=`$(CPP) $(CPPFLAGS) $(VERSION) | grep '^provider' | awk '{ print $$2 }' | sed 's/"//g'`\
	SHOST=`$(CPP) $(CPPFLAGS) $(VERSION) | grep '^host_sub' | awk '{ print $$2 }' | sed 's/"//g'`\
	export SVERS SPROV;\
	sed "s/VERSION/$$SVERS/;s,VDATE,$$VDATE,;s/PROVIDER/$$SPROV/;s/HOST_SUB/$$SHOST/;s,INS_BASE,$(INS_BASE)," $(SRCFILE) > $@ ; :
