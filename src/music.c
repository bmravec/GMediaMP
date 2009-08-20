/*
 *      music.c
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
#include "music.h"

G_DEFINE_TYPE(Music, music, G_TYPE_OBJECT)

struct _MusicPrivate {
    GtkBuilder *builder;
    Shell *shell;
    GMediaDB *db;

    GtkWidget *widget;
    GtkWidget *artist;
    GtkWidget *album;
    GtkWidget *title;

    GtkTreeModel *artist_store;
    GtkTreeModel *album_store;
    GtkTreeModel *title_store;
    GtkTreeModel *album_filter;
    GtkTreeModel *title_filter;

    gchar *sel_artist;
    gchar *sel_album;

    gint num_artists, num_albums;
};

static guint signal_add;
static guint signal_replace;

typedef gint (*MusicCompareFunc) (gpointer a, gpointer b);

static void str_column_func (GtkTreeViewColumn *column, GtkCellRenderer *cell,
    GtkTreeModel *model, GtkTreeIter *iter, gchar *data);
static void int_column_func (GtkTreeViewColumn *column, GtkCellRenderer *cell,
    GtkTreeModel *model, GtkTreeIter *iter, gchar *data);
static gpointer initial_import (Music *self);

static void artist_cursor_changed (GtkTreeView *view, Music *self);
static void album_cursor_changed (GtkTreeView *view, Music *self);

static void artist_row_activated (GtkTreeView *view, GtkTreePath *path,
    GtkTreeViewColumn *column, Music *self);
static void album_row_activated (GtkTreeView *view, GtkTreePath *path,
    GtkTreeViewColumn *column, Music *self);
static void title_row_activated (GtkTreeView *view, GtkTreePath *path,
    GtkTreeViewColumn *column, Music *self);

static gboolean insert_iter (GtkListStore *store, GtkTreeIter *iter,
    gpointer ne, MusicCompareFunc cmp, gint l, gboolean create);

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
artist_select (GtkWidget *widget,
               gchar *artist,
               Music *self)
{
    g_print ("Artist Select: (%s)\n", artist);
//    album_set_filter (ALBUM (self->priv->album), artist);
//    title_set_filter (TITLE (self->priv->title), artist, "");
}

static void
artist_replace (GtkWidget *widget,
                gchar *artist,
                Music *self)
{
    g_print ("Artist replace: (%s)\n", artist);
}

static void
album_select (GtkWidget *widget,
              gchar *album,
              Music *self)
{
    g_print ("Album Select: (%s)\n", album);
//    title_set_filter (TITLE (self->priv->title), NULL, album);
}

static void
album_replace (GtkWidget *widget,
               gchar *album,
               Music *self)
{
    g_print ("Album replace: (%s)\n", album);
}

static void
title_replace (GtkWidget *widget,
               Entry *entry,
               Music *self)
{
    g_signal_emit (G_OBJECT (self), signal_replace, 0, entry);
}

static void
music_finalize (GObject *object)
{
    Music *self = MUSIC (object);

    G_OBJECT_CLASS (music_parent_class)->finalize (object);
}

static void
music_class_init (MusicClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (MusicPrivate));

    object_class->finalize = music_finalize;

    signal_add = g_signal_new ("entry-add", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1, G_TYPE_POINTER);

    signal_replace = g_signal_new ("entry-replace", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void
music_init (Music *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), MUSIC_TYPE, MusicPrivate);

    self->priv->num_artists = 0;
    self->priv->num_albums = 0;
}

Music*
music_new ()
{
    return g_object_new (MUSIC_TYPE, NULL);
}

gboolean
music_activate (Music *self)
{
    self->priv->shell = shell_new ();
    self->priv->builder = shell_get_builder (self->priv->shell);
    self->priv->db = gmediadb_new ("Music");

    GError *err = NULL;
//    gtk_builder_add_from_file (self->priv->builder, SHARE_DIR "ui/music.ui", NULL);
    gtk_builder_add_from_file (self->priv->builder, "data/ui/music.ui", &err);

    if (err) {
        g_print ("ERROR ADDING: %s", err->message);
        g_error_free (err);
    }

    self->priv->widget = GTK_WIDGET (
        gtk_builder_get_object (self->priv->builder, "music_vpane"));
    self->priv->artist = GTK_WIDGET (
        gtk_builder_get_object (self->priv->builder, "music_artist"));
    self->priv->album = GTK_WIDGET (
        gtk_builder_get_object (self->priv->builder, "music_album"));
    self->priv->title = GTK_WIDGET (
        gtk_builder_get_object (self->priv->builder, "music_title"));

    self->priv->artist_store = GTK_TREE_MODEL (
        gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_UINT));
    self->priv->album_store = GTK_TREE_MODEL (
        gtk_list_store_new (4, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_BOOLEAN, G_TYPE_STRING));
    self->priv->title_store = GTK_TREE_MODEL (
        gtk_list_store_new (2, G_TYPE_OBJECT, G_TYPE_BOOLEAN));
    self->priv->album_filter = gtk_tree_model_filter_new (self->priv->album_store, NULL);
    self->priv->title_filter = gtk_tree_model_filter_new (self->priv->title_store, NULL);

    gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->artist), self->priv->artist_store);
    gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->album), self->priv->album_filter);
    gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->title), self->priv->title_filter);

    self->priv->sel_artist = g_strdup ("");
    self->priv->sel_album = g_strdup ("");

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    // Create Columns for Artist
    renderer = gtk_cell_renderer_text_new ();
    g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
    column = gtk_tree_view_column_new_with_attributes ("Artist", renderer, "text", 0, NULL);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->artist), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("#", renderer, "text", 1, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->artist), column);

    gtk_list_store_insert_with_values (GTK_LIST_STORE (self->priv->artist_store),
        NULL, 0, 0, "All 0 Artists", 1, 0, -1);

    // Create Columns for Album
    renderer = gtk_cell_renderer_text_new ();
    g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
    column = gtk_tree_view_column_new_with_attributes ("Album", renderer, "text", 0, NULL);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->album), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("#", renderer, "text", 1, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->album), column);

    gtk_list_store_insert_with_values (GTK_LIST_STORE (self->priv->album_store),
        NULL, 0, 0, "All 0 Albums", 1, 0, 2, TRUE, 3, NULL, -1);

    // Create Columns for Title
    renderer = gtk_cell_renderer_pixbuf_new ();
    column = gtk_tree_view_column_new_with_attributes ("", renderer, NULL);
//    gtk_tree_view_column_set_cell_data_func (column, renderer,
//        (GtkTreeCellDataFunc) status_column_func, NULL, NULL);
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
    column = gtk_tree_view_column_new_with_attributes ("Artist", renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
        (GtkTreeCellDataFunc) str_column_func, "artist", NULL);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->title), column);

    renderer = gtk_cell_renderer_text_new ();
    g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
    column = gtk_tree_view_column_new_with_attributes ("Album", renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
        (GtkTreeCellDataFunc) str_column_func, "album", NULL);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->title), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Time", renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
        (GtkTreeCellDataFunc) int_column_func, "duration", NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->title), column);

    // Connect Signals
    g_signal_connect (G_OBJECT (self->priv->artist), "cursor-changed", G_CALLBACK (artist_cursor_changed), NULL);
    g_signal_connect (G_OBJECT (self->priv->album), "cursor-changed", G_CALLBACK (album_cursor_changed), NULL);

    g_signal_connect (G_OBJECT (self->priv->artist), "row-activated", G_CALLBACK (artist_row_activated), NULL);
    g_signal_connect (G_OBJECT (self->priv->album), "row-activated", G_CALLBACK (album_row_activated), NULL);
    g_signal_connect (G_OBJECT (self->priv->title), "row-activated", G_CALLBACK (title_row_activated), NULL);

    shell_add_widget (self->priv->shell, gtk_label_new ("Library"), "Library", NULL);
    shell_add_widget (self->priv->shell, self->priv->widget, "Library/Music", NULL);

    gtk_widget_show_all (self->priv->widget);

    g_thread_create ((GThreadFunc) initial_import, self, FALSE, NULL);
}

gboolean
music_deactivate (Music *self)
{

}

void
music_add_entry (Music *self, Entry *entry)
{
//    artist_add_entry (ARTIST (self->priv->artist), entry_get_artist (entry));
    GtkTreeIter first, iter;
    gint cnt;
    gchar *new_str, *str;
    gboolean res;

//    gdk_threads_enter ();

    // Add artist to view
    str = entry_get_tag_str (entry, "artist");
    res = insert_iter (GTK_LIST_STORE (self->priv->artist_store), &iter,
        str, (MusicCompareFunc) g_strcmp0, 1, FALSE);

    gtk_tree_model_get (GTK_TREE_MODEL (self->priv->artist_store), &iter, 1, &cnt, -1);
    gtk_list_store_set (self->priv->artist_store, &iter, 0, str, 1, cnt + 1, -1);

    gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->artist_store), &first);
    gtk_tree_model_get (GTK_TREE_MODEL (self->priv->artist_store), &first, 1, &cnt, -1);

    if (res) {
        new_str = g_strdup_printf ("All %d Artists", ++self->priv->num_artists);
        gtk_list_store_set (self->priv->artist_store, &first, 0, new_str, 1, cnt + 1, -1);
        g_free (new_str);
    } else {
        gtk_list_store_set (self->priv->artist_store, &first, 1, cnt + 1, -1);
    }

    // Add album to view
    str = entry_get_tag_str (entry, "album");
    res = insert_iter (GTK_LIST_STORE (self->priv->album_store), &iter,
        str, (MusicCompareFunc) g_strcmp0, 1, FALSE);

    gtk_tree_model_get (GTK_TREE_MODEL (self->priv->album_store), &iter, 1, &cnt, -1);
    gtk_list_store_set (self->priv->album_store, &iter, 0, str, 1, cnt + 1, -1);

    gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->album_store), &first);
    gtk_tree_model_get (GTK_TREE_MODEL (self->priv->album_store), &first, 1, &cnt, -1);

    if (res) {
        new_str = g_strdup_printf ("All %d Albums", ++self->priv->num_albums);
        gtk_list_store_set (self->priv->album_store, &first, 0, new_str, 1, cnt + 1, -1);
        g_free (new_str);
    } else {
        gtk_list_store_set (self->priv->album_store, &first, 1, cnt + 1, -1);
    }

    insert_iter (GTK_LIST_STORE (self->priv->title_store), &iter, entry,
        (MusicCompareFunc) entry_cmp, 0, TRUE);

    gtk_list_store_set (GTK_LIST_STORE (self->priv->title_store), &iter,
        0, entry, 1, TRUE, -1);

//    gdk_threads_leave ();

//    album_add_entry (ALBUM (self->priv->album),
//        entry_get_album (entry), entry_get_artist (entry));

//    title_add_entry (TITLE (self->priv->title), entry);
}

Entry*
music_get_next (Music *self)
{
//    return title_get_next (TITLE (self->priv->title));
    return NULL;
}

Entry*
music_get_prev (Music *self)
{
//    return title_get_prev (TITLE (self->priv->title));
    return NULL;
}

void
music_remove_entry (Music *self, guint id)
{
/*
    Entry *e = title_remove_entry (TITLE (self->priv->title), id);

    if (e) {
        album_remove_entry (ALBUM (self->priv->album), entry_get_album (e));
        artist_remove_entry (ARTIST (self->priv->artist), entry_get_artist (e));

        g_object_unref (G_OBJECT (e));
    }
*/
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
        g_object_set (G_OBJECT (cell), "text",
            g_strdup_printf ("%d", entry_get_tag_int (entry, data)), NULL);
    } else {
        g_object_set (G_OBJECT (cell), "text", "", NULL);
    }
}

