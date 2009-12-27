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

#include "tag-dialog.h"

static void track_source_init (TrackSourceInterface *iface);
G_DEFINE_TYPE_WITH_CODE (Browser, browser, GTK_TYPE_VPANED,
    G_IMPLEMENT_INTERFACE (TRACK_SOURCE_TYPE, track_source_init);
)

struct _BrowserPrivate {
    Shell *shell;

    MediaStore *store;
    gulong ss_add, ss_remove;

    GtkWidget *pane1;
    GtkWidget *sw1;

    GtkWidget *pane2;
    GtkWidget *sw2;

    GtkWidget *pane3;
    GtkWidget *sw3;
    GtkWidget *top_box;

    GtkTreeSelection *p3_sel;

    GtkListStore *p1_store;
    GtkListStore *p2_store;
    GtkListStore *p3_store;
    GtkTreeModel *p3_filter;

    gchar *s_p1;
    gchar *s_p2;
    Entry *s_entry;

    gint num_p1, num_p2;

    EntryCompareFunc cmp_func;
    gchar *p1_tag, *p1_label;
    gchar *p2_tag, *p2_label;
    gboolean p2_single;
    gchar **tags;
};

static gboolean browser_insert_iter (GtkListStore *store, GtkTreeIter *iter,
    gpointer ne, EntryCompareFunc cmp, gint l, gboolean create, GDestroyNotify destroy);
static void entry_changed (Browser *self, Entry *entry);

static gint browser_default_cmp_func (Entry *e1, Entry *e2);
static void num_column_func (GtkTreeViewColumn *column, GtkCellRenderer *cell,
    GtkTreeModel *model, GtkTreeIter *iter, gchar *data);

static void browser_set_pane_visibility (Browser *self);
static void browser_populate_pane1 (Browser *self);
static void browser_populate_pane2 (Browser *self);
static void browser_populate_pane3 (Browser *self);
static void browser_update_pane3 (Browser *self);

static void pane1_cursor_changed (Browser *self, GtkTreeView *view);
static void pane2_cursor_changed (Browser *self, GtkTreeView *view);

static void pane1_row_activated (Browser *self, GtkTreePath *path,
    GtkTreeViewColumn *column, GtkTreeView *view);
static void pane2_row_activated (Browser *self, GtkTreePath *path,
    GtkTreeViewColumn *column, GtkTreeView *view);
static void pane3_row_activated (Browser *self, GtkTreePath *path,
    GtkTreeViewColumn *column, GtkTreeView *view);

static void on_drop (Browser *self, GdkDragContext *drag_context, gint x,
    gint y, GtkSelectionData *data, guint info, guint time, GtkWidget *widget);

// Interface methods
static Entry *browser_get_next (TrackSource *self);
static Entry *browser_get_prev (TrackSource *self);

// Signals from media-store
static void on_store_add (Browser *self, Entry *entry, MediaStore *ms);
static void on_store_remove (Browser *self, Entry *entry, MediaStore *ms);

static void
track_source_init (TrackSourceInterface *iface)
{
    iface->get_next = browser_get_next;
    iface->get_prev = browser_get_prev;
}

