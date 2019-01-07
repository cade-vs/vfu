bin_PROGRAMS = vfu
EXTRA_PROGRAMS = vfu.yas
vfu_CXXFLAGS = -I$(top_srcdir)/vslib @CURSES_CFLAGS@ @PCRE_CFLAGS@

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

LIBS_COMMON = -lvslib @PCRE_LIBS@

DEFS = -DHAVE_CONFIG_H -DSYSCONFDIR=\"$(sysconfdir)\"
vfu_LDFLAGS  = -L$(top_srcdir)/vslib
vfu_LDADD = $(LIBS_COMMON) -lvscon @CURSES_LIBS@

vfu_yas_CXXFLAGS  = $(vfu_CXXFLAGS) -I$(top_srcdir)/yascreen -DUSE_YASCREEN
vfu_yas_SOURCES = $(vfu_SOURCES)

vfu_yas_LDFLAGS  = $(vfu_LDFLAGS) -L$(top_srcdir)/yascreen
vfu_yas_LDADD = $(LIBS_COMMON) -lvscony -lrt $(top_srcdir)/yascreen/libyascreen.a

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

# Must be indented with tabs not spaces
clean-local:
	file=@top_builddir@/vfu/vfu.yas; \
	if test -f $$file; then \
		rm $$file; \
	fi
