/*
 *      browser.c
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
#include <stdarg.h>

#include "shell.h"
#include "browser.h"

#include "track-source.h"
#include "media-store.h"

#include "tag-dialog.h"

static void track_source_init (TrackSourceInterface *iface);
static void media_store_init (MediaStoreInterface *iface);
G_DEFINE_TYPE_WITH_CODE (Browser, browser, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TRACK_SOURCE_TYPE, track_source_init);
    G_IMPLEMENT_INTERFACE (MEDIA_STORE_TYPE, media_store_init)
)

struct _BrowserPrivate {
    Shell *shell;
    GMediaDB *db;

    GtkWidget *widget;
    GtkWidget *pane1;
    GtkWidget *pane2;
    GtkWidget *pane3;

    GtkTreeSelection *p3_sel;

    GtkTreeModel *p1_store;
    GtkTreeModel *p2_store;
    GtkTreeModel *p3_store;
    GtkTreeModel *p3_filter;

    gchar *s_p1;
    gchar *s_p2;
    Entry *s_entry;

    gint num_p1, num_p2;

    BrowserCompareFunc cmp_func;
    gchar *p1_tag, *p2_tag;
    gboolean p2_single;
    gint mtype;
    gchar *media_type;
    gchar **tags;
};

static void str_column_func (GtkTreeViewColumn *column, GtkCellRenderer *cell,
    GtkTreeModel *model, GtkTreeIter *iter, gchar *data);
static void int_column_func (GtkTreeViewColumn *column, GtkCellRenderer *cell,
    GtkTreeModel *model, GtkTreeIter *iter, gchar *data);
static void status_column_func (GtkTreeViewColumn *column, GtkCellRenderer *cell,
    GtkTreeModel *model, GtkTreeIter *iter, gpointer data);
static void time_column_func (GtkTreeViewColumn *column, GtkCellRenderer *cell,
    GtkTreeModel *model, GtkTreeIter *iter, gchar *data);

static void pane1_cursor_changed (GtkTreeView *view, Browser *self);
static void pane2_cursor_changed (GtkTreeView *view, Browser *self);

static void pane1_row_activated (GtkTreeView *view, GtkTreePath *path,
    GtkTreeViewColumn *column, Browser *self);
static void pane2_row_activated (GtkTreeView *view, GtkTreePath *path,
    GtkTreeViewColumn *column, Browser *self);
static void pane3_row_activated (GtkTreeView *view, GtkTreePath *path,
    GtkTreeViewColumn *column, Browser *self);

static gboolean on_pane1_click (GtkWidget *view, GdkEventButton *event, Browser *self);
static gboolean on_pane2_click (GtkWidget *view, GdkEventButton *event, Browser *self);
static gboolean on_pane3_click (GtkWidget *view, GdkEventButton *event, Browser *self);

static void on_pane3_remove (GtkWidget *item, Browser *self);
static void on_pane3_info (GtkWidget *item, Browser *self);
static void on_pane3_move (Browser *self, GtkWidget *item);

static gpointer initial_import (Browser *self);
static gboolean insert_iter (GtkListStore *store, GtkTreeIter *iter,
    gpointer ne, BrowserCompareFunc cmp, gint l, gboolean create);
static void entry_changed (Entry *entry, Browser *self);

static void on_info_completed (Browser *self, GPtrArray *changes, TagDialog *td);
static void on_info_save (GtkWidget *widget, Browser *self);
static void on_info_cancel (GtkWidget *widget, Browser *self);

static void browser_insert_entry (Browser *self, Entry *entry);
static void browser_deinsert_entry (Browser *self, Entry *entry);

// Interface methods
static Entry *browser_get_next (TrackSource *self);
static Entry *browser_get_prev (TrackSource *self);

static void browser_add_entry (MediaStore *self, gchar **entry);
static void browser_remove_entry (MediaStore *self, Entry *entry);
static guint browser_get_mtype (MediaStore *self);
static gchar *browser_get_name (MediaStore *self);

// Signals from GMediaDB
static void browser_gmediadb_add (GMediaDB *db, guint id, Browser *self);
static void browser_gmediadb_remove (GMediaDB *db, guint id, Browser *self);
static void browser_gmediadb_update (GMediaDB *db, guint id, Browser *self);

static void
track_source_init (TrackSourceInterface *iface)
{
    iface->get_next = browser_get_next;
    iface->get_prev = browser_get_prev;
}

static void
media_store_init (MediaStoreInterface *iface)
{
    iface->add_entry = browser_add_entry;
    iface->rem_entry = browser_remove_entry;
    iface->get_mtype = browser_get_mtype;
    iface->get_name  = browser_get_name;
}

static void
browser_finalize (GObject *object)
{
    Browser *self = BROWSER (object);

    if (self->priv->db) {
        g_object_unref (self->priv->db);
    }

    G_OBJECT_CLASS (browser_parent_class)->finalize (object);
}

static void
browser_class_init (BrowserClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (BrowserPrivate));

    object_class->finalize = browser_finalize;
}

static void
browser_init (Browser *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), BROWSER_TYPE, BrowserPrivate);

    self->priv->num_p1 = 0;
    self->priv->num_p2 = 0;

    self->priv->s_p1 = NULL;
    self->priv->s_p2 = NULL;
    self->priv->s_entry = NULL;

    self->priv->p2_single = FALSE;
}

gboolean
g_ptr_array_contains_string (GPtrArray *array, gchar *str)
{
    gint i;
    for (i = 0; i < array->len; i++) {
        if (!g_strcmp0 (str, g_ptr_array_index (array, i))) {
            return TRUE;
        }
    }

    return FALSE;
}

static void
num_column_func (GtkTreeViewColumn *column,
                 GtkCellRenderer *cell,
                 GtkTreeModel *model,
                 GtkTreeIter *iter,
                 gchar *data)
{
    gint num;

    gtk_tree_model_get (model, iter, 1, &num, -1);

    if (num != 0) {
        gchar *str = g_strdup_printf ("%d", num);
        g_object_set (G_OBJECT (cell), "text", str, NULL);
        g_free (str);
    } else {
        g_object_set (G_OBJECT (cell), "text", "", NULL);
    }
}

Browser*
browser_new (gchar *media_type,
             gint mtype,
             gchar *p1_tag,
             gchar *p2_tag,
             gboolean p2_single,
             BrowserCompareFunc cmp_func,
             ...)
{
    Browser *self = g_object_new (BROWSER_TYPE, NULL);

    self->priv->mtype = mtype;
    self->priv->media_type = g_strdup (media_type);
    self->priv->p2_single = p2_single;
    self->priv->cmp_func = cmp_func;

    if (p1_tag) {
        self->priv->p1_tag = g_ascii_strdown (p1_tag, -1);
    }

    if (p2_tag) {
        self->priv->p2_tag = g_ascii_strdown (p2_tag, -1);
    }

    self->priv->shell = shell_new ();
    self->priv->db = gmediadb_new (media_type);

    GtkBuilder *builder = shell_get_builder (self->priv->shell);
    gtk_builder_add_from_file (builder, SHARE_DIR "/ui/browser.ui", NULL);

    self->priv->widget = GTK_WIDGET (gtk_builder_get_object (builder, "browser_vpane"));
    self->priv->pane1  = GTK_WIDGET (gtk_builder_get_object (builder, "browser_pane1"));
    self->priv->pane2  = GTK_WIDGET (gtk_builder_get_object (builder, "browser_pane2"));
    self->priv->pane3  = GTK_WIDGET (gtk_builder_get_object (builder, "browser_pane3"));

    self->priv->p1_store = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_UINT));
    self->priv->p2_store = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_UINT));
    self->priv->p3_store = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_OBJECT, G_TYPE_BOOLEAN));

    self->priv->p3_filter = gtk_tree_model_filter_new (self->priv->p3_store, NULL);
    gtk_tree_model_filter_set_visible_column (GTK_TREE_MODEL_FILTER (self->priv->p3_filter), 1);

    gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->pane1), self->priv->p1_store);
    gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->pane2), self->priv->p2_store);
    gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->pane3), self->priv->p3_filter);

    self->priv->p3_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->pane3));
    gtk_tree_selection_set_mode (self->priv->p3_sel, GTK_SELECTION_MULTIPLE);

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    if (p1_tag) {
        renderer = gtk_cell_renderer_text_new ();
        g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
        column = gtk_tree_view_column_new_with_attributes (p1_tag, renderer, "text", 0, NULL);
        gtk_tree_view_column_set_expand (column, TRUE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->pane1), column);

        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes ("#", renderer, NULL);
        gtk_tree_view_column_set_cell_data_func (column, renderer,
            (GtkTreeCellDataFunc) num_column_func, NULL, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->pane1), column);
    }

    if (p2_tag) {
        renderer = gtk_cell_renderer_text_new ();
        g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
        column = gtk_tree_view_column_new_with_attributes (p2_tag, renderer, "text", 0, NULL);
        gtk_tree_view_column_set_expand (column, TRUE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->pane2), column);

        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes ("#", renderer, NULL);
        gtk_tree_view_column_set_cell_data_func (column, renderer,
            (GtkTreeCellDataFunc) num_column_func, NULL, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->pane2), column);
    }

    renderer = gtk_cell_renderer_pixbuf_new ();
    column = gtk_tree_view_column_new_with_attributes ("", renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
        (GtkTreeCellDataFunc) status_column_func, NULL, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->pane3), column);

    va_list va;
    GPtrArray *tags = g_ptr_array_new ();

    g_ptr_array_add (tags, g_strdup ("id"));
    g_ptr_array_add (tags, g_strdup ("location"));

    if (self->priv->p1_tag) {
        g_ptr_array_add (tags, self->priv->p1_tag);
    }

    if (self->priv->p2_tag) {
        g_ptr_array_add (tags, self->priv->p2_tag);
    }

    va_start (va, cmp_func);

    gint i;
    while (TRUE) {
        gchar *label = va_arg (va, gchar*);
        if (!label) {
            break;
        }
        gchar *tag = va_arg (va, gchar*);
        gboolean expand = va_arg (va, gboolean);
        GtkTreeCellDataFunc df = va_arg (va, GtkTreeCellDataFunc);

        if (!g_ptr_array_contains_string (tags, tag)) {
            g_ptr_array_add (tags, tag);
        }

        renderer = gtk_cell_renderer_text_new ();
        g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
        column = gtk_tree_view_column_new_with_attributes (label, renderer, NULL);
        gtk_tree_view_column_set_cell_data_func (column, renderer, df, tag, NULL);
        g_object_set (column, "expand", expand, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->pane3), column);
    }

    va_end (va);

    g_ptr_array_add (tags, NULL);
    self->priv->tags = (gchar**) g_ptr_array_free (tags, FALSE);

    g_signal_connect (G_OBJECT (self->priv->pane1), "cursor-changed", G_CALLBACK (pane1_cursor_changed), self);
    g_signal_connect (G_OBJECT (self->priv->pane2), "cursor-changed", G_CALLBACK (pane2_cursor_changed), self);

    g_signal_connect (G_OBJECT (self->priv->pane1), "row-activated", G_CALLBACK (pane1_row_activated), self);
    g_signal_connect (G_OBJECT (self->priv->pane2), "row-activated", G_CALLBACK (pane2_row_activated), self);
    g_signal_connect (G_OBJECT (self->priv->pane3), "row-activated", G_CALLBACK (pane3_row_activated), self);

    g_signal_connect (self->priv->pane1, "button-press-event", G_CALLBACK (on_pane1_click), self);
    g_signal_connect (self->priv->pane2, "button-press-event", G_CALLBACK (on_pane2_click), self);
    g_signal_connect (self->priv->pane3, "button-press-event", G_CALLBACK (on_pane3_click), self);

    g_signal_connect (self->priv->db, "add-entry", G_CALLBACK (browser_gmediadb_add), self);
    g_signal_connect (self->priv->db, "remove-entry", G_CALLBACK (browser_gmediadb_remove), self);
    g_signal_connect (self->priv->db, "update-entry", G_CALLBACK (browser_gmediadb_update), self);

    shell_register_track_source (self->priv->shell, TRACK_SOURCE (self));
    shell_register_media_store (self->priv->shell, MEDIA_STORE (self));

    gtk_widget_show_all (self->priv->widget);

    if (!self->priv->p1_tag) {
        gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "browser_hbox1")));
    }

    if (!self->priv->p2_tag) {
        gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "browser_sw2")));
    }

    g_thread_create ((GThreadFunc) initial_import, self, TRUE, NULL);

    return self;
}

GtkWidget*
browser_get_widget (Browser *self)
{
    return self->priv->widget;
}

static void
browser_add_entry (MediaStore *self, gchar **entry)
{
    BrowserPrivate *priv = BROWSER (self)->priv;
/*TODO: Fix
    if (entry_get_tag_str (entry, "pane1") == NULL) {
        entry_set_tag_str (entry, "pane1", "Unknown Show");
    }

    if (entry_get_tag_str (entry, "pane2") == NULL) {
        entry_set_tag_str (entry, "pane2", "Unknown Season");
    }
*/
    gmediadb_add_entry (priv->db, entry);
}

