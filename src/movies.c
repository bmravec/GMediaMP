/*
 *      movies.c
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
#include <gmediadb.h>

#include "shell.h"
#include "movies.h"

#include "track-source.h"
#include "media-store.h"

static void track_source_init (TrackSourceInterface *iface);
static void media_store_init (MediaStoreInterface *iface);
G_DEFINE_TYPE_WITH_CODE (Movies, movies, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TRACK_SOURCE_TYPE, track_source_init);
    G_IMPLEMENT_INTERFACE (MEDIA_STORE_TYPE, media_store_init)
)

struct _MoviesPrivate {
    Shell *shell;
    GMediaDB *db;

    GtkWidget *widget;
    GtkWidget *title;

    GtkTreeModel *title_store;
    GtkTreeSelection *title_sel;

    Entry *s_entry;
};

typedef gint (*MoviesCompareFunc) (gpointer a, gpointer b);

static void str_column_func (GtkTreeViewColumn *column, GtkCellRenderer *cell,
    GtkTreeModel *model, GtkTreeIter *iter, gchar *data);
static void int_column_func (GtkTreeViewColumn *column, GtkCellRenderer *cell,
    GtkTreeModel *model, GtkTreeIter *iter, gchar *data);
static void status_column_func (GtkTreeViewColumn *column, GtkCellRenderer *cell,
    GtkTreeModel *model, GtkTreeIter *iter, gpointer data);
static void time_column_func (GtkTreeViewColumn *column, GtkCellRenderer *cell,
    GtkTreeModel *model, GtkTreeIter *iter, gchar *data);

static void title_row_activated (GtkTreeView *view, GtkTreePath *path,
    GtkTreeViewColumn *column, Movies *self);

static gboolean on_title_click (GtkWidget *view, GdkEventButton *event, Movies *self);
static void on_title_remove (GtkWidget *item, Movies *self);

static gpointer initial_import (Movies *self);
static gboolean insert_iter (GtkListStore *store, GtkTreeIter *iter,
    gpointer ne, MoviesCompareFunc cmp, gint l, gboolean create);
static void entry_changed (Entry *entry, Movies *self);

// Interface methods
static Entry *movies_get_next (TrackSource *self);
static Entry *movies_get_prev (TrackSource *self);

static void movies_insert_entry (Movies *self, Entry *entry);
static void movies_add_entry (MediaStore *self, Entry *entry);
static void movies_remove_entry (MediaStore *self, Entry *entry);

static guint movies_get_mtype (MediaStore *self);

// Signals from GMediaDB
static void movies_gmediadb_add (GMediaDB *db, guint id, Movies *self);
static void movies_gmediadb_remove (GMediaDB *db, guint id, Movies *self);
static void movies_gmediadb_update (GMediaDB *db, guint id, Movies *self);

static GtkWidget*
add_scroll_bars (GtkWidget *widget)
{
    GtkWidget *sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (sw), widget);

    return sw;
}

static void
track_source_init (TrackSourceInterface *iface)
{
    iface->get_next = movies_get_next;
    iface->get_prev = movies_get_prev;
}

static void
media_store_init (MediaStoreInterface *iface)
{
    iface->add_entry = movies_add_entry;
    iface->rem_entry = movies_remove_entry;
    iface->get_mtype = movies_get_mtype;
}

static void
movies_finalize (GObject *object)
{
    Movies *self = MOVIES (object);

    G_OBJECT_CLASS (movies_parent_class)->finalize (object);
}

static void
movies_class_init (MoviesClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (MoviesPrivate));

    object_class->finalize = movies_finalize;
}

static void
movies_init (Movies *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), MOVIES_TYPE, MoviesPrivate);

    self->priv->s_entry = NULL;
}

Movies*
movies_new ()
{
    return g_object_new (MOVIES_TYPE, NULL);
}

gboolean
movies_activate (Movies *self)
{
    self->priv->shell = shell_new ();
    self->priv->db = gmediadb_new ("Movies");


    self->priv->title = gtk_tree_view_new ();
    self->priv->widget = add_scroll_bars (self->priv->title);

    self->priv->title_store = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_OBJECT));
    gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->title), self->priv->title_store);

    self->priv->title_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->title));
    gtk_tree_selection_set_mode (self->priv->title_sel, GTK_SELECTION_MULTIPLE);

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    // Create Columns for Title
    renderer = gtk_cell_renderer_pixbuf_new ();
    column = gtk_tree_view_column_new_with_attributes ("", renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
        (GtkTreeCellDataFunc) status_column_func, NULL, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->title), column);

    renderer = gtk_cell_renderer_text_new ();
    g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
    column = gtk_tree_view_column_new_with_attributes ("Title", renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
        (GtkTreeCellDataFunc) str_column_func, "title", NULL);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->title), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Time", renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
        (GtkTreeCellDataFunc) time_column_func, "duration", NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->title), column);

    // Connect Signals
    g_signal_connect (G_OBJECT (self->priv->title), "row-activated", G_CALLBACK (title_row_activated), self);

    g_signal_connect (self->priv->db, "add-entry", G_CALLBACK (movies_gmediadb_add), self);
    g_signal_connect (self->priv->db, "remove-entry", G_CALLBACK (movies_gmediadb_remove), self);
    g_signal_connect (self->priv->db, "update-entry", G_CALLBACK (movies_gmediadb_update), self);

    shell_add_widget (self->priv->shell, gtk_label_new ("Library"), "Library", NULL);
    shell_add_widget (self->priv->shell, self->priv->widget, "Library/Movies", NULL);

    g_signal_connect (self->priv->title, "button-press-event", G_CALLBACK (on_title_click), self);

    gtk_widget_show_all (self->priv->widget);

    initial_import (self);
}

gboolean
movies_deactivate (Movies *self)
{

}

static void
movies_add_entry (MediaStore *self, Entry *entry)
{
    MoviesPrivate *priv = MOVIES (self)->priv;

    gchar **keys, **vals;

    guint size = entry_get_key_value_pairs (entry, &keys, &vals);

    gmediadb_add_entry (priv->db, keys, vals);

    g_strfreev (keys);
    g_strfreev (vals);
}

static void
movies_insert_entry (Movies *self, Entry *entry)
{
    GtkTreeIter iter;

    insert_iter (GTK_LIST_STORE (self->priv->title_store), &iter, entry,
        (MoviesCompareFunc) entry_cmp, 0, TRUE);

    gtk_list_store_set (GTK_LIST_STORE (self->priv->title_store), &iter,
        0, entry, -1);

    g_signal_connect (entry, "entry-changed", G_CALLBACK (entry_changed), self);
}

static void
movies_remove_entry (MediaStore *self, Entry *entry)
{
    MoviesPrivate *priv = MOVIES (self)->priv;

    gmediadb_remove_entry (priv->db, entry_get_id (entry));
}

static guint
movies_get_mtype (MediaStore *self)
{
    return MEDIA_SONG;
}

static Entry*
movies_get_next (TrackSource *self)
{
    MoviesPrivate *priv = MOVIES (self)->priv;
    GtkTreeIter iter;
    Entry *entry;

    gtk_tree_model_get_iter_first (priv->title_store, &iter);
    gtk_tree_model_get (priv->title_store, &iter, 0, &entry, -1);

    if (!priv->s_entry) {
        priv->s_entry = entry;
        return entry;
    }

    while (entry != priv->s_entry) {
        if (!gtk_tree_model_iter_next (priv->title_store, &iter))
            break;
        gtk_tree_model_get (priv->title_store, &iter, 0, &entry, -1);
    }

    if (entry == priv->s_entry) {
        if (!gtk_tree_model_iter_next (priv->title_store, &iter)) {
            priv->s_entry = NULL;
            return NULL;
        }

        gtk_tree_model_get (priv->title_store, &iter, 0, &entry, -1);
        priv->s_entry = entry;
        return entry;
    }

    gtk_tree_model_get_iter_first (priv->title_store, &iter);
    gtk_tree_model_get (priv->title_store, &iter, 0, &entry, -1);
    priv->s_entry = entry;
    return entry;
}

static Entry*
movies_get_prev (TrackSource *self)
{
    MoviesPrivate *priv = MOVIES (self)->priv;
    GtkTreeIter iter;
    Entry *ep = NULL, *en = NULL;

    if (!priv->s_entry) {
        return NULL;
    }

    gtk_tree_model_get_iter_first (priv->title_store, &iter);
    gtk_tree_model_get (priv->title_store, &iter, 0, &en, -1);

    while (en != priv->s_entry) {
        if (!gtk_tree_model_iter_next (priv->title_store, &iter))
            return NULL;
        ep = en;
        gtk_tree_model_get (priv->title_store, &iter, 0, &en, -1);
    }

    priv->s_entry = ep;
    return ep;
}

static void
str_column_func (GtkTreeViewColumn *column,
                 GtkCellRenderer *cell,
                 GtkTreeModel *model,
                 GtkTreeIter *iter,
                 gchar *data)
{
    Entry *entry;

    gtk_tree_model_get (model, iter, 0, &entry, -1);
    if (entry) {
        g_object_set (G_OBJECT (cell), "text", entry_get_tag_str (entry, data), NULL);
    } else {
        g_object_set (G_OBJECT (cell), "text", "", NULL);
    }
}

static void
int_column_func (GtkTreeViewColumn *column,
                 GtkCellRenderer *cell,
                 GtkTreeModel *model,
                 GtkTreeIter *iter,
                 gchar *data)
{
    Entry *entry;

    gtk_tree_model_get (model, iter, 0, &entry, -1);
    if (entry) {
        gchar *str = g_strdup_printf ("%d", entry_get_tag_int (entry, data));
        g_object_set (G_OBJECT (cell), "text", str, NULL);
        g_free (str);
    } else {
        g_object_set (G_OBJECT (cell), "text", "", NULL);
    }
}

static void
time_column_func (GtkTreeViewColumn *column,
                  GtkCellRenderer *cell,
                  GtkTreeModel *model,
                  GtkTreeIter *iter,
                  gchar *data)
{
    Entry *entry;

    gtk_tree_model_get (model, iter, 0, &entry, -1);
    if (entry) {
        gchar *str = time_to_string ((gdouble) entry_get_tag_int (entry, data));
        g_object_set (G_OBJECT (cell), "text", str, NULL);
        g_free (str);
    } else {
        g_object_set (G_OBJECT (cell), "text", "", NULL);
    }
}

static void
status_column_func (GtkTreeViewColumn *column,
                    GtkCellRenderer *cell,
                    GtkTreeModel *model,
                    GtkTreeIter *iter,
                    gpointer data)
{
    Entry *entry;

    gtk_tree_model_get (model, iter, 0, &entry, -1);

    g_object_set (G_OBJECT (cell),
        "stock-id", entry_get_state_string (entry),
        "stock-size", GTK_ICON_SIZE_MENU,
        NULL);
}

static void
entry_changed (Entry *entry, Movies *self)
{
    GtkTreeIter iter;
    Entry *e;

    gtk_tree_model_get_iter_first (self->priv->title_store, &iter);

    do {
        gtk_tree_model_get (self->priv->title_store, &iter, 0, &e, -1);
        if (e == entry) {
            GtkTreePath *path = gtk_tree_model_get_path (self->priv->title_store, &iter);
            gtk_tree_model_row_changed (self->priv->title_store, path, &iter);
            gtk_tree_path_free (path);
            return;
        }
    } while (gtk_tree_model_iter_next (self->priv->title_store, &iter));
}

static gpointer
initial_import (Movies *self)
{
    gchar *tags[] = { "id", "title", "duration", "location", NULL };
    GPtrArray *entries = gmediadb_get_all_entries (self->priv->db, tags);

    int i;
    for (i = 0; i < entries->len; i++) {
        gchar **entry = g_ptr_array_index (entries, i);
        Entry *e = entry_new (entry[0] ? atoi (entry[0]) : 0);
        entry_set_tag_str (e, "title", entry[1] ? entry[1] : "");

        entry_set_tag_int (e, "duration", entry[2] ? atoi (entry[2]) : 0);

        entry_set_media_type (e, MEDIA_MOVIE);
        entry_set_location (e, entry[3]);

        movies_insert_entry (self, e);

        g_object_unref (G_OBJECT (e));
    }

    g_ptr_array_free (entries, TRUE);
}

static void
title_row_activated (GtkTreeView *view,
                     GtkTreePath *path,
                     GtkTreeViewColumn *column,
                     Movies *self)
{
    GtkTreeIter iter;
    Entry *entry;

    gtk_tree_model_get_iter (self->priv->title_store, &iter, path);
    gtk_tree_model_get (self->priv->title_store, &iter, 0, &entry, -1);

    self->priv->s_entry = entry;
    track_source_emit_play (TRACK_SOURCE (self), entry);
}

static gboolean
insert_iter (GtkListStore *store,
             GtkTreeIter *iter,
             gpointer ne,
             MoviesCompareFunc cmp,
             gint l,
             gboolean create)
{
    GtkTreeIter new_iter;
    gpointer e;
    gint m, res;
    gint r = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (store), NULL);

    if (r == l) {
        gtk_list_store_append (store, iter);
        return TRUE;
    }

    gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (store), iter, NULL, l);
    gtk_tree_model_get (GTK_TREE_MODEL (store), iter, 0, &e, -1);

    res = cmp (ne, e);
    if (res == 0 && !create) {
        return FALSE;
    } else if (res <= 0) {
        gtk_list_store_insert (store, iter, l);
        return TRUE;
    }

    gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (store), iter, NULL, r - 1);
    gtk_tree_model_get (GTK_TREE_MODEL (store), iter, 0, &e, -1);

    res = cmp (ne, e);
    if (res == 0 && !create) {
        return FALSE;
    } else if (res >= 0) {
        gtk_list_store_append (store, iter);
        return TRUE;
    }

    while (l+1 != r) {
        m = (l + r) / 2;

        gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (store), iter, NULL, m);
        gtk_tree_model_get (GTK_TREE_MODEL (store), iter, 0, &e, -1);

        gint res = cmp (ne, e);
        if (!res && !create) {
            return FALSE;
        } else if (!res && create) {
            gtk_list_store_insert_before (store, &new_iter, iter);
            *iter = new_iter;
            return TRUE;
        } else if (res < 0) {
            r = m;
        } else {
            l = m;
        }
    }

    gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (store), iter, NULL, m);
    gtk_tree_model_get (GTK_TREE_MODEL (store), iter, 0, &e, -1);

    res = cmp (ne, e);
    if (res > 0) {
        gtk_list_store_insert_after (store, &new_iter, iter);
        *iter = new_iter;
        return TRUE;
    } else if (res < 0 || create) {
        gtk_list_store_insert_before (store, &new_iter, iter);
        *iter = new_iter;
        return TRUE;
    }

    return FALSE;
}

static gboolean
on_title_click (GtkWidget *view, GdkEventButton *event, Movies *self)
{
    if (event->button == 3) {
        GtkTreePath *cpath;
        gboolean retval = TRUE;

        gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (view), event->x, event->y,
            &cpath, NULL, NULL, NULL);

        if (cpath) {
            if (!gtk_tree_selection_path_is_selected (self->priv->title_sel, cpath)) {
                retval = FALSE;
            }

            gtk_tree_path_free (cpath);
        }

        GtkWidget *menu = gtk_menu_new ();

        GtkWidget *item = gtk_image_menu_item_new_from_stock (GTK_STOCK_REMOVE, NULL);
        gtk_menu_append (GTK_MENU (menu), item);
        g_signal_connect (item, "activate", G_CALLBACK (on_title_remove), self);

        item = gtk_image_menu_item_new_from_stock (GTK_STOCK_DELETE, NULL);
        gtk_menu_append (GTK_MENU (menu), item);

        gtk_widget_show_all (menu);

        gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
                        event->button, event->time);

        return retval;
    }
    return FALSE;
}

static void
on_title_remove (GtkWidget *item, Movies *self)
{
    Entry **entries;
    GtkTreeIter iter;
    guint size, i;

    GList *rows = gtk_tree_selection_get_selected_rows (self->priv->title_sel, NULL);
    size = g_list_length (rows);

    entries = g_new0 (Entry*, size);
    for (i = 0; i < size; i++) {
        gtk_tree_model_get_iter (self->priv->title_store, &iter, g_list_nth_data (rows, i));
        gtk_tree_model_get (self->priv->title_store, &iter, 0, &entries[i], -1);
    }

    g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (rows);

    for (i = 0; i < size; i++) {
        media_store_remove_entry (MEDIA_STORE (self), entries[i]);
    }

    g_free (entries);
}

static void
movies_gmediadb_add (GMediaDB *db, guint id, Movies *self)
{
    gchar *tags[] = { "id", "title", "duration", "location", NULL };
    gchar **entry = gmediadb_get_entry (self->priv->db, id, tags);

    Entry *e = entry_new (entry[0] ? atoi (entry[0]) : 0);
    entry_set_tag_str (e, "title", entry[1] ? entry[1] : "");

    entry_set_tag_int (e, "duration", entry[2] ? atoi (entry[2]) : 0);

    entry_set_media_type (e, MEDIA_MOVIE);
    entry_set_location (e, entry[3]);

    gdk_threads_enter ();
    movies_insert_entry (self, e);
    gdk_threads_leave ();

    g_object_unref (G_OBJECT (e));
}

static void
movies_gmediadb_remove (GMediaDB *db, guint id, Movies *self)
{
    Entry *entry;
    GtkTreeIter iter;

    gtk_tree_model_iter_children (self->priv->title_store, &iter, NULL);

    do {
        gtk_tree_model_get (self->priv->title_store, &iter, 0, &entry, -1);

        if (entry_get_id (entry) == id) {
            gtk_list_store_remove (GTK_LIST_STORE (self->priv->title_store), &iter);
            g_object_unref (entry);
        }
    } while (gtk_tree_model_iter_next (self->priv->title_store, &iter));
}

static void
movies_gmediadb_update (GMediaDB *db, guint id, Movies *self)
{

}