static void
browser_finalize (GObject *object)
{
    Browser *self = BROWSER (object);

    //TODO: Free memory
    if (self->priv->store) {
        g_signal_handler_disconnect (self->priv->store, self->priv->ss_add);
        g_signal_handler_disconnect (self->priv->store, self->priv->ss_remove);

        g_object_unref (self->priv->store);
        self->priv->store = NULL;
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
    GtkWidget *sw;

    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), BROWSER_TYPE, BrowserPrivate);

    self->priv->num_p1 = 0;
    self->priv->num_p2 = 0;
    self->priv->s_p1 = NULL;
    self->priv->s_p2 = NULL;
    self->priv->s_entry = NULL;
    self->priv->p2_single = FALSE;
    self->priv->cmp_func = browser_default_cmp_func;
    self->priv->ss_add = -1;
    self->priv->ss_remove = -1;

    // Build UI
    self->priv->top_box = gtk_hbox_new (TRUE, 5);

    self->priv->sw1 = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (self->priv->sw1),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    self->priv->pane1 = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (self->priv->sw1), self->priv->pane1);
    gtk_box_pack_start (GTK_BOX (self->priv->top_box),
        self->priv->sw1, TRUE, TRUE, 0);

    self->priv->sw2 = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (self->priv->sw2),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    self->priv->pane2 = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (self->priv->sw2), self->priv->pane2);
    gtk_box_pack_start (GTK_BOX (self->priv->top_box), self->priv->sw2, TRUE, TRUE, 0);

    gtk_paned_add1 (GTK_PANED (self), self->priv->top_box);

    self->priv->sw3 = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (self->priv->sw3),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    self->priv->pane3 = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (self->priv->sw3), self->priv->pane3);

    gtk_paned_add2 (GTK_PANED (self), self->priv->sw3);

    gtk_widget_show_all (self->priv->sw3);

    // Create Internal GtkListStores
    self->priv->p1_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_UINT);
    self->priv->p2_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_UINT);
    self->priv->p3_store = gtk_list_store_new (2, G_TYPE_OBJECT, G_TYPE_BOOLEAN);
    self->priv->p3_filter = gtk_tree_model_filter_new (
        GTK_TREE_MODEL (self->priv->p3_store), NULL);
    gtk_tree_model_filter_set_visible_column (
        GTK_TREE_MODEL_FILTER (self->priv->p3_filter), 1);

    gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->pane1),
        GTK_TREE_MODEL (self->priv->p1_store));
    gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->pane2),
        GTK_TREE_MODEL (self->priv->p2_store));
    gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->pane3),
        GTK_TREE_MODEL (self->priv->p3_filter));

    self->priv->p3_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->pane3));
    gtk_tree_selection_set_mode (self->priv->p3_sel, GTK_SELECTION_MULTIPLE);

    g_signal_connect_swapped (self->priv->pane1, "cursor-changed",
        G_CALLBACK (pane1_cursor_changed), self);
    g_signal_connect_swapped (self->priv->pane2, "cursor-changed",
        G_CALLBACK (pane2_cursor_changed), self);

    g_signal_connect_swapped (self->priv->pane1, "row-activated",
        G_CALLBACK (pane1_row_activated), self);
    g_signal_connect_swapped (self->priv->pane2, "row-activated",
        G_CALLBACK (pane2_row_activated), self);
    g_signal_connect_swapped (self->priv->pane3, "row-activated",
        G_CALLBACK (pane3_row_activated), self);

    // Setup main widget as a drag destination for filenames
    GtkTargetEntry dest_te = { "text/uri-list", 0, 1 };
    gtk_drag_dest_set (GTK_WIDGET (self), GTK_DEST_DEFAULT_ALL, &dest_te, 1,
        GDK_ACTION_COPY | GDK_ACTION_MOVE);
    g_signal_connect_swapped (self, "drag-data-received", G_CALLBACK (on_drop), self);
}

Browser*
browser_new (Shell *shell)
{
    return browser_new_with_model (shell, NULL);
}

Browser*
browser_new_with_model (Shell *shell, MediaStore *store)
{
    Browser *self = g_object_new (BROWSER_TYPE, NULL);

    self->priv->shell = g_object_ref (shell);

    browser_set_model (self, store);

    return self;
}

void
browser_set_model (Browser *self, MediaStore *store)
{
    GtkTreeIter iter;
    gint i;

    if (self->priv->store) {
        g_signal_handler_disconnect (self->priv->store, self->priv->ss_add);
        g_signal_handler_disconnect (self->priv->store, self->priv->ss_remove);

        g_object_unref (self->priv->store);
        self->priv->store = NULL;
    }

    if (store) {
        self->priv->store = g_object_ref (store);

        self->priv->ss_add = g_signal_connect_data (self->priv->store,
            "add-entry", G_CALLBACK (on_store_add), self,
            NULL, G_CONNECT_SWAPPED);

        self->priv->ss_remove = g_signal_connect_data (self->priv->store,
            "remove-entry", G_CALLBACK (on_store_remove), self,
            NULL, G_CONNECT_SWAPPED);
    }

    if (!store) {
        gtk_list_store_clear (self->priv->p1_store);
        gtk_list_store_clear (self->priv->p2_store);
        gtk_list_store_clear (self->priv->p3_store);
        return;
    }

    browser_populate_pane1 (self);
    browser_populate_pane2 (self);
    browser_populate_pane3 (self);
}

MediaStore*
browser_get_model (Browser *self)
{
    return self->priv->store;
}

