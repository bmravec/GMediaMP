/*
 *      now-playing.c
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
#include "now-playing.h"

G_DEFINE_TYPE(NowPlaying, now_playing, G_TYPE_OBJECT)

struct _NowPlayingPrivate {
    Shell *shell;
    GtkWidget *widget;
};

static guint signal_changed;

static void
now_playing_finalize (GObject *object)
{
    NowPlaying *self = NOW_PLAYING (object);

    G_OBJECT_CLASS (now_playing_parent_class)->finalize (object);
}

static void
now_playing_class_init (NowPlayingClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (NowPlayingPrivate));

    object_class->finalize = now_playing_finalize;

    signal_changed = g_signal_new ("now_playing-changed", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);
}

static void
now_playing_init (NowPlaying *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), NOW_PLAYING_TYPE, NowPlayingPrivate);
    self->priv->widget = gtk_drawing_area_new ();
}

NowPlaying*
now_playing_new ()
{
    return g_object_new (NOW_PLAYING_TYPE, NULL);
}

gboolean
now_playing_activate (NowPlaying *self)
{
    self->priv->shell = shell_new ();

    shell_add_widget (self->priv->shell, self->priv->widget, "Now Playing", NULL);

    gtk_widget_show_all (self->priv->widget);
}

gboolean
now_playing_deactivate (NowPlaying *self)
{

}
