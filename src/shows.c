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

static gpointer initial_import (Shows *self);
static gboolean insert_iter (GtkListStore *store, GtkTreeIter *iter,
    gpointer ne, ShowsCompareFunc cmp, gint l, gboolean create);
static void entry_changed (Entry *entry, Shows *self);

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
        NULL, 0, 0, "All 0 Seasons", 1, 0, -1);

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

    const gchar *artist = entry_get_tag_str (entry, "artist");
    const gchar *album = entry_get_tag_str (entry, "album");

    gboolean visible = (priv->s_show == NULL ||
                       !g_strcmp0 (priv->s_show, artist)) &&
                       (priv->s_season == NULL ||
                       !g_strcmp0 (priv->s_season, album));

    // Add artist to view
    res = insert_iter (GTK_LIST_STORE (priv->show_store), &iter,
        (gpointer) artist, (ShowsCompareFunc) g_strcmp0, 1, FALSE);

    gtk_tree_model_get (priv->show_store, &iter, 1, &cnt, -1);
    gtk_list_store_set (GTK_LIST_STORE (priv->show_store), &iter,
        0, artist, 1, cnt + 1, -1);

    gtk_tree_model_get_iter_first (priv->show_store, &first);
    gtk_tree_model_get (priv->show_store, &first, 1, &cnt, -1);

    if (res) {
        new_str = g_strdup_printf ("All %d Artists", ++priv->num_shows);
        gtk_list_store_set (GTK_LIST_STORE (priv->show_store), &first,
            0, new_str, 1, cnt + 1, -1);
        g_free (new_str);
    } else {
        gtk_list_store_set (GTK_LIST_STORE (priv->show_store), &first,
            1, cnt + 1, -1);
    }

    // Add album to view
    res = insert_iter (GTK_LIST_STORE (priv->season_store), &iter,
        (gpointer) album, (ShowsCompareFunc) g_strcmp0, 1, FALSE);

    gtk_tree_model_get (priv->season_store, &iter, 1, &cnt, -1);
    gtk_list_store_set (GTK_LIST_STORE (priv->season_store), &iter,
        0, album, 1, cnt + 1, 2, visible, 3, artist, -1);

    if (visible) {
        gtk_tree_model_get_iter_first (priv->season_store, &first);
        gtk_tree_model_get (priv->season_store, &first, 1, &cnt, -1);

        if (res) {
            new_str = g_strdup_printf ("All %d Albums", ++priv->num_seasons);
            gtk_list_store_set (GTK_LIST_STORE (priv->season_store), &first,
                0, new_str, 1, cnt + 1, -1);
            g_free (new_str);
        } else {
            gtk_list_store_set (GTK_LIST_STORE (priv->season_store), &first,
                1, cnt + 1, -1);
        }
    }

    insert_iter (GTK_LIST_STORE (priv->title_store), &iter, entry,
        (ShowsCompareFunc) entry_cmp, 0, TRUE);

    gtk_list_store_set (GTK_LIST_STORE (priv->title_store), &iter,
        0, entry, 1, visible, -1);

    g_signal_connect (G_OBJECT (entry), "entry-changed",
        G_CALLBACK (entry_changed), SHOWS (self));
}

static void
shows_remove_entry (MediaStore *self, Entry *entry)
{
    ShowsPrivate *priv = SHOWS (self)->priv;
    g_print ("SHOWS REMOVE ENTRY: %s\n", entry_get_location (entry));

    gmediadb_remove_entry (priv->db, entry_get_id (entry));
}