static void
browser_insert_entry (Browser *self, Entry *entry)
{
    GtkTreeIter first, iter;
    gint cnt;
    gchar *new_str;
    gboolean res;
    gboolean tv = TRUE;

    if (self->priv->p1_tag) {
        const gchar *pane1 = entry_get_tag_str (entry, self->priv->p1_tag);

        tv = self->priv->s_p1 == NULL || !g_strcmp0 (self->priv->s_p1, pane1);

        // Add pane1 to view
        res = insert_iter (GTK_LIST_STORE (self->priv->p1_store), &iter,
            (gpointer) pane1, (BrowserCompareFunc) g_strcmp0, 1, FALSE);

        gtk_tree_model_get (self->priv->p1_store, &iter, 1, &cnt, -1);
        gtk_list_store_set (GTK_LIST_STORE (self->priv->p1_store), &iter,
            0, pane1, 1, cnt + 1, -1);

        gtk_tree_model_get_iter_first (self->priv->p1_store, &first);
        gtk_tree_model_get (self->priv->p1_store, &first, 1, &cnt, -1);

        if (res) {
            new_str = g_strdup_printf ("All %d %ss", ++self->priv->num_p1, self->priv->p1_tag);
            gtk_list_store_set (GTK_LIST_STORE (self->priv->p1_store), &first,
                0, new_str, 1, cnt + 1, -1);
            g_free (new_str);
        } else {
            gtk_list_store_set (GTK_LIST_STORE (self->priv->p1_store), &first,
                1, cnt + 1, -1);
        }

        if (self->priv->p2_tag) {
            if ((!self->priv->p2_single && self->priv->s_p2 == NULL) || !g_strcmp0 (self->priv->s_p1, pane1)) {
                const gchar *pane2 = entry_get_tag_str (entry, self->priv->p2_tag);

                tv &= (self->priv->s_p2 == NULL || !g_strcmp0 (self->priv->s_p2, pane2));

                // Add pane2 to view
                res = insert_iter (GTK_LIST_STORE (self->priv->p2_store), &iter,
                    (gpointer) pane2, (BrowserCompareFunc) g_strcmp0, 1, FALSE);

                gtk_tree_model_get (self->priv->p2_store, &iter, 1, &cnt, -1);
                gtk_list_store_set (GTK_LIST_STORE (self->priv->p2_store), &iter,
                    0, pane2, 1, cnt + 1, -1);

                gtk_tree_model_get_iter_first (self->priv->p2_store, &first);
                gtk_tree_model_get (self->priv->p2_store, &first, 1, &cnt, -1);

                if (res) {
                    new_str = g_strdup_printf ("All %d %ss", ++self->priv->num_p2, self->priv->p2_tag);
                    gtk_list_store_set (GTK_LIST_STORE (self->priv->p2_store), &first,
                        0, new_str, 1, cnt + 1, -1);
                    g_free (new_str);
                } else {
                    gtk_list_store_set (GTK_LIST_STORE (self->priv->p2_store), &first,
                        1, cnt + 1, -1);
                }
            }
        }
    }

    insert_iter (GTK_LIST_STORE (self->priv->p3_store), &iter, entry,
        self->priv->cmp_func, 0, TRUE);

    gtk_list_store_set (GTK_LIST_STORE (self->priv->p3_store), &iter,
        0, entry, 1, tv, -1);

    g_signal_connect (G_OBJECT (entry), "entry-changed",
        G_CALLBACK (entry_changed), self);
}

