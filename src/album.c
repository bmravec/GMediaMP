/*
 *      album.c
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

#include "album.h"
#include "gmediamp-marshal.h"

G_DEFINE_TYPE(Album, album, GTK_TYPE_TREE_VIEW)

struct _AlbumPrivate {
    GtkListStore *store;
    GtkTreeModel *filter;
    
    guint visible_albums;
};

static guint signal_select;
static guint signal_replace;

static void
album_finalize (GObject *object)
{
    Album *self = ALBUM (object);
    
    G_OBJECT_CLASS (album_parent_class)->finalize (object);
}

static void
album_class_init (AlbumClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);
    
    g_type_class_add_private ((gpointer) klass, sizeof (AlbumPrivate));
    
    object_class->finalize = album_finalize;
    
    signal_select = g_signal_new ("select", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, gmediamp_marshal_VOID__STRING_STRING,
        G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);
    
    signal_replace = g_signal_new ("replace", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, gmediamp_marshal_VOID__STRING_STRING,
        G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);
}

static void
album_init (Album *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), ALBUM_TYPE, AlbumPrivate);
    
    self->priv->store = gtk_list_store_new (4,
        G_TYPE_STRING, G_TYPE_INT, G_TYPE_BOOLEAN, G_TYPE_STRING);
    
    self->priv->filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (self->priv->store), NULL);
    
    gtk_tree_view_set_model (GTK_TREE_VIEW (self), self->priv->filter);
    
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Album", renderer, "text", 0, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self), column);
    
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("#", renderer, "text", 1, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self), column);
    
    gtk_list_store_insert_with_values (GTK_LIST_STORE (self->priv->store), NULL, 0,
        0, "All 0 Albums", 1, 0, 2, TRUE, 3, 0, -1);
}

GtkWidget*
album_new ()
{
    return g_object_new (ALBUM_TYPE, NULL);
}

