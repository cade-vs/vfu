# The values here are used by the configure script and defined as
# macros in the generated config.h when ./configure is run. Traditionally the
# third field is the "bug report" address, but using a URL is acceptable
AC_INIT([vfu], [4.17-dev], [https://github.com/cade-vs/vfu/issues],,[http://cade.datamax.bg/vfu/])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([subdir-objects foreign no-dependencies])
AC_PREREQ(2.69)
AC_CONFIG_HEADERS([config.h:config.in])

# Don't add -g and -O2 by default
: ${CFLAGS=""}

# RELEASE TODO: set this to "no"
# This will add debug and profiling options to ./configure
AX_CHECK_ENABLE_DEBUG([yes])

AC_PROG_CC
AC_PROG_CXX
AC_CONFIG_MACRO_DIRS([build-aux/m4])
AC_PROG_INSTALL
AC_SYS_LARGEFILE

# This prevents problems such as those shown in
# https://github.com/theimpossibleastronaut/rmw/commit/90200c2df06b16f16b5d21d25c51966c0ee65b23#commitcomment-30291627
# and https://github.com/theimpossibleastronaut/rmw/issues/21
AM_MAINTAINER_MODE

AX_WITH_CURSES

if test -f configure.ac; then
    AC_MSG_ERROR([configure should be run in a separate build directory], [1])
fi

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