void
browser_set_pane1_tag (Browser *self,
                       const gchar *label,
                       const gchar *tag)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    if (self->priv->p1_tag) {
        g_free (self->priv->p1_tag);
        self->priv->p1_tag = NULL;

        g_free (self->priv->p1_label);
        self->priv->p1_label = NULL;
    }

    if (tag) {
        self->priv->p1_tag = g_strdup (tag);
        self->priv->p1_label = g_strdup (label);

        if (column = gtk_tree_view_get_column (GTK_TREE_VIEW (self->priv->pane1), 0)) {
            gtk_tree_view_column_set_title (column, label);
        } else {
            renderer = gtk_cell_renderer_text_new ();
            g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
            column = gtk_tree_view_column_new_with_attributes (label, renderer, "text", 0, NULL);
            gtk_tree_view_column_set_expand (column, TRUE);
            gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->pane1), column);

            renderer = gtk_cell_renderer_text_new ();
            column = gtk_tree_view_column_new_with_attributes ("#", renderer, NULL);
            gtk_tree_view_column_set_cell_data_func (column, renderer,
                (GtkTreeCellDataFunc) num_column_func, NULL, NULL);
            gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->pane1), column);
        }

        if (!self->priv->store) {
            return;
        }

        if (self->priv->s_p1) {
            g_free (self->priv->s_p1);
            self->priv->s_p1 = NULL;

            browser_populate_pane2 (self);
            browser_update_pane3 (self);
        }

        browser_populate_pane1 (self);
    }

    browser_set_pane_visibility (self);
}

gchar*
browser_get_pane1_tag (Browser *self)
{
    return g_strdup (self->priv->p1_tag);
}

void
browser_set_pane2_tag (Browser *self,
                       const gchar *label,
                       const gchar *tag)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    if (self->priv->p2_tag) {
        g_free (self->priv->p2_tag);
        self->priv->p2_tag = NULL;

        g_free (self->priv->p2_label);
        self->priv->p2_label = NULL;
    }

    if (tag) {
        self->priv->p2_tag = g_strdup (tag);
        self->priv->p2_label = g_strdup (label);

        if (column = gtk_tree_view_get_column (GTK_TREE_VIEW (self->priv->pane2), 0)) {
            gtk_tree_view_column_set_title (column, label);
        } else {
            renderer = gtk_cell_renderer_text_new ();
            g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
            column = gtk_tree_view_column_new_with_attributes (label, renderer, "text", 0, NULL);
            gtk_tree_view_column_set_expand (column, TRUE);
            gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->pane2), column);

            renderer = gtk_cell_renderer_text_new ();
            column = gtk_tree_view_column_new_with_attributes ("#", renderer, NULL);
            gtk_tree_view_column_set_cell_data_func (column, renderer,
                (GtkTreeCellDataFunc) num_column_func, NULL, NULL);
            gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->pane2), column);
        }

        if (!self->priv->store) {
            return;
        }

        if (self->priv->s_p2) {
            g_free (self->priv->s_p2);
            self->priv->s_p2 = NULL;

            browser_update_pane3 (self);
        }

        browser_populate_pane2 (self);
    }

    browser_set_pane_visibility (self);
}

gchar*
browser_get_pane2_tag (Browser *self)
{
    return g_strdup (self->priv->p2_tag);
}

void
browser_set_pane2_single_mode (Browser *self, gboolean single_mode)
{
    if (self->priv->p2_single != single_mode) {
        self->priv->p2_single = single_mode;

        browser_populate_pane2 (self);
    }
}

gboolean
browser_get_pane2_single_mode (Browser *self)
{
    return self->priv->p2_single;
}

