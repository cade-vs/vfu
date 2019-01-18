bin_PROGRAMS = vfu
AM_CXXFLAGS = -Wall -I$(top_srcdir)/vslib -I$(top_builddir) @PCRE_CFLAGS@

vfu_SOURCES = \
    see.cpp \
    vfu.cpp \
    vfuarc.cpp \
    vfucopy.cpp \
    vfudir.cpp \
    vfufiles.cpp \
    vfumenu.cpp \
    vfuopt.cpp \
    vfusys.cpp \
    vfutools.cpp \
    vfuuti.cpp \
    vfuview.cpp

DEFS = @DEFS@ -DSYSCONFDIR=\"$(sysconfdir)\"
LDFLAGS  = -L$(top_builddir)/vslib @LDFLAGS@
LDADD = -lvscon -lvslib @PCRE_LIBS@

# These get packaged when 'make dist' is run.
noinst_HEADERS = \
    see.h  \
    vfucopy.h   \
    vfufiles.h  \
    vfumenu.h   \
    vfusetup.h  \
    vfutools.h  \
    vfuview.h   \
    vfuarc.h    \
    vfudir.h    \
    vfu.h   \
    vfuopt.h    \
    vfusys.h    \
    vfuuti.h
