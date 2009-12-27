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
#include "device-ipod.h"
#include "ipod-store.h"
#include "shell.h"
#include "column-funcs.h"
#include "catagory-display.h"

static void device_init (DeviceInterface *iface);
G_DEFINE_TYPE_WITH_CODE (DeviceIPod, device_ipod, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (DEVICE_TYPE, device_init)
)

struct _DeviceIPodPrivate {
    Shell *shell;

    GMount *mount;

    GtkWidget *main_pane;
    GtkWidget *cat_display;

    IPodStore *audio, *movie, *podcast, *audiobook, *musicvideo, *tvshow;
    Browser *audiob, *movieb, *podcastb, *audiobookb, *musicvideob, *tvshowb;

    Itdb_iTunesDB *itdb;
};

static void device_ipod_build_main_view (DeviceIPod *self);

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

    if (self->priv->mount) {
        g_object_unref (self->priv->mount);
        self->priv->mount = NULL;
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

Device*
device_ipod_new (Shell *shell, GMount *mount)
{
    if (!mount) {
        return NULL;
    }

    DeviceIPod *self = g_object_new (DEVICE_IPOD_TYPE, NULL);

    self->priv->shell = g_object_ref (shell);
    self->priv->mount = g_object_ref (mount);

    GError *err = NULL;

    GFile *mproot = g_mount_get_root (mount);
    gchar *dev_root = g_file_get_path (mproot);
    g_object_unref (mproot);

    self->priv->itdb = itdb_parse (dev_root, &err);

    g_free (dev_root);

    if (err) {
        g_error_free (err);
        err = NULL;
    }

    if (!self->priv->itdb) {
        g_object_unref (self);
        return NULL;
    }

//    GtkWidget *widget = gtk_label_new (dev_root);
    device_ipod_build_main_view (self);

    gchar *name = g_mount_get_name (mount);
    shell_add_widget (shell, self->priv->main_pane, name, NULL);

    self->priv->audio = ipod_store_new (self->priv->itdb, ITDB_MEDIATYPE_AUDIO);
    self->priv->audiob = browser_new_with_model (shell, MEDIA_STORE (self->priv->audio));
    browser_add_column (self->priv->audiob, "Title", "title",
        TRUE, (GtkTreeCellDataFunc) str_column_func);
    browser_add_column (self->priv->audiob, "Duration", "duration",
        FALSE, (GtkTreeCellDataFunc) time_column_func);
    gchar *n2 = g_strdup_printf ("%s/Music", name);
    shell_add_widget (shell, GTK_WIDGET (self->priv->audiob), n2, NULL);
    g_free (n2);
    shell_register_track_source (shell, TRACK_SOURCE (self->priv->audiob));

    self->priv->movie = ipod_store_new (self->priv->itdb, ITDB_MEDIATYPE_MOVIE);
    self->priv->movieb = browser_new_with_model (shell, MEDIA_STORE (self->priv->movie));
    browser_add_column (self->priv->movieb, "Title", "title",
        TRUE, (GtkTreeCellDataFunc) str_column_func);
    browser_add_column (self->priv->movieb, "Duration", "duration",
        FALSE, (GtkTreeCellDataFunc) time_column_func);
    n2 = g_strdup_printf ("%s/Movies", name);
    shell_add_widget (shell, GTK_WIDGET (self->priv->movieb), n2, NULL);
    g_free (n2);
    shell_register_track_source (shell, TRACK_SOURCE (self->priv->movieb));

    self->priv->podcast = ipod_store_new (self->priv->itdb, ITDB_MEDIATYPE_PODCAST);
    self->priv->podcastb = browser_new_with_model (shell, MEDIA_STORE (self->priv->podcast));
    browser_add_column (self->priv->podcastb, "Title", "title",
        TRUE, (GtkTreeCellDataFunc) str_column_func);
    browser_add_column (self->priv->podcastb, "Duration", "duration",
        FALSE, (GtkTreeCellDataFunc) time_column_func);
    n2 = g_strdup_printf ("%s/Podcasts", name);
    shell_add_widget (shell, GTK_WIDGET (self->priv->podcastb), n2, NULL);
    g_free (n2);
    shell_register_track_source (shell, TRACK_SOURCE (self->priv->podcastb));

    self->priv->audiobook = ipod_store_new (self->priv->itdb, ITDB_MEDIATYPE_AUDIOBOOK);
    self->priv->audiobookb = browser_new_with_model (shell, MEDIA_STORE (self->priv->audiobook));
    browser_add_column (self->priv->audiobookb, "Title", "title",
        TRUE, (GtkTreeCellDataFunc) str_column_func);
    browser_add_column (self->priv->audiobookb, "Duration", "duration",
        FALSE, (GtkTreeCellDataFunc) time_column_func);
    n2 = g_strdup_printf ("%s/Audio Books", name);
    shell_add_widget (shell, GTK_WIDGET (self->priv->audiobookb), n2, NULL);
    g_free (n2);
    shell_register_track_source (shell, TRACK_SOURCE (self->priv->audiobookb));

    self->priv->musicvideo = ipod_store_new (self->priv->itdb, ITDB_MEDIATYPE_MUSICVIDEO);
    self->priv->musicvideob = browser_new_with_model (shell, MEDIA_STORE (self->priv->musicvideo));
    browser_add_column (self->priv->musicvideob, "Title", "title",
        TRUE, (GtkTreeCellDataFunc) str_column_func);
    browser_add_column (self->priv->musicvideob, "Duration", "duration",
        FALSE, (GtkTreeCellDataFunc) time_column_func);
    n2 = g_strdup_printf ("%s/Music Videos", name);
    shell_add_widget (shell, GTK_WIDGET (self->priv->musicvideob), n2, NULL);
    g_free (n2);
    shell_register_track_source (shell, TRACK_SOURCE (self->priv->musicvideob));

    self->priv->tvshow = ipod_store_new (self->priv->itdb, ITDB_MEDIATYPE_TVSHOW);
    self->priv->tvshowb = browser_new_with_model (shell, MEDIA_STORE (self->priv->tvshow));
    browser_add_column (self->priv->tvshowb, "Title", "title",
        TRUE, (GtkTreeCellDataFunc) str_column_func);
    browser_add_column (self->priv->tvshowb, "Duration", "duration",
        FALSE, (GtkTreeCellDataFunc) time_column_func);
    n2 = g_strdup_printf ("%s/TV Shows", name);
    shell_add_widget (shell, GTK_WIDGET (self->priv->tvshowb), n2, NULL);
    g_free (n2);
    shell_register_track_source (shell, TRACK_SOURCE (self->priv->tvshowb));

    g_free (name);

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
    return g_mount_get_name (DEVICE_IPOD (self)->priv->mount);
}

static void
device_ipod_build_main_view (DeviceIPod *self)
{
    gchar *name = g_mount_get_name (self->priv->mount);
    GFile *file = g_mount_get_root (self->priv->mount);

    GFileInfo *fsize = g_file_query_filesystem_info (file, G_FILE_ATTRIBUTE_FILESYSTEM_SIZE, NULL, NULL);
    GFileInfo *ffree = g_file_query_filesystem_info (file, G_FILE_ATTRIBUTE_FILESYSTEM_FREE, NULL, NULL);

    guint64 fgsize = g_file_info_get_attribute_uint64 (fsize, G_FILE_ATTRIBUTE_FILESYSTEM_SIZE);
    guint64 fgfree = g_file_info_get_attribute_uint64 (ffree, G_FILE_ATTRIBUTE_FILESYSTEM_FREE);

    guint64 audiosize = 0, moviesize = 0, podcastsize = 0, audiobooksize = 0, musicvideosize = 0, tvshowsize = 0;
    guint64 datasize = 0;
    guint64 totsize = fgsize;

    GList *iter;
    for (iter = self->priv->itdb->tracks; iter; iter = iter->next) {
        Itdb_Track *track = (Itdb_Track*) iter->data;

        switch (track->mediatype) {
            case ITDB_MEDIATYPE_AUDIO: audiosize += track->size; break;
            case ITDB_MEDIATYPE_MOVIE: moviesize += track->size; break;
            case ITDB_MEDIATYPE_PODCAST: podcastsize += track->size; break;
            case ITDB_MEDIATYPE_AUDIOBOOK: audiobooksize += track->size; break;
            case ITDB_MEDIATYPE_MUSICVIDEO: musicvideosize += track->size; break;
            case ITDB_MEDIATYPE_TVSHOW: tvshowsize += track->size; break;
        };
    }

    datasize = fgsize - fgfree - audiosize - moviesize - podcastsize - audiobooksize - musicvideosize - tvshowsize;

    fgsize /= (1024 * 1024);
    fgfree /= (1024 * 1024);

    const Itdb_IpodInfo *info = itdb_device_get_ipod_info (self->priv->itdb->device);

    gchar *info_str = g_strdup_printf ("%s\nCapacity: %.2fGB\nGeneration: %s\nModel: %s\nFree: %d.%02d GB / %d.%02d GB"
        "\nAudio: %ld Bytes (%.2f%%)\n"
        "Movies: %ld Bytes (%.2f%%)\n"
        "Podcasts: %ld Bytes (%.2f%%)\n"
        "AudioBooks: %ld Bytes (%.2f%%)\n"
        "Music Videos: %ld Bytes (%.2f%%)\n"
        "TV Shows: %ld Bytes (%.2f%%)\n"
        "Extra Data: %ld Bytes (%.2f%%)",
        name, info->capacity, itdb_info_get_ipod_generation_string (info->ipod_generation),
        itdb_info_get_ipod_model_name_string (info->ipod_model),
        fgfree / 1024, (fgfree % 1024) / 10, fgsize / 1024, (fgsize % 1024) / 10,
        audiosize, 100.0 * audiosize / totsize,
        moviesize, 100.0 * moviesize / totsize,
        podcastsize, 100.0 * podcastsize / totsize,
        audiobooksize, 100.0 * audiobooksize / totsize,
        musicvideosize, 100.0 * musicvideosize / totsize,
        tvshowsize, 100.0 * tvshowsize / totsize,
        datasize, 100.0 * datasize / totsize);
    g_free (name);

    g_object_unref (fsize);
    g_object_unref (ffree);

    GtkWidget *vbox = gtk_vbox_new (TRUE, 0);

    gtk_box_pack_start (GTK_BOX (vbox), gtk_label_new (info_str), FALSE, FALSE, 0);

    self->priv->cat_display = catagory_display_new ();

    catagory_display_set_capacity (CATAGORY_DISPLAY (self->priv->cat_display), totsize);
    catagory_display_set_catagory (CATAGORY_DISPLAY (self->priv->cat_display), "Music", audiosize);
    catagory_display_set_catagory (CATAGORY_DISPLAY (self->priv->cat_display), "Movies", moviesize);
    catagory_display_set_catagory (CATAGORY_DISPLAY (self->priv->cat_display), "Podcasts", podcastsize);
    catagory_display_set_catagory (CATAGORY_DISPLAY (self->priv->cat_display), "Audio Books", audiobooksize);
    catagory_display_set_catagory (CATAGORY_DISPLAY (self->priv->cat_display), "Music Videos", musicvideosize);
    catagory_display_set_catagory (CATAGORY_DISPLAY (self->priv->cat_display), "TV Shows", tvshowsize);
    catagory_display_set_catagory (CATAGORY_DISPLAY (self->priv->cat_display), "Extra Data", datasize);

    gtk_box_pack_start (GTK_BOX (vbox), self->priv->cat_display, TRUE, TRUE, 0);

    gtk_widget_show_all (vbox);

    self->priv->main_pane = vbox;
    g_free (info_str);
}
