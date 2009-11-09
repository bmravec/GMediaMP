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

#include "shell.h"
#include "player.h"
#include "progress.h"
#include "playlist.h"

#include "music.h"
#include "movies.h"
#include "shows.h"

#include "tag-handler.h"
#include "tray.h"
#include "mini-pane.h"

#include "track-source.h"
#include "media-store.h"

G_DEFINE_TYPE(Shell, shell, G_TYPE_OBJECT)

struct _ShellPrivate {
    Player *player;

    Music *music;
    Movies *movies;
    Shows *shows;

    Playlist *playlist;
    Tray *tray;
    TagHandler *tag_handler;

    GtkBuilder *builder;

    GtkTreeStore *sidebar_store;

    GtkWidget *progress_bars;

    TrackSource *playing_source;
    Entry *playing_entry;

    GPtrArray *stores;

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

static gdouble prev_ratio;
static Shell *instance = NULL;

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

static void on_entry_move (MediaStore *ms, Entry *entry, Shell *self);

static void
on_destroy (GtkWidget *widget, Shell *self)
{
    shell_quit (self);
}

static void
on_ts_play (TrackSource *ts, Entry *entry, Shell *self)
{
    self->priv->playing_source = ts;
    g_object_ref (self->priv->playing_source);

    self->priv->playing_entry = entry;
    g_object_ref (self->priv->playing_entry);

    player_load (self->priv->player, entry);
    player_play (self->priv->player);

    update_info_label (self);
}

static void
on_player_ratio (Player *player, guint pos, Shell *self)
{
    guint len = player_get_length (self->priv->player);

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
    };
}

static gboolean
on_pos_change_value (GtkWidget *range,
                     GtkScrollType scroll,
                     gdouble value,
                     Shell *self)
{
    gint len = player_get_length (self->priv->player);

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

    self->priv->builder = gtk_builder_new ();

    self->priv->player = player_new (0, NULL);
    self->priv->music = music_new ();
    self->priv->movies = movies_new ();
    self->priv->shows = shows_new ();
    self->priv->playlist = playlist_new ();
    self->priv->tray = tray_new ();
    self->priv->tag_handler = tag_handler_new ();
    self->priv->mini_pane = mini_pane_new ();

    g_ptr_array_add (self->priv->stores, self->priv->music);
    g_ptr_array_add (self->priv->stores, self->priv->movies);
    g_ptr_array_add (self->priv->stores, self->priv->shows);

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
    self->priv->sidebar_store = gtk_tree_store_new (3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT);
    gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->sidebar_view),
        GTK_TREE_MODEL (self->priv->sidebar_store));

    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;

    column = gtk_tree_view_column_new ();
    renderer = gtk_cell_renderer_pixbuf_new ();
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_add_attribute (column, renderer, "pixbuf", 0);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, renderer, TRUE);
    gtk_tree_view_column_add_attribute (column, renderer, "text", 1);
    gtk_tree_view_column_set_expand (column, TRUE);

    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->sidebar_view), column);

    g_signal_connect (self->priv->window, "destroy", G_CALLBACK (on_destroy), self);
    g_signal_connect (self->priv->tb_prev, "clicked", G_CALLBACK (prev_cb), self);
    g_signal_connect (self->priv->tb_next, "clicked", G_CALLBACK (next_cb), self);
    g_signal_connect (self->priv->tb_pause, "clicked", G_CALLBACK (pause_cb), self);
    g_signal_connect (self->priv->tb_play, "clicked", G_CALLBACK (play_cb), self);

    g_signal_connect (self->priv->tray, "play", G_CALLBACK (play_cb), self);
    g_signal_connect (self->priv->tray, "pause", G_CALLBACK (pause_cb), self);
    g_signal_connect (self->priv->tray, "next", G_CALLBACK (next_cb), self);
    g_signal_connect (self->priv->tray, "previous", G_CALLBACK (prev_cb), self);

    g_signal_connect (self->priv->player, "play", G_CALLBACK (play_cb), self);
    g_signal_connect (self->priv->player, "pause", G_CALLBACK (pause_cb), self);
    g_signal_connect (self->priv->player, "next", G_CALLBACK (next_cb), self);
    g_signal_connect (self->priv->player, "previous", G_CALLBACK (prev_cb), self);
    g_signal_connect (self->priv->player, "volume-changed", G_CALLBACK (on_player_vol_changed), self);

    // Hook up TrackSource signals
    g_signal_connect (self->priv->music, "entry-play", G_CALLBACK (on_ts_play), self);
    g_signal_connect (self->priv->movies, "entry-play", G_CALLBACK (on_ts_play), self);
    g_signal_connect (self->priv->shows, "entry-play", G_CALLBACK (on_ts_play), self);

    // Hook up MediaStore signals
    g_signal_connect (self->priv->tag_handler, "entry-move", G_CALLBACK (on_entry_move), self);
    g_signal_connect (self->priv->music, "entry-move", G_CALLBACK (on_entry_move), self);
    g_signal_connect (self->priv->movies, "entry-move", G_CALLBACK (on_entry_move), self);
    g_signal_connect (self->priv->shows, "entry-move", G_CALLBACK (on_entry_move), self);

    self->priv->visible = TRUE;
    gtk_widget_show (self->priv->window);

    GtkTreeSelection *tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->sidebar_view));
    g_signal_connect (tree_selection, "changed", G_CALLBACK (selector_changed_cb), self);

    g_signal_connect (self->priv->player, "pos-changed", G_CALLBACK (on_player_ratio), self);
    g_signal_connect (self->priv->player, "state-changed", G_CALLBACK (on_player_state_changed), self);
    g_signal_connect (self->priv->player, "eos", G_CALLBACK (on_player_eos), self);

    g_signal_connect (self->priv->play_pos, "change-value", G_CALLBACK (on_pos_change_value), self);
    g_signal_connect (self->priv->tb_vol, "value-changed", G_CALLBACK (on_vol_changed), self);

    self->priv->playing_source = NULL;
    self->priv->playing_entry = NULL;
}

