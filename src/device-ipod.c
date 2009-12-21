/*
 *      device-ipod.c
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
#include <gpod/itdb.h>

#include "browser.h"
#include "device.h"
#include "device-ipod.h"
#include "ipod-store.h"
#include "shell.h"

static void device_init (DeviceInterface *iface);
G_DEFINE_TYPE_WITH_CODE (DeviceIPod, device_ipod, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (DEVICE_TYPE, device_init)
)

struct _DeviceIPodPrivate {
    Shell *shell;

    IPodStore *audio, *movie, *podcast, *audiobook, *musicvideo, *tvshow;
    Browser *audiob, *movieb, *podcastb, *audiobookb, *musicvideob, *tvshowb;

    Itdb_iTunesDB *itdb;
};

// Device interface methods
void device_ipod_add_entry (Device *self, gchar **entry);
void device_ipod_remove_entry (Device *self, Entry *entry);
gchar *device_ipod_get_name (Device *self);

// Internal methods to DeviceIPod
void device_ipod_initial_import (DeviceIPod *self);

static void
device_ipod_finalize (GObject *object)
{
    DeviceIPod *self = DEVICE_IPOD (object);

    if (self->priv->shell) {
        g_object_unref (self->priv->shell);
        self->priv->shell = NULL;
    }

    if (self->priv->itdb) {
        itdb_free (self->priv->itdb);
        self->priv->itdb = NULL;
    }

    G_OBJECT_CLASS (device_ipod_parent_class)->finalize (object);
}

static void
device_init (DeviceInterface *iface)
{
    iface->add_entry = device_ipod_add_entry;
    iface->rem_entry = device_ipod_remove_entry;
    iface->get_name = device_ipod_get_name;
}

static void
device_ipod_class_init (DeviceIPodClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (DeviceIPodPrivate));

    object_class->finalize = device_ipod_finalize;
}

static void
device_ipod_init (DeviceIPod *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), DEVICE_IPOD_TYPE, DeviceIPodPrivate);
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
        g_object_unref (entry);
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
        g_object_unref (entry);
    } else {
        g_object_set (G_OBJECT (cell), "text", "", NULL);
    }
}

Device*
device_ipod_new (Shell *shell, const gchar *dev_root)
{
    DeviceIPod *self = g_object_new (DEVICE_IPOD_TYPE, NULL);

    self->priv->shell = g_object_ref (shell);

    GError *err = NULL;
    self->priv->itdb = itdb_parse (dev_root, &err);

    if (err) {
        g_error_free (err);
        err = NULL;
    }

    if (!self->priv->itdb) {
        g_object_unref (self);
        return NULL;
    }

    GtkWidget *widget = gtk_label_new (dev_root);
    shell_add_widget (shell, widget, "IPOD", NULL);

    self->priv->audio = ipod_store_new (self->priv->itdb, ITDB_MEDIATYPE_AUDIO);
    self->priv->audiob = browser_new_with_model (shell, MEDIA_STORE (self->priv->audio));
    browser_add_column (self->priv->audiob, "Title", "title",
        TRUE, (GtkTreeCellDataFunc) str_column_func);
    browser_add_column (self->priv->audiob, "Duration", "duration",
        FALSE, (GtkTreeCellDataFunc) time_column_func);
    shell_add_widget (shell, GTK_WIDGET (self->priv->audiob), "IPOD/Music", NULL);
    shell_register_track_source (shell, TRACK_SOURCE (self->priv->audiob));

    self->priv->movie = ipod_store_new (self->priv->itdb, ITDB_MEDIATYPE_MOVIE);
    self->priv->movieb = browser_new_with_model (shell, MEDIA_STORE (self->priv->movie));
    browser_add_column (self->priv->movieb, "Title", "title",
        TRUE, (GtkTreeCellDataFunc) str_column_func);
    browser_add_column (self->priv->movieb, "Duration", "duration",
        FALSE, (GtkTreeCellDataFunc) time_column_func);
    shell_add_widget (shell, GTK_WIDGET (self->priv->movieb), "IPOD/Movies", NULL);
    shell_register_track_source (shell, TRACK_SOURCE (self->priv->movieb));

    self->priv->podcast = ipod_store_new (self->priv->itdb, ITDB_MEDIATYPE_PODCAST);
    self->priv->podcastb = browser_new_with_model (shell, MEDIA_STORE (self->priv->podcast));
    browser_add_column (self->priv->podcastb, "Title", "title",
        TRUE, (GtkTreeCellDataFunc) str_column_func);
    browser_add_column (self->priv->podcastb, "Duration", "duration",
        FALSE, (GtkTreeCellDataFunc) time_column_func);
    shell_add_widget (shell, GTK_WIDGET (self->priv->podcastb), "IPOD/Podcasts", NULL);
    shell_register_track_source (shell, TRACK_SOURCE (self->priv->podcastb));

    self->priv->audiobook = ipod_store_new (self->priv->itdb, ITDB_MEDIATYPE_AUDIOBOOK);
    self->priv->audiobookb = browser_new_with_model (shell, MEDIA_STORE (self->priv->audiobook));
    browser_add_column (self->priv->audiobookb, "Title", "title",
        TRUE, (GtkTreeCellDataFunc) str_column_func);
    browser_add_column (self->priv->audiobookb, "Duration", "duration",
        FALSE, (GtkTreeCellDataFunc) time_column_func);
    shell_add_widget (shell, GTK_WIDGET (self->priv->audiobookb), "IPOD/Audio Books", NULL);
    shell_register_track_source (shell, TRACK_SOURCE (self->priv->audiobookb));

    self->priv->musicvideo = ipod_store_new (self->priv->itdb, ITDB_MEDIATYPE_MUSICVIDEO);
    self->priv->musicvideob = browser_new_with_model (shell, MEDIA_STORE (self->priv->musicvideo));
    browser_add_column (self->priv->musicvideob, "Title", "title",
        TRUE, (GtkTreeCellDataFunc) str_column_func);
    browser_add_column (self->priv->musicvideob, "Duration", "duration",
        FALSE, (GtkTreeCellDataFunc) time_column_func);
    shell_add_widget (shell, GTK_WIDGET (self->priv->musicvideob), "IPOD/Music Videos", NULL);
    shell_register_track_source (shell, TRACK_SOURCE (self->priv->musicvideob));

    self->priv->tvshow = ipod_store_new (self->priv->itdb, ITDB_MEDIATYPE_TVSHOW);
    self->priv->tvshowb = browser_new_with_model (shell, MEDIA_STORE (self->priv->tvshow));
    browser_add_column (self->priv->tvshowb, "Title", "title",
        TRUE, (GtkTreeCellDataFunc) str_column_func);
    browser_add_column (self->priv->tvshowb, "Duration", "duration",
        FALSE, (GtkTreeCellDataFunc) time_column_func);
    shell_add_widget (shell, GTK_WIDGET (self->priv->tvshowb), "IPOD/TV Shows", NULL);
    shell_register_track_source (shell, TRACK_SOURCE (self->priv->tvshowb));

    return DEVICE (self);
}

void
device_ipod_add_entry (Device *self, gchar **entry)
{

}

void
device_ipod_remove_entry (Device *self, Entry *entry)
{

}

gchar*
device_ipod_get_name (Device *self)
{
    return "IPod";
}
