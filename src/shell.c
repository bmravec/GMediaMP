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
#include "browser.h"

G_DEFINE_TYPE(Shell, shell, G_TYPE_OBJECT)

struct _ShellPrivate {
    Player *player;
    GtkWidget *nowplaying;
    GtkWidget *browser;

    GtkBuilder *builder;

    GtkTreeStore *sidebar_store;

    GtkWidget *progress_bars;

    GtkWidget *window;
    GtkWidget *sidebar;
    GtkWidget *sidebar_view;
    GtkWidget *sidebar_book;
    GtkWidget *play_pos;
    GtkWidget *info_label;
    GtkWidget *time_label;

    GtkWidget *tb_prev;
    GtkWidget *tb_stop;
    GtkWidget *tb_pause;
    GtkWidget *tb_play;
    GtkWidget *tb_next;
    GtkWidget *tb_vol;
};

const gchar *builtin_uis[] = {
//    SHARE_DIR "ui/main.ui",
//    SHARE_DIR "ui/browser.ui"
    "data/ui/main.ui",
    "data/ui/browser.ui"
};

guint signal_eos;
guint signal_tag;
guint signal_state;
guint signal_ratio;

static gdouble prev_ratio;
static Shell *instance = NULL;

static void selector_changed_cb (GtkTreeSelection *selection, Shell *self);

static void
on_destroy (GtkWidget *widget, Shell *self)
{
    shell_quit (self);
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

    self->priv->player = player_new ();
    self->priv->builder = gtk_builder_new ();

    gint i;
    for (i = 0; i < sizeof (builtin_uis) / sizeof (gchar*); i++) {
        gtk_builder_add_from_file (self->priv->builder, builtin_uis[i], NULL);
    }

    self->priv->window = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "main_win"));
    self->priv->progress_bars = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "progress_bars"));
    self->priv->sidebar = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "sidebar"));
    self->priv->sidebar_view = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "sidebar_view"));
    self->priv->sidebar_book = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "sidebar_book"));
    self->priv->play_pos = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "play_pos"));
    self->priv->info_label = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "info_label"));
    self->priv->time_label = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "time_label"));

    self->priv->tb_prev = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "tb_prev"));
    self->priv->tb_stop = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "tb_stop"));
    self->priv->tb_pause = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "tb_pause"));
    self->priv->tb_play = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "tb_play"));
    self->priv->tb_next = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "tb_next"));
    self->priv->tb_vol = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "tb_vol"));

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

    g_signal_connect (G_OBJECT (self->priv->window), "destroy",
        G_CALLBACK (on_destroy), self);

    gtk_widget_show_all (self->priv->window);
    gtk_widget_hide (self->priv->tb_pause);

    GtkTreeSelection *tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->sidebar_view));
    g_signal_connect (tree_selection, "changed", G_CALLBACK (selector_changed_cb), self);
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

    gtk_tree_store_append (self->priv->sidebar_store, &iter, parent);
    gtk_tree_store_set (self->priv->sidebar_store, &iter,
        0, icon ? gdk_pixbuf_new_from_file_at_size (icon, 16, 16, NULL) : NULL,
        1, path[len-1],
        2, page,
        -1);

    if (parent) {
        gtk_tree_iter_free (parent);
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
    gtk_box_pack_start (GTK_BOX (self->priv->progress_bars), progress_get_widget (p), FALSE, FALSE, 0);
}

gboolean
shell_remove_progress (Shell *self, Progress *p)
{
    gtk_container_remove (GTK_CONTAINER (self->priv->progress_bars), progress_get_widget (p));
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

void
shell_run (Shell *self)
{
    gtk_main ();
}

void
shell_quit (Shell *self)
{
    gtk_main_quit ();
}

int
main (int argc, char *argv[])
{
    gtk_init (&argc, &argv);

    Shell *shell = shell_new ();

//    shell_add_widget (shell, gtk_label_new ("Library"), "Library", "/usr/share/icons/picard-32.png");
//    shell_add_widget (shell, gtk_label_new ("Music"), "Library/Music", "/usr/share/icons/picard-32.png");
//    shell_add_widget (shell, gtk_label_new ("Music Videos"), "Library/Music Videos", "/usr/share/icons/picard-32.png");
//    shell_add_widget (shell, gtk_label_new ("Devices"), "Devices", "/usr/share/icons/picard-32.png");
//    shell_add_widget (shell, gtk_label_new ("IPod"), "Devices/IPod", "/usr/share/icons/picard-32.png");
//    shell_add_widget (shell, gtk_label_new ("Music"), "Devices/IPod/Music", "/usr/share/icons/picard-32.png");

    Playlist *playlist = playlist_new ();
    Browser *browser = browser_new ();

    player_activate (shell_get_player (shell));
    playlist_activate (playlist);
    browser_activate (browser);

    shell_run (shell);
}