static void
browser_remove_entry (MediaStore *self, Entry *entry)
{
    gmediadb_remove_entry (BROWSER (self)->priv->db, entry_get_id (entry));
}

static void
browser_deinsert_entry (Browser *self, Entry *entry)
{
    GtkTreeIter first, iter;
    gint cnt;
    gchar *new_str;

    if (self->priv->p1_tag) {
        const gchar *pane1 = entry_get_tag_str (entry, self->priv->p1_tag);

        insert_iter (GTK_LIST_STORE (self->priv->p1_store), &iter,
            (gpointer) pane1, (BrowserCompareFunc) g_strcmp0, 1, FALSE);

        gtk_tree_model_get (self->priv->p1_store, &iter, 1, &cnt, -1);

        if (cnt == 1) {
            gtk_list_store_remove (GTK_LIST_STORE (self->priv->p1_store), &iter);

            gtk_tree_model_get_iter_first (self->priv->p1_store, &first);
            gtk_tree_model_get (self->priv->p1_store, &first, 1, &cnt, -1);

            new_str = g_strdup_printf ("All %d %ss", --self->priv->num_p1, self->priv->p1_tag);
            gtk_list_store_set (GTK_LIST_STORE (self->priv->p1_store), &first,
                0, new_str, 1, cnt - 1, -1);
            g_free (new_str);
        } else {
            gtk_list_store_set (GTK_LIST_STORE (self->priv->p1_store), &iter, 1, cnt - 1, -1);

            gtk_tree_model_get_iter_first (self->priv->p1_store, &first);
            gtk_tree_model_get (self->priv->p1_store, &first, 1, &cnt, -1);
            gtk_list_store_set (GTK_LIST_STORE (self->priv->p1_store), &first, 1, cnt - 1, -1);
        }

        if ( self->priv->p2_tag) {
            if ((!self->priv->p2_single && self->priv->s_p2 == NULL) || !g_strcmp0 (self->priv->s_p1, pane1)) {
                const gchar *pane2 = entry_get_tag_str (entry, self->priv->p2_tag);

                insert_iter (GTK_LIST_STORE (self->priv->p2_store), &iter, (gpointer) pane2,
                         (BrowserCompareFunc) g_strcmp0, 1, FALSE);

                gtk_tree_model_get (self->priv->p2_store, &iter, 1, &cnt, -1);

                if (cnt == 1) {
                    gtk_list_store_remove (GTK_LIST_STORE (self->priv->p2_store), &iter);

                    gtk_tree_model_get_iter_first (self->priv->p2_store, &first);
                    gtk_tree_model_get (self->priv->p2_store, &first, 1, &cnt, -1);

                    new_str = g_strdup_printf ("All %d %ss", --self->priv->num_p2, self->priv->p2_tag);
                    gtk_list_store_set (GTK_LIST_STORE (self->priv->p2_store), &first,
                        0, new_str, 1, cnt - 1, -1);
                    g_free (new_str);
                } else {
                    gtk_list_store_set (GTK_LIST_STORE (self->priv->p2_store), &iter, 1, cnt - 1, -1);

                    gtk_tree_model_get_iter_first (self->priv->p2_store, &first);
                    gtk_tree_model_get (self->priv->p2_store, &first, 1, &cnt, -1);
                    gtk_list_store_set (GTK_LIST_STORE (self->priv->p2_store), &first,
                        1, cnt - 1, -1);
                }
            }
        }
    }

    insert_iter (GTK_LIST_STORE (self->priv->p3_store), &iter, entry,
        self->priv->cmp_func, 0, FALSE);

    gtk_list_store_remove (GTK_LIST_STORE (self->priv->p3_store), &iter);

    g_object_unref (entry);
}