void
browser_set_compare_func (Browser *self, EntryCompareFunc func)
{
    gboolean resort = FALSE;

    if (func && self->priv->cmp_func != func) {
        self->priv->cmp_func = func;
        resort = TRUE;
    } else if (self->priv->cmp_func != browser_default_cmp_func) {
        self->priv->cmp_func = browser_default_cmp_func;
        resort = TRUE;
    }

    if (!self->priv->store) {
        return;
    }

    if (resort) {
        GtkTreeIter oi, ni;
        Entry *e;

        GtkListStore *p3_store = gtk_list_store_new (2, G_TYPE_OBJECT, G_TYPE_BOOLEAN);

        if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->p3_store), &oi)) {
            return;
        }

        do {
            gtk_tree_model_get (GTK_TREE_MODEL (self->priv->p3_store), &oi, 0, &e, -1);

            gboolean vis = TRUE;

            if (self->priv->p1_tag) {
                vis &= (!self->priv->s_p1 ||
                        !g_strcmp0 (self->priv->s_p1, entry_get_tag_str (e, self->priv->p1_tag)));
            }

            if (self->priv->p2_tag) {
                vis &= (!self->priv->s_p2 ||
                        !g_strcmp0 (self->priv->s_p2, entry_get_tag_str (e, self->priv->p2_tag)));
            }

            browser_insert_iter (p3_store, &ni, e,
                self->priv->cmp_func, 0, TRUE, g_object_unref);

            gtk_list_store_set (p3_store, &ni, 0, e, 1, vis, -1);

            g_object_unref (e);
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->p3_store), &oi));

        g_object_unref (self->priv->p3_store);
        g_object_unref (self->priv->p3_filter);

        self->priv->p3_store = p3_store;
        self->priv->p3_filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (p3_store), NULL);
        gtk_tree_model_filter_set_visible_column (GTK_TREE_MODEL_FILTER (self->priv->p3_filter), 1);

        gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->pane3), self->priv->p3_filter);
    }
}

void
browser_add_column (Browser *self,
                    const gchar *label,
                    const gchar *tag,
                    gboolean expand,
                    GtkTreeCellDataFunc col_func)
{
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
    g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);

    GtkTreeViewColumn *column =
        gtk_tree_view_column_new_with_attributes (label, renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer, col_func,
        g_strdup (tag), g_free);
    g_object_set (column, "expand", expand, NULL);

    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->pane3), column);
}

static void
browser_set_pane_visibility (Browser *self)
{
    if (self->priv->p1_tag) {
        gtk_widget_show_all (self->priv->top_box);
        if (self->priv->p2_tag) {
            gtk_widget_show_all (self->priv->sw2);
        } else {
            gtk_widget_hide (self->priv->sw2);
        }
    } else {
        gtk_widget_hide (self->priv->top_box);
    }
}

static void
browser_populate_pane1 (Browser *self)
{
    GtkTreeIter iter;
    gint i, cnt, num_p1 = 0;

    gtk_list_store_clear (self->priv->p1_store);

    if (!self->priv->store || !self->priv->p1_tag) {
        return;
    }

    Entry **entries = media_store_get_all_entries (self->priv->store);

    for (i = 0; entries[i]; i++) {
        const gchar *pane1 = entry_get_tag_str (entries[i], self->priv->p1_tag);

        gint res = browser_insert_iter (self->priv->p1_store, &iter,
            (gpointer) pane1, (EntryCompareFunc) g_strcmp0, 0, FALSE, g_free);

        gtk_tree_model_get (GTK_TREE_MODEL (self->priv->p1_store),
            &iter, 1, &cnt, -1);
        gtk_list_store_set (self->priv->p1_store, &iter,
            0, pane1, 1, cnt + 1, -1);

        if (res) {
            num_p1++;
        }
    }

    self->priv->num_p1 = num_p1;

    g_free (entries);

    gchar *new_str = g_strdup_printf ("All %d %ss", num_p1, self->priv->p1_label);

    gtk_list_store_insert_with_values (self->priv->p1_store, NULL, 0,
        0, new_str, 1, i, -1);

    g_free (new_str);

    GtkTreePath *root = gtk_tree_path_new_from_string ("0");
    gtk_tree_selection_select_path (gtk_tree_view_get_selection (
        GTK_TREE_VIEW (self->priv->pane1)), root);
    gtk_tree_path_free (root);
}