static gpointer
initial_import (Music *self)
{
    gchar *tags[] = { "id", "artist", "album", "title", "duration", "track", "location", NULL };
    GPtrArray *entries = gmediadb_get_all_entries (self->priv->db, tags);

    gdk_threads_enter ();

    int i;
    for (i = 0; i < entries->len; i++) {
        gchar **entry = g_ptr_array_index (entries, i);
        Entry *e = entry_new (entry[0] ? atoi (entry[0]) : 0);
        g_print ("(%s,%s,%s)\n", entry[1], entry[2], entry[3]);
//        entry_set_artist (e, entry[1]);
//        entry_set_album (e, entry[2]);
//        entry_set_title (e, entry[3]);
//        entry_set_duration (e, entry[4] ? atoi (entry[4]): 0);
//        entry_set_track (e, entry[5] ? atoi (entry[5]) : 0);
        entry_set_tag_str (e, "artist", entry[1] ? entry[1] : "");
        entry_set_tag_str (e, "album", entry[2] ? entry[2] : "");
        entry_set_tag_str (e, "title", entry[3] ? entry[3] : "");

        entry_set_tag_int (e, "duration", entry[4] ? atoi (entry[4]) : 0);
        entry_set_tag_int (e, "track", entry[5] ? atoi (entry[5]) : 0);

        entry_set_location (e, entry[6]);

        music_add_entry (self, e);

        g_object_unref (G_OBJECT (e));
    }

    gdk_threads_leave ();

    g_ptr_array_free (entries, TRUE);
}

