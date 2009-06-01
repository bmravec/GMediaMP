INCLUDES = $(PROG_CFLAGS) $(GTK_CFLAGS)

gmediamp-marshal.c: gmediamp-marshal.list
	$(GLIB_GENMARSHAL) --prefix=gmediamp_marshal gmediamp-marshal.list --body > gmediamp-marshal.c

gmediamp-marshal.h: gmediamp-marshal.list
	$(GLIB_GENMARSHAL) --prefix=gmediamp_marshal gmediamp-marshal.list --header > gmediamp-marshal.h

bin_PROGRAMS = gmediamp

gmediamp_LDADD = $(PROG_LIBS) $(GTK_LIBS)
gmediamp_SOURCES = \
    gmediamp-marshal.c gmediamp-marshal.h \
    side-pane.c side-pane.h \
    gmediadb.c gmediadb.h \
    album.c album.h \
    artist.c artist.h \
    browser.c browser.h \
    title.c title.h \
    entry.h gmediamp.c

BUILT_SOURCES = \
    gmediamp-marshal.c \
    gmediamp-marshal.h

EXTRA_DIST = gmediamp-marshal.list

CLEANFILES = \
    gmediamp-marshal.h \
    gmediamp-marshal.c
