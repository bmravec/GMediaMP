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
track_column_func (GtkTreeViewColumn *column,
                   GtkCellRenderer *cell,
                   GtkTreeModel *model,
                   GtkTreeIter *iter,
                   gpointer data)
{
    Entry *entry;
    
    gtk_tree_model_get (model, iter, 0, &entry, -1);
    
    g_object_set (G_OBJECT (cell), "text",
        g_strdup_printf ("%d", entry->track), NULL);
}

static void
title_column_func (GtkTreeViewColumn *column,
                   GtkCellRenderer *cell,
                   GtkTreeModel *model,
                   GtkTreeIter *iter,
                   gpointer data)
{
    Entry *entry;
    
    gtk_tree_model_get (model, iter, 0, &entry, -1);
    
    g_object_set (G_OBJECT (cell), "text", entry->title, NULL);
}

static void
artist_column_func (GtkTreeViewColumn *column,
                    GtkCellRenderer *cell,
                    GtkTreeModel *model,
                    GtkTreeIter *iter,
                    gpointer data)
{
    Entry *entry;
    
    gtk_tree_model_get (model, iter, 0, &entry, -1);
    
    g_object_set (G_OBJECT (cell), "text", entry->artist, NULL);
}

static void
album_column_func (GtkTreeViewColumn *column,
                   GtkCellRenderer *cell,
                   GtkTreeModel *model,
                   GtkTreeIter *iter,
                   gpointer data)
{
    Entry *entry;
    
    gtk_tree_model_get (model, iter, 0, &entry, -1);
    
    g_object_set (G_OBJECT (cell), "text", entry->album, NULL);
}

static void
duration_column_func (GtkTreeViewColumn *column,
                      GtkCellRenderer *cell,
                      GtkTreeModel *model,
                      GtkTreeIter *iter,
                      gpointer data)
{
    g_object_set (G_OBJECT (cell), "text", "0:00", NULL);
}

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
    Entry *entry;
    
    gtk_tree_model_get_iter (GTK_TREE_MODEL (self->priv->filter), &iter, path);
    
    gtk_tree_model_get (GTK_TREE_MODEL (self->priv->filter),
        &iter, 0, &entry, -1);
    
    g_signal_emit (G_OBJECT (self), signal_replace, 0, entry);
}

static void
title_init (Title *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), TITLE_TYPE, TitlePrivate);
    
    self->priv->store = gtk_list_store_new (2, G_TYPE_POINTER, G_TYPE_BOOLEAN);
    
    self->priv->filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (self->priv->store), NULL);
    gtk_tree_model_filter_set_visible_column (GTK_TREE_MODEL_FILTER (self->priv->filter), 1);
    
    gtk_tree_view_set_model (GTK_TREE_VIEW (self), self->priv->filter);
    
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Track", renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
        (GtkTreeCellDataFunc) track_column_func, NULL, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self), column);
    
    renderer = gtk_cell_renderer_text_new ();
    g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
    column = gtk_tree_view_column_new_with_attributes ("Title", renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
        (GtkTreeCellDataFunc) title_column_func, NULL, NULL);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self), column);
    
    renderer = gtk_cell_renderer_text_new ();
    g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
    column = gtk_tree_view_column_new_with_attributes ("Artist", renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
        (GtkTreeCellDataFunc) artist_column_func, NULL, NULL);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self), column);
    
    renderer = gtk_cell_renderer_text_new ();
    g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
    column = gtk_tree_view_column_new_with_attributes ("Album", renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
        (GtkTreeCellDataFunc) album_column_func, NULL, NULL);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self), column);
    
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Time", renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
        (GtkTreeCellDataFunc) duration_column_func, NULL, NULL);
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
    
    gtk_list_store_set (self->priv->store, iter, 0, entry, 1, visible, -1);
}

gint
title_compare_entries (Entry *entry,
                       Entry *e)
{
    gint res;
    
    res = g_strcmp0 (entry->artist, e->artist);
    if (res != 0)
        return res;
    
    res = g_strcmp0 (entry->album, e->album);
    if (res != 0)
        return res;
    
    if (e->track != entry->track)
        return entry->track - e->track;
    
    res = g_strcmp0 (entry->title, e->title);
    if (res != 0)
        return res;
}

void
title_add_entry (Title *self, Entry *entry)
{
    GtkTreeIter iter, new_iter;
    Entry *e;
    
    if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->store), &iter)) {
        gtk_list_store_append (self->priv->store, &iter);
        title_set_data (self, &iter, entry);
        return;
    }
    
    do {
        gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), &iter, 0, &e, -1);
        
        gint res = title_compare_entries (entry, e);
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
    
    Entry *e;
    GtkTreeIter iter;
    gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->store), &iter);
    
    if (!g_strcmp0 (self->priv->s_artist, "") && !g_strcmp0 (self->priv->s_album, "")) {
        do {
            gtk_list_store_set (self->priv->store, &iter, 1, TRUE, -1);
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->store), &iter));
    } else {
        do {
            gboolean visible = FALSE;
            
            gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), &iter,
                0, &e, -1);
            
            if ((!g_strcmp0 (self->priv->s_artist, "") ||
                 !g_strcmp0 (self->priv->s_artist, e->artist)) &&
                (!g_strcmp0 (self->priv->s_album, "") ||
                 !g_strcmp0 (self->priv->s_album, e->album))) {
                visible = TRUE;
            }
            
            gtk_list_store_set (self->priv->store, &iter, 1, visible, -1);
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->store), &iter));
    }
}

