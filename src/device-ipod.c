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

#include "device-ipod.h"
#include "device.h"
#include "shell.h"

static void device_init (DeviceInterface *iface);
G_DEFINE_TYPE_WITH_CODE (DeviceIPod, device_ipod, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (DEVICE_TYPE, device_init)
)

struct _DeviceIPodPrivate {
    Shell *shell;

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

    g_thread_create ((GThreadFunc) device_ipod_initial_import, self, TRUE, NULL);

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

void
device_ipod_initial_import (DeviceIPod *self)
{
    GList *iter;

    g_print ("List of tracks\n");

    for (iter = self->priv->itdb->tracks; iter; iter = iter->next) {
        Itdb_Track *track = (Itdb_Track*) iter->data;

        if (track) {
            g_print ("%s by %s from %s of type %d\n", track->title,
                track->artist, track->album, track->mediatype);
        }
    }

    g_print ("List done\n");
}
