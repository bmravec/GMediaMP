INCLUDES = $(PROG_CFLAGS) $(GTK_CFLAGS) $(GLIB_FLAGS) $(AVCODEC_CFLAGS) \
    $(PULSE_CFLAGS) $(GST_CFLAGS) $(GPOD_CFLAGS) -DSHARE_DIR=\"$(pkgdatadir)\"

bin_PROGRAMS = gmediamp

gmediamp_LDADD = $(PROG_LIBS) $(GTK_LIBS) $(GLIB_LIBS) $(AVCODEC_LIBS) \
    $(PULSE_LIBS) $(GST_LIBS) $(GPOD_LIBS)

if PLAYERGSTREAMER
    gstplayer_sources = player-gst.c player-gst.h
else
    gstplayer_sources =
endif

if PLAYERAVCODEC
    avplayer_sources = player-av.c player-av.h
else
    avplayer_sources =
endif

if ENABLEIPOD
    ipod_sources = device-ipod.c device-ipod.h ipod-store.c ipod-store.h
else
    ipod_sources =
endif

gmediamp_SOURCES = \
    shell.c shell.h \
    column-funcs.c column-funcs.h \
    plugin-manager.c plugin-manager.h \
    player.c player.h \
    $(gstplayer_sources) \
    $(avplayer_sources) \
    entry.c entry.h \
    tray.c tray.h \
    mini-pane.c mini-pane.h \
    progress.c progress.h \
    playlist.c playlist.h \
    track-source.c track-source.h \
    media-store.c media-store.h \
    gmediadb-store.c gmediadb-store.h \
    tag-dialog.c tag-dialog.h \
    browser.c browser.h \
    tag-reader.c tag-reader.h \
    device-manager.c device-manager.h \
    device.c device.h \
    $(ipod_sources)

EXTRA_DIST=gmediamp.schemas
CLEANFILES=

schemadir   = @GCONF_SCHEMA_FILE_DIR@
schema_DATA = gmediamp.schemas

install-data-local: $(schema_DATA)
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)
