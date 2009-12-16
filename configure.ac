AC_PREREQ(2.59)
AC_INIT([gmediamp],[0.6])
AC_CONFIG_SRCDIR([src/shell.c])
AM_INIT_AUTOMAKE([dist-bzip2])

AC_CONFIG_HEADERS(config.h)

AC_PROG_CC
AC_PROG_INSTALL
AM_GCONF_SOURCE_2

AM_PATH_GLIB_2_0(,,,gthread gobject gio)
AM_PATH_GTK_2_0()

PKG_PROG_PKG_CONFIG

need_gst=false
need_avcodec=false
need_pulse=false
need_gpod=false

AC_ARG_WITH(player,
   AC_HELP_STRING([--with-player=avcodec|gstreamer],
   [Select the player implementation to use (default avcodec)]),,
   with_player=avcodec)

AC_ARG_WITH(tag_reader,
   AC_HELP_STRING([--with-tag-reader=avcodec],
   [Select the tag reader implementation to use (default avcodec)]),,
   with_tag_reader=avcodec)

AC_ARG_ENABLE([ipod],
    AS_HELP_STRING([--enable-ipod],[Enable ipod support]),
    [AC_DEFINE(ENABLE_IPOD, 1, [Enable ipod support]) need_gpod=true])

if test "x$with_player" = xgstreamer; then
    AC_DEFINE(USE_PLAYER_GSTREAMER, 1, [Use gstreamer for media playback])
    need_gst=true
fi

if test "x$with_player" = xavcodec; then
    AC_DEFINE(USE_PLAYER_AVCODEC, 1, [Use libavcodec for media playback])
    need_avcodec=true
    need_pulse=true
fi

if test "x$with_tag_reader" = xavcodec; then
    AC_DEFINE(USE_TAG_READER_AVCODEC, 1, [Use libavcodec for reading tags])
    need_avcodec=true
fi

if test "$need_pulse" = true; then
    PKG_CHECK_MODULES(PULSE, libpulse-simple)
    AC_SUBST(PULSE_CFLAGS)
    AC_SUBST(PULSE_LIBS)
fi

if test "$need_avcodec" = true; then
    PKG_CHECK_MODULES(AVCODEC, libavcodec libavformat libswscale)
    AC_SUBST(AVCODEC_CFLAGS)
    AC_SUBST(AVCODEC_LIBS)
fi

if test "$need_gst" = true; then
    PKG_CHECK_MODULES(GST, gstreamer-0.10 gstreamer-interfaces-0.10)
    AC_SUBST(GST_CFLAGS)
    AC_SUBST(GST_LIBS)
fi

if test "$need_gpod" = true; then
    PKG_CHECK_MODULES(GPOD, libgpod-1.0)
    AC_SUBST(GPOD_CFLAGS)
    AC_SUBST(GPOD_LIBS)
fi

AM_CONDITIONAL([PLAYERGSTREAMER], [test x$with_player = xgstreamer])
AM_CONDITIONAL([PLAYERAVCODEC], [test x$with_player = xavcodec])
AM_CONDITIONAL([ENABLEIPOD], [test x$enable_ipod = xyes])

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

AC_MSG_NOTICE([GMediaMP was configured with the following options:])
AC_MSG_NOTICE([Player: $with_player])
AC_MSG_NOTICE([Tag Reader: $with_tag_reader])

if test x$enable_ipod = xyes; then
    AC_MSG_NOTICE([ipod support enabled])
else
    AC_MSG_NOTICE([ipod support disabled])
fi

AC_MSG_NOTICE([End options])
