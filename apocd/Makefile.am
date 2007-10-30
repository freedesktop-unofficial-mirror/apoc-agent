INCLUDES = \
	-I${top_builddir}/libapoc/

apocd_LDADD = ${top_builddir}/libapoc/libapoc.la -lapoc

bindir = $(APOCD_DIR)
bin_PROGRAMS = apocd

apocd_SOURCES = apocd.c 

if SOLARIS
apocd_SOURCES += solaris.c
else
apocd_SOURCES += linux.c
endif