static void
shows_deinsert_entry (Shows *self, Entry *entry)
{
    ShowsPrivate *priv = self->priv;

    GtkTreeIter first, iter;
    gint cnt;
    gchar *new_str;

    const gchar *artist = entry_get_tag_str (entry, "artist");
    const gchar *album = entry_get_tag_str (entry, "album");

    gboolean visible = (priv->s_show == NULL ||
                       !g_strcmp0 (priv->s_show, artist));

    insert_iter (GTK_LIST_STORE (priv->show_store), &iter, (gpointer) artist,
                 (ShowsCompareFunc) g_strcmp0, 1, FALSE);

    gtk_tree_model_get (priv->show_store, &iter, 1, &cnt, -1);

    if (cnt == 1) {
        gtk_list_store_remove (GTK_LIST_STORE (priv->show_store), &iter);

        gtk_tree_model_get_iter_first (priv->show_store, &first);
        gtk_tree_model_get (priv->show_store, &first, 1, &cnt, -1);

        new_str = g_strdup_printf ("All %d Artists", --priv->num_shows);
        gtk_list_store_set (GTK_LIST_STORE (priv->show_store), &first,
            0, new_str, 1, cnt - 1, -1);
        g_free (new_str);
    } else {
        gtk_list_store_set (GTK_LIST_STORE (priv->show_store), &iter, 1, cnt - 1, -1);
    }

    insert_iter (GTK_LIST_STORE (priv->season_store), &iter, (gpointer) album,
                 (ShowsCompareFunc) g_strcmp0, 1, FALSE);

    gtk_tree_model_get (priv->season_store, &iter, 1, &cnt, -1);

    if (cnt == 1) {
        gtk_list_store_remove (GTK_LIST_STORE (priv->season_store), &iter);

        if (visible) {
            gtk_tree_model_get_iter_first (priv->season_store, &first);
            gtk_tree_model_get (priv->season_store, &first, 1, &cnt, -1);

            new_str = g_strdup_printf ("All %d Albums", --priv->num_seasons);
            gtk_list_store_set (GTK_LIST_STORE (priv->season_store), &first,
                0, new_str, 1, cnt - 1, -1);
            g_free (new_str);
        }
    } else {
        gtk_list_store_set (GTK_LIST_STORE (priv->season_store), &iter, 1, cnt - 1, -1);
    }

    insert_iter (GTK_LIST_STORE (priv->title_store), &iter, entry,
        (ShowsCompareFunc) entry_cmp, 0, FALSE);

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
    gchar *tags[] = { "id", "artist", "album", "title", "duration", "track", "location", NULL };
    GPtrArray *entries = gmediadb_get_all_entries (self->priv->db, tags);

    int i;
    for (i = 0; i < entries->len; i++) {
        gchar **entry = g_ptr_array_index (entries, i);
        Entry *e = entry_new (entry[0] ? atoi (entry[0]) : 0);
        entry_set_tag_str (e, "artist", entry[1] ? entry[1] : "");
        entry_set_tag_str (e, "album", entry[2] ? entry[2] : "");
        entry_set_tag_str (e, "title", entry[3] ? entry[3] : "");

        entry_set_tag_int (e, "duration", entry[4] ? atoi (entry[4]) : 0);
        entry_set_tag_int (e, "track", entry[5] ? atoi (entry[5]) : 0);

        entry_set_media_type (e, MEDIA_SONG);
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
    GtkTreeIter iter;
    gchar *art, *artist, *path_str = NULL;
    gboolean changed = FALSE;

    gint albums = 0, tracks = 0, cnt;

    gtk_tree_view_get_cursor (GTK_TREE_VIEW (self->priv->show), &path, NULL);

    if (path) {
        path_str = gtk_tree_path_to_string (path);
    }

    if (path == NULL || !g_strcmp0 (path_str, "0")) {
        if (self->priv->s_show) {
            changed = TRUE;
            g_free (self->priv->s_show);
            self->priv->s_show = NULL;

            gtk_tree_model_get_iter_first (self->priv->season_store, &iter);
            while (gtk_tree_model_iter_next (self->priv->season_store, &iter)) {
                gtk_tree_model_get (self->priv->season_store, &iter, 1, &cnt, -1);

                albums++;
                tracks += cnt;

                gtk_list_store_set (GTK_LIST_STORE (self->priv->season_store),
                    &iter, 2, TRUE, -1);
            }

            gchar *new_str = g_strdup_printf ("All %d Albums", albums);
            gtk_tree_model_get_iter_first (self->priv->season_store, &iter);
            gtk_list_store_set (GTK_LIST_STORE (self->priv->season_store), &iter,
                0, new_str, 1, tracks, -1);
            g_free (new_str);
        }
    } else {
        gtk_tree_model_get_iter (self->priv->show_store, &iter, path);
        gtk_tree_model_get (self->priv->show_store, &iter, 0, &art, -1);

        if (g_strcmp0 (self->priv->s_show, art)) {
            changed = TRUE;
            if (self->priv->s_show)
                g_free (self->priv->s_show);
            self->priv->s_show = g_strdup (art);

            gtk_tree_model_get_iter_first (self->priv->season_store, &iter);
            while (gtk_tree_model_iter_next (self->priv->season_store, &iter)) {
                gtk_tree_model_get (self->priv->season_store, &iter,
                    1, &cnt, 3, &artist, -1);

                gboolean visible = !g_strcmp0 (self->priv->s_show, artist);
                if (visible) {
                    albums++;
                    tracks += cnt;
                }

                gtk_list_store_set (GTK_LIST_STORE (self->priv->season_store),
                    &iter, 2, !g_strcmp0 (self->priv->s_show, artist), -1);
                g_free (artist);
            }

            gchar *new_str = g_strdup_printf ("All %d Albums", albums);
            gtk_tree_model_get_iter_first (self->priv->season_store, &iter);
            gtk_list_store_set (GTK_LIST_STORE (self->priv->season_store), &iter,
                0, new_str, 1, tracks, -1);
            g_free (new_str);
        }

        g_free (art);
    }

    if (path_str)
        g_free (path_str);

    if (path)
        gtk_tree_path_free (path);

    if (changed) {
        season_cursor_changed (view, self);
    }
}

static void
season_cursor_changed (GtkTreeView *view, Shows *self)
{
    GtkTreePath *path;
    GtkTreeIter iter;
    gchar *alb, *path_str = NULL;
    Entry *e;

    gtk_tree_view_get_cursor (GTK_TREE_VIEW (self->priv->season), &path, NULL);

    if (path)
        path_str = gtk_tree_path_to_string (path);

    if (path == NULL || !g_strcmp0 (path_str, "0")) {
        if (self->priv->s_season || GTK_WIDGET (view) == self->priv->show) {
            g_free (self->priv->s_season);
            self->priv->s_season = NULL;

            gtk_tree_model_get_iter_first (self->priv->title_store, &iter);
            do {
                gtk_tree_model_get (self->priv->title_store, &iter, 0, &e, -1);

                gboolean visible = self->priv->s_show == NULL ||
                                   !g_strcmp0 (self->priv->s_show, entry_get_tag_str (e, "artist"));

                gtk_list_store_set (GTK_LIST_STORE (self->priv->title_store),
                    &iter, 1, visible, -1);
            } while (gtk_tree_model_iter_next (self->priv->title_store, &iter));
        }
    } else {
        gtk_tree_model_get_iter (self->priv->season_store, &iter, path);
        gtk_tree_model_get (self->priv->season_store, &iter, 0, &alb, -1);

        if (g_strcmp0 (self->priv->s_season, alb) || GTK_WIDGET (view) == self->priv->show) {
            if (self->priv->s_season)
                g_free (self->priv->s_season);
            self->priv->s_season = g_strdup (alb);

            gtk_tree_model_get_iter_first (self->priv->title_store, &iter);
            do {
                gtk_tree_model_get (self->priv->title_store, &iter, 0, &e, -1);

                gboolean visible = (self->priv->s_show == NULL ||
                                   !g_strcmp0 (self->priv->s_show, entry_get_tag_str (e, "artist"))) &&
                                   (self->priv->s_season == NULL ||
                                   !g_strcmp0 (self->priv->s_season, entry_get_tag_str (e, "album")));

                gtk_list_store_set (GTK_LIST_STORE (self->priv->title_store),
                    &iter, 1, visible, -1);
            } while (gtk_tree_model_iter_next (self->priv->title_store, &iter));
        }

        g_free (alb);
    }

    if (path_str)
        g_free (path_str);

    if (path)
        gtk_tree_path_free (path);
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
    gchar *tags[] = { "id", "artist", "album", "title", "duration", "track", "location", NULL };
    gchar **entry = gmediadb_get_entry (self->priv->db, id, tags);

    Entry *e = entry_new (entry[0] ? atoi (entry[0]) : 0);
    entry_set_tag_str (e, "artist", entry[1] ? entry[1] : "");
    entry_set_tag_str (e, "album", entry[2] ? entry[2] : "");
    entry_set_tag_str (e, "title", entry[3] ? entry[3] : "");

    entry_set_tag_int (e, "duration", entry[4] ? atoi (entry[4]) : 0);
    entry_set_tag_int (e, "track", entry[5] ? atoi (entry[5]) : 0);

    entry_set_media_type (e, MEDIA_SONG);
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
on_title_remove (GtkWidget *item, Shows *self)
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
