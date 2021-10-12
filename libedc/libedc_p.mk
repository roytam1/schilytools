#ident @(#)libedc_p.mk	1.2 07/06/30 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/profiled
#.SEARCHLIST:	. $(ARCHDIR) stdio $(ARCHDIR)
#VPATH=		.:stdio:$(ARCHDIR)
INSDIR=		lib
#TARGETLIB=	oedc
TARGETLIB=	edc_ecc
CPPOPTS +=	-Iold
COPTS +=	$(COPTGPROF)
#
# Selectively increase the opimisation for libedc for better performance
#
# The code has been tested for correctness with this level of optimisation
# If your GCC creates defective code, you found a GCC bug that should
# be reported to the GCC people. As a workaround, you may remove the next
# lines to fall back to the standard optimisation level.
#
_XARCH_OPT=	$(OARCH:%cc64=-xarch=generic64) 
XARCH_OPT=	$(_XARCH_OPT:%cc=-xarch=generic) 

SUNPROCOPTOPT=	-fast $(XARCH_OPT)
GCCOPTOPT=	-O3  -fexpensive-optimizations
#
CFILES=		edc_ecc.c
HFILES=
#include		Targets
LIBS=		

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