static void
artist_cursor_changed (GtkTreeView *treeview, Music *self)
{

    GtkTreePath *path;

    gtk_tree_view_get_cursor (GTK_TREE_VIEW (treeview), &path, NULL);

    gchar *path_str = gtk_tree_path_to_string (path);
/*
    if (!g_strcmp0 (path_str, "0")) {
        g_signal_emit (G_OBJECT (self), signal_select, 0, "");
    } else {
        GtkTreeIter iter;
        gchar *art;

        gtk_tree_model_get_iter (GTK_TREE_MODEL (self->priv->store),
            &iter, path);

        gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store),
            &iter, 0, &art, -1);

        g_signal_emit (G_OBJECT (self), signal_select, 0, art);
        g_free (art);
    }

    g_free (path_str);
    gtk_tree_path_free (path);
*/
    g_print ("ARTIST-CC: %s\n", path_str);
}

static void
album_cursor_changed (GtkTreeView *view, Music *self)
{
    g_print ("ALBUM-CC\n");
}

static void
artist_row_activated (GtkTreeView *view,
                      GtkTreePath *path,
                      GtkTreeViewColumn *column,
                      Music *self)
{
    /*
    GtkTreeIter iter;
    gchar *art;
    gchar *path_str = gtk_tree_path_to_string (path);

    if (!g_strcmp0 (path_str, "0")) {
        g_signal_emit (G_OBJECT (self), signal_replace, 0, "");
    } else {
        gtk_tree_model_get_iter (GTK_TREE_MODEL (self->priv->store),
            &iter, path);

        gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store),
            &iter, 0, &art, -1);

        g_signal_emit (G_OBJECT (self), signal_replace, 0, art);
        g_free (art);
    }

    g_free (path_str);
    */
    g_print ("ARTIST-RA\n");
}


static void
album_row_activated (GtkTreeView *view,
                     GtkTreePath *path,
                     GtkTreeViewColumn *column,
                     Music *self)
{
    g_print ("ALBUM-RA\n");
}

static void
title_row_activated (GtkTreeView *view,
                     GtkTreePath *path,
                     GtkTreeViewColumn *column,
                     Music *self)
{
    g_print ("TITLE-RA\n");
}

static gboolean
insert_iter (GtkListStore *store,
             GtkTreeIter *iter,
             gpointer ne,
             MusicCompareFunc cmp,
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