Shell*
shell_new ()
{
    if (!instance) {
        instance = g_object_new (SHELL_TYPE, NULL);
    }

    return instance;
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
        gtk_tree_store_append (self->priv->sidebar_store, &iter, parent);
    } else {
        do {
            gchar *str;

            gtk_tree_model_get (GTK_TREE_MODEL (self->priv->sidebar_store), &iter, 1, &str, -1);

            if (!g_strcmp0 (str, path[i])) {
                return TRUE;
            }
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->sidebar_store), &iter));

        gtk_tree_store_append (self->priv->sidebar_store, &iter, parent);
    }

    gtk_tree_store_set (self->priv->sidebar_store, &iter,
        0, icon ? gdk_pixbuf_new_from_file_at_size (icon, 16, 16, NULL) : NULL,
        1, path[len-1],
        2, page,
        -1);

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

    if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
        gtk_tree_model_get (GTK_TREE_MODEL (self->priv->sidebar_store), &iter,
                        2, &page, -1);

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

    player_activate (shell->priv->player);
    playlist_activate (shell->priv->playlist);
    music_activate (shell->priv->music);
    movies_activate (shell->priv->movies);
    shows_activate (shell->priv->shows);
    tag_handler_activate (shell->priv->tag_handler);
    tray_activate (shell->priv->tray);

    mini_pane_activate (MINI_PANE (shell->priv->mini_pane));

    gtk_widget_show (shell->priv->mini_pane);

    gtk_box_pack_start (GTK_BOX (shell->priv->sidebar), shell->priv->mini_pane, FALSE, FALSE, 0);

    shell_run (shell);

    g_object_unref (shell->priv->music);
    g_object_unref (shell->priv->movies);
    g_object_unref (shell->priv->shows);
}

static void
import_file_cb (GtkMenuItem *item, Shell *self)
{
    GtkWidget *dialog;

    dialog = gtk_file_chooser_dialog_new ("Import File",
                    GTK_WINDOW (self->priv->window),
                    GTK_FILE_CHOOSER_ACTION_OPEN,
                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                    "Import", GTK_RESPONSE_ACCEPT,
                    NULL);
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *filename;

        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

        shell_import_path (self, filename);
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

        shell_import_path (self, filename);

        g_free (filename);
    }

    gtk_widget_destroy (dialog);
}

static void
shell_import_thread_rec (Shell *self, const gchar *path)
{
    if (g_file_test (path, G_FILE_TEST_IS_REGULAR)) {
        tag_handler_add_entry (self->priv->tag_handler, path);
//        g_print ("IMPORTING: %s\n", path);
    } else if (g_file_test (path, G_FILE_TEST_IS_DIR)) {
        GDir *dir = g_dir_open (path, 0, NULL);
        const gchar *entry;
        while (entry = g_dir_read_name (dir)) {
            gchar *new_path = g_strdup_printf ("%s/%s", path, entry);
//            gchar *new_path = g_strjoin ("/", path, entry, NULL);
//            import_recurse (new_path);
            shell_import_thread_rec (self, new_path);
            g_free (new_path);
        }

        g_dir_close (dir);
    }
}

static gpointer
shell_import_thread (gchar *path)
{
    shell_import_thread_rec (shell_new (), path);
    g_free (path);
}

gboolean
shell_import_path (Shell *self, const gchar *path)
{
//    g_print ("IMPORTING: %s\n", path);

    g_thread_create ((GThreadFunc) shell_import_thread, (gpointer) g_strdup (path), FALSE, NULL);
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

static void
on_entry_move (MediaStore *ms, Entry *entry, Shell *self)
{
    gint i;

    for (i = 0; i < self->priv->stores->len; i++) {
        MediaStore *nms = MEDIA_STORE (g_ptr_array_index (self->priv->stores, i));
        if (entry_get_media_type (entry) == media_store_get_media_type (nms)) {
            media_store_add_entry (nms, entry);
        }
    }
}
