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

    GtkWidget *delete_button;
    GtkWidget *remove_button;
    GtkWidget *add_button;

    GtkWidget *view;
    GtkListStore *store;
};

static void
playlist_finalize (GObject *object)
{
    Playlist *self = PLAYLIST (object);

    if (self->priv->shell) {
        g_object_unref (self->priv->shell);
        self->priv->shell = NULL;
    }

    G_OBJECT_CLASS (playlist_parent_class)->finalize (object);
}

static void
playlist_class_init (PlaylistClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (PlaylistPrivate));

    object_class->finalize = playlist_finalize;
}

static void
playlist_init (Playlist *self)
{
    GtkWidget *sw, *hbox;
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), PLAYLIST_TYPE, PlaylistPrivate);

    self->priv->widget = gtk_vbox_new (FALSE, 0);

    self->priv->store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
    self->priv->view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (self->priv->store));

    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (sw), self->priv->view);
    gtk_box_pack_start (GTK_BOX (self->priv->widget), sw, TRUE, TRUE, 0);

    hbox = gtk_hbox_new (FALSE, 0);
    self->priv->delete_button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
    self->priv->remove_button = gtk_button_new_from_stock (GTK_STOCK_REMOVE);
    self->priv->add_button = gtk_button_new_from_stock (GTK_STOCK_ADD);

    gtk_box_pack_start (GTK_BOX (hbox), self->priv->delete_button, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), gtk_drawing_area_new (), TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), self->priv->remove_button, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), self->priv->add_button, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (self->priv->widget), hbox, FALSE, FALSE, 0);
}

Playlist*
playlist_new (Shell *shell)
{
    Playlist *self = g_object_new (PLAYLIST_TYPE, NULL);

    self->priv->shell = g_object_ref (shell);

    shell_add_widget (self->priv->shell, self->priv->widget, "Playlists", NULL);

    gtk_widget_show_all (self->priv->widget);

    return self;
}