static gboolean
insert_iter (GtkListStore *store,
             GtkTreeIter *iter,
             gpointer ne,
             BrowserCompareFunc cmp,
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

static guint
browser_get_mtype (MediaStore *self)
{
    return BROWSER (self)->priv->mtype;
}

static gchar*
browser_get_name (MediaStore *self)
{
    return BROWSER (self)->priv->media_type;
}

static Entry*
browser_get_next (TrackSource *self)
{
    BrowserPrivate *priv = BROWSER (self)->priv;
    GtkTreeIter iter;
    Entry *entry;

    gtk_tree_model_get_iter_first (priv->p3_filter, &iter);
    gtk_tree_model_get (priv->p3_filter, &iter, 0, &entry, -1);

    if (!priv->s_entry) {
        priv->s_entry = entry;
        return entry;
    }

    while (entry != priv->s_entry) {
        if (!gtk_tree_model_iter_next (priv->p3_filter, &iter))
            break;
        gtk_tree_model_get (priv->p3_filter, &iter, 0, &entry, -1);
    }

    if (entry == priv->s_entry) {
        if (!gtk_tree_model_iter_next (priv->p3_filter, &iter)) {
            priv->s_entry = NULL;
            return NULL;
        }

        gtk_tree_model_get (priv->p3_filter, &iter, 0, &entry, -1);
        priv->s_entry = entry;
        return entry;
    }

    gtk_tree_model_get_iter_first (priv->p3_filter, &iter);
    gtk_tree_model_get (priv->p3_filter, &iter, 0, &entry, -1);
    priv->s_entry = entry;
    return entry;
}

static Entry*
browser_get_prev (TrackSource *self)
{
    BrowserPrivate *priv = BROWSER (self)->priv;
    GtkTreeIter iter;
    Entry *ep = NULL, *en = NULL;

    if (!priv->s_entry) {
        return NULL;
    }

    gtk_tree_model_get_iter_first (priv->p3_filter, &iter);
    gtk_tree_model_get (priv->p3_filter, &iter, 0, &en, -1);

    while (en != priv->s_entry) {
        if (!gtk_tree_model_iter_next (priv->p3_filter, &iter))
            return NULL;
        ep = en;
        gtk_tree_model_get (priv->p3_filter, &iter, 0, &en, -1);
    }

    priv->s_entry = ep;
    return ep;
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
entry_changed (Entry *entry, Browser *self)
{
    GtkTreeIter iter;
    Entry *e;

    if (!gtk_tree_model_get_iter_first (self->priv->p3_filter, &iter)) {
        return;
    }

    do {
        gtk_tree_model_get (self->priv->p3_filter, &iter, 0, &e, -1);
        if (e == entry) {
            GtkTreePath *path = gtk_tree_model_get_path (self->priv->p3_filter, &iter);
            gtk_tree_model_row_changed (self->priv->p3_filter, path, &iter);
            gtk_tree_path_free (path);
            return;
        }
    } while (gtk_tree_model_iter_next (self->priv->p3_filter, &iter));
}

static gpointer
initial_import (Browser *self)
{
    GPtrArray *entries = gmediadb_get_all_entries (self->priv->db, self->priv->tags);

    GtkTreeIter iter;
    GtkListStore *p1_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_UINT);
    GtkListStore *p2_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_UINT);
    GtkListStore *p3_store = gtk_list_store_new (2, G_TYPE_OBJECT, G_TYPE_BOOLEAN);

    GtkTreeModel *p3_filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (p3_store), NULL);
    gtk_tree_model_filter_set_visible_column (GTK_TREE_MODEL_FILTER (p3_filter), 1);

    gint num_p1 = 0, num_p2 = 0, cnt, res;

    int i, j;
    for (i = 0; i < entries->len; i++) {
        gchar **entry = g_ptr_array_index (entries, i);
        Entry *e = entry_new (entry[0] ? atoi (entry[0]) : 0);

        entry_set_location (e, entry[1]);
        entry_set_media_type (e, self->priv->mtype);

        for (j = 2; self->priv->tags[j]; j++) {
            entry_set_tag_str (e, self->priv->tags[j], entry[j]);
        }

        if (self->priv->p1_tag) {
            const gchar *pane1 = entry_get_tag_str (e, self->priv->p1_tag);

            // Add pane1 to view
            res = insert_iter (p1_store, &iter, (gpointer) pane1,
                (BrowserCompareFunc) g_strcmp0, 0, FALSE);

            gtk_tree_model_get (GTK_TREE_MODEL (p1_store), &iter, 1, &cnt, -1);
            gtk_list_store_set (p1_store, &iter, 0, pane1, 1, cnt + 1, -1);

            if (res) {
                num_p1++;
            }

            if (self->priv->p2_tag && !self->priv->p2_single) {
                const gchar *pane2 = entry_get_tag_str (e, self->priv->p2_tag);

                // Add pane2 to view
                res = insert_iter (p2_store, &iter, (gpointer) pane2,
                    (BrowserCompareFunc) g_strcmp0, 0, FALSE);

                gtk_tree_model_get (GTK_TREE_MODEL (p2_store), &iter, 1, &cnt, -1);
                gtk_list_store_set (p2_store, &iter, 0, pane2, 1, cnt + 1, -1);

                if (res) {
                    num_p2++;
                }
            }
        }

        insert_iter (p3_store, &iter, e, self->priv->cmp_func, 0, TRUE);
        gtk_list_store_set (p3_store, &iter, 0, e, 1, TRUE, -1);
        g_signal_connect (G_OBJECT (e), "entry-changed", G_CALLBACK (entry_changed), self);

        g_object_unref (G_OBJECT (e));
    }

    if (self->priv->p1_tag) {
        gchar *new_str = g_strdup_printf ("All %d %ss", num_p1, self->priv->p1_tag);

        gtk_list_store_insert_with_values (p1_store, NULL, 0,
            0, new_str, 1, entries->len, -1);

        g_free (new_str);

    }

    if (self->priv->p2_tag) {
        if (self->priv->p2_single) {
            gchar *str = g_strdup_printf ("Select a %s", self->priv->p1_tag);
            gtk_list_store_insert_with_values (p2_store, NULL, 0,
                0, str, 1, 0, -1);
            g_free (str);
        } else {
            gchar *str = g_strdup_printf ("All %d %ss", num_p2, self->priv->p2_tag);
            gtk_list_store_insert_with_values (p2_store, NULL, 0,
                0, str, 1, entries->len, -1);
            g_free (str);
        }
    }

    gdk_threads_enter ();
    gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->pane1), GTK_TREE_MODEL (p1_store));
    gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->pane2), GTK_TREE_MODEL (p2_store));
    gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->pane3), GTK_TREE_MODEL (p3_filter));

    GtkTreePath *root = gtk_tree_path_new_from_string ("0");
    gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->pane1)), root);
    gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->pane2)), root);
    gtk_tree_path_free (root);
    gdk_threads_leave ();

    if (self->priv->p1_store) {
        g_object_unref (self->priv->p1_store);
        self->priv->p1_store = GTK_TREE_MODEL (p1_store);
    }

    if (self->priv->p2_store) {
        g_object_unref (self->priv->p2_store);
        self->priv->p2_store = GTK_TREE_MODEL (p2_store);
    }

    if (self->priv->p3_store) {
        g_object_unref (self->priv->p3_store);
        self->priv->p3_store = GTK_TREE_MODEL (p3_store);
    }

    if (self->priv->p3_filter) {
        g_object_unref (self->priv->p3_filter);
        self->priv->p3_filter = GTK_TREE_MODEL (p3_filter);
    }

    g_ptr_array_free (entries, TRUE);
}

