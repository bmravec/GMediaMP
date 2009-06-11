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

G_DEFINE_TYPE(Album, album, GTK_TYPE_TREE_VIEW)

struct _AlbumPrivate {
    GtkListStore *store;
    GtkTreeModel *filter;
    
    guint visible_albums;
    gchar *s_artist;
};

static guint signal_select;
static guint signal_replace;

static void
album_cursor_changed (Album *self,
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
        gchar *alb;
        
        gtk_tree_model_get_iter (self->priv->filter,
            &iter, path);
        
        gtk_tree_model_get (self->priv->filter,
            &iter, 0, &alb, -1);
        
        g_signal_emit (G_OBJECT (self), signal_select, 0, alb);
        g_free (alb);
    }
    
    g_free (path_str);
    gtk_tree_path_free (path);
}

static void
album_row_activated (Album *self,
                     GtkTreePath *path,
                     GtkTreeViewColumn *column,
                     gpointer user_data)
{
    GtkTreeIter iter;
    gchar *alb;
    gchar *path_str = gtk_tree_path_to_string (path);
    
    if (!g_strcmp0 (path_str, "0")) {
        g_signal_emit (G_OBJECT (self), signal_replace, 0, "");
    } else {
        gtk_tree_model_get_iter (self->priv->filter,
            &iter, path);
        
        gtk_tree_model_get (self->priv->filter,
            &iter, 0, &alb, -1);
        
        g_signal_emit (G_OBJECT (self), signal_replace, 0, alb);
        g_free (alb);
    }
    
    g_free (path_str);
}

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
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING,
        G_TYPE_NONE, 1, G_TYPE_STRING);
    
    signal_replace = g_signal_new ("entry-replace", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING,
        G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void
album_init (Album *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), ALBUM_TYPE, AlbumPrivate);
    
    g_signal_connect (G_OBJECT (self), "cursor-changed",
        G_CALLBACK (album_cursor_changed), NULL);
    
    g_signal_connect (G_OBJECT (self), "row-activated",
        G_CALLBACK (album_row_activated), NULL);
    
    self->priv->store = gtk_list_store_new (4,
        G_TYPE_STRING, G_TYPE_INT, G_TYPE_BOOLEAN, G_TYPE_STRING);
    
    self->priv->filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (self->priv->store), NULL);
    gtk_tree_model_filter_set_visible_column (GTK_TREE_MODEL_FILTER (self->priv->filter), 2);
    
    gtk_tree_view_set_model (GTK_TREE_VIEW (self), self->priv->filter);
    
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    renderer = gtk_cell_renderer_text_new ();
    g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
    column = gtk_tree_view_column_new_with_attributes ("Album", renderer, "text", 0, NULL);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self), column);
    
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("#", renderer, "text", 1, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self), column);
    
    gtk_list_store_insert_with_values (GTK_LIST_STORE (self->priv->store),
        NULL, 0, 0, "All 0 Albums", 1, 0, 2, TRUE, 3, 0, -1);
    
    self->priv->visible_albums = 0;
    self->priv->s_artist = g_strdup ("");
}

GtkWidget*
album_new ()
{
    return g_object_new (ALBUM_TYPE, NULL);
}

void
album_set_data (Album *self,
                GtkTreeIter *iter,
                gchar *album,
                gchar *artist,
                gboolean isnew)
{
    GtkTreeIter first;
    gint cnt;
    gchar *new_str;
    gboolean visible = FALSE;
    if (!g_strcmp0 (self->priv->s_artist, artist) ||
        !g_strcmp0 (self->priv->s_artist, "")) {
        visible = TRUE;
    }
    
    gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), iter, 1, &cnt, -1);
    gtk_list_store_set (self->priv->store, iter,
        0, album,
        1, cnt + 1,
        2, visible,
        3, artist, -1);
    
    if (visible) {
        gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->store), &first);
        gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), &first, 1, &cnt, -1);
    
        if (isnew) {
            new_str = g_strdup_printf ("All %d Albums", ++self->priv->visible_albums);
            gtk_list_store_set (self->priv->store, &first, 0, new_str, 1, cnt + 1, -1);
        } else {
            gtk_list_store_set (self->priv->store, &first, 1, cnt + 1, -1);
        }
    }
}

