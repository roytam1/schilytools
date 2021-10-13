#ident "@(#)libboshcmd.mk	1.2 18/08/01 "
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		lib
TARGETLIB=	boshcmd

CPPOPTS +=	-DSCHILY_INCLUDES	# Tell the code to use schily include files
CPPOPTS +=	-DUSE_LARGEFILES	# Allow Large Files (> 2 GB)
CPPOPTS +=	-DVFORK			# Use vfork() if possible

CPPOPTS	+=	-I../sh

include		Targets

LIBS=		-lfind -lc
XMK_FILE=	

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
count: $(HFILES) $(CFILES) 
	count $r1