static void
pane1_cursor_changed (GtkTreeView *view, Browser *self)
{
    GtkTreePath *path;
    GtkTreeIter iter, siter, first;
    gchar *pane1, *path_str = NULL;
    const gchar *pane2 = NULL;
    Entry *e;
    gint tracks, cnt;

    gtk_tree_view_get_cursor (GTK_TREE_VIEW (self->priv->pane1), &path, NULL);

    if (path) {
        path_str = gtk_tree_path_to_string (path);
    }

    if (path == NULL || !g_strcmp0 (path_str, "0")) {
        if (self->priv->s_p1) {
            g_free (self->priv->s_p1);
            self->priv->s_p1 = NULL;

            // Clear view
            gtk_tree_model_get_iter_first (self->priv->p2_store, &iter);
            while (gtk_list_store_remove (GTK_LIST_STORE (self->priv->p2_store), &iter));

            if (self->priv->p2_single || !self->priv->p2_tag) {
                if (self->priv->p2_tag) {
                    gchar *str = g_strdup_printf ("Select a %s...", self->priv->p1_tag);
                    gtk_list_store_insert_with_values (GTK_LIST_STORE (self->priv->p2_store),
                        NULL, 0, 0, str, 1, 0, -1);
                    g_free (str);
                }

                gtk_tree_model_get_iter_first (self->priv->p3_store, &iter);
                do {
                    gtk_list_store_set (GTK_LIST_STORE (self->priv->p3_store),
                        &iter, 1, TRUE, -1);
                } while (gtk_tree_model_iter_next (self->priv->p3_store, &iter));
            } else {
                gtk_list_store_append (GTK_LIST_STORE (self->priv->p2_store), &first);

                gint tcnt = 1, scnt = 1;
                const gchar *centry = NULL, *nentry;
                self->priv->num_p2 = 1;

                gtk_tree_model_get_iter_first (self->priv->p3_store, &iter);

                gtk_tree_model_get (self->priv->p3_store, &iter, 0, &e, -1);
                centry = entry_get_tag_str (e, self->priv->p2_tag);
                gtk_list_store_append (GTK_LIST_STORE (self->priv->p2_store), &siter);

                while (gtk_tree_model_iter_next (self->priv->p3_store, &iter)) {
                    gtk_list_store_set (GTK_LIST_STORE (self->priv->p3_store), &iter, 1, TRUE, -1);
                    gtk_tree_model_get (self->priv->p3_store, &iter, 0, &e, -1);
                    nentry = entry_get_tag_str (e, self->priv->p2_tag);
                    if (g_strcmp0 (centry, nentry)) {
                        gtk_list_store_set (GTK_LIST_STORE (self->priv->p2_store), &siter,
                            0, centry, 1, scnt, -1);

                        insert_iter (GTK_LIST_STORE (self->priv->p2_store), &siter,
                            (gpointer) nentry, (BrowserCompareFunc) g_strcmp0, 1, TRUE);

                        scnt = 0;
                        centry = nentry;
                        self->priv->num_p2++;
                    }

                    tcnt++;
                    scnt++;
                }

                gchar *str = g_strdup_printf ("All %d %ss", self->priv->num_p2, self->priv->p1_tag);
                gtk_list_store_set (GTK_LIST_STORE (self->priv->p2_store), &first, 0, str, 1, tcnt, -1);
                g_free (str);
            }
        }
    } else {
        gtk_tree_model_get_iter (self->priv->p1_store, &iter, path);
        gtk_tree_model_get (self->priv->p1_store, &iter, 0, &pane1, -1);

        if (g_strcmp0 (self->priv->s_p1, pane1)) {
            if (self->priv->s_p1) {
                g_free (self->priv->s_p1);
            }

            self->priv->s_p1 = g_strdup (pane1);

            // Clear view
            gtk_tree_model_get_iter_first (self->priv->p2_store, &iter);
            while (gtk_list_store_remove (GTK_LIST_STORE (self->priv->p2_store), &iter));

            gtk_tree_model_get_iter_first (self->priv->p3_store, &iter);
            gtk_list_store_append (GTK_LIST_STORE (self->priv->p2_store), &siter);
            cnt = 0;
            self->priv->num_p2 = 0;
            gboolean started = FALSE;
            do {
                gtk_tree_model_get (self->priv->p3_store, &iter, 0, &e, -1);
                if (!g_strcmp0 (self->priv->s_p1, entry_get_tag_str (e, self->priv->p1_tag))) {
                    if (self->priv->p2_tag) {
                        if (pane2) {
                            if (!g_strcmp0 (pane2, entry_get_tag_str (e, self->priv->p2_tag))) {
                                tracks++;
                                cnt++;
                            } else {
                                gtk_list_store_set (GTK_LIST_STORE (self->priv->p2_store),
                                    &siter, 0, pane2, 1, tracks, -1);
                                self->priv->num_p2++;
                                gtk_list_store_append (GTK_LIST_STORE (self->priv->p2_store),
                                    &siter);

                                tracks = 1;
                                cnt++;

                                pane2 = entry_get_tag_str (e, self->priv->p2_tag);
                            }
                        } else {
                            tracks = 1;
                            cnt++;

                            pane2 = entry_get_tag_str (e, self->priv->p2_tag);
                        }
                    }

                    gtk_list_store_set (GTK_LIST_STORE (self->priv->p3_store),
                        &iter, 1, TRUE, -1);
                } else {
                    gtk_list_store_set (GTK_LIST_STORE (self->priv->p3_store),
                        &iter, 1, FALSE, -1);
                }
            } while (gtk_tree_model_iter_next (self->priv->p3_store, &iter));

            if (self->priv->p2_tag) {
                gtk_list_store_set (GTK_LIST_STORE (self->priv->p2_store),
                    &siter, 0, pane2, 1, tracks, -1);
                self->priv->num_p2++;

                gchar *str = g_strdup_printf ("All %d %ss", self->priv->num_p2, self->priv->p2_tag);
                gtk_list_store_insert_with_values (GTK_LIST_STORE (self->priv->p2_store),
                    NULL, 0, 0, str, 1, cnt, -1);
                g_free (str);
            }
        }
    }

    if (path_str) {
        g_free (path_str);
    }

    if (path) {
        gtk_tree_path_free (path);
    }
}

