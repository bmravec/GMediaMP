/*
 *      artist.c
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

#include "artist.h"

G_DEFINE_TYPE(Artist, artist, GTK_TYPE_TREE_VIEW)

struct _ArtistPrivate {
    GtkListStore *store;
    
    guint num_of_artists;
};

static guint signal_select;
static guint signal_replace;

static void
artist_cursor_changed (Artist *self,
                   GtkTreeView *treeview,
                   gpointer user_data)
{
    GtkTreePath *path;
    
    gtk_tree_view_get_cursor (GTK_TREE_VIEW (self), &path, NULL);
    
    gchar *path_str = gtk_tree_path_to_string (path);
    
    if (!g_strcmp0 (path_str, "0")) {
        g_signal_emit (G_OBJECT (self), signal_select, 0, "");
    } else {
        GtkTreeIter iter;
        gchar *art;
        
        gtk_tree_model_get_iter (GTK_TREE_MODEL (self->priv->store),
            &iter, path);
        
        gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store),
            &iter, 0, &art, -1);
        
        g_signal_emit (G_OBJECT (self), signal_select, 0, art);
        g_free (art);
    }
    
    g_free (path_str);
    gtk_tree_path_free (path);
}

static void
artist_row_activated (Artist *self,
                  GtkTreePath *path,
                  GtkTreeViewColumn *column,
                  gpointer user_data)
{
    GtkTreeIter iter;
    gchar *art;
    gchar *path_str = gtk_tree_path_to_string (path);
    
    if (!g_strcmp0 (path_str, "0")) {
        g_signal_emit (G_OBJECT (self), signal_replace, 0, "");
    } else {
        gtk_tree_model_get_iter (GTK_TREE_MODEL (self->priv->store),
            &iter, path);
        
        gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store),
            &iter, 0, &art, -1);
        
        g_signal_emit (G_OBJECT (self), signal_replace, 0, art);
        g_free (art);
    }
    
    g_free (path_str);
}

static void
artist_finalize (GObject *object)
{
    Artist *self = ARTIST (object);
    
    G_OBJECT_CLASS (artist_parent_class)->finalize (object);
}

static void
artist_class_init (ArtistClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);
    
    g_type_class_add_private ((gpointer) klass, sizeof (ArtistPrivate));
    
    object_class->finalize = artist_finalize;

    signal_select = g_signal_new ("select", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING,
        G_TYPE_NONE, 1, G_TYPE_STRING);
    
    signal_replace = g_signal_new ("entry-replace", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING,
        G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void
artist_init (Artist *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), ARTIST_TYPE, ArtistPrivate);
    
    g_signal_connect (G_OBJECT (self), "cursor-changed",
        G_CALLBACK (artist_cursor_changed), NULL);
    
    g_signal_connect (G_OBJECT (self), "row-activated",
        G_CALLBACK (artist_row_activated), NULL);
    
    self->priv->store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
    gtk_tree_view_set_model (GTK_TREE_VIEW (self), GTK_TREE_MODEL (self->priv->store));
    
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    renderer = gtk_cell_renderer_text_new ();
    g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
    column = gtk_tree_view_column_new_with_attributes ("Artist", renderer, "text", 0, NULL);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self), column);
    
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("#", renderer, "text", 1, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self), column);
    
    gtk_list_store_insert_with_values (GTK_LIST_STORE (self->priv->store),
        NULL, 0, 0, "All 0 Artists", 1, 0, -1);
    
    self->priv->num_of_artists = 0;
}

GtkWidget*
artist_new ()
{
    return g_object_new (ARTIST_TYPE, NULL);
}

void
artist_set_data (Artist *self,
                 GtkTreeIter *iter,
                 gchar *entry,
                 gboolean isnew)
{
    GtkTreeIter first;
    gint cnt;
    gchar *new_str;
    
    gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), iter, 1, &cnt, -1);
    gtk_list_store_set (self->priv->store, iter, 0, entry, 1, cnt + 1, -1);
    
    gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->store), &first);
    gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), &first, 1, &cnt, -1);
    
    if (isnew) {
        new_str = g_strdup_printf ("All %d Artists", ++self->priv->num_of_artists);
        gtk_list_store_set (self->priv->store, &first, 0, new_str, 1, cnt + 1, -1);
    } else {
        gtk_list_store_set (self->priv->store, &first, 1, cnt + 1, -1);
    }
}

void
artist_add_entry (Artist *self, gchar *artist)
{
    GtkTreeIter iter, new_iter;
    gchar *s;
    gint res;
    gint l = 1, m;
    gint r = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (self->priv->store),
                                               NULL);
    
    if (r == l) {
        gtk_list_store_append (self->priv->store, &iter);
        artist_set_data (self, &iter, artist, TRUE);
        return;
    }
    
    gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (self->priv->store),
        &iter, NULL, l);
    gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), &iter, 0, &s, -1);
    res = g_strcmp0 (artist, s);
    g_free (s);
    if (res < 0) {
        gtk_list_store_insert (self->priv->store, &iter, l);
        artist_set_data (self, &iter, artist, TRUE);
        return;
    } else if (!res) {
        artist_set_data (self, &iter, artist, FALSE);
        return;
    }
    
    gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (self->priv->store),
        &iter, NULL, r - 1);
    gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), &iter, 0, &s, -1);
    res = g_strcmp0 (artist, s);
    g_free (s);
    if (res > 0) {
        gtk_list_store_append (self->priv->store, &iter);
        artist_set_data (self, &iter, artist, TRUE);
        return;
    } else if (!res) {
        artist_set_data (self, &iter, artist, FALSE);
        return;
    }
    
    while (l+1 != r) {
        m = (l + r) / 2;
        
        gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (self->priv->store),
            &iter, NULL, m);
        gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), &iter, 0, &s, -1);
        
        res = g_strcmp0 (artist, s);
        g_free (s);
        if (!res) {
            artist_set_data (self, &iter, artist, FALSE);
            return;
        } else if (res < 0) {
            r = m;
        } else {
            l = m;
        }
    }
    
    gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (self->priv->store),
        &iter, NULL, m);
    gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), &iter, 0, &s, -1);
    
    res = g_strcmp0 (artist, s);
    g_free (s);
    
    if (res > 0) {
        gtk_list_store_insert_after (self->priv->store, &new_iter, &iter);
        artist_set_data (self, &new_iter, artist, TRUE);
    } else if (res < 0) {
        gtk_list_store_insert_before (self->priv->store, &new_iter, &iter);
        artist_set_data (self, &new_iter, artist, TRUE);
    } else {
        artist_set_data (self, &iter, artist, FALSE);
    }
}

void
artist_remove_entry (Artist *self,
                     gchar *artist)
{
    
}

