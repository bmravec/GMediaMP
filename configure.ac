AC_PREREQ(2.59)
AC_INIT([gmediamp],[0.6])
AC_CONFIG_SRCDIR([src/shell.c])
AM_INIT_AUTOMAKE([dist-bzip2])

AC_CONFIG_HEADERS(config.h)

AC_PROG_CC
AC_PROG_INSTALL
AM_GCONF_SOURCE_2

AM_PATH_GLIB_2_0(,,,gthread gobject)
AM_PATH_GTK_2_0()

PKG_PROG_PKG_CONFIG

AC_ARG_WITH(player,
   AC_HELP_STRING([--with-player=avcodec|gstreamer],
   [Select the player implementation to use (default avcodec)]),,
   with_player=avcodec)

if test "x$with_player" = xgstreamer; then
    AC_DEFINE(USE_GSTREAMER_PLAYER, 1, [Use gstreamer for media playback])

    PKG_CHECK_MODULES(GST, gstreamer-0.10 gstreamer-interfaces-0.10)
    AC_SUBST(GST_CFLAGS)
    AC_SUBST(GST_LIBS)
fi

if test "x$with_player" = xavcodec; then
    AC_DEFINE(USE_AVCODEC_PLAYER, 1, [Use libavcodec for media playback])

#    PKG_CHECK_MODULES(AVCODEC, libavcodec libavformat libswscale)
#    AC_SUBST(AVCODEC_CFLAGS)
#    AC_SUBST(AVCODEC_LIBS)

    PKG_CHECK_MODULES(PULSE, libpulse-simple)
    AC_SUBST(PULSE_CFLAGS)
    AC_SUBST(PULSE_LIBS)
fi

#needed all the time because there is only one type of tag-reader
PKG_CHECK_MODULES(AVCODEC, libavcodec libavformat libswscale)
AC_SUBST(AVCODEC_CFLAGS)
AC_SUBST(AVCODEC_LIBS)

AM_CONDITIONAL([GSTREAMERPLAYER], [test x$with_player = xgstreamer])
AM_CONDITIONAL([AVCODECPLAYER], [test x$with_player = xavcodec])

PKG_CHECK_MODULES(PROG, gmediadb libnotify gconf-2.0 xv)
AC_SUBST(PROG_CFLAGS)
AC_SUBST(PROG_LIBS)

AC_PATH_PROG(GCONFTOOL, gconftool-2)

AC_OUTPUT([
Makefile
src/Makefile
data/Makefile
data/ui/Makefile
data/imgs/Makefile
])