void
album_add_entry (Album *self,
                 gchar *album,
                 gchar *artist)
{
    GtkTreeIter iter, new_iter;
    gchar *s;
    gint res;
    gint l = 1, m;
    gint r = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (self->priv->store),
                                               NULL);
    
    if (r == l) {
        gtk_list_store_append (self->priv->store, &iter);
        album_set_data (self, &iter, album, artist, TRUE);
        return;
    }
    
    gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (self->priv->store),
        &iter, NULL, l);
    gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), &iter, 0, &s, -1);
    res = g_strcmp0 (album, s);
    g_free (s);
    if (res < 0) {
        gtk_list_store_insert (self->priv->store, &iter, l);
        album_set_data (self, &iter, album, artist, TRUE);
        return;
    } else if (!res) {
        album_set_data (self, &iter, album, artist, FALSE);
        return;
    }
    
    gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (self->priv->store),
        &iter, NULL, r - 1);
    gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), &iter, 0, &s, -1);
    res = g_strcmp0 (album, s);
    g_free (s);
    if (res > 0) {
        gtk_list_store_append (self->priv->store, &iter);
        album_set_data (self, &iter, album, artist, TRUE);
        return;
    } else if (!res) {
        album_set_data (self, &iter, album, artist, FALSE);
        return;
    }
    
    while (l+1 != r) {
        m = (l + r) / 2;
        
        gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (self->priv->store),
            &iter, NULL, m);
        gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), &iter, 0, &s, -1);
        
        res = g_strcmp0 (album, s);
        g_free (s);
        if (!res) {
            album_set_data (self, &iter, album, artist, FALSE);
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
    
    res = g_strcmp0 (album, s);
    g_free (s);
    
    if (res > 0) {
        gtk_list_store_insert_after (self->priv->store, &new_iter, &iter);
        album_set_data (self, &new_iter, album, artist, TRUE);
    } else if (res < 0) {
        gtk_list_store_insert_before (self->priv->store, &new_iter, &iter);
        album_set_data (self, &new_iter, album, artist, TRUE);
    } else {
        album_set_data (self, &iter, album, artist, FALSE);
    }
}

void
album_remove_entry (Album *self,
                    gchar *album)
{
    
}

void
album_set_filter (Album *self,
                  gchar *s_artist)
{
    if (self->priv->s_artist) {
        g_free (self->priv->s_artist);
        self->priv->s_artist = NULL;
    }
    
    if (s_artist) {
        self->priv->s_artist = g_strdup (s_artist);
    }
    
    GtkTreeIter iter, first;
    guint total_tracks = 0, alb_tracks;
    
    self->priv->visible_albums = 0;
    
    gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->store), &iter);
    if (!g_strcmp0 (self->priv->s_artist, "")) {
        while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->store), &iter)) {
            gtk_list_store_set (self->priv->store, &iter, 2, TRUE, -1);
            gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), &iter,
                1, &alb_tracks, -1);
            total_tracks += alb_tracks;
            self->priv->visible_albums++;
        }
    } else {
        while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->store), &iter)) {
            gchar *artist;
            gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), &iter,
                1, &alb_tracks, 3, &artist, -1);
            gboolean visible = !g_strcmp0 (artist, self->priv->s_artist) ? TRUE : FALSE;
            gtk_list_store_set (self->priv->store, &iter,
                2, visible, -1);
            
            if (visible) {
                total_tracks += alb_tracks;
                self->priv->visible_albums++;
            }
        }
    }
    
    gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->store), &first);
    
    gchar *new_str = g_strdup_printf ("All %d Albums", self->priv->visible_albums);
    gtk_list_store_set (self->priv->store, &first, 0, new_str, 1, total_tracks, -1);
}

