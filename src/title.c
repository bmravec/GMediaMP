/*
 *      title.c
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

#include "title.h"

G_DEFINE_TYPE(Title, title, GTK_TYPE_TREE_VIEW)

struct _TitlePrivate {
    GtkListStore *store;
    GtkTreeModel *filter;
    
    gchar *s_artist, *s_album;
};

static guint signal_add;
static guint signal_replace;

static void
title_finalize (GObject *object)
{
    Title *self = TITLE (object);
    
    G_OBJECT_CLASS (title_parent_class)->finalize (object);
}

static void
title_class_init (TitleClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);
    
    g_type_class_add_private ((gpointer) klass, sizeof (TitlePrivate));
    
    object_class->finalize = title_finalize;
    
    signal_replace = g_signal_new ("entry-replace", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1, G_TYPE_POINTER);
    
    signal_add = g_signal_new ("entry-add", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1, G_TYPE_POINTER);
}

void
title_row_activated (Title *self,
                     GtkTreePath *path,
                     GtkTreeViewColumn *column,
                     gpointer user_data)
{
    GtkTreeIter iter;
    Entry entry;
    
    gtk_tree_model_get_iter (GTK_TREE_MODEL (self->priv->filter), &iter, path);
    gtk_tree_model_get (GTK_TREE_MODEL (self->priv->filter), &iter,
        0, &entry.track,
        1, &entry.title,
        2, &entry.artist,
        3, &entry.album,
        4, &entry.duration,
        6, &entry.id,
        7, &entry.location,
        -1);
    
    g_signal_emit (G_OBJECT (self), signal_replace, 0, &entry);
}

static void
title_init (Title *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), TITLE_TYPE, TitlePrivate);
    
    self->priv->store = gtk_list_store_new (8,
        G_TYPE_INT,     // 0 Track
        G_TYPE_STRING,  // 1 Title
        G_TYPE_STRING,  // 2 Artist
        G_TYPE_STRING,  // 3 Album
        G_TYPE_INT,     // 4 Duration
        G_TYPE_BOOLEAN, // 5 Visible
        G_TYPE_INT,     // 6 ID
        G_TYPE_STRING); // 7 Location
    
    self->priv->filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (self->priv->store), NULL);
    gtk_tree_model_filter_set_visible_column (GTK_TREE_MODEL_FILTER (self->priv->filter), 5);
    
    gtk_tree_view_set_model (GTK_TREE_VIEW (self), self->priv->filter);
    
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Track", renderer, "text", 0, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self), column);
    
    renderer = gtk_cell_renderer_text_new ();
    g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
    column = gtk_tree_view_column_new_with_attributes ("Title", renderer, "text", 1, NULL);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self), column);
    
    renderer = gtk_cell_renderer_text_new ();
    g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
    column = gtk_tree_view_column_new_with_attributes ("Artist", renderer, "text", 2, NULL);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self), column);
    
    renderer = gtk_cell_renderer_text_new ();
    g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
    column = gtk_tree_view_column_new_with_attributes ("Album", renderer, "text", 3, NULL);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self), column);
    
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Time", renderer, "text", 4, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self), column);
    
    self->priv->s_artist = g_strdup ("");
    self->priv->s_album = g_strdup ("");
    
    g_signal_connect (G_OBJECT (self), "row-activated",
        G_CALLBACK (title_row_activated), NULL);
}

GtkWidget*
title_new ()
{
    return g_object_new (TITLE_TYPE, NULL);
}

void
title_set_data (Title *self,
                GtkTreeIter *iter,
                Entry *entry)
{
    gboolean visible = FALSE;
    
    if ((!g_strcmp0 (self->priv->s_artist, "") ||
         !g_strcmp0 (self->priv->s_artist, entry->artist)) &&
        (!g_strcmp0 (self->priv->s_album, "") ||
         !g_strcmp0 (self->priv->s_album, entry->album))) {
        visible = TRUE;
    }
    
    gtk_list_store_set (self->priv->store, iter,
        0, entry->track,
        1, entry->title,
        2, entry->artist,
        3, entry->album,
        4, entry->duration,
        5, visible,
        6, entry->id,
        7, entry->location, -1);
}

gint
title_compare_entries (Title *self,
                       GtkTreeIter *iter,
                       Entry *entry)
{
    gchar *str;
    gint res;
    
    gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), iter, 2, &str, -1);
    res = g_strcmp0 (entry->artist, str);
    if (res != 0)
        return res;
    
    gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), iter, 3, &str, -1);
    res = g_strcmp0 (entry->album, str);
    if (res != 0)
        return res;
    
    gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), iter, 0, &res, -1);
    if (res != entry->track)
        return entry->track - res;
    
    gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), iter, 1, &str, -1);
    res = g_strcmp0 (entry->title, str);
    if (res != 0)
        return res;
}

void
title_add_entry (Title *self, Entry *entry)
{
    GtkTreeIter iter, new_iter;
    gchar *str;
    
    if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->store), &iter)) {
        gtk_list_store_append (self->priv->store, &iter);
        title_set_data (self, &iter, entry);
        return;
    }
    
    do {
        gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), &iter, 0, &str, -1);
        
        gint res = title_compare_entries (self, &iter, entry);
        if (!res) {
            gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), &iter, -1);
            title_set_data (self, &iter, entry);
            return;
        } else if (res < 0) {
            gtk_list_store_insert_before (self->priv->store, &new_iter, &iter);
            title_set_data (self, &new_iter, entry);
            return;
        }
    } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->store), &iter));

    gtk_list_store_append (self->priv->store, &iter);
    title_set_data (self, &iter, entry);
}

void
title_set_filter (Title *self, gchar *artist, gchar *album)
{
    if (artist) {
        if (self->priv->s_artist) {
            g_free (self->priv->s_artist);
        }
        
        self->priv->s_artist = g_strdup (artist);
    }
    
    if (album) {
        if (self->priv->s_album) {
            g_free (self->priv->s_album);
        }
        
        self->priv->s_album = g_strdup (album);
    }
    
    GtkTreeIter iter;
    gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->store), &iter);
    
    if (!g_strcmp0 (self->priv->s_artist, "") && !g_strcmp0 (self->priv->s_album, "")) {
        do {
            gtk_list_store_set (self->priv->store, &iter, 5, TRUE, -1);
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->store), &iter));
    } else {
        do {
            gboolean visible = FALSE;
            gchar *art, *alb;
            
            gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), &iter,
                2, &art, 3, &alb, -1);
            
            if ((!g_strcmp0 (self->priv->s_artist, "") ||
                 !g_strcmp0 (self->priv->s_artist, art)) &&
                (!g_strcmp0 (self->priv->s_album, "") ||
                 !g_strcmp0 (self->priv->s_album, alb))) {
                visible = TRUE;
            }
            
            gtk_list_store_set (self->priv->store, &iter, 5, visible, -1);
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->store), &iter));
    }
}

