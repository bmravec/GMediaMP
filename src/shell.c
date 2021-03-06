/*
 *      shell.c
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

#include "browser.h"
#include "device-manager.h"
#include "mini-pane.h"
#include "player.h"
#include "playlist.h"
#include "progress.h"
#include "shell.h"
#include "tag-reader.h"
#include "tray.h"
#include "gmediadb-store.h"
#include "column-funcs.h"

G_DEFINE_TYPE(Shell, shell, G_TYPE_OBJECT)

struct _ShellPrivate {
    Player *player;

    Browser *musicb, *moviesb, *showsb, *music_videosb;
    GMediaDBStore *musics, *moviess, *showss, *music_videoss;

    Playlist *playlist;
    Tray *tray;
    TagReader *tag_reader;
    DeviceManager *device_manager;

    GtkBuilder *builder;

    GtkTreeModel *sidebar_store;

    GtkWidget *progress_bars;

    TrackSource *playing_source;
    Entry *playing_entry;

    GPtrArray *stores, *devices;

    gboolean visible;

    GtkWidget *mini_pane;
    GtkWidget *window;
    GtkWidget *sidebar;
    GtkWidget *sidebar_view;
    GtkWidget *sidebar_book;
    GtkWidget *play_pos;
    GtkWidget *info_label;
    GtkWidget *time_label;

    GtkWidget *tb_prev;
    GtkWidget *tb_pause;
    GtkWidget *tb_play;
    GtkWidget *tb_next;
    GtkWidget *tb_vol;
};

guint signal_eos;
guint signal_tag;
guint signal_state;
guint signal_ratio;

GStaticRecMutex rmutex;

static void selector_changed_cb (GtkTreeSelection *selection, Shell *self);
static void import_file_cb (GtkMenuItem *item, Shell *self);
static void import_dir_cb (GtkMenuItem *item, Shell *self);

static void play_cb (GtkWidget *widget, Shell *self);
static void pause_cb (GtkWidget *widget, Shell *self);
static void prev_cb (GtkWidget *widget, Shell *self);
static void next_cb (GtkWidget *widget, Shell *self);
static void update_info_label (Shell *self);
static gboolean on_pos_change_value (GtkWidget *range, GtkScrollType scroll,
    gdouble value, Shell *self);
static void on_vol_changed (GtkWidget *widget, gdouble val, Shell *self);
static void on_player_vol_changed (GtkWidget *widget, gdouble val, Shell *self);

static void
on_destroy (GtkWidget *widget, Shell *self)
{
    shell_quit (self);
}

static void
on_ts_play (Shell *self, Entry *entry, TrackSource *ts)
{
    self->priv->playing_source = ts;
    g_object_ref (self->priv->playing_source);

    self->priv->playing_entry = entry;
    g_object_ref (self->priv->playing_entry);

    if (entry) {
        g_print ("Playing file: %s\n", entry_get_location (entry));
    }

    player_load (self->priv->player, entry);
    player_play (self->priv->player);

    update_info_label (self);
}

static void
on_player_ratio (Player *player, guint pos, Shell *self)
{
    guint len = player_get_duration (self->priv->player);

    if (len > 0.0) {
        gtk_range_set_range (GTK_RANGE (self->priv->play_pos), 0.0, len);
        gtk_range_set_value (GTK_RANGE (self->priv->play_pos), pos);
    } else {
        gtk_range_set_range (GTK_RANGE (self->priv->play_pos), 0.0, 1.0);
        gtk_range_set_value (GTK_RANGE (self->priv->play_pos), 0.0);
    }

    gchar *pos_str = time_to_string (pos);
    gchar *len_str = time_to_string (len);

    gchar *str = g_strdup_printf ("%s of %s", pos_str, len_str);
    gtk_label_set_text (GTK_LABEL (self->priv->time_label), str);
    g_free (str);

    g_free (pos_str);
    g_free (len_str);
}

static void
on_player_eos (Player *player, Shell *self)
{
    if (self->priv->playing_entry) {
        g_object_unref (self->priv->playing_entry);
        self->priv->playing_entry = NULL;
    }

    if (self->priv->playing_source) {
        Entry *ne = track_source_get_next (self->priv->playing_source);
        if (ne) {
            player_load (self->priv->player, ne);
            player_play (self->priv->player);

            self->priv->playing_entry = ne;
            g_object_ref (self->priv->playing_entry);
        } else {
            player_close (self->priv->player);
        }
    } else {
        player_close (self->priv->player);
    }

    update_info_label (self);
}

static void
on_player_state_changed (Player *player, guint state, Shell *self)
{
    if (state == PLAYER_STATE_PLAYING) {
        gtk_widget_show (self->priv->tb_pause);
        gtk_widget_hide (self->priv->tb_play);
    } else {
        gtk_widget_show (self->priv->tb_play);
        gtk_widget_hide (self->priv->tb_pause);
    }

//    if (state == PLAYER_STATE_PLAYING || state == PLAYER_STATE_PAUSED) {
//        gtk_widget_show (self->priv->mini_pane);
//    } else {
//        gtk_widget_hide (self->priv->mini_pane);
//    }
}

static gboolean
on_pos_change_value (GtkWidget *range,
                     GtkScrollType scroll,
                     gdouble value,
                     Shell *self)
{
    gint len = player_get_duration (self->priv->player);

    if (value > len) value = len;
    if (value < 0.0) value = 0.0;

    player_set_position (self->priv->player,  value);

    return FALSE;
}

static void
shell_finalize (GObject *object)
{
    Shell *self = SHELL (object);

    G_OBJECT_CLASS (shell_parent_class)->finalize (object);
}

static void
shell_class_init (ShellClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (ShellPrivate));

    object_class->finalize = shell_finalize;

    signal_eos = g_signal_new ("eos", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    signal_tag = g_signal_new ("tag", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    signal_state = g_signal_new ("state-changed", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__UINT,
        G_TYPE_NONE, 1, G_TYPE_UINT);

    signal_ratio = g_signal_new ("ratio-changed", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__DOUBLE,
        G_TYPE_NONE, 1, G_TYPE_DOUBLE);
}

static void
shell_init (Shell *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), SHELL_TYPE, ShellPrivate);

    self->priv->stores = g_ptr_array_new ();
    self->priv->devices = g_ptr_array_new ();

    self->priv->builder = gtk_builder_new ();

    // Load objects from main.ui
    gtk_builder_add_from_file (self->priv->builder, SHARE_DIR "/ui/main.ui", NULL);
    self->priv->window = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "main_win"));
    self->priv->progress_bars = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "progress_bars"));
    self->priv->sidebar = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "sidebar"));
    self->priv->sidebar_view = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "sidebar_view"));
    self->priv->sidebar_book = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "sidebar_book"));
    self->priv->play_pos = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "play_pos"));
    self->priv->info_label = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "info_label"));
    self->priv->time_label = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "time_label"));

    self->priv->tb_prev = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "tb_prev"));
    self->priv->tb_pause = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "tb_pause"));
    self->priv->tb_play = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "tb_play"));
    self->priv->tb_next = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "tb_next"));
    self->priv->tb_vol = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "tb_vol"));

    gtk_window_set_icon_from_file (GTK_WINDOW (self->priv->window),
        SHARE_DIR "/imgs/tray-icon.svg", NULL);

    // Connect signals for menu items
    g_signal_connect (gtk_builder_get_object (self->priv->builder, "menu_quit"),"activate", G_CALLBACK (shell_quit), NULL);
    g_signal_connect (gtk_builder_get_object (self->priv->builder, "menu_import_file"),"activate", G_CALLBACK (import_file_cb), self);
    g_signal_connect (gtk_builder_get_object (self->priv->builder, "menu_import_directory"),"activate", G_CALLBACK (import_dir_cb), self);

    // Create stores and columns
    self->priv->sidebar_store = GTK_TREE_MODEL (gtk_tree_store_new (4, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT, GTK_TYPE_WIDGET));
    gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->sidebar_view),
        GTK_TREE_MODEL (self->priv->sidebar_store));

    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;

    column = gtk_tree_view_column_new ();
    renderer = gtk_cell_renderer_pixbuf_new ();
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_add_attribute (column, renderer, "pixbuf", 0);

    renderer = gtk_cell_renderer_text_new ();
    g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
    gtk_tree_view_column_pack_start (column, renderer, TRUE);
    gtk_tree_view_column_add_attribute (column, renderer, "text", 1);
    gtk_tree_view_column_set_expand (column, TRUE);

    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->sidebar_view), column);

    g_signal_connect (self->priv->window, "destroy", G_CALLBACK (on_destroy), self);
    g_signal_connect (self->priv->tb_prev, "clicked", G_CALLBACK (prev_cb), self);
    g_signal_connect (self->priv->tb_next, "clicked", G_CALLBACK (next_cb), self);
    g_signal_connect (self->priv->tb_pause, "clicked", G_CALLBACK (pause_cb), self);
    g_signal_connect (self->priv->tb_play, "clicked", G_CALLBACK (play_cb), self);

    self->priv->visible = TRUE;
    gtk_widget_show (self->priv->window);

    GtkTreeSelection *tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->sidebar_view));
    g_signal_connect (tree_selection, "changed", G_CALLBACK (selector_changed_cb), self);

    g_signal_connect (self->priv->play_pos, "change-value", G_CALLBACK (on_pos_change_value), self);
    g_signal_connect (self->priv->tb_vol, "value-changed", G_CALLBACK (on_vol_changed), self);

    self->priv->playing_source = NULL;
    self->priv->playing_entry = NULL;
}

Shell*
shell_new ()
{
    Shell *self = g_object_new (SHELL_TYPE, NULL);

    self->priv->player = player_new (self);
    self->priv->tag_reader = tag_reader_new (self);
    self->priv->tray = tray_new (self);
    self->priv->mini_pane = mini_pane_new (self);
    self->priv->playlist = playlist_new (self);

    shell_add_widget (self, gtk_label_new ("Library"), "Library", NULL);

    self->priv->device_manager = device_manager_new (self);

    g_signal_connect (self->priv->player, "play", G_CALLBACK (play_cb), self);
    g_signal_connect (self->priv->player, "pause", G_CALLBACK (pause_cb), self);
    g_signal_connect (self->priv->player, "next", G_CALLBACK (next_cb), self);
    g_signal_connect (self->priv->player, "previous", G_CALLBACK (prev_cb), self);
    g_signal_connect (self->priv->player, "volume-changed", G_CALLBACK (on_player_vol_changed), self);

    g_signal_connect (self->priv->player, "pos-changed", G_CALLBACK (on_player_ratio), self);
    g_signal_connect (self->priv->player, "state-changed", G_CALLBACK (on_player_state_changed), self);
    g_signal_connect (self->priv->player, "eos", G_CALLBACK (on_player_eos), self);

    g_signal_connect (self->priv->tray, "play", G_CALLBACK (play_cb), self);
    g_signal_connect (self->priv->tray, "pause", G_CALLBACK (pause_cb), self);
    g_signal_connect (self->priv->tray, "next", G_CALLBACK (next_cb), self);
    g_signal_connect (self->priv->tray, "previous", G_CALLBACK (prev_cb), self);

    return self;
}

gboolean
shell_register_track_source (Shell *self, TrackSource *ts)
{
    g_signal_connect_swapped (ts, "entry-play", G_CALLBACK (on_ts_play), self);
}

gboolean
shell_register_media_store (Shell *self, MediaStore *ms)
{
//    g_signal_connect_swapped (ms, "entry-move", G_CALLBACK (on_entry_move), self);

    g_ptr_array_add (self->priv->stores, ms);
}

gboolean
shell_register_device (Shell *self, Device *dev)
{
    g_ptr_array_add (self->priv->devices, dev);
}

Player*
shell_get_player (Shell *self)
{
    return self->priv->player;
}

gboolean
shell_add_widget (Shell *self,
                  GtkWidget *widget,
                  gchar *name,
                  gchar *icon)
{
    GtkTreeIter iter, *parent = NULL;
    gchar **path = g_strsplit (name, "/", 0);
    gint len = 0, i, page = -1;
    GtkTreePath *tpath;

    while (path[len++]);
    len--;

    for (i = 0; i < len - 1; i++) {
        gboolean found = FALSE;
        gtk_tree_model_iter_children (GTK_TREE_MODEL (self->priv->sidebar_store),
            &iter, parent);

        if (parent) {
            gtk_tree_iter_free (parent);
            parent = NULL;
        }

        do {
            gchar *str;

            gtk_tree_model_get (GTK_TREE_MODEL (self->priv->sidebar_store), &iter, 1, &str, -1);

            if (!g_strcmp0 (str, path[i])) {
                found = TRUE;
                break;
            }
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->sidebar_store), &iter));

        if (found) {
            parent = gtk_tree_iter_copy (&iter);
        } else {
            return FALSE;
        }
    }

    if (widget) {
        page = gtk_notebook_append_page (GTK_NOTEBOOK (self->priv->sidebar_book),
            widget, NULL);

        gtk_widget_show (widget);
    }

    if (!gtk_tree_model_iter_children (GTK_TREE_MODEL (self->priv->sidebar_store),
        &iter, parent)) {
        gtk_tree_store_append (GTK_TREE_STORE (self->priv->sidebar_store), &iter, parent);
    } else {
        do {
            gchar *str;

            gtk_tree_model_get (GTK_TREE_MODEL (self->priv->sidebar_store), &iter, 1, &str, -1);

            if (!g_strcmp0 (str, path[i])) {
                return TRUE;
            }
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->sidebar_store), &iter));

        gtk_tree_store_append (GTK_TREE_STORE (self->priv->sidebar_store), &iter, parent);
    }

    gtk_tree_store_set (GTK_TREE_STORE (self->priv->sidebar_store), &iter,
        0, icon ? gdk_pixbuf_new_from_file_at_size (icon, 16, 16, NULL) : NULL,
        1, path[len-1],
        2, page,
        3, widget,
        -1);

    if (tpath = gtk_tree_model_get_path (GTK_TREE_MODEL (self->priv->sidebar_store), &iter)) {
        gtk_tree_view_expand_to_path (GTK_TREE_VIEW (self->priv->sidebar_view), tpath);
        gtk_tree_path_free (tpath);
    }

    if (parent) {
        gtk_tree_iter_free (parent);
        parent = NULL;
    }

    g_strfreev (path);

    return TRUE;
}

gboolean
shell_remove_widget (Shell *self, gchar *name)
{

}

gboolean
shell_add_progress (Shell *self, Progress *p)
{
    gdk_threads_enter ();
    gtk_box_pack_start (GTK_BOX (self->priv->progress_bars), progress_get_widget (p), FALSE, FALSE, 0);
    gdk_threads_leave ();
}

gboolean
shell_remove_progress (Shell *self, Progress *p)
{
    gdk_threads_enter ();
    gtk_container_remove (GTK_CONTAINER (self->priv->progress_bars), progress_get_widget (p));
    gdk_threads_leave ();
}

static void
selector_changed_cb (GtkTreeSelection *selection, Shell *self)
{
    GtkTreeIter iter;
    int page;
    gchar *name;

    g_print ("Selector changed\n");

    if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
        gtk_tree_model_get (GTK_TREE_MODEL (self->priv->sidebar_store), &iter,
                        1, &name, 2, &page, -1);

        if (!g_strcmp0 (name, "Now Playing")) {
            player_set_video_destination (self->priv->player, NULL);
        } else {
            player_set_video_destination (self->priv->player,
                GTK_WIDGET (self->priv->mini_pane));
        }

        g_free (name);

        if (page >= 0) {
            gtk_notebook_set_current_page (GTK_NOTEBOOK (self->priv->sidebar_book), page);
        }
    }
}

GtkBuilder*
shell_get_builder (Shell *self)
{
    return self->priv->builder;
}

void
shell_run (Shell *self)
{
    gdk_threads_enter ();
    gtk_main ();
    gdk_threads_leave ();
}

void
shell_quit (Shell *self)
{
    gtk_main_quit ();
}

void
shell_toggle_visibility (Shell *self)
{
    if (self->priv->visible) {
        gtk_widget_hide (self->priv->window);
        self->priv->visible = FALSE;
    } else {
        gtk_widget_show (self->priv->window);
        self->priv->visible = TRUE;
    }
}

void
shell_hide (Shell *self)
{
    gtk_widget_hide (self->priv->window);
}

void
shell_show (Shell *self)
{
    gtk_widget_show (self->priv->window);
}

void
lock_function () {
    g_static_rec_mutex_lock (&rmutex);
}

void
unlock_function () {
    g_static_rec_mutex_unlock (&rmutex);
}

static gint
tvshow_entry_cmp (Entry *e1, Entry *e2)
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

    if (entry_get_tag_int (e2, "tracknumber") != entry_get_tag_int (e1, "tracknumber"))
        return entry_get_tag_int (e1, "tracknumber") - entry_get_tag_int (e2, "tracknumber");

    res = g_strcmp0 (entry_get_tag_str (e1, "title"), entry_get_tag_str (e2, "title"));
    if (res != 0)
        return res;

    return -1;
}

static gint
music_entry_cmp (Entry *e1, Entry *e2)
{
    gint res;
    if (entry_get_id (e1) == entry_get_id (e2))
        return 0;

    res = g_strcmp0 (entry_get_tag_str (e1, "artist"), entry_get_tag_str (e2, "artist"));
    if (res != 0)
        return res;

    res = g_strcmp0 (entry_get_tag_str (e1, "album"), entry_get_tag_str (e2, "album"));
    if (res != 0)
        return res;

    if (entry_get_tag_int (e2, "tracknumber") != entry_get_tag_int (e1, "tracknumber"))
        return entry_get_tag_int (e1, "tracknumber") - entry_get_tag_int (e2, "tracknumber");

    res = g_strcmp0 (entry_get_tag_str (e1, "title"), entry_get_tag_str (e2, "title"));
    if (res != 0)
        return res;

    return -1;
}

static gint
movie_entry_cmp (Entry *e1, Entry *e2)
{
    return g_strcmp0 (entry_get_tag_str (e1, "title"), entry_get_tag_str (e2, "title"));
}

static gint
music_video_entry_cmp (Entry *e1, Entry *e2)
{
    gint res;
    if (entry_get_id (e1) == entry_get_id (e2))
        return 0;

    res = g_strcmp0 (entry_get_tag_str (e1, "artist"), entry_get_tag_str (e2, "artist"));
    if (res != 0)
        return res;

    res = g_strcmp0 (entry_get_tag_str (e1, "title"), entry_get_tag_str (e2, "title"));
    if (res != 0)
        return res;

    return -1;
}

int
main (int argc, char *argv[])
{
    g_type_init ();

    g_static_rec_mutex_init (&rmutex);

    gdk_threads_set_lock_functions (G_CALLBACK (lock_function),
        G_CALLBACK (unlock_function));

    g_thread_init (NULL);
    gdk_threads_init ();

    gtk_init (&argc, &argv);

    Shell *shell = shell_new ();

    // Create Music store/widget
    shell->priv->musics = gmediadb_store_new ("Music", MEDIA_SONG);
    shell->priv->musicb = browser_new_with_model (shell, MEDIA_STORE (shell->priv->musics));

    browser_add_column (shell->priv->musicb, "Track", "tracknumber",
        FALSE, (GtkTreeCellDataFunc) int_column_func);
    browser_add_column (shell->priv->musicb, "Title", "title",
        TRUE, (GtkTreeCellDataFunc) str_column_func);
    browser_add_column (shell->priv->musicb, "Artist", "artist",
        TRUE, (GtkTreeCellDataFunc) str_column_func);
    browser_add_column (shell->priv->musicb, "Album", "album",
        TRUE, (GtkTreeCellDataFunc) str_column_func);
    browser_add_column (shell->priv->musicb, "Duration", "duration",
        FALSE, (GtkTreeCellDataFunc) time_column_func);
    browser_set_compare_func (shell->priv->musicb, music_entry_cmp);
    browser_set_pane1_tag (shell->priv->musicb, "Artist", "artist");
    browser_set_pane2_tag (shell->priv->musicb, "Album", "album");

    shell_add_widget (shell, GTK_WIDGET (shell->priv->musicb), "Library/Music", NULL);
    shell_register_track_source (shell, TRACK_SOURCE (shell->priv->musicb));
    shell_register_media_store (shell, MEDIA_STORE (shell->priv->musics));


    // Create Movies store/widget
    shell->priv->moviess = gmediadb_store_new ("Movies", MEDIA_MOVIE);
    shell->priv->moviesb = browser_new_with_model (shell, MEDIA_STORE (shell->priv->moviess));

    browser_add_column (shell->priv->moviesb, "Title", "title",
        TRUE, (GtkTreeCellDataFunc) str_column_func);
    browser_add_column (shell->priv->moviesb, "Duration", "duration",
        FALSE, (GtkTreeCellDataFunc) time_column_func);
    browser_set_compare_func (shell->priv->moviesb, movie_entry_cmp);

    shell_add_widget (shell, GTK_WIDGET (shell->priv->moviesb), "Library/Movies", NULL);
    shell_register_track_source (shell, TRACK_SOURCE (shell->priv->moviesb));
    shell_register_media_store (shell, MEDIA_STORE (shell->priv->moviess));

    // Create MusicVideo store/widget
    shell->priv->music_videoss = gmediadb_store_new ("MusicVideos", MEDIA_MUSIC_VIDEO);
    shell->priv->music_videosb = browser_new_with_model (shell, MEDIA_STORE (shell->priv->music_videoss));

    browser_add_column (shell->priv->music_videosb, "Title", "title",
        TRUE, (GtkTreeCellDataFunc) str_column_func);
    browser_add_column (shell->priv->music_videosb, "Artist", "artist",
        TRUE, (GtkTreeCellDataFunc) str_column_func);
    browser_add_column (shell->priv->music_videosb, "Duration", "duration",
        FALSE, (GtkTreeCellDataFunc) time_column_func);
    browser_set_compare_func (shell->priv->music_videosb, music_video_entry_cmp);

    browser_set_pane1_tag (shell->priv->music_videosb, "Artist", "artist");

    shell_add_widget (shell, GTK_WIDGET (shell->priv->music_videosb), "Library/MusicVideos", NULL);
    shell_register_track_source (shell, TRACK_SOURCE (shell->priv->music_videosb));
    shell_register_media_store (shell, MEDIA_STORE (shell->priv->music_videoss));

    // Create TVShows store/widget
    shell->priv->showss = gmediadb_store_new ("TVShows", MEDIA_TVSHOW);
    shell->priv->showsb = browser_new_with_model (shell, MEDIA_STORE (shell->priv->showss));

    browser_add_column (shell->priv->showsb, "Track", "tracknumber",
        FALSE, (GtkTreeCellDataFunc) int_column_func);
    browser_add_column (shell->priv->showsb, "Title", "title",
        TRUE, (GtkTreeCellDataFunc) str_column_func);
    browser_add_column (shell->priv->showsb, "Show", "show",
        TRUE, (GtkTreeCellDataFunc) str_column_func);
    browser_add_column (shell->priv->showsb, "Season", "season",
        TRUE, (GtkTreeCellDataFunc) str_column_func);
    browser_add_column (shell->priv->showsb, "Duration", "duration",
        FALSE, (GtkTreeCellDataFunc) time_column_func);

    browser_set_pane1_tag (shell->priv->showsb, "Show", "show");
    browser_set_pane2_tag (shell->priv->showsb, "Season", "season");

    browser_set_compare_func (shell->priv->showsb, tvshow_entry_cmp);
    browser_set_pane2_single_mode (shell->priv->showsb, TRUE);

    shell_add_widget (shell, GTK_WIDGET (shell->priv->showsb), "Library/TVShows", NULL);
    shell_register_track_source (shell, TRACK_SOURCE (shell->priv->showsb));
    shell_register_media_store (shell, MEDIA_STORE (shell->priv->showss));

    gtk_widget_show (shell->priv->mini_pane);

    gtk_box_pack_start (GTK_BOX (shell->priv->sidebar), shell->priv->mini_pane, FALSE, FALSE, 0);

    shell_select_path (shell, "Library/Music");

    shell_run (shell);

    g_object_unref (shell->priv->player);
    if (shell->priv->playing_source) {
        g_object_unref (shell->priv->playing_source);
        shell->priv->playing_source = NULL;
    }

    browser_set_model (shell->priv->musicb, NULL);
    browser_set_model (shell->priv->moviesb, NULL);
    browser_set_model (shell->priv->music_videosb, NULL);
    browser_set_model (shell->priv->showsb, NULL);

    g_object_unref (shell->priv->musics);
    g_object_unref (shell->priv->moviess);
    g_object_unref (shell->priv->music_videoss);
    g_object_unref (shell->priv->showss);

    return 0;
}

static void
import_file_cb (GtkMenuItem *item, Shell *self)
{
    GtkWidget *dialog;
    GSList *fnames, *iter;

    dialog = gtk_file_chooser_dialog_new ("Import File",
                    GTK_WINDOW (self->priv->window),
                    GTK_FILE_CHOOSER_ACTION_OPEN,
                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                    "Import", GTK_RESPONSE_ACCEPT,
                    NULL);

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *filename;

        fnames = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dialog));
//        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

        for (iter = fnames; iter; iter = iter->next) {
            g_print ("URI: %s\n", iter->data);
            shell_import_path (self, iter->data, NULL);
        }

//        shell_import_path (self, filename, "Music");
        g_free (filename);
    }

    gtk_widget_destroy (dialog);
}

static void
import_dir_cb (GtkMenuItem *item, Shell *self)
{
    GtkWidget *dialog;

    dialog = gtk_file_chooser_dialog_new ("Import Directory",
                    GTK_WINDOW (self->priv->window),
                    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                    "Import", GTK_RESPONSE_ACCEPT,
                    NULL);
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename;

        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

        shell_import_path (self, filename, NULL);

        g_free (filename);
    }

    gtk_widget_destroy (dialog);
}

static void
shell_import_thread_rec (Shell *self, const gchar *path, const gchar *mtype)
{
    if (g_file_test (path, G_FILE_TEST_IS_REGULAR)) {
        tag_reader_queue_entry (self->priv->tag_reader, path, mtype);
//        g_print ("IMPORTING: %s\n", path);
    } else if (g_file_test (path, G_FILE_TEST_IS_DIR)) {
        GDir *dir = g_dir_open (path, 0, NULL);
        const gchar *entry;
        while (entry = g_dir_read_name (dir)) {
            gchar *new_path = g_strdup_printf ("%s/%s", path, entry);
//            gchar *new_path = g_strjoin ("/", path, entry, NULL);
//            import_recurse (new_path);
            shell_import_thread_rec (self, new_path, mtype);
            g_free (new_path);
        }

        g_dir_close (dir);
    }
}

struct SIData {
    Shell *shell;
    gchar *path;
    gchar *mtype;
};

static gpointer
shell_import_thread (struct SIData *d)
{
    shell_import_thread_rec (d->shell, d->path, d->mtype);

    g_object_unref (d->shell);
    g_free (d->path);
    if (d->mtype) {
        g_free (d->mtype);
    }
    g_free (d);
}

gboolean
shell_import_path (Shell *self, const gchar *path, const gchar *mtype)
{
//    g_print ("IMPORTING: %s\n", path);
    struct SIData *d = g_new0 (struct SIData, 1);

    d->shell = g_object_ref (self);
    d->path = g_strdup (path);
    if (mtype) {
        d->mtype = g_strdup (mtype);
    }

    g_thread_create ((GThreadFunc) shell_import_thread, (gpointer) d, FALSE, NULL);
}

static void
update_info_label (Shell *self)
{
    if (!self->priv->playing_entry) {
        gtk_label_set_text (GTK_LABEL (self->priv->info_label), "Not Playing");
        return;
    }

    const gchar *artist = entry_get_tag_str (self->priv->playing_entry, "artist");
    const gchar *album = entry_get_tag_str (self->priv->playing_entry, "album");
    const gchar *title = entry_get_tag_str (self->priv->playing_entry, "title");

    GString *str = g_string_new (title);

    if (album) {
        g_string_append_printf (str, " from %s", album);
    }

    if (artist) {
        g_string_append_printf (str, " by %s", artist);
    }

    gtk_label_set_text (GTK_LABEL (self->priv->info_label), str->str);

    g_string_free (str, TRUE);
}

static void
play_cb (GtkWidget *widget, Shell *self)
{
    if (self->priv->playing_entry) {
        player_play (self->priv->player);
    } else if (self->priv->playing_source) {
        Entry *ne = track_source_get_next (self->priv->playing_source);
        if (ne) {
            player_load (self->priv->player, ne);
            player_play (self->priv->player);

            self->priv->playing_entry = ne;
            g_object_ref (ne);

            update_info_label (self);
        }
    }
}

static void
pause_cb (GtkWidget *widget, Shell *self)
{
    player_pause (self->priv->player);
}

static void
prev_cb (GtkWidget *widget, Shell *self)
{
    if (!self->priv->playing_source || !self->priv->playing_entry) {
        return;
    }

    if (self->priv->playing_entry) {
        g_object_unref (self->priv->playing_entry);
        self->priv->playing_entry = NULL;
    }

    Entry *ne = track_source_get_prev (self->priv->playing_source);
    if (ne) {
        player_load (self->priv->player, ne);
        player_play (self->priv->player);

        self->priv->playing_entry = ne;
        g_object_ref (self->priv->playing_entry);
    } else {
        player_close (self->priv->player);
    }

    update_info_label (self);
}

static void
next_cb (GtkWidget *widget, Shell *self)
{
    if (!self->priv->playing_source || !self->priv->playing_entry) {
        return;
    }

    if (self->priv->playing_entry) {
        g_object_unref (self->priv->playing_entry);
        self->priv->playing_entry = NULL;
    }

    Entry *ne = track_source_get_next (self->priv->playing_source);
    if (ne) {
        player_load (self->priv->player, ne);
        player_play (self->priv->player);

        self->priv->playing_entry = ne;
        g_object_ref (self->priv->playing_entry);
    } else {
        player_close (self->priv->player);
    }

    update_info_label (self);
}

static void
on_vol_changed (GtkWidget *widget, gdouble val, Shell *self)
{
    player_set_volume (self->priv->player, val);
}

static void
on_player_vol_changed (GtkWidget *widget, gdouble val, Shell *self)
{
    gtk_scale_button_set_value (GTK_SCALE_BUTTON (self->priv->tb_vol), val);
}

gchar**
shell_get_media_stores (Shell *self)
{
    gchar **list = g_new0 (gchar*, self->priv->stores->len+1);

    gint i;
    for (i = 0; i < self->priv->stores->len; i++) {
        MediaStore *ms = MEDIA_STORE (g_ptr_array_index (self->priv->stores, i));
        list[i] = media_store_get_name (ms);
    }

    return list;
}

gboolean
shell_move_to (Shell *self, gchar **e, const gchar *ms_name)
{
    gint i;
    for (i = 0; i < self->priv->stores->len; i++) {
        MediaStore *ms = MEDIA_STORE (g_ptr_array_index (self->priv->stores, i));
        if (!g_strcmp0 (media_store_get_name (ms), ms_name)) {
            media_store_add_entry (ms, e);
        }
    }
}

static gboolean
shell_select_widget_rec (Shell *self,
                         GtkWidget *widget,
                         GtkTreeIter *parent)
{
    GtkTreeIter iter;
    GtkWidget *w;

    gtk_tree_model_get (self->priv->sidebar_store, parent, 3, &w, -1);
    if (w == widget) {
        GtkTreePath *path = gtk_tree_model_get_path (self->priv->sidebar_store, parent);

        gtk_tree_view_set_cursor (GTK_TREE_VIEW (self->priv->sidebar_view),
            path, NULL, FALSE);

        gtk_tree_path_free (path);
        return TRUE;
    }

    if (gtk_tree_model_iter_children (self->priv->sidebar_store, &iter, parent)) {
        do {
            if (shell_select_widget_rec (self, widget, &iter)) {
                return TRUE;
            }
        } while (gtk_tree_model_iter_next (self->priv->sidebar_store, &iter));
    }

    return FALSE;
}

gboolean
shell_select_widget (Shell *self, GtkWidget *widget)
{
    GtkTreeIter iter;

    if (gtk_tree_model_iter_children (self->priv->sidebar_store, &iter, NULL)) {
        do {
            if (shell_select_widget_rec (self, widget, &iter)) {
                return TRUE;
            }
        } while (gtk_tree_model_iter_next (self->priv->sidebar_store, &iter));
    }

    return FALSE;
}

gboolean
shell_select_path (Shell *self, const gchar *name)
{
    GtkTreeIter iter, *parent = NULL;
    gchar **path = g_strsplit (name, "/", 0);
    gint len = 0, i, page = -1;
    GtkTreePath *tpath;

    while (path[len++]);
    len--;

    for (i = 0; i < len - 1; i++) {
        gboolean found = FALSE;
        gtk_tree_model_iter_children (GTK_TREE_MODEL (self->priv->sidebar_store),
            &iter, parent);

        if (parent) {
            gtk_tree_iter_free (parent);
            parent = NULL;
        }

        do {
            gchar *str;

            gtk_tree_model_get (GTK_TREE_MODEL (self->priv->sidebar_store), &iter, 1, &str, -1);

            if (!g_strcmp0 (str, path[i])) {
                found = TRUE;
                break;
            }
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->sidebar_store), &iter));

        if (found) {
            parent = gtk_tree_iter_copy (&iter);
        } else {
            return FALSE;
        }
    }

    if (!gtk_tree_model_iter_children (GTK_TREE_MODEL (self->priv->sidebar_store),
        &iter, parent)) {
        return FALSE;
    } else {
        gboolean found = FALSE;
        do {
            gchar *str;

            gtk_tree_model_get (GTK_TREE_MODEL (self->priv->sidebar_store), &iter, 1, &str, -1);

            if (!g_strcmp0 (str, path[i])) {
                found = TRUE;
                break;
            }
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->sidebar_store), &iter));

        if (!found) {
            return FALSE;
        }
    }

    if (tpath = gtk_tree_model_get_path (GTK_TREE_MODEL (self->priv->sidebar_store), &iter)) {
        gtk_tree_view_expand_to_path (GTK_TREE_VIEW (self->priv->sidebar_view), tpath);
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (self->priv->sidebar_view),
            tpath, NULL, FALSE);
        gtk_tree_path_free (tpath);
    }

    if (parent) {
        gtk_tree_iter_free (parent);
        parent = NULL;
    }

    g_strfreev (path);

    return TRUE;
}
