#
#*******************************************************************************
#* $RCSfile: Makefile,v $
#*
#* Description:
#*
#* Last change: $Date: 2007/04/20 15:01:50 $ $Revision: 1.3 $
#*
#* Copyright 2003 Sun Microsystems, Inc. All rights reserved. Use of this
#* product is subject to license terms.
#*
#*******************************************************************************
#


PRJ=../../..

include $(PRJ)/Makefile.common

ifdef WNT

APOCSOURCES 	= papiPort.c
APOCOBJS		= $(APOCSOURCES:%.c=$(OBJDIR)/%.o)
APOCINCLUDES	= -I../.. $(GSSINCLUDES) $(INCLUDES)

all:	apoc

clean:	apocclean

apoc:	mkdirs $(APOCOBJS)

apocclean:
	rm -f $(APOCOBJS)

mkdirs:
	$(mkoutdirs)

$(OBJDIR)/%.o:	%.c
	$(CC) $(CFLAGSSO) $(APOCINCLUDES) -c $*.c -Fo$(OBJDIR)/$*.o -Fd$(OBJDIR)/$*.pdb

else

apoc:

all:

clean:

apoc:

apocclean:

mkdirs:

endif
