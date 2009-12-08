INCLUDES = $(PROG_CFLAGS) $(GTK_CFLAGS) $(GLIB_FLAGS) $(AVCODEC_CFLAGS) \
    $(PULSE_CFLAGS) $(GST_CFLAGS) -DSHARE_DIR=\"$(pkgdatadir)\"

bin_PROGRAMS = gmediamp

gmediamp_LDADD = $(PROG_LIBS) $(GTK_LIBS) $(GLIB_LIBS) $(AVCODEC_LIBS) \
    $(PULSE_LIBS) $(GST_LIBS)

gmediamp_SOURCES = \
    shell.c shell.h \
    plugin-manager.c plugin-manager.h \
    player.c player.h \
    entry.c entry.h \
    tray.c tray.h \
    mini-pane.c mini-pane.h \
    tag-handler.c tag-handler.h \
    progress.c progress.h \
    playlist.c playlist.h \
    track-source.c track-source.h \
    media-store.c media-store.h \
    player-av.c player-av.h \
    player-gst.c player-gst.h \
    tag-dialog.c tag-dialog.h \
    browser.c browser.h

EXTRA_DIST=gmediamp.schemas
CLEANFILES=

schemadir   = @GCONF_SCHEMA_FILE_DIR@
schema_DATA = gmediamp.schemas

install-data-local: $(schema_DATA)
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)
