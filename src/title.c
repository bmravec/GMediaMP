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
    
    self->priv->store = gtk_list_store_new (6,
        G_TYPE_INT,     // Track
        G_TYPE_STRING,  // Title
        G_TYPE_STRING,  // Artist
        G_TYPE_STRING,  // Album
        G_TYPE_INT,     // Duration
        G_TYPE_BOOLEAN);// Visible
    
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
}

GtkWidget*
title_new ()
{
    return g_object_new (TITLE_TYPE, NULL);
}

void
title_add_entry (Title *self, Entry *entry)
{
    
}

