/*
 *      device-manager.c
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

#include "../config.h"

#include <gtk/gtk.h>
#include <gio/gio.h>

#include "device-manager.h"
#include "device.h"

#ifdef ENABLE_IPOD
#include "device-ipod.h"
#endif

#include "shell.h"

G_DEFINE_TYPE(DeviceManager, device_manager, G_TYPE_OBJECT)

struct _DeviceManagerPrivate {
    Shell *shell;

    GPtrArray *devices;

    GVolumeMonitor *monitor;
};

typedef Device* DeviceNewFunc (Shell*, GMount *mount);

DeviceNewFunc *dev_new_funcs[] = {
#ifdef ENABLE_IPOD
    device_ipod_new,
#endif
    NULL
};

static void on_mount_added (DeviceManager *self, GMount *mount, GVolumeMonitor *volume_monitor);
static void on_mount_pre_unmount (DeviceManager *self, GMount *mount, GVolumeMonitor *volume_monitor);
static void on_mount_removed (DeviceManager *self, GMount *mount, GVolumeMonitor *volume_monitor);

static void
device_manager_finalize (GObject *object)
{
    DeviceManager *self = DEVICE_MANAGER (object);

    if (self->priv->shell) {
        g_object_unref (self->priv->shell);
        self->priv->shell = NULL;
    }

    if (self->priv->devices) {
        g_ptr_array_foreach (self->priv->devices, (GFunc) g_object_unref, NULL);
        g_ptr_array_free (self->priv->devices, TRUE);
        self->priv->devices = NULL;
    }

    G_OBJECT_CLASS (device_manager_parent_class)->finalize (object);
}

static void
device_manager_class_init (DeviceManagerClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (DeviceManagerPrivate));

    object_class->finalize = device_manager_finalize;
}

static void
device_manager_init (DeviceManager *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), DEVICE_MANAGER_TYPE, DeviceManagerPrivate);

    self->priv->devices = g_ptr_array_new ();
}

DeviceManager*
device_manager_new (Shell *shell)
{
    DeviceManager *self = g_object_new (DEVICE_MANAGER_TYPE, NULL);

    self->priv->shell = g_object_ref (shell);

    self->priv->monitor = g_volume_monitor_get ();

    GList *mps, *iter;
    mps = g_volume_monitor_get_mounts (self->priv->monitor);

    for (iter = mps; iter; iter = iter->next) {
        GMount *mp = G_MOUNT (iter->data);

        on_mount_added (self, mp, self->priv->monitor);

        g_object_unref (mp);
    }

    g_list_free (mps);

    g_signal_connect_swapped (self->priv->monitor, "mount-added",
        G_CALLBACK (on_mount_added), self);
    g_signal_connect_swapped (self->priv->monitor, "mount-pre-unmount",
        G_CALLBACK (on_mount_pre_unmount), self);
    g_signal_connect_swapped (self->priv->monitor, "mount-removed",
        G_CALLBACK (on_mount_removed), self);

    g_print ("Device manager ready %x\n", self->priv->monitor);

    return self;
}

static void
on_mount_added (DeviceManager *self,
                GMount *mount,
                GVolumeMonitor *volume_monitor)
{
    gchar *name = g_mount_get_name (mount);
    g_print ("Mount Added %s\n", name);
    g_free (name);

    Device *new_dev = NULL;
    gint i;
    for (i = 0; dev_new_funcs[i] && !new_dev; i++) {
        new_dev = dev_new_funcs[i] (self->priv->shell, mount);
    }

    if (new_dev) {
        g_ptr_array_add (self->priv->devices, new_dev);
    }
}

static void
on_mount_pre_unmount (DeviceManager *self,
                      GMount *mount,
                      GVolumeMonitor *volume_monitor)
{
    gchar *name = g_mount_get_name (mount);
    g_print ("Pre-Unmount %s\n", name);
    g_free (name);
}

static void
on_mount_removed (DeviceManager *self,
                  GMount *mount,
                  GVolumeMonitor *volume_monitor)
{
    gchar *name = g_mount_get_name (mount);
    g_print ("Mount Removed %s\n", name);
    g_free (name);
}
