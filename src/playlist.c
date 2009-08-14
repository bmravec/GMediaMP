/*
 *      playlist.c
 *
 *      Copyright 2009 Brett Mravec <brett.mravec@gmail.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#include <gtk/gtk.h>

#include "shell.h"
#include "playlist.h"

G_DEFINE_TYPE(Playlist, playlist, G_TYPE_OBJECT)

struct _PlaylistPrivate {
    Shell *shell;
    GtkWidget *widget;
};

static guint signal_changed;

static void
playlist_finalize (GObject *object)
{
    Playlist *self = PLAYLIST (object);

    G_OBJECT_CLASS (playlist_parent_class)->finalize (object);
}

static void
playlist_class_init (PlaylistClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (PlaylistPrivate));

    object_class->finalize = playlist_finalize;

    signal_changed = g_signal_new ("playlist-changed", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);
}

static void
playlist_init (Playlist *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), PLAYLIST_TYPE, PlaylistPrivate);

    self->priv->widget = gtk_vbox_new (FALSE, 0);
}

Playlist*
playlist_new ()
{
    return g_object_new (PLAYLIST_TYPE, NULL);
}

gboolean
playlist_activate (Playlist *self)
{
    self->priv->shell = shell_new ();

    shell_add_widget (self->priv->shell, self->priv->widget, "Playlists", NULL);
}

gboolean
playlist_deactivate (Playlist *self)
{

}