static void
pane2_cursor_changed (GtkTreeView *view, Browser *self)
{
    GtkTreePath *path;
    GtkTreeIter iter;
    gchar *pane2, *path_str = NULL;
    Entry *e;

    gtk_tree_view_get_cursor (GTK_TREE_VIEW (self->priv->pane2), &path, NULL);

    if (path)
        path_str = gtk_tree_path_to_string (path);

    if (path == NULL || !g_strcmp0 (path_str, "0")) {
        if (self->priv->s_p2) {
            g_free (self->priv->s_p2);
            self->priv->s_p2 = NULL;

            gtk_tree_model_get_iter_first (self->priv->p3_store, &iter);
            do {
                gtk_tree_model_get (self->priv->p3_store, &iter, 0, &e, -1);

                gboolean visible = self->priv->s_p1 == NULL ||
                    !g_strcmp0 (self->priv->s_p1, entry_get_tag_str (e, self->priv->p1_tag));

                gtk_list_store_set (GTK_LIST_STORE (self->priv->p3_store),
                    &iter, 1, visible, -1);
            } while (gtk_tree_model_iter_next (self->priv->p3_store, &iter));
        }
    } else {
        gtk_tree_model_get_iter (self->priv->p2_store, &iter, path);
        gtk_tree_model_get (self->priv->p2_store, &iter, 0, &pane2, -1);

        if (g_strcmp0 (self->priv->s_p2, pane2)) {
            if (self->priv->s_p2) {
                g_free (self->priv->s_p2);
            }

            self->priv->s_p2 = g_strdup (pane2);

            gtk_tree_model_get_iter_first (self->priv->p3_store, &iter);
            do {
                gtk_tree_model_get (self->priv->p3_store, &iter, 0, &e, -1);

                gboolean visible = (self->priv->s_p1 == NULL ||
                                   !g_strcmp0 (self->priv->s_p1, entry_get_tag_str (e, self->priv->p1_tag))) &&
                                   (self->priv->s_p2 == NULL ||
                                   !g_strcmp0 (self->priv->s_p2, entry_get_tag_str (e, self->priv->p2_tag)));

                gtk_list_store_set (GTK_LIST_STORE (self->priv->p3_store),
                    &iter, 1, visible, -1);
            } while (gtk_tree_model_iter_next (self->priv->p3_store, &iter));
        }

        g_free (pane2);
    }

    if (path_str) {
        g_free (path_str);
    }

    if (path) {
        gtk_tree_path_free (path);
    }
}

