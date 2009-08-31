AC_PREREQ(2.59)
AC_INIT([gmediamp],[0.6])
AC_CONFIG_SRCDIR([src/shell.c])
AM_INIT_AUTOMAKE([dist-bzip2])

AC_PROG_CC
AC_PROG_INSTALL
AM_GCONF_SOURCE_2

PKG_PROG_PKG_CONFIG

DBUS_REQUIRED=0.60

PKG_CHECK_MODULES(PROG, glib-2.0 gthread-2.0 gmediadb libnotify gconf-2.0 taglib_c vorbisfile
    dbus-1 >= $DBUS_REQUIRED
    dbus-glib-1 >= $DBUS_REQUIRED
    alsa
    libavcodec
    libavformat
    libswscale
    xv)
AC_SUBST(PROG_CFLAGS)
AC_SUBST(PROG_LIBS)

PKG_CHECK_MODULES(GTK, gtk+-2.0)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

AC_PATH_PROG(DBUSBINDINGTOOL, dbus-binding-tool)
AC_SUBST(DBUSBINDINGTOOL)

AC_PATH_PROG(GCONFTOOL, gconftool-2)

AC_OUTPUT([
Makefile
src/Makefile
data/Makefile
data/ui/Makefile
data/imgs/Makefile
])
