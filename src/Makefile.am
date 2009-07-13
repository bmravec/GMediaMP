INCLUDES = $(PROG_CFLAGS) $(GTK_CFLAGS)

bin_PROGRAMS = gmediamp

gmediamp_LDADD = $(PROG_LIBS) $(GTK_LIBS)
gmediamp_SOURCES = \
    side-pane.c side-pane.h \
    album.c album.h \
    artist.c artist.h \
    browser.c browser.h \
    title.c title.h \
    player.c player.h \
    entry.c entry.h \
    tray.c tray.h \
    gmediamp.c

EXTRA_DIST=
CLEANFILES=

schemadir   = @GCONF_SCHEMA_FILE_DIR@
schema_DATA = gmediamp.schemas

install-data-local: $(schema_DATA)
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)