static void
pane1_row_activated (GtkTreeView *view,
                      GtkTreePath *path,
                      GtkTreeViewColumn *column,
                      Browser *self)
{
    Entry *entry = browser_get_next (TRACK_SOURCE (self));
    track_source_emit_play (TRACK_SOURCE (self), entry);
}

static void
pane2_row_activated (GtkTreeView *view,
                     GtkTreePath *path,
                     GtkTreeViewColumn *column,
                     Browser *self)
{
    Entry *entry = browser_get_next (TRACK_SOURCE (self));
    track_source_emit_play (TRACK_SOURCE (self), entry);
}

static void
pane3_row_activated (GtkTreeView *view,
                     GtkTreePath *path,
                     GtkTreeViewColumn *column,
                     Browser *self)
{
    GtkTreeIter iter;
    Entry *entry;

    gtk_tree_model_get_iter (self->priv->p3_filter, &iter, path);
    gtk_tree_model_get (self->priv->p3_filter, &iter, 0, &entry, -1);

    self->priv->s_entry = entry;
    track_source_emit_play (TRACK_SOURCE (self), entry);
}

static void
browser_gmediadb_add (GMediaDB *db, guint id, Browser *self)
{
    gchar **entry = gmediadb_get_entry (self->priv->db, id, self->priv->tags);

    Entry *e = entry_new (entry[0] ? atoi (entry[0]) : 0);

    entry_set_location (e, entry[1]);
    entry_set_media_type (e, self->priv->mtype);

    gint j;
    for (j = 2; self->priv->tags[j]; j++) {
        entry_set_tag_str (e, self->priv->tags[j], entry[j]);
    }

    gdk_threads_enter ();
    browser_insert_entry (self, e);
    gdk_threads_leave ();

    g_object_unref (G_OBJECT (e));
}

static void
browser_gmediadb_remove (GMediaDB *db, guint id, Browser *self)
{
    Entry *entry;
    GtkTreeIter iter;

    gtk_tree_model_iter_children (self->priv->p3_store, &iter, NULL);

    do {
        gtk_tree_model_get (self->priv->p3_store, &iter, 0, &entry, -1);

        if (entry_get_id (entry) == id) {
            gdk_threads_enter ();
            browser_deinsert_entry (self, entry);
            gdk_threads_leave ();
            break;
        }
    } while (gtk_tree_model_iter_next (self->priv->p3_store, &iter));
}

static void
browser_gmediadb_update (GMediaDB *db, guint id, Browser *self)
{
    Entry *oe;

    gchar **entry = gmediadb_get_entry (self->priv->db, id, self->priv->tags);

    Entry *e = entry_new (entry[0] ? atoi (entry[0]) : 0);

    entry_set_location (e, entry[1]);
    entry_set_media_type (e, self->priv->mtype);

    gint j;
    for (j = 2; self->priv->tags[j]; j++) {
        entry_set_tag_str (e, self->priv->tags[j], entry[j]);
    }

    GtkTreeIter iter;
    gtk_tree_model_get_iter_first (self->priv->p3_store, &iter);
    do {
        gtk_tree_model_get (self->priv->p3_store, &iter, 0, &oe, -1);
        if (entry_get_id (oe) == id) {
            break;
        }
        oe = NULL;
    } while (gtk_tree_model_iter_next (self->priv->p3_store, &iter));

    if (oe) {
        browser_deinsert_entry (self, oe);
    }

    browser_insert_entry (self, e);
}

static gboolean
on_pane1_click (GtkWidget *view, GdkEventButton *event, Browser *self)
{
    if (event->button == 3) {
        g_print ("RBUTTON ARTIST\n");

        return TRUE;
    }
    return FALSE;
}

static gboolean
on_pane2_click (GtkWidget *view, GdkEventButton *event, Browser *self)
{
    if (event->button == 3) {
        g_print ("RBUTTON ALBUM\n");
        return TRUE;
    }
    return FALSE;
}

static GtkWidget*
browser_get_move_submenu (Browser *self)
{
    GtkWidget *menu = gtk_menu_new ();

    gchar **dests = shell_get_media_stores (self->priv->shell);

    gint i;
    for (i = 0; dests[i]; i++) {
        if (g_strcmp0 (self->priv->media_type, dests[i])) {
            GtkWidget *item = gtk_menu_item_new_with_label (dests[i]);
            gtk_menu_append (GTK_MENU (menu), item);
            g_signal_connect_swapped (item, "activate", G_CALLBACK (on_pane3_move), self);
        }
    }

    return menu;
}

