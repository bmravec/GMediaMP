/*
 *      shows.c
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
#include "shows.h"

#include "track-source.h"
#include "media-store.h"

#include "tag-dialog.h"

static void track_source_init (TrackSourceInterface *iface);
static void media_store_init (MediaStoreInterface *iface);
G_DEFINE_TYPE_WITH_CODE (Shows, shows, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TRACK_SOURCE_TYPE, track_source_init);
    G_IMPLEMENT_INTERFACE (MEDIA_STORE_TYPE, media_store_init)
)

struct _ShowsPrivate {
    Shell *shell;
    GMediaDB *db;

    GtkWidget *widget;
    GtkWidget *show;
    GtkWidget *season;
    GtkWidget *title;

    GtkTreeSelection *title_sel;

    GtkTreeModel *show_store;
    GtkTreeModel *season_store;
    GtkTreeModel *title_store;
    GtkTreeModel *title_filter;

    gchar *s_show;
    gchar *s_season;
    Entry *s_entry;

    gint num_shows, num_seasons;
};

typedef gint (*ShowsCompareFunc) (gpointer a, gpointer b);

static void str_column_func (GtkTreeViewColumn *column, GtkCellRenderer *cell,
    GtkTreeModel *model, GtkTreeIter *iter, gchar *data);
static void int_column_func (GtkTreeViewColumn *column, GtkCellRenderer *cell,
    GtkTreeModel *model, GtkTreeIter *iter, gchar *data);
static void status_column_func (GtkTreeViewColumn *column, GtkCellRenderer *cell,
    GtkTreeModel *model, GtkTreeIter *iter, gpointer data);
static void time_column_func (GtkTreeViewColumn *column, GtkCellRenderer *cell,
    GtkTreeModel *model, GtkTreeIter *iter, gchar *data);

static void show_cursor_changed (GtkTreeView *view, Shows *self);
static void season_cursor_changed (GtkTreeView *view, Shows *self);

static void show_row_activated (GtkTreeView *view, GtkTreePath *path,
    GtkTreeViewColumn *column, Shows *self);
static void season_row_activated (GtkTreeView *view, GtkTreePath *path,
    GtkTreeViewColumn *column, Shows *self);
static void title_row_activated (GtkTreeView *view, GtkTreePath *path,
    GtkTreeViewColumn *column, Shows *self);

static gboolean on_show_click (GtkWidget *view, GdkEventButton *event, Shows *self);
static gboolean on_season_click (GtkWidget *view, GdkEventButton *event, Shows *self);
static gboolean on_title_click (GtkWidget *view, GdkEventButton *event, Shows *self);

static void on_title_remove (GtkWidget *item, Shows *self);
static void on_title_info (GtkWidget *item, Shows *self);

static gpointer initial_import (Shows *self);
static gboolean insert_iter (GtkListStore *store, GtkTreeIter *iter,
    gpointer ne, ShowsCompareFunc cmp, gint l, gboolean create);
static void entry_changed (Entry *entry, Shows *self);

static void on_info_completed (Shows *self, GPtrArray *changes, TagDialog *td);
static void on_info_save (GtkWidget *widget, Shows *self);
static void on_info_cancel (GtkWidget *widget, Shows *self);

static gint show_entry_cmp (Entry *e1, Entry *e2);

// Interface methods
static Entry *shows_get_next (TrackSource *self);
static Entry *shows_get_prev (TrackSource *self);

static void shows_add_entry (MediaStore *self, Entry *entry);
static void shows_insert_entry (Shows *self, Entry *entry);
static void shows_remove_entry (MediaStore *self, Entry *entry);
static void shows_deinsert_entry (Shows *self, Entry *entry);
static guint shows_get_mtype (MediaStore *self);

// Signals from GMediaDB
static void shows_gmediadb_add (GMediaDB *db, guint id, Shows *self);
static void shows_gmediadb_remove (GMediaDB *db, guint id, Shows *self);
static void shows_gmediadb_update (GMediaDB *db, guint id, Shows *self);

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
    iface->get_next = shows_get_next;
    iface->get_prev = shows_get_prev;
}

static void
media_store_init (MediaStoreInterface *iface)
{
    iface->add_entry = shows_add_entry;
    iface->rem_entry = shows_remove_entry;
    iface->get_mtype = shows_get_mtype;
}

static void
shows_finalize (GObject *object)
{
    Shows *self = SHOWS (object);

    if (self->priv->db) {
        g_object_unref (self->priv->db);
    }

    G_OBJECT_CLASS (shows_parent_class)->finalize (object);
}

static void
shows_class_init (ShowsClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (ShowsPrivate));

    object_class->finalize = shows_finalize;
}

static void
shows_init (Shows *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), SHOWS_TYPE, ShowsPrivate);

    self->priv->num_shows = 0;
    self->priv->num_seasons = 0;

    self->priv->s_show = NULL;
    self->priv->s_season = NULL;
    self->priv->s_entry = NULL;
}

Shows*
shows_new ()
{
    return g_object_new (SHOWS_TYPE, NULL);
}

gboolean
shows_activate (Shows *self)
{
    self->priv->shell = shell_new ();
    self->priv->db = gmediadb_new ("TVShows");

    GtkBuilder *builder = shell_get_builder (self->priv->shell);

    GError *err = NULL;
    gtk_builder_add_from_file (builder, SHARE_DIR "/ui/shows.ui", &err);

    if (err) {
        g_print ("ERROR ADDING: %s", err->message);
        g_error_free (err);
    }

    self->priv->widget = GTK_WIDGET (gtk_builder_get_object (builder, "shows_vpane"));
    self->priv->show = GTK_WIDGET (gtk_builder_get_object (builder, "shows_show"));
    self->priv->season = GTK_WIDGET (gtk_builder_get_object (builder, "shows_season"));
    self->priv->title = GTK_WIDGET (gtk_builder_get_object (builder, "shows_title"));

    self->priv->show_store = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_UINT));
    self->priv->season_store = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_UINT));
    self->priv->title_store = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_OBJECT, G_TYPE_BOOLEAN));

    self->priv->title_filter = gtk_tree_model_filter_new (self->priv->title_store, NULL);
    gtk_tree_model_filter_set_visible_column (GTK_TREE_MODEL_FILTER (self->priv->title_filter), 1);

    gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->show), self->priv->show_store);
    gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->season), self->priv->season_store);
    gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->title), self->priv->title_filter);

    self->priv->title_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->title));
    gtk_tree_selection_set_mode (self->priv->title_sel, GTK_SELECTION_MULTIPLE);

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    // Create Columns for Shows
    renderer = gtk_cell_renderer_text_new ();
    g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
    column = gtk_tree_view_column_new_with_attributes ("Show", renderer, "text", 0, NULL);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->show), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("#", renderer, "text", 1, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->show), column);

    gtk_list_store_insert_with_values (GTK_LIST_STORE (self->priv->show_store),
        NULL, 0, 0, "All 0 Shows", 1, 0, -1);

    // Create Columns for Seasons
    renderer = gtk_cell_renderer_text_new ();
    g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
    column = gtk_tree_view_column_new_with_attributes ("Season", renderer, "text", 0, NULL);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->season), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("#", renderer, "text", 1, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->season), column);

    gtk_list_store_insert_with_values (GTK_LIST_STORE (self->priv->season_store),
        NULL, 0, 0, "Select a show...", 1, 0, -1);

    // Select All for both show and season
    GtkTreePath *root = gtk_tree_path_new_from_string ("0");
    gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->show)), root);
    gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->season)), root);
    gtk_tree_path_free (root);

    // Create Columns for Title
    renderer = gtk_cell_renderer_pixbuf_new ();
    column = gtk_tree_view_column_new_with_attributes ("", renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
        (GtkTreeCellDataFunc) status_column_func, NULL, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->title), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Track", renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
        (GtkTreeCellDataFunc) int_column_func, "track", NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->title), column);

    renderer = gtk_cell_renderer_text_new ();
    g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
    column = gtk_tree_view_column_new_with_attributes ("Title", renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
        (GtkTreeCellDataFunc) str_column_func, "title", NULL);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->title), column);

    renderer = gtk_cell_renderer_text_new ();
    g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
    column = gtk_tree_view_column_new_with_attributes ("Show", renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
        (GtkTreeCellDataFunc) str_column_func, "show", NULL);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->title), column);

    renderer = gtk_cell_renderer_text_new ();
    g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
    column = gtk_tree_view_column_new_with_attributes ("Season", renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
        (GtkTreeCellDataFunc) str_column_func, "season", NULL);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->title), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Time", renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
        (GtkTreeCellDataFunc) time_column_func, "duration", NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->title), column);

    // Connect Signals
    g_signal_connect (G_OBJECT (self->priv->show), "cursor-changed", G_CALLBACK (show_cursor_changed), self);
    g_signal_connect (G_OBJECT (self->priv->season), "cursor-changed", G_CALLBACK (season_cursor_changed), self);

    g_signal_connect (G_OBJECT (self->priv->show), "row-activated", G_CALLBACK (show_row_activated), self);
    g_signal_connect (G_OBJECT (self->priv->season), "row-activated", G_CALLBACK (season_row_activated), self);
    g_signal_connect (G_OBJECT (self->priv->title), "row-activated", G_CALLBACK (title_row_activated), self);

    g_signal_connect (self->priv->show, "button-press-event", G_CALLBACK (on_show_click), self);
    g_signal_connect (self->priv->season, "button-press-event", G_CALLBACK (on_season_click), self);
    g_signal_connect (self->priv->title, "button-press-event", G_CALLBACK (on_title_click), self);

    g_signal_connect (self->priv->db, "add-entry", G_CALLBACK (shows_gmediadb_add), self);
    g_signal_connect (self->priv->db, "remove-entry", G_CALLBACK (shows_gmediadb_remove), self);
    g_signal_connect (self->priv->db, "update-entry", G_CALLBACK (shows_gmediadb_update), self);

    shell_add_widget (self->priv->shell, gtk_label_new ("Library"), "Library", NULL);
    shell_add_widget (self->priv->shell, self->priv->widget, "Library/TV Shows", NULL);

    gtk_widget_show_all (self->priv->widget);

//    g_thread_create ((GThreadFunc) initial_import, self, FALSE, NULL);
    initial_import (self);
}

gboolean
shows_deactivate (Shows *self)
{

}

static void
shows_add_entry (MediaStore *self, Entry *entry)
{
    ShowsPrivate *priv = SHOWS (self)->priv;

    if (entry_get_tag_str (entry, "show") == NULL) {
        entry_set_tag_str (entry, "show", "Unknown Show");
    }

    if (entry_get_tag_str (entry, "season") == NULL) {
        entry_set_tag_str (entry, "season", "Unknown Season");
    }

    gchar **keys, **vals;

    guint size = entry_get_key_value_pairs (entry, &keys, &vals);

    gmediadb_add_entry (priv->db, keys, vals);

    g_strfreev (keys);
    g_strfreev (vals);
}

static void
shows_insert_entry (Shows *self, Entry *entry)
{
    ShowsPrivate *priv = self->priv;
    GtkTreeIter first, iter;
    gint cnt;
    gchar *new_str;
    gboolean res;

    const gchar *show = entry_get_tag_str (entry, "show");
    const gchar *season = entry_get_tag_str (entry, "season");

    gboolean sev = !g_strcmp0 (priv->s_show, show);
    gboolean tv = (priv->s_show == NULL || sev) &&
        (priv->s_season == NULL || !g_strcmp0 (priv->s_season, season));

    // Add show to view
    res = insert_iter (GTK_LIST_STORE (priv->show_store), &iter,
        (gpointer) show, (ShowsCompareFunc) g_strcmp0, 1, FALSE);

    gtk_tree_model_get (priv->show_store, &iter, 1, &cnt, -1);
    gtk_list_store_set (GTK_LIST_STORE (priv->show_store), &iter,
        0, show, 1, cnt + 1, -1);

    gtk_tree_model_get_iter_first (priv->show_store, &first);
    gtk_tree_model_get (priv->show_store, &first, 1, &cnt, -1);

    if (res) {
        new_str = g_strdup_printf ("All %d Shows", ++priv->num_shows);
        gtk_list_store_set (GTK_LIST_STORE (priv->show_store), &first,
            0, new_str, 1, cnt + 1, -1);
        g_free (new_str);
    } else {
        gtk_list_store_set (GTK_LIST_STORE (priv->show_store), &first,
            1, cnt + 1, -1);
    }

    // Add season to view
    if (sev) {
        res = insert_iter (GTK_LIST_STORE (priv->season_store), &iter,
            (gpointer) season, (ShowsCompareFunc) g_strcmp0, 1, FALSE);

        gtk_tree_model_get (priv->season_store, &iter, 1, &cnt, -1);
        gtk_list_store_set (GTK_LIST_STORE (priv->season_store), &iter,
            0, season, 1, cnt + 1, -1);

        gtk_tree_model_get_iter_first (priv->season_store, &first);
        gtk_tree_model_get (priv->season_store, &first, 1, &cnt, -1);

        if (res) {
            new_str = g_strdup_printf ("All %d Seasons", ++priv->num_seasons);
            gtk_list_store_set (GTK_LIST_STORE (priv->season_store), &first,
                0, new_str, 1, cnt + 1, -1);
            g_free (new_str);
        } else {
            gtk_list_store_set (GTK_LIST_STORE (priv->season_store), &first,
                1, cnt + 1, -1);
        }
    }

    insert_iter (GTK_LIST_STORE (priv->title_store), &iter, entry,
        (ShowsCompareFunc) show_entry_cmp, 0, TRUE);

    gtk_list_store_set (GTK_LIST_STORE (priv->title_store), &iter,
        0, entry, 1, tv, -1);

    g_signal_connect (G_OBJECT (entry), "entry-changed",
        G_CALLBACK (entry_changed), SHOWS (self));
}

static void
shows_remove_entry (MediaStore *self, Entry *entry)
{
    gmediadb_remove_entry (SHOWS (self)->priv->db, entry_get_id (entry));
}

static void
shows_deinsert_entry (Shows *self, Entry *entry)
{
    ShowsPrivate *priv = self->priv;

    GtkTreeIter first, iter;
    gint cnt;
    gchar *new_str;

    const gchar *show = entry_get_tag_str (entry, "show");
    const gchar *season = entry_get_tag_str (entry, "season");

    gboolean visible = !g_strcmp0 (priv->s_show, show);

    insert_iter (GTK_LIST_STORE (priv->show_store), &iter, (gpointer) show,
                 (ShowsCompareFunc) g_strcmp0, 1, FALSE);

    gtk_tree_model_get (priv->show_store, &iter, 1, &cnt, -1);

    if (cnt == 1) {
        gtk_list_store_remove (GTK_LIST_STORE (priv->show_store), &iter);

        gtk_tree_model_get_iter_first (priv->show_store, &first);
        gtk_tree_model_get (priv->show_store, &first, 1, &cnt, -1);

        new_str = g_strdup_printf ("All %d Shows", --priv->num_shows);
        gtk_list_store_set (GTK_LIST_STORE (priv->show_store), &first,
            0, new_str, 1, cnt - 1, -1);
        g_free (new_str);
    } else {
        gtk_list_store_set (GTK_LIST_STORE (priv->show_store), &iter, 1, cnt - 1, -1);

        gtk_tree_model_get_iter_first (priv->show_store, &first);
        gtk_tree_model_get (priv->show_store, &first, 1, &cnt, -1);
        gtk_list_store_set (GTK_LIST_STORE (priv->show_store), &first, 1, cnt - 1, -1);
    }

    if (visible) {
        insert_iter (GTK_LIST_STORE (priv->season_store), &iter, (gpointer) season,
                 (ShowsCompareFunc) g_strcmp0, 1, FALSE);

        gtk_tree_model_get (priv->season_store, &iter, 1, &cnt, -1);

        if (cnt == 1) {
            gtk_list_store_remove (GTK_LIST_STORE (priv->season_store), &iter);

            gtk_tree_model_get_iter_first (priv->season_store, &first);
            gtk_tree_model_get (priv->season_store, &first, 1, &cnt, -1);

            new_str = g_strdup_printf ("All %d Season", --priv->num_seasons);
            gtk_list_store_set (GTK_LIST_STORE (priv->season_store), &first,
                0, new_str, 1, cnt - 1, -1);
            g_free (new_str);
        } else {
            gtk_list_store_set (GTK_LIST_STORE (priv->season_store), &iter, 1, cnt - 1, -1);

            gtk_tree_model_get_iter_first (priv->season_store, &first);
            gtk_tree_model_get (priv->season_store, &first, 1, &cnt, -1);
            gtk_list_store_set (GTK_LIST_STORE (priv->season_store), &first,
                1, cnt - 1, -1);
        }
    }

    insert_iter (GTK_LIST_STORE (priv->title_store), &iter, entry,
        (ShowsCompareFunc) show_entry_cmp, 0, FALSE);

    gtk_list_store_remove (GTK_LIST_STORE (priv->title_store), &iter);

    g_object_unref (entry);
}

static guint
shows_get_mtype (MediaStore *self)
{
    return MEDIA_TVSHOW;
}

static Entry*
shows_get_next (TrackSource *self)
{
    ShowsPrivate *priv = SHOWS (self)->priv;
    GtkTreeIter iter;
    Entry *entry;

    gtk_tree_model_get_iter_first (priv->title_filter, &iter);
    gtk_tree_model_get (priv->title_filter, &iter, 0, &entry, -1);

    if (!priv->s_entry) {
        priv->s_entry = entry;
        return entry;
    }

    while (entry != priv->s_entry) {
        if (!gtk_tree_model_iter_next (priv->title_filter, &iter))
            break;
        gtk_tree_model_get (priv->title_filter, &iter, 0, &entry, -1);
    }

    if (entry == priv->s_entry) {
        if (!gtk_tree_model_iter_next (priv->title_filter, &iter)) {
            priv->s_entry = NULL;
            return NULL;
        }

        gtk_tree_model_get (priv->title_filter, &iter, 0, &entry, -1);
        priv->s_entry = entry;
        return entry;
    }

    gtk_tree_model_get_iter_first (priv->title_filter, &iter);
    gtk_tree_model_get (priv->title_filter, &iter, 0, &entry, -1);
    priv->s_entry = entry;
    return entry;
}

static Entry*
shows_get_prev (TrackSource *self)
{
    ShowsPrivate *priv = SHOWS (self)->priv;
    GtkTreeIter iter;
    Entry *ep = NULL, *en = NULL;

    if (!priv->s_entry) {
        return NULL;
    }

    gtk_tree_model_get_iter_first (priv->title_filter, &iter);
    gtk_tree_model_get (priv->title_filter, &iter, 0, &en, -1);

    while (en != priv->s_entry) {
        if (!gtk_tree_model_iter_next (priv->title_filter, &iter))
            return NULL;
        ep = en;
        gtk_tree_model_get (priv->title_filter, &iter, 0, &en, -1);
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
entry_changed (Entry *entry, Shows *self)
{
    GtkTreeIter iter;
    Entry *e;

    gtk_tree_model_get_iter_first (self->priv->title_filter, &iter);

    do {
        gtk_tree_model_get (self->priv->title_filter, &iter, 0, &e, -1);
        if (e == entry) {
            GtkTreePath *path = gtk_tree_model_get_path (self->priv->title_filter, &iter);
            gtk_tree_model_row_changed (self->priv->title_filter, path, &iter);
            gtk_tree_path_free (path);
            return;
        }
    } while (gtk_tree_model_iter_next (self->priv->title_filter, &iter));
}

static gpointer
initial_import (Shows *self)
{
    gchar *tags[] = { "id", "show", "season", "title", "duration", "track", "location", NULL };
    GPtrArray *entries = gmediadb_get_all_entries (self->priv->db, tags);

    int i;
    for (i = 0; i < entries->len; i++) {
        gchar **entry = g_ptr_array_index (entries, i);
        Entry *e = entry_new (entry[0] ? atoi (entry[0]) : 0);
        entry_set_tag_str (e, "show", entry[1] ? entry[1] : "");
        entry_set_tag_str (e, "season", entry[2] ? entry[2] : "");
        entry_set_tag_str (e, "title", entry[3] ? entry[3] : "");

        entry_set_tag_int (e, "duration", entry[4] ? atoi (entry[4]) : 0);
        entry_set_tag_int (e, "track", entry[5] ? atoi (entry[5]) : 0);

        entry_set_media_type (e, MEDIA_TVSHOW);
        entry_set_location (e, entry[6]);

        shows_insert_entry (self, e);

        g_object_unref (G_OBJECT (e));
    }

    g_ptr_array_free (entries, TRUE);
}

static void
show_cursor_changed (GtkTreeView *view, Shows *self)
{
    GtkTreePath *path;
    GtkTreeIter iter, siter;
    gchar *show, *path_str = NULL;
    const gchar *season = NULL;
    Entry *e;
    gint seasons, tracks, cnt;

    gtk_tree_view_get_cursor (GTK_TREE_VIEW (self->priv->show), &path, NULL);

    if (path) {
        path_str = gtk_tree_path_to_string (path);
    }

    if (path == NULL || !g_strcmp0 (path_str, "0")) {
        if (self->priv->s_show) {
            g_free (self->priv->s_show);
            self->priv->s_show = NULL;

            // Clear view
            gtk_tree_model_get_iter_first (self->priv->season_store, &iter);
            while (gtk_list_store_remove (GTK_LIST_STORE (self->priv->season_store), &iter));

            gtk_list_store_insert_with_values (GTK_LIST_STORE (self->priv->season_store),
                NULL, 0, 0, "Select a show...", 1, 0, -1);

            gtk_tree_model_get_iter_first (self->priv->title_store, &iter);
            do {
                gtk_list_store_set (GTK_LIST_STORE (self->priv->title_store),
                    &iter, 1, TRUE, -1);
            } while (gtk_tree_model_iter_next (self->priv->title_store, &iter));
        }
    } else {
        gtk_tree_model_get_iter (self->priv->show_store, &iter, path);
        gtk_tree_model_get (self->priv->show_store, &iter, 0, &show, -1);

        if (g_strcmp0 (self->priv->s_show, show)) {
            if (self->priv->s_show) {
                g_free (self->priv->s_show);
            }

            self->priv->s_show = g_strdup (show);

            // Clear view
            gtk_tree_model_get_iter_first (self->priv->season_store, &iter);
            while (gtk_list_store_remove (GTK_LIST_STORE (self->priv->season_store), &iter));

            gtk_tree_model_get_iter_first (self->priv->title_store, &iter);
            gtk_list_store_append (GTK_LIST_STORE (self->priv->season_store), &siter);
            cnt = 0;
            seasons = 0;
            gboolean started = FALSE;
            do {
                gtk_tree_model_get (self->priv->title_store, &iter, 0, &e, -1);
                if (!g_strcmp0 (self->priv->s_show, entry_get_tag_str (e, "show"))) {
                    if (season) {
                        if (!g_strcmp0 (season, entry_get_tag_str (e, "season"))) {
                            tracks++;
                            cnt++;
                        } else {
                            gtk_list_store_set (GTK_LIST_STORE (self->priv->season_store),
                                &siter, 0, season, 1, tracks, -1);
                            seasons++;
                            gtk_list_store_append (GTK_LIST_STORE (self->priv->season_store),
                                &siter);

                            tracks = 1;
                            cnt++;

                            season = entry_get_tag_str (e, "season");
                        }
                    } else {
                        tracks = 1;
                        cnt++;

                        season = entry_get_tag_str (e, "season");
                    }

                    gtk_list_store_set (GTK_LIST_STORE (self->priv->title_store),
                        &iter, 1, TRUE, -1);
                } else {
                    gtk_list_store_set (GTK_LIST_STORE (self->priv->title_store),
                        &iter, 1, FALSE, -1);
                }
            } while (gtk_tree_model_iter_next (self->priv->title_store, &iter));

            gtk_list_store_set (GTK_LIST_STORE (self->priv->season_store),
                &siter, 0, season, 1, tracks, -1);
            seasons++;

            gchar *str = g_strdup_printf ("All %d seasons", seasons);
            gtk_list_store_insert_with_values (GTK_LIST_STORE (self->priv->season_store),
                NULL, 0, 0, str, 1, cnt, -1);
            g_free (str);
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
season_cursor_changed (GtkTreeView *view, Shows *self)
{
    GtkTreePath *path;
    GtkTreeIter iter;
    gchar *season, *path_str = NULL;
    Entry *e;

    gtk_tree_view_get_cursor (GTK_TREE_VIEW (self->priv->season), &path, NULL);

    if (path)
        path_str = gtk_tree_path_to_string (path);

    if (path == NULL || !g_strcmp0 (path_str, "0")) {
        if (self->priv->s_season) {
            g_free (self->priv->s_season);
            self->priv->s_season = NULL;

            gtk_tree_model_get_iter_first (self->priv->title_store, &iter);
            do {
                gtk_tree_model_get (self->priv->title_store, &iter, 0, &e, -1);

                gboolean visible = self->priv->s_show == NULL ||
                    !g_strcmp0 (self->priv->s_show, entry_get_tag_str (e, "show"));

                gtk_list_store_set (GTK_LIST_STORE (self->priv->title_store),
                    &iter, 1, visible, -1);
            } while (gtk_tree_model_iter_next (self->priv->title_store, &iter));
        }
    } else {
        gtk_tree_model_get_iter (self->priv->season_store, &iter, path);
        gtk_tree_model_get (self->priv->season_store, &iter, 0, &season, -1);

        if (g_strcmp0 (self->priv->s_season, season)) {
            if (self->priv->s_season) {
                g_free (self->priv->s_season);
            }

            self->priv->s_season = g_strdup (season);

            gtk_tree_model_get_iter_first (self->priv->title_store, &iter);
            do {
                gtk_tree_model_get (self->priv->title_store, &iter, 0, &e, -1);

                gboolean visible = (self->priv->s_show == NULL ||
                                   !g_strcmp0 (self->priv->s_show, entry_get_tag_str (e, "show"))) &&
                                   (self->priv->s_season == NULL ||
                                   !g_strcmp0 (self->priv->s_season, entry_get_tag_str (e, "season")));

                gtk_list_store_set (GTK_LIST_STORE (self->priv->title_store),
                    &iter, 1, visible, -1);
            } while (gtk_tree_model_iter_next (self->priv->title_store, &iter));
        }

        g_free (season);
    }

    if (path_str) {
        g_free (path_str);
    }

    if (path) {
        gtk_tree_path_free (path);
    }
}

static void
show_row_activated (GtkTreeView *view,
                      GtkTreePath *path,
                      GtkTreeViewColumn *column,
                      Shows *self)
{
    Entry *entry = shows_get_next (TRACK_SOURCE (self));
    track_source_emit_play (TRACK_SOURCE (self), entry);
}


static void
season_row_activated (GtkTreeView *view,
                     GtkTreePath *path,
                     GtkTreeViewColumn *column,
                     Shows *self)
{
    Entry *entry = shows_get_next (TRACK_SOURCE (self));
    track_source_emit_play (TRACK_SOURCE (self), entry);
}

static void
title_row_activated (GtkTreeView *view,
                     GtkTreePath *path,
                     GtkTreeViewColumn *column,
                     Shows *self)
{
    GtkTreeIter iter;
    Entry *entry;

    gtk_tree_model_get_iter (self->priv->title_filter, &iter, path);
    gtk_tree_model_get (self->priv->title_filter, &iter, 0, &entry, -1);

    self->priv->s_entry = entry;
    track_source_emit_play (TRACK_SOURCE (self), entry);
}

static gboolean
insert_iter (GtkListStore *store,
             GtkTreeIter *iter,
             gpointer ne,
             ShowsCompareFunc cmp,
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

static void
shows_gmediadb_add (GMediaDB *db, guint id, Shows *self)
{
    gchar *tags[] = { "id", "show", "season", "title", "duration", "track", "location", NULL };
    gchar **entry = gmediadb_get_entry (self->priv->db, id, tags);

    Entry *e = entry_new (entry[0] ? atoi (entry[0]) : 0);
    entry_set_tag_str (e, "show", entry[1] ? entry[1] : "");
    entry_set_tag_str (e, "season", entry[2] ? entry[2] : "");
    entry_set_tag_str (e, "title", entry[3] ? entry[3] : "");

    entry_set_tag_int (e, "duration", entry[4] ? atoi (entry[4]) : 0);
    entry_set_tag_int (e, "track", entry[5] ? atoi (entry[5]) : 0);

    entry_set_media_type (e, MEDIA_TVSHOW);
    entry_set_location (e, entry[6]);

    gdk_threads_enter ();
    shows_insert_entry (self, e);
    gdk_threads_leave ();

    g_object_unref (G_OBJECT (e));
}

static void
shows_gmediadb_remove (GMediaDB *db, guint id, Shows *self)
{
    Entry *entry;
    GtkTreeIter iter;

    gtk_tree_model_iter_children (self->priv->title_store, &iter, NULL);

    do {
        gtk_tree_model_get (self->priv->title_store, &iter, 0, &entry, -1);

        if (entry_get_id (entry) == id) {
            gdk_threads_enter ();
            shows_deinsert_entry (self, entry);
            gdk_threads_leave ();
            break;
        }
    } while (gtk_tree_model_iter_next (self->priv->title_store, &iter));
}

static void
shows_gmediadb_update (GMediaDB *db, guint id, Shows *self)
{
    Entry *oe;

    gchar *tags[] = { "id", "show", "season", "title", "duration", "track", "location", NULL };
    gchar **entry = gmediadb_get_entry (self->priv->db, id, tags);

    Entry *e = entry_new (entry[0] ? atoi (entry[0]) : 0);
    entry_set_tag_str (e, "show", entry[1] ? entry[1] : "");
    entry_set_tag_str (e, "season", entry[2] ? entry[2] : "");
    entry_set_tag_str (e, "title", entry[3] ? entry[3] : "");

    entry_set_tag_int (e, "duration", entry[4] ? atoi (entry[4]) : 0);
    entry_set_tag_int (e, "track", entry[5] ? atoi (entry[5]) : 0);

    entry_set_media_type (e, MEDIA_TVSHOW);
    entry_set_location (e, entry[6]);

    GtkTreeIter iter;
    gtk_tree_model_get_iter_first (self->priv->title_store, &iter);
    do {
        gtk_tree_model_get (self->priv->title_store, &iter, 0, &oe, -1);
        if (entry_get_id (oe) == id) {
            break;
        }
        oe = NULL;
    } while (gtk_tree_model_iter_next (self->priv->title_store, &iter));

    if (oe) {
        shows_deinsert_entry (self, oe);
    }

    shows_insert_entry (self, e);
}

static gboolean
on_show_click (GtkWidget *view, GdkEventButton *event, Shows *self)
{
    if (event->button == 3) {
        g_print ("RBUTTON ARTIST\n");

        return TRUE;
    }
    return FALSE;
}

static gboolean
on_season_click (GtkWidget *view, GdkEventButton *event, Shows *self)
{
    if (event->button == 3) {
        g_print ("RBUTTON ALBUM\n");
        return TRUE;
    }
    return FALSE;
}

static gboolean
on_title_click (GtkWidget *view, GdkEventButton *event, Shows *self)
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

        GtkWidget *item = gtk_image_menu_item_new_from_stock (GTK_STOCK_INFO, NULL);
        gtk_menu_item_set_label (GTK_MENU_ITEM (item), "Get Info");
        gtk_menu_append (GTK_MENU (menu), item);
        g_signal_connect (item, "activate", G_CALLBACK (on_title_info), self);

        gtk_menu_append (GTK_MENU (menu), gtk_separator_menu_item_new ());

        item = gtk_image_menu_item_new_from_stock (GTK_STOCK_REMOVE, NULL);
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
on_title_remove (GtkWidget *item, Shows *self)
{
    Entry **entries;
    GtkTreeIter iter;
    guint size, i;

    GList *rows = gtk_tree_selection_get_selected_rows (self->priv->title_sel, NULL);
    size = g_list_length (rows);

    entries = g_new0 (Entry*, size);
    for (i = 0; i < size; i++) {
        gtk_tree_model_get_iter (self->priv->title_filter, &iter, g_list_nth_data (rows, i));
        gtk_tree_model_get (self->priv->title_filter, &iter, 0, &entries[i], -1);
    }

    g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (rows);

    for (i = 0; i < size; i++) {
        gdk_threads_leave ();
        media_store_remove_entry (MEDIA_STORE (self), entries[i]);
        gdk_threads_enter ();
    }

    g_free (entries);
}

static void
on_title_info (GtkWidget *item, Shows *self)
{
    Entry *e;
    guint size, i, j;
    gchar **keys, **vals;
    GtkTreeIter iter;

    TagDialog *td = tag_dialog_new ();
    g_signal_connect_swapped (td, "completed", G_CALLBACK (on_info_completed), self);

    GList *rows = gtk_tree_selection_get_selected_rows (self->priv->title_sel, NULL);
    size = g_list_length (rows);

    if (size <= 0) {
        return;
    }

    for (i = 0; i < size; i++) {
        gtk_tree_model_get_iter (self->priv->title_filter, &iter, g_list_nth_data (rows, i));
        gtk_tree_model_get (self->priv->title_filter, &iter, 0, &e, -1);

        gchar **kvs = gmediadb_get_entry (self->priv->db, entry_get_id (e), NULL);

        tag_dialog_add_entry (td, kvs);

        g_free (kvs);
    }

    g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (rows);

    tag_dialog_show (td);
}

static void
on_info_completed (Shows *self, GPtrArray *changes, TagDialog *td)
{
    g_print ("on_td_completed: ");
    if (changes && changes->len > 0) {
        gint i;
        g_print ("%d changes\n", changes->len / 2);
        for (i = 0; i < changes->len; i += 2) {
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

    GList *rows = gtk_tree_selection_get_selected_rows (self->priv->title_sel, NULL);
    size = g_list_length (rows);

    entries = g_new0 (Entry*, size);
    for (i = 0; i < size; i++) {
        gtk_tree_model_get_iter (self->priv->title_filter, &iter, g_list_nth_data (rows, i));
        gtk_tree_model_get (self->priv->title_filter, &iter, 0, &entries[i], -1);
    }

    g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (rows);

    keys = g_new0 (gchar*, changes->len / 2 + 1);
    vals = g_new0 (gchar*, changes->len / 2 + 1);

    for (i = 0; i < changes->len / 2; i++) {
        keys[i] = changes->pdata[2*i];
        vals[i] = changes->pdata[2*i+1];
    }

    for (i = 0; i < size; i++) {
        gmediadb_update_entry (self->priv->db, entry_get_id (entries[i]), keys, vals);
    }

    g_strfreev (keys);
    g_strfreev (vals);

    g_free (entries);
}

static gint
show_entry_cmp (Entry *e1, Entry *e2)
{
    gint res;
    if (entry_get_id (e1) == entry_get_id (e2))
        return 0;

    res = g_strcmp0 (entry_get_tag_str (e1, "show"), entry_get_tag_str (e2, "show"));
    if (res != 0)
        return res;

    res = g_strcmp0 (entry_get_tag_str (e1, "season"), entry_get_tag_str (e2, "season"));
    if (res != 0)
        return res;

    if (entry_get_tag_int (e2, "track") != entry_get_tag_int (e1, "track"))
        return entry_get_tag_int (e1, "track") - entry_get_tag_int (e2, "track");

    res = g_strcmp0 (entry_get_tag_str (e1, "title"), entry_get_tag_str (e2, "title"));
    if (res != 0)
        return res;

    return -1;
}
