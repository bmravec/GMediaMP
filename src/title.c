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
    column = gtk_tree_view_column_new_with_attributes ("Title", renderer, "text", 1, NULL);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self), column);
    
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Artist", renderer, "text", 2, NULL);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self), column);
    
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Album", renderer, "text", 3, NULL);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self), column);
    
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Time", renderer, "text", 4, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self), column);
    
    self->priv->s_artist = g_strdup ("");
    self->priv->s_album = g_strdup ("");
}

GtkWidget*
title_new ()
{
    return g_object_new (TITLE_TYPE, NULL);
}

void
title_add_entry (Title *self, Entry *entry)
{
    GtkTreeIter iter;
    gboolean visible = FALSE;
    
    if ((!g_strcmp0 (self->priv->s_artist, "") ||
         !g_strcmp0 (self->priv->s_artist, entry->artist)) &&
        (!g_strcmp0 (self->priv->s_album, "") ||
         !g_strcmp0 (self->priv->s_album, entry->album))) {
        visible = TRUE;
    }

    gtk_list_store_append (self->priv->store, &iter);
    
    gtk_list_store_set (self->priv->store, &iter,
        0, entry->track,
        1, entry->title,
        2, entry->artist,
        3, entry->album,
        4, entry->duration,
        5, visible,
        6, entry->id,
        7, entry->location, -1);
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