static gboolean
on_pane3_click (GtkWidget *view, GdkEventButton *event, Browser *self)
{
    if (event->button == 3) {
        GtkTreePath *cpath;
        gboolean retval = TRUE;

        gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (view), event->x, event->y,
            &cpath, NULL, NULL, NULL);

        if (cpath) {
            if (!gtk_tree_selection_path_is_selected (self->priv->p3_sel, cpath)) {
                retval = FALSE;
            }

            gtk_tree_path_free (cpath);
        }

        GtkWidget *menu = gtk_menu_new ();

        GtkWidget *item = gtk_image_menu_item_new_from_stock (GTK_STOCK_INFO, NULL);
        gtk_menu_item_set_label (GTK_MENU_ITEM (item), "Get Info");
        gtk_menu_append (GTK_MENU (menu), item);
        g_signal_connect (item, "activate", G_CALLBACK (on_pane3_info), self);

        gtk_menu_append (GTK_MENU (menu), gtk_separator_menu_item_new ());

        item = gtk_image_menu_item_new_from_stock (GTK_STOCK_CONVERT, NULL);
        gtk_menu_item_set_label (GTK_MENU_ITEM (item), "Move");
        gtk_menu_append (GTK_MENU (menu), item);

        gtk_menu_item_set_submenu (GTK_MENU_ITEM (item),
            browser_get_move_submenu (self));

        gtk_menu_append (GTK_MENU (menu), gtk_separator_menu_item_new ());

        item = gtk_image_menu_item_new_from_stock (GTK_STOCK_REMOVE, NULL);
        gtk_menu_append (GTK_MENU (menu), item);
        g_signal_connect (item, "activate", G_CALLBACK (on_pane3_remove), self);

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
on_pane3_move (Browser *self, GtkWidget *item)
{
    Entry **entries;
    GtkTreeIter iter;
    guint size, i;

    const gchar *label = gtk_menu_item_get_label (GTK_MENU_ITEM (item));

    GList *rows = gtk_tree_selection_get_selected_rows (self->priv->p3_sel, NULL);
    size = g_list_length (rows);

    entries = g_new0 (Entry*, size);
    for (i = 0; i < size; i++) {
        gtk_tree_model_get_iter (self->priv->p3_filter, &iter, g_list_nth_data (rows, i));
        gtk_tree_model_get (self->priv->p3_filter, &iter, 0, &entries[i], -1);
    }

    g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (rows);

    for (i = 0; i < size; i++) {
        gchar **kvs = gmediadb_get_entry (self->priv->db, entry_get_id (entries[i]), NULL);

        media_store_remove_entry (MEDIA_STORE (self), entries[i]);
        entry_set_media_type (entries[i], MEDIA_TVSHOW);

        gint j;
        for (j = 0; kvs[j]; j += 2) {
            entry_set_tag_str (entries[i], kvs[j], kvs[j+1]);
        }

        shell_move_to (self->priv->shell, kvs, label);
    }

    g_free (entries);
}

static void
on_pane3_remove (GtkWidget *item, Browser *self)
{
    Entry **entries;
    GtkTreeIter iter;
    guint size, i;

    GList *rows = gtk_tree_selection_get_selected_rows (self->priv->p3_sel, NULL);
    size = g_list_length (rows);

    entries = g_new0 (Entry*, size);
    for (i = 0; i < size; i++) {
        gtk_tree_model_get_iter (self->priv->p3_filter, &iter, g_list_nth_data (rows, i));
        gtk_tree_model_get (self->priv->p3_filter, &iter, 0, &entries[i], -1);
    }

    g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (rows);

    for (i = 0; i < size; i++) {
        media_store_remove_entry (MEDIA_STORE (self), entries[i]);
    }

    g_free (entries);
}

static void
on_pane3_info (GtkWidget *item, Browser *self)
{
    Entry *e;
    guint size, i, j;
    gchar **keys, **vals;
    GtkTreeIter iter;

    TagDialog *td = tag_dialog_new ();
    g_signal_connect_swapped (td, "completed", G_CALLBACK (on_info_completed), self);

    GList *rows = gtk_tree_selection_get_selected_rows (self->priv->p3_sel, NULL);
    size = g_list_length (rows);

    if (size <= 0) {
        return;
    }

    for (i = 0; i < size; i++) {
        gtk_tree_model_get_iter (self->priv->p3_filter, &iter, g_list_nth_data (rows, i));
        gtk_tree_model_get (self->priv->p3_filter, &iter, 0, &e, -1);

        gchar **kvs = gmediadb_get_entry (self->priv->db, entry_get_id (e), NULL);

        tag_dialog_add_entry (td, kvs);

        g_free (kvs);
    }

    g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (rows);

    tag_dialog_show (td);
}

static void
on_info_completed (Browser *self, GPtrArray *changes, TagDialog *td)
{
    g_print ("on_td_completed: ");
    if (changes && changes->pdata[0]) {
        gint i;
        g_print ("%d changes\n", changes->len / 2);
        for (i = 0; changes->pdata[i]; i += 2) {
            g_print ("%s :: %s\n",
                changes->pdata[i],
                changes->pdata[i+1]);
        }
        g_print ("----------------------\n");
    } else {
        g_print ("No changes recorded\n");
        return;
    }

    Entry **entries;
    GtkTreeIter iter;
    guint size, i, j;
    gchar **keys, **vals;

    GList *rows = gtk_tree_selection_get_selected_rows (self->priv->p3_sel, NULL);
    size = g_list_length (rows);

    entries = g_new0 (Entry*, size);
    for (i = 0; i < size; i++) {
        gtk_tree_model_get_iter (self->priv->p3_filter, &iter, g_list_nth_data (rows, i));
        gtk_tree_model_get (self->priv->p3_filter, &iter, 0, &entries[i], -1);
    }

    g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (rows);

    for (i = 0; i < size; i++) {
        gmediadb_update_entry (self->priv->db, entry_get_id (entries[i]), (gchar**) changes->pdata);
    }

    g_free (entries);
}
