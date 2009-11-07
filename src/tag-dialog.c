/*
 *      tag-dialog.c
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

#include "tag-dialog.h"

G_DEFINE_TYPE(TagDialog, tag_dialog, G_TYPE_OBJECT)

struct _TagDialogPrivate {
    GPtrArray *changes;

    GtkWidget *dialog;
    GtkWidget *view;
    GtkTreeModel *store;
};

static guint signal_completed;

static void on_response (TagDialog *self, gint response_id, GtkDialog *dialog);
static void on_add (TagDialog *self, GtkWidget *btn);
static void on_remove (TagDialog *self, GtkWidget *btn);
static void on_edit_key (TagDialog *self, gchar *path,
         gchar *new_text, GtkCellRendererText *renderer);
static void on_edit_val (TagDialog *self, gchar *path,
         gchar *new_text, GtkCellRendererText *renderer);

static void
tag_dialog_finalize (GObject *object)
{
    TagDialog *self = TAG_DIALOG (object);

    tag_dialog_close (self);

    G_OBJECT_CLASS (tag_dialog_parent_class)->finalize (object);
}

static void
tag_dialog_class_init (TagDialogClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (TagDialogPrivate));

    object_class->finalize = tag_dialog_finalize;

    signal_completed = g_signal_new ("completed", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1, G_TYPE_PTR_ARRAY);
}

static void
tag_dialog_init (TagDialog *self)
{
    GtkWidget *vbox, *hbox;

    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), TAG_DIALOG_TYPE, TagDialogPrivate);

    self->priv->changes = g_ptr_array_new ();

    self->priv->dialog = gtk_dialog_new_with_buttons (
        "Edit file tags...", NULL, GTK_DIALOG_MODAL,
        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL);

    gtk_window_set_default_size (GTK_WINDOW (self->priv->dialog), 350, 300);

    self->priv->store = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING));
    self->priv->view = gtk_tree_view_new_with_model (self->priv->store);

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    renderer = gtk_cell_renderer_text_new ();
    g_object_set (renderer, "editable", TRUE, NULL);
    g_signal_connect_swapped (renderer, "edited", G_CALLBACK (on_edit_key), self);
    column = gtk_tree_view_column_new_with_attributes ("Tag", renderer, "text", 0, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->view), column);

    renderer = gtk_cell_renderer_text_new ();
    g_object_set (renderer, "editable", TRUE, NULL);
    g_signal_connect_swapped (renderer, "edited", G_CALLBACK (on_edit_val), self);
    column = gtk_tree_view_column_new_with_attributes ("Value", renderer, "text", 1, NULL);
    g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->view), column);

    vbox = gtk_dialog_get_content_area (GTK_DIALOG (self->priv->dialog));

    GtkWidget *sb = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sb),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    gtk_container_add (GTK_CONTAINER (sb), self->priv->view);
    gtk_box_pack_start (GTK_BOX (vbox), sb, TRUE, TRUE, 0);

    hbox = gtk_hbox_new (FALSE, 0);

    GtkWidget *add, *remove;
    add = gtk_button_new_with_label ("Add");
    remove = gtk_button_new_with_label ("Remove");

    gtk_box_pack_start (GTK_BOX (hbox), remove, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), add, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    g_signal_connect_swapped (add, "clicked", G_CALLBACK (on_add), self);
    g_signal_connect_swapped (remove, "clicked", G_CALLBACK (on_remove), self);

    g_signal_connect_swapped (self->priv->dialog, "response",
                              G_CALLBACK (on_response), self);
}

TagDialog*
tag_dialog_new ()
{
    return g_object_new (TAG_DIALOG_TYPE, NULL);
}

gboolean
tag_dialog_add_entry (TagDialog *self, gchar **keys, gchar **vals)
{
    gint i;
    for (i = 0; keys[i] && vals[i]; i++) {
        GtkTreeIter iter;
        gboolean isnew = TRUE;

        if (!g_strcmp0 (keys[i], "location") ||
            !g_strcmp0 (keys[i], "duration")) {
            continue;
        }

        if (gtk_tree_model_get_iter_first (self->priv->store, &iter)) {
            do {
                gchar *key;

                gtk_tree_model_get (self->priv->store, &iter, 0, &key, -1);

                if (!g_strcmp0 (key, keys[i])) {
                    g_free (key);

                    gchar *val;
                    gtk_tree_model_get (self->priv->store, &iter, 1, &val, -1);
                    if (g_strcmp0 (val, vals[i])) {
                        gtk_list_store_set (GTK_LIST_STORE (self->priv->store), &iter,
                            1, "(Different for multiple entries)", -1);
                    }

                    g_free (val);

                    isnew = FALSE;
                    break;
                }

                g_free (key);
            } while (gtk_tree_model_iter_next (self->priv->store, &iter));
        }

        if (isnew) {
            gtk_list_store_append (GTK_LIST_STORE (self->priv->store), &iter);
            gtk_list_store_set (GTK_LIST_STORE (self->priv->store), &iter,
                0, keys[i], 1, vals[i], -1);
        }
    }
}

gboolean
tag_dialog_show (TagDialog *self)
{
    gtk_widget_show_all (self->priv->dialog);
}

gboolean
tag_dialog_close (TagDialog *self)
{
    gtk_widget_hide (self->priv->dialog);
    g_object_unref (self->priv->dialog);
}

static void
on_response (TagDialog *self, gint response_id, GtkDialog *dialog)
{
    switch (response_id) {
        case GTK_RESPONSE_DELETE_EVENT:
        case GTK_RESPONSE_REJECT:
            g_signal_emit (self, signal_completed, 0, NULL);
            break;
        case GTK_RESPONSE_ACCEPT:
            g_signal_emit (self, signal_completed, 0, self->priv->changes);
            break;
    };

    gtk_widget_hide (self->priv->dialog);
}

static void
on_add (TagDialog *self, GtkWidget *btn)
{
    GtkTreeIter iter;
    GtkTreePath *path;

    gtk_list_store_append (GTK_LIST_STORE (self->priv->store), &iter);

    path = gtk_tree_model_get_path (self->priv->store, &iter);

    gtk_tree_view_set_cursor (GTK_TREE_VIEW (self->priv->view), path,
        gtk_tree_view_get_column (GTK_TREE_VIEW (self->priv->view), 0), TRUE);

    gtk_tree_path_free (path);
}

static void
on_remove (TagDialog *self, GtkWidget *btn)
{
    GtkTreeIter iter;
    GtkTreePath *path;

    gtk_tree_view_get_cursor (GTK_TREE_VIEW (self->priv->view), &path, NULL);

    if (gtk_tree_model_get_iter (self->priv->store, &iter, path)) {
        gchar *key;

        gtk_tree_model_get (self->priv->store, &iter, 0, &key, -1);

        gtk_list_store_remove (GTK_LIST_STORE (self->priv->store), &iter);

        if (key && g_strcmp0 ("", key)) {
            gint i;
            for (i = 0; i < self->priv->changes->len; i += 2) {
                if (!g_strcmp0 (key, self->priv->changes->pdata[i])) {
                    if (self->priv->changes->pdata[i+1]) {
                        g_free (self->priv->changes->pdata[i+1]);
                        self->priv->changes->pdata[i+1] = NULL;
                    }
                    break;
                }
            }

            if (i >= self->priv->changes->len) {
                g_ptr_array_add (self->priv->changes, key);
                g_ptr_array_add (self->priv->changes, NULL);
            }
        }
    }

    if (path) {
        gtk_tree_path_free (path);
    }
}

static void
on_edit_key (TagDialog *self,
             gchar *path,
             gchar *new_key,
             GtkCellRendererText *renderer)
{
    GtkTreeIter iter;

    if (!g_strcmp0 (new_key, "")) {
        return;
    }

    if (gtk_tree_model_get_iter_first (self->priv->store, &iter)) {
        do {
            gchar *key;

            gtk_tree_model_get (self->priv->store, &iter, 0, &key, -1);

            if (!g_strcmp0 (key, new_key)) {
                g_free (key);
                return;
            }

            g_free (key);
        } while (gtk_tree_model_iter_next (self->priv->store, &iter));
    }

    if (gtk_tree_model_get_iter_from_string (self->priv->store, &iter, path)) {
        gchar *old_key, *val;

        gtk_tree_model_get (self->priv->store, &iter, 0, &old_key, 1, &val, -1);

        if (!g_strcmp0 (old_key, new_key)) {
            g_free (old_key);
            g_free (val);
            return;
        }

        gtk_list_store_set (GTK_LIST_STORE (self->priv->store), &iter,
            0, new_key, -1);

        if (old_key) {
            gint i;
            for (i = 0; i < self->priv->changes->len; i += 2) {
                if (!g_strcmp0 (old_key, self->priv->changes->pdata[i])) {
                    if (self->priv->changes->pdata[i+1]) {
                        g_free (self->priv->changes->pdata[i+1]);
                        self->priv->changes->pdata[i+1] = NULL;
                    }
                    break;
                }
            }
        }

        g_ptr_array_add (self->priv->changes, g_strdup (new_key));

        if (val) {
            g_ptr_array_add (self->priv->changes, val);
        } else {
            g_ptr_array_add (self->priv->changes, g_strdup (""));
        }
    }
}

static void
on_edit_val (TagDialog *self,
             gchar *path,
             gchar *new_val,
             GtkCellRendererText *renderer)
{
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter_from_string (self->priv->store, &iter, path)) {
        gchar *key, *old_val;

        gtk_tree_model_get (self->priv->store, &iter, 0, &key, 1, &old_val, -1);

        if (!g_strcmp0 (old_val, new_val)) {
            g_free (key);
            g_free (old_val);
            return;
        }

        gtk_list_store_set (GTK_LIST_STORE (self->priv->store), &iter,
            1, new_val, -1);

        if (key) {
            gint i;
            for (i = 0; i < self->priv->changes->len; i += 2) {
                if (!g_strcmp0 (key, self->priv->changes->pdata[i])) {
                    if (self->priv->changes->pdata[i+1]) {
                        g_free (self->priv->changes->pdata[i+1]);
                        self->priv->changes->pdata[i+1] = g_strdup (new_val);
                        return;
                    }
                }
            }

            g_ptr_array_add (self->priv->changes, key);
            g_ptr_array_add (self->priv->changes, g_strdup (new_val));
        }
    }
}
