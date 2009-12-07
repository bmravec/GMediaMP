AC_PREREQ(2.59)
AC_INIT([gmediamp],[0.6])
AC_CONFIG_SRCDIR([src/shell.c])
AM_INIT_AUTOMAKE([dist-bzip2])

AC_PROG_CC
AC_PROG_INSTALL
AM_GCONF_SOURCE_2

AM_PATH_GLIB_2_0(,,,gthread gobject)
AM_PATH_GTK_2_0()

PKG_PROG_PKG_CONFIG

DBUS_REQUIRED=0.60

PKG_CHECK_MODULES(PROG, gmediadb libnotify gconf-2.0
    xv)

AC_SUBST(PROG_CFLAGS)
AC_SUBST(PROG_LIBS)

PKG_CHECK_MODULES(AVCODEC, libavcodec libavformat libswscale)
AC_SUBST(AVCODEC_CFLAGS)
AC_SUBST(AVCODEC_LIBS)

PKG_CHECK_MODULES(PULSE, libpulse-simple)
AC_SUBST(PULSE_CFLAGS)
AC_SUBST(PULSE_LIBS)

AC_PATH_PROG(GCONFTOOL, gconftool-2)

AC_OUTPUT([
Makefile
src/Makefile
data/Makefile
data/ui/Makefile
data/imgs/Makefile
])