static void
browser_populate_pane2 (Browser *self)
{
    GtkTreeIter iter;
    gint i, cnt, num_p2 = 0, tcnt = 0;

    if (!self->priv->store || !self->priv->p2_tag) {
        gtk_list_store_clear (self->priv->p2_store);
        return;
    }

    if (self->priv->s_p1 == NULL && self->priv->p2_single) {
        gtk_list_store_clear (self->priv->p2_store);

        gchar *str = g_strdup_printf ("Select a %s", self->priv->p1_label);
        gtk_list_store_insert_with_values (self->priv->p2_store, NULL, 0,
            0, str, 1, 0, -1);
        g_free (str);

        return;
    }

    GtkListStore *p2_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);

    Entry **entries = media_store_get_all_entries (self->priv->store);

    for (i = 0; entries[i]; i++) {
        const gchar *pane1 = entry_get_tag_str (entries[i], self->priv->p1_tag);
        const gchar *pane2 = entry_get_tag_str (entries[i], self->priv->p2_tag);

        if (self->priv->s_p1 && g_strcmp0 (self->priv->s_p1, pane1)) {
            continue;
        }

        gint res = browser_insert_iter (p2_store, &iter, (gpointer) pane2,
            (EntryCompareFunc) g_strcmp0, 0, FALSE, g_free);

        gtk_tree_model_get (GTK_TREE_MODEL (p2_store),
            &iter, 1, &cnt, -1);
        gtk_list_store_set (p2_store, &iter,
            0, pane2, 1, cnt + 1, -1);
        tcnt++;

        if (res) {
            num_p2++;
        }
    }

    self->priv->num_p2 = num_p2;

    g_free (entries);

    gchar *new_str = g_strdup_printf ("All %d %ss", num_p2, self->priv->p2_label);

    gtk_list_store_insert_with_values (p2_store, NULL, 0,
        0, new_str, 1, tcnt, -1);

    g_free (new_str);

    g_object_unref (self->priv->p2_store);
    self->priv->p2_store = p2_store;
    gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->pane2),
                             GTK_TREE_MODEL (p2_store));

    if (self->priv->s_p2) {
        // If there is still a selected p2_tag, select it
        gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->p2_store), &iter);
        do {
            gchar *str;
            gtk_tree_model_get (GTK_TREE_MODEL (self->priv->p2_store), &iter, 0, &str, -1);

            if (!g_strcmp0 (str, self->priv->s_p2)) {
                gtk_tree_selection_select_iter (gtk_tree_view_get_selection (
                    GTK_TREE_VIEW (self->priv->pane2)), &iter);
                g_free (str);
                break;
            }
            g_free (str);
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->p2_store), &iter));
    } else {
        // if not just select the first (all) entry
        GtkTreePath *root = gtk_tree_path_new_from_string ("0");
        gtk_tree_selection_select_path (gtk_tree_view_get_selection (
            GTK_TREE_VIEW (self->priv->pane2)), root);
        gtk_tree_path_free (root);
    }
}

static void
browser_populate_pane3 (Browser *self)
{
    GtkTreeIter iter;
    gint i;

    GtkListStore *p3_store = gtk_list_store_new (2, G_TYPE_OBJECT, G_TYPE_BOOLEAN);

    if (!self->priv->store) {
        return;
    }

    Entry **entries = media_store_get_all_entries (self->priv->store);

    for (i = 0; entries[i]; i++) {
        gboolean vis = TRUE;

        if (self->priv->p1_tag) {
            const gchar *pane1 = entry_get_tag_str (entries[i], self->priv->p1_tag);
            if (self->priv->s_p1 && g_strcmp0 (self->priv->s_p1, pane1)) {
                vis = FALSE;
            }
        }

        if (self->priv->p2_tag) {
            const gchar *pane2 = entry_get_tag_str (entries[i], self->priv->p2_tag);
            if (self->priv->s_p2 && g_strcmp0 (self->priv->s_p2, pane2)) {
                vis = FALSE;
            }
        }

//        if (vis) {
            browser_insert_iter (p3_store, &iter, entries[i],
                self->priv->cmp_func, 0, TRUE, g_object_unref);

            gtk_list_store_set (p3_store, &iter, 0, entries[i], 1, vis, -1);
//        }
    }

    g_object_unref (self->priv->p3_store);
    g_object_unref (self->priv->p3_filter);
    self->priv->p3_store = p3_store;
    self->priv->p3_filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (self->priv->p3_store), NULL);
    gtk_tree_model_filter_set_visible_column (GTK_TREE_MODEL_FILTER (self->priv->p3_filter), 1);

    gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->pane3), self->priv->p3_filter);

    g_free (entries);
}

