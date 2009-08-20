INCLUDES = $(PROG_CFLAGS) $(GTK_CFLAGS) \
    -DSHARE_DIR=\"$(pkgdatadir)\"

bin_PROGRAMS = gmediamp

gmediamp_LDADD = $(PROG_LIBS) $(GTK_LIBS)
gmediamp_SOURCES = \
    shell.c shell.h \
    plugin-manager.c plugin-manager.h \
    album.c album.h \
    artist.c artist.h \
    browser.c browser.h \
    title.c title.h \
    player.c player.h \
    entry.c entry.h \
    tray.c tray.h \
    tag-handler.c tag-handler.h \
    progress.c progress.h \
    playlist.c playlist.h \
    music.c music.h

EXTRA_DIST=
CLEANFILES=

schemadir   = @GCONF_SCHEMA_FILE_DIR@
schema_DATA = gmediamp.schemas

install-data-local: $(schema_DATA)
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)