static void
browser_update_pane3 (Browser *self)
{
    GtkTreeIter iter;
    Entry *e;

    if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->p3_store), &iter)) {
        return;
    }

    do {
        gtk_tree_model_get (GTK_TREE_MODEL (self->priv->p3_store), &iter, 0, &e, -1);

        gboolean vis = TRUE;

        if (self->priv->p1_tag) {
            vis &= (!self->priv->s_p1 ||
                    !g_strcmp0 (self->priv->s_p1, entry_get_tag_str (e, self->priv->p1_tag)));
        }

        if (self->priv->p2_tag) {
            vis &= (!self->priv->s_p2 ||
                    !g_strcmp0 (self->priv->s_p2, entry_get_tag_str (e, self->priv->p2_tag)));
        }

        gtk_list_store_set (self->priv->p3_store, &iter, 1, vis, -1);
        g_object_unref (e);
    } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->p3_store), &iter));

    g_object_unref (self->priv->p3_filter);
    self->priv->p3_filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (self->priv->p3_store), NULL);
    gtk_tree_model_filter_set_visible_column (GTK_TREE_MODEL_FILTER (self->priv->p3_filter), 1);

    gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->pane3), self->priv->p3_filter);
}

static gboolean
browser_insert_iter (GtkListStore *store,
                     GtkTreeIter *iter,
                     gpointer ne,
                     EntryCompareFunc cmp,
                     gint l,
                     gboolean create,
                     GDestroyNotify destroy)
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
    destroy (e);
    if (res == 0 && !create) {
        return FALSE;
    } else if (res <= 0) {
        gtk_list_store_insert (store, iter, l);
        return TRUE;
    }

    gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (store), iter, NULL, r - 1);
    gtk_tree_model_get (GTK_TREE_MODEL (store), iter, 0, &e, -1);

    res = cmp (ne, e);
    destroy (e);
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
        destroy (e);
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
    destroy (e);
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
        g_object_unref (entry);
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
entry_changed (Browser *self, Entry *entry)
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

static gint
browser_default_cmp_func (Entry *e1, Entry *e2)
{
    if (e1 > e2) {
        return 1;
    } else if (e1 < e2) {
        return -1;
    } else {
        return 0;
    }
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

static void
pane1_cursor_changed (Browser *self, GtkTreeView *view)
{
    GtkTreePath *path;
    GtkTreeIter iter;
    gchar *pane1, *path_str = NULL;
    gboolean update = FALSE;

    gtk_tree_view_get_cursor (GTK_TREE_VIEW (self->priv->pane1), &path, NULL);

    if (path) {
        path_str = gtk_tree_path_to_string (path);
    }

    if (path == NULL || !g_strcmp0 (path_str, "0")) {
        if (self->priv->s_p1) {
            g_free (self->priv->s_p1);
            self->priv->s_p1 = NULL;
            update = TRUE;
        }

        if (self->priv->s_p2 && self->priv->p2_single) {
            g_free (self->priv->s_p2);
            self->priv->s_p2 = NULL;
        }
    } else {
        gtk_tree_model_get_iter (GTK_TREE_MODEL (self->priv->p1_store),
                                 &iter, path);
        gtk_tree_model_get (GTK_TREE_MODEL (self->priv->p1_store), &iter,
                            0, &pane1, -1);

        if (g_strcmp0 (self->priv->s_p1, pane1)) {
            if (self->priv->s_p1) {
                g_free (self->priv->s_p1);
            }

            self->priv->s_p1 = g_strdup (pane1);

            if (self->priv->s_p2) {
                g_free (self->priv->s_p2);
                self->priv->s_p2 = NULL;
            }

            update = TRUE;
        }

        if (pane1) {
            g_free (pane1);
        }
    }

    if (update) {
        browser_populate_pane2 (self);
        browser_update_pane3 (self);
    }

    if (path_str) {
        g_free (path_str);
    }

    if (path) {
        gtk_tree_path_free (path);
    }
}

static void
pane2_cursor_changed (Browser *self, GtkTreeView *view)
{
    GtkTreePath *path;
    GtkTreeIter iter;
    gchar *pane2, *path_str = NULL;

    gtk_tree_view_get_cursor (GTK_TREE_VIEW (self->priv->pane2), &path, NULL);

    if (path) {
        path_str = gtk_tree_path_to_string (path);
    }

    if (path == NULL || !g_strcmp0 (path_str, "0")) {
        if (self->priv->s_p2) {
            g_free (self->priv->s_p2);
            self->priv->s_p2 = NULL;
        }
    } else {
        gtk_tree_model_get_iter (GTK_TREE_MODEL (self->priv->p2_store),
                                 &iter, path);
        gtk_tree_model_get (GTK_TREE_MODEL (self->priv->p2_store), &iter,
                            0, &pane2, -1);

        if (g_strcmp0 (self->priv->s_p2, pane2)) {
            if (self->priv->s_p2) {
                g_free (self->priv->s_p2);
            }

            self->priv->s_p2 = g_strdup (pane2);
        }

        if (pane2) {
            g_free (pane2);
        }
    }

    browser_update_pane3 (self);

    if (path_str) {
        g_free (path_str);
    }

    if (path) {
        gtk_tree_path_free (path);
    }
}

static void
pane1_row_activated (Browser *self,
                     GtkTreePath *path,
                     GtkTreeViewColumn *column,
                     GtkTreeView *view)
{
    Entry *entry = browser_get_next (TRACK_SOURCE (self));
    track_source_emit_play (TRACK_SOURCE (self), entry);
}

static void
pane2_row_activated (Browser *self,
                     GtkTreePath *path,
                     GtkTreeViewColumn *column,
                     GtkTreeView *view)
{
    Entry *entry = browser_get_next (TRACK_SOURCE (self));
    track_source_emit_play (TRACK_SOURCE (self), entry);
}

static void
pane3_row_activated (Browser *self,
                     GtkTreePath *path,
                     GtkTreeViewColumn *column,
                     GtkTreeView *view)
{
    GtkTreeIter iter;
    Entry *entry;

    gtk_tree_model_get_iter (self->priv->p3_filter, &iter, path);
    gtk_tree_model_get (self->priv->p3_filter, &iter, 0, &entry, -1);

    self->priv->s_entry = entry;
    track_source_emit_play (TRACK_SOURCE (self), entry);
}

static void
on_store_add (Browser *self, Entry *entry, MediaStore *ms)
{
    GtkTreeIter first, iter;
    gint cnt;
    gchar *new_str;
    gboolean res;
    gboolean tv = TRUE;

    gdk_threads_enter ();

    if (self->priv->p1_tag) {
        const gchar *pane1 = entry_get_tag_str (entry, self->priv->p1_tag);

        tv = self->priv->s_p1 == NULL || !g_strcmp0 (self->priv->s_p1, pane1);

        // Add pane1 to view
        res = browser_insert_iter (self->priv->p1_store, &iter, (gpointer) pane1,
            (EntryCompareFunc) g_strcmp0, 1, FALSE, (GDestroyNotify) g_free);

        gtk_tree_model_get (GTK_TREE_MODEL (self->priv->p1_store), &iter,
                            1, &cnt, -1);
        gtk_list_store_set (self->priv->p1_store, &iter,
                            0, pane1, 1, cnt + 1, -1);

        gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->p1_store), &first);
        gtk_tree_model_get (GTK_TREE_MODEL (self->priv->p1_store), &first, 1, &cnt, -1);

        if (res) {
            new_str = g_strdup_printf ("All %d %ss", ++self->priv->num_p1, self->priv->p1_tag);
            gtk_list_store_set (self->priv->p1_store, &first,
                0, new_str, 1, cnt + 1, -1);
            g_free (new_str);
        } else {
            gtk_list_store_set (self->priv->p1_store, &first,
                1, cnt + 1, -1);
        }

        if (self->priv->p2_tag) {
            if ((!self->priv->p2_single && self->priv->s_p2 == NULL) || !g_strcmp0 (self->priv->s_p1, pane1)) {
                const gchar *pane2 = entry_get_tag_str (entry, self->priv->p2_tag);

                tv &= (self->priv->s_p2 == NULL || !g_strcmp0 (self->priv->s_p2, pane2));

                // Add pane2 to view
                res = browser_insert_iter (self->priv->p2_store, &iter, (gpointer) pane2,
                    (EntryCompareFunc) g_strcmp0, 1, FALSE, g_free);

                gtk_tree_model_get (GTK_TREE_MODEL (self->priv->p2_store), &iter,
                                    1, &cnt, -1);
                gtk_list_store_set (self->priv->p2_store, &iter,
                                    0, pane2, 1, cnt + 1, -1);

                gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->p2_store), &first);
                gtk_tree_model_get (GTK_TREE_MODEL (self->priv->p2_store), &first, 1, &cnt, -1);

                if (res) {
                    new_str = g_strdup_printf ("All %d %ss",
                        ++self->priv->num_p2, self->priv->p2_tag);

                    gtk_list_store_set (self->priv->p2_store, &first,
                        0, new_str, 1, cnt + 1, -1);

                    g_free (new_str);
                } else {
                    gtk_list_store_set (self->priv->p2_store, &first,
                        1, cnt + 1, -1);
                }
            }
        }
    }

    browser_insert_iter (self->priv->p3_store, &iter, entry, self->priv->cmp_func,
                         0, TRUE, g_object_unref);

    gtk_list_store_set (self->priv->p3_store, &iter, 0, entry, 1, tv, -1);

    gdk_threads_leave ();
}

static void
on_store_remove (Browser *self, Entry *entry, MediaStore *ms)
{
    GtkTreeIter first, iter;
    gint cnt;
    gchar *new_str;

    gdk_threads_enter ();

    if (self->priv->p1_tag) {
        const gchar *pane1 = entry_get_tag_str (entry, self->priv->p1_tag);

        browser_insert_iter (self->priv->p1_store, &iter,
            (gpointer) pane1, (EntryCompareFunc) g_strcmp0, 1, FALSE, g_free);

        gtk_tree_model_get (GTK_TREE_MODEL (self->priv->p1_store), &iter, 1, &cnt, -1);

        if (cnt == 1) {
            gtk_list_store_remove (self->priv->p1_store, &iter);

            gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->p1_store), &first);
            gtk_tree_model_get (GTK_TREE_MODEL (self->priv->p1_store), &first, 1, &cnt, -1);

            new_str = g_strdup_printf ("All %d %ss", --self->priv->num_p1, self->priv->p1_tag);
            gtk_list_store_set (self->priv->p1_store, &first,
                0, new_str, 1, cnt - 1, -1);
            g_free (new_str);
        } else {
            gtk_list_store_set (self->priv->p1_store, &iter, 1, cnt - 1, -1);

            gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->p1_store), &first);
            gtk_tree_model_get (GTK_TREE_MODEL (self->priv->p1_store), &first, 1, &cnt, -1);
            gtk_list_store_set (self->priv->p1_store, &first, 1, cnt - 1, -1);
        }

        if ( self->priv->p2_tag) {
            if ((!self->priv->p2_single && self->priv->s_p2 == NULL) || !g_strcmp0 (self->priv->s_p1, pane1)) {
                const gchar *pane2 = entry_get_tag_str (entry, self->priv->p2_tag);

                browser_insert_iter (self->priv->p2_store, &iter, (gpointer) pane2,
                         (EntryCompareFunc) g_strcmp0, 1, FALSE, g_free);

                gtk_tree_model_get (GTK_TREE_MODEL (self->priv->p2_store), &iter, 1, &cnt, -1);

                if (cnt == 1) {
                    gtk_list_store_remove (self->priv->p2_store, &iter);

                    gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->p2_store), &first);
                    gtk_tree_model_get (GTK_TREE_MODEL (self->priv->p2_store), &first, 1, &cnt, -1);

                    new_str = g_strdup_printf ("All %d %ss", --self->priv->num_p2, self->priv->p2_tag);
                    gtk_list_store_set (self->priv->p2_store, &first,
                        0, new_str, 1, cnt - 1, -1);
                    g_free (new_str);
                } else {
                    gtk_list_store_set (self->priv->p2_store, &iter, 1, cnt - 1, -1);

                    gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->p2_store), &first);
                    gtk_tree_model_get (GTK_TREE_MODEL (self->priv->p2_store), &first, 1, &cnt, -1);
                    gtk_list_store_set (self->priv->p2_store, &first,
                        1, cnt - 1, -1);
                }
            }
        }
    }

    browser_insert_iter (self->priv->p3_store, &iter, entry,
                         self->priv->cmp_func, 0, FALSE, g_object_unref);

    gtk_list_store_remove (self->priv->p3_store, &iter);

    gdk_threads_leave ();
}

static void
on_drop (Browser *self, GdkDragContext *drag_context, gint x, gint y,
         GtkSelectionData *data, guint info, guint time, GtkWidget *widget)
{
    if (info == 1) {
        gchar **uris = g_strsplit (data->data, "\r\n", -1);

        if (uris) {
            gint i;
            for (i = 0; uris[i]; i++) {
                gchar *ustr = g_uri_unescape_string (uris[i]+7, "");
                shell_import_path (self->priv->shell, ustr, media_store_get_name (self->priv->store));
                g_free (ustr);
            }

            g_strfreev (uris);
        }
    } else {
        g_print ("Unknown info %d\n", info);
    }
}
